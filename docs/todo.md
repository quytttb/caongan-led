# Việc còn lại và rủi ro — chặn vận hành

## Kỹ thuật cần hoàn thiện

- [ ] Phân trang/tiếp tục quét thực sự khi cây nguồn vượt 5.000 entry; hiện chỉ cảnh báo và có thể bỏ sót lâu dài. Đo backlog, công bằng, độ trễ ≤5 giây ở tải danh định và độ lớn phù hợp. Kiểm thử cây thư mục ngày mới, đổi tên, ghi dở, khóa và permission trên Windows.
- [ ] Thiết kế I/O UNC có timeout/cancel; `ApplicationController::shutdown()` hiện dùng `BlockingQueuedConnection` **trước** timeout chờ thread, có thể treo vô hạn khi I/O bị chặn. Cần cô lập nguồn để một nguồn mất kết nối không chặn nguồn khác và UI, kiểm thử reconnect/thử hủy thật.
- [ ] Checkpoint dựa trên đường dẫn/size/mtime; thay config/profile hoặc sửa file nhưng giữ size/mtime có thể không được đọc lại. Thiết kế version/hash cấu hình và cơ chế reprocess an toàn có giới hạn; xác minh thay mapping, alias, root và restart. Snapshot đã lọc root/trạm khi phục hồi nhưng việc chuyển về root cũ không bảo đảm đọc lại các file cũ đã checkpoint.
- [ ] Theo dõi clock monotonic/nhảy giờ: hiện freshness dựa vào đồng hồ hệ thống hiện tại; chỉnh giờ lùi có thể làm số cũ thành mới. Test NTP/jump và xử lý an toàn trước pilot.
- [ ] Kiểm thử giới hạn bộ nhớ theo thời gian: bộ nhớ theo file được cắt theo tập ứng viên mỗi lần quét, nhưng chưa đo được tính ổn định thực tế hoặc hành vi khi cây bị cắt. Đánh giá quota DB/WAL, disk full, log và tốc độ thực. Các trường `storage.log*` chưa áp dụng; logger dùng giá trị mặc định, dọn log cũ khi khởi tạo, chưa chứng minh retention/tổng dung lượng xuyên ngày trong 72 giờ.
- [ ] Tăng xác thực JSON: một số trường sai kiểu được thay bằng giá trị mặc định; kiểm tra đầy đủ schema, ý nghĩa đơn vị và audit log thay đổi. Xác nhận an toàn khi chính và backup cùng lỗi. Không coi cấu hình mẫu là cấu hình vận hành.
- [ ] Diagnostics cần làm rõ sức khỏe DB, lần file cuối và timeout nguồn, thêm bằng chứng read-only, permission, recovery, lỗi I/O và cảnh báo nếu mode/tín hiệu không đúng. Chưa có cảnh báo EDID/DPI đầy đủ; tên QScreen có thể trùng/thay đổi. Cần thử mất/cắm lại màn và đảm bảo không hiện nhầm trên output công khai.
- [ ] Kiểm tra snapshot QML/tràn chữ, đặc biệt Flow 6 chữ số, âm và số dài, độ đọc trên LED; test chuyển qua các trang/chế độ. `qmllint --unqualified disable` chỉ là giải pháp tạm, không che dấu rằng loại cảnh báo bị tắt rộng.
- [ ] Kiểm tra giấy phép mã logger/Qt/font/runtime và gói source/license khi bàn giao; UI kit đã pin thành submodule, vẫn cần kiểm tra tính tái lập từ bản clone sạch.

## Chưa xác minh tại nơi triển khai

- [ ] Build Release và package Windows/Visual C++ x64; chạy trên máy sạch không có Qt/dev tools, kiểm tra QML, qwindows, SQLite, redistributable, shortcut có CWD khác và quyền tài khoản. PowerShell và XML scheduler chưa chạy trên Windows.
- [ ] LED thật: mode 1920×1200 qua HDMI/quang/EDID, physical 1280×960 mm/tỷ lệ, không cắt hoặc méo; Extend/Duplicate, 100/125/150% DPI, hotplug, sleep/wake, reboot và crash. Hai tỷ lệ 16:10 và 4:3 cần đo, không tự bù.
- [ ] Thu và duyệt file khí/nước thật, thư mục nhận/quyền/cơ chế hoàn tất, nhịp, mã trạm, timestamp, correction, pH, PM, NH₄⁺/NH₄-N, NOx, trạng thái bridge, ngưỡng và nội dung quy chuẩn. Đối chiếu với nguồn được phép; không tranh inbox với app cũ.
- [ ] Soak ≥72 giờ trên máy đích/tương đương, ghi CPU/RAM/disk/latency, restart/reconnect và kiểm tra log/DB không tăng vô hạn; chạy pilot và rollback theo biên bản được phê duyệt.
