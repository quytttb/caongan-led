#include "domain/ParameterCatalog.h"
#include "domain/Tt10Time.h"
#include "ingestion/IngestionService.h"
#include "storage/SnapshotStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace caongan;

namespace {
bool writeInput(const QString &path, const QByteArray &content, const QDateTime &mtime)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(content) != content.size() || !file.flush())
        return false;
    return file.setFileTime(mtime, QFileDevice::FileModificationTime);
}

void scanNow(IngestionService &service)
{
    service.requestRescan();
    QVERIFY(QMetaObject::invokeMethod(&service, "tick", Qt::DirectConnection));
}

bool containsChannel(const QVector<Reading> &readings, const QString &channel)
{
    for (const auto &reading : readings) {
        if (reading.channelId == channel)
            return true;
    }
    return false;
}
} // namespace

class TestIngestion : public QObject
{
    Q_OBJECT
private slots:
    void batchesEventuallyReachEveryFile();
    void rejectedNameDoesNotPoisonValidCopy();
    void conflictSurvivesRestart();
    void maintenanceSurvivesRestart();
    void thresholdReloadReevaluatesCurrentReading();
    void sharedRootFiltersCrossStationWithoutNoise();
    void futureTimestampRetriesAndRecoversWithoutPoisoningCheckpoint();
};

void TestIngestion::batchesEventuallyReachEveryFile()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QVERIFY(QDir().mkpath(temp.filePath("input")));
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QString oldTime = tt10::formatTimestamp(now.addSecs(-90));

    // CO được sắp trước O2 theo mtime; quá một batch 20 file.
    for (int i = 0; i < 25; ++i) {
        const QString timestamp = tt10::formatTimestamp(now.addSecs(-40 - i));
        const QString name = QStringLiteral("TNg_NDCN_KHITO1_%1.txt").arg(timestamp);
        const QByteArray content = QStringLiteral("CO\t%1\tmg/Nm3\t%2\t00\n")
                                       .arg(i + 1).arg(timestamp).toUtf8();
        QVERIFY(writeInput(temp.filePath("input/" + name), content, now.addSecs(-i)));
    }
    const QByteArray oxygen = QStringLiteral("O2\t5.5\t%V\t%1\t00\n").arg(oldTime).toUtf8();
    const QString oxygenName = QStringLiteral("TNg_NDCN_KHITO1_%1.txt").arg(oldTime);
    const QString oxygenPath = temp.filePath("input/" + oxygenName);
    QVERIFY(writeInput(oxygenPath, oxygen, now.addSecs(-100)));
    const QDateTime originalMtime = QFileInfo(oxygenPath).lastModified();

    AppConfig config = defaultAppConfig();
    config.sources[0].folder = temp.filePath("input");
    const QString db = temp.filePath("snapshot.db");
    IngestionService service;
    service.start(config, db);
    QTest::qWait(1300);
    scanNow(service);
    scanNow(service);
    service.shutdown();

    SnapshotStore store(db);
    QVERIFY(store.open());
    QVector<Reading> readings;
    QVERIFY(store.loadReadings(&readings));
    QVERIFY(containsChannel(readings, QStringLiteral("o2")));
    QCOMPARE(QFileInfo(oxygenPath).lastModified(), originalMtime);
    QFile input(oxygenPath);
    QVERIFY(input.open(QIODevice::ReadOnly));
    QCOMPARE(input.readAll(), oxygen);
    input.close();
    store.close();

    // Khi đổi root rồi restart không phục hồi snapshot của nguồn cũ.
    QVERIFY(QDir().mkpath(temp.filePath("new-input")));
    config.sources[0].folder = temp.filePath("new-input");
    IngestionService resumed;
    QSignalSpy statesSpy(&resumed, &IngestionService::channelsChanged);
    resumed.start(config, db);
    QVERIFY(!statesSpy.isEmpty());
    const auto states = qvariant_cast<QVector<ChannelState>>(statesSpy.constLast().at(0));
    for (const ChannelState &state : states) {
        if (state.config.id == QLatin1String("o2") || state.config.id == QLatin1String("co"))
            QVERIFY(!state.hasReading);
    }
    resumed.shutdown();
}

void TestIngestion::rejectedNameDoesNotPoisonValidCopy()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QVERIFY(QDir().mkpath(temp.filePath("input")));
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QString timestamp = tt10::formatTimestamp(now.addSecs(-30));
    const QString name = QStringLiteral("TNg_NDCN_KHITO1_%1.txt").arg(timestamp);
    const QByteArray content = QStringLiteral("O2\t4.2\t%V\t%1\t00\n").arg(timestamp).toUtf8();
    QVERIFY(writeInput(temp.filePath("input/1_" + name), content, now));
    QVERIFY(writeInput(temp.filePath("input/" + name), content, now.addSecs(-10)));

    AppConfig config = defaultAppConfig();
    config.sources[0].folder = temp.filePath("input");
    config.sources[0].allowLegacyPrefix = false;
    const QString db = temp.filePath("snapshot.db");
    IngestionService service;
    service.start(config, db);
    QTest::qWait(1300);
    scanNow(service);
    service.shutdown();

    SnapshotStore store(db);
    QVERIFY(store.open());
    QVector<Reading> readings;
    QVERIFY(store.loadReadings(&readings));
    QVERIFY(containsChannel(readings, QStringLiteral("o2")));
}

void TestIngestion::conflictSurvivesRestart()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QVERIFY(QDir().mkpath(temp.filePath("input")));
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QString timestamp = tt10::formatTimestamp(now.addSecs(-30));
    const QString stem = QStringLiteral("TNg_NDCN_KHITO1_%1").arg(timestamp);
    const QByteArray first = QStringLiteral("CO\t5\tmg/Nm3\t%1\t00\n").arg(timestamp).toUtf8();
    const QByteArray second = QStringLiteral("CO\t6\tmg/Nm3\t%1\t00\n").arg(timestamp).toUtf8();
    QVERIFY(writeInput(temp.filePath("input/" + stem + ".txt"), first, now));
    QVERIFY(writeInput(temp.filePath("input/" + stem + "_1.txt"), second, now.addSecs(-1)));

    AppConfig config = defaultAppConfig();
    config.sources[0].folder = temp.filePath("input");
    ChannelConfig *co = config.channelById(QStringLiteral("co"));
    QVERIFY(co);
    co->threshold.enabled = true;
    co->threshold.hasHigh = true;
    co->threshold.high = 100;
    const QString db = temp.filePath("snapshot.db");
    IngestionService service;
    service.start(config, db);
    QTest::qWait(1300);
    scanNow(service);
    service.shutdown();

    IngestionService resumed;
    QSignalSpy statesSpy(&resumed, &IngestionService::channelsChanged);
    resumed.start(config, db);
    QVERIFY(!statesSpy.isEmpty());
    const auto states = qvariant_cast<QVector<ChannelState>>(statesSpy.constFirst().at(0));
    bool checked = false;
    for (const ChannelState &state : states) {
        if (state.config.id == QLatin1String("co")) {
            QVERIFY(state.hasReading);
            QCOMPARE(state.display, DisplayState::Conflict);
            QCOMPARE(state.threshold, ThresholdResult::NotEvaluated);
            checked = true;
        }
    }
    QVERIFY(checked);
    resumed.shutdown();
}

void TestIngestion::maintenanceSurvivesRestart()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QVERIFY(QDir().mkpath(temp.filePath("input")));
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QString timestamp = tt10::formatTimestamp(now.addSecs(-30));
    const QString fileName = QStringLiteral("TNg_NDCN_KHITO1_%1.txt").arg(timestamp);
    const QByteArray input = QStringLiteral("CO\t5\tmg/Nm3\t%1\t04\n").arg(timestamp).toUtf8();
    QVERIFY(writeInput(temp.filePath("input/" + fileName), input, now));

    AppConfig config = defaultAppConfig();
    config.sources[0].folder = temp.filePath("input");
    ChannelConfig *co = config.channelById(QStringLiteral("co"));
    QVERIFY(co);
    co->threshold.enabled = true;
    co->threshold.hasHigh = true;
    co->threshold.high = 100;
    const QString db = temp.filePath("snapshot.db");

    IngestionService service;
    service.start(config, db);
    QTest::qWait(1300);
    scanNow(service);
    service.shutdown();

    IngestionService resumed;
    QSignalSpy statesSpy(&resumed, &IngestionService::channelsChanged);
    resumed.start(config, db);
    QVERIFY(!statesSpy.isEmpty());
    const auto states = qvariant_cast<QVector<ChannelState>>(statesSpy.constFirst().at(0));
    bool checked = false;
    for (const ChannelState &state : states) {
        if (state.config.id == QLatin1String("co")) {
            QVERIFY(state.hasReading);
            QCOMPARE(state.reading.deviceStatus, DeviceStatus::Maintenance);
            QCOMPARE(state.statusText, QStringLiteral("BẢO TRÌ"));
            QCOMPARE(state.threshold, ThresholdResult::NotEvaluated);
            checked = true;
        }
    }
    QVERIFY(checked);
    resumed.shutdown();
    QFile original(temp.filePath("input/" + fileName));
    QVERIFY(original.open(QIODevice::ReadOnly));
    QCOMPARE(original.readAll(), input);
}

void TestIngestion::thresholdReloadReevaluatesCurrentReading()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QVERIFY(QDir().mkpath(temp.filePath("input")));
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QString timestamp = tt10::formatTimestamp(now.addSecs(-30));
    const QString fileName = QStringLiteral("TNg_NDCN_KHITO1_%1.txt").arg(timestamp);
    const QByteArray input = QStringLiteral("CO\t46\tmg/Nm3\t%1\t00\n").arg(timestamp).toUtf8();
    QVERIFY(writeInput(temp.filePath("input/" + fileName), input, now));

    AppConfig config = defaultAppConfig();
    config.sources[0].folder = temp.filePath("input");
    IngestionService service;
    QSignalSpy statesSpy(&service, &IngestionService::channelsChanged);
    service.start(config, temp.filePath("snapshot.db"));
    QTest::qWait(1300);
    scanNow(service);

    const auto coState = [&statesSpy]() {
        const auto states = qvariant_cast<QVector<ChannelState>>(statesSpy.constLast().at(0));
        for (const ChannelState &state : states) {
            if (state.config.id == QLatin1String("co"))
                return state;
        }
        return ChannelState{};
    };
    QVERIFY(!statesSpy.isEmpty());
    QVERIFY(coState().hasReading);
    QCOMPARE(coState().display, DisplayState::WithinLimit);
    config.channelById(QStringLiteral("co"))->threshold.high = 30;
    service.reloadConfig(config);
    QVERIFY(coState().hasReading);
    QCOMPARE(coState().display, DisplayState::ExceedsLimit);
    QCOMPARE(coState().reading.rawValue, QStringLiteral("46"));
    service.shutdown();
}

void TestIngestion::sharedRootFiltersCrossStationWithoutNoise()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString root = temp.path() + QStringLiteral("/shared");
    QVERIFY(QDir().mkpath(root + QStringLiteral("/2026/11/18")));
    QVERIFY(QDir().mkpath(root + QStringLiteral("/20261118")));

    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QString ts = tt10::formatTimestamp(now.addSecs(-20));
    const QString gasRel = QStringLiteral("2026/11/18/TNg_NDCN_KHITO1_%1.txt").arg(ts);
    const QString waterRel = QStringLiteral("20261118/TNg_NDCN_NUONT1_%1.txt").arg(ts);

    const QByteArray gasContent = QStringLiteral(
        "SO2\t10.0\tmg/Nm3\t%1\t00\n"
        "NOx\t20.0\tmg/Nm3\t%1\t00\n"
        "PM\t30.0\tmg/Nm3\t%1\t00\n"
        "O2\t5.6\t%V\t%1\t00\n"
        "Temp\t135.0\toC\t%1\t00\n"
        "Flow\t853200.0\tm3/h\t%1\t00\n"
        "P\t92.00\tkPa\t%1\t00\n"
        "CO\t15.0\tmg/Nm3\t%1\t00\n").arg(ts).toUtf8();
    const QByteArray waterContent = QStringLiteral(
        "TSS\t11.34\tmg/L\t%1\t00\n"
        "COD\t12.50\tmg/L\t%1\t00\n"
        "NH4+\t0.37\tmg/L\t%1\t00\n"
        "Temp\t23.37\toC\t%1\t00\n"
        "pH\t7.70\t-\t%1\t00\n"
        "Flow out 1\t0.30\tm3/h\t%1\t00\n"
        "Flow in 1\t0.00\tm3/h\t%1\t00\n"
        "Flow in 2\t0.00\tm3/h\t%1\t00\n").arg(ts).toUtf8();

    QVERIFY(writeInput(root + QLatin1Char('/') + gasRel, gasContent, now));
    QVERIFY(writeInput(root + QLatin1Char('/') + waterRel, waterContent, now));

    AppConfig config = defaultAppConfig();
    config.sources[0].folder = root;
    config.sources[1].folder = root;
    const QString db = temp.filePath(QStringLiteral("snapshot.db"));

    IngestionService service;
    QSignalSpy diagSpy(&service, &IngestionService::diagnosticsBatch);
    service.start(config, db);
    QTest::qWait(1300);
    scanNow(service);
    service.shutdown();

    for (const QVariantList &args : diagSpy) {
        const auto batch = qvariant_cast<QVector<DiagnosticEntry>>(args.at(0));
        for (const DiagnosticEntry &entry : batch) {
            QVERIFY2(entry.code != QLatin1String("other_station"), qPrintable(entry.message));
            QVERIFY2(entry.code != QLatin1String("unmapped_symbol"), qPrintable(entry.message));
        }
    }

    SnapshotStore store(db);
    QVERIFY(store.open());
    QVector<Reading> readings;
    QVERIFY(store.loadReadings(&readings));
    QCOMPARE(readings.size(), 12);

    const QString cleanRoot = QDir::cleanPath(root);
    CheckpointRecord record;
    QVERIFY(store.checkpointFor(QStringLiteral("khi-thai|%1|%2").arg(cleanRoot, gasRel), &record));
    QVERIFY(store.checkpointFor(QStringLiteral("nuoc-thai|%1|%2").arg(cleanRoot, waterRel), &record));
    QVERIFY(!store.checkpointFor(QStringLiteral("nuoc-thai|%1|%2").arg(cleanRoot, gasRel), &record));
    QVERIFY(!store.checkpointFor(QStringLiteral("khi-thai|%1|%2").arg(cleanRoot, waterRel), &record));
}

void TestIngestion::futureTimestampRetriesAndRecoversWithoutPoisoningCheckpoint()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString inputDir = temp.filePath(QStringLiteral("input"));
    QVERIFY(QDir().mkpath(inputDir));

    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QString futureTs = tt10::formatTimestamp(now.addSecs(124));
    const QString rel = QStringLiteral("TNg_NDCN_KHITO1_%1.txt").arg(futureTs);
    const QByteArray content = QStringLiteral("CO\t55\tmg/Nm3\t%1\t00\n").arg(futureTs).toUtf8();
    QVERIFY(writeInput(inputDir + QLatin1Char('/') + rel, content, now));

    AppConfig config = defaultAppConfig();
    config.sources[0].folder = inputDir;
    config.sources[0].futureToleranceSeconds = 120;
    const QString db = temp.filePath(QStringLiteral("snapshot.db"));

    IngestionService service;
    service.start(config, db);
    QTest::qWait(1300);
    scanNow(service);

    {
        SnapshotStore store(db);
        QVERIFY(store.open());
        QVector<Reading> readings;
        QVERIFY(store.loadReadings(&readings));
        QVERIFY(!containsChannel(readings, QStringLiteral("co")));
        CheckpointRecord cp;
        const QString key = QStringLiteral("khi-thai|%1|%2").arg(QDir::cleanPath(inputDir), rel);
        QVERIFY(!store.checkpointFor(key, &cp));
    }

    QTest::qWait(3200);
    scanNow(service);
    service.shutdown();

    SnapshotStore store(db);
    QVERIFY(store.open());
    QVector<Reading> readings;
    QVERIFY(store.loadReadings(&readings));
    QVERIFY(containsChannel(readings, QStringLiteral("co")));
    CheckpointRecord cp;
    const QString key = QStringLiteral("khi-thai|%1|%2").arg(QDir::cleanPath(inputDir), rel);
    QVERIFY(store.checkpointFor(key, &cp));
    QCOMPARE(cp.outcome, QStringLiteral("ok"));
}

QTEST_GUILESS_MAIN(TestIngestion)
#include "tst_ingestion.moc"
