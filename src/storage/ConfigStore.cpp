#include "ConfigStore.h"
#include "domain/ConfigCodec.h"
#include "domain/ParameterCatalog.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>

#include <utility>

namespace caongan {

ConfigStore::ConfigStore(QString configPath)
    : m_configPath(configPath.isEmpty() ? QStringLiteral("caongan-led.json") : std::move(configPath))
{
}

QString ConfigStore::backupPath() const
{
    return m_configPath + QStringLiteral(".backup");
}

bool ConfigStore::exists() const
{
    return QFileInfo::exists(m_configPath);
}

bool ConfigStore::readConfigFile(const QString &path, AppConfig &config, QStringList *errors) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errors)
            errors->append(QStringLiteral("Không mở được cấu hình %1: %2").arg(path, file.errorString()));
        return false;
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errors)
            errors->append(QStringLiteral("JSON cấu hình không hợp lệ tại %1: %2")
                               .arg(path, parseError.errorString()));
        return false;
    }

    QStringList parseErrors;
    AppConfig parsed;
    if (!configFromJson(doc.object(), parsed, &parseErrors)) {
        if (errors)
            errors->append(parseErrors);
        return false;
    }

    const QList<ConfigIssue> issues = validateConfig(parsed);
    if (!issues.isEmpty()) {
        if (errors)
            errors->append(configIssuesToStrings(issues));
        return false;
    }

    config = parsed;
    return true;
}

ConfigStore::LoadStatus ConfigStore::load(AppConfig &config, QStringList *messages)
{
    QStringList errors;
    AppConfig fileConfig;

    if (!exists()) {
        config = defaultAppConfig();
        QString saveError;
        if (!save(config, &saveError) && messages)
            messages->append(QStringLiteral("Không ghi được cấu hình mặc định: %1").arg(saveError));
        return LoadStatus::CreatedDefault;
    }

    if (readConfigFile(m_configPath, fileConfig, &errors)) {
        config = fileConfig;
        return LoadStatus::Loaded;
    }

    AppConfig backupConfig;
    QStringList backupErrors;
    if (readConfigFile(backupPath(), backupConfig, &backupErrors)) {
        config = backupConfig;
        if (messages) {
            messages->append(QStringLiteral("Cấu hình hiện tại lỗi, đã dùng bản backup hợp lệ gần nhất."));
            messages->append(errors);
        }
        return LoadStatus::RecoveredFromBackup;
    }

    if (messages) {
        messages->append(QStringLiteral("Cấu hình hiện tại và backup đều lỗi; ứng dụng chạy ở trạng thái an toàn."));
        messages->append(errors);
        messages->append(backupErrors);
    }
    config = defaultAppConfig();
    return LoadStatus::Failed;
}

bool ConfigStore::save(const AppConfig &config, QString *error)
{
    const QList<ConfigIssue> issues = validateConfig(config);
    if (!issues.isEmpty()) {
        if (error)
            *error = configIssuesToStrings(issues).join(QStringLiteral("; "));
        return false;
    }

    QDir().mkpath(QFileInfo(m_configPath).absolutePath());

    // Chỉ backup khi file hiện tại thực sự parse/validate được. QSaveFile
    // giữ backup cũ nếu ghi/commit thất bại, thay vì xóa nó trước khi copy.
    AppConfig current;
    QStringList ignored;
    if (exists() && readConfigFile(m_configPath, current, &ignored)) {
        const QByteArray backupJson = QJsonDocument(configToJson(current)).toJson(QJsonDocument::Indented);
        QSaveFile backup(backupPath());
        if (!backup.open(QIODevice::WriteOnly | QIODevice::Text)
            || backup.write(backupJson) != backupJson.size()
            || !backup.commit()) {
            if (error)
                *error = QStringLiteral("Không tạo được backup cấu hình tại %1: %2")
                             .arg(backupPath(), backup.errorString());
            return false;
        }
    }

    const QJsonDocument doc(configToJson(config));
    QSaveFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("Không mở được file cấu hình để ghi: %1").arg(file.errorString());
        return false;
    }
    const QByteArray json = doc.toJson(QJsonDocument::Indented);
    if (file.write(json) != json.size()) {
        if (error)
            *error = QStringLiteral("Ghi cấu hình thất bại: %1").arg(file.errorString());
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        if (error)
            *error = QStringLiteral("Không commit được cấu hình: %1").arg(file.errorString());
        return false;
    }
    return true;
}

} // namespace caongan
