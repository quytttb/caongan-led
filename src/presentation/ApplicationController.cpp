#include "ApplicationController.h"

#include "DiagnosticsModel.h"
#include "LedParameterModel.h"
#include "SimulationFactory.h"
#include "domain/ConfigCodec.h"
#include "domain/DisplayState.h"
#include "domain/ParameterCatalog.h"
#include "domain/Tt10Parser.h"
#include "domain/Tt10Time.h"
#include "ingestion/IngestionService.h"
#include "utils/AppPaths.h"
#include "utils/LogSetup.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMetaObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScreen>
#include <QUrl>

#include <utility>

namespace caongan {

namespace {
constexpr qint64 kLogPreviewBytes = 1024 * 1024;
constexpr qint64 kLogExportMaxBytes = 64 * 1024 * 1024;

QString canonicalDestination(const QString &path)
{
    const QFileInfo info(path);
    const QString parent = info.dir().canonicalPath();
    return parent.isEmpty() ? info.absoluteFilePath() : QDir(parent).filePath(info.fileName());
}

bool sameDestination(const QString &first, const QString &second)
{
#ifdef Q_OS_WIN
    constexpr Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
#else
    constexpr Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#endif
    return QString::compare(QDir::cleanPath(canonicalDestination(first)),
                            QDir::cleanPath(canonicalDestination(second)), sensitivity) == 0;
}

bool isWithinDirectory(const QString &filePath, const QString &directory)
{
    if (directory.trimmed().isEmpty())
        return false;
    const QString root = QDir(directory).canonicalPath();
    if (root.isEmpty())
        return false;
    const QString relative = QDir(root).relativeFilePath(filePath);
    return !QDir::isAbsolutePath(relative) && relative != QLatin1String("..")
           && !relative.startsWith(QLatin1String("../"));
}

QList<QScreen *> matchingScreens(const QString &name)
{
    QList<QScreen *> matches;
    if (name.isEmpty())
        return matches;
    for (QScreen *screen : QGuiApplication::screens()) {
        if (screen && screen->name() == name)
            matches.append(screen);
    }
    return matches;
}
} // namespace

ApplicationController::ApplicationController(LedParameterModel *ledModel,
                                             DiagnosticsModel *diagnosticsModel,
                                             QObject *parent)
    : QObject(parent)
    , m_ledModel(ledModel)
    , m_diagnosticsModel(diagnosticsModel)
{
}

ApplicationController::~ApplicationController()
{
    shutdown();
}

void ApplicationController::initialize(const QString &configPathOverride,
                                       bool previewMode,
                                       bool simulate,
                                       const QString &scenario)
{
    if (m_initialized)
        return;
    m_initialized = true;
    m_previewMode = previewMode;

    AppPaths::ensureDirectories();

    const QString path = configPathOverride.trimmed().isEmpty()
        ? AppPaths::configFilePath()
        : configPathOverride;
    m_configStore = std::make_unique<ConfigStore>(path);
    qInfo() << "[controller] Khởi tạo với cấu hình" << path << "preview" << previewMode;

    QStringList messages;
    loadConfiguration(&messages);
    m_ledModel->setConfig(m_config);

    connect(qGuiApp, &QGuiApplication::screenAdded, this, &ApplicationController::refreshScreens);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &ApplicationController::refreshScreens);
    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, &ApplicationController::refreshScreens);

    startIngestion();
    updateWindowVisibility();
    updateStatusSummary();

    if (simulate)
        setSimulation(true, scenario.isEmpty() ? simulationScenarioNames().value(0) : scenario);
}

void ApplicationController::shutdown()
{
    if (m_shuttingDown)
        return;
    m_shuttingDown = true;

    if (m_ingestion && m_ingestionThread && m_ingestionThread->isRunning()) {
        // Gọi shutdown đồng bộ để SQLite/QTimer/watcher được đóng đúng thread
        // trước khi event loop dừng hẳn.
        if (QThread::currentThread() == m_ingestionThread) {
            m_ingestion->shutdown();
        } else {
            QMetaObject::invokeMethod(m_ingestion, "shutdown", Qt::BlockingQueuedConnection);
        }
        m_ingestionThread->quit();
        if (!m_ingestionThread->wait(10000)) {
            // Không xóa QThread đang chạy (undefined behavior). Bỏ parent và để
            // tiến trình kết thúc; đây là đường thoát khẩn cấp cuối cùng.
            qWarning() << "[controller] Ingestion thread không dừng trong 10 giây — thoát tiến trình";
            m_ingestion = nullptr;
            m_ingestionThread->setParent(nullptr);
            m_ingestionThread = nullptr;
            return;
        }
    }

    if (m_ingestion) {
        delete m_ingestion;
        m_ingestion = nullptr;
    }
}

void ApplicationController::startIngestion()
{
    if (m_ingestionThread)
        return;

    qRegisterMetaType<AppConfig>("caongan::AppConfig");
    qRegisterMetaType<ChannelState>("caongan::ChannelState");
    qRegisterMetaType<QVector<ChannelState>>("QVector<caongan::ChannelState>");
    qRegisterMetaType<DiagnosticEntry>("caongan::DiagnosticEntry");
    qRegisterMetaType<QVector<DiagnosticEntry>>("QVector<caongan::DiagnosticEntry>");

    m_ingestionThread = new QThread(this);
    m_ingestionThread->setObjectName(QStringLiteral("CaonganLedIngestion"));
    m_ingestion = new IngestionService();
    m_ingestion->moveToThread(m_ingestionThread);

    connect(m_ingestion, &IngestionService::channelsChanged,
            this, &ApplicationController::onChannelsChanged);
    connect(m_ingestion, &IngestionService::diagnosticsBatch,
            this, &ApplicationController::onDiagnosticsBatch);
    connect(m_ingestion, &IngestionService::sourceStatusChanged,
            this, &ApplicationController::onSourceStatusChanged);

    m_ingestionThread->start();

    const AppConfig config = m_config;
    const QString snapshotPath = AppPaths::databasePath();
    IngestionService *service = m_ingestion;
    QMetaObject::invokeMethod(m_ingestion, [service, config, snapshotPath]() {
        service->start(config, snapshotPath);
    }, Qt::QueuedConnection);
}

void ApplicationController::sendConfigToIngestion()
{
    if (!m_ingestion)
        return;
    const AppConfig config = m_config;
    IngestionService *service = m_ingestion;
    QMetaObject::invokeMethod(m_ingestion, [service, config]() {
        service->reloadConfig(config);
    }, Qt::QueuedConnection);
}

bool ApplicationController::loadConfiguration(QStringList *messages)
{
    if (!m_configStore) {
        m_config = defaultAppConfig();
        return false;
    }

    QStringList localMessages;
    const ConfigStore::LoadStatus status = m_configStore->load(m_config, &localMessages);

    QString error;
    if (status == ConfigStore::LoadStatus::Failed) {
        error = localMessages.join(QStringLiteral("\n"));
    } else if (status == ConfigStore::LoadStatus::RecoveredFromBackup) {
        error = QStringLiteral("Cấu hình hiện tại lỗi — đang dùng backup hợp lệ gần nhất.\n")
                + localMessages.join(QStringLiteral("\n"));
    }

    if (m_configError != error) {
        m_configError = error;
        emit configErrorChanged();
    }
    if (messages)
        *messages = localMessages;
    return status != ConfigStore::LoadStatus::Failed;
}

QString ApplicationController::commitConfiguration(const AppConfig &next, bool reloadIngestion)
{
    if (!m_configStore)
        return QStringLiteral("Chưa khởi tạo cấu hình");

    AppConfig normalized = next;
    normalized.channels = orderChannels(normalized.channels);

    const QList<ConfigIssue> issues = validateConfig(normalized);
    if (!issues.isEmpty()) {
        writeApplicationLog(LogLevel::Warning,
                            QStringLiteral("[config] Cấu hình không hợp lệ: %1")
                                .arg(configIssuesToStrings(issues).join(QStringLiteral("; "))));
        return configIssuesToStrings(issues).join(QStringLiteral("\n"));
    }

    QString saveError;
    if (!m_configStore->save(normalized, &saveError)) {
        writeApplicationLog(LogLevel::Warning,
                            QStringLiteral("[config] Không lưu được cấu hình: %1").arg(saveError));
        return saveError;
    }

    qInfo() << "[config] Đã lưu cấu hình, áp dụng lại nguồn:" << reloadIngestion;

    m_config = normalized;
    if (m_configError != QString()) {
        m_configError.clear();
        emit configErrorChanged();
    }
    emit configChanged();

    if (reloadIngestion) {
        if (m_simulationActive)
            applySimulationScenario(m_simulationScenario);
        else
            sendConfigToIngestion();
    }
    updateWindowVisibility();
    updateStatusSummary();
    return {};
}

QVariantMap ApplicationController::branding() const
{
    QVariantMap map;
    map.insert(QStringLiteral("companyName"), m_config.branding.companyName);
    map.insert(QStringLiteral("branchName"), m_config.branding.branchName);
    map.insert(QStringLiteral("address"), m_config.branding.address);
    map.insert(QStringLiteral("regulationText"), m_config.branding.regulationText);
    map.insert(QStringLiteral("footerNote"), m_config.branding.footerNote);
    return map;
}

QVariantList ApplicationController::sources() const
{
    QVariantList list;
    for (const SourceConfig &source : m_config.sources) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), source.id);
        map.insert(QStringLiteral("name"), source.name);
        map.insert(QStringLiteral("folder"), source.folder);
        map.insert(QStringLiteral("stationCode"), source.stationCode);
        map.insert(QStringLiteral("staleSeconds"), source.staleAfterSeconds);
        map.insert(QStringLiteral("futureToleranceSeconds"), source.futureToleranceSeconds);
        const QVariantMap status = m_sourceStatuses.value(source.id).toMap();
        map.insert(QStringLiteral("available"), status.value(QStringLiteral("available"), false));
        map.insert(QStringLiteral("statusMessage"), status.value(QStringLiteral("message"), QString()));
        list.append(map);
    }
    return list;
}

QVariantList ApplicationController::channelSettings() const
{
    QVariantList list;
    for (const ChannelConfig &channel : orderChannels(m_config.channels)) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), channel.id);
        map.insert(QStringLiteral("symbol"), channel.symbol);
        map.insert(QStringLiteral("displayName"), channel.displayName);
        map.insert(QStringLiteral("group"), channelGroupId(channel.group));
        map.insert(QStringLiteral("groupTitle"), channelGroupTitle(channel.group));
        map.insert(QStringLiteral("unit"), channel.unit);
        map.insert(QStringLiteral("decimals"), channel.decimals);
        map.insert(QStringLiteral("sourceName"),
                   m_config.sourceById(channel.sourceId)
                       ? m_config.sourceById(channel.sourceId)->name
                       : QString());
        map.insert(QStringLiteral("thresholdEnabled"), channel.threshold.enabled);
        map.insert(QStringLiteral("lowText"),
                   channel.threshold.hasLow
                       ? formatReadingValue(channel.threshold.low, channel.decimals)
                       : QString());
        map.insert(QStringLiteral("highText"),
                   channel.threshold.hasHigh
                       ? formatReadingValue(channel.threshold.high, channel.decimals)
                       : QString());
        map.insert(QStringLiteral("staleSeconds"), channel.staleAfterSeconds);
        list.append(map);
    }
    return list;
}

QVariantList ApplicationController::screens() const
{
    QVariantList list;
    const QScreen *primary = QGuiApplication::primaryScreen();
    for (const QScreen *screen : QGuiApplication::screens()) {
        if (!screen)
            continue;
        QVariantMap map;
        const QString name = screen->name();
        const QRect geometry = screen->geometry();
        map.insert(QStringLiteral("name"), name);
        map.insert(QStringLiteral("label"),
                   QStringLiteral("%1  (%2×%3)%4")
                       .arg(name.isEmpty() ? QStringLiteral("Màn hình không tên") : name)
                       .arg(geometry.width())
                       .arg(geometry.height())
                       .arg(screen == primary ? QStringLiteral(" — màn chính") : QString()));
        map.insert(QStringLiteral("primary"), screen == primary);
        map.insert(QStringLiteral("width"), geometry.width());
        map.insert(QStringLiteral("height"), geometry.height());
        list.append(map);
    }
    return list;
}

QString ApplicationController::targetScreenName() const
{
    return m_config.display.targetScreenName;
}

bool ApplicationController::positionLedWindow(QWindow *window) const
{
    if (!window)
        return false;
    if (m_previewMode)
        return true;

    const QString target = m_config.display.targetScreenName.trimmed();
    if (target.isEmpty())
        return false;
    const QList<QScreen *> matches = matchingScreens(target);
    if (matches.size() != 1)
        return false;
    QScreen *screen = matches.constFirst();
    // Di chuyển trước khi hiện toàn màn hình, kể cả khi các màn nằm
    // trên cùng một virtual desktop (setScreen không tự dời tọa độ).
    window->setScreen(screen);
    window->setPosition(screen->geometry().topLeft());
    return true;
}

QStringList ApplicationController::simulationScenarios() const
{
    return simulationScenarioNames();
}

bool ApplicationController::ledFullScreen() const
{
    return !m_previewMode && m_config.display.fullScreen;
}

QString ApplicationController::configPath() const
{
    return m_configStore ? m_configStore->configPath() : AppPaths::configFilePath();
}

QString ApplicationController::dataDir() const
{
    return AppPaths::dataDir();
}

QString ApplicationController::logDir() const
{
    return AppPaths::logDir();
}

QString ApplicationController::databasePath() const
{
    return AppPaths::databasePath();
}

QString ApplicationController::appVersion() const
{
    return QCoreApplication::applicationVersion();
}

QString ApplicationController::updateSource(const QString &id,
                                            const QString &folder,
                                            const QString &stationCode,
                                            int staleSeconds)
{
    AppConfig next = m_config;
    bool found = false;
    for (SourceConfig &source : next.sources) {
        if (source.id != id)
            continue;
        found = true;
        const QString trimmedStation = stationCode.trimmed();
        if (trimmedStation.isEmpty())
            return QStringLiteral("Mã trạm không được để trống");
        if (staleSeconds < 10)
            return QStringLiteral("Stale timeout phải >= 10 giây");

        const QString trimmedFolder = folder.trimmed();
        if (!trimmedFolder.isEmpty() && !QDir::isAbsolutePath(trimmedFolder))
            return QStringLiteral("Thư mục nguồn phải là đường dẫn tuyệt đối");
        source.folder = trimmedFolder.isEmpty() ? QString() : QDir::cleanPath(trimmedFolder);
        if (source.folder == QLatin1String("."))
            source.folder.clear();
        source.stationCode = trimmedStation;
        source.staleAfterSeconds = staleSeconds;

        for (ChannelConfig &channel : next.channels) {
            if (channel.sourceId != id)
                continue;
            channel.stationCode = trimmedStation;
            channel.staleAfterSeconds = staleSeconds;
        }
        break;
    }
    if (!found)
        return QStringLiteral("Không tìm thấy nguồn %1").arg(id);

    return commitConfiguration(next, true);
}

QString ApplicationController::parseOptionalNumber(const QString &text, bool *present, double *value) const
{
    const QString trimmed = text.trimmed();
    *present = !trimmed.isEmpty();
    if (!*present)
        return {};
    if (!parseStrictNumber(trimmed, value))
        return QStringLiteral("Giá trị không phải số hợp lệ: %1").arg(trimmed);
    return {};
}

QString ApplicationController::buildThreshold(bool enabled,
                                              const QString &lowText,
                                              const QString &highText,
                                              Threshold *threshold) const
{
    if (!threshold)
        return QStringLiteral("Lỗi nội bộ: thiếu đích ngưỡng");

    Threshold built;
    built.enabled = enabled;
    if (enabled) {
        bool hasLow = false;
        bool hasHigh = false;
        double low = 0.0;
        double high = 0.0;
        const QString lowError = parseOptionalNumber(lowText, &hasLow, &low);
        if (!lowError.isEmpty())
            return lowError;
        const QString highError = parseOptionalNumber(highText, &hasHigh, &high);
        if (!highError.isEmpty())
            return highError;
        if (!hasLow && !hasHigh)
            return QStringLiteral("Ngưỡng đang bật nhưng chưa nhập cận dưới/cận trên");
        if (hasLow && hasHigh && low > high)
            return QStringLiteral("Ngưỡng dưới lớn hơn ngưỡng trên");
        built.hasLow = hasLow;
        built.hasHigh = hasHigh;
        built.low = low;
        built.high = high;
    }
    *threshold = built;
    return {};
}

QString ApplicationController::updateChannelThreshold(const QString &id,
                                                      bool enabled,
                                                      const QString &lowText,
                                                      const QString &highText)
{
    AppConfig next = m_config;
    ChannelConfig *channel = next.channelById(id);
    if (!channel)
        return QStringLiteral("Không tìm thấy kênh %1").arg(id);

    Threshold threshold;
    const QString error = buildThreshold(enabled, lowText, highText, &threshold);
    if (!error.isEmpty())
        return error;
    channel->threshold = threshold;
    return commitConfiguration(next, true);
}

QString ApplicationController::updateChannelDecimals(const QString &id, int decimals)
{
    if (decimals < 0 || decimals > 6)
        return QStringLiteral("Số chữ số thập phân phải trong 0..6");
    AppConfig next = m_config;
    ChannelConfig *channel = next.channelById(id);
    if (!channel)
        return QStringLiteral("Không tìm thấy kênh %1").arg(id);
    channel->decimals = decimals;
    return commitConfiguration(next, true);
}

QString ApplicationController::updateChannelSettings(const QString &id,
                                                     int decimals,
                                                     bool thresholdEnabled,
                                                     const QString &lowText,
                                                     const QString &highText)
{
    if (decimals < 0 || decimals > 6)
        return QStringLiteral("Số chữ số thập phân phải trong 0..6");

    AppConfig next = m_config;
    ChannelConfig *channel = next.channelById(id);
    if (!channel)
        return QStringLiteral("Không tìm thấy kênh %1").arg(id);

    Threshold threshold;
    const QString error = buildThreshold(thresholdEnabled, lowText, highText, &threshold);
    if (!error.isEmpty())
        return error;

    channel->decimals = decimals;
    channel->threshold = threshold;
    return commitConfiguration(next, true);
}

QString ApplicationController::updateBranding(const QString &companyName,
                                              const QString &branchName,
                                              const QString &address,
                                              const QString &regulationText,
                                              const QString &footerNote)
{
    AppConfig next = m_config;
    next.branding.companyName = companyName.trimmed();
    next.branding.branchName = branchName.trimmed();
    next.branding.address = address.trimmed();
    next.branding.regulationText = regulationText.trimmed();
    next.branding.footerNote = footerNote.trimmed();
    return commitConfiguration(next, false);
}

QString ApplicationController::updateTargetScreen(const QString &name)
{
    return updateDisplay(name, m_config.display.fullScreen);
}

QString ApplicationController::updateDisplay(const QString &targetScreenName, bool fullScreen)
{
    const QString name = targetScreenName.trimmed();
    if (!name.isEmpty()) {
        const qsizetype count = matchingScreens(name).size();
        if (count == 0)
            return QStringLiteral("Màn hình “%1” không còn kết nối").arg(name);
        if (count > 1)
            return QStringLiteral("Có nhiều màn hình cùng tên “%1”; không thể chọn an toàn").arg(name);
    }

    const bool fullScreenChanged = (m_config.display.fullScreen != fullScreen);
    AppConfig next = m_config;
    next.display.targetScreenName = name;
    next.display.fullScreen = fullScreen;

    const QString error = commitConfiguration(next, false);
    if (error.isEmpty() && fullScreenChanged) {
        // Cùng một output nhưng đổi windowed/full-screen vẫn phải cập nhật cửa
        // sổ LED; updateWindowVisibility() không phát khi visibility không đổi.
        emit ledWindowChanged();
        updateWindowVisibility();
    }
    return error;
}

QString ApplicationController::reloadConfiguration()
{
    QStringList messages;
    const bool ok = loadConfiguration(&messages);
    m_ledModel->setConfig(m_config);
    updateWindowVisibility();
    sendConfigToIngestion();
    if (m_simulationActive)
        applySimulationScenario(m_simulationScenario);
    emit configChanged();
    if (!ok)
        return m_configError;
    return messages.join(QStringLiteral("\n"));
}

void ApplicationController::requestRescan()
{
    if (!m_ingestion)
        return;
    IngestionService *service = m_ingestion;
    QMetaObject::invokeMethod(m_ingestion, [service]() { service->requestRescan(); },
                              Qt::QueuedConnection);
}

void ApplicationController::setSimulation(bool active, const QString &scenario)
{
    if (!active) {
        if (!m_simulationActive)
            return;
        qInfo() << "[controller] Kết thúc mô phỏng, tiếp tục nhập nguồn";
        m_simulationActive = false;
        m_simulationScenario.clear();
        m_ledModel->setConfig(m_config);
        m_lastStates.clear();
        m_exhaustCaption = QStringLiteral("Chưa có mốc đo");
        m_wastewaterCaption = QStringLiteral("Chưa có mốc đo");
        updateStatusSummary();
        emit simulationChanged();
        emit statesChanged();

        if (m_ingestion) {
            IngestionService *service = m_ingestion;
            QMetaObject::invokeMethod(m_ingestion, [service]() {
                service->setPaused(false);
                service->requestRescan();
            }, Qt::QueuedConnection);
        }
        return;
    }

    const QStringList names = simulationScenarioNames();
    const QString chosen = names.contains(scenario) ? scenario : names.value(0);
    qInfo() << "[controller] Bắt đầu kịch bản mô phỏng" << chosen << "; tạm dừng nhập nguồn";
    m_simulationActive = true;
    m_simulationScenario = chosen;

    if (m_ingestion) {
        IngestionService *service = m_ingestion;
        QMetaObject::invokeMethod(m_ingestion, [service]() { service->setPaused(true); },
                                  Qt::QueuedConnection);
    }

    const QVector<ChannelState> states = buildSimulationStates(
        m_config, chosen, tt10::nowUtc());
    m_lastStates = states;
    m_ledModel->setStates(states);
    recomputeCaptions(states);
    updateStatusSummary();
    emit simulationChanged();
    emit statesChanged();
}

void ApplicationController::applySimulationScenario(const QString &scenario)
{
    if (!m_simulationActive) {
        setSimulation(true, scenario);
        return;
    }
    setSimulation(true, scenario);
}

void ApplicationController::clearDiagnostics()
{
    m_diagnosticsModel->clear();
}

void ApplicationController::openLogFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(AppPaths::logDir()));
}

void ApplicationController::openDataFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(AppPaths::dataDir()));
}

QString ApplicationController::refreshLogFiles()
{
    static const QRegularExpression allowed(
        QStringLiteral("^caongan-led-\\d{8}\\.log(?:\\.[1-5])?$"));
    QStringList files;
    const auto entries = QDir(AppPaths::logDir()).entryInfoList(
        {QStringLiteral("caongan-led-*.log*")}, QDir::Files | QDir::NoSymLinks, QDir::Time);
    for (const QFileInfo &entry : entries) {
        if (allowed.match(entry.fileName()).hasMatch())
            files.append(entry.fileName());
    }
    if (files != m_logFiles) {
        m_logFiles = files;
        emit logFilesChanged();
    }

    if (files.isEmpty()) {
        m_selectedLog.clear();
        m_logContent.clear();
        m_logViewNote = QStringLiteral("Chưa có tệp log.");
        emit logViewChanged();
        return {};
    }
    const QString currentName = QFileInfo(currentLogFilePath()).fileName();
    const QString selected = files.contains(m_selectedLog) ? m_selectedLog
        : (files.contains(currentName) ? currentName : files.constFirst());
    return selectLog(selected);
}

QString ApplicationController::selectLog(const QString &fileName)
{
    const QFileInfo info(QDir(AppPaths::logDir()).filePath(fileName));
    if (!m_logFiles.contains(fileName) || !info.isFile() || info.isSymLink())
        return QStringLiteral("Tệp log không tồn tại hoặc không được phép truy cập.");

    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return QStringLiteral("Không đọc được log: %1").arg(file.errorString());

    const bool truncated = file.size() > kLogPreviewBytes;
    if (truncated) {
        if (!file.seek(file.size() - kLogPreviewBytes))
            return QStringLiteral("Không tìm được vị trí đọc log: %1").arg(file.errorString());
        file.readLine(kLogPreviewBytes); // Bỏ dòng bị cắt ở đầu, kể cả UTF-8 giữa ký tự.
    }
    const QByteArray content = file.read(kLogPreviewBytes);
    if (file.error() != QFileDevice::NoError)
        return QStringLiteral("Không đọc được log: %1").arg(file.errorString());

    m_selectedLog = fileName;
    m_logContent = QString::fromUtf8(content);
    m_logViewNote = truncated
        ? QStringLiteral("Đang xem tối đa 1 MiB cuối; sao chép phần đang xem, xuất toàn bộ tệp.")
        : QStringLiteral("Sao chép phần đang xem hoặc xuất toàn bộ tệp.");
    emit logViewChanged();
    return {};
}

QString ApplicationController::copyLog()
{
    if (m_logContent.isEmpty())
        return QStringLiteral("Không có nội dung log để sao chép.");
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return QStringLiteral("Không truy cập được clipboard.");
    clipboard->setText(m_logContent);
    return {};
}

QString ApplicationController::exportLog(const QUrl &destination)
{
    if (!destination.isLocalFile() || destination.toLocalFile().isEmpty())
        return QStringLiteral("Hãy chọn một tệp đích trên máy.");
    if (!m_logFiles.contains(m_selectedLog))
        return QStringLiteral("Hãy chọn tệp log trước khi xuất.");

    const QFileInfo sourceInfo(QDir(AppPaths::logDir()).filePath(m_selectedLog));
    if (!sourceInfo.isFile() || sourceInfo.isSymLink())
        return QStringLiteral("Tệp log đã thay đổi hoặc không còn tồn tại.");
    QFile source(sourceInfo.absoluteFilePath());
    if (!source.open(QIODevice::ReadOnly))
        return QStringLiteral("Không đọc được log: %1").arg(source.errorString());
    const qint64 size = source.size();
    if (size > kLogExportMaxBytes)
        return QStringLiteral("Tệp log lớn hơn giới hạn xuất 64 MiB.");

    const QFileInfo targetInfo(destination.toLocalFile());
    const QString parent = targetInfo.dir().canonicalPath();
    if (parent.isEmpty() || targetInfo.isDir() || targetInfo.isSymLink())
        return QStringLiteral("Thư mục đích không hợp lệ hoặc tệp đích là liên kết.");
    const QString target = QDir(parent).filePath(targetInfo.fileName());
    if (isWithinDirectory(target, AppPaths::logDir())
        || isWithinDirectory(target, AppPaths::dataDir())
        || isWithinDirectory(target, AppPaths::configDir())
        || sameDestination(target, configPath())
        || sameDestination(target, configPath() + QStringLiteral(".backup"))) {
        return QStringLiteral("Không xuất đè vào thư mục dữ liệu, cấu hình hoặc log của ứng dụng.");
    }
    for (const SourceConfig &config : m_config.sources) {
        if (isWithinDirectory(target, config.folder))
            return QStringLiteral("Không xuất vào thư mục nguồn chỉ đọc.");
    }

    QSaveFile output(target);
    if (!output.open(QIODevice::WriteOnly))
        return QStringLiteral("Không tạo được tệp xuất: %1").arg(output.errorString());
    qint64 remaining = size;
    while (remaining > 0) {
        const QByteArray chunk = source.read(qMin<qint64>(remaining, 64 * 1024));
        if (chunk.isEmpty() || output.write(chunk) != chunk.size())
            return QStringLiteral("Không xuất được log: tệp nguồn thay đổi hoặc lỗi ghi.");
        remaining -= chunk.size();
    }
    if (!output.commit())
        return QStringLiteral("Không lưu được tệp xuất: %1").arg(output.errorString());
    qInfo() << "[log] Đã xuất" << m_selectedLog << "số byte" << size << "đến" << target;
    return {};
}

void ApplicationController::quit()
{
    QCoreApplication::quit();
}

void ApplicationController::onChannelsChanged(const QVector<caongan::ChannelState> &states)
{
    if (m_simulationActive)
        return;
    m_lastStates = states;
    m_ledModel->setStates(states);
    recomputeCaptions(states);
    updateStatusSummary();
    emit statesChanged();
}

void ApplicationController::onDiagnosticsBatch(const QVector<caongan::DiagnosticEntry> &entries)
{
    m_diagnosticsModel->appendBatch(entries);
}

void ApplicationController::onSourceStatusChanged(const QString &sourceId,
                                                  bool available,
                                                  const QString &message)
{
    QVariantMap status;
    const SourceConfig *source = m_config.sourceById(sourceId);
    status.insert(QStringLiteral("name"), source ? source->name : sourceId);
    status.insert(QStringLiteral("available"), available);
    status.insert(QStringLiteral("message"), message);
    m_sourceStatuses.insert(sourceId, status);
    m_sourceMessages.insert(sourceId, message);
    updateStatusSummary();
    // Chỉ phát trạng thái nguồn: form cấu hình không được reset khi watcher
    // cập nhật thông điệp "đang theo dõi" vài giây một lần.
    emit sourceStatusesChanged();
}

void ApplicationController::refreshScreens()
{
    emit screensChanged();
    updateWindowVisibility();
}

void ApplicationController::updateWindowVisibility()
{
    if (m_previewMode) {
        if (!m_ledWindowVisible) {
            m_ledWindowVisible = true;
            emit ledWindowChanged();
        }
        if (!m_screenWarning.isEmpty()) {
            m_screenWarning.clear();
            emit screenWarningChanged();
        }
        return;
    }

    bool visible = false;
    QString warning;
    const QString target = m_config.display.targetScreenName.trimmed();
    if (target.isEmpty()) {
        warning = QStringLiteral("Chưa chọn màn hình LED trong cửa sổ vận hành.");
    } else {
        const qsizetype count = matchingScreens(target).size();
        if (count == 1)
            visible = true;
        else if (count > 1)
            warning = QStringLiteral("Có nhiều màn hình cùng tên “%1” — không thể chọn an toàn.").arg(target);
        else
            warning = QStringLiteral("Màn hình “%1” không còn kết nối — không tự dời màn công khai.").arg(target);
    }

    const bool changed = (visible != m_ledWindowVisible) || (warning != m_screenWarning);
    m_ledWindowVisible = visible;
    m_screenWarning = warning;
    if (changed) {
        emit ledWindowChanged();
        emit screenWarningChanged();
    }
}

void ApplicationController::updateStatusSummary()
{
    QString summary;
    if (m_simulationActive) {
        summary = QStringLiteral("Đã tạm dừng nhập nguồn");
    } else if (m_config.sources.isEmpty()) {
        summary = QStringLiteral("Chưa cấu hình nguồn dữ liệu");
    } else {
        int online = 0;
        for (const SourceConfig &source : m_config.sources) {
            if (m_sourceStatuses.value(source.id).toMap().value(QStringLiteral("available"), false).toBool())
                ++online;
        }
        summary = QStringLiteral("Đang chạy • %1/%2 nguồn sẵn sàng")
                      .arg(online).arg(m_config.sources.size());
    }
    if (summary != m_statusSummary) {
        m_statusSummary = summary;
        emit statusSummaryChanged();
    }
}

void ApplicationController::recomputeCaptions(const QVector<ChannelState> &states)
{
    auto captionFor = [&states](ChannelGroup group) -> QString {
        QDateTime min;
        QDateTime max;
        for (const ChannelState &state : states) {
            if (state.config.group != group || !state.hasReading)
                continue;
            if (!state.reading.measuredAtUtc.isValid())
                continue;
            if (!min.isValid() || state.reading.measuredAtUtc < min)
                min = state.reading.measuredAtUtc;
            if (!max.isValid() || state.reading.measuredAtUtc > max)
                max = state.reading.measuredAtUtc;
        }
        if (!min.isValid())
            return QStringLiteral("Chưa có mốc đo");
        if (min == max) {
            return QStringLiteral("Mốc đo: %1")
                .arg(tt10::formatLocalDisplay(min, QStringLiteral("HH:mm:ss dd/MM/yyyy")));
        }
        return QStringLiteral("Mốc đo không đồng bộ: %1 – %2")
            .arg(tt10::formatLocalDisplay(min, QStringLiteral("HH:mm:ss")),
                 tt10::formatLocalDisplay(max, QStringLiteral("HH:mm:ss dd/MM/yyyy")));
    };

    const QString exhaust = captionFor(ChannelGroup::Exhaust);
    const QString wastewater = captionFor(ChannelGroup::Wastewater);
    if (exhaust != m_exhaustCaption) {
        m_exhaustCaption = exhaust;
        emit statesChanged();
    }
    if (wastewater != m_wastewaterCaption) {
        m_wastewaterCaption = wastewater;
        emit statesChanged();
    }
}

} // namespace caongan
