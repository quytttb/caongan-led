# CI/CD — build, đóng gói và phát hành

Ba workflow **CI**, **Dev Build** và **Build Release** được tách riêng cho ứng dụng Qt Quick
desktop. Mỗi checkout lấy submodule đệ quy; Qt lấy phiên bản duy nhất từ
`packaging/qt_version.txt` (6.11.1). Actions và nguồn aqtinstall được ghim revision.

## CI

`.github/workflows/ci.yml` chạy khi push `main`/`master`, pull request, được gọi thủ công
hoặc được **Build Release** gọi lại với `source_sha`/`build_version`:

- **Linux**: cài Qt, build Release/Ninja, chạy CTest (bao gồm simulator và đường file → app),
  `qmllint` với `-W 0` nhưng tạm tắt toàn bộ nhóm `unqualified`, rồi chạy thử QML offscreen
  có `QT_FATAL_WARNINGS=1`. Sau đó build **DEB** tự chứa Qt, kiểm tra SHA-256 và chạy app
  chỉ với Qt trong gói; cài/gỡ gói trên Ubuntu 24.04 sạch trong Docker và chạy cả nền tảng
  `xcb` (Xvfb) lẫn `offscreen`. Preview thường và mô phỏng dùng vùng dữ liệu riêng; chỉ preview
  thường được yêu cầu tạo SQLite snapshot. Artifact `caongan-led-linux-<commit>` giữ 7 ngày.
- **Windows 2022 x64**: cài Qt, dùng Visual Studio DevShell để thiết lập MSVC (không phụ
  thuộc action Node 20), chạy `packaging/windows/build.ps1` (build Release và CTest), deploy
  bằng `windeployqt`, kiểm tra plugin Windows/SQLite, tạo ZIP + SHA-256 và chạy smoke test
  package ở cả chế độ thường và mô phỏng, kiểm tra SQLite riêng ở chế độ thường. Sau đó tạo **MSI** bằng WiX 3 (có sẵn trên runner), cài/gỡ im lặng trên runner
  cô lập và chạy lại smoke trên đúng thư mục đã cài. Artifact
  `caongan-led-windows-<commit>` giữ 7 ngày. Qt SQLite driver là bắt buộc, không được bỏ qua
  test khi plugin thiếu.

Các bước đóng gói đọc tool từ `.package-tools` (commit của workflow) trong khi mã ứng dụng
vẫn ở `source_sha`; nhờ vậy có thể đóng gói lại một tag cũ chưa có script MSI/DEB.

Không đặt secret, dữ liệu nguồn hay cấu hình vận hành trong Git hoặc artifact. Các file TT10
trong gói là mẫu giả. `qmllint` chưa kiểm tra nhóm `unqualified`; CI chạy xanh không chứng
minh màn thật đọc rõ, không thay thế máy Windows sạch, thiết bị nguồn hoặc thử 72 giờ.

## Dev Build

`.github/workflows/dev-build.yml` tự tạo artifact sau khi **CI của một lần push lên
`main`/`master` thành công**. Nó lấy đúng package theo commit và run CI, kiểm tra lại
SHA-256 rồi tải lên:

- `caongan-led-dev-windows-<commit>`: ZIP và MSI.
- `caongan-led-dev-linux-<commit>`: DEB.

Mỗi artifact kèm thông tin nguồn và giữ 7 ngày. Cách này không build trùng và không tạo bản
dev từ PR, CI lỗi hoặc CI bị hủy. Có thể chạy **Dev Build → Run workflow** với branch được
chọn: trường hợp này chạy lại CI cho đúng commit trước khi tạo artifact. Dev chỉ là kênh
thử nghiệm; executable vẫn được build CMake `Release` tối ưu, không phải bản `Debug`.

## Build Release

`.github/workflows/build-release.yml` chạy khi đẩy tag `vMAJOR.MINOR.PATCH` hoặc chọn
**Build Release → Run workflow**, nhập `tag` (ví dụ `v1.0.0`) và chọn branch/ref nguồn. Nếu
tag đã tồn tại, workflow luôn build commit của tag đó; nếu chưa có, build commit của ref đã
chọn và chỉ tạo tag sau khi CI thành công. Phiên bản ứng dụng và tên gói lấy từ tag, không
cần sửa CMake thủ công; CI kiểm tra cả phiên bản đã configure và phiên bản ứng dụng. Để
build lại cùng commit/tag thủ công, truyền `-DCAONGAN_BUILD_VERSION=1.0.0` cho CMake hoặc
`-Version '1.0.0'` cho script Windows.

Workflow kiểm tra tag không bị đổi trong lúc build, chạy toàn bộ CI của đúng commit, xác
minh đủ ba cặp package/checksum (ZIP, MSI, DEB) rồi **đăng GitHub Release công khai**, đánh
dấu bản mới nhất; không phải draft hoặc prerelease. Chạy bằng CLI:
`gh workflow run build-release.yml --ref main -f tag=v1.0.0`. Không cần token cá nhân: bước
phát hành dùng `GITHUB_TOKEN` với `contents: write` giới hạn cho job đó.

Chỉ người có thẩm quyền được tạo tag hoặc chạy phát hành; cần kiểm tra giấy phép, kết quả
Windows/LED và checklist **trước khi** đẩy tag hoặc chạy workflow, vì tag mới và release sẽ
được tạo công khai tự động. Việc push nhánh thông thường không tạo tag hay release.

## Gói phát hành

| Nền tảng | Tệp | Ghi chú |
|----------|-----|---------|
| Windows x64 | `caongan-led-<ver>-win-x64.zip` | Giải nén và chạy `caongan_led.exe`; không cần cài. |
| Windows x64 | `caongan-led-<ver>-win-x64.msi` | Cài per-machine vào `Program Files\Cao Ngan LED`, tạo shortcut Start Menu; cần quyền admin và Microsoft Visual C++ 2015-2022 x64 Redistributable. Gỡ bằng **Apps & features**; không xóa dữ liệu ứng dụng. |
| Linux amd64 | `caongan-led_<ver>_amd64.deb` | Tự chứa Qt trong `/opt/caongan-led`; cài bằng `sudo apt install ./caongan-led_<ver>_amd64.deb`; cần Ubuntu 24.04 trở lên. |

Mỗi tệp có `.sha256` đi kèm. Script không đóng gói hay cài Visual C++ Redistributable, không
ký MSI/DEB và chưa publish APT repository. Xem `packaging/windows/README.md` và
`packaging/linux/README.md` để build/kiểm tra cục bộ.
