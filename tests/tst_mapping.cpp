#include "domain/ParameterCatalog.h"
#include "domain/ParameterMapper.h"
#include "domain/Tt10Parser.h"

#include <QTest>

using namespace caongan;

namespace {

SourceConfig sourceFor(const QString &id, const QString &name, const QString &station)
{
    SourceConfig source;
    source.id = id;
    source.name = name;
    source.stationCode = station;
    source.allowLegacyPrefix = true;
    return source;
}

QString fileNameFor(const QString &station)
{
    return QStringLiteral("TNg_NDCN_%1_20260101120000.txt").arg(station);
}

const Reading *findReading(const QVector<Reading> &readings, const QString &channelId)
{
    for (const Reading &reading : readings) {
        if (reading.channelId == channelId)
            return &reading;
    }
    return nullptr;
}

bool hasDiagnostic(const QVector<DiagnosticEntry> &diagnostics, const QString &code)
{
    for (const DiagnosticEntry &entry : diagnostics) {
        if (entry.code == code)
            return true;
    }
    return false;
}

} // namespace

class TestMapping : public QObject
{
    Q_OBJECT

private slots:
    void mapsByExplicitAliasOnly();
    void unitAliasesAndMismatch();
    void missingValueStillMaps();
    void waterSourceExcludesNh4N();
    void noFuzzyMatchBetweenNitrogenSpecies();
    void unmappedSymbolsReportedOnce();
};

void TestMapping::mapsByExplicitAliasOnly()
{
    const QByteArray content = QByteArrayLiteral(
        "o2\t12.3\t%\t20260101120000\t00\n"
        "NOX\t180.4\tmg/Nm³\t20260101120000\t00\n"
        "NO2\t20.0\tmg/Nm³\t20260101120000\t00\n"
        "SO₂\t95.2\tmg/Nm³\t20260101120000\t00\n"
        "PM\t18.7\tmg/Nm³\t20260101120000\t00\n"
        "Temp\t132.5\t°C\t20260101120000\t00\n"
        "Flow\t185000\tm³/h\t20260101120000\t00\n"
        "P\t-1.25\tkPa\t20260101120000\t00\n");

    const SourceConfig source = sourceFor(QStringLiteral("khi-thai"), QStringLiteral("Khí thải"),
                                          QStringLiteral("KHITO1"));
    const ParseResult parsed = parseTt10Content(fileNameFor(QStringLiteral("KHITO1")), content,
                                                source, {});
    QVERIFY2(parsed.accepted, qPrintable(parsed.errorMessage));

    const MapResult mapped = ParameterMapper::map(parsed, source, defaultAppConfig(),
                                                  QStringLiteral("/tmp/a.txt"),
                                                  QStringLiteral("hash"), {});
    QCOMPARE(mapped.readings.size(), 7);
    QVERIFY(findReading(mapped.readings, QStringLiteral("o2")));
    QVERIFY(findReading(mapped.readings, QStringLiteral("nox")));
    QVERIFY(findReading(mapped.readings, QStringLiteral("so2")));
    QVERIFY(findReading(mapped.readings, QStringLiteral("pm")));
    QVERIFY(findReading(mapped.readings, QStringLiteral("temp_flue")));
    QVERIFY(findReading(mapped.readings, QStringLiteral("flow_flue")));
    QVERIFY(findReading(mapped.readings, QStringLiteral("pressure_flue")));
    QVERIFY(!findReading(mapped.readings, QStringLiteral("nh4")));
    QVERIFY(hasDiagnostic(mapped.diagnostics, QStringLiteral("unmapped_symbol")));
}

void TestMapping::unitAliasesAndMismatch()
{
    const QByteArray content = QByteArrayLiteral(
        "CO\t45.6\tmg/m3\t20260101120000\t00\n"
        "Temp\t130\toC\t20260101120000\t00\n");

    const SourceConfig source = sourceFor(QStringLiteral("khi-thai"), QStringLiteral("Khí thải"),
                                          QStringLiteral("KHITO1"));
    const ParseResult parsed = parseTt10Content(fileNameFor(QStringLiteral("KHITO1")), content,
                                                source, {});
    QVERIFY(parsed.accepted);

    const MapResult mapped = ParameterMapper::map(parsed, source, defaultAppConfig(),
                                                  QStringLiteral("/tmp/a.txt"),
                                                  QStringLiteral("hash"), {});
    QCOMPARE(mapped.readings.size(), 2);

    const Reading *co = findReading(mapped.readings, QStringLiteral("co"));
    QVERIFY(co);
    // mg/m3 không phải mg/Nm3: giữ nguyên số nhưng đánh dấu sai đơn vị.
    QCOMPARE(co->validity, Validity::UnitMismatch);
    QCOMPARE(co->unit, QStringLiteral("mg/m3"));
    QVERIFY(hasDiagnostic(mapped.diagnostics, QStringLiteral("unit_mismatch")));

    const Reading *temp = findReading(mapped.readings, QStringLiteral("temp_flue"));
    QVERIFY(temp);
    QCOMPARE(temp->validity, Validity::Valid);
    QCOMPARE(temp->unit, QStringLiteral("oC"));
}

void TestMapping::missingValueStillMaps()
{
    const QByteArray content = QByteArrayLiteral("O2\t---\t%\t20260101120000\t00\n");
    const SourceConfig source = sourceFor(QStringLiteral("khi-thai"), QStringLiteral("Khí thải"),
                                          QStringLiteral("KHITO1"));
    const ParseResult parsed = parseTt10Content(fileNameFor(QStringLiteral("KHITO1")), content,
                                                source, {});
    QVERIFY(parsed.accepted);

    const MapResult mapped = ParameterMapper::map(parsed, source, defaultAppConfig(),
                                                  QStringLiteral("/tmp/a.txt"),
                                                  QStringLiteral("hash"), {});
    QCOMPARE(mapped.readings.size(), 1);
    const Reading *o2 = findReading(mapped.readings, QStringLiteral("o2"));
    QVERIFY(o2);
    QCOMPARE(o2->validity, Validity::Missing);
    QVERIFY(!o2->value.has_value());
    QVERIFY(hasDiagnostic(mapped.diagnostics, QStringLiteral("missing_value")));
}

void TestMapping::waterSourceExcludesNh4N()
{
    const QByteArray content = QByteArrayLiteral(
        "pH\t7.2\t-\t20260101120000\t00\n"
        "NH4+\t5.4\tmg/L\t20260101120000\t00\n"
        "NH4-N\t4.2\tmg/L\t20260101120000\t00\n");

    const SourceConfig source = sourceFor(QStringLiteral("nuoc-thai"), QStringLiteral("Nước thải"),
                                          QStringLiteral("NUONT1"));
    const ParseResult parsed = parseTt10Content(fileNameFor(QStringLiteral("NUONT1")), content,
                                                source, {});
    QVERIFY(parsed.accepted);

    const MapResult mapped = ParameterMapper::map(parsed, source, defaultAppConfig(),
                                                  QStringLiteral("/tmp/a.txt"),
                                                  QStringLiteral("hash"), {});
    QCOMPARE(mapped.readings.size(), 2);
    QVERIFY(findReading(mapped.readings, QStringLiteral("ph")));
    const Reading *nh4 = findReading(mapped.readings, QStringLiteral("nh4"));
    QVERIFY(nh4);
    QCOMPARE(nh4->rawSymbol, QStringLiteral("NH4+"));
    QVERIFY(!findReading(mapped.readings, QStringLiteral("nh4_n")));
}

void TestMapping::noFuzzyMatchBetweenNitrogenSpecies()
{
    AppConfig config = defaultAppConfig();
    ChannelConfig *nox = config.channelById(QStringLiteral("nox"));
    QVERIFY(nox);
    QCOMPARE(nox->aliases, QStringList({QStringLiteral("NOx"), QStringLiteral("NOX"),
                                        QStringLiteral("NOₓ")}));

    const QByteArray content = QByteArrayLiteral(
        "NO\t10\tmg/Nm³\t20260101120000\t00\n"
        "NO2\t20\tmg/Nm³\t20260101120000\t00\n"
        "NOx\t30\tmg/Nm³\t20260101120000\t00\n");
    const SourceConfig source = sourceFor(QStringLiteral("khi-thai"), QStringLiteral("Khí thải"),
                                          QStringLiteral("KHITO1"));
    const ParseResult parsed = parseTt10Content(fileNameFor(QStringLiteral("KHITO1")), content,
                                                source, {});
    QVERIFY(parsed.accepted);

    const MapResult mapped = ParameterMapper::map(parsed, source, config,
                                                  QStringLiteral("/tmp/a.txt"),
                                                  QStringLiteral("hash"), {});
    QCOMPARE(mapped.readings.size(), 1);
    QCOMPARE(mapped.readings.constFirst().channelId, QStringLiteral("nox"));
    QCOMPARE(mapped.readings.constFirst().rawSymbol, QStringLiteral("NOx"));
    QVERIFY(hasDiagnostic(mapped.diagnostics, QStringLiteral("unmapped_symbol")));
}

void TestMapping::unmappedSymbolsReportedOnce()
{
    const QByteArray content = QByteArrayLiteral(
        "ABC\t1\tx\t20260101120000\t00\n"
        "ABC\t2\tx\t20260101120000\t00\n"
        "O2\t12.3\t%\t20260101120000\t00\n");
    const SourceConfig source = sourceFor(QStringLiteral("khi-thai"), QStringLiteral("Khí thải"),
                                          QStringLiteral("KHITO1"));
    const ParseResult parsed = parseTt10Content(fileNameFor(QStringLiteral("KHITO1")), content,
                                                source, {});
    QVERIFY(parsed.accepted);

    const MapResult mapped = ParameterMapper::map(parsed, source, defaultAppConfig(),
                                                  QStringLiteral("/tmp/a.txt"),
                                                  QStringLiteral("hash"), {});
    QCOMPARE(mapped.readings.size(), 1);
    int unmappedCount = 0;
    for (const DiagnosticEntry &entry : mapped.diagnostics) {
        if (entry.code == QLatin1String("unmapped_symbol"))
            ++unmappedCount;
    }
    QCOMPARE(unmappedCount, 1);
}

QTEST_GUILESS_MAIN(TestMapping)
#include "tst_mapping.moc"
