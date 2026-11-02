#include "domain/DisplayState.h"
#include "domain/Freshness.h"
#include "domain/LatestReadingStore.h"
#include "domain/ParameterCatalog.h"
#include "domain/ThresholdEvaluator.h"
#include "domain/Tt10Time.h"

#include <QDateTime>
#include <QDir>
#include <QTest>

#include <limits>

using namespace caongan;

namespace {

QDateTime utc(const QString &text)
{
    return QDateTime::fromString(text, Qt::ISODate);
}

Reading makeReading(const QString &id,
                    const QString &sourceId,
                    const QString &station,
                    const QString &unit,
                    std::optional<double> value,
                    const QDateTime &measuredAt,
                    DeviceStatus status = DeviceStatus::Measuring,
                    Validity validity = Validity::Valid)
{
    Reading reading;
    reading.channelId = id;
    reading.sourceId = sourceId;
    reading.stationCode = station;
    reading.rawSymbol = id.toUpper();
    reading.rawValue = value.has_value() ? QString::number(*value) : QStringLiteral("---");
    reading.value = value;
    reading.unit = unit;
    reading.measuredAtUtc = measuredAt;
    reading.measuredAtText = tt10::formatTimestamp(measuredAt);
    reading.deviceStatus = status;
    reading.validity = validity;
    reading.receivedAtUtc = measuredAt.addSecs(5);
    return reading;
}

Reading coReading(std::optional<double> value,
                  const QDateTime &measuredAt,
                  DeviceStatus status = DeviceStatus::Measuring,
                  Validity validity = Validity::Valid)
{
    return makeReading(QStringLiteral("co"), QStringLiteral("khi-thai"),
                       QStringLiteral("KHITO1"), QStringLiteral("mg/Nm³"),
                       value, measuredAt, status, validity);
}

const ChannelState *findState(const QVector<ChannelState> &states, const QString &id)
{
    for (const ChannelState &state : states) {
        if (state.config.id == id)
            return &state;
    }
    return nullptr;
}

AppConfig configWithCoThreshold(bool enabled, bool hasLow, double low, bool hasHigh, double high)
{
    AppConfig config = defaultAppConfig();
    ChannelConfig *co = config.channelById(QStringLiteral("co"));
    co->threshold.enabled = enabled;
    co->threshold.hasLow = hasLow;
    co->threshold.low = low;
    co->threshold.hasHigh = hasHigh;
    co->threshold.high = high;
    return config;
}

} // namespace

class TestReadingStore : public QObject
{
    Q_OBJECT

private slots:
    void appliesLatestByMeasurementTime();
    void duplicateAndConflict();
    void rejectsUnknownSourceAndFuture();
    void zeroIsValidReading();
    void newerInvalidReplacesOldGood();
    void deviceStatusOverridesThreshold();
    void maintenanceAndFaultLabels();
    void freshnessBoundaries();
    void staleAndFutureDisplay();
    void unitMismatch();
    void thresholdBoundaries();
    void thresholdNotEvaluatedWhenNotTrustworthy();
    void thresholdComparedBeforeRounding();
    void setConfigDropsIncompatibleReading();
    void loadPersistedUsesCurrentClock();
    void restoredConflictAndWrongStation();
    void formattingHelpers();
};

void TestReadingStore::appliesLatestByMeasurementTime()
{
    LatestReadingStore store;
    store.setConfig(defaultAppConfig());
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));

    QCOMPARE(store.apply(coReading(10.0, now.addSecs(-300)), now).code,
             ApplyOutcome::Code::Accepted);
    QCOMPARE(store.apply(coReading(5.0, now.addSecs(-420)), now).code,
             ApplyOutcome::Code::OutOfOrderIgnored);
    QCOMPARE(store.apply(coReading(20.0, now.addSecs(-240)), now).code,
             ApplyOutcome::Code::Accepted);

    Reading latest;
    QVERIFY(store.latestReading(QStringLiteral("co"), &latest));
    QCOMPARE(latest.value.value_or(-1.0), 20.0);

    const QVector<ChannelState> states = store.evaluate(now);
    QCOMPARE(states.size(), 12);
    const ChannelState *co = findState(states, QStringLiteral("co"));
    QVERIFY(co);
    QVERIFY(co->hasReading);
    QCOMPARE(co->valueText, QStringLiteral("20.0"));
    QCOMPARE(co->display, DisplayState::Measuring);
    QCOMPARE(co->threshold, ThresholdResult::NotConfigured);
    QCOMPARE(co->freshness, Freshness::Fresh);
}

void TestReadingStore::duplicateAndConflict()
{
    LatestReadingStore store;
    store.setConfig(configWithCoThreshold(true, false, 0.0, true, 100.0));
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));
    const QDateTime measured = now.addSecs(-60);

    QCOMPARE(store.apply(coReading(10.0, measured), now).code,
             ApplyOutcome::Code::Accepted);
    QCOMPARE(store.apply(coReading(10.0, measured), now).code,
             ApplyOutcome::Code::DuplicateIgnored);
    QCOMPARE(store.apply(coReading(11.0, measured), now).code,
             ApplyOutcome::Code::ConflictRecorded);

    Reading latest;
    QVERIFY(store.latestReading(QStringLiteral("co"), &latest));
    QVERIFY(latest.conflict);
    QCOMPARE(latest.value.value_or(-1.0), 10.0);

    QVector<ChannelState> states = store.evaluate(now);
    const ChannelState *co = findState(states, QStringLiteral("co"));
    QVERIFY(co);
    QCOMPARE(co->validity, Validity::Conflict);
    QCOMPARE(co->display, DisplayState::Conflict);
    QCOMPARE(co->threshold, ThresholdResult::NotEvaluated);
    QCOMPARE(co->severity, QStringLiteral("alarm"));

    // Bản ghi mới hơn hợp lệ phải xóa cờ xung đột của mốc cũ.
    QCOMPARE(store.apply(coReading(12.0, measured.addSecs(60)), now).code,
             ApplyOutcome::Code::Accepted);
    QVERIFY(store.latestReading(QStringLiteral("co"), &latest));
    QVERIFY(!latest.conflict);
    states = store.evaluate(now);
    co = findState(states, QStringLiteral("co"));
    QVERIFY(co);
    QCOMPARE(co->display, DisplayState::WithinLimit);
}

void TestReadingStore::rejectsUnknownSourceAndFuture()
{
    LatestReadingStore store;
    store.setConfig(defaultAppConfig());
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));

    Reading unknown = coReading(1.0, now);
    unknown.channelId = QStringLiteral("khong-co");
    QCOMPARE(store.apply(unknown, now).code, ApplyOutcome::Code::UnknownChannel);

    Reading wrongSource = coReading(1.0, now);
    wrongSource.sourceId = QStringLiteral("nuoc-thai");
    QCOMPARE(store.apply(wrongSource, now).code, ApplyOutcome::Code::SourceMismatch);

    Reading wrongStation = coReading(1.0, now);
    wrongStation.stationCode = QStringLiteral("OTHER");
    QCOMPARE(store.apply(wrongStation, now).code, ApplyOutcome::Code::SourceMismatch);

    Reading tooFuture = coReading(1.0, now.addSecs(121));
    QCOMPARE(store.apply(tooFuture, now).code, ApplyOutcome::Code::FutureRejected);

    Reading withinTolerance = coReading(2.0, now.addSecs(120));
    QCOMPARE(store.apply(withinTolerance, now).code, ApplyOutcome::Code::Accepted);

    const ChannelState *co = findState(store.evaluate(now), QStringLiteral("co"));
    QVERIFY(co);
    QCOMPARE(co->freshness, Freshness::Fresh);
    QCOMPARE(co->display, DisplayState::Measuring);
}

void TestReadingStore::zeroIsValidReading()
{
    LatestReadingStore store;
    store.setConfig(configWithCoThreshold(true, false, 0.0, true, 100.0));
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));

    QCOMPARE(store.apply(coReading(0.0, now.addSecs(-30)), now).code,
             ApplyOutcome::Code::Accepted);
    const ChannelState *co = findState(store.evaluate(now), QStringLiteral("co"));
    QVERIFY(co);
    QVERIFY(co->hasReading);
    QVERIFY(co->reading.value.has_value());
    QCOMPARE(*co->reading.value, 0.0);
    QCOMPARE(co->display, DisplayState::WithinLimit);
    QCOMPARE(co->valueText, QStringLiteral("0.0"));
    QCOMPARE(co->validity, Validity::Valid);
}

void TestReadingStore::newerInvalidReplacesOldGood()
{
    LatestReadingStore store;
    store.setConfig(configWithCoThreshold(true, false, 0.0, true, 100.0));
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));

    QCOMPARE(store.apply(coReading(42.0, now.addSecs(-120)), now).code,
             ApplyOutcome::Code::Accepted);
    QCOMPARE(store.apply(coReading(std::nullopt, now.addSecs(-60),
                                   DeviceStatus::Measuring, Validity::Invalid), now).code,
             ApplyOutcome::Code::Accepted);

    const ChannelState *co = findState(store.evaluate(now), QStringLiteral("co"));
    QVERIFY(co);
    QCOMPARE(co->display, DisplayState::Invalid);
    QCOMPARE(co->valueText, QStringLiteral("—"));
    QCOMPARE(co->threshold, ThresholdResult::NotEvaluated);
}

void TestReadingStore::deviceStatusOverridesThreshold()
{
    LatestReadingStore store;
    store.setConfig(configWithCoThreshold(true, false, 0.0, true, 1000.0));
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));

    QCOMPARE(store.apply(coReading(5.0, now.addSecs(-30), DeviceStatus::Calibrating), now).code,
             ApplyOutcome::Code::Accepted);
    const ChannelState *calibrating = findState(store.evaluate(now), QStringLiteral("co"));
    QVERIFY(calibrating);
    QCOMPARE(calibrating->display, DisplayState::Calibrating);
    QCOMPARE(calibrating->threshold, ThresholdResult::NotEvaluated);
    QCOMPARE(calibrating->severity, QStringLiteral("warning"));

    QCOMPARE(store.apply(coReading(5.0, now.addSecs(-20), DeviceStatus::DeviceFault), now).code,
             ApplyOutcome::Code::Accepted);
    const ChannelState *fault = findState(store.evaluate(now), QStringLiteral("co"));
    QVERIFY(fault);
    QCOMPARE(fault->display, DisplayState::DeviceFault);
    QCOMPARE(fault->threshold, ThresholdResult::NotEvaluated);

    QCOMPARE(store.apply(coReading(5.0, now.addSecs(-10), DeviceStatus::Unknown), now).code,
             ApplyOutcome::Code::Accepted);
    const ChannelState *unknown = findState(store.evaluate(now), QStringLiteral("co"));
    QVERIFY(unknown);
    QCOMPARE(unknown->display, DisplayState::UnknownStatus);
    QCOMPARE(unknown->threshold, ThresholdResult::NotEvaluated);
}

void TestReadingStore::maintenanceAndFaultLabels()
{
    LatestReadingStore store;
    store.setConfig(configWithCoThreshold(true, false, 0.0, true, 100.0));
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));
    QCOMPARE(store.apply(coReading(200.0, now.addSecs(-30), DeviceStatus::Maintenance), now).code,
             ApplyOutcome::Code::Accepted);
    QVector<ChannelState> states = store.evaluate(now);
    const ChannelState *co = findState(states, QStringLiteral("co"));
    QVERIFY(co);
    QCOMPARE(co->display, DisplayState::Maintenance);
    QCOMPARE(co->statusText, QStringLiteral("BẢO TRÌ"));
    QCOMPARE(co->stateKey, QStringLiteral("maintenance"));
    QCOMPARE(co->threshold, ThresholdResult::NotEvaluated);
    QCOMPARE(co->valueText, QStringLiteral("—"));

    QCOMPARE(store.apply(coReading(20.0, now.addSecs(-20), DeviceStatus::DeviceFault), now).code,
             ApplyOutcome::Code::Accepted);
    states = store.evaluate(now);
    co = findState(states, QStringLiteral("co"));
    QVERIFY(co);
    QCOMPARE(co->statusText, QStringLiteral("LỖI"));
    QCOMPARE(co->threshold, ThresholdResult::NotEvaluated);
}

void TestReadingStore::freshnessBoundaries()
{
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));
    Reading reading = coReading(1.0, now.addSecs(-899));

    FreshnessResult result = evaluateFreshness(reading, now, 900, 120);
    QCOMPARE(result.freshness, Freshness::Fresh);
    QCOMPARE(result.ageSeconds, 899);

    reading.measuredAtUtc = now.addSecs(-900);
    result = evaluateFreshness(reading, now, 900, 120);
    QCOMPARE(result.freshness, Freshness::Stale);
    QCOMPARE(result.ageSeconds, 900);

    reading.measuredAtUtc = now.addSecs(120);
    QCOMPARE(evaluateFreshness(reading, now, 900, 120).freshness, Freshness::Fresh);

    reading.measuredAtUtc = now.addSecs(121);
    QCOMPARE(evaluateFreshness(reading, now, 900, 120).freshness, Freshness::FutureTimestamp);

    reading.measuredAtUtc = QDateTime();
    QCOMPARE(evaluateFreshness(reading, now, 900, 120).freshness, Freshness::FutureTimestamp);
}

void TestReadingStore::staleAndFutureDisplay()
{
    LatestReadingStore store;
    store.setConfig(configWithCoThreshold(true, false, 0.0, true, 100.0));
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));

    QCOMPARE(store.apply(coReading(50.0, now.addSecs(-901)), now).code,
             ApplyOutcome::Code::Accepted);
    const ChannelState *co = findState(store.evaluate(now), QStringLiteral("co"));
    QVERIFY(co);
    QCOMPARE(co->freshness, Freshness::Stale);
    QCOMPARE(co->display, DisplayState::Stale);
    QCOMPARE(co->threshold, ThresholdResult::NotEvaluated);
    QCOMPARE(co->statusText, QStringLiteral("DỮ LIỆU CŨ"));
    QCOMPARE(co->severity, QStringLiteral("muted"));

    Reading future = coReading(50.0, now.addSecs(100));
    store.apply(future, now);
    const QDateTime earlier = now.addSecs(-30);
    const ChannelState *futureState = findState(store.evaluate(earlier), QStringLiteral("co"));
    QVERIFY(futureState);
    QCOMPARE(futureState->freshness, Freshness::FutureTimestamp);
    QCOMPARE(futureState->display, DisplayState::FutureTimestamp);
    QCOMPARE(futureState->threshold, ThresholdResult::NotEvaluated);
}

void TestReadingStore::unitMismatch()
{
    LatestReadingStore store;
    store.setConfig(configWithCoThreshold(true, false, 0.0, true, 100.0));
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));

    Reading wrongUnit = coReading(10.0, now.addSecs(-30));
    wrongUnit.unit = QStringLiteral("mg/m³");
    QCOMPARE(store.apply(wrongUnit, now).code, ApplyOutcome::Code::Accepted);
    const ChannelState *co = findState(store.evaluate(now), QStringLiteral("co"));
    QVERIFY(co);
    QCOMPARE(co->validity, Validity::UnitMismatch);
    QCOMPARE(co->display, DisplayState::UnitMismatch);
    QCOMPARE(co->threshold, ThresholdResult::NotEvaluated);

    Reading normalized = coReading(10.0, now.addSecs(-20));
    normalized.unit = QStringLiteral("mg/Nm3");
    QCOMPARE(store.apply(normalized, now).code, ApplyOutcome::Code::Accepted);
    co = findState(store.evaluate(now), QStringLiteral("co"));
    QVERIFY(co);
    QCOMPARE(co->validity, Validity::Valid);
    QCOMPARE(co->display, DisplayState::WithinLimit);
}

void TestReadingStore::thresholdBoundaries()
{
    ChannelConfig co;
    co.id = QStringLiteral("co");
    co.decimals = 1;
    co.threshold.enabled = true;
    co.threshold.hasLow = true;
    co.threshold.low = 5.0;
    co.threshold.hasHigh = true;
    co.threshold.high = 10.0;

    Reading reading = coReading(5.0, utc(QStringLiteral("2026-01-01T05:00:00Z")));
    QCOMPARE(evaluateThreshold(co, reading, Validity::Valid, Freshness::Fresh, false),
             ThresholdResult::WithinConfiguredLimit);

    reading.value = 10.0;
    QCOMPARE(evaluateThreshold(co, reading, Validity::Valid, Freshness::Fresh, false),
             ThresholdResult::WithinConfiguredLimit);

    reading.value = 4.999;
    QCOMPARE(evaluateThreshold(co, reading, Validity::Valid, Freshness::Fresh, false),
             ThresholdResult::ExceedsConfiguredLimit);

    reading.value = 10.001;
    QCOMPARE(evaluateThreshold(co, reading, Validity::Valid, Freshness::Fresh, false),
             ThresholdResult::ExceedsConfiguredLimit);

    ChannelConfig ph;
    ph.id = QStringLiteral("ph");
    ph.threshold.enabled = true;
    ph.threshold.hasLow = true;
    ph.threshold.low = 6.0;
    ph.threshold.hasHigh = true;
    ph.threshold.high = 9.0;
    reading.value = 6.0;
    QCOMPARE(evaluateThreshold(ph, reading, Validity::Valid, Freshness::Fresh, false),
             ThresholdResult::WithinConfiguredLimit);
    reading.value = 9.0;
    QCOMPARE(evaluateThreshold(ph, reading, Validity::Valid, Freshness::Fresh, false),
             ThresholdResult::WithinConfiguredLimit);
    reading.value = 5.9;
    QCOMPARE(evaluateThreshold(ph, reading, Validity::Valid, Freshness::Fresh, false),
             ThresholdResult::ExceedsConfiguredLimit);
    reading.value = 9.1;
    QCOMPARE(evaluateThreshold(ph, reading, Validity::Valid, Freshness::Fresh, false),
             ThresholdResult::ExceedsConfiguredLimit);

    ph.threshold.enabled = false;
    QCOMPARE(evaluateThreshold(ph, reading, Validity::Valid, Freshness::Fresh, false),
             ThresholdResult::NotConfigured);
}

void TestReadingStore::thresholdNotEvaluatedWhenNotTrustworthy()
{
    ChannelConfig co;
    co.threshold.enabled = true;
    co.threshold.hasHigh = true;
    co.threshold.high = 100.0;
    Reading reading = coReading(10.0, utc(QStringLiteral("2026-01-01T05:00:00Z")));

    QCOMPARE(evaluateThreshold(co, reading, Validity::Invalid, Freshness::Fresh, false),
             ThresholdResult::NotEvaluated);
    QCOMPARE(evaluateThreshold(co, reading, Validity::UnitMismatch, Freshness::Fresh, false),
             ThresholdResult::NotEvaluated);
    QCOMPARE(evaluateThreshold(co, reading, Validity::Valid, Freshness::Stale, false),
             ThresholdResult::NotEvaluated);
    QCOMPARE(evaluateThreshold(co, reading, Validity::Valid, Freshness::FutureTimestamp, false),
             ThresholdResult::NotEvaluated);
    QCOMPARE(evaluateThreshold(co, reading, Validity::Valid, Freshness::Fresh, true),
             ThresholdResult::NotEvaluated);

    reading.deviceStatus = DeviceStatus::Calibrating;
    QCOMPARE(evaluateThreshold(co, reading, Validity::Valid, Freshness::Fresh, false),
             ThresholdResult::NotEvaluated);

    reading.deviceStatus = DeviceStatus::Measuring;
    reading.value = std::numeric_limits<double>::quiet_NaN();
    QCOMPARE(evaluateThreshold(co, reading, Validity::Valid, Freshness::Fresh, false),
             ThresholdResult::NotApplicable);

    co.threshold = Threshold();
    co.threshold.enabled = true;
    QCOMPARE(evaluateThreshold(co, reading, Validity::Valid, Freshness::Fresh, false),
             ThresholdResult::NotConfigured);
}

void TestReadingStore::thresholdComparedBeforeRounding()
{
    LatestReadingStore store;
    AppConfig config = configWithCoThreshold(true, false, 0.0, true, 10.0);
    config.channelById(QStringLiteral("co"))->decimals = 1;
    store.setConfig(config);
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));

    QCOMPARE(store.apply(coReading(10.04, now.addSecs(-30)), now).code,
             ApplyOutcome::Code::Accepted);
    const ChannelState *co = findState(store.evaluate(now), QStringLiteral("co"));
    QVERIFY(co);
    QCOMPARE(co->valueText, QStringLiteral("10.0"));
    QCOMPARE(co->threshold, ThresholdResult::ExceedsConfiguredLimit);
    QCOMPARE(co->display, DisplayState::ExceedsLimit);
}

void TestReadingStore::setConfigDropsIncompatibleReading()
{
    LatestReadingStore store;
    AppConfig config = defaultAppConfig();
    store.setConfig(config);
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));
    QCOMPARE(store.apply(coReading(10.0, now.addSecs(-30)), now).code,
             ApplyOutcome::Code::Accepted);
    QVERIFY(store.hasReading(QStringLiteral("co")));

    ChannelConfig *co = config.channelById(QStringLiteral("co"));
    co->sourceId = QStringLiteral("nuoc-thai");
    co->stationCode = QStringLiteral("NUONT1");
    store.setConfig(config);
    QVERIFY(!store.hasReading(QStringLiteral("co")));

    const ChannelState *state = findState(store.evaluate(now), QStringLiteral("co"));
    QVERIFY(state);
    QCOMPARE(state->display, DisplayState::NoData);
    QVERIFY(!state->hasReading);

    LatestReadingStore changedFolder;
    config = defaultAppConfig();
    changedFolder.setConfig(config);
    QCOMPARE(changedFolder.apply(coReading(10.0, now.addSecs(-30)), now).code,
             ApplyOutcome::Code::Accepted);
    config.sources[0].folder = QDir::tempPath();
    changedFolder.setConfig(config);
    QVERIFY(!changedFolder.hasReading(QStringLiteral("co")));
}

void TestReadingStore::loadPersistedUsesCurrentClock()
{
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));

    LatestReadingStore staleStore;
    staleStore.setConfig(defaultAppConfig());
    staleStore.loadPersisted({coReading(10.0, now.addSecs(-1000))}, now);
    QVERIFY(staleStore.hasReading(QStringLiteral("co")));
    const ChannelState *stale = findState(staleStore.evaluate(now), QStringLiteral("co"));
    QVERIFY(stale);
    QCOMPARE(stale->freshness, Freshness::Stale);
    QCOMPARE(stale->display, DisplayState::Stale);

    LatestReadingStore futureStore;
    futureStore.setConfig(defaultAppConfig());
    futureStore.loadPersisted({coReading(10.0, now.addSecs(1000))}, now);
    QVERIFY(!futureStore.hasReading(QStringLiteral("co")));
}

void TestReadingStore::restoredConflictAndWrongStation()
{
    const QDateTime now = utc(QStringLiteral("2026-01-01T05:10:00Z"));
    Reading conflict = coReading(10.0, now.addSecs(-30));
    conflict.conflict = true;
    LatestReadingStore restored;
    restored.setConfig(configWithCoThreshold(true, false, 0.0, true, 100.0));
    restored.loadPersisted({conflict}, now);
    const QVector<ChannelState> states = restored.evaluate(now);
    const ChannelState *co = findState(states, QStringLiteral("co"));
    QVERIFY(co);
    QCOMPARE(co->display, DisplayState::Conflict);
    QCOMPARE(co->threshold, ThresholdResult::NotEvaluated);

    conflict.stationCode = QStringLiteral("OTHER");
    LatestReadingStore wrongStation;
    wrongStation.setConfig(defaultAppConfig());
    wrongStation.loadPersisted({conflict}, now);
    QVERIFY(!wrongStation.hasReading(QStringLiteral("co")));
}

void TestReadingStore::formattingHelpers()
{
    QCOMPARE(formatReadingValue(1.234, 2), QStringLiteral("1.23"));
    QCOMPARE(formatReadingValue(1.0, 0), QStringLiteral("1"));
    QCOMPARE(formatReadingValue(1.0, -1), QStringLiteral("1"));
    QCOMPARE(formatReadingValue(1.0, 9), QStringLiteral("1.000000"));
    QCOMPARE(formatReadingValue(std::numeric_limits<double>::quiet_NaN(), 1), QStringLiteral("—"));

    QCOMPARE(formatAge(0), QStringLiteral("0 giây"));
    QCOMPARE(formatAge(59), QStringLiteral("59 giây"));
    QCOMPARE(formatAge(60), QStringLiteral("1 phút"));
    QCOMPARE(formatAge(3600), QStringLiteral("1 giờ 0 phút"));
    QCOMPARE(formatAge(86400), QStringLiteral("1 ngày"));
    QCOMPARE(formatAge(-1), QStringLiteral("tương lai"));

    Threshold threshold;
    QCOMPARE(formatThreshold(threshold, 1), QString());
    threshold.enabled = true;
    threshold.hasHigh = true;
    threshold.high = 10.0;
    QCOMPARE(formatThreshold(threshold, 1), QStringLiteral("≤ 10.0"));
    threshold.hasLow = true;
    threshold.low = 5.0;
    QCOMPARE(formatThreshold(threshold, 1), QStringLiteral("5.0 – 10.0"));
    threshold.hasHigh = false;
    QCOMPARE(formatThreshold(threshold, 1), QStringLiteral("≥ 5.0"));

    QCOMPARE(displayStateKey(DisplayState::ExceedsLimit), QStringLiteral("exceedsLimit"));
    QCOMPARE(displayStateText(DisplayState::ExceedsLimit), QStringLiteral("VƯỢT NGƯỠNG"));
    QCOMPARE(displayStateSeverity(DisplayState::ExceedsLimit), QStringLiteral("alarm"));
    QCOMPARE(displayStateSeverity(DisplayState::NoData), QStringLiteral("missing"));
}

QTEST_GUILESS_MAIN(TestReadingStore)
#include "tst_reading_store.moc"
