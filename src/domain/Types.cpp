#include "Types.h"

namespace caongan {

const SourceConfig *AppConfig::sourceById(const QString &id) const
{
    for (const auto &source : sources) {
        if (source.id == id)
            return &source;
    }
    return nullptr;
}

const ChannelConfig *AppConfig::channelById(const QString &id) const
{
    for (const auto &channel : channels) {
        if (channel.id == id)
            return &channel;
    }
    return nullptr;
}

ChannelConfig *AppConfig::channelById(const QString &id)
{
    for (auto &channel : channels) {
        if (channel.id == id)
            return &channel;
    }
    return nullptr;
}

QString channelGroupId(ChannelGroup group)
{
    return group == ChannelGroup::Exhaust ? QStringLiteral("exhaust")
                                          : QStringLiteral("wastewater");
}

QString channelGroupTitle(ChannelGroup group)
{
    return group == ChannelGroup::Exhaust ? QStringLiteral("KHÍ THẢI")
                                          : QStringLiteral("NƯỚC THẢI");
}

QString deviceStatusToCode(DeviceStatus status)
{
    switch (status) {
    case DeviceStatus::Measuring:
        return QStringLiteral("00");
    case DeviceStatus::Calibrating:
        return QStringLiteral("01");
    case DeviceStatus::DeviceFault:
        return QStringLiteral("02");
    case DeviceStatus::Unknown:
        break;
    }
    return QStringLiteral("--");
}

DeviceStatus deviceStatusFromCode(const QString &code)
{
    if (code == QLatin1String("00"))
        return DeviceStatus::Measuring;
    if (code == QLatin1String("01"))
        return DeviceStatus::Calibrating;
    if (code == QLatin1String("02"))
        return DeviceStatus::DeviceFault;
    return DeviceStatus::Unknown;
}

} // namespace caongan
