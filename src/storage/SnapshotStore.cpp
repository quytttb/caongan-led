#include "SnapshotStore.h"
#include "domain/ParameterCatalog.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>
#include <QVariant>

#include <utility>

namespace caongan {

namespace {

constexpr int kSchemaVersion = 1;

QString iso(const QDateTime &dt)
{
    return dt.isValid() ? dt.toUTC().toString(Qt::ISODateWithMs) : QString();
}

QDateTime fromIso(const QVariant &value)
{
    if (!value.isValid() || value.isNull())
        return {};
    return QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
}

QString deviceStatusText(DeviceStatus status)
{
    return deviceStatusToCode(status);
}

DeviceStatus deviceStatusFromText(const QString &text)
{
    return deviceStatusFromCode(text);
}

QString validityText(Validity validity)
{
    switch (validity) {
    case Validity::Valid:        return QStringLiteral("valid");
    case Validity::Missing:      return QStringLiteral("missing");
    case Validity::Invalid:      return QStringLiteral("invalid");
    case Validity::UnitMismatch: return QStringLiteral("unit_mismatch");
    case Validity::Conflict:     return QStringLiteral("conflict");
    }
    return QStringLiteral("invalid");
}

Validity validityFromText(const QString &text)
{
    if (text == QLatin1String("valid"))
        return Validity::Valid;
    if (text == QLatin1String("missing"))
        return Validity::Missing;
    if (text == QLatin1String("unit_mismatch"))
        return Validity::UnitMismatch;
    if (text == QLatin1String("conflict"))
        return Validity::Conflict;
    return Validity::Invalid;
}

} // namespace

SnapshotStore::SnapshotStore(QString databasePath)
    : m_databasePath(databasePath.isEmpty() ? QStringLiteral("snapshot.db") : std::move(databasePath))
{
}

SnapshotStore::~SnapshotStore()
{
    close();
}

void SnapshotStore::setError(const QString &message)
{
    m_lastError = message;
}

bool SnapshotStore::open(QString *error)
{
    if (m_open)
        return true;

    QDir().mkpath(QFileInfo(m_databasePath).absolutePath());
    m_connectionName = QStringLiteral("caongan_led_snapshot_%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_databasePath);
    if (!db.open()) {
        setError(QStringLiteral("Không mở được SQLite %1: %2").arg(m_databasePath, db.lastError().text()));
        if (error)
            *error = m_lastError;
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return false;
    }

    QSqlQuery pragma(db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    pragma.exec(QStringLiteral("PRAGMA busy_timeout=3000"));

    if (!ensureSchema(error)) {
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
        return false;
    }

    m_open = true;
    return true;
}

bool SnapshotStore::ensureSchema(QString *error)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS latest_reading ("
            " channel_id TEXT PRIMARY KEY,"
            " source_id TEXT NOT NULL,"
            " station_code TEXT,"
            " raw_symbol TEXT,"
            " raw_value TEXT,"
            " value_num REAL,"
            " unit TEXT,"
            " measured_at_utc TEXT NOT NULL,"
            " measured_at_text TEXT,"
            " device_status TEXT,"
            " validity TEXT,"
            " conflict INTEGER NOT NULL DEFAULT 0,"
            " issue_code TEXT,"
            " issue_message TEXT,"
            " file_path TEXT,"
            " content_hash TEXT,"
            " received_at_utc TEXT)"))) {
        setError(QStringLiteral("Tạo bảng latest_reading thất bại: %1").arg(query.lastError().text()));
        if (error)
            *error = m_lastError;
        return false;
    }

    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS checkpoint ("
            " file_key TEXT PRIMARY KEY,"
            " source_id TEXT,"
            " file_path TEXT,"
            " size INTEGER,"
            " mtime_ms INTEGER,"
            " content_hash TEXT,"
            " outcome TEXT,"
            " message TEXT,"
            " processed_at_utc TEXT)"))) {
        setError(QStringLiteral("Tạo bảng checkpoint thất bại: %1").arg(query.lastError().text()));
        if (error)
            *error = m_lastError;
        return false;
    }

    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_checkpoint_hash "
                              "ON checkpoint(content_hash, outcome)"));
    query.exec(QStringLiteral("PRAGMA user_version=%1").arg(kSchemaVersion));
    return true;
}

void SnapshotStore::close()
{
    if (m_connectionName.isEmpty())
        return;
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
    m_connectionName.clear();
    m_open = false;
}

bool SnapshotStore::isOpen() const
{
    return m_open;
}

bool SnapshotStore::loadReadings(QVector<Reading> *readings, QString *error)
{
    if (!m_open) {
        if (error)
            *error = QStringLiteral("SQLite chưa mở");
        return false;
    }
    if (!readings)
        return false;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT * FROM latest_reading"))) {
        setError(QStringLiteral("Đọc snapshot thất bại: %1").arg(query.lastError().text()));
        if (error)
            *error = m_lastError;
        return false;
    }

    while (query.next()) {
        Reading reading;
        reading.channelId = query.value(QStringLiteral("channel_id")).toString();
        reading.sourceId = query.value(QStringLiteral("source_id")).toString();
        reading.stationCode = query.value(QStringLiteral("station_code")).toString();
        reading.rawSymbol = query.value(QStringLiteral("raw_symbol")).toString();
        reading.rawValue = query.value(QStringLiteral("raw_value")).toString();
        const QVariant numeric = query.value(QStringLiteral("value_num"));
        if (!numeric.isNull())
            reading.value = numeric.toDouble();
        reading.unit = query.value(QStringLiteral("unit")).toString();
        reading.measuredAtUtc = fromIso(query.value(QStringLiteral("measured_at_utc")));
        reading.measuredAtText = query.value(QStringLiteral("measured_at_text")).toString();
        reading.deviceStatus = deviceStatusFromText(query.value(QStringLiteral("device_status")).toString());
        reading.validity = validityFromText(query.value(QStringLiteral("validity")).toString());
        reading.conflict = query.value(QStringLiteral("conflict")).toInt() != 0;
        reading.issueCode = query.value(QStringLiteral("issue_code")).toString();
        reading.issueMessage = query.value(QStringLiteral("issue_message")).toString();
        reading.filePath = query.value(QStringLiteral("file_path")).toString();
        reading.contentHash = query.value(QStringLiteral("content_hash")).toString();
        reading.receivedAtUtc = fromIso(query.value(QStringLiteral("received_at_utc")));
        readings->append(reading);
    }
    return true;
}

bool SnapshotStore::saveTransaction(const QVector<Reading> &readings,
                                    const QVector<CheckpointRecord> &checkpoints,
                                    QString *error)
{
    if (!m_open) {
        if (error)
            *error = QStringLiteral("SQLite chưa mở");
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    if (!db.transaction()) {
        setError(QStringLiteral("Bắt đầu transaction thất bại: %1").arg(db.lastError().text()));
        if (error)
            *error = m_lastError;
        return false;
    }

    QSqlQuery upsert(db);
    upsert.prepare(QStringLiteral(
        "INSERT INTO latest_reading (channel_id, source_id, station_code, raw_symbol, raw_value,"
        " value_num, unit, measured_at_utc, measured_at_text, device_status, validity, conflict,"
        " issue_code, issue_message, file_path, content_hash, received_at_utc)"
        " VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        " ON CONFLICT(channel_id) DO UPDATE SET"
        " source_id=excluded.source_id, station_code=excluded.station_code,"
        " raw_symbol=excluded.raw_symbol, raw_value=excluded.raw_value, value_num=excluded.value_num,"
        " unit=excluded.unit, measured_at_utc=excluded.measured_at_utc,"
        " measured_at_text=excluded.measured_at_text, device_status=excluded.device_status,"
        " validity=excluded.validity, conflict=excluded.conflict, issue_code=excluded.issue_code,"
        " issue_message=excluded.issue_message, file_path=excluded.file_path,"
        " content_hash=excluded.content_hash, received_at_utc=excluded.received_at_utc"));

    for (const Reading &reading : readings) {
        upsert.addBindValue(reading.channelId);
        upsert.addBindValue(reading.sourceId);
        upsert.addBindValue(reading.stationCode);
        upsert.addBindValue(reading.rawSymbol);
        upsert.addBindValue(reading.rawValue);
        upsert.addBindValue(reading.value.has_value() ? QVariant(*reading.value) : QVariant());
        upsert.addBindValue(reading.unit);
        upsert.addBindValue(iso(reading.measuredAtUtc));
        upsert.addBindValue(reading.measuredAtText);
        upsert.addBindValue(deviceStatusText(reading.deviceStatus));
        upsert.addBindValue(validityText(reading.validity));
        upsert.addBindValue(reading.conflict ? 1 : 0);
        upsert.addBindValue(reading.issueCode);
        upsert.addBindValue(reading.issueMessage);
        upsert.addBindValue(reading.filePath);
        upsert.addBindValue(reading.contentHash);
        upsert.addBindValue(iso(reading.receivedAtUtc));
        if (!upsert.exec()) {
            setError(QStringLiteral("Ghi snapshot thất bại: %1").arg(upsert.lastError().text()));
            db.rollback();
            if (error)
                *error = m_lastError;
            return false;
        }
    }

    QSqlQuery checkpoint(db);
    checkpoint.prepare(QStringLiteral(
        "INSERT INTO checkpoint (file_key, source_id, file_path, size, mtime_ms, content_hash,"
        " outcome, message, processed_at_utc) VALUES (?,?,?,?,?,?,?,?,?)"
        " ON CONFLICT(file_key) DO UPDATE SET"
        " source_id=excluded.source_id, file_path=excluded.file_path, size=excluded.size,"
        " mtime_ms=excluded.mtime_ms, content_hash=excluded.content_hash, outcome=excluded.outcome,"
        " message=excluded.message, processed_at_utc=excluded.processed_at_utc"));
    for (const CheckpointRecord &item : checkpoints) {
        checkpoint.addBindValue(item.fileKey);
        checkpoint.addBindValue(item.sourceId);
        checkpoint.addBindValue(item.filePath);
        checkpoint.addBindValue(item.size);
        checkpoint.addBindValue(item.mtimeMs);
        checkpoint.addBindValue(item.contentHash);
        checkpoint.addBindValue(item.outcome);
        checkpoint.addBindValue(item.message);
        checkpoint.addBindValue(iso(item.processedAtUtc));
        if (!checkpoint.exec()) {
            setError(QStringLiteral("Ghi checkpoint thất bại: %1").arg(checkpoint.lastError().text()));
            db.rollback();
            if (error)
                *error = m_lastError;
            return false;
        }
    }

    if (!db.commit()) {
        setError(QStringLiteral("Commit transaction thất bại: %1").arg(db.lastError().text()));
        db.rollback();
        if (error)
            *error = m_lastError;
        return false;
    }
    return true;
}

bool SnapshotStore::checkpointFor(const QString &fileKey, CheckpointRecord *record, QString *error)
{
    if (!m_open || !record) {
        if (error)
            *error = QStringLiteral("SQLite chưa mở");
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT * FROM checkpoint WHERE file_key=?"));
    query.addBindValue(fileKey);
    if (!query.exec()) {
        setError(QStringLiteral("Đọc checkpoint thất bại: %1").arg(query.lastError().text()));
        if (error)
            *error = m_lastError;
        return false;
    }
    if (!query.next())
        return false;

    record->fileKey = query.value(QStringLiteral("file_key")).toString();
    record->sourceId = query.value(QStringLiteral("source_id")).toString();
    record->filePath = query.value(QStringLiteral("file_path")).toString();
    record->size = query.value(QStringLiteral("size")).toLongLong();
    record->mtimeMs = query.value(QStringLiteral("mtime_ms")).toLongLong();
    record->contentHash = query.value(QStringLiteral("content_hash")).toString();
    record->outcome = query.value(QStringLiteral("outcome")).toString();
    record->message = query.value(QStringLiteral("message")).toString();
    record->processedAtUtc = fromIso(query.value(QStringLiteral("processed_at_utc")));
    return true;
}

bool SnapshotStore::hasProcessedContent(const QString &contentHash, QString *error)
{
    if (!m_open || contentHash.isEmpty()) {
        if (error)
            *error = QStringLiteral("SQLite chưa mở");
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT 1 FROM checkpoint WHERE content_hash=? AND outcome='ok' LIMIT 1"));
    query.addBindValue(contentHash);
    if (!query.exec()) {
        setError(QStringLiteral("Kiểm tra hash đã xử lý thất bại: %1").arg(query.lastError().text()));
        if (error)
            *error = m_lastError;
        return false;
    }
    return query.next();
}

bool SnapshotStore::prune(int retentionDays, int maxRows, QString *error)
{
    if (!m_open) {
        if (error)
            *error = QStringLiteral("SQLite chưa mở");
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    if (!db.transaction()) {
        if (error)
            *error = db.lastError().text();
        return false;
    }

    QSqlQuery old(db);
    old.prepare(QStringLiteral("DELETE FROM checkpoint WHERE processed_at_utc < ?"));
    old.addBindValue(iso(QDateTime::currentDateTimeUtc().addDays(-qMax(1, retentionDays))));
    if (!old.exec()) {
        setError(QStringLiteral("Prune checkpoint theo ngày thất bại: %1").arg(old.lastError().text()));
        db.rollback();
        if (error)
            *error = m_lastError;
        return false;
    }

    QSqlQuery count(db);
    count.prepare(QStringLiteral("DELETE FROM checkpoint WHERE file_key IN ("
                                 " SELECT file_key FROM checkpoint ORDER BY processed_at_utc ASC"
                                 " LIMIT MAX(0, (SELECT COUNT(*) FROM checkpoint) - ?))"));
    count.addBindValue(qMax(100, maxRows));
    if (!count.exec()) {
        setError(QStringLiteral("Prune checkpoint theo số dòng thất bại: %1").arg(count.lastError().text()));
        db.rollback();
        if (error)
            *error = m_lastError;
        return false;
    }

    if (!db.commit()) {
        setError(QStringLiteral("Commit prune thất bại: %1").arg(db.lastError().text()));
        db.rollback();
        if (error)
            *error = m_lastError;
        return false;
    }
    return true;
}

} // namespace caongan
