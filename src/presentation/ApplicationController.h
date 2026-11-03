#pragma once

#include "domain/Types.h"
#include "storage/ConfigStore.h"

#include <QObject>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>
#include <QHash>
#include <QUrl>
#include <QWindow>

#include <memory>

class QScreen;

namespace caongan {

class DiagnosticsModel;
class IngestionService;
class LedParameterModel;

class ApplicationController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap branding READ branding NOTIFY configChanged)
    Q_PROPERTY(QVariantList sources READ sources NOTIFY configChanged)
    Q_PROPERTY(QVariantList channelSettings READ channelSettings NOTIFY configChanged)
    Q_PROPERTY(QVariantList screens READ screens NOTIFY screensChanged)
    Q_PROPERTY(QVariantMap sourceStatuses READ sourceStatuses NOTIFY sourceStatusesChanged)
    Q_PROPERTY(QString targetScreenName READ targetScreenName NOTIFY configChanged)
    Q_PROPERTY(QString screenWarning READ screenWarning NOTIFY screenWarningChanged)
    Q_PROPERTY(bool simulationActive READ simulationActive NOTIFY simulationChanged)
    Q_PROPERTY(bool previewMode READ previewMode CONSTANT)
    Q_PROPERTY(bool ledWindowVisible READ ledWindowVisible NOTIFY ledWindowChanged)
    Q_PROPERTY(QString exhaustCaption READ exhaustCaption NOTIFY statesChanged)
    Q_PROPERTY(QString wastewaterCaption READ wastewaterCaption NOTIFY statesChanged)
    Q_PROPERTY(QString statusSummary READ statusSummary NOTIFY statusSummaryChanged)
    Q_PROPERTY(QString configError READ configError NOTIFY configErrorChanged)
    Q_PROPERTY(QString configPath READ configPath NOTIFY configChanged)
    Q_PROPERTY(QString dataDir READ dataDir CONSTANT)
    Q_PROPERTY(QString logDir READ logDir CONSTANT)
    Q_PROPERTY(QString databasePath READ databasePath CONSTANT)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QStringList logFiles READ logFiles NOTIFY logFilesChanged)
    Q_PROPERTY(QString selectedLog READ selectedLog NOTIFY logViewChanged)
    Q_PROPERTY(QString logContent READ logContent NOTIFY logViewChanged)
    Q_PROPERTY(QString logViewNote READ logViewNote NOTIFY logViewChanged)

public:
    ApplicationController(LedParameterModel *ledModel,
                          DiagnosticsModel *diagnosticsModel,
                          QObject *parent = nullptr);
    ~ApplicationController() override;

    void initialize(const QString &configPathOverride,
                    bool previewMode,
                    bool simulate = false,
                    const QString &scenario = {});
    void shutdown();

    QVariantMap branding() const;
    QVariantList sources() const;
    QVariantList channelSettings() const;
    QVariantList screens() const;
    QVariantMap sourceStatuses() const { return m_sourceStatuses; }
    QString targetScreenName() const;
    QString screenWarning() const { return m_screenWarning; }
    bool simulationActive() const { return m_simulationActive; }
    bool previewMode() const { return m_previewMode; }
    bool ledWindowVisible() const { return m_ledWindowVisible; }
    QString exhaustCaption() const { return m_exhaustCaption; }
    QString wastewaterCaption() const { return m_wastewaterCaption; }
    QString statusSummary() const { return m_statusSummary; }
    QString configError() const { return m_configError; }
    QString configPath() const;
    QString dataDir() const;
    QString logDir() const;
    QString databasePath() const;
    QString appVersion() const;
    QStringList logFiles() const { return m_logFiles; }
    QString selectedLog() const { return m_selectedLog; }
    QString logContent() const { return m_logContent; }
    QString logViewNote() const { return m_logViewNote; }

    Q_INVOKABLE QString updateSource(const QString &id,
                                     const QString &folder,
                                     const QString &stationCode,
                                     int staleSeconds);
    Q_INVOKABLE QString updateTargetScreen(const QString &name);
    Q_INVOKABLE bool positionLedWindow(QWindow *window) const;
    Q_INVOKABLE QString reloadConfiguration();
    Q_INVOKABLE void requestRescan();
    Q_INVOKABLE void clearDiagnostics();
    Q_INVOKABLE void openLogFolder();
    Q_INVOKABLE void openDataFolder();
    Q_INVOKABLE QString refreshLogFiles();
    Q_INVOKABLE QString selectLog(const QString &fileName);
    Q_INVOKABLE QString copyLog();
    Q_INVOKABLE QString exportLog(const QUrl &destination);
    Q_INVOKABLE void quit();

signals:
    void configChanged();
    void screensChanged();
    void sourceStatusesChanged();
    void screenWarningChanged();
    void simulationChanged();
    void ledWindowChanged();
    void statesChanged();
    void statusSummaryChanged();
    void configErrorChanged();
    void logFilesChanged();
    void logViewChanged();

private slots:
    void onChannelsChanged(const QVector<caongan::ChannelState> &states);
    void onDiagnosticsBatch(const QVector<caongan::DiagnosticEntry> &entries);
    void onSourceStatusChanged(const QString &sourceId, bool available, const QString &message);
    void refreshScreens();

private:
    bool loadConfiguration(QStringList *messages);
    QString commitConfiguration(const AppConfig &next, bool reloadIngestion);
    void startIngestion();
    void sendConfigToIngestion();
    void updateWindowVisibility();
    void updateStatusSummary();
    void recomputeCaptions(const QVector<ChannelState> &states);
    void applySimulation();

    LedParameterModel *m_ledModel = nullptr;
    DiagnosticsModel *m_diagnosticsModel = nullptr;
    std::unique_ptr<ConfigStore> m_configStore;
    AppConfig m_config;
    IngestionService *m_ingestion = nullptr;
    QThread *m_ingestionThread = nullptr;
    QVector<ChannelState> m_lastStates;
    QVariantMap m_sourceStatuses;
    QHash<QString, QString> m_sourceMessages;
    QString m_exhaustCaption = QStringLiteral("Chưa có mốc đo");
    QString m_wastewaterCaption = QStringLiteral("Chưa có mốc đo");
    QString m_statusSummary = QStringLiteral("Đang khởi động");
    QString m_configError;
    QString m_screenWarning;
    QString m_simulationScenario;
    QStringList m_logFiles;
    QString m_selectedLog;
    QString m_logContent;
    QString m_logViewNote;
    bool m_previewMode = false;
    bool m_simulationActive = false;
    bool m_ledWindowVisible = false;
    bool m_initialized = false;
    bool m_shuttingDown = false;
};

} // namespace caongan
