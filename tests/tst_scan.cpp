#include "ingestion/DirectoryScan.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>

using namespace caongan;

namespace {

bool writeFile(const QString &path, const QByteArray &content)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(content) == content.size();
}

SourceConfig sourceFor(const QString &folder)
{
    SourceConfig source;
    source.id = QStringLiteral("khi-thai");
    source.name = QStringLiteral("Khí thải");
    source.stationCode = QStringLiteral("KHITO1");
    source.folder = folder;
    source.recursive = true;
    source.maxDepth = 4;
    return source;
}

QSet<QString> relativePaths(const DirectoryScanResult &result)
{
    QSet<QString> paths;
    for (const FileCandidate &candidate : result.candidates)
        paths.insert(candidate.relativePath);
    return paths;
}

} // namespace

class TestScan : public QObject
{
    Q_OBJECT

private slots:
    void scansOnlyStableTxtFiles();
    void honorsDepthAndRecursive();
    void reportsMissingOrEmptyFolder();
    void truncatesAtEntryLimit();
    void preservesSourceFiles();
    void candidatesSortedByMtime();
    void stabilityNeedsTwoObservations();
    void stabilityDetectsChange();
    void stabilityForgetAndPrune();
};

void TestScan::scansOnlyStableTxtFiles()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString root = dir.path();
    QVERIFY(writeFile(root + QStringLiteral("/TNg_NDCN_KHITO1_20260101120000.txt"),
                      QByteArrayLiteral("O2\t12.3\t%\t20260101120000\t00\n")));
    QVERIFY(writeFile(root + QStringLiteral("/sub/TNg_NDCN_KHITO1_20260101120100.txt"),
                      QByteArrayLiteral("O2\t12.4\t%\t20260101120100\t00\n")));
    QVERIFY(writeFile(root + QStringLiteral("/sub/deep/TNg_NDCN_KHITO1_20260101120200.txt"),
                      QByteArrayLiteral("O2\t12.5\t%\t20260101120200\t00\n")));
    QVERIFY(writeFile(root + QStringLiteral("/empty.txt"), QByteArray()));
    QVERIFY(writeFile(root + QStringLiteral("/note.md"), QByteArrayLiteral("not tt10")));
    QVERIFY(writeFile(root + QStringLiteral("/dang-ghi.tmp"), QByteArrayLiteral("x")));
    QVERIFY(writeFile(root + QStringLiteral("/dang-ghi.part"), QByteArrayLiteral("x")));
    QVERIFY(writeFile(root + QStringLiteral("/.hidden/TNg_NDCN_KHITO1_20260101120300.txt"),
                      QByteArrayLiteral("hidden")));

    const DirectoryScanResult result = scanSourceFolder(sourceFor(root), 5000);
    QVERIFY(result.rootAvailable);
    QVERIFY(!result.truncated);
    QCOMPARE(result.candidates.size(), 3);

    const QSet<QString> paths = relativePaths(result);
    QVERIFY(paths.contains(QStringLiteral("TNg_NDCN_KHITO1_20260101120000.txt")));
    QVERIFY(paths.contains(QStringLiteral("sub/TNg_NDCN_KHITO1_20260101120100.txt")));
    QVERIFY(paths.contains(QStringLiteral("sub/deep/TNg_NDCN_KHITO1_20260101120200.txt")));

    const QString cleanRoot = QDir::cleanPath(root);
    for (const FileCandidate &candidate : result.candidates) {
        QVERIFY(candidate.size > 0);
        QVERIFY(!candidate.relativePath.contains(QLatin1Char('\\')));
        QCOMPARE(candidate.fileKey,
                 QStringLiteral("khi-thai|%1|%2").arg(cleanRoot, candidate.relativePath));
    }
}

void TestScan::honorsDepthAndRecursive()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString root = dir.path();
    QVERIFY(writeFile(root + QStringLiteral("/root.txt"), QByteArrayLiteral("x")));
    QVERIFY(writeFile(root + QStringLiteral("/sub/child.txt"), QByteArrayLiteral("x")));

    SourceConfig source = sourceFor(root);
    source.maxDepth = 0;
    DirectoryScanResult result = scanSourceFolder(source);
    QCOMPARE(result.candidates.size(), 1);
    QCOMPARE(result.candidates.constFirst().relativePath, QStringLiteral("root.txt"));

    source = sourceFor(root);
    source.recursive = false;
    result = scanSourceFolder(source);
    QCOMPARE(result.candidates.size(), 1);
    QCOMPARE(result.candidates.constFirst().relativePath, QStringLiteral("root.txt"));

    source = sourceFor(root);
    source.maxDepth = 1;
    result = scanSourceFolder(source);
    QCOMPARE(result.candidates.size(), 2);
}

void TestScan::reportsMissingOrEmptyFolder()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    SourceConfig emptyFolder = sourceFor(QString());
    DirectoryScanResult result = scanSourceFolder(emptyFolder);
    QVERIFY(!result.rootAvailable);
    QVERIFY(!result.issues.isEmpty());

    result = scanSourceFolder(sourceFor(dir.filePath(QStringLiteral("khong-ton-tai"))));
    QVERIFY(!result.rootAvailable);
    QVERIFY(!result.issues.isEmpty());
}

void TestScan::truncatesAtEntryLimit()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    for (int i = 0; i < 5; ++i) {
        QVERIFY(writeFile(dir.filePath(QStringLiteral("file-%1.txt").arg(i)),
                          QByteArrayLiteral("x")));
    }

    const DirectoryScanResult result = scanSourceFolder(sourceFor(dir.path()), 2);
    QVERIFY(result.rootAvailable);
    QVERIFY(result.truncated);
    QVERIFY(result.candidates.size() <= 2);
    QVERIFY(!result.issues.isEmpty());
}

void TestScan::preservesSourceFiles()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("TNg_NDCN_KHITO1_20260101120000.txt"));
    QVERIFY(writeFile(path, QByteArrayLiteral("O2\t12.3\t%\t20260101120000\t00\n")));

    QFileInfo before(path);
    const QByteArray contentBefore = [&path]() {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }();
    const QDateTime mtimeBefore = before.lastModified();
    const QStringList entriesBefore = QDir(dir.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot);

    const DirectoryScanResult result = scanSourceFolder(sourceFor(dir.path()));
    QCOMPARE(result.candidates.size(), 1);

    QFileInfo after(path);
    QVERIFY(after.exists());
    QCOMPARE(after.size(), before.size());
    QCOMPARE(after.lastModified(), mtimeBefore);
    const QByteArray contentAfter = [&path]() {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }();
    QCOMPARE(contentAfter, contentBefore);
    QCOMPARE(QDir(dir.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot), entriesBefore);
}

void TestScan::candidatesSortedByMtime()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    for (int i = 0; i < 4; ++i) {
        QVERIFY(writeFile(dir.filePath(QStringLiteral("file-%1.txt").arg(i)),
                          QByteArrayLiteral("x")));
    }
    const DirectoryScanResult result = scanSourceFolder(sourceFor(dir.path()));
    QCOMPARE(result.candidates.size(), 4);
    for (int i = 1; i < result.candidates.size(); ++i)
        QVERIFY(result.candidates.at(i - 1).mtimeMs >= result.candidates.at(i).mtimeMs);
}

void TestScan::stabilityNeedsTwoObservations()
{
    FileStabilityTracker tracker;
    const QString key = QStringLiteral("khi-thai|/root|a.txt");
    const QDateTime t0 = QDateTime::fromString(QStringLiteral("2026-01-01T00:00:00Z"), Qt::ISODate);

    QCOMPARE(tracker.observe(key, 100, 1000, t0), FileStabilityTracker::Status::Waiting);
    QCOMPARE(tracker.observe(key, 100, 1000, t0.addMSecs(500)),
             FileStabilityTracker::Status::Waiting);
    QCOMPARE(tracker.observe(key, 100, 1000, t0.addMSecs(1200)),
             FileStabilityTracker::Status::Stable);
}

void TestScan::stabilityDetectsChange()
{
    FileStabilityTracker tracker;
    const QString key = QStringLiteral("khi-thai|/root|a.txt");
    const QDateTime t0 = QDateTime::fromString(QStringLiteral("2026-01-01T00:00:00Z"), Qt::ISODate);

    QCOMPARE(tracker.observe(key, 100, 1000, t0), FileStabilityTracker::Status::Waiting);
    QCOMPARE(tracker.observe(key, 101, 1000, t0.addMSecs(500)),
             FileStabilityTracker::Status::Changed);
    QCOMPARE(tracker.observe(key, 101, 1000, t0.addMSecs(1700)),
             FileStabilityTracker::Status::Stable);
}

void TestScan::stabilityForgetAndPrune()
{
    FileStabilityTracker tracker;
    const QDateTime t0 = QDateTime::currentDateTimeUtc();
    const QString a = QStringLiteral("a");
    const QString b = QStringLiteral("b");

    QCOMPARE(tracker.observe(a, 1, 1, t0), FileStabilityTracker::Status::Waiting);
    QCOMPARE(tracker.observe(b, 1, 1, t0), FileStabilityTracker::Status::Waiting);

    tracker.forget(a);
    QCOMPARE(tracker.observe(a, 1, 1, t0), FileStabilityTracker::Status::Waiting);

    tracker.pruneMissing({b});
    QCOMPARE(tracker.observe(a, 1, 1, t0), FileStabilityTracker::Status::Waiting);
    QCOMPARE(tracker.observe(b, 1, 1, t0.addMSecs(10)), FileStabilityTracker::Status::Waiting);

    tracker.clear();
    QCOMPARE(tracker.observe(b, 1, 1, t0), FileStabilityTracker::Status::Waiting);
}

QTEST_GUILESS_MAIN(TestScan)
#include "tst_scan.moc"
