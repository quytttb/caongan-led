#pragma once

#include "Types.h"

#include <QByteArray>
#include <QString>
#include <QVector>

namespace caongan {

struct ParseIssue {
    int line = 0;
    QString code;
    QString message;
};

struct FileNameInfo {
    bool valid = false;
    QString stationCode;
    QString timestampText;
    QDateTime timestampUtc;
    QString error;
};

struct ParseLimits {
    int maxLines = 500;
    int maxFieldChars = 1024;
    qint64 maxFileBytes = 2 * 1024 * 1024;
};

struct ParseResult {
    bool accepted = false;
    bool stationMatches = false;
    QString stationCode;
    QString fileNameTimestampText;
    QDateTime fileNameTimestampUtc;
    bool fileNameTimestampMismatch = false;
    QVector<Tt10Record> records;
    QVector<ParseIssue> issues;
    QString errorCode;
    QString errorMessage;
};

FileNameInfo parseFileName(const QString &fileName, const SourceConfig &source);
ParseResult parseTt10Content(const QString &fileName,
                             const QByteArray &content,
                             const SourceConfig &source,
                             const QDateTime &nowUtc,
                             const ParseLimits &limits = {});

// Dùng chung cho parser và test: parse số theo locale C, không nhận NaN/Inf,
// dấu phẩy thập phân hoặc ký pháp khoa học chưa được khai báo.
bool parseStrictNumber(const QString &text, double *value);

} // namespace caongan
