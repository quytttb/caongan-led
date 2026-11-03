# Cấu hình

Khi chưa có file, ứng dụng tạo JSON mặc định; vị trí chính xác xem cửa sổ **Chẩn đoán**. Thông thường Windows là dưới `%LOCALAPPDATA%\CaoNgan\CaonganLed\config\caongan-led.json`; database/log cùng vùng dữ liệu ứng dụng, không nằm trong Program Files. `--config` đổi đường dẫn JSON nhưng **không** đổi data/log/lock. Các biến môi trường `CAONGAN_LED_BASE_DIR`, `CAONGAN_LED_CONFIG_DIR`, `CAONGAN_LED_DATA_DIR`, `CAONGAN_LED_LOG_DIR` thay vị trí tương ứng; chỉ dùng theo quy trình quản trị.

Mẫu đầy đủ: `sample-data/caongan-led.example.json`. Sao chép để thử; không sửa bản mẫu trong gói. `schemaVersion` là 1; danh sách phải đủ 12 ID cố định, alias không được trùng giữa các kênh cùng nguồn. App validate trước khi lưu, dùng `QSaveFile`; trước một lần ghi hợp lệ, snapshot của bản chính hợp lệ cũ được ghi atomic sang `.backup`. Nếu chính hỏng mà backup hợp lệ, app dùng backup và báo lỗi; nếu cả hai hỏng, app báo lỗi và chạy với cấu hình mặc định an toàn, không dùng ngưỡng đã bị hỏng. Cần xử lý nguyên nhân thay vì coi cảnh báo là bình thường.

Các nhóm chính:

- `branding`: tên, chi nhánh, địa chỉ; không có form chỉnh thương hiệu. Footer màu cố định trong giao diện LED, không đọc các trường `footerNote`/`regulationText` cũ.
- `display`: `targetScreenName` là tên Qt, rỗng thì không mở LED công khai; bố cục logic 1600×960 theo tỉ lệ màn vật lý 1600×960 mm, không phải độ phân giải pixel đã xác minh. Các khung logic cũ 1920×1200 và 1920×1152 được đọc và chuyển sang bố cục mới. LED luôn toàn màn hình, không viền; `fullScreen` chỉ để tương thích. Tên màn không bảo đảm duy nhất hay bất biến.
- `sources`: `id`, `folder` tuyệt đối hoặc rỗng, `stationCode`, `staleAfterSeconds` (mặc định 900), `futureToleranceSeconds` (120), `allowLegacyPrefix` (true), `groupedFiveColumn` (false), `recursive` và `maxDepth`. Mã `KHITO1`/`NUONT1` chỉ là ví dụ mặc định chưa xác nhận.
- `channels`: 12 ID/thứ tự tại catalog, `sourceId`/`stationCode`, `aliases`, `unit`, `acceptedUnits`, `decimals`, `staleAfterSeconds`. Dải đo và ngưỡng cố định trong catalog; trường `threshold` của cấu hình cũ được bỏ qua. Giá trị bằng cận vẫn trong khoảng; so sánh trước làm tròn. Đơn vị cấu hình phải tương thích với ngưỡng cố định. Thay timeout ở form nguồn cũng cập nhật timeout các kênh thuộc nguồn.
- `storage`: giới hạn checkpoint SQLite được dùng khi pruning; các trường `logRetentionDays`, `logMaxFileBytes`, `logMaxTotalBytes` hiện **chưa được nối với logger runtime**. Logger dùng mặc định trong mã; xem [todo](todo.md).

Form vận hành cho phép sửa thư mục, mã trạm, timeout nguồn và màn đích; bảng dải đo/ngưỡng chỉ đọc. Các trường như alias, đơn vị, grouped mode và recursive hiện không có đầy đủ UI; chỉ người có thẩm quyền sửa bản sao JSON sau backup, xác minh mapping/đơn vị và test lại. App không có RBAC riêng: bảo vệ cửa sổ và file bằng quyền tài khoản Windows.

**Lưu ý checkpoint:** thay profile/alias/mapping rồi reload không bảo đảm file cũ cùng size/mtime được parse lại, kể cả đã bấm quét. Hãy xác minh bằng file thử mới và quy trình cache riêng đã phê duyệt; không tự xóa database vận hành để ép đọc. Xem [todo](todo.md).
