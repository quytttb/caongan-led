#pragma once

#include "Types.h"

namespace caongan {

struct ChannelEvaluation {
    DisplayState display = DisplayState::NoData;
    QString valueText = QStringLiteral("—");
    QString statusText;
    QString stateKey;
    QString severity;
    QString detailText;
};

// Đánh giá một kênh từ dữ liệu đã lưu. Hàm thuần, không phụ thuộc QML.
ChannelEvaluation evaluateDisplay(const ChannelConfig &config,
                                  const Reading *reading,
                                  Validity validity,
                                  Freshness freshness,
                                  ThresholdResult threshold,
                                  bool conflict,
                                  const QDateTime &nowUtc);

QString displayStateKey(DisplayState state);
QString displayStateText(DisplayState state);
QString displayStateSeverity(DisplayState state);
QString formatReadingValue(double value, int decimals);
QString formatAge(int ageSeconds);
QString formatThreshold(const Threshold &threshold, int decimals);

} // namespace caongan
