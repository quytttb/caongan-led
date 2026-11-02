# Cao Ngạn LED

Ứng dụng thử nghiệm C++20 / Qt Quick hiển thị 12 thông số khí thải và nước thải; đọc file TT10, không kết nối cảm biến, Modbus hay thay đổi file nguồn. Cửa sổ LED và cửa sổ vận hành tách riêng; khung thiết kế 1920 × 1200.

**Chưa sẵn sàng thay ứng dụng vận hành.** Bản trước đã build/đóng gói Windows trên GitHub-hosted runner, nhưng smoke cũ không phát hiện lỗi ẩn cửa sổ; bản sửa cửa sổ hiện chưa được kiểm thử Windows. Chưa thử trên Windows sạch/máy đích, LED/EDID, file nguồn thực hoặc chạy bền 72 giờ; còn các hạn chế kỹ thuật tại [docs/todo.md](docs/todo.md). Không cài Task Scheduler, thay cấu hình receiver hay chạy song song trên inbox thật trước khi được phê duyệt.

## Chạy thử trên Linux

Yêu cầu Qt **6.11.1** với Core, Gui, Qml, Quick, Quick Controls 2, Quick Layouts, SQL/SQLite và Qt Test; CMake ≥ 3.24, Ninja, compiler C++20 và Python ≥ 3.10 cho test simulator. GUI simulator cần thêm Python Tkinter. `shared/logger-ui-kit` phải có đúng revision trong [provenance](docs/provenance.md).

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/6.11.1/gcc_64" -DCAONGAN_BUILD_TESTS=ON
cmake --build build -j 4
ctest --test-dir build --output-on-failure
./build/caongan_led --preview --simulate --scenario 'Flow 6 chữ số'
```

LED luôn toàn màn hình, không viền; bố cục giữ tỉ lệ thiết kế 1920 × 1200 nên có thể có dải đen khi màn hình khác tỉ lệ. `--preview` hiển thị LED toàn màn hình trên màn chính, không cần chọn output; dùng Alt+Tab để chuyển về cửa sổ vận hành khi chỉ có một màn hình. Cửa sổ vận hành có thanh tiêu đề riêng, hỗ trợ kéo, thu nhỏ, phóng to và đổi kích thước.

`--simulate` tạm dừng nhập nguồn và hiển thị dữ liệu giả **không có nhãn trên màn LED**. Không dùng số hoặc ngưỡng thử làm dữ liệu vận hành. Cấu hình và database mặc định vẫn được tạo trong vùng dữ liệu ứng dụng; để cách ly hoàn toàn, đặt `CAONGAN_LED_BASE_DIR` tới một thư mục thử riêng. Chạy `--help` để xem tùy chọn; `--config <đường-dẫn>` chỉ đổi file cấu hình, không đổi database/log hay khóa một instance.

Trang **Nhật ký** trong cửa sổ vận hành cho phép chọn log hiện tại hoặc log đã xoay vòng, làm mới, sao chép nội dung đang xem và xuất toàn bộ tệp. Màn xem giới hạn 1 MiB cuối; tệp xuất tối đa 64 MiB. Log có thể chứa đường dẫn, giá trị và trạng thái kênh, cần kiểm tra trước khi chia sẻ.

Clone với `git clone --recurse-submodules`, hoặc chạy `git submodule update --init --recursive` sau khi clone. Mẫu cấu hình [sample-data/caongan-led.example.json](sample-data/caongan-led.example.json) tương ứng mặc định: thư mục nguồn trống, ngưỡng tắt, chưa chọn màn. File tại `sample-data/inputs/` là dữ liệu **giả**, mốc cố định 01/01/2026 và sẽ hiển thị cũ khi vượt timeout. Để thử đọc file, dùng bản sao thư mục thử và cấu hình đường dẫn tuyệt đối; đừng đưa mẫu lên LED công khai như dữ liệu thật.

## Tài liệu

- [Kiến trúc](docs/architecture.md), [hợp đồng đầu vào](docs/input-contract.md), [cấu hình](docs/config-contract.md).
- [Hướng dẫn vận hành](docs/runbook.md), [hướng dẫn thử](docs/walkthrough.md), [checklist nghiệm thu](docs/acceptance-checklist.md).
- [Windows build/deploy và rollback](packaging/windows/README.md); [nguồn gốc](docs/provenance.md), [giấy phép bên thứ ba](THIRD_PARTY_NOTICES.md), [việc còn lại](docs/todo.md).
- [CI/CD và artifact thử nghiệm](docs/ci-cd.md).
- [Simulator TT10 cho test thủ công](docs/simulator.md).

Thứ tự cố định: khí thải **O₂, CO, NOx, SO₂, PM, Temp, Flow, P**; nước thải **pH, TSS, COD, NH₄⁺**. `00` là đang đo, không có nghĩa là đạt ngưỡng hay tuân thủ quy chuẩn. Không chuyển đổi NH₄-N, PM mg/m³ hay NO/NO₂ thành giá trị khác.

`04` được ứng dụng hỗ trợ là bảo trì theo yêu cầu riêng; mã này không có trong Bảng 35 TT10 và cần xác nhận với nguồn dữ liệu.
