#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

namespace caongan {

// ---------------------------------------------------------------------------
// Trạng thái hiển thị: bốn chiều độc lập theo kế hoạch (thiết bị / hợp lệ /
// độ mới / so ngưỡng). Không suy diễn "đạt chuẩn" từ trạng thái thiết bị.
// ---------------------------------------------------------------------------

enum class ChannelGroup {
    Exhaust,
    Wastewater,
};

enum class DeviceStatus {
    Measuring,     // TT10 00
    Calibrating,   // TT10 01
    DeviceFault,   // TT10 02
    Maintenance,   // 04: mã mở rộng của nguồn, không thuộc Bảng 35 TT10
    Unknown,       // thiếu, 03 hoặc mã lạ
};

enum class Validity {
    Valid,
    Missing,       // chưa có dữ liệu hoặc placeholder ---
    Invalid,       // số/timestamp/định dạng không hợp lệ
    UnitMismatch,  // đơn vị không khớp cấu hình
    Conflict,      // cùng mốc đo nhưng nội dung khác
};

enum class Freshness {
    NoReading,
    Fresh,
    Stale,
    FutureTimestamp,
};

enum class ThresholdResult {
    NotConfigured,
    NotEvaluated,
    WithinConfiguredLimit,
    ExceedsConfiguredLimit,
    NotApplicable,
};

enum class DisplayState {
    NoData,
    Measuring,
    WithinLimit,
    ExceedsLimit,
    Calibrating,
    DeviceFault,
    Maintenance,
    Stale,
    FutureTimestamp,
    Invalid,
    UnitMismatch,
    Conflict,
    UnknownStatus,
};

struct Threshold {
    bool enabled = false;
    bool hasLow = false;
    double low = 0.0;
    bool hasHigh = false;
    double high = 0.0;

    bool configured() const { return enabled && (hasLow || hasHigh); }
};

struct BrandingConfig {
    QString companyName;
    QString branchName;
    QString address;
    QString regulationText;
    QString footerNote;
};

struct DisplayConfig {
    QString targetScreenName;
    bool fullScreen = true;
    int designWidth = 1920;
    int designHeight = 1200;
};

struct StorageConfig {
    int checkpointRetentionDays = 30;
    int checkpointMaxRows = 50000;
    int logRetentionDays = 14;
    qint64 logMaxFileBytes = 4 * 1024 * 1024;
    qint64 logMaxTotalBytes = 64 * 1024 * 1024;
};

struct SourceConfig {
    QString id;
    QString name;
    QString folder;
    QString stationCode;
    bool allowLegacyPrefix = true;
    bool groupedFiveColumn = false;
    bool recursive = true;
    int maxDepth = 4;
    int staleAfterSeconds = 900;
    int futureToleranceSeconds = 120;
};

struct ChannelConfig {
    QString id;
    ChannelGroup group = ChannelGroup::Exhaust;
    QString symbol;
    QString displayName;
    QString unit;
    QStringList acceptedUnits;
    QStringList aliases;
    QString sourceId;
    QString stationCode;
    int decimals = 1;
    Threshold threshold;
    int staleAfterSeconds = 900;
};

struct AppConfig {
    int schemaVersion = 1;
    BrandingConfig branding;
    DisplayConfig display;
    StorageConfig storage;
    QList<SourceConfig> sources;
    QList<ChannelConfig> channels;

    const SourceConfig *sourceById(const QString &id) const;
    const ChannelConfig *channelById(const QString &id) const;
    ChannelConfig *channelById(const QString &id);
};

// Một bản ghi thô đã parse từ file TT10, trước khi map vào kênh hiển thị.
struct Tt10Record {
    QString symbol;
    QString rawValue;
    std::optional<double> value;
    QString unit;
    QString timestampText;
    QDateTime measuredAtUtc;
    QString statusText;
    DeviceStatus deviceStatus = DeviceStatus::Unknown;
    Validity validity = Validity::Valid;
    QString issueCode;
    QString issueMessage;
    int line = 0;
};

// Một giá trị đã map vào kênh LED.
struct Reading {
    QString channelId;
    QString sourceId;
    QString stationCode;
    QString rawSymbol;
    QString rawValue;
    std::optional<double> value;
    QString unit;
    QDateTime measuredAtUtc;
    QString measuredAtText;
    DeviceStatus deviceStatus = DeviceStatus::Unknown;
    Validity validity = Validity::Valid;
    bool conflict = false;
    QString issueCode;
    QString issueMessage;
    QString filePath;
    QString contentHash;
    QDateTime receivedAtUtc;
};

// Trạng thái đã đánh giá cho một ô LED, đủ dữ liệu để QML chỉ render.
struct ChannelState {
    ChannelConfig config;
    Reading reading;
    bool hasReading = false;
    Validity validity = Validity::Missing;
    Freshness freshness = Freshness::NoReading;
    ThresholdResult threshold = ThresholdResult::NotConfigured;
    DisplayState display = DisplayState::NoData;
    QString valueText;
    QString statusText;
    QString stateKey;
    QString severity;
    QString detailText;
    QString sourceName;
    int ageSeconds = 0;
};

struct DiagnosticEntry {
    QDateTime atUtc;
    QString severity;   // info / warning / error
    QString sourceId;
    QString channelId;
    QString code;
    QString message;
};

struct ConfigIssue {
    QString field;
    QString message;
};

QString channelGroupId(ChannelGroup group);
QString channelGroupTitle(ChannelGroup group);
QString deviceStatusToCode(DeviceStatus status);
DeviceStatus deviceStatusFromCode(const QString &code);

} // namespace caongan

Q_DECLARE_METATYPE(caongan::AppConfig)
Q_DECLARE_METATYPE(caongan::ChannelState)
Q_DECLARE_METATYPE(QVector<caongan::ChannelState>)
Q_DECLARE_METATYPE(caongan::DiagnosticEntry)
Q_DECLARE_METATYPE(QVector<caongan::DiagnosticEntry>)
