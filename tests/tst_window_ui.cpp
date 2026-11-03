#include "domain/ParameterCatalog.h"
#include "presentation/ApplicationController.h"
#include "presentation/DiagnosticsModel.h"
#include "presentation/LedParameterModel.h"
#include "storage/ConfigStore.h"

#include <QGuiApplication>
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

class TestWindowUi : public QObject
{
    Q_OBJECT
private slots:
    void ledAlwaysFullScreen_data();
    void ledAlwaysFullScreen();
    void operatorChrome();
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

QTEST_MAIN(TestWindowUi)
#include "tst_window_ui.moc"
