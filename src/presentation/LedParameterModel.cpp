#include "LedParameterModel.h"

#include "domain/DisplayState.h"
#include "domain/ParameterCatalog.h"
#include "domain/Tt10Time.h"

namespace caongan {

LedParameterModel::LedParameterModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int LedParameterModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_states.size();
}

QVariant LedParameterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_states.size())
        return {};

    const ChannelState &state = m_states.at(index.row());
    switch (role) {
    case ChannelIdRole:  return state.config.id;
    case SymbolRole:     return state.config.symbol;
    case UnitRole:       return state.config.unit;
    case RangeTextRole:  return state.config.rangeText;
    case ValueTextRole:  return state.valueText;
    case StatusTextRole: return state.statusText;
    case StateKeyRole:   return state.stateKey;
    case DetailTextRole: return state.detailText;
    case TimeTextRole:
        return state.hasReading && state.reading.measuredAtUtc.isValid()
            ? tt10::formatLocalDisplay(state.reading.measuredAtUtc, QStringLiteral("HH:mm:ss dd/MM"))
            : QStringLiteral("—");
    case ThresholdTextRole:
        if (!state.config.threshold.configured())
            return QStringLiteral("Chưa đặt ngưỡng");
        // pH hiển thị ngưỡng ngay trên LED: không làm tròn mất phần thập phân
        // của ngưỡng người vận hành vừa lưu.
        if (state.config.id == QStringLiteral("ph"))
            return formatShortestThreshold(state.config.threshold);
        return formatThreshold(state.config.threshold, state.config.decimals);
    default:
        return {};
    }
}

QHash<int, QByteArray> LedParameterModel::roleNames() const
{
    return {
        {ChannelIdRole, "channelId"},
        {SymbolRole, "symbol"},
        {UnitRole, "unit"},
        {RangeTextRole, "rangeText"},
        {ValueTextRole, "valueText"},
        {StatusTextRole, "statusText"},
        {StateKeyRole, "stateKey"},
        {DetailTextRole, "detailText"},
        {TimeTextRole, "timeText"},
        {ThresholdTextRole, "thresholdText"},
    };
}

void LedParameterModel::setConfig(const AppConfig &config)
{
    m_config = config;
    m_config.channels = orderChannels(m_config.channels);
    rebuildPlaceholders();
}

void LedParameterModel::rebuildPlaceholders()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    beginResetModel();
    m_states.clear();
    m_states.reserve(m_config.channels.size());
    for (const ChannelConfig &channel : m_config.channels)
        m_states.append(makePlaceholderChannelState(channel, now));
    endResetModel();
}

void LedParameterModel::setStates(const QVector<ChannelState> &states)
{
    if (states.isEmpty()) {
        rebuildPlaceholders();
        return;
    }
    beginResetModel();
    m_states = states;
    endResetModel();
}

} // namespace caongan
