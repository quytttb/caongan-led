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
    case ChannelIdRole:     return state.config.id;
    case SymbolRole:        return state.config.symbol;
    case DisplayNameRole:   return state.config.displayName;
    case UnitRole:          return state.config.unit;
    case RangeTextRole:     return state.config.rangeText;
    case GroupRole:         return channelGroupId(state.config.group);
    case GroupTitleRole:    return channelGroupTitle(state.config.group);
    case ValueTextRole:     return state.valueText;
    case StatusTextRole:    return state.statusText;
    case StateKeyRole:      return state.stateKey;
    case SeverityRole:      return state.severity;
    case DetailTextRole:    return state.detailText;
    case TimeTextRole:
        return state.hasReading && state.reading.measuredAtUtc.isValid()
            ? tt10::formatLocalDisplay(state.reading.measuredAtUtc, QStringLiteral("HH:mm:ss dd/MM"))
            : QStringLiteral("—");
    case ThresholdTextRole: {
        if (!state.config.threshold.configured())
            return QStringLiteral("Chưa đặt ngưỡng");
        return formatThreshold(state.config.threshold, state.config.id == QStringLiteral("ph") ? 0 : state.config.decimals);
    }
    case SourceNameRole:    return state.sourceName;
    case HasReadingRole:    return state.hasReading;
    case AgeTextRole:       return state.hasReading ? formatAge(state.ageSeconds) : QStringLiteral("—");
    default:                return {};
    }
}

QHash<int, QByteArray> LedParameterModel::roleNames() const
{
    return {
        {ChannelIdRole, "channelId"},
        {SymbolRole, "symbol"},
        {DisplayNameRole, "displayName"},
        {UnitRole, "unit"},
        {RangeTextRole, "rangeText"},
        {GroupRole, "group"},
        {GroupTitleRole, "groupTitle"},
        {ValueTextRole, "valueText"},
        {StatusTextRole, "statusText"},
        {StateKeyRole, "stateKey"},
        {SeverityRole, "severity"},
        {DetailTextRole, "detailText"},
        {TimeTextRole, "timeText"},
        {ThresholdTextRole, "thresholdText"},
        {SourceNameRole, "sourceName"},
        {HasReadingRole, "hasReading"},
        {AgeTextRole, "ageText"},
    };
}

void LedParameterModel::setConfig(const AppConfig &config)
{
    m_config = config;
    m_config.channels = orderChannels(m_config.channels);
    rebuildPlaceholders();
    emit statesChanged();
}

void LedParameterModel::rebuildPlaceholders()
{
    beginResetModel();
    m_states.clear();
    for (const ChannelConfig &channel : m_config.channels) {
        ChannelState state;
        state.config = channel;
        if (const SourceConfig *source = m_config.sourceById(channel.sourceId))
            state.sourceName = source->name;
        state.hasReading = false;
        state.validity = Validity::Missing;
        state.freshness = Freshness::NoReading;
        state.threshold = channel.threshold.configured()
            ? ThresholdResult::NotEvaluated
            : ThresholdResult::NotConfigured;
        const ChannelEvaluation display = evaluateDisplay(
            channel, nullptr, Validity::Missing, Freshness::NoReading,
            state.threshold, false, QDateTime::currentDateTimeUtc());
        state.display = display.display;
        state.valueText = display.valueText;
        state.statusText = display.statusText;
        state.stateKey = display.stateKey;
        state.severity = display.severity;
        state.detailText = display.detailText;
        m_states.append(state);
    }
    endResetModel();
}

void LedParameterModel::setStates(const QVector<ChannelState> &states)
{
    if (states.isEmpty()) {
        rebuildPlaceholders();
        emit statesChanged();
        return;
    }
    beginResetModel();
    m_states = states;
    endResetModel();
    emit statesChanged();
}

} // namespace caongan
