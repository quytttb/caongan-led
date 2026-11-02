#include "SimulationFactory.h"

#include "domain/LatestReadingStore.h"
#include "domain/ParameterCatalog.h"
#include "domain/Tt10Time.h"

#include <QHash>

namespace caongan {

namespace {

struct DemoValue {
    double value;
    int decimals;
};

QHash<QString, DemoValue> demoValues()
{
    return {
        {QStringLiteral("o2"), {5.6, 1}},
        {QStringLiteral("co"), {46.0, 1}},
        {QStringLiteral("nox"), {97.0, 1}},
        {QStringLiteral("so2"), {89.0, 1}},
        {QStringLiteral("pm"), {27.0, 1}},
        {QStringLiteral("temp_flue"), {127.0, 1}},
        {QStringLiteral("flow_flue"), {538964.0, 0}},
        {QStringLiteral("pressure_flue"), {0.92, 2}},
        {QStringLiteral("ph"), {7.9, 1}},
        {QStringLiteral("tss"), {26.0, 1}},
        {QStringLiteral("cod"), {37.0, 1}},
        {QStringLiteral("nh4"), {0.87, 2}},
    };
}

Reading makeReading(const ChannelConfig &channel,
                    const SourceConfig *source,
                    double value,
                    const QDateTime &measuredAtUtc,
                    const QDateTime &receivedAtUtc,
                    DeviceStatus status = DeviceStatus::Measuring,
                    const QString &unitOverride = {})
{
    Reading reading;
    reading.channelId = channel.id;
    reading.sourceId = channel.sourceId;
    reading.stationCode = channel.stationCode;
    reading.rawSymbol = channel.aliases.value(0, channel.symbol);
    reading.value = value;
    reading.rawValue = QString::number(value, 'f', channel.decimals);
    reading.unit = unitOverride.isEmpty() ? channel.unit : unitOverride;
    reading.measuredAtUtc = measuredAtUtc;
    reading.measuredAtText = tt10::formatTimestamp(measuredAtUtc);
    reading.deviceStatus = status;
    reading.validity = Validity::Valid;
    reading.receivedAtUtc = receivedAtUtc;
    reading.filePath = QStringLiteral("(mô phỏng)");
    reading.contentHash = QStringLiteral("simulation");
    Q_UNUSED(source)
    return reading;
}

ChannelConfig *findChannel(AppConfig &config, const QString &id)
{
    return config.channelById(id);
}

} // namespace

QStringList simulationScenarioNames()
{
    return {
        QStringLiteral("Bình thường"),
        QStringLiteral("Vượt ngưỡng"),
        QStringLiteral("Trong giới hạn"),
        QStringLiteral("Hiệu chuẩn"),
        QStringLiteral("Lỗi thiết bị"),
        QStringLiteral("Bảo trì"),
        QStringLiteral("Dữ liệu cũ"),
        QStringLiteral("Thiếu pH"),
        QStringLiteral("Sai đơn vị"),
        QStringLiteral("Xung đột"),
        QStringLiteral("Flow 6 chữ số"),
    };
}

QVector<ChannelState> buildSimulationStates(const AppConfig &config,
                                            const QString &scenario,
                                            const QDateTime &nowUtc)
{
    AppConfig demo = config;
    demo.channels = orderChannels(demo.channels);

    if (scenario == QStringLiteral("Vượt ngưỡng")) {
        if (auto *so2 = findChannel(demo, QStringLiteral("so2"))) {
            so2->threshold.enabled = true;
            so2->threshold.hasHigh = true;
            so2->threshold.high = 50.0;
        }
        if (auto *cod = findChannel(demo, QStringLiteral("cod"))) {
            cod->threshold.enabled = true;
            cod->threshold.hasHigh = true;
            cod->threshold.high = 30.0;
        }
    } else if (scenario == QStringLiteral("Trong giới hạn")) {
        if (auto *so2 = findChannel(demo, QStringLiteral("so2"))) {
            so2->threshold.enabled = true;
            so2->threshold.hasHigh = true;
            so2->threshold.high = 200.0;
        }
        if (auto *cod = findChannel(demo, QStringLiteral("cod"))) {
            cod->threshold.enabled = true;
            cod->threshold.hasHigh = true;
            cod->threshold.high = 100.0;
        }
    }

    LatestReadingStore store;
    store.setConfig(demo);

    const QHash<QString, DemoValue> values = demoValues();
    const QDateTime freshTime = nowUtc.addSecs(-60);
    const QDateTime oldTime = nowUtc.addSecs(-3 * 3600);

    for (const ChannelConfig &channel : demo.channels) {
        if (scenario == QStringLiteral("Thiếu pH") && channel.id == QLatin1String("ph"))
            continue;

        const DemoValue demoValue = values.value(channel.id, {0.0, channel.decimals});
        QDateTime measured = freshTime;
        DeviceStatus status = DeviceStatus::Measuring;
        QString unitOverride;

        if (scenario == QStringLiteral("Dữ liệu cũ")
            && (channel.id == QLatin1String("o2") || channel.id == QLatin1String("ph"))) {
            measured = oldTime;
        }
        if (scenario == QStringLiteral("Hiệu chuẩn")
            && (channel.id == QLatin1String("o2") || channel.id == QLatin1String("ph"))) {
            status = DeviceStatus::Calibrating;
        }
        if (scenario == QStringLiteral("Lỗi thiết bị")
            && (channel.id == QLatin1String("so2") || channel.id == QLatin1String("cod"))) {
            status = DeviceStatus::DeviceFault;
        }
        if (scenario == QStringLiteral("Bảo trì")
            && (channel.id == QLatin1String("so2") || channel.id == QLatin1String("cod"))) {
            status = DeviceStatus::Maintenance;
        }
        if (scenario == QStringLiteral("Sai đơn vị") && channel.id == QLatin1String("pm"))
            unitOverride = QStringLiteral("mg/m3");
        if (scenario == QStringLiteral("Flow 6 chữ số") && channel.id == QLatin1String("flow_flue")) {
            // Giữ nguyên 6 chữ số có nghĩa để kiểm tra không elide/không đổi ký pháp.
        }

        const SourceConfig *source = demo.sourceById(channel.sourceId);
        store.apply(makeReading(channel, source, demoValue.value, measured, nowUtc, status, unitOverride), nowUtc);

        if (scenario == QStringLiteral("Xung đột") && channel.id == QLatin1String("so2")) {
            Reading conflicting = makeReading(channel, source, demoValue.value + 10.0,
                                              measured, nowUtc, status, unitOverride);
            store.apply(conflicting, nowUtc);
        }
    }

    return store.evaluate(nowUtc);
}

} // namespace caongan
