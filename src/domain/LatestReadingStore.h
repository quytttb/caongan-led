#pragma once

#include "Types.h"

#include <QHash>

namespace caongan {

struct ApplyOutcome {
    enum class Code {
        Accepted,
        DuplicateIgnored,
        ConflictRecorded,
        OutOfOrderIgnored,
        FutureRejected,
        UnknownChannel,
        SourceMismatch,
    };
    Code code = Code::Accepted;
    QString message;
};

// Giữ bản ghi mới nhất theo thời gian đo của từng kênh. Không dùng thứ tự nhận
// file. Xung đột cùng mốc được đánh dấu thay vì để file đến trước thắng.
class LatestReadingStore
{
public:
    void setConfig(const AppConfig &config);
    const AppConfig &config() const { return m_config; }

    ApplyOutcome apply(const Reading &reading, const QDateTime &nowUtc);
    void loadPersisted(const QVector<Reading> &readings, const QDateTime &nowUtc = {});

    QVector<ChannelState> evaluate(const QDateTime &nowUtc) const;
    QVector<Reading> readings() const;
    bool latestReading(const QString &channelId, Reading *reading) const;
    bool hasReading(const QString &channelId) const;
    void clear();

private:
    AppConfig m_config;
    QHash<QString, Reading> m_latest;
};

} // namespace caongan
