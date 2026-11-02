#pragma once

#include <QString>

namespace caongan {

struct LogOptions {
    qint64 maxFileBytes = 4 * 1024 * 1024;
    qint64 maxTotalBytes = 64 * 1024 * 1024;
    int retentionDays = 14;
};

// Cảnh báo vận hành là dữ liệu chẩn đoán, không phải Qt warning có thể
// trở thành fatal khi người kiểm thử bật QT_FATAL_WARNINGS.
enum class LogLevel { Info, Warning, Error };

void setupLogging(const QString &logDir, const LogOptions &options = {});
QString currentLogFilePath();
void writeApplicationLog(LogLevel level, const QString &message);

} // namespace caongan
