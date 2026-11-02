#include "domain/ParameterCatalog.h"
#include "presentation/ApplicationController.h"
#include "presentation/DiagnosticsModel.h"
#include "presentation/LedParameterModel.h"
#include "presentation/SimulationFactory.h"

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

QTEST_MAIN(TestPresentation)
#include "tst_presentation.moc"
