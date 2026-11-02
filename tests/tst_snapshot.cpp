#include "domain/Tt10Time.h"
#include "storage/SnapshotStore.h"

#include <QDateTime>
#include <QFile>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QTest>

using namespace caongan;

namespace {

Reading makeReading(const QString &channelId,
                    const QString &sourceId,
                    const QString &station,
                    const QString &unit,
                    std::optional<double> value,
                    const QDateTime &measuredAtUtc)
{
    Reading reading;
    reading.channelId = channelId;
    reading.sourceId = sourceId;
    reading.stationCode = station;
    reading.rawSymbol = channelId.toUpper();
    reading.rawValue = value.has_value() ? QString::number(*value) : QStringLiteral("---");
    reading.value = value;
    reading.unit = unit;
    reading.measuredAtUtc = measuredAtUtc;
    reading.measuredAtText = tt10::formatTimestamp(measuredAtUtc);
    reading.deviceStatus = DeviceStatus::Measuring;
    reading.validity = value.has_value() ? Validity::Valid : Validity::Missing;
    reading.filePath = QStringLiteral("C:/nguon/file.txt");
    reading.contentHash = QStringLiteral("hash-%1-%2").arg(channelId, reading.measuredAtText);
    reading.receivedAtUtc = measuredAtUtc.addSecs(5);
    return reading;
}

const Reading *findReading(const QVector<Reading> &readings, const QString &id)
{
    for (const Reading &reading : readings) {
        if (reading.channelId == id)
            return &reading;
    }
    return nullptr;
}

} // namespace

class TestSnapshot : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void roundTrip();
    void upsertKeepsOneRowPerChannel();
    void checkpointAndHashLookup();
    void pruneOldCheckpoints();
    void corruptDatabaseFailsOpen();
    void operationsRequireOpen();
};

void TestSnapshot::initTestCase()
{
    QVERIFY2(QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")),
             "Qt SQLite driver là dependency bắt buộc, không được bỏ qua test");
}

void TestSnapshot::roundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SnapshotStore store(dir.filePath(QStringLiteral("data/snapshot.db")));
    QString error;
    QVERIFY2(store.open(&error), qPrintable(error));
    QVERIFY(store.isOpen());

    const QDateTime measured = QDateTime::fromString(QStringLiteral("2026-01-01T05:00:00.000Z"),
                                                     Qt::ISODateWithMs);
    QVERIFY(measured.isValid());

    QVector<Reading> readings;
    readings.append(makeReading(QStringLiteral("co"), QStringLiteral("khi-thai"),
                                QStringLiteral("KHITO1"), QStringLiteral("mg/Nm³"),
                                45.6, measured));
    Reading missing = makeReading(QStringLiteral("ph"), QStringLiteral("nuoc-thai"),
                                  QStringLiteral("NUONT1"), QStringLiteral("-"),
                                  std::nullopt, measured.addSecs(60));
    readings.append(missing);

    CheckpointRecord checkpoint;
    checkpoint.fileKey = QStringLiteral("khi-thai|C:/nguon|file.txt");
    checkpoint.sourceId = QStringLiteral("khi-thai");
    checkpoint.filePath = QStringLiteral("C:/nguon/file.txt");
    checkpoint.size = 123;
    checkpoint.mtimeMs = 456789;
    checkpoint.contentHash = QStringLiteral("abc");
    checkpoint.outcome = QStringLiteral("ok");
    checkpoint.message = QStringLiteral("Đã nhập");
    checkpoint.processedAtUtc = measured.addSecs(10);

    QVERIFY2(store.saveTransaction(readings, {checkpoint}, &error), qPrintable(error));

    QVector<Reading> loaded;
    QVERIFY2(store.loadReadings(&loaded, &error), qPrintable(error));
    QCOMPARE(loaded.size(), 2);

    const Reading *co = findReading(loaded, QStringLiteral("co"));
    QVERIFY(co);
    QCOMPARE(co->sourceId, QStringLiteral("khi-thai"));
    QCOMPARE(co->stationCode, QStringLiteral("KHITO1"));
    QCOMPARE(co->unit, QStringLiteral("mg/Nm³"));
    QVERIFY(co->value.has_value());
    QCOMPARE(*co->value, 45.6);
    QCOMPARE(co->measuredAtUtc.toUTC(), measured);
    QCOMPARE(co->deviceStatus, DeviceStatus::Measuring);
    QCOMPARE(co->validity, Validity::Valid);
    QVERIFY(!co->conflict);
    QCOMPARE(co->contentHash, QStringLiteral("hash-co-20260101120000"));

    const Reading *ph = findReading(loaded, QStringLiteral("ph"));
    QVERIFY(ph);
    QVERIFY(!ph->value.has_value());
    QCOMPARE(ph->validity, Validity::Missing);
}

void TestSnapshot::upsertKeepsOneRowPerChannel()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SnapshotStore store(dir.filePath(QStringLiteral("snapshot.db")));
    QVERIFY(store.open());

    const QDateTime t = QDateTime::fromString(QStringLiteral("2026-01-01T05:00:00Z"), Qt::ISODate);
    QVector<Reading> first;
    first.append(makeReading(QStringLiteral("co"), QStringLiteral("khi-thai"),
                             QStringLiteral("KHITO1"), QStringLiteral("mg/Nm³"), 10.0, t));
    first.append(makeReading(QStringLiteral("ph"), QStringLiteral("nuoc-thai"),
                             QStringLiteral("NUONT1"), QStringLiteral("-"), 7.1, t));
    QVERIFY(store.saveTransaction(first, {}));

    QVector<Reading> second;
    second.append(makeReading(QStringLiteral("co"), QStringLiteral("khi-thai"),
                              QStringLiteral("KHITO1"), QStringLiteral("mg/Nm³"), 20.0, t.addSecs(60)));
    QVERIFY(store.saveTransaction(second, {}));

    QVector<Reading> loaded;
    QVERIFY(store.loadReadings(&loaded));
    QCOMPARE(loaded.size(), 2);
    const Reading *co = findReading(loaded, QStringLiteral("co"));
    QVERIFY(co);
    QCOMPARE(*co->value, 20.0);
    QCOMPARE(co->measuredAtUtc.toUTC(), t.addSecs(60));
}

void TestSnapshot::checkpointAndHashLookup()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SnapshotStore store(dir.filePath(QStringLiteral("snapshot.db")));
    QVERIFY(store.open());

    CheckpointRecord record;
    record.fileKey = QStringLiteral("nuoc-thai|D:/nguon|a.txt");
    record.sourceId = QStringLiteral("nuoc-thai");
    record.filePath = QStringLiteral("D:/nguon/a.txt");
    record.size = 99;
    record.mtimeMs = 111;
    record.contentHash = QStringLiteral("hash-ok");
    record.outcome = QStringLiteral("ok");
    record.processedAtUtc = QDateTime::currentDateTimeUtc();

    CheckpointRecord rejected = record;
    rejected.fileKey = QStringLiteral("nuoc-thai|D:/nguon|b.txt");
    rejected.contentHash = QStringLiteral("hash-rejected");
    rejected.outcome = QStringLiteral("rejected");

    QVERIFY(store.saveTransaction({}, {record, rejected}));

    QVERIFY(store.hasProcessedContent(QStringLiteral("hash-ok")));
    QVERIFY(!store.hasProcessedContent(QStringLiteral("hash-rejected")));
    QVERIFY(!store.hasProcessedContent(QStringLiteral("khong-co")));

    CheckpointRecord loaded;
    QVERIFY(store.checkpointFor(record.fileKey, &loaded));
    QCOMPARE(loaded.sourceId, record.sourceId);
    QCOMPARE(loaded.filePath, record.filePath);
    QCOMPARE(loaded.size, record.size);
    QCOMPARE(loaded.mtimeMs, record.mtimeMs);
    QCOMPARE(loaded.contentHash, record.contentHash);
    QCOMPARE(loaded.outcome, record.outcome);

    CheckpointRecord missing;
    QVERIFY(!store.checkpointFor(QStringLiteral("khong-ton-tai"), &missing));
}

void TestSnapshot::pruneOldCheckpoints()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SnapshotStore store(dir.filePath(QStringLiteral("snapshot.db")));
    QVERIFY(store.open());

    const QDateTime now = QDateTime::currentDateTimeUtc();
    CheckpointRecord old;
    old.fileKey = QStringLiteral("old");
    old.sourceId = QStringLiteral("khi-thai");
    old.contentHash = QStringLiteral("old-hash");
    old.outcome = QStringLiteral("ok");
    old.processedAtUtc = now.addDays(-10);
    QVERIFY(store.saveTransaction({}, {old}));

    QVector<CheckpointRecord> recent;
    for (int i = 0; i < 105; ++i) {
        CheckpointRecord item;
        item.fileKey = QStringLiteral("recent-%1").arg(i);
        item.sourceId = QStringLiteral("khi-thai");
        item.contentHash = QStringLiteral("hash-%1").arg(i);
        item.outcome = QStringLiteral("ok");
        item.processedAtUtc = now.addSecs(i);
        recent.append(item);
    }
    QVERIFY(store.saveTransaction({}, recent));

    QString error;
    QVERIFY2(store.prune(1, 100, &error), qPrintable(error));

    CheckpointRecord loaded;
    QVERIFY(!store.checkpointFor(QStringLiteral("old"), &loaded));
    QVERIFY(store.checkpointFor(QStringLiteral("recent-104"), &loaded));
}

void TestSnapshot::corruptDatabaseFailsOpen()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("snapshot.db"));

    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray(4096, 'x'));
    file.close();

    SnapshotStore store(path);
    QString error;
    QVERIFY(!store.open(&error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!store.isOpen());
}

void TestSnapshot::operationsRequireOpen()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SnapshotStore store(dir.filePath(QStringLiteral("snapshot.db")));

    QVector<Reading> readings;
    QString error;
    QVERIFY(!store.loadReadings(&readings, &error));
    QVERIFY(!store.saveTransaction({}, {}, &error));
    CheckpointRecord record;
    QVERIFY(!store.checkpointFor(QStringLiteral("x"), &record, &error));
    QVERIFY(!store.hasProcessedContent(QStringLiteral("x"), &error));
    QVERIFY(!store.prune(1, 100, &error));
}

QTEST_GUILESS_MAIN(TestSnapshot)
#include "tst_snapshot.moc"
