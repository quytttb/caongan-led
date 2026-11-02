#include "LatestReadingStore.h"
#include "DisplayState.h"
#include "Freshness.h"
#include "ParameterCatalog.h"
#include "ThresholdEvaluator.h"

#include <QDir>
#include <algorithm>

namespace caongan {

namespace {

bool sameOptionalValue(const std::optional<double> &a, const std::optional<double> &b)
{
    if (a.has_value() != b.has_value())
        return false;
    if (!a.has_value())
        return true;
    return *a == *b;
}

bool sameReadingContent(const Reading &a, const Reading &b)
{
    return sameOptionalValue(a.value, b.value)
           && normalizeUnit(a.unit) == normalizeUnit(b.unit)
           && a.deviceStatus == b.deviceStatus
           && a.validity == b.validity;
}

int futureToleranceFor(const AppConfig &config, const QString &sourceId)
{
    if (const SourceConfig *source = config.sourceById(sourceId))
        return source->futureToleranceSeconds;
    return 120;
}

} // namespace

void LatestReadingStore::setConfig(const AppConfig &config)
{
    const AppConfig previous = m_config;
    m_config = config;
    m_config.channels = orderChannels(m_config.channels);

    // Kênh bị bỏ hoặc đổi nguồn/trạm thì không được tiếp tục hiển thị số cũ.
    const auto ids = m_latest.keys();
    for (const QString &id : ids) {
        const ChannelConfig *channel = m_config.channelById(id);
        if (!channel) {
            m_latest.remove(id);
            continue;
        }
        const Reading &reading = m_latest.value(id);
        const SourceConfig *oldSource = previous.sourceById(reading.sourceId);
        const SourceConfig *newSource = m_config.sourceById(reading.sourceId);
        if (channel->sourceId != reading.sourceId
            || channel->stationCode.compare(reading.stationCode, Qt::CaseInsensitive) != 0
            || !oldSource || !newSource
            || QDir::cleanPath(oldSource->folder) != QDir::cleanPath(newSource->folder)) {
            m_latest.remove(id);
        }
    }
}

ApplyOutcome LatestReadingStore::apply(const Reading &reading, const QDateTime &nowUtc)
{
    const ChannelConfig *channel = m_config.channelById(reading.channelId);
    if (!channel) {
        return {ApplyOutcome::Code::UnknownChannel,
                QStringLiteral("Kênh %1 không có trong cấu hình").arg(reading.channelId)};
    }
    if (channel->sourceId != reading.sourceId
        || channel->stationCode.compare(reading.stationCode, Qt::CaseInsensitive) != 0) {
        return {ApplyOutcome::Code::SourceMismatch,
                QStringLiteral("Kênh %1 không thuộc nguồn/trạm %2/%3")
                    .arg(reading.channelId, reading.sourceId, reading.stationCode)};
    }

    if (!reading.measuredAtUtc.isValid()) {
        return {ApplyOutcome::Code::FutureRejected,
                QStringLiteral("Kênh %1: timestamp không hợp lệ").arg(reading.channelId)};
    }

    const int tolerance = futureToleranceFor(m_config, reading.sourceId);
    const qint64 futureSeconds = nowUtc.secsTo(reading.measuredAtUtc);
    if (futureSeconds > tolerance) {
        return {ApplyOutcome::Code::FutureRejected,
                QStringLiteral("Kênh %1: mốc đo %2 vượt tương lai %3 giây")
                    .arg(reading.channelId, reading.measuredAtText)
                    .arg(futureSeconds)};
    }

    auto it = m_latest.find(reading.channelId);
    if (it == m_latest.end()) {
        // Bản snapshot phục hồi có thể đã mang conflict; không được biến nó
        // thành số bình thường sau restart. Bản parse mới mặc định conflict=false.
        m_latest.insert(reading.channelId, reading);
        return {ApplyOutcome::Code::Accepted, {}};
    }

    const Reading &current = it.value();
    const qint64 delta = current.measuredAtUtc.secsTo(reading.measuredAtUtc);
    if (delta > 0) {
        Reading stored = reading;
        stored.conflict = false;
        it.value() = stored;
        return {ApplyOutcome::Code::Accepted, {}};
    }

    if (delta < 0) {
        return {ApplyOutcome::Code::OutOfOrderIgnored,
                QStringLiteral("Kênh %1: bỏ qua bản ghi cũ %2 (đang giữ %3)")
                    .arg(reading.channelId, reading.measuredAtText, current.measuredAtText)};
    }

    if (sameReadingContent(current, reading)) {
        return {ApplyOutcome::Code::DuplicateIgnored,
                QStringLiteral("Kênh %1: bản ghi trùng mốc %2").arg(reading.channelId, reading.measuredAtText)};
    }

    // Cùng mốc nhưng khác nội dung: giữ bản đầu, đánh dấu conflict. Không cho
    // thứ tự nhận file quyết định và không dùng số đang tranh chấp để so ngưỡng.
    it.value().conflict = true;
    it.value().issueCode = QStringLiteral("conflict");
    it.value().issueMessage =
        QStringLiteral("Cùng mốc %1 nhưng khác giá trị/trạng thái/đơn vị — cần đối chiếu nguồn")
            .arg(reading.measuredAtText);
    return {ApplyOutcome::Code::ConflictRecorded,
            QStringLiteral("Kênh %1: xung đột tại mốc %2")
                .arg(reading.channelId, reading.measuredAtText)};
}

void LatestReadingStore::loadPersisted(const QVector<Reading> &readings, const QDateTime &nowUtc)
{
    const QDateTime now = nowUtc.isValid() ? nowUtc : QDateTime::currentDateTimeUtc();
    for (const Reading &reading : readings)
        apply(reading, now);
}

QVector<ChannelState> LatestReadingStore::evaluate(const QDateTime &nowUtc) const
{
    QVector<ChannelState> states;
    states.reserve(m_config.channels.size());

    for (const ChannelConfig &channel : orderChannels(m_config.channels)) {
        ChannelState state;
        state.config = channel;
        if (const SourceConfig *source = m_config.sourceById(channel.sourceId))
            state.sourceName = source->name;

        const auto it = m_latest.constFind(channel.id);
        if (it == m_latest.constEnd()) {
            state.hasReading = false;
            state.validity = Validity::Missing;
            state.freshness = Freshness::NoReading;
            state.threshold = channel.threshold.configured()
                ? ThresholdResult::NotEvaluated
                : ThresholdResult::NotConfigured;
            const ChannelEvaluation display = evaluateDisplay(
                channel, nullptr, Validity::Missing, Freshness::NoReading,
                state.threshold, false, nowUtc);
            state.display = display.display;
            state.valueText = display.valueText;
            state.statusText = display.statusText;
            state.stateKey = display.stateKey;
            state.severity = display.severity;
            state.detailText = display.detailText;
            states.append(state);
            continue;
        }

        const Reading &reading = it.value();
        state.hasReading = true;
        state.reading = reading;

        Validity validity = reading.validity;
        if (validity == Validity::Valid && !unitAccepted(channel, reading.unit))
            validity = Validity::UnitMismatch;
        if (reading.conflict)
            validity = Validity::Conflict;
        state.validity = validity;

        const int futureTolerance = futureToleranceFor(m_config, reading.sourceId);
        const FreshnessResult fresh = evaluateFreshness(
            reading, nowUtc, channel.staleAfterSeconds, futureTolerance);
        state.freshness = fresh.freshness;
        state.ageSeconds = fresh.ageSeconds;

        state.threshold = evaluateThreshold(
            channel, reading, validity, state.freshness, reading.conflict);

        const ChannelEvaluation display = evaluateDisplay(
            channel, &reading, validity, state.freshness, state.threshold,
            reading.conflict, nowUtc);
        state.display = display.display;
        state.valueText = display.valueText;
        state.statusText = display.statusText;
        state.stateKey = display.stateKey;
        state.severity = display.severity;
        state.detailText = display.detailText;

        states.append(state);
    }

    return states;
}

QVector<Reading> LatestReadingStore::readings() const
{
    QVector<Reading> result;
    for (const ChannelConfig &channel : orderChannels(m_config.channels)) {
        const auto it = m_latest.constFind(channel.id);
        if (it != m_latest.constEnd())
            result.append(it.value());
    }
    return result;
}

bool LatestReadingStore::hasReading(const QString &channelId) const
{
    return m_latest.contains(channelId);
}

bool LatestReadingStore::latestReading(const QString &channelId, Reading *reading) const
{
    if (!reading)
        return false;
    const auto it = m_latest.constFind(channelId);
    if (it == m_latest.constEnd())
        return false;
    *reading = it.value();
    return true;
}

void LatestReadingStore::clear()
{
    m_latest.clear();
}

} // namespace caongan
