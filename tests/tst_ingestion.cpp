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

QTEST_GUILESS_MAIN(TestIngestion)
#include "tst_ingestion.moc"
