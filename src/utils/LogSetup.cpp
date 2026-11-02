#include "LogSetup.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>

#include <cstdio>

namespace caongan {

namespace {

constexpr int kKeepRotatedPerDay = 5;

struct LogState {
    QMutex mutex;
    QFile file;
    QString dir;
    QString dateKey;
    QString currentPath;
    LogOptions options;
    bool active = false;
};

LogState &state()
{
    static LogState s;
    return s;
}

QString dateKeyNow()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd"));
}

QString fileNameForDate(const QString &dateKey)
{
    return QStringLiteral("caongan-led-%1.log").arg(dateKey);
}

void pruneOldDaysLocked(LogState &s)
{
    QDir dir(s.dir);
    const QDateTime cutoff = QDateTime::currentDateTime().addDays(-qMax(1, s.options.retentionDays));
    const auto entries = dir.entryInfoList({QStringLiteral("caongan-led-*.log*")}, QDir::Files, QDir::Name);
    for (const QFileInfo &info : entries) {
        const QString name = info.fileName();
        static const QRegularExpression re(QStringLiteral("^caongan-led-(\\d{8})\\.log(?:\\.(\\d+))?$"));
        const auto match = re.match(name);
        if (!match.hasMatch())
            continue;
        const QDate date = QDate::fromString(match.captured(1), QStringLiteral("yyyyMMdd"));
        if (date.isValid() && date < cutoff.date())
            QFile::remove(info.absoluteFilePath());
    }
}

void enforceTotalSizeLocked(LogState &s)
{
    if (s.dir.isEmpty() || s.options.maxTotalBytes <= 0)
        return;
    QDir dir(s.dir);
    auto entries = dir.entryInfoList({QStringLiteral("caongan-led-*.log*")}, QDir::Files, QDir::Time);
    qint64 total = 0;
    for (const QFileInfo &info : entries)
        total += info.size();

    // Xóa file cũ nhất (cuối danh sách sắp theo thời gian giảm dần).
    for (int i = entries.size() - 1; i >= 0 && total > s.options.maxTotalBytes; --i) {
        const QFileInfo &info = entries.at(i);
        if (info.absoluteFilePath() == QFileInfo(s.currentPath).absoluteFilePath())
            continue;
        const qint64 size = info.size();
        if (QFile::remove(info.absoluteFilePath()))
            total -= size;
    }
}

void rotateLocked(LogState &s)
{
    if (!s.file.isOpen())
        return;
    s.file.close();
    const QString base = s.dir + QLatin1Char('/') + fileNameForDate(s.dateKey);
    QFile::remove(base + QStringLiteral(".%1").arg(kKeepRotatedPerDay));
    for (int i = kKeepRotatedPerDay - 1; i >= 1; --i) {
        QFile::rename(base + QStringLiteral(".%1").arg(i),
                      base + QStringLiteral(".%1").arg(i + 1));
    }
    QFile::rename(base, base + QStringLiteral(".1"));

    s.file.setFileName(base);
    if (!s.file.open(QIODevice::Append | QIODevice::Text)) {
        s.active = false;
        return;
    }
    s.currentPath = base;
    s.active = true;
}

void openForDateLocked(LogState &s, const QString &dateKey)
{
    if (s.file.isOpen())
        s.file.close();
    s.dateKey = dateKey;
    const QString path = s.dir + QLatin1Char('/') + fileNameForDate(dateKey);
    s.file.setFileName(path);
    s.currentPath = path;
    s.active = s.file.open(QIODevice::Append | QIODevice::Text);
    if (s.active && s.file.size() > s.options.maxFileBytes)
        rotateLocked(s);
    enforceTotalSizeLocked(s);
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    Q_UNUSED(context)

    const char *level = "DEBUG";
    switch (type) {
    case QtInfoMsg:
        level = "INFO";
        break;
    case QtWarningMsg:
        level = "WARN";
        break;
    case QtCriticalMsg:
        level = "ERROR";
        break;
    case QtFatalMsg:
        level = "FATAL";
        break;
    default:
        break;
    }

    const QString line = QStringLiteral("[%1] %2: %3\n")
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
                                  QLatin1String(level), message);

    LogState &s = state();
    QMutexLocker locker(&s.mutex);
    if (!s.dir.isEmpty()) {
        const QString today = dateKeyNow();
        if (!s.file.isOpen() || s.dateKey != today)
            openForDateLocked(s, today);
        if (s.file.isOpen()) {
            if (s.file.size() + line.toUtf8().size() > s.options.maxFileBytes)
                rotateLocked(s);
            if (s.file.isOpen()) {
                s.file.write(line.toUtf8());
                s.file.flush();
            }
        }
    }

    std::fprintf(stderr, "%s", line.toUtf8().constData());
}

} // namespace

void setupLogging(const QString &logDir, const LogOptions &options)
{
    LogState &s = state();
    QMutexLocker locker(&s.mutex);
    if (s.file.isOpen())
        s.file.close();
    s.dir = logDir;
    s.options = options;
    QDir().mkpath(logDir);

    pruneOldDaysLocked(s);
    openForDateLocked(s, dateKeyNow());
    enforceTotalSizeLocked(s);
    s.active = s.file.isOpen();

    qInstallMessageHandler(messageHandler);
}

QString currentLogFilePath()
{
    LogState &s = state();
    QMutexLocker locker(&s.mutex);
    return s.currentPath;
}

void writeApplicationLog(LogLevel level, const QString &message)
{
    QtMsgType type = QtInfoMsg;
    if (level == LogLevel::Warning)
        type = QtWarningMsg;
    else if (level == LogLevel::Error)
        type = QtCriticalMsg;
    messageHandler(type, QMessageLogContext(), message);
}

} // namespace caongan
