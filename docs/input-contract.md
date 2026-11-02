# Hợp đồng đầu vào TT10 dự kiến

Chưa có file khí/nước thực được nhà máy duyệt. Các giả định ở đây phải đối chiếu với producer trước tích hợp; file trong `sample-data/` và `tests/fixtures/` chỉ là giả lập.

## File và bản ghi

- Tên `TenTinh_TenCoso_TenTram_YYYYMMDDHHMMSS.txt`; so mã trạm không phân biệt hoa/thường. Hậu tố bản sao Windows dạng `_1`, `__1` hoặc ` (1)` được bỏ khi nhận diện. Tiền tố `1_` chỉ hợp lệ khi `allowLegacyPrefix` bật; **mặc định hiện đang bật** ở hai nguồn, tắt nếu không cần. Tiền tố khác bị từ chối.
- UTF-8 hợp lệ, có thể BOM; CRLF/LF. Một thông số mỗi dòng, 5 trường TAB: `ký hiệu`, `giá trị`, `đơn vị`, `thời gian đo`, `trạng thái`. Số dòng linh hoạt; giới hạn hiện tại 2 MiB/file, 500 dòng kể cả dòng trống cuối, 1.024 ký tự/trường. Nếu bật `groupedFiveColumn`, cho phép nhiều nhóm 5 trường trên một dòng.
- Số dùng dấu chấm, không nhận NaN/Inf, dấu phẩy hay ký pháp khoa học. Zero thật hợp lệ. `---`, `--`, `N/A`, rỗng không phải số 0. Timestamp 14 chữ số hợp lệ theo UTC+7 bất kể timezone máy; timestamp tên file không thay timestamp đo. Timestamp không hợp lệ hoặc lỗi cấu trúc khiến batch bị từ chối.
- `00` đang đo, `01` hiệu chuẩn, `02` lỗi thiết bị. Mã khác/trường thứ 5 thiếu được giữ là **không xác định**, không tự gán `00`; dòng 4 trường được nhận diện để báo trạng thái không xác định. Không suy ra ngưỡng từ trạng thái thiết bị.
- Chỉ so alias đã cấu hình, không fuzzy match. PM mặc định `mg/Nm³` không tương đương `mg/m³`; NH₄-N không được map sang NH₄⁺; không tổng hợp NOx. Alias `NH4` không ghi rõ cơ sở, phải xác nhận producer thực sự báo NH₄⁺ trước khi cho phép vận hành. Không tự quy đổi vật lý.

## Nhập thư mục

Đặt đường dẫn tuyệt đối riêng theo nguồn. App không đổi tên, sửa, move hay xóa file nguồn; bỏ qua file trống, phần mở rộng tạm và symlink phát hiện được trong quá trình quét. Không dùng hai app tranh file inbox nếu app cũ move/xóa; cần luồng bản sao ổn định được duyệt. Chỉ dùng folder có quyền đọc, kiểm tra bằng tài khoản chạy app.

App quét định kỳ khoảng 2 giây, yêu cầu hai quan sát ổn định cách nhau tối thiểu 1,2 giây, kiểm tra metadata sau đọc và retry lỗi mở file có backoff. Producer ghi file tạm rồi rename khi hoàn thành vẫn là phương án tốt hơn. Các giới hạn 5.000 entry/lần và 20 file/batch, rủi ro UNC/locked file, checkpoint và đồng hồ được ghi tại [todo.md](todo.md). Không suy ra mọi file đã được nhập chỉ từ nguồn báo sẵn sàng.
