#include "presentation/ApplicationController.h"
#include "presentation/DiagnosticsModel.h"
#include "presentation/LedParameterModel.h"
#include "utils/AppPaths.h"
#include "utils/LogSetup.h"

#include "ThemeSetup.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStringList>
#include <QTimer>

#include <cstdio>

#ifndef CAONGAN_LED_VERSION
#define CAONGAN_LED_VERSION "0.1.0"
#endif

namespace {

void loadApplicationFonts()
{
    const QStringList files = {
        QStringLiteral("Inter-Regular.otf"),
        QStringLiteral("Inter-Medium.otf"),
        QStringLiteral("Inter-SemiBold.otf"),
        QStringLiteral("Inter-Bold.otf"),
        QStringLiteral("Inter-Italic.otf"),
        QStringLiteral("Inter-BoldItalic.otf"),
    };

    bool loadedAny = false;
    for (const QString &file : files) {
        const int id = QFontDatabase::addApplicationFont(
            QStringLiteral(":/fonts/Inter/%1").arg(file));
        if (id < 0) {
            qWarning() << "Không nạp được font:" << file;
            continue;
        }
        loadedAny = true;
    }

    if (loadedAny) {
        QFont font(QStringLiteral("Inter"));
        font.setPointSize(10);
        QGuiApplication::setFont(font);
    }
}

QString absoluteConfigPath(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return {};
    return QFileInfo(trimmed).absoluteFilePath();
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("CaoNgan"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("caongan.local"));
    QCoreApplication::setApplicationName(QStringLiteral("CaonganLed"));
    QCoreApplication::setApplicationVersion(QStringLiteral(CAONGAN_LED_VERSION));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Cao Ngạn LED"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Màn LED quan trắc khí thải và nước thải — đọc file TT10 chỉ đọc."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption configOption(
        QStringList{QStringLiteral("c"), QStringLiteral("config")},
        QStringLiteral("Đường dẫn file cấu hình JSON (mặc định dưới AppLocalData)."),
        QStringLiteral("path"));
    QCommandLineOption previewOption(
        QStringList{QStringLiteral("preview")},
        QStringLiteral("Mở cửa sổ LED dạng cửa sổ để kiểm tra (không cần màn LED thật)."));
    QCommandLineOption simulateOption(
        QStringList{QStringLiteral("simulate")},
        QStringLiteral("Bật dữ liệu mô phỏng khi khởi động; không ghi vào cấu hình."));
    QCommandLineOption scenarioOption(
        QStringList{QStringLiteral("scenario")},
        QStringLiteral("Tên cảnh mô phỏng dùng với --simulate."),
        QStringLiteral("name"));
    QCommandLineOption verifyWindowsOption(
        QStringList{QStringLiteral("verify-preview-windows")},
        QStringLiteral("Kiểm tra hai cửa sổ preview thực sự hiển thị, rồi thoát (dành cho test)."));
    parser.addOption(configOption);
    parser.addOption(previewOption);
    parser.addOption(simulateOption);
    parser.addOption(scenarioOption);
    parser.addOption(verifyWindowsOption);
    parser.process(app);

    if (parser.isSet(verifyWindowsOption) && !parser.isSet(previewOption)) {
        std::fprintf(stderr, "--verify-preview-windows yêu cầu --preview.\n");
        return 2;
    }

    caongan::AppPaths::ensureDirectories();
    caongan::setupLogging(caongan::AppPaths::logDir());

    qInfo() << "Khởi động Cao Ngạn LED" << QCoreApplication::applicationVersion();

    // Một instance cho một vùng dữ liệu. Không dùng khóa theo CWD để tránh hai
    // bản chạy ghi cùng cấu hình/snapshot nhưng khác shortcut.
    QLockFile instanceLock(caongan::AppPaths::dataDir()
                           + QStringLiteral("/caongan-led.lock"));
    instanceLock.setStaleLockTime(30000);
    if (!instanceLock.tryLock(250)) {
        qCritical() << "Đã có một instance Cao Ngạn LED đang chạy (hoặc lock còn sót):"
                    << instanceLock.fileName();
        std::fprintf(stderr, "Cao Ngạn LED đang chạy ở một tiến trình khác.\n");
        return 2;
    }

    // Theme Material phải được đặt trước khi tạo QML engine. Không dùng
    // LoggerKit.Components nên không cần QtGraphs.
    LoggerKit::Theme::applyQuickControlsStyle();

    loadApplicationFonts();

    caongan::LedParameterModel ledModel;
    caongan::DiagnosticsModel diagnosticsModel;
    caongan::ApplicationController controller(&ledModel, &diagnosticsModel);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("ledParameterModel"), &ledModel);
    engine.rootContext()->setContextProperty(QStringLiteral("diagnosticsModel"), &diagnosticsModel);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() {
                         qCritical() << "Không tạo được QML root object";
                         QCoreApplication::exit(1);
                     }, Qt::QueuedConnection);
    QObject::connect(&app, &QCoreApplication::aboutToQuit,
                     &controller, &caongan::ApplicationController::shutdown);

    controller.initialize(absoluteConfigPath(parser.value(configOption)),
                          parser.isSet(previewOption),
                          parser.isSet(simulateOption),
                          parser.value(scenarioOption));

    engine.loadFromModule(QStringLiteral("CaonganLed.App"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Không nạp được QML module CaonganLed.App/Main";
        return 1;
    }

    if (parser.isSet(verifyWindowsOption)) {
        QTimer::singleShot(500, &app, []() {
            bool ledVisible = false;
            bool operatorVisible = false;
            for (const QWindow *window : QGuiApplication::allWindows()) {
                if (window->objectName() == QLatin1String("ledWindow"))
                    ledVisible = window->isVisible();
                else if (window->objectName() == QLatin1String("operatorWindow"))
                    operatorVisible = window->isVisible();
            }
            if (!ledVisible || !operatorVisible) {
                qCritical() << "Kiểm tra preview thất bại: LED hiển thị =" << ledVisible
                            << ", vận hành hiển thị =" << operatorVisible;
                QCoreApplication::exit(3);
                return;
            }
            qInfo() << "PREVIEW_WINDOWS_OK";
            QCoreApplication::exit(0);
        });
    }

    qInfo() << "Sẵn sàng; config =" << controller.configPath();
    const int exitCode = app.exec();
    qInfo() << "Thoát Cao Ngạn LED, mã" << exitCode;
    return exitCode;
}
