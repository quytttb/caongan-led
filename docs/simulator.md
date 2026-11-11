# Simulator TT10 — dữ liệu giả lập

`tools/tt10_simulator.py` tạo hai luồng file TT10 giả để thử đường nhập thực tế. Dùng Python **3.10 trở lên**; giao diện cần `tkinter`. File không phải dữ liệu quan trắc và **không được nối vào ứng dụng hay LED vận hành**. Đây là công cụ độc lập, không giao tiếp cảm biến, Modbus, mạng hoặc thay đổi file CEMS.

## Dùng bản cài preview trên laptop

1. Mở ứng dụng **CÔNG TY NHIỆT ĐIỆN CAO NGẠN - TKV** và **Cao Ngạn — Simulator** trong danh sách ứng dụng, hoặc chạy `caongan-led-preview` và `caongan-led-simulator` từ terminal.
2. Trong Simulator, sửa 12 giá trị nếu muốn, chọn **Bình thường**, rồi bấm **Bắt đầu**. Chờ khoảng vài giây cho hai lần kiểm tra file ổn định. Hai nguồn và snapshot của bản preview được cấu hình sẵn tại `~/.local/share/caongan-led-test/`.
3. Đổi tình huống trong Simulator; lần ghi tiếp theo áp dụng cảnh mới. **Ghi ngay** ghi một bộ, **Dừng** dừng phát; đóng cửa sổ cũng dừng. Công cụ tự dừng sau 120 lần ghi và giới hạn 1.000 file mỗi nguồn; không tự xóa file.
4. Màn LED không tự gắn nhãn dữ liệu giả. Dùng riêng màn và cấu hình thử, không đưa thư mục test sang cấu hình thật.

Tình huống có sẵn: bình thường, giá trị cao, hiệu chuẩn `01`, lỗi thiết bị `02`, dữ liệu cũ, thiếu pH, PM sai đơn vị, xung đột SO₂ cùng timestamp, Flow 6 chữ số và trạng thái lạ `03`. Hai cảnh **dữ liệu cũ/thiếu pH** cố ý ngừng gửi kênh tương ứng: hãy gửi bình thường trước, chuyển cảnh rồi chờ timeout; sandbox cài sẵn dùng 15 giây. So sánh giá trị thử với ngưỡng đang cấu hình; chỉ sửa ngưỡng trong môi trường thử cô lập.

## Chạy từ mã nguồn

```sh
python3 tools/tt10_simulator.py                     # giao diện
python3 tools/tt10_simulator.py --prepare           # tạo sandbox rỗng được đánh dấu
python3 tools/tt10_simulator.py --once --scenario fault
python3 tools/tt10_simulator.py --help
```

`--root` nhận đường dẫn tuyệt đối tới sandbox **mới/rỗng** hoặc sandbox đã có marker hợp lệ. Simulator từ chối thư mục không rỗng chưa có marker, từ chối symlink tại root/input và không ghi đè file sẵn có; file tạm được tạo riêng rồi công bố atomic bằng hard link. Điều này yêu cầu filesystem hỗ trợ hard link. Công cụ không tự xóa file đã phát. Cấu hình app thủ công phải trỏ hai nguồn tới `inputs/khi-thai`, `inputs/nuoc-thai` trong sandbox, trạm lần lượt `KHITO1`, `NUONT1` và phải dùng vùng config/database riêng.

Timestamp trong file là giờ Việt Nam UTC+7, độ phân giải một giây. Nếu hai bộ khác nhau được ghi cùng giây, app có thể báo xung đột như thiết kế; GUI hạn chế ghi liên tiếp quá nhanh. Test tự động: `python3 tests/test_tt10_simulator.py` và `ctest --test-dir build-ci --output-on-failure` (bao gồm test simulator → file → app → SQLite).
