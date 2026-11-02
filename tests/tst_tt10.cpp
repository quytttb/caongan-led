#include "domain/Tt10Parser.h"
#include "domain/Tt10Time.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTest>

using namespace caongan;

namespace {

SourceConfig flueSource()
{
    SourceConfig source;
    source.id = QStringLiteral("khi-thai");
    source.name = QStringLiteral("Khí thải");
    source.stationCode = QStringLiteral("KHITO1");
    source.allowLegacyPrefix = false;
    source.staleAfterSeconds = 900;
    source.futureToleranceSeconds = 120;
    return source;
}

QString standardFileName()
{
    return QStringLiteral("TNg_NDCN_KHITO1_20260101120000.txt");
}

QByteArray exhaustContent()
{
    return QByteArrayLiteral(
        "O2\t12.3\t%\t20260101120000\t00\n"
        "CO\t45.6\tmg/Nm³\t20260101120000\t00\r\n");
}

} // namespace

class TestTt10 : public QObject
{
    Q_OBJECT

private slots:
    void timestampUtc7();
    void timestampRejectsInvalid();
    void timestampLeapYear();
    void timestampFormatting();

    void strictNumber_data();
    void strictNumber();

    void parsesBomCrlfAndUtf8();
    void readsSampleFixture();
    void fourFieldsDoesNotMeanMeasuring();
    void unknownStatusIsNotMeasuring();
    void placeholderIsMissingNotZero();
    void invalidNumberKeepsRecordInvalid();
    void zeroIsValid();
    void fileNameTimestampMismatchFlag();
    void structuralErrorRejectsWholeFile();
    void groupedFiveColumnOnlyWhenEnabled();
    void emptyFileRejected();
    void limitsAreEnforced();
    void fileNameContract();
    void stationMismatchRejected();
};

void TestTt10::timestampUtc7()
{
    QString error;
    const QDateTime utc = tt10::parseTimestamp(QStringLiteral("20260101120000"), &error);
    QVERIFY2(utc.isValid(), qPrintable(error));
    QCOMPARE(utc.toUTC().toString(Qt::ISODate), QStringLiteral("2026-01-01T05:00:00Z"));
    QCOMPARE(tt10::formatTimestamp(utc), QStringLiteral("20260101120000"));
    QCOMPARE(tt10::formatLocalDisplay(utc, QStringLiteral("HH:mm:ss dd/MM/yyyy")),
             QStringLiteral("12:00:00 01/01/2026"));
    QCOMPARE(utc.timeSpec(), Qt::UTC);
}

void TestTt10::timestampRejectsInvalid()
{
    const QStringList bad = {
        QString(),
        QStringLiteral("2026010112000"),      // 13 digits
        QStringLiteral("202601011200000"),    // 15 digits
        QStringLiteral("2026010112000a"),
        QStringLiteral("20261301120000"),     // month 13
        QStringLiteral("20260100120000"),     // day 0
        QStringLiteral("20260132240000"),     // hour 24
        QStringLiteral("20260101246000"),     // minute 60
        QStringLiteral("20260101126000"),     // second 60
    };
    for (const QString &text : bad) {
        QString error;
        const QDateTime parsed = tt10::parseTimestamp(text, &error);
        QVERIFY2(!parsed.isValid(), qPrintable(text));
        QVERIFY2(!error.isEmpty(), qPrintable(text));
    }
}

void TestTt10::timestampLeapYear()
{
    QString error;
    QVERIFY(tt10::parseTimestamp(QStringLiteral("20240229120000"), &error).isValid());
    QVERIFY2(!tt10::parseTimestamp(QStringLiteral("20260229120000"), &error).isValid(),
             "2026 không phải năm nhuận");
}

void TestTt10::timestampFormatting()
{
    QCOMPARE(tt10::formatTimestamp(QDateTime()), QStringLiteral("----------------"));
    QCOMPARE(tt10::formatLocalDisplay(QDateTime()), QStringLiteral("—"));
}

void TestTt10::strictNumber_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("accepted");
    QTest::addColumn<double>("value");

    QTest::newRow("zero") << QStringLiteral("0") << true << 0.0;
    QTest::newRow("integer") << QStringLiteral("42") << true << 42.0;
    QTest::newRow("negative") << QStringLiteral("-12.5") << true << -12.5;
    QTest::newRow("plus") << QStringLiteral("+3.25") << true << 3.25;
    QTest::newRow("leading-dot") << QStringLiteral(".5") << true << 0.5;
    QTest::newRow("trailing-dot") << QStringLiteral("5.") << true << 5.0;
    QTest::newRow("spaces") << QStringLiteral("  7.50  ") << true << 7.5;

    QTest::newRow("comma") << QStringLiteral("1,5") << false << 0.0;
    QTest::newRow("exponent") << QStringLiteral("1e3") << false << 0.0;
    QTest::newRow("nan") << QStringLiteral("NaN") << false << 0.0;
    QTest::newRow("inf") << QStringLiteral("Inf") << false << 0.0;
    QTest::newRow("empty") << QStringLiteral("") << false << 0.0;
    QTest::newRow("placeholder") << QStringLiteral("---") << false << 0.0;
    QTest::newRow("hex") << QStringLiteral("0x10") << false << 0.0;
    QTest::newRow("double-dot") << QStringLiteral("1.2.3") << false << 0.0;
}

void TestTt10::strictNumber()
{
    QFETCH(QString, text);
    QFETCH(bool, accepted);
    QFETCH(double, value);

    double parsed = -1.0;
    QCOMPARE(parseStrictNumber(text, &parsed), accepted);
    if (accepted)
        QCOMPARE(parsed, value);
}

void TestTt10::parsesBomCrlfAndUtf8()
{
    QByteArray content;
    content.append("\xEF\xBB\xBF", 3);
    content.append(exhaustContent());

    const ParseResult result = parseTt10Content(standardFileName(), content, flueSource(),
                                                QDateTime::fromString(QStringLiteral("2026-01-01T05:10:00Z"), Qt::ISODate));
    QVERIFY2(result.accepted, qPrintable(result.errorMessage));
    QCOMPARE(result.records.size(), 2);
    QCOMPARE(result.records.at(0).symbol, QStringLiteral("O2"));
    QCOMPARE(result.records.at(0).value.value_or(-1.0), 12.3);
    QCOMPARE(result.records.at(1).unit, QStringLiteral("mg/Nm³"));
    QCOMPARE(result.records.at(1).deviceStatus, DeviceStatus::Measuring);
    QCOMPARE(result.records.at(1).line, 2);
    QVERIFY(result.issues.isEmpty());
}

void TestTt10::readsSampleFixture()
{
    const QString path = QFINDTESTDATA("fixtures/tt10-sample/TNg_NDCN_KHITO1_20260101120000.txt");
    QVERIFY2(!path.isEmpty(), "Không tìm thấy fixture TT10 mẫu");
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));

    SourceConfig source = flueSource();
    source.allowLegacyPrefix = true;
    const ParseResult result = parseTt10Content(QFileInfo(path).fileName(), file.readAll(), source,
                                                QDateTime::fromString(QStringLiteral("2026-01-01T05:10:00Z"), Qt::ISODate));
    QVERIFY2(result.accepted, qPrintable(result.errorMessage));
    QCOMPARE(result.records.size(), 8);
    QCOMPARE(result.records.at(0).symbol, QStringLiteral("O2"));
    QCOMPARE(result.records.at(7).symbol, QStringLiteral("P"));
}

void TestTt10::fourFieldsDoesNotMeanMeasuring()
{
    const QByteArray content = QByteArrayLiteral("O2\t12.3\t%\t20260101120000\n");
    const ParseResult result = parseTt10Content(standardFileName(), content, flueSource(), {});
    QVERIFY(result.accepted);
    QCOMPARE(result.records.size(), 1);
    const Tt10Record &record = result.records.constFirst();
    QCOMPARE(record.deviceStatus, DeviceStatus::Unknown);
    QCOMPARE(record.issueCode, QStringLiteral("missing_status"));
    QCOMPARE(record.validity, Validity::Valid);
    QCOMPARE(record.value.value_or(-1.0), 12.3);
}

void TestTt10::unknownStatusIsNotMeasuring()
{
    const QByteArray content = QByteArrayLiteral("O2\t12.3\t%\t20260101120000\t03\n");
    const ParseResult result = parseTt10Content(standardFileName(), content, flueSource(), {});
    QVERIFY(result.accepted);
    const Tt10Record &record = result.records.constFirst();
    QCOMPARE(record.deviceStatus, DeviceStatus::Unknown);
    QCOMPARE(record.issueCode, QStringLiteral("unknown_status"));
    QCOMPARE(record.value.value_or(-1.0), 12.3);
    QCOMPARE(deviceStatusToCode(record.deviceStatus), QStringLiteral("--"));
}

void TestTt10::placeholderIsMissingNotZero()
{
    const QByteArray content = QByteArrayLiteral("O2\t---\t%\t20260101120000\t00\n");
    const ParseResult result = parseTt10Content(standardFileName(), content, flueSource(), {});
    QVERIFY(result.accepted);
    const Tt10Record &record = result.records.constFirst();
    QCOMPARE(record.validity, Validity::Missing);
    QVERIFY(!record.value.has_value());
    QCOMPARE(record.issueCode, QStringLiteral("missing_value"));
}

void TestTt10::invalidNumberKeepsRecordInvalid()
{
    for (const QByteArray &raw : {QByteArrayLiteral("1,5"), QByteArrayLiteral("NaN"),
                                  QByteArrayLiteral("1e3"), QByteArrayLiteral("")}) {
        const QByteArray content = "O2\t" + raw + "\t%\t20260101120000\t00\n";
        const ParseResult result = parseTt10Content(standardFileName(), content, flueSource(), {});
        QVERIFY(result.accepted);
        const Tt10Record &record = result.records.constFirst();
        QVERIFY2(!record.value.has_value(), raw.constData());
        if (raw.isEmpty())
            QCOMPARE(record.validity, Validity::Missing);
        else
            QCOMPARE(record.validity, Validity::Invalid);
    }
}

void TestTt10::zeroIsValid()
{
    const QByteArray content = QByteArrayLiteral("CO\t0\tmg/Nm³\t20260101120000\t00\n");
    const ParseResult result = parseTt10Content(standardFileName(), content, flueSource(), {});
    QVERIFY(result.accepted);
    const Tt10Record &record = result.records.constFirst();
    QCOMPARE(record.validity, Validity::Valid);
    QVERIFY(record.value.has_value());
    QCOMPARE(*record.value, 0.0);
}

void TestTt10::fileNameTimestampMismatchFlag()
{
    const QByteArray content = QByteArrayLiteral("O2\t12.3\t%\t20260101120001\t00\n");
    const ParseResult result = parseTt10Content(standardFileName(), content, flueSource(), {});
    QVERIFY(result.accepted);
    QVERIFY(result.fileNameTimestampMismatch);
}

void TestTt10::structuralErrorRejectsWholeFile()
{
    const QByteArray content = QByteArrayLiteral(
        "O2\t12.3\t%\t20260101120000\t00\n"
        "CO\t45.6\n");
    const ParseResult result = parseTt10Content(standardFileName(), content, flueSource(), {});
    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("structure_error"));
    QVERIFY(result.records.isEmpty());
}

void TestTt10::groupedFiveColumnOnlyWhenEnabled()
{
    const QByteArray content = QByteArrayLiteral(
        "O2\t12.3\t%\t20260101120000\t00\tCO\t45.6\tmg/Nm³\t20260101120000\t00\n");

    SourceConfig strict = flueSource();
    ParseResult result = parseTt10Content(standardFileName(), content, strict, {});
    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("structure_error"));

    SourceConfig grouped = flueSource();
    grouped.groupedFiveColumn = true;
    result = parseTt10Content(standardFileName(), content, grouped, {});
    QVERIFY2(result.accepted, qPrintable(result.errorMessage));
    QCOMPARE(result.records.size(), 2);
    QCOMPARE(result.records.at(1).symbol, QStringLiteral("CO"));
}

void TestTt10::emptyFileRejected()
{
    const ParseResult result = parseTt10Content(standardFileName(), QByteArrayLiteral("\n\n"),
                                                flueSource(), {});
    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("empty_file"));
}

void TestTt10::limitsAreEnforced()
{
    ParseLimits limits;
    limits.maxFileBytes = 16;
    ParseResult result = parseTt10Content(standardFileName(), exhaustContent(), flueSource(), {}, limits);
    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("file_too_large"));

    limits = {};
    limits.maxLines = 1;
    result = parseTt10Content(standardFileName(), exhaustContent(), flueSource(), {}, limits);
    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("too_many_lines"));

    limits = {};
    limits.maxFieldChars = 3;
    const QByteArray longField = QByteArrayLiteral("O2\t123456\t%\t20260101120000\t00\n");
    result = parseTt10Content(standardFileName(), longField, flueSource(), {}, limits);
    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("structure_error"));
}

void TestTt10::fileNameContract()
{
    SourceConfig source = flueSource();

    FileNameInfo info = parseFileName(QStringLiteral("TNg_NDCN_KHITO1_20260101120000.txt"), source);
    QVERIFY2(info.valid, qPrintable(info.error));
    QCOMPARE(info.stationCode, QStringLiteral("KHITO1"));
    QCOMPARE(info.timestampText, QStringLiteral("20260101120000"));

    // Copy suffix do Windows tạo ra được bỏ qua, không đổi timestamp/mã trạm.
    info = parseFileName(QStringLiteral("TNg_NDCN_KHITO1_20260101120000__1.txt"), source);
    QVERIFY2(info.valid, qPrintable(info.error));
    info = parseFileName(QStringLiteral("TNg_NDCN_KHITO1_20260101120000 (2).txt"), source);
    QVERIFY2(info.valid, qPrintable(info.error));

    // Tiền tố legacy 1_ chỉ hợp lệ khi nguồn bật.
    const QString legacy = QStringLiteral("1_TNg_NDCN_KHITO1_20260101120000.txt");
    info = parseFileName(legacy, source);
    QVERIFY(!info.valid);
    source.allowLegacyPrefix = true;
    info = parseFileName(legacy, source);
    QVERIFY2(info.valid, qPrintable(info.error));

    info = parseFileName(QStringLiteral("2_TNg_NDCN_KHITO1_20260101120000.txt"), source);
    QVERIFY(!info.valid);
    info = parseFileName(QStringLiteral("TNg_KHITO1_20260101120000.txt"), source);
    QVERIFY(!info.valid);
    info = parseFileName(QStringLiteral("TNg_NDCN_KHITO1_20260229120000.txt"), source);
    QVERIFY(!info.valid);
    info = parseFileName(QStringLiteral("TNg_NDCN_KHITO1_20260101120000.csv"), source);
    QVERIFY(!info.valid);
}

void TestTt10::stationMismatchRejected()
{
    SourceConfig source = flueSource();
    const ParseResult result = parseTt10Content(
        QStringLiteral("TNg_NDCN_KHITO2_20260101120000.txt"),
        QByteArrayLiteral("O2\t12.3\t%\t20260101120000\t00\n"), source, {});
    QVERIFY(!result.accepted);
    QCOMPARE(result.errorCode, QStringLiteral("bad_file_name"));
    QCOMPARE(result.stationCode, QStringLiteral("KHITO2"));
    QVERIFY(!result.stationMatches);
}

QTEST_GUILESS_MAIN(TestTt10)
#include "tst_tt10.moc"
