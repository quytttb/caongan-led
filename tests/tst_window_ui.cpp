#include "domain/ParameterCatalog.h"
#include "presentation/ApplicationController.h"
#include "presentation/DiagnosticsModel.h"
#include "presentation/LedParameterModel.h"
#include "storage/ConfigStore.h"
#include "utils/AppPaths.h"

#include <QGuiApplication>
#include <QFile>
#include <QFont>
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
    void ledAlwaysFullScreen_data();
    void ledAlwaysFullScreen();
    void operatorChrome();
    void simulationIsMarkedAndIsolated();
};

void TestWindowUi::ledAlwaysFullScreen_data()
{
    QTest::addColumn<bool>("preview");
    QTest::newRow("legacy-windowed-production") << false;
    QTest::newRow("legacy-windowed-preview") << true;
}

void TestWindowUi::ledAlwaysFullScreen()
{
    QFETCH(bool, preview);
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());

    // Cấu hình cũ có fullScreen=false vẫn phải đọc được nhưng không được
    // khiến LED hiện title bar hoặc ra khỏi chế độ toàn màn hình.
    AppConfig config = defaultAppConfig();
    config.branding.branchName = QStringLiteral("Chi nhánh tổng công ty điện lực TKV - CTCP");
    config.branding.address =
        QStringLiteral("Ngõ 517 đường Dương Tự Minh, Phường Quan Triều, tỉnh Thái Nguyên");
    config.display.targetScreenName = QStringLiteral("Test LED");
    config.display.fullScreen = false;
    const QString configPath = temp.filePath(QStringLiteral("config.json"));
    ConfigStore store(configPath);
    QString error;
    QVERIFY2(store.save(config, &error), qPrintable(error));

    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    controller.initialize(configPath, preview);
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
    if (preview)
        QCOMPARE(window->geometry(), QGuiApplication::primaryScreen()->geometry());
    else
        QCOMPARE(window->screen()->name(), QStringLiteral("Test LED"));
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
    for (int i = 0; i < 12; ++i) {
        auto *tile = findVisualItem(board, QStringLiteral("parameterTile_%1").arg(i));
        QVERIFY2(tile, qPrintable(QStringLiteral("Thiếu ô %1").arg(i)));
        QVERIFY(tile->x() >= 0 && tile->x() + tile->width() <= board->width());
        QVERIFY(tile->y() >= 0 && tile->y() + tile->height() <= 821);
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
    QVERIFY(flow->setProperty("valueText", QStringLiteral("1000000")));
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
    auto *within = findVisualItem(board, QStringLiteral("legendWithin"));
    auto *exceeds = findVisualItem(board, QStringLiteral("legendExceeds"));
    QVERIFY(within);
    QVERIFY(exceeds);
    QVERIFY(within->y() >= 809 && within->y() + within->height() < exceeds->y());
    QVERIFY(exceeds->y() + exceeds->height() <= board->height());

    // Khi hủy chọn output ở chế độ vận hành, LED phải ẩn, không chuyển
    // sang màn PC; chọn lại output thì vẫn toàn màn hình, không viền.
    if (!preview) {
        QCOMPARE(controller.updateTargetScreen(QString()), QString());
        QTRY_COMPARE(window->visibility(), QWindow::Hidden);
        QCOMPARE(controller.updateTargetScreen(QStringLiteral("Test LED")), QString());
        QTRY_COMPARE(window->visibility(), QWindow::FullScreen);
        QCOMPARE(window->screen()->name(), QStringLiteral("Test LED"));
        emit controller.screensChanged();
        QTRY_COMPARE(window->visibility(), QWindow::FullScreen);
        QCOMPARE(window->screen()->name(), QStringLiteral("Test LED"));
    }
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
    controller.initialize(temp.filePath(QStringLiteral("config.json")), false);
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
    QTRY_COMPARE(window->visibility(), QWindow::Windowed);
    QVERIFY(!window->property("clockText").isValid());
    QVERIFY(window->findChild<QQuickItem *>(QStringLiteral("operatorLogo")));

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
    QTRY_VERIFY(!window->isVisible());
    object.reset();
    controller.shutdown();
}

void TestWindowUi::simulationIsMarkedAndIsolated()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());
    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    controller.initialize(temp.filePath(QStringLiteral("config.json")), true, true,
                          QStringLiteral("Vượt ngưỡng"));
    QVERIFY(controller.simulationActive());
    QCOMPARE(controller.statusSummary(), QStringLiteral("DỮ LIỆU MÔ PHỎNG"));
    QVERIFY(!QFile::exists(AppPaths::databasePath()));

    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("ledParameterModel"), &led);
    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(CAONGAN_QML_SOURCE_DIR "/LedWindow.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto *window = qobject_cast<QQuickWindow *>(object.get());
    QVERIFY(window);
    auto *notice = findVisualItem(window->contentItem(), QStringLiteral("simulationNotice"));
    QVERIFY(notice);
    QVERIFY(notice->isVisible());
    QCOMPARE(notice->property("text").toString(), QStringLiteral("DỮ LIỆU MÔ PHỎNG"));
    auto *so2 = findVisualItem(window->contentItem(), QStringLiteral("parameterTile_3"));
    QVERIFY(so2);
    QCOMPARE(so2->property("stateKey").toString(), QStringLiteral("exceedsLimit"));
    if (qEnvironmentVariableIsSet("CAONGAN_LED_TEST_SCREENSHOT")) {
        QTRY_VERIFY(window->isExposed());
        QVERIFY(window->grabWindow().save(qEnvironmentVariable("CAONGAN_LED_TEST_SCREENSHOT")));
    }
    object.reset();
    controller.shutdown();
}

QTEST_MAIN(TestWindowUi)
#include "tst_window_ui.moc"
