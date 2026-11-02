# Kiến trúc và giới hạn hiện tại

`src/domain` chứa catalog cố định, parser TT10, mapping, chọn bản ghi, độ mới và đánh giá ngưỡng; không đặt quyết định tuân thủ trong QML. `src/ingestion` chạy trong một QThread: watcher root + quét định kỳ, chờ metadata ổn định, đọc chỉ đọc, parse/map rồi cập nhật snapshot. `src/storage` lưu JSON qua `QSaveFile` và SQLite cục bộ (`latest_reading`, checkpoint), transaction cho thay đổi/snapshot tương ứng. `src/presentation` cung cấp model, controller, diagnostics và mô phỏng; `src/app/qml` render hai cửa sổ.

- Chọn theo timestamp đo từng kênh; trùng cùng mốc/nội dung bỏ qua; khác nội dung cùng mốc đánh dấu conflict. Snapshot conflict giữ cờ khi restart; snapshot không khớp nguồn/trạm/root đang cấu hình không được nạp.
- Trạng thái thiết bị, độ mới, hợp lệ, ngưỡng độc lập. Chỉ dùng ngưỡng đang cấu hình khi bản ghi đo hợp lệ, mới và đúng đơn vị. Dữ liệu lỗi/thiếu/hiệu chuẩn/cũ/sai đơn vị/conflict không hiển thị là bình thường trong giới hạn.
- Ngưỡng trong cảnh mô phỏng là tạm thời; màn LED không có nhãn dữ liệu giả và nguồn tạm dừng xử lý khi bật. Cửa sổ vận hành báo nguồn đang tạm dừng; chuyển về thật sẽ quét lại. Chỉ dùng trên môi trường tách biệt.
- Màn được chọn theo tên `QScreen`, không theo chỉ số; nếu không thấy hoặc có nhiều màn cùng tên, LED bị ẩn và cảnh báo. `--preview` không cần màn đích. Tên màn không đảm bảo ổn định hoặc tương ứng EDID qua mọi cấu hình Windows.
- Bảo vệ file đang ghi bằng 2 quan sát metadata, đọc lại metadata sau đọc; điều này không chứng minh producer đã hoàn tất nếu producer tạm dừng. Phải xác nhận cơ chế rename hoàn tất của producer.

Giới hạn quan trọng: một lần quét duyệt tối đa 5.000 entry, hiện **không** phân trang phần vượt giới hạn; app báo lỗi nguồn khi bị cắt. Một batch xét tối đa 20 file ổn định, theo vòng; không đảm bảo độ trễ ≤5 giây khi có backlog. Một worker có thể bị I/O UNC chặn, và shutdown hiện chờ lệnh worker đồng bộ trước timeout: **không được coi là có timeout/cancel bảo đảm**. Xem [todo](todo.md) trước pilot.
