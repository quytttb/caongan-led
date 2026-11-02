#pragma once

#include <QDateTime>
#include <QString>

namespace caongan::tt10 {

// TT10 luôn diễn giải YYYYMMDDHHMMSS theo giờ Việt Nam UTC+7, không phụ thuộc
// timezone Windows đang cấu hình. Nội bộ lưu/so sánh bằng UTC.
inline constexpr int kVietnamOffsetSeconds = 7 * 60 * 60;
inline constexpr int kTimestampDigits = 14;

QDateTime parseTimestamp(const QString &text, QString *error = nullptr);
QString formatTimestamp(const QDateTime &utc);
QString formatLocalDisplay(const QDateTime &utc, const QString &format = QStringLiteral("HH:mm:ss dd/MM/yyyy"));
QDateTime nowUtc();

} // namespace caongan::tt10
