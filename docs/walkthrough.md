# Hướng dẫn kiểm tra phát triển

## Test tự động

Build và chạy `ctest --test-dir build --output-on-failure` theo README. Các suite kiểm tra parser/giờ, config, SQLite, scan, store/ngưỡng, mapping, ingestion và presentation. `tst_ingestion` thử hơn 20 file và file sai tên có cùng nội dung; `tst_presentation` dùng nhiều màn offscreen giả (kể cả trùng tên) và kiểm tra cảnh mô phỏng. `tst_qml_windows` kiểm tra cả cửa sổ LED lẫn vận hành thực sự có trạng thái visible, thay vì chỉ kiểm tra tiến trình còn chạy. Đây không thay thế kiểm thử Windows/LED, khả năng đọc chữ hoặc test 72 giờ.

Qt tự tạo target `caongan_led_qmllint`. Để kiểm tra riêng QML bằng kit Linux đã pin sau build:

```sh
"$HOME/Qt/6.11.1/gcc_64/bin/qmllint" --unqualified disable -W 0 \
  -I build -I build/CaonganLed/App src/app/qml/*.qml
```

`--unqualified disable` bỏ **toàn bộ** loại cảnh báo này, không chỉ context property; không chứng minh mọi truy cập đúng. Lint đầy đủ không tắt nhóm đó vẫn báo các truy cập context/delegate; cần chuyển dần sang API có kiểu. Smoke runtime là bước bổ sung bắt buộc.

## Preview các cảnh

Chạy `--preview` với `CAONGAN_LED_BASE_DIR` riêng và dùng simulator TT10 độc lập theo [hướng dẫn simulator](simulator.md). Màn LED không có nhãn dữ liệu giả; chỉ thử trên màn tách biệt. Quan sát đủ 12 ô, dải đo, hai badge chú giải, dấu tiếng Việt/chỉ số hóa học và các cảnh lỗi không mang màu trong giới hạn. Thử giá trị Flow 1.000.000 và các biên pH 6/9.

Để thử file: chép `sample-data/inputs/` sang thư mục test riêng, cấu hình từng root bằng đường dẫn tuyệt đối và trạm mẫu. Các timestamp 2026-01-01 là cố định nên sẽ stale; không đổi giờ máy để làm chúng mới. Muốn thử dữ liệu mới, tạo file giả mới với timestamp giờ Việt Nam (UTC+7), tuyệt đối không thay nguồn thật. Snapshot và checkpoint có thể giữ trạng thái từ lần trước, vì vậy dùng base dir test mới cho mỗi thí nghiệm.

## Đối chiếu tại hiện trường

Ghi kết quả, thời gian, phiên bản, kit, PC, Windows/DPI, ảnh/bằng chứng và người thử theo [acceptance-checklist](acceptance-checklist.md). Không đánh dấu đã nghiệm thu chỉ vì CTest/offscreen pass.
