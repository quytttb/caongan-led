# CI/CD — build và phát hành Windows

Ba workflow **CI**, **Dev Build** và **Build Release** được tách riêng cho ứng dụng Qt Quick desktop. Mỗi checkout lấy submodule đệ quy; Qt lấy phiên bản duy nhất từ `packaging/qt_version.txt` (6.11.1), không kéo Graphs/Modbus/HTTP Server. Actions và nguồn aqtinstall được ghim revision.

## CI

`.github/workflows/ci.yml` chạy khi push `main`/`master`, pull request hoặc được gọi thủ công:

- **Linux**: cài Qt, build Release/Ninja, chạy CTest (bao gồm simulator và đường file → app), `qmllint` với `-W 0` nhưng tạm tắt toàn bộ nhóm `unqualified`, rồi chạy thử QML offscreen có `QT_FATAL_WARNINGS=1`.
- **Windows 2022 x64**: cài Qt, dùng Visual Studio DevShell để thiết lập MSVC (không phụ thuộc action Node 20), chạy `packaging/windows/build.ps1` (build Release và CTest), deploy bằng `windeployqt`, kiểm tra plugin Windows/SQLite, tạo ZIP + SHA-256 và chạy smoke test package. Artifact `caongan-led-windows-<commit>` giữ 7 ngày. Qt SQLite driver là bắt buộc, không được bỏ qua test khi plugin thiếu.

Không đặt secret, dữ liệu nguồn hay cấu hình vận hành trong Git hoặc artifact. Các file TT10 trong gói là mẫu giả. `qmllint` chưa kiểm tra nhóm `unqualified`; CI chạy xanh không chứng minh màn thật đọc rõ, không thay thế máy Windows sạch, thiết bị nguồn hoặc thử 72 giờ.

Lần chạy tham chiếu: [CI 41548568987](https://github.com/quytttb/caongan-led/actions/runs/41548568987), commit `27b4024`; Linux và Windows đều build/test, Windows đã deploy và chạy smoke mới. Chưa thay thế kiểm tra trên máy Windows sạch hay PC/LED đích.

## Dev Build

`.github/workflows/dev-build.yml` tự tạo artifact sau khi **CI của một lần push lên `main`/`master` thành công**. Nó lấy đúng package theo commit và run CI, kiểm tra lại SHA-256 rồi tải lên artifact `caongan-led-dev-windows-<commit>`, kèm thông tin nguồn; giữ 7 ngày. Cách này không build trùng và không tạo bản dev từ PR, CI lỗi hoặc CI bị hủy.

Có thể chạy **Dev Build → Run workflow** với branch được chọn: trường hợp này chạy lại CI cho đúng commit trước khi tạo artifact. Dev chỉ là kênh thử nghiệm; executable vẫn được build CMake `Release` tối ưu, không phải bản `Debug`.

## Build Release

`.github/workflows/build-release.yml` chạy khi đẩy tag `vMAJOR.MINOR.PATCH` hoặc chọn **Build Release → Run workflow**, nhập `tag` (ví dụ `v1.0.0`) và chọn branch/ref nguồn. Nếu tag đã tồn tại, workflow luôn build commit của tag đó; nếu chưa có, build commit của ref đã chọn và chỉ tạo tag sau khi CI thành công. Phiên bản ứng dụng và tên gói lấy từ tag, không cần sửa CMake thủ công; CI kiểm tra cả phiên bản đã configure và phiên bản ứng dụng. Để build lại cùng commit/tag thủ công, truyền `-DCAONGAN_BUILD_VERSION=1.0.0` cho CMake hoặc `-Version '1.0.0'` cho script Windows. Workflow kiểm tra tag không bị đổi trong lúc build, chạy toàn bộ CI của đúng commit, xác minh SHA-256 rồi **đăng GitHub Release công khai**, đánh dấu bản mới nhất; không phải draft hoặc prerelease. Chạy bằng CLI: `gh workflow run build-release.yml --ref main -f tag=v1.0.0`. Không cần token cá nhân: bước phát hành dùng `GITHUB_TOKEN` với `contents: write` giới hạn cho job đó. Luồng phát hành theo tag chưa được chạy end-to-end.

Chỉ người có thẩm quyền được tạo tag hoặc chạy phát hành; cần kiểm tra giấy phép, kết quả Windows/LED và checklist **trước khi** đẩy tag hoặc chạy workflow, vì tag mới và release sẽ được tạo công khai tự động. Việc push nhánh thông thường không tạo tag hay release. Windows runtime trong CI đã có MSVC redistributable; script không đóng gói/cài redistributable, và gói chưa mặc nhiên đủ điều kiện phân phối.
