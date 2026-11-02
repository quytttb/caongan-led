#pragma once

#include "Types.h"

namespace caongan {

// Thứ tự 12 vị trí trên màn LED là hợp đồng hiển thị: 8 khí thải rồi 4 nước thải.
// Cấu hình có thể đổi đơn vị/ngưỡng/precision nhưng không đổi thứ tự này.
QStringList canonicalChannelIds();
QList<ChannelConfig> defaultChannels();
QList<SourceConfig> defaultSources();
AppConfig defaultAppConfig();

// Sắp xếp theo thứ tự chuẩn; kênh lạ (nếu có) giữ sau cùng để không mất dữ liệu
// cấu hình nhưng không chen vào 12 ô đã chốt.
QList<ChannelConfig> orderChannels(const QList<ChannelConfig> &channels);

QString normalizeUnit(const QString &unit);
QString normalizeSymbol(const QString &symbol);
bool unitAccepted(const ChannelConfig &channel, const QString &unit);

} // namespace caongan
