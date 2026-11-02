# Checklist nghiệm thu — chưa thực hiện

Ghi phiên bản/hash package, ngày giờ, người thử/duyệt, máy/Windows/GPU, tài khoản, màn/EDID/DPI, nguồn thử, log/ảnh và kết quả từng mục. Để trống khi chưa có bằng chứng; không đánh dấu đạt vì test giả lập.

## Trước tích hợp

- [ ] Nhà máy duyệt đường nhận/bản sao chỉ đọc, cơ chế hoàn tất upload và quyền; xác nhận không tranh inbox/move file với app cũ.
- [ ] Có file khí và nước thực đã loại thông tin nhạy cảm, station/alias/encoding/cycle/status/correction được ký duyệt. Nguồn pH có thật.
- [ ] Đối chiếu NH₄⁺ hay NH₄-N, PM mg/m³ hay mg/Nm³, NOx, đơn vị/cơ sở đo; có ngưỡng, precision và lời quy chuẩn được duyệt. Không suy từ PDF/ngưỡng mẫu.
- [ ] Các blocker tại `todo.md`, bao gồm shutdown UNC, quét phân trang, clock jump, checkpoint và giới hạn bộ nhớ/log, được xử lý hoặc có phương án đã phê duyệt và kiểm thử.

## Gói và dữ liệu

- [x] Windows Release build + CTest và smoke package trên GitHub-hosted runner ([run 41395509271](https://github.com/quytttb/caongan-led/actions/runs/41395509271)).
- [ ] Kiểm tra package/Qt license/MSVC và chạy trên máy sạch, đúng tài khoản; qwindows/SQLite/QML hoạt động và CWD khác không làm đổi dữ liệu.
- [ ] 12 kênh đúng vị trí, đối chiếu từng giá trị, đơn vị, timestamp và status với nguồn được phép. 0 thật hợp lệ; thiếu pH không thành 0; ngưỡng mặc định tắt; so trước làm tròn.
- [ ] Thử hiệu chuẩn, lỗi, unknown, stale, sai unit, conflict, future, gửi bù, file thiếu/sai cấu trúc, upload dở/locked/reconnect; không có lỗi nào trông như bình thường trong giới hạn. Nguồn không đổi tên/nội dung/mtime do app.
- [ ] Backup/rollback config, cache và package; config hỏng, DB lỗi, disk full, hai instance, reboot/crash; không xóa/move nguồn. Đo từ file hoàn tất/được phát hiện đến render tại tải danh định, mục tiêu ban đầu ≤5 giây.

## Màn hình và độ bền

- [ ] Tín hiệu 1920×1200 qua HDMI/quang, EDID và mapping trên vật lý 1280×960 mm đã đo/duyệt; không cắt, kéo giãn hay méo ngoài phương án phê duyệt.
- [ ] Đủ 12 ô không cuộn, Flow 6 chữ số/âm/số dài không elide; đọc rõ tiếng Việt và ký hiệu hóa học từ khoảng cách thực, cả ngày/đêm. Branding/footer được duyệt.
- [ ] Extend/Duplicate/1 màn, DPI 100/125/150%, mất/cắm lại LED, sleep/wake/reboot; LED không tự che cửa sổ vận hành khi mất màn. Kiểm tra không lộ popup vận hành lên LED.
- [ ] Soak ít nhất **72 giờ** trên máy đích/tương đương; ghi CPU, RAM, disk, log/DB, latency, lỗi và phục hồi nguồn. Pilot bản sao và phương án rollback có người chịu trách nhiệm.

Kết luận: **CHƯA NGHIỆM THU**. Người duyệt / ngày / ghi chú: ____________________
