#pragma once

#include <QString>

namespace caongan::AppPaths {

// Đường dẫn tuyệt đối, không phụ thuộc CWD. Mặc định nằm dưới AppLocalData
// của Windows; có thể override bằng biến môi trường khi test/portable.
QString baseDir();
QString dataDir();
QString configDir();
QString logDir();
QString databasePath();
QString configFilePath();
QString defaultSnapshotPath();

void ensureDirectories();

} // namespace caongan::AppPaths
