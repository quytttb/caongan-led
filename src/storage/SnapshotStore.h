#pragma once

#include "domain/Types.h"

#include <QDateTime>
#include <QString>
#include <QVector>

namespace caongan {

struct CheckpointRecord {
    QString fileKey;
    QString sourceId;
    QString filePath;
    qint64 size = 0;
    qint64 mtimeMs = 0;
    QString contentHash;
    QString outcome;   // ok / rejected / skipped_other_station / too_large ...
    QString message;
    QDateTime processedAtUtc;
};

// SQLite tối giản: snapshot 12 kênh + checkpoint file. Không phải kho lịch sử.
class SnapshotStore
{
public:
    explicit SnapshotStore(QString databasePath = {});
    ~SnapshotStore();

    SnapshotStore(const SnapshotStore &) = delete;
    SnapshotStore &operator=(const SnapshotStore &) = delete;

    bool open(QString *error = nullptr);
    void close();
    bool isOpen() const;

    bool loadReadings(QVector<Reading> *readings, QString *error = nullptr);
    bool saveTransaction(const QVector<Reading> &readings,
                         const QVector<CheckpointRecord> &checkpoints,
                         QString *error = nullptr);
    bool checkpointFor(const QString &fileKey, CheckpointRecord *record, QString *error = nullptr);
    bool hasProcessedContent(const QString &contentHash, QString *error = nullptr);
    bool prune(int retentionDays, int maxRows, QString *error = nullptr);

    QString databasePath() const { return m_databasePath; }
    QString lastError() const { return m_lastError; }

private:
    bool ensureSchema(QString *error);
    void setError(const QString &message);

    QString m_databasePath;
    QString m_connectionName;
    QString m_lastError;
    bool m_open = false;
};

} // namespace caongan
