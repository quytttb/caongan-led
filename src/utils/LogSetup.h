#pragma once

#include <QString>

namespace caongan {

struct LogOptions {
    qint64 maxFileBytes = 4 * 1024 * 1024;
    qint64 maxTotalBytes = 64 * 1024 * 1024;
    int retentionDays = 14;
};

void setupLogging(const QString &logDir, const LogOptions &options = {});
QString currentLogFilePath();

} // namespace caongan
