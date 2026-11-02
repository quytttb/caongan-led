#include "DisplayState.h"
#include "Tt10Time.h"

#include <QLocale>
#include <cmath>

namespace caongan {

QString displayStateKey(DisplayState state)
{
    switch (state) {
    case DisplayState::NoData:          return QStringLiteral("noData");
    case DisplayState::Measuring:       return QStringLiteral("measuring");
    case DisplayState::WithinLimit:     return QStringLiteral("withinLimit");
    case DisplayState::ExceedsLimit:    return QStringLiteral("exceedsLimit");
    case DisplayState::Calibrating:     return QStringLiteral("calibrating");
    case DisplayState::DeviceFault:     return QStringLiteral("deviceFault");
    case DisplayState::Stale:           return QStringLiteral("stale");
    case DisplayState::FutureTimestamp: return QStringLiteral("futureTimestamp");
    case DisplayState::Invalid:         return QStringLiteral("invalid");
    case DisplayState::UnitMismatch:    return QStringLiteral("unitMismatch");
    case DisplayState::Conflict:        return QStringLiteral("conflict");
    case DisplayState::UnknownStatus:   return QStringLiteral("unknownStatus");
    }
    return QStringLiteral("noData");
}

QString displayStateText(DisplayState state)
{
    switch (state) {
    case DisplayState::NoData:          return QStringLiteral("CHƯA CÓ DỮ LIỆU");
    case DisplayState::Measuring:       return QStringLiteral("ĐANG ĐO");
    case DisplayState::WithinLimit:     return QStringLiteral("TRONG GIỚI HẠN");
    case DisplayState::ExceedsLimit:    return QStringLiteral("VƯỢT NGƯỠNG");
    case DisplayState::Calibrating:     return QStringLiteral("HIỆU CHUẨN");
    case DisplayState::DeviceFault:     return QStringLiteral("BÁO LỖI THIẾT BỊ");
    case DisplayState::Stale:           return QStringLiteral("DỮ LIỆU CŨ");
    case DisplayState::FutureTimestamp: return QStringLiteral("THỜI GIAN KHÔNG HỢP LỆ");
    case DisplayState::Invalid:         return QStringLiteral("DỮ LIỆU KHÔNG HỢP LỆ");
    case DisplayState::UnitMismatch:    return QStringLiteral("SAI ĐƠN VỊ");
    case DisplayState::Conflict:        return QStringLiteral("XUNG ĐỘT DỮ LIỆU");
    case DisplayState::UnknownStatus:   return QStringLiteral("TRẠNG THÁI KHÔNG XÁC ĐỊNH");
    }
    return QStringLiteral("CHƯA CÓ DỮ LIỆU");
}

QString displayStateSeverity(DisplayState state)
{
    switch (state) {
    case DisplayState::Measuring:
    case DisplayState::WithinLimit:
        return QStringLiteral("normal");
    case DisplayState::ExceedsLimit:
    case DisplayState::DeviceFault:
    case DisplayState::Conflict:
        return QStringLiteral("alarm");
    case DisplayState::Calibrating:
    case DisplayState::UnitMismatch:
        return QStringLiteral("warning");
    case DisplayState::Stale:
    case DisplayState::FutureTimestamp:
    case DisplayState::Invalid:
    case DisplayState::UnknownStatus:
        return QStringLiteral("muted");
    case DisplayState::NoData:
        return QStringLiteral("missing");
    }
    return QStringLiteral("missing");
}

QString formatReadingValue(double value, int decimals)
{
    if (!std::isfinite(value))
        return QStringLiteral("—");
    const int digits = qBound(0, decimals, 6);
    return QString::number(value, 'f', digits);
}

QString formatAge(int ageSeconds)
{
    if (ageSeconds < 0)
        return QStringLiteral("tương lai");
    if (ageSeconds < 60)
        return QStringLiteral("%1 giây").arg(ageSeconds);
    if (ageSeconds < 3600)
        return QStringLiteral("%1 phút").arg(ageSeconds / 60);
    if (ageSeconds < 86400)
        return QStringLiteral("%1 giờ %2 phút").arg(ageSeconds / 3600).arg((ageSeconds % 3600) / 60);
    return QStringLiteral("%1 ngày").arg(ageSeconds / 86400);
}

QString formatThreshold(const Threshold &threshold, int decimals)
{
    if (!threshold.configured())
        return {};
    const QString low = threshold.hasLow ? formatReadingValue(threshold.low, decimals) : QStringLiteral("…");
    const QString high = threshold.hasHigh ? formatReadingValue(threshold.high, decimals) : QStringLiteral("…");
    if (threshold.hasLow && threshold.hasHigh)
        return QStringLiteral("%1 – %2").arg(low, high);
    if (threshold.hasHigh)
        return QStringLiteral("≤ %1").arg(high);
    return QStringLiteral("≥ %1").arg(low);
}

ChannelEvaluation evaluateDisplay(const ChannelConfig &config,
                                  const Reading *reading,
                                  Validity validity,
                                  Freshness freshness,
                                  ThresholdResult threshold,
                                  bool conflict,
                                  const QDateTime &nowUtc)
{
    Q_UNUSED(nowUtc)

    ChannelEvaluation result;
    result.display = DisplayState::NoData;
    result.valueText = QStringLiteral("—");
    result.statusText = displayStateText(result.display);
    result.stateKey = displayStateKey(result.display);
    result.severity = displayStateSeverity(result.display);
    result.detailText = QStringLiteral("Chưa nhận được bản ghi cho kênh này");

    if (!reading)
        return result;

    const QString measured = tt10::formatLocalDisplay(reading->measuredAtUtc);
    const QString age = formatAge(reading->measuredAtUtc.secsTo(nowUtc));

    auto apply = [&result](DisplayState state) {
        result.display = state;
        result.statusText = displayStateText(state);
        result.stateKey = displayStateKey(state);
        result.severity = displayStateSeverity(state);
    };

    if (conflict) {
        apply(DisplayState::Conflict);
        result.detailText = QStringLiteral("Cùng mốc %1 nhưng nội dung khác — không dùng để so ngưỡng").arg(measured);
        return result;
    }

    if (validity == Validity::Missing) {
        apply(DisplayState::NoData);
        result.detailText = QStringLiteral("Bản ghi %1 không có giá trị").arg(measured);
        return result;
    }
    if (validity == Validity::Invalid) {
        apply(DisplayState::Invalid);
        result.detailText = reading->issueMessage.isEmpty()
            ? QStringLiteral("Bản ghi %1 không hợp lệ").arg(measured)
            : reading->issueMessage;
        return result;
    }
    if (validity == Validity::UnitMismatch) {
        apply(DisplayState::UnitMismatch);
        result.detailText = QStringLiteral("Đơn vị nhận được “%1” không khớp cấu hình “%2”")
                                .arg(reading->unit, config.unit);
        return result;
    }

    if (freshness == Freshness::FutureTimestamp) {
        apply(DisplayState::FutureTimestamp);
        result.detailText = QStringLiteral("Mốc đo %1 vượt quá dung sai giờ tương lai").arg(measured);
        return result;
    }
    if (freshness == Freshness::Stale) {
        apply(DisplayState::Stale);
        result.detailText = QStringLiteral("Mốc đo %1, đã cũ %2").arg(measured, age);
        return result;
    }

    if (reading->deviceStatus == DeviceStatus::DeviceFault) {
        apply(DisplayState::DeviceFault);
        result.detailText = QStringLiteral("Thiết bị báo lỗi tại mốc %1").arg(measured);
        return result;
    }
    if (reading->deviceStatus == DeviceStatus::Calibrating) {
        apply(DisplayState::Calibrating);
        result.detailText = QStringLiteral("Thiết bị đang hiệu chuẩn tại mốc %1").arg(measured);
        return result;
    }
    if (reading->deviceStatus == DeviceStatus::Unknown) {
        apply(DisplayState::UnknownStatus);
        result.detailText = QStringLiteral("Mã trạng thái thiết bị không hỗ trợ tại mốc %1").arg(measured);
        return result;
    }

    if (!reading->value.has_value()) {
        apply(DisplayState::NoData);
        result.detailText = QStringLiteral("Bản ghi %1 không có giá trị số").arg(measured);
        return result;
    }

    if (threshold == ThresholdResult::ExceedsConfiguredLimit) {
        apply(DisplayState::ExceedsLimit);
        result.detailText = QStringLiteral("Mốc đo %1 vượt ngưỡng đã cấu hình").arg(measured);
    } else if (threshold == ThresholdResult::WithinConfiguredLimit) {
        apply(DisplayState::WithinLimit);
        result.detailText = QStringLiteral("Mốc đo %1 trong ngưỡng đã cấu hình").arg(measured);
    } else {
        apply(DisplayState::Measuring);
        result.detailText = QStringLiteral("Mốc đo %1, chưa cấu hình ngưỡng").arg(measured);
    }

    result.valueText = formatReadingValue(*reading->value, config.decimals);
    return result;
}

} // namespace caongan
