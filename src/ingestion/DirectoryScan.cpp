#include "DirectoryScan.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>
#include <queue>
#include <vector>

namespace caongan {

namespace {

bool isTemporaryName(const QString &name)
{
    const QString lower = name.toLower();
    return lower.endsWith(QLatin1String(".tmp"))
           || lower.endsWith(QLatin1String(".part"))
           || lower.endsWith(QLatin1String(".filepart"))
           || lower.endsWith(QLatin1String(".crdownload"))
           || lower.endsWith(QLatin1String(".partial"))
           || name.endsWith(QLatin1Char('~'));
}

struct DirEntry {
    QString absPath;
    QString relPath;
    int depth = 0;
    qint64 dateUpperBound = 99999999LL;
    qint64 mtimeMs = 0;
};

struct DirEntryCompare {
    bool operator()(const DirEntry &a, const DirEntry &b) const
    {
        if (a.dateUpperBound != b.dateUpperBound)
            return a.dateUpperBound < b.dateUpperBound;
        if (a.mtimeMs != b.mtimeMs)
            return a.mtimeMs < b.mtimeMs;
        return a.relPath < b.relPath;
    }
};

qint64 computeDateUpperBound(const QString &relPath)
{
    if (relPath.isEmpty())
        return 99999999LL;

    const QStringList segments = relPath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (segments.isEmpty())
        return 99999999LL;

    static const QRegularExpression eightDigits(QStringLiteral("^\\d{8}$"));
    static const QRegularExpression fourDigits(QStringLiteral("^\\d{4}$"));
    static const QRegularExpression oneOrTwoDigits(QStringLiteral("^\\d{1,2}$"));

    const QString &seg0 = segments.at(0);
    if (eightDigits.match(seg0).hasMatch())
        return seg0.toLongLong();

    if (fourDigits.match(seg0).hasMatch()) {
        const qint64 year = seg0.toLongLong();
        if (segments.size() == 1)
            return year * 10000LL + 1231LL;
        const QString &seg1 = segments.at(1);
        if (segments.size() == 2 && oneOrTwoDigits.match(seg1).hasMatch())
            return year * 10000LL + seg1.toLongLong() * 100LL + 31LL;
        if (segments.size() >= 3 && oneOrTwoDigits.match(seg1).hasMatch()
            && oneOrTwoDigits.match(segments.at(2)).hasMatch()) {
            return year * 10000LL + seg1.toLongLong() * 100LL + segments.at(2).toLongLong();
        }
        return 99999999LL;
    }

    return 99999999LL;
}

void scanPrioritized(const QDir &root,
                     const QString &rootPath,
                     const SourceConfig &source,
                     int maxEntries,
                     QVector<FileCandidate> *out,
                     bool *truncated,
                     QStringList *issues)
{
    const QString cleanRoot = QDir::cleanPath(rootPath);
    const int maxQueueSize = qMax(maxEntries * 10, 50000);
    int visited = 0;

    std::priority_queue<DirEntry, std::vector<DirEntry>, DirEntryCompare> queue;
    DirEntry rootEntry;
    rootEntry.absPath = root.absolutePath();
    rootEntry.relPath = QString();
    rootEntry.depth = 0;
    rootEntry.dateUpperBound = 99999999LL;
    rootEntry.mtimeMs = QFileInfo(rootEntry.absPath).lastModified().toMSecsSinceEpoch();
    queue.push(rootEntry);

    while (!queue.empty() && !*truncated) {
        const DirEntry entry = queue.top();
        queue.pop();

        if (entry.depth > source.maxDepth)
            continue;

        if (entry.depth > 0) {
            if (++visited > maxEntries) {
                *truncated = true;
                issues->append(QStringLiteral("Quét vượt giới hạn %1 entry — không quét hết; cần rút gọn thư mục nguồn")
                                   .arg(maxEntries));
                break;
            }
        }

        const QDir dir(entry.absPath);
        const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
        for (const QFileInfo &info : files) {
            if (++visited > maxEntries) {
                *truncated = true;
                issues->append(QStringLiteral("Quét vượt giới hạn %1 entry — không quét hết; cần rút gọn thư mục nguồn")
                                   .arg(maxEntries));
                break;
            }

            const QString name = info.fileName();
            if (isTemporaryName(name))
                continue;
            if (!name.endsWith(QLatin1String(".txt"), Qt::CaseInsensitive))
                continue;
            if (!info.isFile() || info.isSymLink())
                continue;
            if (info.size() <= 0)
                continue;

            FileCandidate candidate;
            candidate.absolutePath = QDir::cleanPath(info.absoluteFilePath());
            candidate.relativePath = QDir(rootPath).relativeFilePath(candidate.absolutePath);
            candidate.relativePath.replace(QLatin1Char('\\'), QLatin1Char('/'));
            // Khóa checkpoint phải gồm cả root đã cấu hình: đổi thư mục nguồn
            // không được coi file trùng tên/tương đối là cùng một file.
            candidate.fileKey = source.id + QLatin1Char('|') + cleanRoot + QLatin1Char('|')
                                + candidate.relativePath;
            candidate.size = info.size();
            candidate.mtimeMs = info.lastModified().toMSecsSinceEpoch();
            out->append(candidate);
        }

        if (*truncated || !source.recursive || entry.depth >= source.maxDepth)
            continue;

        const QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                        QDir::Name | QDir::Reversed);
        for (const QFileInfo &info : subdirs) {
            if (info.fileName().startsWith(QLatin1Char('.')))
                continue;
            if (info.isSymLink())
                continue;
            if (int(queue.size()) >= maxQueueSize)
                break;

            const QString absSub = QDir::cleanPath(info.absoluteFilePath());
            QString relSub = QDir(rootPath).relativeFilePath(absSub);
            relSub.replace(QLatin1Char('\\'), QLatin1Char('/'));

            DirEntry child;
            child.absPath = absSub;
            child.relPath = relSub;
            child.depth = entry.depth + 1;
            child.dateUpperBound = computeDateUpperBound(relSub);
            child.mtimeMs = info.lastModified().toMSecsSinceEpoch();
            queue.push(child);
        }
    }
}

} // namespace

DirectoryScanResult scanSourceFolder(const SourceConfig &source, int maxEntries)
{
    DirectoryScanResult result;
    if (source.folder.trimmed().isEmpty()) {
        result.issues.append(QStringLiteral("Nguồn %1 chưa cấu hình thư mục").arg(source.name));
        return result;
    }

    QDir root(source.folder);
    if (!root.exists()) {
        result.issues.append(QStringLiteral("Thư mục nguồn không tồn tại/không truy cập được: %1").arg(source.folder));
        return result;
    }
    result.rootAvailable = true;

    scanPrioritized(root, root.absolutePath(), source, maxEntries,
                    &result.candidates, &result.truncated, &result.issues);

    std::stable_sort(result.candidates.begin(), result.candidates.end(),
                     [](const FileCandidate &a, const FileCandidate &b) {
                         if (a.mtimeMs != b.mtimeMs)
                             return a.mtimeMs > b.mtimeMs;
                         return a.relativePath < b.relativePath;
                     });
    return result;
}

FileStabilityTracker::Status FileStabilityTracker::observe(const QString &fileKey,
                                                           qint64 size,
                                                           qint64 mtimeMs,
                                                           const QDateTime &nowUtc,
                                                           int minStableMs)
{
    const qint64 nowMs = nowUtc.toMSecsSinceEpoch();
    auto it = m_observations.find(fileKey);
    if (it == m_observations.end()) {
        Observation obs;
        obs.size = size;
        obs.mtimeMs = mtimeMs;
        obs.firstSeenMs = nowMs;
        obs.observations = 1;
        m_observations.insert(fileKey, obs);
        return Status::Waiting;
    }

    Observation &obs = it.value();
    if (obs.size != size || obs.mtimeMs != mtimeMs) {
        obs.size = size;
        obs.mtimeMs = mtimeMs;
        obs.firstSeenMs = nowMs;
        obs.observations = 1;
        return Status::Changed;
    }

    ++obs.observations;
    if (obs.observations >= 2 && nowMs - obs.firstSeenMs >= qMax(0, minStableMs))
        return Status::Stable;
    return Status::Waiting;
}

void FileStabilityTracker::forget(const QString &fileKey)
{
    m_observations.remove(fileKey);
}

void FileStabilityTracker::pruneMissing(const QSet<QString> &seenKeys)
{
    const auto keys = m_observations.keys();
    for (const QString &key : keys) {
        if (!seenKeys.contains(key))
            m_observations.remove(key);
    }
}

void FileStabilityTracker::clear()
{
    m_observations.clear();
}

} // namespace caongan
