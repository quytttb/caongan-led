#include "domain/ConfigCodec.h"
#include "domain/ParameterCatalog.h"
#include "domain/ThresholdEvaluator.h"
#include "storage/ConfigStore.h"
#include "utils/AppPaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

using namespace caongan;

namespace {

bool hasIssueField(const QList<ConfigIssue> &issues, const QString &field)
{
    for (const ConfigIssue &issue : issues) {
        if (issue.field == field)
            return true;
    }
    return false;
}

bool hasIssueText(const QList<ConfigIssue> &issues, const QString &needle)
{
    for (const ConfigIssue &issue : issues) {
        if (issue.message.contains(needle, Qt::CaseInsensitive)
            || issue.field.contains(needle, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

QByteArray readAll(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}

} // namespace

class TestConfig : public QObject
{
    Q_OBJECT

private slots:
    void defaultsAreValidAndOrdered();
    void sampleMatchesDefaults();
    void relativeSourceRejected();
    void jsonRoundTrip();
    void schemaVersionRejected();
    void invalidGroupRejected();
    void missingChannelRejected();
    void unknownChannelRejected();
    void stationMismatchRejected();
    void duplicateAliasRejected();
    void thresholdValidation();
    void fixedThresholds_data();
    void fixedThresholds();
    void oldDisplayConfigMigrates();
    void incompatibleThresholdUnitRejected();
    void displaySizeValidation();
    void duplicateSourceRejected();
    void decimalsValidation();
    void storeCreatesDefault();
    void storeRecoversFromBackup();
    void storeRejectsInvalidSave();
    void storePreservesMainWhenBackupFails();
    void storeFailsWhenMainAndBackupBroken();
    void appPathsHonorEnvironment();
};

void TestConfig::defaultsAreValidAndOrdered()
{
    const AppConfig config = defaultAppConfig();
    const QList<ConfigIssue> issues = validateConfig(config);
    QVERIFY2(issues.isEmpty(), qPrintable(configIssuesToStrings(issues).join("\n")));

    QCOMPARE(config.sources.size(), 2);
    QCOMPARE(config.sources.at(0).id, QStringLiteral("khi-thai"));
    QCOMPARE(config.sources.at(0).stationCode, QStringLiteral("KHITO1"));
    QCOMPARE(config.sources.at(1).id, QStringLiteral("nuoc-thai"));
    QCOMPARE(config.sources.at(1).stationCode, QStringLiteral("NUONT1"));
    QVERIFY(config.sources.at(0).folder.isEmpty());
    QVERIFY(config.sources.at(1).folder.isEmpty());

    const QStringList expected = {
        QStringLiteral("o2"), QStringLiteral("co"), QStringLiteral("nox"),
        QStringLiteral("so2"), QStringLiteral("pm"), QStringLiteral("temp_flue"),
        QStringLiteral("flow_flue"), QStringLiteral("pressure_flue"),
        QStringLiteral("ph"), QStringLiteral("tss"), QStringLiteral("cod"),
        QStringLiteral("nh4"),
    };
    QStringList actual;
    for (const ChannelConfig &channel : config.channels)
        actual.append(channel.id);
    QCOMPARE(actual, expected);

    QCOMPARE(config.channels.at(0).group, ChannelGroup::Exhaust);
    QCOMPARE(config.channels.at(8).group, ChannelGroup::Wastewater);
    QCOMPARE(config.channels.at(4).unit, QStringLiteral("mg/Nm³"));
    QVERIFY(config.channels.at(0).threshold.enabled);
    QVERIFY(!unitAccepted(config.channels.at(4), QStringLiteral("mg/m³")));
}

void TestConfig::sampleMatchesDefaults()
{
    const QString path = QFINDTESTDATA("../sample-data/caongan-led.example.json");
    QVERIFY(!path.isEmpty());
    ConfigStore store(path);
    AppConfig config;
    QStringList messages;
    QCOMPARE(store.load(config, &messages), ConfigStore::LoadStatus::Loaded);
    QCOMPARE(configToJson(config), configToJson(defaultAppConfig()));
}

void TestConfig::relativeSourceRejected()
{
    AppConfig config = defaultAppConfig();
    config.sources[0].folder = QStringLiteral("relative/input");
    QVERIFY(hasIssueField(validateConfig(config), QStringLiteral("sources.khi-thai.folder")));
}

void TestConfig::jsonRoundTrip()
{
    AppConfig config = defaultAppConfig();
    config.branding.companyName = QStringLiteral("Công ty thử nghiệm");
    config.display.targetScreenName = QStringLiteral("LED-1");
    config.display.fullScreen = false;
    config.sources[0].folder = QStringLiteral("/tmp/khong-gui");

    const QJsonObject original = configToJson(config);
    AppConfig parsed;
    QStringList errors;
    QVERIFY2(configFromJson(original, parsed, &errors),
             qPrintable(errors.join("\n")));
    QCOMPARE(configToJson(parsed), original);
    QCOMPARE(parsed.display.targetScreenName, QStringLiteral("LED-1"));
    QVERIFY(!parsed.display.fullScreen);
    // Ngưỡng và dải đo cố định: đọc lại luôn ép về danh mục chuẩn.
    ChannelConfig *parsedCo = parsed.channelById(QStringLiteral("co"));
    QVERIFY(parsedCo);
    QVERIFY(parsedCo->threshold.enabled);
    QCOMPARE(parsedCo->threshold.high, 1500.0);
    QVERIFY(!parsedCo->rangeText.isEmpty());
}

void TestConfig::schemaVersionRejected()
{
    QJsonObject object = configToJson(defaultAppConfig());
    object.insert(QStringLiteral("schemaVersion"), 2);
    AppConfig parsed;
    QStringList errors;
    QVERIFY(!configFromJson(object, parsed, &errors));
    QVERIFY(errors.join(QStringLiteral(";")).contains(QStringLiteral("schemaVersion")));
}

void TestConfig::invalidGroupRejected()
{
    QJsonObject object = configToJson(defaultAppConfig());
    QJsonArray channels = object.value(QStringLiteral("channels")).toArray();
    QJsonObject first = channels.at(0).toObject();
    first.insert(QStringLiteral("group"), QStringLiteral("khong-hop-le"));
    channels.replace(0, first);
    object.insert(QStringLiteral("channels"), channels);

    AppConfig parsed;
    QStringList errors;
    QVERIFY(!configFromJson(object, parsed, &errors));
    QVERIFY(!errors.isEmpty());
}

void TestConfig::missingChannelRejected()
{
    AppConfig config = defaultAppConfig();
    for (int i = 0; i < config.channels.size(); ++i) {
        if (config.channels.at(i).id == QLatin1String("cod")) {
            config.channels.removeAt(i);
            break;
        }
    }
    const QList<ConfigIssue> issues = validateConfig(config);
    QVERIFY(hasIssueText(issues, QStringLiteral("Thiếu kênh bắt buộc")));
}

void TestConfig::unknownChannelRejected()
{
    AppConfig config = defaultAppConfig();
    ChannelConfig extra;
    extra.id = QStringLiteral("extra");
    extra.group = ChannelGroup::Exhaust;
    extra.symbol = QStringLiteral("EXTRA");
    extra.displayName = QStringLiteral("Kênh lạ");
    extra.unit = QStringLiteral("mg/Nm³");
    extra.acceptedUnits = {QStringLiteral("mg/Nm³")};
    extra.aliases = {QStringLiteral("EXTRA")};
    extra.sourceId = QStringLiteral("khi-thai");
    extra.stationCode = QStringLiteral("KHITO1");
    config.channels.append(extra);

    const QList<ConfigIssue> issues = validateConfig(config);
    QVERIFY(hasIssueText(issues, QStringLiteral("12 vị trí")));
}

void TestConfig::stationMismatchRejected()
{
    AppConfig config = defaultAppConfig();
    config.channelById(QStringLiteral("o2"))->stationCode = QStringLiteral("KHITO2");
    const QList<ConfigIssue> issues = validateConfig(config);
    QVERIFY(hasIssueField(issues, QStringLiteral("channels.o2.stationCode")));
}

void TestConfig::duplicateAliasRejected()
{
    AppConfig config = defaultAppConfig();
    config.channelById(QStringLiteral("co"))->aliases.append(QStringLiteral("O2"));
    const QList<ConfigIssue> issues = validateConfig(config);
    QVERIFY(hasIssueText(issues, QStringLiteral("Alias O2 trùng")));
}

void TestConfig::thresholdValidation()
{
    // Ngưỡng đã cố định theo nhà máy, không còn validate bật/tắt hay nhập cận.
    const AppConfig config = defaultAppConfig();
    const ChannelConfig *co = config.channelById(QStringLiteral("co"));
    QVERIFY(co);
    QVERIFY(co->threshold.enabled);
    QVERIFY(co->threshold.hasLow);
    QVERIFY(co->threshold.hasHigh);
    QCOMPARE(co->threshold.low, 0.0);
    QCOMPARE(co->threshold.high, 1500.0);
    QCOMPARE(co->rangeText, QStringLiteral("0 - 1.500 mg/Nm³"));
    const ChannelConfig *ph = config.channelById(QStringLiteral("ph"));
    QVERIFY(ph);
    QCOMPARE(ph->threshold.low, 6.0);
    QCOMPARE(ph->threshold.high, 9.0);
    QVERIFY(validateConfig(config).isEmpty());
}

void TestConfig::fixedThresholds_data()
{
    QTest::addColumn<QString>("id");
    QTest::addColumn<double>("low");
    QTest::addColumn<double>("high");
    QTest::addColumn<QString>("range");
    QTest::newRow("O2") << QStringLiteral("o2") << 0.0 << 25.0 << QStringLiteral("0 - 25 %V");
    QTest::newRow("CO") << QStringLiteral("co") << 0.0 << 1500.0 << QStringLiteral("0 - 1.500 mg/Nm³");
    QTest::newRow("NOx") << QStringLiteral("nox") << 0.0 << 1500.0 << QStringLiteral("0 - 1.500 mg/Nm³");
    QTest::newRow("SO2") << QStringLiteral("so2") << 0.0 << 1000.0 << QStringLiteral("0 - 1.000 mg/Nm³");
    QTest::newRow("PM") << QStringLiteral("pm") << 0.0 << 1000.0 << QStringLiteral("0 - 1.000 mg/Nm³");
    QTest::newRow("Temp") << QStringLiteral("temp_flue") << 0.0 << 500.0 << QStringLiteral("0 - 500 °C");
    QTest::newRow("Flow") << QStringLiteral("flow_flue") << 0.0 << 1000000.0 << QStringLiteral("0 - 1.000.000 m³/h");
    QTest::newRow("P") << QStringLiteral("pressure_flue") << 0.0 << 160.0 << QStringLiteral("0 - 160 kPa");
    QTest::newRow("pH") << QStringLiteral("ph") << 6.0 << 9.0 << QStringLiteral("0 - 14");
    QTest::newRow("TSS") << QStringLiteral("tss") << 0.0 << 55.0 << QStringLiteral("0 - 55 mg/L");
    QTest::newRow("COD") << QStringLiteral("cod") << 0.0 << 82.5 << QStringLiteral("0 - 82,5 mg/L");
    QTest::newRow("NH4") << QStringLiteral("nh4") << 0.0 << 5.5 << QStringLiteral("0 - 5,5 mg/L");
}

void TestConfig::fixedThresholds()
{
    QFETCH(QString, id);
    QFETCH(double, low);
    QFETCH(double, high);
    QFETCH(QString, range);
    const AppConfig config = defaultAppConfig();
    const ChannelConfig *channel = config.channelById(id);
    QVERIFY(channel);
    QVERIFY(channel->threshold.configured());
    QCOMPARE(channel->rangeText, range);
    QCOMPARE(channel->threshold.low, low);
    QCOMPARE(channel->threshold.high, high);
    Reading reading;
    reading.deviceStatus = DeviceStatus::Measuring;
    reading.unit = channel->unit;
    auto evaluate = [&](double value) {
        reading.value = value;
        return evaluateThreshold(*channel, reading, Validity::Valid, Freshness::Fresh, false);
    };
    QCOMPARE(evaluate(low), ThresholdResult::WithinConfiguredLimit);
    QCOMPARE(evaluate(high), ThresholdResult::WithinConfiguredLimit);
    QCOMPARE(evaluate(low - 0.01), ThresholdResult::ExceedsConfiguredLimit);
    QCOMPARE(evaluate(high + 0.01), ThresholdResult::ExceedsConfiguredLimit);
}

void TestConfig::oldDisplayConfigMigrates()
{
    for (const int oldHeight : {1200, 1152, 960}) {
        QJsonObject json = configToJson(defaultAppConfig());
        QJsonObject display = json.value(QStringLiteral("display")).toObject();
        display.insert(QStringLiteral("designWidth"), 1920);
        display.insert(QStringLiteral("designHeight"), oldHeight);
        display.insert(QStringLiteral("targetScreenName"), QStringLiteral("LED-1"));
        json.insert(QStringLiteral("display"), display);
        QJsonArray channels = json.value(QStringLiteral("channels")).toArray();
        QJsonObject co = channels.at(1).toObject();
        co.insert(QStringLiteral("threshold"), QJsonObject{{QStringLiteral("enabled"), false},
                                                            {QStringLiteral("high"), 3}});
        channels.replace(1, co);
        json.insert(QStringLiteral("channels"), channels);
        AppConfig parsed;
        QVERIFY(configFromJson(json, parsed));
        QCOMPARE(parsed.display.designWidth, 1600);
        QCOMPARE(parsed.display.designHeight, 960);
        QCOMPARE(parsed.display.targetScreenName, QStringLiteral("LED-1"));
        QVERIFY(parsed.channelById(QStringLiteral("co"))->threshold.enabled);
        QCOMPARE(parsed.channelById(QStringLiteral("co"))->threshold.high, 1500.0);
        QVERIFY(validateConfig(parsed).isEmpty());
    }
}

void TestConfig::incompatibleThresholdUnitRejected()
{
    AppConfig config = defaultAppConfig();
    ChannelConfig *pm = config.channelById(QStringLiteral("pm"));
    QVERIFY(pm);
    pm->acceptedUnits.append(QStringLiteral("mg/m³"));
    QVERIFY(hasIssueField(validateConfig(config), QStringLiteral("channels.pm.acceptedUnits")));
    pm->unit = QStringLiteral("mg/m³");
    QVERIFY(hasIssueField(validateConfig(config), QStringLiteral("channels.pm.unit")));
}

void TestConfig::displaySizeValidation()
{
    AppConfig config = defaultAppConfig();
    config.display.designWidth = 1280;
    config.display.designHeight = 720;
    const QList<ConfigIssue> issues = validateConfig(config);
    QVERIFY(hasIssueField(issues, QStringLiteral("display.designWidth/Height")));
}

void TestConfig::duplicateSourceRejected()
{
    AppConfig config = defaultAppConfig();
    config.sources[1].id = config.sources[0].id;
    const QList<ConfigIssue> issues = validateConfig(config);
    QVERIFY(hasIssueText(issues, QStringLiteral("Nguồn trùng id")));
}

void TestConfig::decimalsValidation()
{
    AppConfig config = defaultAppConfig();
    config.channelById(QStringLiteral("o2"))->decimals = 9;
    QVERIFY(hasIssueField(validateConfig(config), QStringLiteral("channels.o2.decimals")));
}

void TestConfig::storeCreatesDefault()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("config/caongan-led.json"));
    ConfigStore store(path);

    QStringList messages;
    AppConfig config;
    QCOMPARE(store.load(config, &messages), ConfigStore::LoadStatus::CreatedDefault);
    QVERIFY(store.exists());
    QVERIFY(QFileInfo::exists(path));
    QCOMPARE(config.schemaVersion, 1);
    QVERIFY(messages.isEmpty());

    AppConfig loaded;
    QCOMPARE(store.load(loaded), ConfigStore::LoadStatus::Loaded);
    QCOMPARE(configToJson(loaded), configToJson(config));
}

void TestConfig::storeRecoversFromBackup()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("caongan-led.json"));
    ConfigStore store(path);

    AppConfig config;
    QCOMPARE(store.load(config), ConfigStore::LoadStatus::CreatedDefault);

    config.branding.companyName = QStringLiteral("Lần 1");
    QString error;
    QVERIFY2(store.save(config, &error), qPrintable(error));
    QVERIFY(QFileInfo::exists(store.backupPath()));

    config.branding.companyName = QStringLiteral("Lần 2");
    QVERIFY2(store.save(config, &error), qPrintable(error));

    QFile broken(path);
    QVERIFY(broken.open(QIODevice::WriteOnly | QIODevice::Truncate));
    broken.write("{ not valid json");
    broken.close();

    QStringList messages;
    AppConfig loaded;
    QCOMPARE(store.load(loaded, &messages), ConfigStore::LoadStatus::RecoveredFromBackup);
    QCOMPARE(loaded.branding.companyName, QStringLiteral("Lần 1"));
    QVERIFY(!messages.isEmpty());
}

void TestConfig::storeRejectsInvalidSave()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("caongan-led.json"));
    ConfigStore store(path);

    AppConfig config;
    QCOMPARE(store.load(config), ConfigStore::LoadStatus::CreatedDefault);
    const QByteArray before = readAll(path);
    QVERIFY(!before.isEmpty());

    config.display.designWidth = 1280;
    QString error;
    QVERIFY(!store.save(config, &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(readAll(path), before);
}

void TestConfig::storePreservesMainWhenBackupFails()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("caongan-led.json"));
    ConfigStore store(path);
    AppConfig config;
    QCOMPARE(store.load(config), ConfigStore::LoadStatus::CreatedDefault);
    const QByteArray before = readAll(path);
    // Một directory tại đường dẫn backup khiến backup commit thất bại trên
    // cả Windows và Linux, không cần thay đổi quyền hoặc giả định chạy non-root.
    QVERIFY(QDir().mkpath(store.backupPath()));
    const QString marker = store.backupPath() + QStringLiteral("/marker");
    QFile file(marker);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write("keep"), qint64(4));
    file.close();

    config.branding.companyName = QStringLiteral("Khong duoc luu");
    QString error;
    QVERIFY(!store.save(config, &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(readAll(path), before);
    QCOMPARE(readAll(marker), QByteArray("keep"));
}

void TestConfig::storeFailsWhenMainAndBackupBroken()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("caongan-led.json"));
    ConfigStore store(path);

    AppConfig config;
    QCOMPARE(store.load(config), ConfigStore::LoadStatus::CreatedDefault);

    QFile broken(path);
    QVERIFY(broken.open(QIODevice::WriteOnly | QIODevice::Truncate));
    broken.write("broken");
    broken.close();

    QFile backup(store.backupPath());
    QVERIFY(backup.open(QIODevice::WriteOnly | QIODevice::Truncate));
    backup.write("also broken");
    backup.close();

    QStringList messages;
    AppConfig loaded;
    QCOMPARE(store.load(loaded, &messages), ConfigStore::LoadStatus::Failed);
    QCOMPARE(loaded.schemaVersion, 1);
    QVERIFY(!messages.isEmpty());
}

void TestConfig::appPathsHonorEnvironment()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QByteArray oldBase = qgetenv("CAONGAN_LED_BASE_DIR");

    qputenv("CAONGAN_LED_BASE_DIR", dir.path().toUtf8());
    QCOMPARE(AppPaths::baseDir(), QDir::cleanPath(dir.path()));
    QCOMPARE(AppPaths::dataDir(), QDir::cleanPath(dir.path() + QStringLiteral("/data")));
    QCOMPARE(AppPaths::configDir(), QDir::cleanPath(dir.path() + QStringLiteral("/config")));
    QCOMPARE(AppPaths::logDir(), QDir::cleanPath(dir.path() + QStringLiteral("/logs")));
    QCOMPARE(AppPaths::configFilePath(),
             QDir::cleanPath(dir.path() + QStringLiteral("/config/caongan-led.json")));
    QCOMPARE(AppPaths::databasePath(),
             QDir::cleanPath(dir.path() + QStringLiteral("/data/snapshot.db")));

    AppPaths::ensureDirectories();
    QVERIFY(QFileInfo(AppPaths::dataDir()).isDir());
    QVERIFY(QFileInfo(AppPaths::configDir()).isDir());
    QVERIFY(QFileInfo(AppPaths::logDir()).isDir());

    if (oldBase.isEmpty())
        qunsetenv("CAONGAN_LED_BASE_DIR");
    else
        qputenv("CAONGAN_LED_BASE_DIR", oldBase);
}

QTEST_GUILESS_MAIN(TestConfig)
#include "tst_config.moc"
