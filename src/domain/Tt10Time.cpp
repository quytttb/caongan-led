#include "Tt10Time.h"

#include <QDate>
#include <QTime>
#include <QTimeZone>

namespace caongan::tt10 {

namespace {

QTimeZone vietnamZone()
{
    static const QTimeZone zone = QTimeZone::fromSecondsAheadOfUtc(kVietnamOffsetSeconds);
    return zone;
}

} // namespace

bool isTimestampDigits(const QString &text)
{
    const QString value = text.trimmed();
    if (value.size() != kTimestampDigits)
        return false;
    for (const QChar ch : value) {
        if (ch < QLatin1Char('0') || ch > QLatin1Char('9'))
            return false;
    }
    return true;
}

QDateTime parseTimestamp(const QString &text, QString *error)
{
    const QString value = text.trimmed();
    if (!isTimestampDigits(value)) {
        if (error)
            *error = QStringLiteral("Timestamp phải có đúng 14 chữ số YYYYMMDDHHMMSS");
        return {};
    }

    const int year = value.mid(0, 4).toInt();
    const int month = value.mid(4, 2).toInt();
    const int day = value.mid(6, 2).toInt();
    const int hour = value.mid(8, 2).toInt();
    const int minute = value.mid(10, 2).toInt();
    const int second = value.mid(12, 2).toInt();

    const QDate date(year, month, day);
    const QTime time(hour, minute, second);
    if (!date.isValid() || !time.isValid()) {
        if (error)
            *error = QStringLiteral("Ngày/giờ không tồn tại: %1").arg(value);
        return {};
    }

    QDateTime local(date, time, vietnamZone());
    if (!local.isValid()) {
        if (error)
            *error = QStringLiteral("Không thể diễn giải timestamp UTC+7: %1").arg(value);
        return {};
    }

    const QDateTime utc = local.toUTC();
    // QDateTime chuẩn hóa giờ không hợp lệ (ví dụ 24:00) — kiểm tra round-trip
    // để không chấp nhận giá trị bị Qt tự sửa.
    if (formatTimestamp(utc) != value) {
        if (error)
            *error = QStringLiteral("Timestamp không hợp lệ sau khi chuẩn hóa: %1").arg(value);
        return {};
    }
    return utc;
}

QString formatTimestamp(const QDateTime &utc)
{
    if (!utc.isValid())
        return QStringLiteral("----------------");
    return utc.toTimeZone(vietnamZone()).toString(QStringLiteral("yyyyMMddHHmmss"));
}

QString formatLocalDisplay(const QDateTime &utc, const QString &format)
{
    if (!utc.isValid())
        return QStringLiteral("—");
    return utc.toTimeZone(vietnamZone()).toString(format);
}

QDateTime nowUtc()
{
    return QDateTime::currentDateTimeUtc();
}

} // namespace caongan::tt10
