#include "AppPaths.h"

#include <QDir>
#include <QStandardPaths>

namespace caongan::AppPaths {

namespace {

QString envOr(const char *name, const QString &fallback)
{
    const QString value = qEnvironmentVariable(name).trimmed();
    return value.isEmpty() ? fallback : QDir::cleanPath(value);
}

QString writableBase()
{
    const QString overrideBase = qEnvironmentVariable("CAONGAN_LED_BASE_DIR").trimmed();
    if (!overrideBase.isEmpty())
        return QDir::cleanPath(overrideBase);

    QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.isEmpty()) {
        base = QDir::homePath() + QStringLiteral("/.caongan-led");
    }
    return QDir::cleanPath(base);
}

} // namespace

QString baseDir()
{
    return writableBase();
}

QString dataDir()
{
    return envOr("CAONGAN_LED_DATA_DIR", baseDir() + QStringLiteral("/data"));
}

QString configDir()
{
    return envOr("CAONGAN_LED_CONFIG_DIR", baseDir() + QStringLiteral("/config"));
}

QString logDir()
{
    return envOr("CAONGAN_LED_LOG_DIR", baseDir() + QStringLiteral("/logs"));
}

QString databasePath()
{
    return dataDir() + QStringLiteral("/snapshot.db");
}

QString configFilePath()
{
    return configDir() + QStringLiteral("/caongan-led.json");
}

QString defaultSnapshotPath()
{
    return databasePath();
}

void ensureDirectories()
{
    QDir().mkpath(dataDir());
    QDir().mkpath(configDir());
    QDir().mkpath(logDir());
}

} // namespace caongan::AppPaths
