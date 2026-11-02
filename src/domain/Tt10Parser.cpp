#include "Tt10Parser.h"
#include "Tt10Time.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QStringConverter>
#include <QStringList>
#include <cmath>

namespace caongan {

namespace {

constexpr int kTimestampDigits = 14;

QString trimmed(const QString &value)
{
    return value.trimmed();
}

bool isPlaceholder(const QString &value)
{
    const QString v = value.trimmed();
    return v.isEmpty() || v == QLatin1String("---") || v == QLatin1String("--")
           || v == QLatin1String("N/A") || v == QLatin1String("n/a");
}

void addIssue(ParseResult &result, int line, const QString &code, const QString &message)
{
    result.issues.append({line, code, message});
}

} // namespace

bool parseStrictNumber(const QString &text, double *value)
{
    if (!value)
        return false;
    const QString input = text.trimmed();
    static const QRegularExpression decimalOnly(
        QStringLiteral("^[+-]?(?:\\d+(?:\\.\\d*)?|\\.\\d+)$"));
    if (!decimalOnly.match(input).hasMatch())
        return false;

    bool ok = false;
    const double parsed = input.toDouble(&ok);
    if (!ok || !std::isfinite(parsed))
        return false;
    *value = parsed;
    return true;
}

FileNameInfo parseFileName(const QString &fileName, const SourceConfig &source)
{
    FileNameInfo info;
    const QString base = QFileInfo(fileName).fileName();
    if (!base.endsWith(QLatin1String(".txt"), Qt::CaseInsensitive)) {
        info.error = QStringLiteral("Tên file phải có đuôi .txt");
        return info;
    }

    QString stem = base.left(base.size() - 4);
    // Một số nguồn lặp hậu tố copy của Windows: "…__1", "… (1)".
    // Chỉ bỏ hậu tố copy, không sửa phần timestamp/mã trạm.
    static const QRegularExpression copySuffix(QStringLiteral("(?:_+\\d{1,3}|\\(\\d+\\))$"));
    QString withoutCopy = stem;
    while (true) {
        const auto match = copySuffix.match(withoutCopy);
        if (!match.hasMatch())
            break;
        withoutCopy = withoutCopy.left(match.capturedStart());
    }
    stem = withoutCopy;

    const QStringList parts = stem.split(QLatin1Char('_'));
    // Tên chuẩn: TenTinh_TenCoso_TenTram_YYYYMMDDHHMMSS.
    // Chỉ chấp nhận thêm đúng tiền tố legacy "1_" khi nguồn bật tương thích.
    if (parts.size() == 5) {
        if (!source.allowLegacyPrefix || parts.at(0) != QLatin1String("1")) {
            info.error = source.allowLegacyPrefix
                ? QStringLiteral("Tiền tố file không được hỗ trợ: %1 (chỉ chấp nhận 1_)").arg(parts.at(0))
                : QStringLiteral("Nguồn không bật tiền tố legacy 1_");
            return info;
        }
    } else if (parts.size() != 4) {
        info.error = QStringLiteral("Tên file không đúng dạng TenTinh_TenCoso_TenTram_Timestamp");
        return info;
    }

    const QString timestampText = parts.constLast().trimmed();
    static const QRegularExpression digits(QStringLiteral("^\\d{14}$"));
    if (!digits.match(timestampText).hasMatch()) {
        info.error = QStringLiteral("Tên file không kết thúc bằng timestamp 14 chữ số");
        return info;
    }

    const QString station = parts.at(parts.size() - 2).trimmed();
    if (station.isEmpty()) {
        info.error = QStringLiteral("Tên file thiếu mã trạm trước timestamp");
        return info;
    }

    QString timeError;
    const QDateTime utc = tt10::parseTimestamp(timestampText, &timeError);
    if (!utc.isValid()) {
        info.error = timeError;
        return info;
    }

    if (!source.stationCode.trimmed().isEmpty()
        && station.compare(source.stationCode.trimmed(), Qt::CaseInsensitive) != 0) {
        info.stationCode = station;
        info.timestampText = timestampText;
        info.timestampUtc = utc;
        info.error = QStringLiteral("Mã trạm %1 không thuộc nguồn %2 (đang chọn %3)")
                         .arg(station, source.name, source.stationCode);
        return info;
    }

    info.valid = true;
    info.stationCode = station;
    info.timestampText = timestampText;
    info.timestampUtc = utc;
    return info;
}

ParseResult parseTt10Content(const QString &fileName,
                             const QByteArray &content,
                             const SourceConfig &source,
                             const QDateTime &nowUtc,
                             const ParseLimits &limits)
{
    Q_UNUSED(nowUtc)

    ParseResult result;
    result.accepted = false;

    const FileNameInfo nameInfo = parseFileName(fileName, source);
    result.stationCode = nameInfo.stationCode;
    result.fileNameTimestampText = nameInfo.timestampText;
    result.fileNameTimestampUtc = nameInfo.timestampUtc;
    result.stationMatches = nameInfo.valid;

    if (!nameInfo.valid) {
        result.errorCode = QStringLiteral("bad_file_name");
        result.errorMessage = nameInfo.error;
        addIssue(result, 0, result.errorCode, result.errorMessage);
        return result;
    }

    if (content.size() > limits.maxFileBytes) {
        result.errorCode = QStringLiteral("file_too_large");
        result.errorMessage = QStringLiteral("File %1 bytes vượt giới hạn %2 bytes")
                                  .arg(content.size())
                                  .arg(limits.maxFileBytes);
        addIssue(result, 0, result.errorCode, result.errorMessage);
        return result;
    }

    QStringDecoder decoder(QStringDecoder::Utf8);
    QString text = decoder.decode(content);
    if (decoder.hasError()) {
        result.errorCode = QStringLiteral("bad_encoding");
        result.errorMessage = QStringLiteral("File không phải UTF-8 hợp lệ");
        addIssue(result, 0, result.errorCode, result.errorMessage);
        return result;
    }

    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1);

    const QStringList rawLines = text.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    if (rawLines.size() > limits.maxLines) {
        result.errorCode = QStringLiteral("too_many_lines");
        result.errorMessage = QStringLiteral("File có %1 dòng, vượt giới hạn %2")
                                  .arg(rawLines.size())
                                  .arg(limits.maxLines);
        addIssue(result, 0, result.errorCode, result.errorMessage);
        return result;
    }

    int dataLines = 0;
    bool structuralError = false;

    for (int lineIndex = 0; lineIndex < rawLines.size(); ++lineIndex) {
        QString line = rawLines.at(lineIndex);
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        if (line.trimmed().isEmpty())
            continue;
        ++dataLines;

        const QStringList fields = line.split(QLatin1Char('\t'), Qt::KeepEmptyParts);
        if (fields.size() < 4) {
            addIssue(result, lineIndex + 1, QStringLiteral("bad_column_count"),
                     QStringLiteral("Dòng có %1 trường, cần 4 hoặc 5 trường TAB").arg(fields.size()));
            structuralError = true;
            continue;
        }

        QVector<QStringList> groups;
        if (fields.size() == 5 || fields.size() == 4) {
            groups.append(fields);
        } else if (source.groupedFiveColumn && fields.size() % 5 == 0) {
            for (int i = 0; i < fields.size(); i += 5)
                groups.append(fields.mid(i, 5));
        } else {
            addIssue(result, lineIndex + 1, QStringLiteral("bad_column_count"),
                     QStringLiteral("Dòng có %1 trường; nguồn không bật chế độ nhóm 5 trường lặp")
                         .arg(fields.size()));
            structuralError = true;
            continue;
        }

        for (const QStringList &group : groups) {
            if (group.size() != 5 && group.size() != 4) {
                addIssue(result, lineIndex + 1, QStringLiteral("bad_column_count"),
                         QStringLiteral("Nhóm trường không đủ 5 cột"));
                structuralError = true;
                continue;
            }

            for (const QString &field : group) {
                if (field.size() > limits.maxFieldChars) {
                    addIssue(result, lineIndex + 1, QStringLiteral("field_too_long"),
                             QStringLiteral("Trường dài %1 ký tự, vượt giới hạn %2")
                                 .arg(field.size())
                                 .arg(limits.maxFieldChars));
                    structuralError = true;
                }
            }

            Tt10Record record;
            record.line = lineIndex + 1;
            record.symbol = trimmed(group.at(0));
            record.rawValue = trimmed(group.at(1));
            record.unit = trimmed(group.at(2));
            record.timestampText = trimmed(group.at(3));
            record.statusText = group.size() >= 5 ? trimmed(group.at(4)) : QString();

            if (record.symbol.isEmpty()) {
                addIssue(result, record.line, QStringLiteral("missing_symbol"),
                         QStringLiteral("Dòng thiếu ký hiệu thông số"));
                structuralError = true;
                continue;
            }

            QString timeError;
            record.measuredAtUtc = tt10::parseTimestamp(record.timestampText, &timeError);
            if (!record.measuredAtUtc.isValid()) {
                addIssue(result, record.line, QStringLiteral("bad_timestamp"), timeError);
                structuralError = true;
                continue;
            }

            if (group.size() < 5 || record.statusText.isEmpty()) {
                record.deviceStatus = DeviceStatus::Unknown;
                record.issueCode = QStringLiteral("missing_status");
                record.issueMessage = QStringLiteral("Bản ghi thiếu trường trạng thái thiết bị");
                addIssue(result, record.line, record.issueCode, record.issueMessage);
            } else if (record.statusText == QLatin1String("00")) {
                record.deviceStatus = DeviceStatus::Measuring;
            } else if (record.statusText == QLatin1String("01")) {
                record.deviceStatus = DeviceStatus::Calibrating;
            } else if (record.statusText == QLatin1String("02")) {
                record.deviceStatus = DeviceStatus::DeviceFault;
            } else {
                record.deviceStatus = DeviceStatus::Unknown;
                record.issueCode = QStringLiteral("unknown_status");
                record.issueMessage = QStringLiteral("Mã trạng thái thiết bị không hỗ trợ: %1")
                                         .arg(record.statusText);
                addIssue(result, record.line, record.issueCode, record.issueMessage);
            }

            if (isPlaceholder(record.rawValue)) {
                record.value.reset();
                record.validity = Validity::Missing;
                record.issueCode = QStringLiteral("missing_value");
                record.issueMessage = QStringLiteral("Giá trị trống/placeholder, không thay bằng 0");
            } else {
                double parsed = 0.0;
                if (!parseStrictNumber(record.rawValue, &parsed)) {
                    record.value.reset();
                    record.validity = Validity::Invalid;
                    record.issueCode = QStringLiteral("bad_number");
                    record.issueMessage = QStringLiteral("Giá trị không phải số thập phân hợp lệ: %1")
                                             .arg(record.rawValue);
                    addIssue(result, record.line, record.issueCode, record.issueMessage);
                } else {
                    record.value = parsed;
                    record.validity = Validity::Valid;
                }
            }

            if (record.timestampText != result.fileNameTimestampText) {
                result.fileNameTimestampMismatch = true;
            }

            result.records.append(record);
        }
    }

    if (dataLines == 0) {
        result.errorCode = QStringLiteral("empty_file");
        result.errorMessage = QStringLiteral("File không có dòng dữ liệu");
        addIssue(result, 0, result.errorCode, result.errorMessage);
        return result;
    }

    if (structuralError) {
        // Không nhập một phần file hỏng cấu trúc.
        result.records.clear();
        result.errorCode = QStringLiteral("structure_error");
        result.errorMessage = QStringLiteral("File có lỗi cấu trúc — từ chối toàn bộ batch");
        return result;
    }

    result.accepted = true;
    return result;
}

} // namespace caongan
