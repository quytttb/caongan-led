#pragma once

#include "Types.h"

namespace caongan {

struct FreshnessResult {
    Freshness freshness = Freshness::NoReading;
    int ageSeconds = 0;
};

FreshnessResult evaluateFreshness(const Reading &reading,
                                  const QDateTime &nowUtc,
                                  int staleAfterSeconds,
                                  int futureToleranceSeconds);

} // namespace caongan
