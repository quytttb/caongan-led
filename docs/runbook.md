# Vận hành thử và xử lý sự cố

Chỉ dành cho môi trường thử/pilot đã được phép; chưa thay ứng dụng hiện hữu. Trước ca: kiểm tra giờ hệ thống, 2 output, đường dẫn nguồn là bản sao có quyền đọc, dung lượng đĩa, và ngưỡng/cơ sở đo đã được duyệt. Mở cửa sổ vận hành bằng tài khoản được phân quyền; đường dẫn config/data/log và thông tin nguồn xem ở **Chẩn đoán**.

1. Chọn nguồn khí/nước và mã trạm đã xác nhận, đường dẫn tuyệt đối, timeout được phê duyệt; lưu và kiểm tra thông báo. Không thay đích MDATA hay cấu hình producer.
2. Trong **Hiển thị**, chọn đúng output. LED không hiện nếu chưa chọn hoặc mất output. Không dùng Duplicate/1 màn như thể hai vùng độc lập; `--preview` chỉ dành thử nghiệm. Không tự thay mode/DPI Windows từ ứng dụng.
3. Kiểm tra trạng thái từng kênh, timestamp đo và nguồn. `ĐANG ĐO` không đồng nghĩa với đạt chuẩn. Ngưỡng mặc định tắt; chỉ nhập ngưỡng có văn bản duyệt. Ký hiệu/cơ sở/đơn vị chưa chắc chắn thì giữ thiếu/sai đơn vị và báo người phụ trách.
4. Dùng nút mô phỏng chỉ trên môi trường/màn thử; xác nhận nhãn MÔ PHỎNG, kiểm tra các cảnh và tắt mô phỏng trước vận hành. Mô phỏng làm dừng luồng nhập trong lúc bật.
5. Khi nguồn lỗi, kiểm tra thư mục/quyền, Diagnostics và log; không xóa/move file nguồn. Nếu báo quét vượt 5.000 entry, tổ chức bản sao nguồn nhỏ hơn theo phương án được duyệt; không giả định batch sau tự quét hết. Nếu chỉ một kênh cũ, đối chiếu timestamp từng dòng, không làm mới giả.
6. Khi sai đơn vị/mã trạm/status/xung đột, đối chiếu file và người cấp dữ liệu; không đổi alias hoặc hệ số theo phỏng đoán. Bản ghi mới hơn hợp lệ giải quyết xung đột mặc định; correction cùng timestamp cần quy trình riêng.
7. Nếu config lỗi, dừng thay đổi, sao lưu cả file chính và `.backup`, thu log rồi nhờ người phụ trách khôi phục bản hợp lệ đã duyệt. Nếu SQLite lỗi/đĩa đầy, theo dõi diagnostics và báo ngay: RAM có thể còn cập nhật nhưng phục hồi không được bảo đảm. Không tự xóa database để xử lý.
8. Nếu LED mất kết nối, app ẩn cửa sổ LED thay vì cố dời lên màn vận hành; xác nhận tín hiệu/EDID rồi chọn lại đúng output. Nếu ứng dụng treo khi UNC mất kết nối, báo quản trị; giới hạn shutdown hiện chưa bảo đảm.

## Backup và rollback

Được phép và sau khi dừng app, sao lưu riêng JSON + `.backup`, toàn bộ thư mục `data` (gồm `snapshot.db` và `-wal`/`-shm` nếu còn), log và package/version cũ. Không sao chép SQLite đang ghi như thể là bản backup nhất quán. Ghi rõ tài khoản chạy và các biến môi trường. Khi rollback: dừng task/app mới, khôi phục **package và cấu hình đã duyệt** theo bản backup phù hợp; không chạm source files, receiver, database hay app cũ nếu chưa được phép. Nếu giữ/khôi phục cache, xác minh tương thích phiên bản/schema và đối chiếu mốc đo; ưu tiên quy trình được duyệt thay vì xóa file. Xem [Windows deployment](../packaging/windows/README.md).
