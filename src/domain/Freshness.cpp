#include "Freshness.h"

namespace caongan {

FreshnessResult evaluateFreshness(const Reading &reading,
                                  const QDateTime &nowUtc,
                                  int staleAfterSeconds,
                                  int futureToleranceSeconds)
{
    FreshnessResult result;
    if (!reading.measuredAtUtc.isValid()) {
        result.freshness = Freshness::FutureTimestamp;
        return result;
    }

    const int age = static_cast<int>(reading.measuredAtUtc.secsTo(nowUtc));
    result.ageSeconds = age;

    const int tolerance = qMax(0, futureToleranceSeconds);
    if (age < -tolerance) {
        result.freshness = Freshness::FutureTimestamp;
        return result;
    }

    if (age >= qMax(10, staleAfterSeconds)) {
        result.freshness = Freshness::Stale;
        return result;
    }

    result.freshness = Freshness::Fresh;
    return result;
}

} // namespace caongan
