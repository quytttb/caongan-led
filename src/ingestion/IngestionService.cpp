#include "IngestionService.h"

#include "domain/ParameterCatalog.h"
#include "domain/ParameterMapper.h"
#include "domain/Tt10Parser.h"
#include "domain/Tt10Time.h"
#include "utils/LogSetup.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMetaType>

namespace caongan {

namespace {

constexpr int kScanIntervalMs = 2000;
constexpr int kMaxFilesPerTick = 20;
constexpr int kMaxEntriesPerScan = 5000;
constexpr int kStableMs = 1200;
constexpr int kDiagnosticRepeatSeconds = 60;
constexpr int kPruneIntervalSeconds = 3600;

QString logText(QString text)
{
    text.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    text.replace(QLatin1Char('\r'), QStringLiteral("\\r"));
    text.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    return text;
}

QString sha256Hex(const QByteArray &content)
{
    return QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex());
}

int backoffSeconds(int attempt)
{
    switch (qBound(0, attempt, 4)) {
    case 0: return 1;
    case 1: return 2;
    case 2: return 5;
    case 3: return 10;
    default: return 60;
    }
}

bool belongsToConfiguredRoot(const Reading &reading, const AppConfig &config)
{
    const SourceConfig *source = config.sourceById(reading.sourceId);
    if (!source || source->folder.trimmed().isEmpty()
        || !QDir::isAbsolutePath(source->folder) || !QDir::isAbsolutePath(reading.filePath))
        return false;
    const QString relative = QDir(source->folder).relativeFilePath(reading.filePath);
    return !QDir::isAbsolutePath(relative) && relative != QLatin1String("..")
           && !relative.startsWith(QLatin1String("../"));
}

template<typename T>
void pruneMissingKeys(QHash<QString, T> &items, const QSet<QString> &seen)
{
    for (auto it = items.begin(); it != items.end();) {
        if (!seen.contains(it.key()))
            it = items.erase(it);
        else
            ++it;
    }
}

} // namespace

IngestionService::IngestionService(QObject *parent)
    : QObject(parent)
{
}

IngestionService::~IngestionService()
{
    teardownRuntimes();
}

void IngestionService::start(const AppConfig &config, const QString &snapshotPath)
{
    if (m_started)
        return;

    m_config = config;
    m_config.channels = orderChannels(m_config.channels);
    m_store.setConfig(m_config);
    m_snapshotPath = snapshotPath;
    m_snapshotStore = std::make_unique<SnapshotStore>(snapshotPath);
    qInfo() << "[ingestion] Khởi động, snapshot" << snapshotPath
            << "nguồn" << m_config.sources.size() << "kênh" << m_config.channels.size();

    const QDateTime now = tt10::nowUtc();
    QString dbError;
    if (!m_snapshotStore->open(&dbError)) {
        m_storageHealthy = false;
        addDiagnostic(QStringLiteral("error"), QString(), QString(),
                      QStringLiteral("snapshot_unavailable"),
                      QStringLiteral("Không mở được snapshot SQLite: %1").arg(dbError), now);
    } else {
        QVector<Reading> persisted;
        QString loadError;
        if (!m_snapshotStore->loadReadings(&persisted, &loadError)) {
            m_storageHealthy = false;
            addDiagnostic(QStringLiteral("error"), QString(), QString(),
                          QStringLiteral("snapshot_load_failed"),
                          QStringLiteral("Không đọc được snapshot: %1").arg(loadError), now);
        } else {
            QVector<Reading> matching;
            for (const Reading &reading : persisted) {
                if (belongsToConfiguredRoot(reading, m_config)) {
                    matching.append(reading);
                } else {
                    addDiagnostic(QStringLiteral("warning"), reading.sourceId, reading.channelId,
                                  QStringLiteral("snapshot_source_changed"),
                                  QStringLiteral("Bỏ qua snapshot không thuộc thư mục nguồn đang cấu hình"), now);
                }
            }
            m_store.loadPersisted(matching, now);
        }
    }

    setupRuntime(m_config);
    m_started = true;
    m_firstEmit = true;
    emitStates(now, true);
    flushDiagnostics();

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &IngestionService::tick);
    m_timer->start();

    m_lastPruneUtc = now;
    tick();
}

void IngestionService::reloadConfig(const AppConfig &config)
{
    if (!m_started) {
        m_config = config;
        m_store.setConfig(config);
        return;
    }

    const QDateTime now = tt10::nowUtc();
    m_config = config;
    m_config.channels = orderChannels(m_config.channels);
    m_store.setConfig(m_config);
    teardownRuntimes();
    setupRuntime(m_config);
    m_lastSignatures.clear();
    m_firstEmit = true;
    m_rescanRequested = true;
    emitStates(now, true);
    addDiagnostic(QStringLiteral("info"), QString(), QString(),
                  QStringLiteral("config_reloaded"),
                  QStringLiteral("Đã áp dụng cấu hình mới"), now);
    flushDiagnostics();
}

void IngestionService::setPaused(bool paused)
{
    if (m_paused == paused)
        return;
    m_paused = paused;
    qInfo() << "[ingestion]" << (paused ? "Tạm dừng nhập nguồn" : "Tiếp tục nhập nguồn");
    if (!m_paused) {
        m_rescanRequested = true;
        // Sau mô phỏng, UI đã bị reset; buộc phát lại snapshot thật thay vì
        // chờ một kênh đổi giá trị.
        m_firstEmit = true;
        m_lastSignatures.clear();
        if (m_timer)
            m_timer->start();
    }
}

void IngestionService::requestRescan()
{
    m_rescanRequested = true;
    m_firstEmit = true;
}

void IngestionService::shutdown()
{
    qInfo() << "[ingestion] Dừng dịch vụ";
    if (m_timer) {
        m_timer->stop();
        m_timer->deleteLater();
        m_timer = nullptr;
    }
    teardownRuntimes();
    m_store.clear();
    if (m_snapshotStore) {
        m_snapshotStore->close();
        m_snapshotStore.reset();
    }
    m_started = false;
    emit stopped();
}

void IngestionService::setupRuntime(const AppConfig &config)
{
    m_runtimes.clear();
    for (const SourceConfig &source : config.sources) {
        auto *runtime = new SourceRuntime;
        runtime->config = source;
        runtime->watcher = new QFileSystemWatcher(this);
        connect(runtime->watcher, &QFileSystemWatcher::directoryChanged,
                this, [this](const QString &) { m_rescanRequested = true; });
        connect(runtime->watcher, &QFileSystemWatcher::fileChanged,
                this, [this](const QString &) { m_rescanRequested = true; });
        m_runtimes.append(runtime);
    }
}

void IngestionService::teardownRuntimes()
{
    for (SourceRuntime *runtime : m_runtimes) {
        if (runtime->watcher) {
            runtime->watcher->disconnect(this);
            runtime->watcher->deleteLater();
        }
        delete runtime;
    }
    m_runtimes.clear();
}

void IngestionService::tick()
{
    if (!m_started)
        return;

    const QDateTime now = tt10::nowUtc();
    emit heartbeat(now);

    if (m_paused)
        return;

    const bool scanRequested = m_rescanRequested;
    m_rescanRequested = false;

    for (SourceRuntime *runtime : m_runtimes) {
        if (!runtime)
            continue;

        bool folderAvailable = false;
        if (!runtime->config.folder.trimmed().isEmpty()) {
            const QFileInfo folderInfo(runtime->config.folder);
            folderAvailable = folderInfo.exists() && folderInfo.isDir();
        }

        if (!folderAvailable) {
            QString message;
            if (runtime->config.folder.trimmed().isEmpty())
                message = QStringLiteral("Chưa cấu hình thư mục nguồn");
            else
                message = QStringLiteral("Không truy cập được thư mục %1").arg(runtime->config.folder);
            setSourceStatus(*runtime, false, message);
            runtime->stability.pruneMissing({});
            continue;
        }

        const bool due = !runtime->lastScanUtc.isValid()
                         || runtime->lastScanUtc.msecsTo(now) >= kScanIntervalMs
                         || scanRequested;
        if (!due)
            continue;

        runtime->lastScanUtc = now;
        scanSource(*runtime, now);
    }

    maybePrune(now);
    emitStates(now);
    flushDiagnostics();
}

void IngestionService::scanSource(SourceRuntime &runtime, const QDateTime &nowUtc)
{
    const DirectoryScanResult scan = scanSourceFolder(runtime.config, kMaxEntriesPerScan);
    if (!scan.rootAvailable) {
        setSourceStatus(runtime, false, scan.issues.value(0, QStringLiteral("Nguồn không khả dụng")));
        return;
    }

    if (scan.truncated) {
        const QString issue = scan.issues.value(0, QStringLiteral("Không quét hết thư mục nguồn"));
        setSourceStatus(runtime, false, issue);
        addDiagnostic(QStringLiteral("error"), runtime.config.id, QString(),
                      QStringLiteral("scan_truncated"), issue, nowUtc);
    } else {
        setSourceStatus(runtime, true,
                        QStringLiteral("Đang theo dõi %1").arg(runtime.config.folder));
    }

    updateWatcher(runtime, scan);

    QSet<QString> seenKeys;
    for (const FileCandidate &candidate : scan.candidates) {
        seenKeys.insert(candidate.fileKey);
    }
    // Quan sát toàn bộ tập ứng viên trong giới hạn trước khi chọn batch;
    // duyệt vòng để file đã checkpoint hoặc lỗi không chặn mãi file phía sau.
    QVector<FileStabilityTracker::Status> statuses;
    statuses.reserve(scan.candidates.size());
    for (const FileCandidate &candidate : scan.candidates)
        statuses.append(runtime.stability.observe(candidate.fileKey, candidate.size,
                                                  candidate.mtimeMs, nowUtc, kStableMs));
    runtime.stability.pruneMissing(seenKeys);
    pruneMissingKeys(runtime.memoryProcessed, seenKeys);
    pruneMissingKeys(runtime.backoffUntil, seenKeys);
    pruneMissingKeys(runtime.failureCount, seenKeys);

    const int count = int(scan.candidates.size());
    if (count == 0) {
        runtime.nextCandidateIndex = 0;
        return;
    }
    const int start = runtime.nextCandidateIndex % count;
    int processed = 0;
    for (int offset = 0; offset < count && processed < kMaxFilesPerTick; ++offset) {
        const int index = (start + offset) % count;
        if (statuses.at(index) != FileStabilityTracker::Status::Stable)
            continue;
        processCandidate(runtime, scan.candidates.at(index), nowUtc);
        runtime.nextCandidateIndex = (index + 1) % count;
        ++processed;
    }
}

void IngestionService::updateWatcher(SourceRuntime &runtime, const DirectoryScanResult &scan)
{
    if (!runtime.watcher || !scan.rootAvailable)
        return;
    const QStringList current = runtime.watcher->directories();
    if (!current.contains(runtime.config.folder))
        runtime.watcher->addPath(runtime.config.folder);
}

void IngestionService::processCandidate(SourceRuntime &runtime,
                                        const FileCandidate &candidate,
                                        const QDateTime &nowUtc)
{
    const QDateTime backoff = runtime.backoffUntil.value(candidate.fileKey);
    if (backoff.isValid() && nowUtc < backoff)
        return;

    const QFileInfo before(candidate.absolutePath);
    if (!before.exists() || !before.isFile())
        return;
    if (before.size() != candidate.size
        || before.lastModified().toMSecsSinceEpoch() != candidate.mtimeMs) {
        runtime.stability.forget(candidate.fileKey);
        return;
    }

    const QPair<qint64, qint64> memorySignature(candidate.size, candidate.mtimeMs);
    if (runtime.memoryProcessed.value(candidate.fileKey) == memorySignature)
        return;

    CheckpointRecord existing;
    bool hasExisting = false;
    if (m_snapshotStore && m_snapshotStore->isOpen())
        hasExisting = m_snapshotStore->checkpointFor(candidate.fileKey, &existing);
    if (hasExisting && existing.size == candidate.size && existing.mtimeMs == candidate.mtimeMs)
        return;

    ParseLimits limits;
    if (before.size() > limits.maxFileBytes) {
        CheckpointRecord checkpoint;
        checkpoint.fileKey = candidate.fileKey;
        checkpoint.sourceId = runtime.config.id;
        checkpoint.filePath = candidate.absolutePath;
        checkpoint.size = candidate.size;
        checkpoint.mtimeMs = candidate.mtimeMs;
        checkpoint.contentHash = QString();
        checkpoint.outcome = QStringLiteral("too_large");
        checkpoint.message = QStringLiteral("File vượt giới hạn %1 bytes").arg(limits.maxFileBytes);
        checkpoint.processedAtUtc = nowUtc;
        saveCheckpoint(runtime, checkpoint, nowUtc);
        addDiagnostic(QStringLiteral("error"), runtime.config.id, QString(),
                      QStringLiteral("file_too_large"), checkpoint.message, nowUtc);
        return;
    }

    QFile file(candidate.absolutePath);
    if (!file.open(QIODevice::ReadOnly)) {
        const int failures = runtime.failureCount.value(candidate.fileKey) + 1;
        runtime.failureCount.insert(candidate.fileKey, failures);
        runtime.backoffUntil.insert(candidate.fileKey,
                                    nowUtc.addSecs(backoffSeconds(failures - 1)));
        addDiagnostic(QStringLiteral("warning"), runtime.config.id, QString(),
                      QStringLiteral("file_open_failed"),
                      QStringLiteral("Không đọc được %1: %2 (thử lại sau)")
                          .arg(candidate.relativePath, file.errorString()), nowUtc);
        return;
    }

    const QByteArray content = file.readAll();
    file.close();

    const QFileInfo after(candidate.absolutePath);
    if (!after.exists() || after.size() != before.size()
        || after.lastModified().toMSecsSinceEpoch() != before.lastModified().toMSecsSinceEpoch()) {
        runtime.stability.forget(candidate.fileKey);
        addDiagnostic(QStringLiteral("warning"), runtime.config.id, QString(),
                      QStringLiteral("file_changed_while_reading"),
                      QStringLiteral("File %1 thay đổi trong lúc đọc — bỏ kết quả, chờ ổn định lại")
                          .arg(candidate.relativePath), nowUtc);
        return;
    }

    const QString hash = sha256Hex(content);
    if (hasExisting && existing.contentHash == hash) {
        CheckpointRecord touched = existing;
        touched.size = candidate.size;
        touched.mtimeMs = candidate.mtimeMs;
        touched.processedAtUtc = nowUtc;
        saveCheckpoint(runtime, touched, nowUtc);
        return;
    }

    // Không bỏ qua theo hash toàn cục: tên file, trạm và profile nguồn cũng
    // là một phần của hợp đồng. Parser xác thực từng file; store loại bản ghi
    // trùng theo kênh/mốc/nội dung sau khi đã mapping.

    const QString fileName = QFileInfo(candidate.absolutePath).fileName();
    const ParseResult parsed = parseTt10Content(fileName, content, runtime.config, nowUtc, limits);

    if (!parsed.stationCode.isEmpty()
        && runtime.config.stationCode.compare(parsed.stationCode, Qt::CaseInsensitive) != 0) {
        CheckpointRecord skipped;
        skipped.fileKey = candidate.fileKey;
        skipped.sourceId = runtime.config.id;
        skipped.filePath = candidate.absolutePath;
        skipped.size = candidate.size;
        skipped.mtimeMs = candidate.mtimeMs;
        skipped.contentHash = hash;
        skipped.outcome = QStringLiteral("skipped_other_station");
        skipped.message = QStringLiteral("File thuộc trạm %1, nguồn này chọn %2")
                              .arg(parsed.stationCode, runtime.config.stationCode);
        skipped.processedAtUtc = nowUtc;
        saveCheckpoint(runtime, skipped, nowUtc);
        addDiagnostic(QStringLiteral("info"), runtime.config.id, QString(),
                      QStringLiteral("other_station"), skipped.message, nowUtc);
        return;
    }

    for (const ParseIssue &issue : parsed.issues) {
        const QString severity = (issue.code == QLatin1String("unknown_status")
                                  || issue.code == QLatin1String("missing_status")
                                  || issue.code == QLatin1String("missing_value"))
                                     ? QStringLiteral("warning")
                                     : QStringLiteral("error");
        addDiagnostic(severity, runtime.config.id, QString(), issue.code,
                      QStringLiteral("%1 (dòng %2)").arg(issue.message).arg(issue.line), nowUtc);
    }

    if (!parsed.accepted) {
        CheckpointRecord rejected;
        rejected.fileKey = candidate.fileKey;
        rejected.sourceId = runtime.config.id;
        rejected.filePath = candidate.absolutePath;
        rejected.size = candidate.size;
        rejected.mtimeMs = candidate.mtimeMs;
        rejected.contentHash = hash;
        rejected.outcome = QStringLiteral("rejected");
        rejected.message = parsed.errorMessage.isEmpty()
            ? QStringLiteral("File không hợp lệ")
            : parsed.errorMessage;
        rejected.processedAtUtc = nowUtc;
        saveCheckpoint(runtime, rejected, nowUtc);
        addDiagnostic(QStringLiteral("error"), runtime.config.id, QString(),
                      parsed.errorCode.isEmpty() ? QStringLiteral("parse_rejected") : parsed.errorCode,
                      QStringLiteral("%1: %2").arg(candidate.relativePath, rejected.message), nowUtc);
        return;
    }

    if (parsed.fileNameTimestampMismatch) {
        addDiagnostic(QStringLiteral("warning"), runtime.config.id, QString(),
                      QStringLiteral("filename_timestamp_mismatch"),
                      QStringLiteral("%1: timestamp trong file khác timestamp tên file — dùng timestamp bản ghi")
                          .arg(candidate.relativePath), nowUtc);
    }

    const MapResult mapped = ParameterMapper::map(parsed, runtime.config, m_config,
                                                  candidate.absolutePath, hash, nowUtc);
    for (const DiagnosticEntry &entry : mapped.diagnostics)
        addDiagnostic(entry.severity, entry.sourceId, entry.channelId, entry.code, entry.message, nowUtc);

    QVector<Reading> changed;
    for (const Reading &reading : mapped.readings) {
        const ApplyOutcome outcome = m_store.apply(reading, nowUtc);
        switch (outcome.code) {
        case ApplyOutcome::Code::Accepted:
            changed.append(reading);
            qInfo() << "[reading] nguồn" << reading.sourceId << "kênh" << reading.channelId
                    << "mốc" << reading.measuredAtText << "mã" << deviceStatusToCode(reading.deviceStatus)
                    << "giá trị" << reading.rawValue.left(128) << "đơn vị" << reading.unit.left(64);
            break;
        case ApplyOutcome::Code::ConflictRecorded: {
            Reading stored;
            if (m_store.latestReading(reading.channelId, &stored))
                changed.append(stored);
            addDiagnostic(QStringLiteral("warning"), runtime.config.id, reading.channelId,
                          QStringLiteral("conflict"), outcome.message, nowUtc);
            break;
        }
        case ApplyOutcome::Code::OutOfOrderIgnored:
            addDiagnostic(QStringLiteral("info"), runtime.config.id, reading.channelId,
                          QStringLiteral("out_of_order"), outcome.message, nowUtc);
            break;
        case ApplyOutcome::Code::FutureRejected:
            addDiagnostic(QStringLiteral("warning"), runtime.config.id, reading.channelId,
                          QStringLiteral("future_timestamp"), outcome.message, nowUtc);
            break;
        case ApplyOutcome::Code::DuplicateIgnored:
            break;
        case ApplyOutcome::Code::UnknownChannel:
        case ApplyOutcome::Code::SourceMismatch:
            addDiagnostic(QStringLiteral("error"), runtime.config.id, reading.channelId,
                          QStringLiteral("mapping_error"), outcome.message, nowUtc);
            break;
        }
    }

    CheckpointRecord checkpoint;
    checkpoint.fileKey = candidate.fileKey;
    checkpoint.sourceId = runtime.config.id;
    checkpoint.filePath = candidate.absolutePath;
    checkpoint.size = candidate.size;
    checkpoint.mtimeMs = candidate.mtimeMs;
    checkpoint.contentHash = hash;
    checkpoint.outcome = QStringLiteral("ok");
    checkpoint.message = QStringLiteral("%1 bản ghi").arg(parsed.records.size());
    checkpoint.processedAtUtc = nowUtc;

    persist(changed, {checkpoint}, nowUtc);
    runtime.memoryProcessed.insert(candidate.fileKey, memorySignature);
    runtime.failureCount.remove(candidate.fileKey);
    runtime.backoffUntil.remove(candidate.fileKey);
    runtime.lastFileUtc = nowUtc;

    addDiagnostic(QStringLiteral("info"), runtime.config.id, QString(),
                  QStringLiteral("file_imported"),
                  QStringLiteral("%1: nhập %2 bản ghi, cập nhật %3 kênh")
                      .arg(candidate.relativePath)
                      .arg(parsed.records.size())
                      .arg(changed.size()), nowUtc);
}

void IngestionService::persist(const QVector<Reading> &readings,
                               const QVector<CheckpointRecord> &checkpoints,
                               const QDateTime &nowUtc)
{
    if (!m_snapshotStore || !m_snapshotStore->isOpen())
        return;
    QString error;
    if (!m_snapshotStore->saveTransaction(readings, checkpoints, &error)) {
        m_storageHealthy = false;
        addDiagnostic(QStringLiteral("error"), QString(), QString(),
                      QStringLiteral("snapshot_save_failed"),
                      QStringLiteral("Không ghi được snapshot/checkpoint: %1").arg(error), nowUtc);
    } else {
        m_storageHealthy = true;
    }
}

void IngestionService::saveCheckpoint(SourceRuntime &runtime,
                                      const CheckpointRecord &checkpoint,
                                      const QDateTime &nowUtc)
{
    persist({}, {checkpoint}, nowUtc);
    runtime.memoryProcessed.insert(checkpoint.fileKey,
                                   qMakePair(checkpoint.size, checkpoint.mtimeMs));
}

void IngestionService::setSourceStatus(SourceRuntime &runtime, bool available, const QString &message)
{
    if (runtime.statusInitialised && runtime.available == available && runtime.lastMessage == message)
        return;
    runtime.statusInitialised = true;
    runtime.available = available;
    runtime.lastMessage = message;
    writeApplicationLog(available ? LogLevel::Info : LogLevel::Warning,
                        QStringLiteral("[source] %1 %2: %3")
                            .arg(logText(runtime.config.id),
                                 available ? QStringLiteral("sẵn sàng") : QStringLiteral("không sẵn sàng"),
                                 logText(message)));
    emit sourceStatusChanged(runtime.config.id, available, message);
}

void IngestionService::addDiagnostic(const QString &severity,
                                     const QString &sourceId,
                                     const QString &channelId,
                                     const QString &code,
                                     const QString &message,
                                     const QDateTime &nowUtc)
{
    const QString key = sourceId + QLatin1Char('|') + code + QLatin1Char('|') + message;
    const QDateTime last = m_diagnosticRateLimit.value(key);
    if (last.isValid() && last.secsTo(nowUtc) < kDiagnosticRepeatSeconds)
        return;
    m_diagnosticRateLimit.insert(key, nowUtc);
    if (m_diagnosticRateLimit.size() > 500) {
        const auto keys = m_diagnosticRateLimit.keys();
        for (int i = 0; i < keys.size() / 2; ++i)
            m_diagnosticRateLimit.remove(keys.at(i));
    }

    const LogLevel level = severity == QLatin1String("error") ? LogLevel::Error
        : (severity == QLatin1String("warning") ? LogLevel::Warning : LogLevel::Info);
    writeApplicationLog(level, QStringLiteral("[diagnostic] source=%1 channel=%2 code=%3 %4")
                                  .arg(logText(sourceId), logText(channelId), logText(code), logText(message)));

    DiagnosticEntry entry;
    entry.atUtc = nowUtc;
    entry.severity = severity;
    entry.sourceId = sourceId;
    entry.channelId = channelId;
    entry.code = code;
    entry.message = message;
    m_pendingDiagnostics.append(entry);
    if (m_pendingDiagnostics.size() > 100)
        m_pendingDiagnostics.remove(0, m_pendingDiagnostics.size() - 100);
}

void IngestionService::flushDiagnostics()
{
    if (m_pendingDiagnostics.isEmpty())
        return;
    const QVector<DiagnosticEntry> batch = m_pendingDiagnostics;
    m_pendingDiagnostics.clear();
    emit diagnosticsBatch(batch);
}

void IngestionService::emitStates(const QDateTime &nowUtc, bool force)
{
    const QVector<ChannelState> states = m_store.evaluate(nowUtc);
    bool changed = force || m_firstEmit;
    for (const ChannelState &state : states) {
        const QString previousState = m_loggedStates.value(state.config.id);
        if (previousState != state.stateKey) {
            qInfo() << "[state]" << state.config.id << previousState << "->" << state.stateKey;
            m_loggedStates.insert(state.config.id, state.stateKey);
        }
        const QString signature = state.stateKey + QLatin1Char('|') + state.valueText
                                  + QLatin1Char('|') + state.statusText
                                  + QLatin1Char('|') + state.detailText
                                  + QLatin1Char('|') + QString::number(state.ageSeconds / 60);
        if (m_lastSignatures.value(state.config.id) != signature) {
            m_lastSignatures.insert(state.config.id, signature);
            changed = true;
        }
    }
    if (!changed && !m_firstEmit)
        return;
    m_firstEmit = false;
    emit channelsChanged(states);
}

void IngestionService::maybePrune(const QDateTime &nowUtc)
{
    if (!m_snapshotStore || !m_snapshotStore->isOpen())
        return;
    if (m_lastPruneUtc.isValid()
        && m_lastPruneUtc.secsTo(nowUtc) < kPruneIntervalSeconds)
        return;
    m_lastPruneUtc = nowUtc;

    QString error;
    if (!m_snapshotStore->prune(m_config.storage.checkpointRetentionDays,
                                m_config.storage.checkpointMaxRows, &error)) {
        addDiagnostic(QStringLiteral("warning"), QString(), QString(),
                      QStringLiteral("checkpoint_prune_failed"), error, nowUtc);
    }
}

} // namespace caongan
