#pragma once

#include "domain/Types.h"

#include <QDateTime>
#include <QHash>
#include <QSet>
#include <QString>
#include <QVector>

namespace caongan {

struct FileCandidate {
    QString absolutePath;
    QString relativePath;
    QString fileKey;
    qint64 size = 0;
    qint64 mtimeMs = 0;
};

struct DirectoryScanResult {
    bool rootAvailable = false;
    bool truncated = false;
    QVector<FileCandidate> candidates;
    QStringList issues;
};

// Quét chỉ đọc, có giới hạn độ sâu/số entry. Không move/delete file nguồn.
DirectoryScanResult scanSourceFolder(const SourceConfig &source, int maxEntries = 5000);

class FileStabilityTracker
{
public:
    enum class Status {
        Waiting,
        Stable,
        Changed,
    };

    Status observe(const QString &fileKey,
                   qint64 size,
                   qint64 mtimeMs,
                   const QDateTime &nowUtc,
                   int minStableMs = 1200);
    void forget(const QString &fileKey);
    void pruneMissing(const QSet<QString> &seenKeys);
    void clear();

private:
    struct Observation {
        qint64 size = -1;
        qint64 mtimeMs = -1;
        qint64 firstSeenMs = 0;
        int observations = 0;
    };
    QHash<QString, Observation> m_observations;
};

} // namespace caongan
