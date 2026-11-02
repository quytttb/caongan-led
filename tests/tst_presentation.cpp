#include "domain/ParameterCatalog.h"
#include "presentation/ApplicationController.h"
#include "presentation/DiagnosticsModel.h"
#include "presentation/LedParameterModel.h"
#include "presentation/SimulationFactory.h"
#include "utils/AppPaths.h"

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QTemporaryDir>
#include <QTest>
#include <QWindow>

using namespace caongan;

class TestPresentation : public QObject
{
    Q_OBJECT
private slots:
    void simulationStates();
    void screenPlacement();
    void previewNeedsNoTarget();
    void logViewCopyAndExport();
};

void TestPresentation::simulationStates()
{
    const AppConfig config = defaultAppConfig();
    const auto stateFor = [&](const QString &scenario, const QString &id) {
        const auto states = buildSimulationStates(config, scenario, QDateTime::currentDateTimeUtc());
        for (const ChannelState &state : states) {
            if (state.config.id == id)
                return state;
        }
        return ChannelState{};
    };
    QCOMPARE(buildSimulationStates(config, QStringLiteral("Bình thường"), QDateTime::currentDateTimeUtc()).size(), 12);
    QCOMPARE(stateFor(QStringLiteral("Vượt ngưỡng"), QStringLiteral("so2")).display, DisplayState::ExceedsLimit);
    QCOMPARE(stateFor(QStringLiteral("Hiệu chuẩn"), QStringLiteral("o2")).display, DisplayState::Calibrating);
    QCOMPARE(stateFor(QStringLiteral("Lỗi thiết bị"), QStringLiteral("cod")).display, DisplayState::DeviceFault);
    QCOMPARE(stateFor(QStringLiteral("Bảo trì"), QStringLiteral("cod")).display, DisplayState::Maintenance);
    LedParameterModel model;
    model.setStates(buildSimulationStates(config, QStringLiteral("Bảo trì"), QDateTime::currentDateTimeUtc()));
    bool foundMaintenance = false;
    for (int row = 0; row < model.rowCount(); ++row) {
        const QModelIndex index = model.index(row, 0);
        if (model.data(index, LedParameterModel::ChannelIdRole).toString() == QLatin1String("cod")) {
            QCOMPARE(model.data(index, LedParameterModel::StatusTextRole).toString(), QStringLiteral("BẢO TRÌ"));
            QCOMPARE(model.data(index, LedParameterModel::StateKeyRole).toString(), QStringLiteral("maintenance"));
            foundMaintenance = true;
        }
    }
    QVERIFY(foundMaintenance);
    QCOMPARE(stateFor(QStringLiteral("Dữ liệu cũ"), QStringLiteral("o2")).display, DisplayState::Stale);
    QCOMPARE(stateFor(QStringLiteral("Thiếu pH"), QStringLiteral("ph")).display, DisplayState::NoData);
    QCOMPARE(stateFor(QStringLiteral("Sai đơn vị"), QStringLiteral("pm")).display, DisplayState::UnitMismatch);
    QCOMPARE(stateFor(QStringLiteral("Xung đột"), QStringLiteral("so2")).display, DisplayState::Conflict);
    QCOMPARE(stateFor(QStringLiteral("Flow 6 chữ số"), QStringLiteral("flow_flue")).valueText, QStringLiteral("538964"));
    QVERIFY(!config.channelById(QStringLiteral("so2"))->threshold.enabled);
}

void TestPresentation::screenPlacement()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());
    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    controller.initialize(temp.filePath("config.json"), false, false, {});
    QWindow window;
    QVERIFY(!controller.positionLedWindow(nullptr));
    QVERIFY(!controller.positionLedWindow(&window));
    QVERIFY(!controller.ledWindowVisible());
    QVERIFY(!controller.screenWarning().isEmpty());

    QScreen *screen = nullptr;
    for (QScreen *candidate : QGuiApplication::screens()) {
        if (candidate->name() == QStringLiteral("Test LED"))
            screen = candidate;
    }
    QVERIFY(screen);
    QVERIFY(screen != QGuiApplication::primaryScreen());
    QVERIFY(!controller.updateDisplay(QStringLiteral("Duplicate"), false).isEmpty());
    QVERIFY(!controller.ledWindowVisible());
    QCOMPARE(controller.updateDisplay(screen->name(), false), QString());
    QVERIFY(controller.ledWindowVisible());
    QVERIFY(controller.positionLedWindow(&window));
    QCOMPARE(window.screen(), screen);
    QCOMPARE(window.position(), screen->geometry().topLeft());
    QCOMPARE(controller.updateDisplay(QString(), false), QString());
    QVERIFY(!controller.ledWindowVisible());
    QVERIFY(!controller.positionLedWindow(&window));
    controller.shutdown();
}

void TestPresentation::previewNeedsNoTarget()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());
    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    controller.initialize(temp.filePath("config.json"), true, false, {});
    QWindow window;
    QVERIFY(controller.ledWindowVisible());
    QVERIFY(controller.positionLedWindow(&window));
    controller.shutdown();
}

void TestPresentation::logViewCopyAndExport()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());
    AppPaths::ensureDirectories();
    const QString name = QStringLiteral("caongan-led-20260101.log");
    QFile source(QDir(AppPaths::logDir()).filePath(name));
    QVERIFY(source.open(QIODevice::WriteOnly));
    const QByteArray log = QByteArrayLiteral("[INFO] Bắt đầu\n[WARN] Kiểm tra nguồn\n");
    QCOMPARE(source.write(log), log.size());
    source.close();

    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    QCOMPARE(controller.refreshLogFiles(), QString());
    QVERIFY(controller.logFiles().contains(name));
    QCOMPARE(controller.selectLog(name), QString());
    QCOMPARE(controller.logContent(), QString::fromUtf8(log));
    QVERIFY(!controller.selectLog(QStringLiteral("../config/caongan-led.json")).isEmpty());
    QCOMPARE(controller.copyLog(), QString());
    QCOMPARE(QGuiApplication::clipboard()->text(), QString::fromUtf8(log));

    QVERIFY(!controller.exportLog(QUrl::fromLocalFile(source.fileName())).isEmpty());
    QVERIFY(!controller.exportLog(QUrl::fromLocalFile(QDir(AppPaths::dataDir()).filePath("snapshot.db"))).isEmpty());
    const QString exportedPath = temp.filePath("export.log");
    QCOMPARE(controller.exportLog(QUrl::fromLocalFile(exportedPath)), QString());
    QFile exported(exportedPath);
    QVERIFY(exported.open(QIODevice::ReadOnly));
    QCOMPARE(exported.readAll(), log);

    const QString largeName = QStringLiteral("caongan-led-20260102.log.1");
    QFile large(QDir(AppPaths::logDir()).filePath(largeName));
    QVERIFY(large.open(QIODevice::WriteOnly));
    const QByteArray largeLog = QByteArray(1024 * 1024 + 100, 'x') + "\nCUỐI\n";
    QCOMPARE(large.write(largeLog), largeLog.size());
    large.close();
    QCOMPARE(controller.refreshLogFiles(), QString());
    QCOMPARE(controller.selectLog(largeName), QString());
    QVERIFY(controller.logContent().contains(QStringLiteral("CUỐI")));
    QVERIFY(controller.logContent().size() < largeLog.size());
    QVERIFY(controller.logViewNote().contains(QStringLiteral("1 MiB")));
    const QString largeExport = temp.filePath("large.log");
    QCOMPARE(controller.exportLog(QUrl::fromLocalFile(largeExport)), QString());
    QFile largeCopy(largeExport);
    QVERIFY(largeCopy.open(QIODevice::ReadOnly));
    QCOMPARE(largeCopy.readAll(), largeLog);

    // Cấu hình --config có thể nằm ngoài thư mục config mặc định hoặc qua symlink.
    const QString customDir = temp.filePath("custom-config");
    QVERIFY(QDir().mkpath(customDir));
    QString configPath = customDir + QStringLiteral("/active.json");
#ifdef Q_OS_UNIX
    const QString aliasDir = temp.filePath("config-link");
    QVERIFY(QFile::link(customDir, aliasDir));
    configPath = aliasDir + QStringLiteral("/active.json");
#endif
    LedParameterModel configuredLed;
    DiagnosticsModel configuredDiagnostics;
    ApplicationController configured(&configuredLed, &configuredDiagnostics);
    configured.initialize(configPath, true, false, {});
    QCOMPARE(configured.refreshLogFiles(), QString());
    QVERIFY(!configured.exportLog(QUrl::fromLocalFile(customDir + QStringLiteral("/active.json"))).isEmpty());
    QVERIFY(!configured.exportLog(QUrl::fromLocalFile(customDir + QStringLiteral("/active.json.backup"))).isEmpty());
    configured.shutdown();
}

QTEST_MAIN(TestPresentation)
#include "tst_presentation.moc"
