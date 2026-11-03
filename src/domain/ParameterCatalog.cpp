#include "ParameterCatalog.h"

#include <QHash>

#include <algorithm>

namespace caongan {

namespace {

ChannelConfig makeChannel(const QString &id,
                          ChannelGroup group,
                          const QString &symbol,
                          const QString &displayName,
                          const QString &unit,
                          const QString &rangeText,
                          bool thresholdEnabled,
                          double thresholdLow,
                          double thresholdHigh,
                          const QStringList &acceptedUnits,
                          const QStringList &aliases,
                          const QString &sourceId,
                          const QString &stationCode,
                          int decimals)
{
    ChannelConfig channel;
    channel.id = id;
    channel.group = group;
    channel.symbol = symbol;
    channel.displayName = displayName;
    channel.unit = unit;
    channel.rangeText = rangeText;
    channel.acceptedUnits = acceptedUnits;
    channel.aliases = aliases;
    channel.sourceId = sourceId;
    channel.stationCode = stationCode;
    channel.decimals = decimals;
    channel.threshold.enabled = thresholdEnabled;
    channel.threshold.hasLow = true;
    channel.threshold.low = thresholdLow;
    channel.threshold.hasHigh = true;
    channel.threshold.high = thresholdHigh;
    return channel;
}

} // namespace

QStringList canonicalChannelIds()
{
    return {
        QStringLiteral("o2"),
        QStringLiteral("co"),
        QStringLiteral("nox"),
        QStringLiteral("so2"),
        QStringLiteral("pm"),
        QStringLiteral("temp_flue"),
        QStringLiteral("flow_flue"),
        QStringLiteral("pressure_flue"),
        QStringLiteral("ph"),
        QStringLiteral("tss"),
        QStringLiteral("cod"),
        QStringLiteral("nh4"),
    };
}

QList<SourceConfig> defaultSources()
{
    SourceConfig flue;
    flue.id = QStringLiteral("khi-thai");
    flue.name = QStringLiteral("Khí thải");
    flue.stationCode = QStringLiteral("KHITO1");
    flue.staleAfterSeconds = 900;
    flue.futureToleranceSeconds = 120;

    SourceConfig water;
    water.id = QStringLiteral("nuoc-thai");
    water.name = QStringLiteral("Nước thải");
    water.stationCode = QStringLiteral("NUONT1");
    water.staleAfterSeconds = 900;
    water.futureToleranceSeconds = 120;

    return {flue, water};
}

QList<ChannelConfig> defaultChannels()
{
    const QString flueSource = QStringLiteral("khi-thai");
    const QString waterSource = QStringLiteral("nuoc-thai");
    const QString flueStation = QStringLiteral("KHITO1");
    const QString waterStation = QStringLiteral("NUONT1");

    // Ngưỡng cố định theo dải đo nhà máy cung cấp (khí thải) và quy chuẩn (nước thải).
    // Không cho sửa trên màn hình config.
    const double kLowZero = 0.0;

    QList<ChannelConfig> channels;

    // Khí thải: ngưỡng bằng dải đo
    channels << makeChannel(QStringLiteral("o2"), ChannelGroup::Exhaust,
                            QStringLiteral("O₂"), QStringLiteral("Oxy"),
                            QStringLiteral("%V"), QStringLiteral("0 - 25 %V"),
                            true, kLowZero, 25.0,
                            {QStringLiteral("%V"), QStringLiteral("%")},
                            {QStringLiteral("O2"), QStringLiteral("O₂")},
                            flueSource, flueStation, 1);

    channels << makeChannel(QStringLiteral("co"), ChannelGroup::Exhaust,
                            QStringLiteral("CO"), QStringLiteral("Cacbon monoxit"),
                            QStringLiteral("mg/Nm³"), QStringLiteral("0 - 1.500 mg/Nm³"),
                            true, kLowZero, 1500.0,
                            {QStringLiteral("mg/Nm³")},
                            {QStringLiteral("CO")},
                            flueSource, flueStation, 1);

    channels << makeChannel(QStringLiteral("nox"), ChannelGroup::Exhaust,
                            QStringLiteral("NOₓ"), QStringLiteral("Nitơ oxit"),
                            QStringLiteral("mg/Nm³"), QStringLiteral("0 - 1.500 mg/Nm³"),
                            true, kLowZero, 1500.0,
                            {QStringLiteral("mg/Nm³")},
                            {QStringLiteral("NOx"), QStringLiteral("NOX"), QStringLiteral("NOₓ")},
                            flueSource, flueStation, 1);

    channels << makeChannel(QStringLiteral("so2"), ChannelGroup::Exhaust,
                            QStringLiteral("SO₂"), QStringLiteral("Lưu huỳnh đioxit"),
                            QStringLiteral("mg/Nm³"), QStringLiteral("0 - 1.000 mg/Nm³"),
                            true, kLowZero, 1000.0,
                            {QStringLiteral("mg/Nm³")},
                            {QStringLiteral("SO2"), QStringLiteral("SO₂")},
                            flueSource, flueStation, 1);

    channels << makeChannel(QStringLiteral("pm"), ChannelGroup::Exhaust,
                            QStringLiteral("PM"), QStringLiteral("Bụi"),
                            QStringLiteral("mg/Nm³"), QStringLiteral("0 - 1.000 mg/Nm³"),
                            true, kLowZero, 1000.0,
                            {QStringLiteral("mg/Nm³")},
                            {QStringLiteral("PM")},
                            flueSource, flueStation, 1);

    channels << makeChannel(QStringLiteral("temp_flue"), ChannelGroup::Exhaust,
                            QStringLiteral("Temp"), QStringLiteral("Nhiệt độ khí thải"),
                            QStringLiteral("°C"), QStringLiteral("0 - 500 °C"),
                            true, kLowZero, 500.0,
                            {QStringLiteral("°C")},
                            {QStringLiteral("Temp"), QStringLiteral("TEMP")},
                            flueSource, flueStation, 1);

    channels << makeChannel(QStringLiteral("flow_flue"), ChannelGroup::Exhaust,
                            QStringLiteral("Flow"), QStringLiteral("Lưu lượng khí"),
                            QStringLiteral("m³/h"), QStringLiteral("0 - 1.000.000 m³/h"),
                            true, kLowZero, 1000000.0,
                            {QStringLiteral("m³/h")},
                            {QStringLiteral("Flow")},
                            flueSource, flueStation, 0);

    channels << makeChannel(QStringLiteral("pressure_flue"), ChannelGroup::Exhaust,
                            QStringLiteral("P"), QStringLiteral("Áp suất"),
                            QStringLiteral("kPa"), QStringLiteral("0 - 160 kPa"),
                            true, kLowZero, 160.0,
                            {QStringLiteral("kPa")},
                            {QStringLiteral("P")},
                            flueSource, flueStation, 2);

    // Nước thải: ngưỡng theo quy chuẩn được duyệt (không phải dải đo thiết bị)
    channels << makeChannel(QStringLiteral("ph"), ChannelGroup::Wastewater,
                            QStringLiteral("pH"), QStringLiteral("Độ pH"),
                            QStringLiteral("-"), QStringLiteral("6 - 9"),
                            true, 6.0, 9.0,
                            {QStringLiteral("-")},
                            {QStringLiteral("pH"), QStringLiteral("PH")},
                            waterSource, waterStation, 1);

    channels << makeChannel(QStringLiteral("tss"), ChannelGroup::Wastewater,
                            QStringLiteral("TSS"), QStringLiteral("Chất rắn lơ lửng"),
                            QStringLiteral("mg/L"), QStringLiteral("0 - 55 mg/L"),
                            true, kLowZero, 55.0,
                            {QStringLiteral("mg/L")},
                            {QStringLiteral("TSS")},
                            waterSource, waterStation, 1);

    channels << makeChannel(QStringLiteral("cod"), ChannelGroup::Wastewater,
                            QStringLiteral("COD"), QStringLiteral("Nhu cầu oxy hóa học"),
                            QStringLiteral("mg/L"), QStringLiteral("0 - 82,5 mg/L"),
                            true, kLowZero, 82.5,
                            {QStringLiteral("mg/L")},
                            {QStringLiteral("COD")},
                            waterSource, waterStation, 1);

    channels << makeChannel(QStringLiteral("nh4"), ChannelGroup::Wastewater,
                            QStringLiteral("NH₄⁺"), QStringLiteral("Amoni"),
                            QStringLiteral("mg/L"), QStringLiteral("0 - 5,5 mg/L"),
                            true, kLowZero, 5.5,
                            {QStringLiteral("mg/L")},
                            {QStringLiteral("NH4+"), QStringLiteral("NH₄⁺"),
                             QStringLiteral("NH4"), QStringLiteral("NH₄")},
                            waterSource, waterStation, 2);

    return channels;
}

AppConfig defaultAppConfig()
{
    AppConfig config;
    config.schemaVersion = 1;
    config.branding.companyName = QStringLiteral("CÔNG TY NHIỆT ĐIỆN CAO NGẠN - TKV");
    config.branding.branchName =
        QStringLiteral("CHI NHÁNH TỔNG CÔNG TY ĐIỆN LỰC TKV - CTCP");
    config.branding.address =
        QStringLiteral("Ngõ 517 đường Dương Tự Minh, Phường Quan Triều, tỉnh Thái Nguyên");
    config.sources = defaultSources();
    config.channels = defaultChannels();
    return config;
}

QList<ChannelConfig> orderChannels(const QList<ChannelConfig> &channels)
{
    QHash<QString, int> order;
    const QStringList ids = canonicalChannelIds();
    for (int i = 0; i < ids.size(); ++i)
        order.insert(ids.at(i), i);

    QList<ChannelConfig> sorted = channels;
    std::stable_sort(sorted.begin(), sorted.end(), [&order](const ChannelConfig &a, const ChannelConfig &b) {
        const int ai = order.value(a.id, 1000);
        const int bi = order.value(b.id, 1000);
        if (ai != bi)
            return ai < bi;
        return a.id < b.id;
    });
    return sorted;
}

QString normalizeUnit(const QString &unit)
{
    QString value = unit.trimmed();
    if (value.isEmpty())
        return value;
    // Chỉ chuẩn hóa cách viết tương đương, không quy đổi vật lý.
    value.replace(QStringLiteral("oC"), QStringLiteral("°C"));
    value.replace(QStringLiteral("0C"), QStringLiteral("°C"));
    value.replace(QStringLiteral("° C"), QStringLiteral("°C"));
    value.replace(QStringLiteral("mg/m3"), QStringLiteral("mg/m³"));
    value.replace(QStringLiteral("mg/Nm3"), QStringLiteral("mg/Nm³"));
    value.replace(QStringLiteral("mg/Nm3"), QStringLiteral("mg/Nm³"));
    value.replace(QStringLiteral("mg/nM3"), QStringLiteral("mg/Nm³"));
    value.replace(QStringLiteral("m3/h"), QStringLiteral("m³/h"));
    return value;
}

QString normalizeSymbol(const QString &symbol)
{
    // Chỉ dùng để so khớp alias cấu hình, không "đoán" NO/NO₂/NOx.
    QString value = symbol.trimmed().toUpper();
    value.remove(QChar(0xFEFF));
    return value;
}

bool unitAccepted(const ChannelConfig &channel, const QString &unit)
{
    const QString normalized = normalizeUnit(unit);
    const QString configured = normalizeUnit(channel.unit);
    if (normalized == configured)
        return true;
    for (const QString &accepted : channel.acceptedUnits) {
        if (normalizeUnit(accepted) == normalized)
            return true;
    }
    return false;
}

} // namespace caongan
