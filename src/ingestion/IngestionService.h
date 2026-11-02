#pragma once

#include "DirectoryScan.h"
#include "domain/LatestReadingStore.h"
#include "storage/SnapshotStore.h"

#include <QElapsedTimer>
#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QVector>

#include <memory>

namespace caongan {

// Chạy toàn bộ I/O đọc file + SQLite ngoài UI thread. Chỉ gửi snapshot trạng
// thái qua signal; QML không quyết định hợp lệ/ngưỡng.
class IngestionService : public QObject
{
    Q_OBJECT
public:
    explicit IngestionService(QObject *parent = nullptr);
    ~IngestionService() override;

public slots:
    void start(const caongan::AppConfig &config, const QString &snapshotPath);
    void reloadConfig(const caongan::AppConfig &config);
    void setPaused(bool paused);
    void requestRescan();
    void shutdown();

signals:
    void channelsChanged(QVector<caongan::ChannelState> states);
    void diagnosticsBatch(QVector<caongan::DiagnosticEntry> entries);
    void sourceStatusChanged(const QString &sourceId, bool available, const QString &message);
    void heartbeat(const QDateTime &nowUtc);
    void stopped();

private slots:
    void tick();

private:
    struct SourceRuntime {
        SourceConfig config;
        QFileSystemWatcher *watcher = nullptr;
        FileStabilityTracker stability;
        QDateTime lastScanUtc;
        QDateTime lastFileUtc;
        int nextCandidateIndex = 0;
        bool available = false;
        bool statusInitialised = false;
        QString lastMessage;
        QHash<QString, QPair<qint64, qint64>> memoryProcessed;
        QHash<QString, QDateTime> backoffUntil;
        QHash<QString, int> failureCount;
    };

    void setupRuntime(const AppConfig &config);
    void teardownRuntimes();
    void scanSource(SourceRuntime &runtime, const QDateTime &nowUtc);
    void processCandidate(SourceRuntime &runtime, const FileCandidate &candidate, const QDateTime &nowUtc);
    void updateWatcher(SourceRuntime &runtime, const DirectoryScanResult &scan);
    void setSourceStatus(SourceRuntime &runtime, bool available, const QString &message);
    void addDiagnostic(const QString &severity,
                       const QString &sourceId,
                       const QString &channelId,
                       const QString &code,
                       const QString &message,
                       const QDateTime &nowUtc);
    void flushDiagnostics();
    void emitStates(const QDateTime &nowUtc, bool force = false);
    void maybePrune(const QDateTime &nowUtc);
    void persist(const QVector<Reading> &readings,
                 const QVector<CheckpointRecord> &checkpoints,
                 const QDateTime &nowUtc);
    void saveCheckpoint(SourceRuntime &runtime, const CheckpointRecord &checkpoint, const QDateTime &nowUtc);

    AppConfig m_config;
    LatestReadingStore m_store;
    std::unique_ptr<SnapshotStore> m_snapshotStore;
    QString m_snapshotPath;
    QVector<SourceRuntime *> m_runtimes;
    QTimer *m_timer = nullptr;
    QVector<DiagnosticEntry> m_pendingDiagnostics;
    QHash<QString, QDateTime> m_diagnosticRateLimit;
    QHash<QString, QString> m_lastSignatures;
    QDateTime m_lastPruneUtc;
    bool m_started = false;
    bool m_paused = false;
    bool m_rescanRequested = false;
    bool m_storageHealthy = true;
    bool m_firstEmit = true;
};

} // namespace caongan
