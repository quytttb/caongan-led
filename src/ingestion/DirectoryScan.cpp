#include "DirectoryScan.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <algorithm>

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

void walkDirectory(const QDir &dir,
                   const QString &rootPath,
                   const SourceConfig &source,
                   int depth,
                   int maxEntries,
                   QVector<FileCandidate> *out,
                   int *visited,
                   bool *truncated,
                   QStringList *issues)
{
    if (*truncated)
        return;
    if (depth > source.maxDepth)
        return;

    QDirIterator files(dir.absolutePath(), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);
    while (files.hasNext()) {
        const QString path = files.next();
        if (++(*visited) > maxEntries) {
            *truncated = true;
            issues->append(QStringLiteral("Quét vượt giới hạn %1 entry — không quét hết; cần rút gọn thư mục nguồn").arg(maxEntries));
            return;
        }

        const QFileInfo info(path);
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
        candidate.fileKey = source.id + QLatin1Char('|')
                            + QDir::cleanPath(rootPath) + QLatin1Char('|')
                            + candidate.relativePath;
        candidate.size = info.size();
        candidate.mtimeMs = info.lastModified().toMSecsSinceEpoch();
        out->append(candidate);
    }

    if (!source.recursive || depth >= source.maxDepth)
        return;

    QDirIterator dirs(dir.absolutePath(), QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::NoIteratorFlags);
    while (dirs.hasNext()) {
        const QString path = dirs.next();
        const QFileInfo info(path);
        if (info.fileName().startsWith(QLatin1Char('.')))
            continue;
        if (info.isSymLink())
            continue;
        if (++(*visited) > maxEntries) {
            *truncated = true;
            issues->append(QStringLiteral("Quét vượt giới hạn %1 entry — không quét hết; cần rút gọn thư mục nguồn").arg(maxEntries));
            return;
        }
        walkDirectory(QDir(path), rootPath, source, depth + 1, maxEntries, out, visited, truncated, issues);
        if (*truncated)
            return;
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

    int visited = 0;
    walkDirectory(root, root.absolutePath(), source, 0, maxEntries,
                  &result.candidates, &visited, &result.truncated, &result.issues);

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
