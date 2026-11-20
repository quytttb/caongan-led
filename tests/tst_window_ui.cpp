#include "domain/ParameterCatalog.h"
#include "presentation/ApplicationController.h"
#include "presentation/DiagnosticsModel.h"
#include "presentation/LedParameterModel.h"
#include "storage/ConfigStore.h"
#include "utils/AppPaths.h"

#include <QGuiApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

Q_IMPORT_QML_PLUGIN(LoggerKit_ThemePlugin)

using namespace caongan;

static QQuickItem *findVisualItem(QQuickItem *parent, const QString &name)
{
    if (!parent)
        return nullptr;
    if (parent->objectName() == name)
        return parent;
    for (QQuickItem *child : parent->childItems()) {
        if (auto *found = findVisualItem(child, name))
            return found;
    }
    return nullptr;
}

class TestWindowUi : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void ledAlwaysFullScreen();
    void operatorChrome();
    void cancelExitKeepsLedVisible();
    void thresholdEditor();
    void sourceDraftsAndDisplayReassignment();
};

void TestWindowUi::initTestCase()
{
    QVERIFY(QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Inter/Inter-Regular.otf")) >= 0);
    QVERIFY(QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Inter/Inter-Bold.otf")) >= 0);
}

void TestWindowUi::ledAlwaysFullScreen()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());

    // Cấu hình cũ có fullScreen=false vẫn phải đọc được nhưng không được
    // khiến LED hiện title bar hoặc ra khỏi chế độ toàn màn hình.
    AppConfig config = defaultAppConfig();
    config.branding.branchName = QStringLiteral("Chi nhánh tổng công ty điện lực TKV - CTCP");
    config.branding.address =
        QStringLiteral("Ngõ 517 đường Dương Tự Minh, Phường Quan Triều, tỉnh Thái Nguyên");
    config.display.targetScreenName = QStringLiteral("B Test LED");
    config.display.fullScreen = false;
    const QString configPath = temp.filePath(QStringLiteral("config.json"));
    ConfigStore store(configPath);
    QString error;
    QVERIFY2(store.save(config, &error), qPrintable(error));

    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    controller.initialize(configPath);
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("ledParameterModel"), &led);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(CAONGAN_QML_SOURCE_DIR "/LedWindow.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto *window = qobject_cast<QQuickWindow *>(object.get());
    QVERIFY(window);
    QTRY_COMPARE(window->visibility(), QWindow::FullScreen);
    QVERIFY(window->flags().testFlag(Qt::FramelessWindowHint));
    QVERIFY(window->screen());
    QCOMPARE(window->screen()->name(), QStringLiteral("B Test LED"));
    QCOMPARE(window->geometry(), window->screen()->geometry());

    auto *board = findVisualItem(window->contentItem(), QStringLiteral("ledBoard"));
    QVERIFY(board);
    QCOMPARE(board->width(), 1600.0);
    QCOMPARE(board->height(), 960.0);
    auto *branch = findVisualItem(board, QStringLiteral("branchName"));
    auto *company = findVisualItem(board, QStringLiteral("companyName"));
    auto *address = findVisualItem(board, QStringLiteral("companyAddress"));
    QVERIFY(branch);
    QVERIFY(company);
    QVERIFY(address);
    QCOMPARE(branch->property("text").toString(),
             QStringLiteral("CHI NHÁNH TỔNG CÔNG TY ĐIỆN LỰC TKV - CTCP"));
    QVERIFY(branch->property("font").value<QFont>().bold());
    QCOMPARE(company->property("text").toString(),
             QStringLiteral("CÔNG TY NHIỆT ĐIỆN CAO NGẠN - TKV"));
    QCOMPARE(address->property("text").toString(),
             QStringLiteral("Ngõ 719 đường Dương Tự Minh, Phường Quan Triều, tỉnh Thái Nguyên"));
    QVERIFY2(branch->y() + branch->height() <= company->y(),
             qPrintable(QStringLiteral("Chi nhánh: y=%1 h=%2, công ty: y=%3 h=%4")
                            .arg(branch->y()).arg(branch->height()).arg(company->y()).arg(company->height())));
    QVERIFY2(company->y() + company->height() <= address->y(),
             qPrintable(QStringLiteral("Công ty: y=%1 h=%2, địa chỉ: y=%3 h=%4")
                            .arg(company->y()).arg(company->height()).arg(address->y()).arg(address->height())));
    QVERIFY(address->y() + address->height() <= address->parentItem()->height());
    QVERIFY(!branch->property("truncated").toBool());
    QVERIFY(!company->property("truncated").toBool());
    QVERIFY(!address->property("truncated").toBool());
    QCOMPARE(address->property("font").value<QFont>().pixelSize(), 22);
    for (const QString &name : {QStringLiteral("exhaustCaption"), QStringLiteral("wastewaterCaption")}) {
        auto *caption = findVisualItem(board, name);
        QVERIFY(caption);
        QCOMPARE(caption->property("font").value<QFont>().pixelSize(), 30);
        QVERIFY(caption->setProperty(
            "text", QStringLiteral("Mốc đo không đồng bộ: 00:00:00 – 23:59:59 31/12/9999")));
        QVERIFY2(!caption->property("truncated").toBool(), qPrintable(name));
        QVERIFY2(caption->property("contentHeight").toDouble() <= caption->height() + 1,
                 qPrintable(name));
    }
    for (int i = 0; i < 12; ++i) {
        auto *tile = findVisualItem(board, QStringLiteral("parameterTile_%1").arg(i));
        QVERIFY2(tile, qPrintable(QStringLiteral("Thiếu ô %1").arg(i)));
        QVERIFY(tile->x() >= 0 && tile->x() + tile->width() <= board->width());
        QVERIFY(tile->y() >= 0 && tile->y() + tile->height() <= 821);
        auto *symbol = tile->findChild<QQuickItem *>(QStringLiteral("parameterSymbol"));
        auto *unit = tile->findChild<QQuickItem *>(QStringLiteral("parameterUnit"));
        auto *value = tile->findChild<QQuickItem *>(QStringLiteral("readingValue"));
        auto *range = tile->findChild<QQuickItem *>(QStringLiteral("measurementRange"));
        auto *status = tile->findChild<QQuickItem *>(QStringLiteral("tileStatus"));
        auto *statusText = tile->findChild<QQuickItem *>(QStringLiteral("statusLabelText"));
        QVERIFY(symbol);
        QVERIFY(unit);
        QVERIFY(value);
        QVERIFY(range);
        QVERIFY(status);
        QVERIFY(statusText);
        QCOMPARE(symbol->property("font").value<QFont>().pixelSize(), 36);
        QCOMPARE(unit->property("font").value<QFont>().pixelSize(), 29);
        QCOMPARE(range->property("font").value<QFont>().pixelSize(), 29);
        QCOMPARE(status->property("fontSize").toInt(), 26);
        QVERIFY(symbol->x() + symbol->width() <= unit->x());
        QVERIFY(symbol->y() + symbol->height() <= value->y());
        QVERIFY(unit->y() + unit->height() <= value->y());
        QVERIFY(value->y() + value->height() <= range->y());
        QVERIFY(range->y() + range->height() <= status->y());
        QVERIFY(status->y() + status->height() <= tile->height());
        QVERIFY(!unit->property("truncated").toBool());
        QVERIFY(range->property("contentWidth").toDouble() <= range->width() + 1);
        QVERIFY(range->property("contentHeight").toDouble() <= range->height() + 1);
        QVERIFY(statusText->property("contentWidth").toDouble() <= statusText->width() + 1);
        QVERIFY(statusText->property("contentHeight").toDouble() <= status->height() - 2);
    }
    auto *flow = findVisualItem(board, QStringLiteral("parameterTile_6"));
    auto *ph = findVisualItem(board, QStringLiteral("parameterTile_8"));
    QVERIFY(flow);
    QVERIFY(ph);
    auto *range = flow->findChild<QQuickItem *>(QStringLiteral("measurementRange"));
    auto *value = flow->findChild<QQuickItem *>(QStringLiteral("readingValue"));
    auto *status = flow->findChild<QQuickItem *>(QStringLiteral("tileStatus"));
    auto *frame = flow->findChild<QQuickItem *>(QStringLiteral("tileFrame"));
    QVERIFY(range);
    QVERIFY(value);
    QVERIFY(status);
    QVERIFY(frame);
    QCOMPARE(range->property("text").toString(), QStringLiteral("(0 - 1000000 m³/h)"));
    auto *phRange = ph->findChild<QQuickItem *>(QStringLiteral("measurementRange"));
    QVERIFY(phRange);
    QCOMPARE(phRange->property("text").toString(), QStringLiteral("(6 – 9)"));
    QVERIFY(ph->setProperty("rangeText", QStringLiteral("6.123456789012345 – 8.123456789012345")));
    QVERIFY(phRange->property("contentWidth").toDouble() <= phRange->width() + 1);
    QVERIFY(flow->setProperty("valueText", QStringLiteral("1000000")));
    QTRY_VERIFY(value->property("contentWidth").toDouble() <= value->width() + 1);
    QVERIFY(value->property("contentHeight").toDouble() <= value->height() + 1);
    QVERIFY(flow->setProperty("valueText", QStringLiteral("-538964")));
    QTRY_VERIFY(value->property("contentWidth").toDouble() <= value->width() + 1);
    QTRY_VERIFY(range->property("contentWidth").toDouble() <= range->width() + 1);
    QVERIFY(flow->setProperty("stateKey", QStringLiteral("withinLimit")));
    QCOMPARE(status->property("text").toString(), QStringLiteral("ĐANG ĐO"));
    QCOMPARE(value->property("color").value<QColor>(), QColor("#00E5FF"));
    QVERIFY(flow->setProperty("stateKey", QStringLiteral("exceedsLimit")));
    QVERIFY(status->property("text").toString().contains(QStringLiteral("VƯỢT NGƯỠNG")));
    QCOMPARE(value->property("color").value<QColor>(), QColor("#FF5252"));
    QCOMPARE(range->property("color").value<QColor>(), QColor("#00E5FF"));
    QCOMPARE(frame->property("border").value<QObject *>()->property("color").value<QColor>(), QColor("#00E5FF"));
    QVERIFY(flow->setProperty("stateKey", QStringLiteral("deviceFault")));
    QCOMPARE(value->property("color").value<QColor>(), QColor("#FF5252"));
    QCOMPARE(range->property("color").value<QColor>(), QColor("#FF5252"));
    QVERIFY(flow->setProperty("stateKey", QStringLiteral("unknownStatus")));
    QCOMPARE(status->property("text").toString(), QStringLiteral("LỖI TRẠNG THÁI"));
    auto *statusLabel = flow->findChild<QQuickItem *>(QStringLiteral("statusLabelText"));
    QVERIFY(statusLabel);
    QVERIFY(flow->setProperty("stateKey", QStringLiteral("withinLimit")));
    QCOMPARE(statusLabel->property("color").value<QColor>(), QColor("#00E5FF"));
    QVERIFY(flow->setProperty("stateKey", QStringLiteral("measuring")));
    QCOMPARE(statusLabel->property("color").value<QColor>(), QColor("#00E5FF"));
    QVERIFY(flow->setProperty("stateKey", QStringLiteral("exceedsLimit")));
    QCOMPARE(statusLabel->property("color").value<QColor>(), QColor("#FF5252"));
    const QList<QPair<QString, QString>> statuses = {
        {QStringLiteral("noData"), QStringLiteral("CHƯA CÓ DỮ LIỆU")},
        {QStringLiteral("measuring"), QStringLiteral("ĐANG ĐO")},
        {QStringLiteral("exceedsLimit"), QStringLiteral("ĐANG ĐO · VƯỢT NGƯỠNG")},
        {QStringLiteral("deviceFault"), QStringLiteral("LỖI THIẾT BỊ")},
        {QStringLiteral("calibrating"), QStringLiteral("HIỆU CHUẨN")},
        {QStringLiteral("maintenance"), QStringLiteral("BẢO TRÌ")},
        {QStringLiteral("stale"), QStringLiteral("DỮ LIỆU CŨ")},
        {QStringLiteral("futureTimestamp"), QStringLiteral("SAI GIỜ ĐO")},
        {QStringLiteral("invalid"), QStringLiteral("DỮ LIỆU LỖI")},
        {QStringLiteral("unitMismatch"), QStringLiteral("SAI ĐƠN VỊ")},
        {QStringLiteral("conflict"), QStringLiteral("XUNG ĐỘT")},
        {QStringLiteral("unknownStatus"), QStringLiteral("LỖI TRẠNG THÁI")},
    };
    for (const auto &[key, text] : statuses) {
        QVERIFY(flow->setProperty("statusText", text));
        QVERIFY(flow->setProperty("stateKey", key));
        QCOMPARE(status->property("text").toString(), text);
        QVERIFY2(statusLabel->property("contentWidth").toDouble() <= statusLabel->width() + 1,
                 qPrintable(key));
        QVERIFY2(statusLabel->property("contentHeight").toDouble() <= status->height() - 2,
                 qPrintable(key));
        QVERIFY2(statusLabel->x() >= 0 && statusLabel->x() + statusLabel->width() <= status->width(),
                 qPrintable(key));
    }
    auto *within = findVisualItem(board, QStringLiteral("legendWithin"));
    auto *exceeds = findVisualItem(board, QStringLiteral("legendExceeds"));
    QVERIFY(within);
    QVERIFY(exceeds);
    QVERIFY(within->y() >= 809 && within->y() + within->height() < exceeds->y());
    QVERIFY(exceeds->y() + exceeds->height() <= board->height());
    for (auto *legend : {within, exceeds}) {
        auto *text = legend->findChild<QQuickItem *>(QStringLiteral("legendText"));
        QVERIFY(text);
        QCOMPARE(text->property("font").value<QFont>().pixelSize(), 32);
        QVERIFY(text->property("contentWidth").toDouble() <= text->width() + 1);
        QVERIFY(text->property("contentHeight").toDouble() <= text->height() + 1);
    }

    // Khi hủy chọn output ở chế độ vận hành, LED phải ẩn, không chuyển
    // sang màn PC; chọn lại output thì vẫn toàn màn hình, không viền.
    QCOMPARE(controller.updateTargetScreen(QString()), QString());
    QTRY_COMPARE(window->visibility(), QWindow::Hidden);
    QCOMPARE(controller.updateTargetScreen(QStringLiteral("B Test LED")), QString());
    QTRY_COMPARE(window->visibility(), QWindow::FullScreen);
    QCOMPARE(window->screen()->name(), QStringLiteral("B Test LED"));
    emit controller.screensChanged();
    QTRY_COMPARE(window->visibility(), QWindow::FullScreen);
    QCOMPARE(window->screen()->name(), QStringLiteral("B Test LED"));
    object.reset();
    controller.shutdown();
}

void TestWindowUi::operatorChrome()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());
    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    controller.initialize(temp.filePath(QStringLiteral("config.json")));
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("diagnosticsModel"), &diagnostics);
    engine.rootContext()->setContextProperty(QStringLiteral("ledParameterModel"), &led);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(CAONGAN_QML_SOURCE_DIR "/OperatorWindow.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto *window = qobject_cast<QQuickWindow *>(object.get());
    QVERIFY(window);
    // Khởi động: cửa sổ cấu hình tự sang màn còn lại (màn chính) và phóng to.
    QTRY_COMPARE(window->visibility(), QWindow::Maximized);
    QCOMPARE(window->screen(), QGuiApplication::primaryScreen());
    QVERIFY(!window->property("clockText").isValid());
    QVERIFY(window->findChild<QQuickItem *>(QStringLiteral("operatorLogo")));

    // Trở về cửa sổ để kiểm tra bố cục theo các độ rộng khác nhau.
    window->showNormal();
    QTRY_COMPARE(window->visibility(), QWindow::Windowed);

    // Header và dòng dữ liệu phải dùng cùng độ rộng kể cả khi có thanh cuộn.
    QVERIFY(window->setProperty("pageIndex", 1));
    auto *header = findVisualItem(window->contentItem(), QStringLiteral("channelHeader"));
    auto *channel = findVisualItem(window->contentItem(), QStringLiteral("channelRow_ph"));
    QVERIFY(header);
    QVERIFY(channel);
    QTRY_COMPARE(header->width(), channel->width());
    QCOMPARE(channel->property("rangeText").toString(), QStringLiteral("0 - 14"));
    for (const int width : {1120, 1440}) {
        window->setWidth(width);
        QTRY_COMPARE(window->width(), width);
        QTRY_COMPARE(header->width(), channel->width());
        for (int i = 0; i < 5; ++i) {
            const QString name = QStringLiteral("channelCell_%1").arg(i);
            auto *headerCell = findVisualItem(header, name);
            auto *dataCell = findVisualItem(channel, name);
            QVERIFY(headerCell);
            QVERIFY(dataCell);
            QTRY_COMPARE(headerCell->x(), dataCell->x());
            QTRY_COMPARE(headerCell->width(), dataCell->width());
        }
    }
    if (qEnvironmentVariableIsSet("CAONGAN_LED_TEST_OPERATOR_SCREENSHOT")) {
        QTRY_VERIFY(window->isExposed());
        QVERIFY(window->grabWindow().save(qEnvironmentVariable("CAONGAN_LED_TEST_OPERATOR_SCREENSHOT")));
    }
#ifndef Q_OS_WIN
    QVERIFY(window->flags().testFlag(Qt::FramelessWindowHint));
#endif

    auto *dragArea = window->findChild<QQuickItem *>(QStringLiteral("operatorTitleBarDragArea"));
    auto *minimize = window->findChild<QQuickItem *>(QStringLiteral("operatorMinimizeButton"));
    auto *maximize = window->findChild<QQuickItem *>(QStringLiteral("operatorMaximizeButton"));
    auto *close = window->findChild<QQuickItem *>(QStringLiteral("operatorCloseButton"));
    QVERIFY(dragArea);
    QVERIFY(minimize);
    QVERIFY(maximize);
    QVERIFY(close);

    const auto click = [window](QQuickItem *item) {
        const QPoint point = item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point);
    };
    click(maximize);
    QTRY_COMPARE(window->visibility(), QWindow::Maximized);
    click(maximize);
    QTRY_COMPARE(window->visibility(), QWindow::Windowed);
    const QPoint titlePoint = dragArea->mapToScene(QPointF(100, dragArea->height() / 2)).toPoint();
    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, titlePoint);
    QTRY_COMPARE(window->visibility(), QWindow::Maximized);
    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, titlePoint);
    QTRY_COMPARE(window->visibility(), QWindow::Windowed);
    click(minimize);
    QTRY_COMPARE(window->visibility(), QWindow::Minimized);
    window->showNormal();
    QTRY_COMPARE(window->visibility(), QWindow::Windowed);

    click(close);
    auto *exitDialog = window->findChild<QObject *>(QStringLiteral("exitConfirmationDialog"));
    QVERIFY(exitDialog);
    QTRY_VERIFY(exitDialog->property("visible").toBool());
    QVERIFY(window->isVisible());
    if (qEnvironmentVariableIsSet("CAONGAN_LED_TEST_EXIT_DIALOG_SCREENSHOT")) {
        QTRY_VERIFY(exitDialog->property("opened").toBool());
        QVERIFY(window->grabWindow().save(qEnvironmentVariable("CAONGAN_LED_TEST_EXIT_DIALOG_SCREENSHOT")));
    }
    auto *cancelExit = findVisualItem(window->contentItem(), QStringLiteral("cancelExitButton"));
    QVERIFY(cancelExit);
    click(cancelExit);
    QTRY_VERIFY(!exitDialog->property("visible").toBool());
    QVERIFY(window->isVisible());

    // Kể cả yêu cầu đóng từ hệ điều hành cũng phải hỏi trước.
    window->close();
    QTRY_VERIFY(exitDialog->property("visible").toBool());
    QVERIFY(window->isVisible());
    auto *confirmExit = findVisualItem(window->contentItem(), QStringLiteral("confirmExitButton"));
    QVERIFY(confirmExit);
    click(confirmExit);
    QTRY_VERIFY(!window->isVisible());
    object.reset();
    controller.shutdown();
}

void TestWindowUi::cancelExitKeepsLedVisible()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());
    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    controller.initialize(temp.filePath(QStringLiteral("config.json")));
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("diagnosticsModel"), &diagnostics);
    engine.rootContext()->setContextProperty(QStringLiteral("ledParameterModel"), &led);
    QQmlComponent ledComponent(&engine, QUrl::fromLocalFile(QStringLiteral(CAONGAN_QML_SOURCE_DIR "/LedWindow.qml")));
    QQmlComponent operatorComponent(&engine, QUrl::fromLocalFile(QStringLiteral(CAONGAN_QML_SOURCE_DIR "/OperatorWindow.qml")));
    QVERIFY2(ledComponent.isReady(), qPrintable(ledComponent.errorString()));
    QVERIFY2(operatorComponent.isReady(), qPrintable(operatorComponent.errorString()));
    std::unique_ptr<QObject> ledObject(ledComponent.create());
    std::unique_ptr<QObject> operatorObject(operatorComponent.create());
    QVERIFY2(ledObject, qPrintable(ledComponent.errorString()));
    QVERIFY2(operatorObject, qPrintable(operatorComponent.errorString()));
    auto *ledWindow = qobject_cast<QQuickWindow *>(ledObject.get());
    auto *operatorWindow = qobject_cast<QQuickWindow *>(operatorObject.get());
    QVERIFY(ledWindow);
    QVERIFY(operatorWindow);
    QTRY_VERIFY(ledWindow->isVisible());
    QTRY_VERIFY(operatorWindow->isVisible());

    operatorWindow->close();
    auto *exitDialog = operatorWindow->findChild<QObject *>(QStringLiteral("exitConfirmationDialog"));
    QVERIFY(exitDialog);
    QTRY_VERIFY(exitDialog->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(exitDialog, "reject"));
    QTRY_VERIFY(!exitDialog->property("visible").toBool());
    QVERIFY(operatorWindow->isVisible());
    QVERIFY(ledWindow->isVisible());
    QVERIFY(controller.ledWindowVisible());
    operatorObject.reset();
    ledObject.reset();
    controller.shutdown();
}

void TestWindowUi::thresholdEditor()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());
    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    const QString path = temp.filePath(QStringLiteral("config.json"));
    controller.initialize(path);
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("diagnosticsModel"), &diagnostics);
    engine.rootContext()->setContextProperty(QStringLiteral("ledParameterModel"), &led);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(CAONGAN_QML_SOURCE_DIR "/OperatorWindow.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto *window = qobject_cast<QQuickWindow *>(object.get());
    QVERIFY(window);
    QTRY_VERIFY(window->isVisible());
    QVERIFY(window->setProperty("pageIndex", 1));

    const auto click = [window](QQuickItem *item) {
        const QPoint point = item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point);
    };
    auto *edit = findVisualItem(window->contentItem(), QStringLiteral("editThreshold_o2"));
    QVERIFY(edit);
    QTRY_VERIFY(window->isExposed());
    QTRY_VERIFY(edit->isVisible());
    QTRY_VERIFY(edit->width() > 0 && edit->height() > 0);
    click(edit);
    auto *editor = window->findChild<QObject *>(QStringLiteral("thresholdEditor"));
    QVERIFY(editor);
    QTRY_VERIFY(editor->property("visible").toBool());
    if (qEnvironmentVariableIsSet("CAONGAN_LED_TEST_THRESHOLD_DIALOG_SCREENSHOT")) {
        QTRY_VERIFY(editor->property("opened").toBool());
        QVERIFY(window->grabWindow().save(qEnvironmentVariable("CAONGAN_LED_TEST_THRESHOLD_DIALOG_SCREENSHOT")));
    }
    auto *low = findVisualItem(window->contentItem(), QStringLiteral("thresholdLowInput"));
    auto *high = findVisualItem(window->contentItem(), QStringLiteral("thresholdHighInput"));
    auto *save = findVisualItem(window->contentItem(), QStringLiteral("saveThresholdButton"));
    QVERIFY(low);
    QVERIFY(high);
    QVERIFY(save);
    QVERIFY(low->setProperty("text", QStringLiteral("20")));
    QVERIFY(high->setProperty("text", QStringLiteral("10")));
    click(save);
    QTRY_VERIFY(!editor->property("errorText").toString().isEmpty());
    QVERIFY(editor->property("visible").toBool());

    QVERIFY(low->setProperty("text", QStringLiteral("0")));
    QVERIFY(high->setProperty("text", QStringLiteral("10.5")));
    click(save);
    auto *confirm = window->findChild<QObject *>(QStringLiteral("confirmThreshold"));
    QVERIFY(confirm);
    QTRY_VERIFY(confirm->property("visible").toBool());
    if (qEnvironmentVariableIsSet("CAONGAN_LED_TEST_CONFIRM_THRESHOLD_SCREENSHOT")) {
        QTRY_VERIFY(confirm->property("opened").toBool());
        QVERIFY(window->grabWindow().save(qEnvironmentVariable("CAONGAN_LED_TEST_CONFIRM_THRESHOLD_SCREENSHOT")));
    }
    QVERIFY(QMetaObject::invokeMethod(confirm, "reject"));
    QTRY_VERIFY(!confirm->property("visible").toBool());
    QVERIFY(editor->property("visible").toBool());
    QCOMPARE(controller.channelSettings().at(0).toMap().value(QStringLiteral("thresholdHigh")).toString(), QStringLiteral("15"));

    click(save);
    QTRY_VERIFY(confirm->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(confirm, "accept"));
    QTRY_VERIFY(!editor->property("visible").toBool());
    QCOMPARE(controller.channelSettings().at(0).toMap().value(QStringLiteral("thresholdHigh")).toString(), QStringLiteral("10.5"));
    ConfigStore store(path);
    AppConfig saved;
    QCOMPARE(store.load(saved), ConfigStore::LoadStatus::Loaded);
    QCOMPARE(saved.channelById(QStringLiteral("o2"))->threshold.high, 10.5);
    object.reset();
    controller.shutdown();
}

void TestWindowUi::sourceDraftsAndDisplayReassignment()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());
    const QString gasDir = temp.filePath(QStringLiteral("in-gas"));
    QVERIFY(QDir().mkpath(gasDir));

    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    controller.initialize(temp.filePath(QStringLiteral("config.json")));

    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("diagnosticsModel"), &diagnostics);
    engine.rootContext()->setContextProperty(QStringLiteral("ledParameterModel"), &led);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(CAONGAN_QML_SOURCE_DIR "/OperatorWindow.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto *window = qobject_cast<QQuickWindow *>(object.get());
    QVERIFY(window);
    QTRY_COMPARE(window->visibility(), QWindow::Maximized);
    QCOMPARE(window->screen()->name(), QStringLiteral("A Test operator"));
    QVERIFY(window->setProperty("pageIndex", 0));
    QTRY_VERIFY(window->isExposed());

    auto *staleSpin = findVisualItem(window->contentItem(), QStringLiteral("sourceStaleSpin_khi-thai"));
    auto *staleUnit = findVisualItem(window->contentItem(), QStringLiteral("sourceStaleUnit_khi-thai"));
    QVERIFY(staleSpin);
    QVERIFY(staleUnit);
    const QPointF spinPos = staleSpin->mapToScene(QPointF(0, 0));
    const QPointF unitPos = staleUnit->mapToScene(QPointF(0, 0));
    QVERIFY2(unitPos.y() < spinPos.y() + staleSpin->height(), "Nhãn giây bị rớt xuống dòng dưới");
    QVERIFY2(unitPos.x() >= spinPos.x() + staleSpin->width(), "Nhãn giây không nằm cạnh SpinBox");

    auto *waterFolder = findVisualItem(window->contentItem(), QStringLiteral("sourceFolderField_nuoc-thai"));
    QVERIFY(waterFolder);
    const QPoint waterCenter = waterFolder->mapToScene(
        QPointF(waterFolder->width() / 2, waterFolder->height() / 2)).toPoint();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, waterCenter);
    const QByteArray draftPath = QByteArrayLiteral("/tmp/unsaved-water");
    for (const char ch : draftPath)
        QTest::keyClick(window, ch);
    QCOMPARE(waterFolder->property("text").toString(), QString::fromLatin1(draftPath));

    QCOMPARE(controller.updateSource(QStringLiteral("khi-thai"), gasDir, QStringLiteral("KHITO1"), 900),
             QString());
    auto *waterFolderAfter = findVisualItem(window->contentItem(), QStringLiteral("sourceFolderField_nuoc-thai"));
    QVERIFY(waterFolderAfter);
    QCOMPARE(waterFolderAfter->property("text").toString(), QStringLiteral("/tmp/unsaved-water"));

    // Chọn màn chính làm LED thì cửa sổ vận hành phải tự dời sang màn còn lại.
    QCOMPARE(controller.updateTargetScreen(QStringLiteral("A Test operator")), QString());
    QCOMPARE(controller.operatorScreenName(), QStringLiteral("B Test LED"));
    QTRY_COMPARE(window->screen()->name(), QStringLiteral("B Test LED"));

    object.reset();
    controller.shutdown();
}

QTEST_MAIN(TestWindowUi)
#include "tst_window_ui.moc"
