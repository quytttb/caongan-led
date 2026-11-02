#pragma once

#include "Types.h"

#include <QJsonObject>
#include <QStringList>

namespace caongan {

QJsonObject configToJson(const AppConfig &config);
bool configFromJson(const QJsonObject &object, AppConfig &config, QStringList *errors = nullptr);

// Validate toàn bộ cấu hình trước khi áp dụng/ghi. Không tự thay giá trị sai
// bằng mặc định: trả lỗi để người vận hành sửa.
QList<ConfigIssue> validateConfig(const AppConfig &config);
QStringList configIssuesToStrings(const QList<ConfigIssue> &issues);

} // namespace caongan
