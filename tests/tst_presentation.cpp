#include "domain/ParameterCatalog.h"
#include "domain/Tt10Time.h"
#include "presentation/ApplicationController.h"
#include "presentation/DiagnosticsModel.h"
#include "presentation/LedParameterModel.h"
#include "presentation/SimulationFactory.h"
#include "storage/ConfigStore.h"
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
    void editableThresholds();
    void thresholdUpdateReevaluatesRealInput();
    void thresholdSaveFailureDoesNotApply();
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
    QVERIFY(config.channelById(QStringLiteral("so2"))->threshold.enabled);
}

void TestPresentation::screenPlacement()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());
    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    controller.initialize(temp.filePath("config.json"), false);
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
    QVERIFY(!controller.updateTargetScreen(QStringLiteral("Duplicate")).isEmpty());
    QVERIFY(!controller.ledWindowVisible());
    QCOMPARE(controller.updateTargetScreen(screen->name()), QString());
    QVERIFY(controller.ledWindowVisible());
    QVERIFY(controller.positionLedWindow(&window));
    QCOMPARE(window.screen(), screen);
    QCOMPARE(window.position(), screen->geometry().topLeft());
    QCOMPARE(controller.updateTargetScreen(QString()), QString());
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
    controller.initialize(temp.filePath("config.json"), true);
    QWindow window;
    QVERIFY(controller.ledWindowVisible());
    QVERIFY(controller.positionLedWindow(&window));
    QCOMPARE(window.screen(), QGuiApplication::primaryScreen());
    QCOMPARE(window.position(), QGuiApplication::primaryScreen()->geometry().topLeft());
    controller.shutdown();
}

void TestPresentation::editableThresholds()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());
    const QString path = temp.filePath(QStringLiteral("config.json"));

    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    controller.initialize(path, true, true, QStringLiteral("Bình thường"));
    const auto stateFor = [&led](const QString &id, int role) -> QString {
        for (int row = 0; row < led.rowCount(); ++row) {
            const QModelIndex index = led.index(row, 0);
            if (led.data(index, LedParameterModel::ChannelIdRole).toString() == id)
                return led.data(index, role).toString();
        }
        return {};
    };
    QCOMPARE(stateFor(QStringLiteral("cod"), LedParameterModel::StateKeyRole), QStringLiteral("withinLimit"));
    QVERIFY(!controller.validateThreshold(QStringLiteral("unknown"), QStringLiteral("0"), QStringLiteral("1")).isEmpty());
    QVERIFY(!controller.updateThreshold(QStringLiteral("cod"), QStringLiteral("NaN"), QStringLiteral("1")).isEmpty());
    QVERIFY(!controller.updateThreshold(QStringLiteral("cod"), QStringLiteral("1,5"), QStringLiteral("2")).isEmpty());
    QVERIFY(!controller.updateThreshold(QStringLiteral("cod"), QStringLiteral("10"), QStringLiteral("2")).isEmpty());
    QCOMPARE(stateFor(QStringLiteral("cod"), LedParameterModel::StateKeyRole), QStringLiteral("withinLimit"));

    QCOMPARE(controller.updateThreshold(QStringLiteral("cod"), QStringLiteral("0"), QStringLiteral("30")), QString());
    QCOMPARE(stateFor(QStringLiteral("cod"), LedParameterModel::StateKeyRole), QStringLiteral("exceedsLimit"));
    QCOMPARE(controller.updateThreshold(QStringLiteral("ph"), QStringLiteral("6.25"), QStringLiteral("8.75")), QString());
    QCOMPARE(stateFor(QStringLiteral("ph"), LedParameterModel::ThresholdTextRole), QStringLiteral("6.25 – 8.75"));

    ConfigStore store(path);
    AppConfig saved;
    QCOMPARE(store.load(saved), ConfigStore::LoadStatus::Loaded);
    QCOMPARE(saved.channelById(QStringLiteral("cod"))->threshold.high, 30.0);
    QCOMPARE(saved.channelById(QStringLiteral("ph"))->threshold.low, 6.25);
    QCOMPARE(saved.channelById(QStringLiteral("ph"))->threshold.high, 8.75);
    QVERIFY(QFile::exists(store.backupPath()));
    controller.shutdown();

    LedParameterModel reloadedLed;
    DiagnosticsModel reloadedDiagnostics;
    ApplicationController reloaded(&reloadedLed, &reloadedDiagnostics);
    reloaded.initialize(path, true, true, QStringLiteral("Bình thường"));
    QCOMPARE(reloaded.channelSettings().at(8).toMap().value(QStringLiteral("thresholdLow")).toString(), QStringLiteral("6.25"));
    QCOMPARE(reloadedLed.data(reloadedLed.index(10, 0), LedParameterModel::StateKeyRole).toString(), QStringLiteral("exceedsLimit"));
    reloaded.shutdown();
}

void TestPresentation::thresholdSaveFailureDoesNotApply()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());
    const QString path = temp.filePath(QStringLiteral("config.json"));
    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    controller.initialize(path, true, true, QStringLiteral("Bình thường"));
    // Không thể thay backup bằng tệp khi tại đường dẫn đó có một thư mục.
    QVERIFY(QDir().mkpath(path + QStringLiteral(".backup")));
    QVERIFY(!controller.updateThreshold(QStringLiteral("cod"), QStringLiteral("0"), QStringLiteral("30")).isEmpty());
    QCOMPARE(controller.channelSettings().at(10).toMap().value(QStringLiteral("thresholdHigh")).toString(), QStringLiteral("82.5"));
    QCOMPARE(led.data(led.index(10, 0), LedParameterModel::StateKeyRole).toString(), QStringLiteral("withinLimit"));
    ConfigStore store(path);
    AppConfig saved;
    QCOMPARE(store.load(saved), ConfigStore::LoadStatus::Loaded);
    QCOMPARE(saved.channelById(QStringLiteral("cod"))->threshold.high, 82.5);
    controller.shutdown();
}

void TestPresentation::thresholdUpdateReevaluatesRealInput()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    qputenv("CAONGAN_LED_BASE_DIR", temp.path().toUtf8());
    const QString source = temp.filePath(QStringLiteral("input"));
    QVERIFY(QDir().mkpath(source));
    const QString timestamp = tt10::formatTimestamp(QDateTime::currentDateTimeUtc().addSecs(-5));
    const QString name = QStringLiteral("TNg_NDCN_KHITO1_%1.txt").arg(timestamp);
    const QByteArray input = QStringLiteral("CO\t46\tmg/Nm3\t%1\t00\n").arg(timestamp).toUtf8();
    QFile file(QDir(source).filePath(name));
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(input), input.size());
    file.close();

    const QString path = temp.filePath(QStringLiteral("config.json"));
    ConfigStore store(path);
    AppConfig config = defaultAppConfig();
    config.sources[0].folder = source;
    QString error;
    QVERIFY2(store.save(config, &error), qPrintable(error));

    LedParameterModel led;
    DiagnosticsModel diagnostics;
    ApplicationController controller(&led, &diagnostics);
    controller.initialize(path, true);
    QTRY_COMPARE_WITH_TIMEOUT(led.data(led.index(1, 0), LedParameterModel::StateKeyRole).toString(),
                              QStringLiteral("withinLimit"), 10000);
    QCOMPARE(controller.updateThreshold(QStringLiteral("co"), QStringLiteral("0"), QStringLiteral("30")), QString());
    QTRY_COMPARE_WITH_TIMEOUT(led.data(led.index(1, 0), LedParameterModel::StateKeyRole).toString(),
                              QStringLiteral("exceedsLimit"), 10000);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), input);
    file.close();
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
    configured.initialize(configPath, true);
    QCOMPARE(configured.refreshLogFiles(), QString());
    QVERIFY(!configured.exportLog(QUrl::fromLocalFile(customDir + QStringLiteral("/active.json"))).isEmpty());
    QVERIFY(!configured.exportLog(QUrl::fromLocalFile(customDir + QStringLiteral("/active.json.backup"))).isEmpty());
    configured.shutdown();
}

QTEST_MAIN(TestPresentation)
#include "tst_presentation.moc"
