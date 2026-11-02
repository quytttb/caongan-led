#pragma once

#include "Types.h"

namespace caongan {

// So sánh bằng giá trị trước khi làm tròn. Biên được coi là trong giới hạn.
ThresholdResult evaluateThreshold(const ChannelConfig &channel,
                                  const Reading &reading,
                                  Validity validity,
                                  Freshness freshness,
                                  bool conflict);

} // namespace caongan
