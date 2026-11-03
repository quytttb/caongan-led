# Windows: build, đóng gói và triển khai thử

**Tình trạng: thử nghiệm.** Bản trước đã build/deploy trên Windows 2022 GitHub-hosted runner, nhưng smoke cũ không kiểm tra cửa sổ có thực sự hiện. Bản sửa cửa sổ và smoke mới chưa được chạy trên Windows; chưa kiểm tra trên Windows sạch hay PC/LED đích. Không cài tự động, không ghi registry/service, không cấu hình autologon hoặc sửa hệ thống CEMS. Chỉ tiếp tục sau khi kỹ thuật và nhà máy phê duyệt.

## Build

Chuẩn bị source đầy đủ, UI kit đúng revision `24ef01544e398e0e181933a71ee00d40f28c697e`, Qt **6.11.1 MSVC x64** có Qt Quick/Controls/Layouts/SQL/Test, Visual Studio Build Tools/MSVC phù hợp Qt, CMake ≥3.24 và Ninja. Mở **x64 Native Tools/Developer PowerShell**; các script dưới không yêu cầu admin và không tự cài dependency:

```powershell
.\packaging\windows\build.ps1 -QtRoot 'C:\Qt\6.11.1\msvc2022_64' -Version '1.0.0'
.\packaging\windows\deploy.ps1 -QtRoot 'C:\Qt\6.11.1\msvc2022_64' -Version '1.0.0'
```

Build script xác minh phiên bản Qt, revision và working tree kit nếu có Git, configure Ninja Release, build và CTest. `-Version` của build chọn phiên bản ứng dụng mà không sửa source; khi bỏ qua, dùng phiên bản đã lưu trong cache hoặc mặc định trong CMake nếu build mới. Deploy kiểm tra kit trùng cache build, dùng `windeployqt --qmldir` và build directory làm đường dẫn import, không copy compiler runtime; kiểm tra `platforms\qwindows.dll` và `sqldrivers\qsqlite.dll`, tạo thư mục version riêng + ZIP + SHA-256. `-Version` của deploy phải trùng `CMAKE_PROJECT_VERSION` trong cache; script từ chối ghi đè package/ZIP/checksum có sẵn. Nếu bước deploy thất bại, thư mục mới tạo có thể còn dở dang: kiểm tra rồi tự xóa **đúng thư mục đó** nếu được phép, hoặc chọn output root khác; script không tự xóa gì.

Script không chứng minh đủ mọi QML import/DLL hay quyền phân phối. Rà soát `THIRD_PARTY_NOTICES.md`, license Qt thực tế và MSVC Redistributable chính thức; chỉ cài redistributable khi được duyệt. Trên Windows sạch không cài Qt/dev tools, xác minh checksum ZIP, giải nén vào thư mục version có quyền đọc, thử với tài khoản vận hành và CWD khác, kiểm tra log/SQLite/QML/plugin, test 1/2 màn và DPI. Không chạy giả lập trên LED công khai như dữ liệu thật.

Sau deploy, có thể chạy `smoke.ps1 -PackageDir <thu-muc-package> -ScratchDir <thu-muc-moi>` trên Windows: script yêu cầu app xác nhận LED preview hiển thị toàn màn hình không viền, cửa sổ vận hành hiển thị không có title bar native rồi thoát, kiểm tra SQLite, dùng PATH không chứa Qt build. Thư mục scratch phải chưa tồn tại; script không xóa dữ liệu. CI dùng bước này nhưng nó không thay thế kiểm thử máy sạch, LED hay 72 giờ.

## Task Scheduler — chỉ cấu hình sau phê duyệt

`TaskScheduler.example.xml` là **mẫu chưa kiểm chứng**, chứa placeholder `REPLACE_WITH_DOMAIN\USERNAME` và đường dẫn, không import trực tiếp. Dùng Task Scheduler GUI để tạo task dưới đúng tài khoản vận hành:

1. Trigger **At log on** cho tài khoản này; chọn **Run only when user is logged on** (interactive token), least privilege. Kiểm tra chính sách màn hình khóa của nhà máy; không tự bật đăng nhập tự động.
2. Action trỏ tuyệt đối tới `caongan_led.exe` của phiên bản được duyệt; `Start in` là thư mục chứa exe. Không thêm `--preview`/`--simulate` vào task vận hành.
3. Không chạy nhiều instance; bỏ time limit, cấu hình restart sau lỗi theo chính sách, ví dụ 1 phút tối đa 3 lần. Task Scheduler không phát hiện mọi trường hợp treo; không coi restart-on-failure là watchdog.
4. Rà soát pin version, account/quyền/đường dẫn, xuất bản backup task cũ và chỉ tạo/bật task khi được phép. Thử logon/reboot/crash và ghi biên bản. Nếu dùng XML, thay mọi placeholder, kiểm tra encoding và schema trên Windows trước khi import.

## Backup và rollback

Dừng task và app; lưu riêng toàn bộ version hiện hành, JSON + `.backup`, thư mục SQLite cùng WAL/SHM (sau khi app đóng), log và định nghĩa task. Ghi version/checksum và tài khoản. Cài phiên bản thử ở **thư mục mới**, không ghi đè bản cũ; source dữ liệu luôn chỉ đọc. Khi rollback, dừng task mới, trỏ task về package cũ và phục hồi config/cache phù hợp đã được duyệt; kiểm tra LED/nguồn trước khi khởi động task. Không đụng app cũ/receiver/data-logger nếu chưa được phép. Xem thêm `docs/runbook.md` và `docs/acceptance-checklist.md`.
