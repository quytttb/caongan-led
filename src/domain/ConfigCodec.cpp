#include "ConfigCodec.h"
#include "ParameterCatalog.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QDir>
#include <QSet>
#include <cmath>

namespace caongan {

namespace {

QJsonValue optionalNumber(bool present, double value)
{
    return present ? QJsonValue(value) : QJsonValue(QJsonValue::Null);
}

bool readBool(const QJsonObject &obj, const char *key, bool fallback)
{
    const QJsonValue value = obj.value(QLatin1String(key));
    return value.isBool() ? value.toBool() : fallback;
}

int readInt(const QJsonObject &obj, const char *key, int fallback)
{
    const QJsonValue value = obj.value(QLatin1String(key));
    return value.isDouble() ? value.toInt(fallback) : fallback;
}

qint64 readInt64(const QJsonObject &obj, const char *key, qint64 fallback)
{
    const QJsonValue value = obj.value(QLatin1String(key));
    return value.isDouble() ? static_cast<qint64>(value.toDouble()) : fallback;
}

QString readString(const QJsonObject &obj, const char *key, const QString &fallback = {})
{
    const QJsonValue value = obj.value(QLatin1String(key));
    return value.isString() ? value.toString() : fallback;
}

QStringList readStringList(const QJsonObject &obj, const char *key)
{
    QStringList result;
    for (const QJsonValue &value : obj.value(QLatin1String(key)).toArray()) {
        if (value.isString())
            result.append(value.toString());
    }
    return result;
}

QJsonObject thresholdToJson(const Threshold &threshold)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("enabled"), threshold.enabled);
    obj.insert(QStringLiteral("low"), optionalNumber(threshold.hasLow, threshold.low));
    obj.insert(QStringLiteral("high"), optionalNumber(threshold.hasHigh, threshold.high));
    return obj;
}

Threshold thresholdFromJson(const QJsonObject &obj)
{
    Threshold threshold;
    threshold.enabled = readBool(obj, "enabled", false);
    const QJsonValue low = obj.value(QStringLiteral("low"));
    const QJsonValue high = obj.value(QStringLiteral("high"));
    if (low.isDouble()) {
        threshold.hasLow = true;
        threshold.low = low.toDouble();
    }
    if (high.isDouble()) {
        threshold.hasHigh = true;
        threshold.high = high.toDouble();
    }
    return threshold;
}

} // namespace

QJsonObject configToJson(const AppConfig &config)
{
    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), config.schemaVersion);

    QJsonObject branding;
    branding.insert(QStringLiteral("companyName"), config.branding.companyName);
    branding.insert(QStringLiteral("branchName"), config.branding.branchName);
    branding.insert(QStringLiteral("address"), config.branding.address);
    branding.insert(QStringLiteral("regulationText"), config.branding.regulationText);
    branding.insert(QStringLiteral("footerNote"), config.branding.footerNote);
    root.insert(QStringLiteral("branding"), branding);

    QJsonObject display;
    display.insert(QStringLiteral("targetScreenName"), config.display.targetScreenName);
    display.insert(QStringLiteral("fullScreen"), config.display.fullScreen);
    display.insert(QStringLiteral("designWidth"), config.display.designWidth);
    display.insert(QStringLiteral("designHeight"), config.display.designHeight);
    root.insert(QStringLiteral("display"), display);

    QJsonObject storage;
    storage.insert(QStringLiteral("checkpointRetentionDays"), config.storage.checkpointRetentionDays);
    storage.insert(QStringLiteral("checkpointMaxRows"), config.storage.checkpointMaxRows);
    storage.insert(QStringLiteral("logRetentionDays"), config.storage.logRetentionDays);
    storage.insert(QStringLiteral("logMaxFileBytes"), config.storage.logMaxFileBytes);
    storage.insert(QStringLiteral("logMaxTotalBytes"), config.storage.logMaxTotalBytes);
    root.insert(QStringLiteral("storage"), storage);

    QJsonArray sources;
    for (const auto &source : config.sources) {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), source.id);
        obj.insert(QStringLiteral("name"), source.name);
        obj.insert(QStringLiteral("folder"), source.folder);
        obj.insert(QStringLiteral("stationCode"), source.stationCode);
        obj.insert(QStringLiteral("allowLegacyPrefix"), source.allowLegacyPrefix);
        obj.insert(QStringLiteral("groupedFiveColumn"), source.groupedFiveColumn);
        obj.insert(QStringLiteral("recursive"), source.recursive);
        obj.insert(QStringLiteral("maxDepth"), source.maxDepth);
        obj.insert(QStringLiteral("staleAfterSeconds"), source.staleAfterSeconds);
        obj.insert(QStringLiteral("futureToleranceSeconds"), source.futureToleranceSeconds);
        sources.append(obj);
    }
    root.insert(QStringLiteral("sources"), sources);

    QJsonArray channels;
    for (const auto &channel : config.channels) {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), channel.id);
        obj.insert(QStringLiteral("group"), channelGroupId(channel.group));
        obj.insert(QStringLiteral("symbol"), channel.symbol);
        obj.insert(QStringLiteral("displayName"), channel.displayName);
        obj.insert(QStringLiteral("unit"), channel.unit);
        obj.insert(QStringLiteral("acceptedUnits"), QJsonArray::fromStringList(channel.acceptedUnits));
        obj.insert(QStringLiteral("aliases"), QJsonArray::fromStringList(channel.aliases));
        obj.insert(QStringLiteral("sourceId"), channel.sourceId);
        obj.insert(QStringLiteral("stationCode"), channel.stationCode);
        obj.insert(QStringLiteral("decimals"), channel.decimals);
        obj.insert(QStringLiteral("staleAfterSeconds"), channel.staleAfterSeconds);
        obj.insert(QStringLiteral("threshold"), thresholdToJson(channel.threshold));
        channels.append(obj);
    }
    root.insert(QStringLiteral("channels"), channels);

    return root;
}

bool configFromJson(const QJsonObject &object, AppConfig &config, QStringList *errors)
{
    QStringList localErrors;
    if (object.value(QStringLiteral("schemaVersion")).toInt(-1) != 1) {
        localErrors.append(QStringLiteral("schemaVersion phải là 1"));
        if (errors)
            *errors = localErrors;
        return false;
    }

    AppConfig parsed;
    parsed.schemaVersion = 1;

    const QJsonObject branding = object.value(QStringLiteral("branding")).toObject();
    parsed.branding.companyName = readString(branding, "companyName");
    parsed.branding.branchName = readString(branding, "branchName");
    parsed.branding.address = readString(branding, "address");
    parsed.branding.regulationText = readString(branding, "regulationText");
    parsed.branding.footerNote = readString(branding, "footerNote");

    const QJsonObject display = object.value(QStringLiteral("display")).toObject();
    parsed.display.targetScreenName = readString(display, "targetScreenName");
    parsed.display.fullScreen = readBool(display, "fullScreen", true);
    parsed.display.designWidth = readInt(display, "designWidth", 1920);
    parsed.display.designHeight = readInt(display, "designHeight", 1200);

    const QJsonObject storage = object.value(QStringLiteral("storage")).toObject();
    parsed.storage.checkpointRetentionDays = readInt(storage, "checkpointRetentionDays", 30);
    parsed.storage.checkpointMaxRows = readInt(storage, "checkpointMaxRows", 50000);
    parsed.storage.logRetentionDays = readInt(storage, "logRetentionDays", 14);
    parsed.storage.logMaxFileBytes = readInt64(storage, "logMaxFileBytes", 4 * 1024 * 1024);
    parsed.storage.logMaxTotalBytes = readInt64(storage, "logMaxTotalBytes", 64 * 1024 * 1024);

    for (const QJsonValue &value : object.value(QStringLiteral("sources")).toArray()) {
        if (!value.isObject()) {
            localErrors.append(QStringLiteral("sources phải là mảng object"));
            continue;
        }
        const QJsonObject obj = value.toObject();
        SourceConfig source;
        source.id = readString(obj, "id");
        source.name = readString(obj, "name");
        source.folder = readString(obj, "folder");
        source.stationCode = readString(obj, "stationCode");
        source.allowLegacyPrefix = readBool(obj, "allowLegacyPrefix", true);
        source.groupedFiveColumn = readBool(obj, "groupedFiveColumn", false);
        source.recursive = readBool(obj, "recursive", true);
        source.maxDepth = readInt(obj, "maxDepth", 4);
        source.staleAfterSeconds = readInt(obj, "staleAfterSeconds", 900);
        source.futureToleranceSeconds = readInt(obj, "futureToleranceSeconds", 120);
        parsed.sources.append(source);
    }

    for (const QJsonValue &value : object.value(QStringLiteral("channels")).toArray()) {
        if (!value.isObject()) {
            localErrors.append(QStringLiteral("channels phải là mảng object"));
            continue;
        }
        const QJsonObject obj = value.toObject();
        ChannelConfig channel;
        channel.id = readString(obj, "id");
        const QString group = readString(obj, "group");
        if (group == QLatin1String("wastewater"))
            channel.group = ChannelGroup::Wastewater;
        else if (group == QLatin1String("exhaust"))
            channel.group = ChannelGroup::Exhaust;
        else
            localErrors.append(QStringLiteral("Kênh %1: group không hợp lệ").arg(channel.id));
        channel.symbol = readString(obj, "symbol");
        channel.displayName = readString(obj, "displayName");
        channel.unit = readString(obj, "unit");
        channel.acceptedUnits = readStringList(obj, "acceptedUnits");
        channel.aliases = readStringList(obj, "aliases");
        channel.sourceId = readString(obj, "sourceId");
        channel.stationCode = readString(obj, "stationCode");
        channel.decimals = readInt(obj, "decimals", 1);
        channel.staleAfterSeconds = readInt(obj, "staleAfterSeconds", 900);
        channel.threshold = thresholdFromJson(obj.value(QStringLiteral("threshold")).toObject());
        parsed.channels.append(channel);
    }

    if (!localErrors.isEmpty()) {
        if (errors)
            *errors = localErrors;
        return false;
    }

    parsed.channels = orderChannels(parsed.channels);
    config = parsed;
    return true;
}

QList<ConfigIssue> validateConfig(const AppConfig &config)
{
    QList<ConfigIssue> issues;
    if (config.schemaVersion != 1)
        issues.append({QStringLiteral("schemaVersion"), QStringLiteral("Chỉ hỗ trợ schemaVersion = 1")});

    if (config.display.designWidth != 1920 || config.display.designHeight != 1200) {
        issues.append({QStringLiteral("display.designWidth/Height"),
                       QStringLiteral("Khung thiết kế bắt buộc 1920 × 1200 px")});
    }
    if (config.storage.checkpointMaxRows < 100)
        issues.append({QStringLiteral("storage.checkpointMaxRows"),
                       QStringLiteral("checkpointMaxRows phải >= 100")});
    if (config.storage.checkpointRetentionDays < 1)
        issues.append({QStringLiteral("storage.checkpointRetentionDays"),
                       QStringLiteral("checkpointRetentionDays phải >= 1")});

    QSet<QString> sourceIds;
    for (const auto &source : config.sources) {
        if (source.id.trimmed().isEmpty())
            issues.append({QStringLiteral("sources.id"), QStringLiteral("Nguồn thiếu id")});
        if (sourceIds.contains(source.id))
            issues.append({QStringLiteral("sources.id"), QStringLiteral("Nguồn trùng id: %1").arg(source.id)});
        sourceIds.insert(source.id);
        if (source.stationCode.trimmed().isEmpty()) {
            issues.append({QStringLiteral("sources.%1.stationCode").arg(source.id),
                           QStringLiteral("Nguồn %1 chưa có mã trạm").arg(source.id)});
        }
        if (!source.folder.trimmed().isEmpty() && !QDir::isAbsolutePath(source.folder.trimmed())) {
            issues.append({QStringLiteral("sources.%1.folder").arg(source.id),
                           QStringLiteral("Thư mục nguồn phải là đường dẫn tuyệt đối")});
        }
        if (source.staleAfterSeconds < 10)
            issues.append({QStringLiteral("sources.%1.staleAfterSeconds").arg(source.id),
                           QStringLiteral("staleAfterSeconds phải >= 10")});
        if (source.futureToleranceSeconds < 0 || source.futureToleranceSeconds > 3600)
            issues.append({QStringLiteral("sources.%1.futureToleranceSeconds").arg(source.id),
                           QStringLiteral("futureToleranceSeconds phải trong 0..3600")});
        if (source.maxDepth < 0 || source.maxDepth > 8)
            issues.append({QStringLiteral("sources.%1.maxDepth").arg(source.id),
                           QStringLiteral("maxDepth phải trong 0..8")});
    }

    const QStringList canonical = canonicalChannelIds();
    QSet<QString> seenChannels;
    QHash<QString, QString> aliasOwner; // "source|alias" → channel
    for (const auto &channel : config.channels) {
        const QString prefix = QStringLiteral("channels.%1").arg(channel.id);
        if (channel.id.trimmed().isEmpty())
            issues.append({QStringLiteral("channels.id"), QStringLiteral("Kênh thiếu id")});
        if (seenChannels.contains(channel.id))
            issues.append({prefix, QStringLiteral("Kênh trùng id: %1").arg(channel.id)});
        seenChannels.insert(channel.id);
        if (!canonical.contains(channel.id))
            issues.append({prefix, QStringLiteral("Kênh không thuộc 12 vị trí đã chốt")});

        if (!config.sourceById(channel.sourceId)) {
            issues.append({prefix + QStringLiteral(".sourceId"),
                           QStringLiteral("Nguồn %1 không tồn tại").arg(channel.sourceId)});
        } else {
            const SourceConfig *source = config.sourceById(channel.sourceId);
            if (source && source->stationCode != channel.stationCode) {
                issues.append({prefix + QStringLiteral(".stationCode"),
                               QStringLiteral("Mã trạm %1 không khớp nguồn %2 (%3)")
                                   .arg(channel.stationCode, channel.sourceId, source->stationCode)});
            }
        }
        if (channel.aliases.isEmpty())
            issues.append({prefix + QStringLiteral(".aliases"),
                           QStringLiteral("Kênh %1 chưa có alias ký hiệu").arg(channel.symbol)});
        for (const QString &alias : channel.aliases) {
            const QString key = channel.sourceId + QLatin1Char('|') + normalizeSymbol(alias);
            const QString owner = aliasOwner.value(key);
            if (!owner.isEmpty() && owner != channel.id) {
                issues.append({prefix + QStringLiteral(".aliases"),
                               QStringLiteral("Alias %1 trùng với kênh %2 trong cùng nguồn")
                                   .arg(alias, owner)});
            } else if (owner.isEmpty()) {
                aliasOwner.insert(key, channel.id);
            }
        }
        if (channel.acceptedUnits.isEmpty())
            issues.append({prefix + QStringLiteral(".acceptedUnits"),
                           QStringLiteral("Kênh %1 chưa có đơn vị chấp nhận").arg(channel.symbol)});
        if (channel.decimals < 0 || channel.decimals > 6)
            issues.append({prefix + QStringLiteral(".decimals"),
                           QStringLiteral("decimals phải trong 0..6")});
        if (channel.staleAfterSeconds < 10)
            issues.append({prefix + QStringLiteral(".staleAfterSeconds"),
                           QStringLiteral("staleAfterSeconds phải >= 10")});
        if (channel.threshold.enabled) {
            if (!channel.threshold.hasLow && !channel.threshold.hasHigh) {
                issues.append({prefix + QStringLiteral(".threshold"),
                               QStringLiteral("Ngưỡng đang bật nhưng chưa nhập cận dưới/cận trên")});
            }
            if (channel.threshold.hasLow && !std::isfinite(channel.threshold.low))
                issues.append({prefix + QStringLiteral(".threshold.low"), QStringLiteral("Ngưỡng dưới không hợp lệ")});
            if (channel.threshold.hasHigh && !std::isfinite(channel.threshold.high))
                issues.append({prefix + QStringLiteral(".threshold.high"), QStringLiteral("Ngưỡng trên không hợp lệ")});
            if (channel.threshold.hasLow && channel.threshold.hasHigh
                && channel.threshold.low > channel.threshold.high) {
                issues.append({prefix + QStringLiteral(".threshold"),
                               QStringLiteral("Ngưỡng dưới lớn hơn ngưỡng trên")});
            }
        }
    }

    for (const QString &id : canonical) {
        if (!seenChannels.contains(id))
            issues.append({QStringLiteral("channels"), QStringLiteral("Thiếu kênh bắt buộc: %1").arg(id)});
    }

    return issues;
}

QStringList configIssuesToStrings(const QList<ConfigIssue> &issues)
{
    QStringList result;
    for (const auto &issue : issues)
        result.append(QStringLiteral("%1: %2").arg(issue.field, issue.message));
    return result;
}

} // namespace caongan
