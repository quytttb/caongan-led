#pragma once

#include "Tt10Parser.h"
#include "Types.h"

namespace caongan {

struct MapResult {
    QVector<Reading> readings;
    QVector<DiagnosticEntry> diagnostics;
};

// Ánh xạ ký hiệu gốc sang 12 kênh theo alias cấu hình. Không fuzzy-match,
// không tự gán trạm/kênh còn trống.
class ParameterMapper
{
public:
    static MapResult map(const ParseResult &parsed,
                         const SourceConfig &source,
                         const AppConfig &config,
                         const QString &filePath,
                         const QString &contentHash,
                         const QDateTime &receivedAtUtc);
};

} // namespace caongan
