#include "ThresholdEvaluator.h"

#include <cmath>

namespace caongan {

ThresholdResult evaluateThreshold(const ChannelConfig &channel,
                                  const Reading &reading,
                                  Validity validity,
                                  Freshness freshness,
                                  bool conflict)
{
    if (!channel.threshold.configured())
        return ThresholdResult::NotConfigured;

    if (conflict
        || validity != Validity::Valid
        || freshness != Freshness::Fresh
        || reading.deviceStatus != DeviceStatus::Measuring
        || !reading.value.has_value()) {
        return ThresholdResult::NotEvaluated;
    }

    const double value = *reading.value;
    if (!std::isfinite(value))
        return ThresholdResult::NotApplicable;

    if (channel.threshold.hasLow && value < channel.threshold.low)
        return ThresholdResult::ExceedsConfiguredLimit;
    if (channel.threshold.hasHigh && value > channel.threshold.high)
        return ThresholdResult::ExceedsConfiguredLimit;

    return ThresholdResult::WithinConfiguredLimit;
}

} // namespace caongan
