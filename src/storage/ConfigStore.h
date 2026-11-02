#pragma once

#include "domain/Types.h"

#include <QString>
#include <QStringList>

namespace caongan {

class ConfigStore
{
public:
    enum class LoadStatus {
        Loaded,
        CreatedDefault,
        RecoveredFromBackup,
        Failed,
    };

    explicit ConfigStore(QString configPath = {});

    QString configPath() const { return m_configPath; }
    QString backupPath() const;
    bool exists() const;

    LoadStatus load(AppConfig &config, QStringList *messages = nullptr);
    bool save(const AppConfig &config, QString *error = nullptr);

private:
    bool readConfigFile(const QString &path, AppConfig &config, QStringList *errors) const;

    QString m_configPath;
};

} // namespace caongan
