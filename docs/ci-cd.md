# CI/CD — gói Windows thử nghiệm

Workflow tham khảo cách tổ chức của `quytttb/data-logger`, điều chỉnh cho Qt Quick desktop và Windows thay vì tạo `.deb` Raspberry Pi. Mỗi checkout lấy submodule đệ quy; Qt lấy phiên bản duy nhất từ `packaging/qt_version.txt` (6.11.1), không kéo Graphs/Modbus/HTTP Server. Actions và nguồn aqtinstall được ghim revision.

## CI

`.github/workflows/ci.yml` chạy khi push `main`/`master`, pull request hoặc được gọi thủ công:

- **Linux**: cài Qt, build Release/Ninja, chạy CTest (bao gồm simulator và đường file → app), `qmllint` với `-W 0` nhưng tạm tắt toàn bộ nhóm `unqualified`, rồi chạy thử QML offscreen có `QT_FATAL_WARNINGS=1`.
- **Windows 2022 x64**: cài Qt/MSVC, chạy `packaging/windows/build.ps1` (build Release và CTest), deploy bằng `windeployqt`, kiểm tra plugin Windows/SQLite, tạo ZIP + SHA-256, khởi động package ở thư mục làm việc khác với Qt build bị loại khỏi PATH và kiểm tra SQLite cùng trạng thái visible của hai cửa sổ. Artifact `caongan-led-windows-<commit>` giữ 7 ngày. Qt SQLite driver là bắt buộc, không được bỏ qua test khi plugin thiếu.

Không đặt secret, dữ liệu nguồn hay cấu hình vận hành trong Git hoặc artifact. Các file TT10 trong gói là mẫu giả. `qmllint` chưa kiểm tra nhóm `unqualified`; CI chạy xanh không chứng minh màn thật đọc rõ, không thay thế máy Windows sạch, thiết bị nguồn hoặc thử 72 giờ.

Lần chạy tham chiếu ngày 02/11/2026: [CI 41395509271](https://github.com/quytttb/caongan-led/actions/runs/41395509271), commit `38c8ae7`; Linux và Windows đều build/test/deploy, nhưng smoke cũ không phát hiện lỗi cửa sổ bị ẩn. Test cửa sổ và smoke mới được bổ sung sau lần chạy này, chưa xác minh Windows. Artifact cũ có thời hạn, không phải bản phát hành chính thức và không dùng để kiểm tra giao diện.

## Bản nháp theo tag

`.github/workflows/build-release.yml` chỉ chạy khi đẩy tag `vMAJOR.MINOR.PATCH` trùng `project(... VERSION ...)` trong `CMakeLists.txt`. Nó chạy lại toàn bộ CI của đúng commit/tag, tải artifact Windows, xác minh SHA-256 và tạo **draft prerelease**, không tự xuất bản hay đánh dấu bản mới nhất. Không cần token cá nhân: bước tạo draft dùng `GITHUB_TOKEN` với `contents: write` giới hạn cho job đó.

Chỉ người có thẩm quyền được tạo tag, kiểm tra giấy phép, duyệt package, kết quả Windows/LED và checklist trước khi quyết định có xuất bản draft. Việc push nhánh thông thường không tạo tag hay release. Windows runtime trong CI đã có MSVC redistributable; script không đóng gói/cài redistributable, và gói chưa mặc nhiên đủ điều kiện phân phối.
