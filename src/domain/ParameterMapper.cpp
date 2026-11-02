#include "ParameterMapper.h"
#include "ParameterCatalog.h"

#include <QHash>
#include <QSet>

namespace caongan {

namespace {

DiagnosticEntry makeDiagnostic(const QString &severity,
                               const QString &sourceId,
                               const QString &channelId,
                               const QString &code,
                               const QString &message,
                               const QDateTime &nowUtc)
{
    DiagnosticEntry entry;
    entry.atUtc = nowUtc;
    entry.severity = severity;
    entry.sourceId = sourceId;
    entry.channelId = channelId;
    entry.code = code;
    entry.message = message;
    return entry;
}

} // namespace

MapResult ParameterMapper::map(const ParseResult &parsed,
                               const SourceConfig &source,
                               const AppConfig &config,
                               const QString &filePath,
                               const QString &contentHash,
                               const QDateTime &receivedAtUtc)
{
    MapResult result;

    QHash<QString, const ChannelConfig *> aliasToChannel;
    for (const auto &channel : config.channels) {
        if (channel.sourceId != source.id)
            continue;
        for (const QString &alias : channel.aliases)
            aliasToChannel.insert(normalizeSymbol(alias), &channel);
    }

    QSet<QString> unknownSymbols;
    QSet<QString> emittedMissing;

    for (const Tt10Record &record : parsed.records) {
        const QString key = normalizeSymbol(record.symbol);
        const ChannelConfig *channel = aliasToChannel.value(key, nullptr);
        if (!channel) {
            if (!unknownSymbols.contains(record.symbol)) {
                unknownSymbols.insert(record.symbol);
                result.diagnostics.append(
                    makeDiagnostic(QStringLiteral("warning"), source.id, QString(),
                                   QStringLiteral("unmapped_symbol"),
                                   QStringLiteral("Bỏ qua ký hiệu không có trong cấu hình nguồn %1: %2")
                                       .arg(source.name, record.symbol),
                                   receivedAtUtc));
            }
            continue;
        }

        Reading reading;
        reading.channelId = channel->id;
        reading.sourceId = source.id;
        reading.stationCode = parsed.stationCode;
        reading.rawSymbol = record.symbol;
        reading.rawValue = record.rawValue;
        reading.value = record.value;
        reading.unit = record.unit;
        reading.measuredAtUtc = record.measuredAtUtc;
        reading.measuredAtText = record.timestampText;
        reading.deviceStatus = record.deviceStatus;
        reading.validity = record.validity;
        reading.issueCode = record.issueCode;
        reading.issueMessage = record.issueMessage;
        reading.filePath = filePath;
        reading.contentHash = contentHash;
        reading.receivedAtUtc = receivedAtUtc;

        if (reading.validity == Validity::Valid && !unitAccepted(*channel, record.unit)) {
            reading.validity = Validity::UnitMismatch;
            reading.issueCode = QStringLiteral("unit_mismatch");
            reading.issueMessage = QStringLiteral("Đơn vị “%1” không khớp cấu hình “%2”")
                                       .arg(record.unit, channel->unit);
            result.diagnostics.append(
                makeDiagnostic(QStringLiteral("warning"), source.id, channel->id,
                               reading.issueCode, reading.issueMessage, receivedAtUtc));
        }

        if (reading.validity == Validity::Missing
            && !emittedMissing.contains(channel->id)) {
            emittedMissing.insert(channel->id);
            result.diagnostics.append(
                makeDiagnostic(QStringLiteral("warning"), source.id, channel->id,
                               QStringLiteral("missing_value"),
                               QStringLiteral("Kênh %1 không có giá trị tại mốc %2")
                                   .arg(channel->symbol, record.timestampText),
                               receivedAtUtc));
        }

        result.readings.append(reading);
    }

    return result;
}

} // namespace caongan
