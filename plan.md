# Kế hoạch xây dựng phần mềm LED Cao Ngạn — C++ / Qt / QML

- Ngày lập: 02/11/2026.
- Trạng thái: kế hoạch chi tiết, chưa triển khai mã nguồn.
- Phạm vi đã thống nhất: chỉ thay ứng dụng hiển thị LED trên Windows.
- Công nghệ do người dùng chọn: Qt và QML; tái sử dụng có chọn lọc dự án data-logger đã có.
- Thư mục dự án mới dự kiến: `/home/haiquy/Documents/DA cao ngan/APP_MAN_LED_QT/`.
- Dự án tham khảo: `/home/haiquy/Documents/Projects/LOGGER/data-logger/`.
- Mọi đường dẫn của dự án mới dưới đây là dự kiến, chưa được tạo trong bước lập kế hoạch.

## 1. Mục tiêu và giới hạn phạm vi

### 1.1. Sản phẩm cần bàn giao

Ứng dụng Windows tự hiển thị số liệu quan trắc khí thải và nước thải lên màn LED ở cổng nhà máy, thay `QuanTracMoiTruong.exe` cũ. Vận hành liên tục, khởi động lại được, dễ cấu hình tại chỗ và không hiển thị dữ liệu lỗi/cũ như số liệu bình thường.

- Đủ 12 thông số trên cùng một màn, vị trí cố định, không cuộn và không luân phiên trang.
- Màn LED toàn màn hình; cửa sổ cấu hình/chẩn đoán dành cho người vận hành tách riêng.
- Nhận file `.txt` từ luồng dữ liệu được nhà máy cho phép, trước hết là file MDATA 8690 xuất theo TT10.
- Có trạng thái đo, hiệu chuẩn, lỗi thiết bị, dữ liệu cũ, thiếu dữ liệu và lỗi đầu vào.
- Có bản chạy Windows đóng gói đủ runtime, cấu hình, log và hướng dẫn vận hành/khôi phục.

### 1.2. Không đưa vào phiên bản này

- Không xây lại data logger hoặc hệ thống CEMS.
- Không đọc/ghi trực tiếp cảm biến, không tích hợp Modbus RTU/TCP trong ứng dụng LED.
- Không quản lý truyền dữ liệu lên cơ quan quản lý; không thêm xuất báo cáo TT10.
- Không có lịch sử quan trắc, biểu đồ, báo cáo tháng, hiệu chuẩn cảm biến, điều khiển DI/DO.
- Không chuyển dữ liệu lịch sử từ `environmental_data.db` cũ sang ứng dụng mới.
- Không phát triển FTP/FTPS server bên trong app, không thêm web server/REST API mặc định.
- Không dùng SDK card LED: kết nối hiện được mô tả là đường hình ảnh HDMI qua bộ chuyển đổi quang.
- Không tự thay đổi MDATA, bridge WTW, máy chủ nhận file, firewall, tài khoản Windows hay luồng truyền hiện có.

## 2. Các quyết định làm chuẩn

### 2.1. Màn hình và nội dung

- Kích thước hiển thị theo hồ sơ: **1280 × 960 mm**.
- Độ phân giải thiết kế theo hồ sơ: **1920 × 1200 pixel**; không lấy 1920 × 1080 làm chuẩn.
- Đường hình ảnh: PC HDMI → bộ chuyển HDMI/quang → cáp quang → bộ chuyển quang/HDMI → LED.
- `LED DISPLAY.pdf` là nguồn tham khảo nội dung, không phải bố cục bắt buộc.
- Khí thải, theo thứ tự cố định: O₂, CO, NOx, SO₂ / PM, Temp, Flow, P.
- Nước thải, theo thứ tự cố định: pH, TSS, COD, NH₄⁺.
- Nếu có nhiều trạm/ống khói, phải chỉ định trạm cấp dữ liệu cho từng nhóm; không tự lấy trạm bất kỳ hoặc lấy trung bình.

**Điểm cần kiểm tra khi lắp đặt:** kích thước vật lý có tỷ lệ 4:3, trong khi ma trận pixel có tỷ lệ 16:10. Giữ cả hai thông số người dùng đã chốt; kiểm tra EDID, chế độ tín hiệu và mapping của bộ điều khiển bằng mẫu ô vuông/vòng tròn. Không tự sửa thông số hồ sơ, kéo giãn, cắt hình hoặc bù méo khi chưa đo/xác nhận thực tế.

### 2.2. Nguyên tắc dữ liệu

- Bản ghi mới nhất được chọn theo **thời gian đo của từng thông số**, không theo giờ nhận file, tên file mới được copy hay thứ tự quét.
- TT10 chỉ có ba mã trạng thái thiết bị: `00` đang đo, `01` hiệu chuẩn, `02` lỗi thiết bị.
- `00` không có nghĩa là trong giới hạn cho phép.
- Trạng thái thiết bị, độ mới, tính hợp lệ và kết quả so ngưỡng là các chiều độc lập.
- Không suy ra lỗi thiết bị chỉ vì kết quả bằng 0; không áp dụng quy tắc chung COD ≤ 0 → `02`.
- Không tự điền số 0 khi thiếu pH hoặc thông số khác.
- Không tự chuyển NH₄-N sang NH₄⁺, mg/m³ sang mg/Nm³ hoặc tính NOx từ NO/NO₂.
- Không dùng ngưỡng trong database cũ hoặc số ví dụ trong PDF làm ngưỡng vận hành được phê duyệt.
- Không di chuyển, đổi tên, sửa hoặc xóa file nguồn.

## 3. Kết quả khảo sát dự án data-logger và phương án tái sử dụng

### 3.1. Nền tảng đã xác minh

- Backend C++20; Qt Quick/QML, Qt Quick Controls 2; tổ chức theo controller/model và worker.
- Qt được pin tại `packaging/qt_version.txt`: **6.11.1**.
- Build bằng CMake, đăng ký QML qua `qt_add_qml_module`, `QML_ELEMENT`, `QML_SINGLETON`.
- Commit HEAD của data-logger lúc khảo sát: `3407c5ebb6c423edafbea56e24a1601f9010ccac`.
- UI kit ở `shared/logger-ui-kit`, là git submodule; HEAD lúc khảo sát: `24ef01544e398e0e181933a71ee00d40f28c697e`.
- Working tree data-logger có thay đổi chưa commit. Không chỉnh sửa/revert chúng; lúc lấy mã phải ghi rõ phiên bản và nội dung thực sự được lấy, không mặc định toàn bộ working tree trùng HEAD.

### 3.2. Phần dự kiến lấy sang và điều chỉnh

1. **Đường dẫn ứng dụng — `src/utils/system/AppPaths.h/.cpp`.**
   - Giữ cách dùng `QStandardPaths`, tách config/data/log, không phụ thuộc CWD.
   - Đổi định danh ứng dụng và biến môi trường sang namespace LED, không dùng chung dữ liệu với DataLogger.
   - Thư mục file đầu vào là cấu hình riêng, không phải thư mục cache do ứng dụng tự tạo.

2. **Logging — `src/utils/system/LogSetup.h/.cpp`.**
   - Tái sử dụng logging và ý tưởng xoay file theo dung lượng.
   - Đổi tên log, bổ sung tổng dung lượng/retention, kiểm tra chạy xuyên ngày và xử lý không ghi được log.
   - Không bê nguyên giả định logging hiện tại đã có đầy đủ retention cho nhiều ngày.

3. **Thời gian — tham khảo `src/data/db/TimeCodec.h`.**
   - Dùng UTC để lưu/so sánh nội bộ; hiển thị và parse TT10 theo Việt Nam UTC+7 rõ ràng.
   - Viết codec TT10 riêng; không dựa vào timezone đang cài trên Windows.

4. **Model QML — tham khảo `src/core/MonitorModel.h/.cpp` và `src/utils/qml/QmlSingleton.h`.**
   - Lấy cách dùng `QAbstractListModel`, role names, `dataChanged` và đăng ký QML.
   - Viết `LedParameterModel` gọn cho 12 vị trí; bỏ sensor ID theo Modbus, DI/DO, raw register và phần liên quan edge.
   - Không giữ logic `updateValue()` gán `OK/ALARM` thay cho trạng thái thiết bị; tách các chiều trạng thái như mục 7.

5. **Font và nạp tài nguyên — `resources/fonts/Inter/`, `src/app/main.cpp`.**
   - Dùng Inter Regular/SemiBold/Bold nếu thử trên LED đạt; giữ `OFL.txt` trong bộ bàn giao.
   - Lấy cách nhúng font bằng Qt resources và nạp bằng `QFontDatabase`.
   - Kiểm tra tiếng Việt, chỉ số dưới/trên, dấu âm, dấu thập phân và chữ số có độ rộng ổn định.

6. **Build/test — các `CMakeLists.txt`, `tests/`.**
   - Lấy cấu trúc module, khai báo QML resources và cách đăng ký CTest.
   - Test mới dùng Qt Test với assertion còn hoạt động ở Release; không bê nguyên test chỉ dùng C `assert`.
   - Viết lại target cho Windows; không bê cờ linker `-s`, đường dẫn GCC, systemd, polkit, EGLFS hay script đóng gói Pi.

### 3.3. UI kit dùng chung: tái sử dụng nhưng không copy component

- Tuân theo `AGENTS.md` của data-logger: không copy/duplicate các component generic của kit về app.
- Giữ kit dưới dạng submodule ở dự án mới, pin revision; không symlink sang working tree logger để build phụ thuộc máy cá nhân.
- MVP chỉ tích hợp **`LoggerKit.Theme`** bằng CMake của `shared/logger-ui-kit/src/theme`, kèm `LoggerKit::theme_setup`; khai báo biến/module/plugin cần thiết ở host và kiểm tra build Windows ngay từ đầu.
- Lý do chỉ lấy theme: `LoggerKit.Components` hiện kéo dependency `QtGraphs`, trong khi màn LED không cần đồ thị.
- Giao diện cấu hình có thể dùng Qt Quick Controls 2 với theme sẵn có; phần LED có theme độ tương phản cao riêng của app.
- Không dùng nguyên `StatusChip.qml` cho LED vì kích thước hiện tại 24/30 px quá nhỏ. `LedStatusLabel` mới là thành phần đặc thù LED, không phải bản copy của chip generic.
- Không thay API của UI kit hoặc token global để phục vụ riêng màn LED. Nếu sau này cần cả kit Components, phải chấp nhận dependency hoặc có thay đổi riêng được duyệt, kiểm thử với các app dùng chung.
- Giữ giấy phép MIT của kit; kiểm tra giấy phép Qt, font, logo và tài nguyên được phân phối.

### 3.4. Phần chỉ tham khảo, không lấy nguyên xi

- `ReportNaming.cpp`: là bên tạo tên file, phụ thuộc `AppConfig` của logger; chỉ tham khảo quy ước TT10, không mang toàn bộ cấu hình logger sang.
- `SensorSymbolCatalog.h`: tham khảo từ điển, nhưng catalog hiện có `NH4`, không có đầy đủ mọi ký hiệu thực tế cần LED như `NOx`; không dùng làm danh sách bắt buộc để từ chối file MDATA.
- `Tt10ReportWriter.cpp`: là **writer**, không phải parser. Hiện có trạng thái nội bộ `03` và ghép các nhóm 5 trường lên một dòng; không dùng làm bằng chứng mọi file như vậy đều đúng chuẩn TT10.
- Fixture hiện có trong `tests/fixtures/tt10-sample/` dùng nhóm 5 trường lặp trên một dòng: có thể lấy làm **fixture tương thích**, không thay thế fixture chuẩn mỗi thông số một dòng.
- `Database`/DAO: tham khảo connection theo thread, prepared statements và transaction; không copy toàn bộ schema cảm biến/lịch sử/tài khoản.
- Không lấy `main.cpp`, `SettingsController`, `MonitorController`, `DeviceId`, `DeviceLock`, FTP, REST, Modbus, đăng nhập kiosk hoặc shell 1024 × 600 nguyên khối.
- Mã nguồn Pi có thao tác `/proc`, `/sys`, `timedatectl`, `systemctl` và virtual keyboard/EGLFS; tất cả không thuộc app Windows này.

### 3.5. Quy trình đưa mã sang khi bắt đầu triển khai

- Chốt danh sách file lấy; chỉ copy file C++/resource được chọn, giữ nguồn gốc và giấy phép.
- Ghi provenance trong tài liệu dự án mới: đường dẫn nguồn, revision, trạng thái sửa cục bộ nếu có và thay đổi đã làm khi chuyển sang Windows.
- Viết test cho helper đã điều chỉnh trước khi gắn với UI.
- Không sửa repository data-logger hay submodule nguồn trong quá trình chuyển.
- Không copy build directory, database, log, config vận hành, mật khẩu/token hoặc tài nguyên branding không liên quan.

## 4. Stack và kiến trúc ứng dụng mới

### 4.1. Stack đề xuất

- C++20, **Qt 6.11.1**, QML/Qt Quick, Qt Quick Controls 2.
- CMake; Windows kit MSVC 2022 x64 tương ứng bản Qt, xác minh OS/GPU đích trước khi khóa cấu hình phát hành.
- Phát triển/test logic trên Linux được, nhưng bản nghiệm thu phải build và chạy trên Windows thật; không xem build Linux là đủ.
- Qt modules chính: Core, Gui, Qml, Quick, QuickControls2, Sql; Test/QuickTest cho kiểm thử.
- SQLite cục bộ chỉ lưu snapshot gần nhất và checkpoint nhập file, không làm kho lịch sử CEMS.
- Không cần SerialBus, HttpServer, Graphs, VirtualKeyboard hoặc OpenSSL cho chức năng MVP đã chọn.
- Dùng Qt runtime liên kết động; static QML module của kit không đồng nghĩa đóng gói Qt runtime tĩnh.

### 4.2. Luồng xử lý

```text
MDATA / luồng nước thải đã được chấp thuận
                  |
        thư mục TXT sẵn có, chỉ đọc
                  |
   FolderSourceWorker → Tt10Parser → ParameterMapper
                  |
   LatestReadingStore + DataQuality + ThresholdEvaluator
                  |
      LedParameterModel / DiagnosticsModel
                  |
       QML LED window + operator window
```

- I/O đọc/quét file và SQLite chạy ngoài UI thread; gửi batch kết quả qua signal/slot.
- Parse/ánh xạ/so ngưỡng là logic C++ thuần có thể test không cần QML.
- UI thread chỉ giữ model và render; QML không quyết định hợp lệ hay tuân thủ ngưỡng.
- Một worker ingestion tuần tự đủ cho MVP; không thêm hệ worker phức tạp khi chưa có nhu cầu đo được.
- Shutdown có timeout và cancel; không xóa QThread còn chạy, không blocking vô hạn.

### 4.3. Cấu trúc thư mục dự kiến

```text
APP_MAN_LED_QT/
  CMakeLists.txt
  src/
    app/          main.cpp, cửa sổ LED và cửa sổ vận hành
    domain/       Reading, trạng thái, mapping, chọn mới nhất, so ngưỡng
    ingestion/    bộ đọc thư mục và parser TT10
    storage/      config, snapshot SQLite, checkpoint
    presentation/ model/controller cho QML, quản lý màn hình
    qml/          LedBoard, ParameterTile, Settings, Diagnostics
    utils/        AppPaths, LogSetup, TT10 time codec
  shared/logger-ui-kit/    submodule được pin
  resources/     font, logo được duyệt, cấu hình mẫu
  tests/         unit, integration, QML, fixtures
  packaging/windows/
  docs/          build, cấu hình đầu vào, vận hành, nghiệm thu, nguồn mã tái sử dụng
```

## 5. Nguồn dữ liệu và ranh giới tích hợp

### 5.1. Phương án ưu tiên

- Nếu PC đã có thư mục nhận file từ MDATA: app đọc đường dẫn tuyệt đối đó, không thay cách nhận.
- Nếu thư mục do app cũ tiêu thụ và move file: cần thống nhất điểm nhận/bản sao read-only ổn định trước khi chạy song song. Không trông chờ hai app tranh đọc một inbox ngắn hạn.
- Nếu chưa có nơi nhận: nhà máy phê duyệt một FTP/FTPS receiver độc lập hoặc cơ chế chuyển file hiện có. Chốt quyền thư mục, lịch giữ file và hoàn tất upload; app LED vẫn chỉ đọc thư mục.
- Không tự đổi đích truyền MDATA hay thêm kết nối vào hệ thống đang vận hành.
- Ưu tiên thư mục local của PC; nếu dùng UNC/SMB phải kiểm thử mất kết nối, reconnect và quyền truy cập của đúng tài khoản chạy app. Không dựa vào mapped drive của tài khoản khác.

### 5.2. Nước thải

- Bridge WTW có thể là nguồn file trung gian bên ngoài app nếu được cho phép, không tích hợp polling của bridge vào app LED.
- WTW đã khảo sát cung cấp TSS, COD, NH₄-N và nhiệt độ; **pH chưa xác định nguồn**.
- Chỉ cần bốn thông số nước thải trên LED: pH, TSS, COD, NH₄⁺. Ba kênh lưu lượng nước không nằm trong phạm vi màn hình này.
- Với pH chưa có nguồn, phát triển/preview vẫn thực hiện được; vận hành phải hiển thị thiếu dữ liệu, không số giả. Nghiệm thu đủ 12 kênh chỉ thực hiện sau khi có nguồn thực.
- Không coi các hệ số TSS ×1000, NH₄-N ×1,2876 hay quy tắc trạng thái trong bridge cũ là mặc nhiên đúng cho mọi nguồn mới.
- Nếu nhà máy tiếp tục dùng bridge hiện tại, phải giải quyết khả năng bridge gộp vượt ngưỡng với `02` hoặc bỏ sót trạng thái thiết bị. App LED không thể phục hồi ý nghĩa đã bị mất trong file.
- Sửa bridge, nếu cần, là đầu việc tích hợp riêng phải được duyệt; không âm thầm thay trong công việc xây app.

## 6. Hợp đồng file và cơ chế ingestion

### 6.1. Parser chính

- Mẫu tên chuẩn: `TenTinh_TenCoso_TenTram_YYYYMMDDHHMMSS.txt`.
- Mỗi thông số một dòng, đúng 5 trường TAB: ký hiệu, kết quả, đơn vị, thời gian đo, trạng thái thiết bị.
- Chấp nhận số lượng thông số khác nhau; không yêu cầu đúng 8 dòng/trạm.
- Không phụ thuộc tiền tố `1_`, tên tỉnh/cơ sở `TNg/NDCN` hoặc mã trạm hardcode từ app cũ.
- Station/source mapping qua cấu hình rõ ràng; từ chối mapping mơ hồ hoặc trạm không được chọn, không tự gán vào nhóm còn trống.
- UTF-8 mặc định; dung nạp UTF-8 BOM và CRLF/LF một cách có kiểm thử, không đoán bừa encoding khi giải mã lỗi.
- Parse số độc lập locale, dấu chấm thập phân; từ chối NaN/Inf/overflow và định dạng không được khai báo.
- Timestamp phải đủ 14 chữ số, ngày giờ thực sự hợp lệ; diễn giải UTC+7 kể cả khi Windows đặt timezone khác.
- Dùng timestamp trong bản ghi để chọn dữ liệu; timestamp tên file chỉ phục vụ nhận diện/kiểm tra chênh lệch.
- Giới hạn dung lượng file, số dòng và độ dài trường; chỉ đọc file thường trong root đã cấu hình, không theo shortcut hoặc đường dẫn vượt root.

### 6.2. Chế độ tương thích, bật riêng theo nguồn

- Cho phép tên file có tiền tố legacy `1_` khi thực tế cần, không biến nó thành yêu cầu chung.
- Chế độ đọc nhóm 5 trường lặp trên một dòng chỉ bật nếu file thực sự có dạng đó; ghi rõ đây là tương thích nguồn.
- Không mặc định thiếu trường status là `00`; bản ghi 4 trường không được coi là đo bình thường.
- `03` hoặc mã lạ: trạng thái không hỗ trợ/không xác định, không tự đổi thành `00`. Nếu một nguồn có quy ước nội bộ riêng, phải có adapter được duyệt và test riêng.
- Placeholder `---` từ nguồn được biểu diễn thiếu/không hợp lệ, không biến thành số 0.
- Alias ký hiệu có danh sách rõ ràng theo nguồn; không dùng fuzzy matching làm NO, NO₂ và NOx nhập nhằng.

### 6.3. Không đọc file đang ghi dở

- Tốt nhất producer ghi file tạm rồi rename khi xong; xác nhận cơ chế thực tế với bên cấp dữ liệu.
- App bỏ qua `.tmp`, `.part`, file rỗng và file đang thay đổi; chỉ thử file ổn định kích thước/mtime qua ít nhất hai lần kiểm tra.
- Sau đọc, kiểm tra lại metadata; nếu thay đổi thì bỏ kết quả và thử lại.
- Bộ kiểm tra ổn định chỉ là hàng rào bổ sung, không phải bảo đảm tuyệt đối upload hoàn tất. File có lỗi cấu trúc không được nhập một phần.
- Retry có backoff và giới hạn, ví dụ 1/2/5/10 giây với trần cấu hình; không vòng lặp nóng, không ghi hàng nghìn log cùng lỗi.
- File hỏng không được move sang quarantine tại nguồn; lưu thông tin lỗi trong trạng thái/log của app, chỉ recheck khi file thay đổi hoặc có thao tác retry.

### 6.4. Theo dõi thư mục và chống trùng

- Kết hợp `QFileSystemWatcher` với quét định kỳ vì watcher có thể bỏ sự kiện, đặc biệt trên share mạng.
- Có thể theo dõi cây `trạm/yyyy/MM/dd`; ưu tiên thư mục gần hiện tại, quét phục hồi theo batch và có giới hạn, không đọc lại toàn bộ lịch sử mỗi chu kỳ.
- Theo dõi reconnect của root và thư mục mới; lỗi một nguồn không dừng nguồn còn lại.
- Nhận diện phiên bản file bằng đường dẫn tương đối, metadata và content hash; tên file không phải khóa duy nhất.
- Khóa dữ liệu logic gồm nguồn/trạm, thông số và thời gian đo. File copy lại hoặc đổi tên nhưng cùng nội dung không tạo cập nhật giả.
- Cùng timestamp, cùng nội dung: bỏ qua. Cùng timestamp, khác giá trị/status/unit: ghi nhận **xung đột**, không cho thứ tự nhận quyết định và không để số đang tranh chấp mang nhãn trong giới hạn.
- Chính sách chỉnh sửa cùng timestamp phải được thỏa thuận riêng nếu nguồn có nhu cầu correction; mặc định chờ bản ghi mới hơn hoặc xử lý vận hành rõ ràng.
- Bản ghi đến muộn chỉ bổ sung chẩn đoán/checkpoint, không làm lùi số liệu hiện tại.
- Lưu snapshot và checkpoint liên quan trong transaction để restart không gây sai lệch trạng thái đã nhập.

## 7. Mô hình dữ liệu, trạng thái và đánh giá ngưỡng

### 7.1. Dữ liệu mỗi vị trí

- ID vị trí cố định, nhóm khí/nước, mã thông số chuẩn nội bộ, tên/ký hiệu hiển thị.
- Source/trạm, ký hiệu và đơn vị gốc, giá trị gốc có thể rỗng, giá trị hiển thị nếu hợp lệ.
- Thời gian đo UTC và chuỗi thời gian nguồn; giờ nhận/giờ parse tách riêng.
- Mã trạng thái thiết bị gốc; tính hợp lệ; độ mới; kết quả so ngưỡng; nguồn gốc file/hash.
- Số chữ số thập phân và đơn vị mong đợi được cấu hình theo từng thông số.
- Không lưu string đã format làm dữ liệu duy nhất; so ngưỡng bằng giá trị trước làm tròn.

### 7.2. Bốn chiều trạng thái

1. **Thiết bị:** Measuring / Calibrating / DeviceFault / Unknown.
2. **Tính hợp lệ:** Valid / Missing / Invalid / UnitMismatch / Conflict.
3. **Độ mới:** Fresh / Stale / FutureTimestamp; nguồn còn có trạng thái kết nối/lỗi đọc riêng.
4. **So ngưỡng:** WithinConfiguredLimit / ExceedsConfiguredLimit / NotConfigured / NotApplicable / NotEvaluated.

Chỉ được gắn nhãn trong giới hạn khi đồng thời: dữ liệu hợp lệ, đang đo, còn mới, đơn vị/cơ sở đo tương thích và có ngưỡng được phê duyệt.

### 7.3. Quy tắc hiển thị an toàn

- `00` hợp lệ, mới, không có ngưỡng: hiện số và nhãn **ĐANG ĐO**; không ghi đạt chuẩn/trong giới hạn.
- `00` hợp lệ, mới, có ngưỡng: hiện số; vượt ngưỡng dùng đỏ sáng và chữ **VƯỢT NGƯỠNG**.
- `01`: số công khai thay bằng `—`, nhãn **HIỆU CHUẨN**; giá trị thô chỉ xem ở chẩn đoán.
- `02`: `—`, nhãn **LỖI THIẾT BỊ**; không giữ số cũ dưới dạng bình thường.
- Dữ liệu cũ: `—`, nhãn **DỮ LIỆU CŨ** và mốc cuối nếu bố cục cho phép; không giữ nguyên màu bình thường.
- Chưa có dữ liệu: `—`, nhãn **CHƯA CÓ DỮ LIỆU**.
- Sai đơn vị, giá trị không hợp lệ, status lạ hoặc xung đột: `—`, nhãn tương ứng; không so ngưỡng.
- Bản ghi mới hơn mang lỗi/hiệu chuẩn/giá trị không hợp lệ phải thay trạng thái hiện tại, không bỏ qua để tiếp tục khoe số tốt cũ.
- Thiếu một thông số trong file không làm tuổi dữ liệu của nó được reset. Các thông số khác vẫn cập nhật được nếu file đúng cấu trúc.
- File hỏng cấu trúc hoặc timestamp không thể sắp thứ tự: từ chối batch, giữ mốc đo cuối nhưng bật cảnh báo nguồn/lỗi đầu vào; không quảng bá là vừa có số liệu mới.
- Nếu có nhiều nguyên nhân, hiển thị ưu tiên lỗi rõ nhất nhưng giữ đầy đủ cờ trong chẩn đoán; không làm stale che mất lịch sử lỗi thiết bị.

### 7.4. Độ mới và đồng hồ

- Cấu hình chu kỳ dự kiến và stale timeout riêng theo nguồn/thông số; giá trị triển khai phải xác nhận, không mặc định 15 phút là quy định pháp lý.
- Theo dõi độ mới ngay cả khi không có file mới bằng timer nhẹ.
- Khởi động lại phải tính tuổi từ thời gian đo, không từ lúc nạp cache.
- Dùng clock có thể giả lập trong test; phát hiện bước nhảy đồng hồ, không để chỉnh giờ lùi biến số liệu cũ thành mới.
- Timestamp vượt tương lai quá dung sai cấu hình phải báo lỗi thời gian và không trở thành bản ghi hiện tại chặn mọi dữ liệu sau đó.
- Không sửa giờ/timezone/NTP của Windows từ ứng dụng LED.

### 7.5. Đơn vị, cơ sở đo và ngưỡng

- Khởi tạo cấu hình ngưỡng trống; người vận hành nhập theo bộ thông số được duyệt.
- Hỗ trợ ngưỡng trên, dưới hoặc khoảng; quy tắc bằng biên, số chữ số và phạm vi áp dụng ghi rõ trong cấu hình/test.
- Không áp ngưỡng cho chỉ tiêu vận hành như Temp/Flow/P/O₂ nếu chưa có yêu cầu tương ứng.
- `oC` ↔ `°C`, chữ số thường ↔ chỉ số trên có thể chuẩn hóa cách viết; không coi việc đổi cách viết là quy đổi vật lý.
- PM: PDF hiển thị mg/m³, trong khi tài liệu TT10 và dữ liệu cũ có mg/Nm³. Cần chốt cơ sở đo; không đổi nhãn mà giữ nguyên số.
- NH₄⁺ và NH₄-N khác cơ sở báo cáo; chỉ áp dụng hệ số khi có phê duyệt, lưu provenance và tránh đổi lần hai.
- NOx không được tự thay bằng NO, NO₂ hay tổng số học khi chưa có công thức/cơ sở tính được duyệt.
- Tên quy chuẩn, hệ số và lời giải thích trong PDF cần được nhà máy duyệt lại; không sao chép nhận định pháp lý thành cam kết tuân thủ.

## 8. Phương án giao diện LED 1920 × 1200

### 8.1. Bố cục đề xuất

- Lưới 4 cột × 3 hàng, hai hàng khí thải và một hàng nước thải; đủ 12 thông số đồng thời.
- Header gọn: logo được duyệt, tên Công ty Nhiệt điện Cao Ngạn - TKV; dòng chi nhánh/địa chỉ nếu bắt buộc thì không lấn diện tích số đo.
- Tiêu đề nhóm **KHÍ THẢI** và **NƯỚC THẢI**, có mốc đo theo nhóm khi các thông số đồng bộ.
- Không dùng một dòng “số liệu lúc…” chung nếu khí và nước khác thời gian. Nhóm có nhiều mốc phải ghi khoảng/cảnh báo không đồng bộ; chẩn đoán luôn có timestamp từng thông số.
- Footer: chú giải màu/trạng thái và nội dung quy chuẩn đã được duyệt; không biến footer thành chữ nhỏ khó đọc.
- Đồng hồ hệ thống, nếu có, phải ghi rõ là giờ hiện tại, không giả làm giờ đo.

### 8.2. Ngân sách bố cục ban đầu

Đây là khung thử nghiệm, sẽ chỉnh theo đo chữ và thử LED, không phải tọa độ hardcode ở mọi nơi:

- Lề ngoài 24 px; vùng trong rộng 1872 px.
- Khoảng cách cột 16 px → mỗi ô rộng 456 px.
- Header: y=24…156, cao 132 px.
- Tiêu đề khí: y=172…220, cao 48 px.
- Hàng khí 1: y=232…470, cao 238 px.
- Hàng khí 2: y=482…720, cao 238 px.
- Tiêu đề nước: y=736…784, cao 48 px.
- Hàng nước: y=796…1034, cao 238 px.
- Footer: y=1050…1176, cao 126 px; còn lề dưới 24 px.
- Trong ô: ký hiệu và đơn vị ở dòng đầu, số đo lớn ở giữa, trạng thái rõ ở dưới.

### 8.3. Chữ, màu và khả năng đọc

- Nền đen, chữ trắng; số đang đo dùng trắng hoặc cyan sáng, không dùng xanh dương thuần `#0000FF`.
- Vượt ngưỡng: đỏ sáng + nhãn chữ; hiệu chuẩn: vàng sáng; lỗi/mất dữ liệu có nhãn tường minh, không chỉ đổi màu.
- Khởi điểm: số khoảng 104–120 px, tên 40–48 px, đơn vị 30–36 px, trạng thái 30–36 px; chữ đậm, không dùng các badge nhỏ từ màn 7 inch.
- Kiểm tra `Flow` ít nhất 6 chữ số, số âm, số thập phân và giá trị dài nhất dự kiến. Không elide số đo, không đổi thành ký pháp khoa học nếu chưa được duyệt.
- Ưu tiên điều chỉnh bố cục/precision được duyệt; không âm thầm thu số xuống cỡ nhỏ chỉ để lọt ô.
- Không gradient, shadow nặng, hiệu ứng nhấp nháy liên tục, marquee hoặc animation trang trí.
- Preview dùng cùng component và dữ liệu model với LED; dữ liệu mô phỏng luôn có nhãn **MÔ PHỎNG**, không được tự trộn vào luồng vận hành.

### 8.4. Màn hình Windows, DPI và mất kết nối

- Chọn màn bằng thông tin `QScreen`, lưu định danh khi có và fallback có kiểm soát; không giả định LED luôn là screen index 1.
- Hai cửa sổ cần Windows ở chế độ Extend. Khi Duplicate hoặc chỉ một màn, có chế độ bảo trì rõ ràng, không hứa có hai vùng hiển thị độc lập.
- LED không viền/toàn màn hình trên output đã chọn; cửa sổ cấu hình ở màn PC.
- Khung thiết kế giữ 1920 × 1200; quy đổi hợp lý sang tọa độ logical của Qt theo DPR/DPI, không nhầm kích thước cửa sổ logical với pixel tín hiệu.
- Kiểm thử Windows scaling 100/125/150%, EDID qua bộ quang, mất/cắm lại HDMI, sleep/wake và thay đổi thứ tự màn hình.
- Không tự đổi mode hiển thị hệ điều hành. Nếu tín hiệu không đúng chuẩn mục tiêu, cảnh báo và hướng dẫn cấu hình trước nghiệm thu.
- Khi LED biến mất: tiếp tục nhập dữ liệu, báo lỗi ở cửa sổ vận hành, không tự dời màn công khai che cửa sổ cấu hình. Khi trở lại, gắn đúng output theo chính sách đã lưu và tính lại stale.
- Không dùng chung overlay/login/popup của cửa sổ vận hành lên LED công khai.

## 9. Cấu hình, lưu trữ và vận hành 24/7

### 9.1. Cấu hình

- Một cấu hình JSON có `schemaVersion`, được validate đầy đủ và ghi atomic bằng `QSaveFile`.
- Nhóm cấu hình: branding, nguồn thư mục/trạm, alias thông số, đơn vị/precision, chu kỳ/stale, ngưỡng và căn cứ, màn hình đích, giới hạn log/checkpoint.
- Lưu backup cấu hình hợp lệ gần nhất; không tự bỏ qua cấu hình hỏng rồi chạy với ngưỡng mặc định giả.
- Kiểm tra mapping trùng, ngưỡng dưới lớn hơn ngưỡng trên, unit không tương thích, path thiếu quyền và màn hình không còn tồn tại.
- Áp dụng cấu hình theo snapshot: validate xong mới thay; không để một nửa cấu hình cũ và một nửa mới.
- Sửa ngưỡng/đơn vị/mapping phải có xác nhận và log thay đổi vận hành, không cần bê cả hệ tài khoản/RBAC của logger.
- Giới hạn cửa sổ cấu hình bằng tài khoản Windows/quyền local theo chính sách nhà máy; không coi phím tắt ẩn là bảo mật.

### 9.2. SQLite tối giản

- Chỉ lưu latest readings của các vị trí đang dùng, metadata/checkpoint file và một số chẩn đoán giới hạn.
- Không có bảng lịch sử quan trắc tăng vô hạn. Checkpoint có quota/retention cấu hình, ví dụ tối đa 50.000 file hoặc 30 ngày, phải đo lại theo nhịp thực tế.
- Prepared statements, transaction, connection đúng thread; schema có version và migration idempotent.
- Database nằm trên đĩa local, không đặt SQLite WAL trên UNC/share mạng.
- Khi khởi động, cache chỉ là dữ liệu phục hồi: hiển thị rõ trạng thái nguồn và tính lại độ mới; không đổi timestamp sang giờ khởi động.
- Lỗi DB/disk full: có chẩn đoán, tiếp tục xử lý RAM nếu an toàn, báo trạng thái không lưu bền; không crash loop hoặc tự xóa dữ liệu nguồn để lấy dung lượng.
- Database/log của app LED tách biệt hoàn toàn với `environmental_data.db` cũ và dữ liệu data-logger.

### 9.3. Khởi động và phục hồi

- Chặn hai instance bằng `QLockFile` trong vùng dữ liệu ứng dụng.
- Phương án tự khởi động: Windows Task Scheduler trong phiên người dùng tương tác, chạy khi đăng nhập và có chính sách restart khi process lỗi.
- Không chạy GUI như Windows service/Session 0; không tự cài autologon hoặc thay chính sách khóa màn hình.
- Task Scheduler không tự phát hiện mọi trường hợp app bị treo. Nếu nghiệm thu yêu cầu watchdog phát hiện hang thì dùng tác vụ giám sát riêng có heartbeat và retry giới hạn, được duyệt trước khi bật.
- Log có mức độ, rate limit lỗi lặp, rotation và trần tổng dung lượng; không log mật khẩu/token hoặc toàn bộ nội dung nguồn không cần thiết.
- Cửa sổ Diagnostics: đường dẫn nguồn, lần thấy file cuối, thời gian đo từng kênh, lỗi parse, trạng thái DB và màn đích; không mở API mạng chỉ để chẩn đoán.

## 10. Kế hoạch thực hiện theo giai đoạn

Mỗi giai đoạn là một đơn vị thay đổi kiểm thử được. Chỉ bắt đầu triển khai sau khi người dùng chuyển khỏi Plan mode và cho phép thực hiện.

### P0 — Khóa hợp đồng đầu vào và bộ mẫu

- [ ] Xác minh Windows/version/CPU/GPU, số màn, EDID và mode tín hiệu qua bộ quang.
- [ ] Xác nhận thư mục nhận file và ai có quyền cấu hình receiver; giải quyết app cũ move file khi chạy song song.
- [ ] Thu ít nhất một file khí và một file nước thực tế, thêm file lỗi/hiệu chuẩn nếu có; không sửa nguồn.
- [ ] Chốt station mapping, ký hiệu/encoding/đơn vị, chu kỳ, stale timeout và quy tắc correction.
- [ ] Chốt nguồn pH, cơ sở NH₄, PM, NOx và bộ ngưỡng/nội dung quy chuẩn được duyệt.
- [ ] Tạo fixture đã loại thông tin nhạy cảm; tách fixture mô phỏng với fixture thực.
- Đầu ra: hợp đồng nguồn + fixture + danh sách điểm còn thiếu; chưa có đủ dữ liệu thật vẫn có thể đi P1–P3 bằng fixture có nhãn.
- Điều kiện qua: không còn giả định ngầm rằng mọi file có 8 dòng, status thiếu là 00 hoặc mọi nguồn cùng timestamp.

### P1 — Khung dự án, tái sử dụng và build Windows sớm

- [ ] Tạo dự án riêng `APP_MAN_LED_QT`, module CMake tối giản, định danh app riêng.
- [ ] Pin Qt/toolchain và UI kit; chuyển AppPaths/LogSetup/font/helper cần thiết theo mục 3.
- [ ] Làm smoke window QML và cấu hình hai cửa sổ, dùng file đầu vào mẫu ở thư mục test.
- [ ] Viết test AppPaths/config/log và thiết lập CTest/qmllint.
- [ ] Build Release Windows và chạy smoke package trên máy không cài Qt trước khi xây nhiều chức năng.
- Đầu ra: bộ khung build được Linux/Windows, không kéo Modbus/Graphs/HTTP/logic Pi.
- Điều kiện qua: app không ghi vào Program Files/CWD và không đụng repository logger.

### P2 — Parser, mapping và kiểm tra chất lượng

- [ ] Viết Reading/types, Tt10TimeCodec, parser chuẩn 5 trường/dòng và source profile.
- [ ] Viết alias/unit validation, trạng thái unknown, xử lý placeholder và error diagnostics.
- [ ] Viết LatestReadingStore, conflict policy, freshness và ThresholdEvaluator.
- [ ] Test dữ liệu trễ, trùng, tương lai, thiếu dòng, lỗi/hiệu chuẩn và giá trị 0 hợp lệ.
- Đầu ra: thư viện logic độc lập UI, đầu vào → snapshot 12 vị trí có test.
- Điều kiện qua: không có đường code biến calibration/fault/unknown/stale thành trong giới hạn.

### P3 — Giao diện LED và preview

- [ ] Dựng LedBoard/ParameterTile/LedStatusLabel theo khung 1920 × 1200.
- [ ] Cố định thứ tự 12 thông số và chia nhóm khí/nước; branding/footer do cấu hình cấp.
- [ ] Gắn LedParameterModel, không đặt nghiệp vụ vào JavaScript QML.
- [ ] Tạo bộ cảnh mô phỏng: bình thường, vượt, chưa có ngưỡng, hiệu chuẩn, lỗi, stale, thiếu pH, flow 6 chữ số.
- [ ] Kiểm tra cỡ chữ, không tràn, không elide số, tương phản và đồng hồ/mốc đo rõ nghĩa.
- Đầu ra: preview đầy đủ trạng thái, có nhãn mô phỏng khi dùng fixture.
- Điều kiện qua: đủ 12 ô không cuộn; người dùng duyệt bố cục trước khi chốt hình thức.

### P4 — Đọc thư mục thật và phục hồi trạng thái

- [ ] FolderSourceWorker với watcher + periodic scan, file ổn định, retry, backpressure và reconnect.
- [ ] Snapshot/checkpoint SQLite tối giản, transaction và retention có trần.
- [ ] Thử trên bản sao thư mục thực, tên có tiếng Việt/khoảng trắng và cây thư mục theo ngày.
- [ ] Mô phỏng upload dở, file bị khóa Windows, rename, sửa file, copy lại, out-of-order, nhiều nguồn và nguồn mất quyền.
- [ ] Chứng minh nguồn không đổi tên/nội dung/mtime, không có file nào bị app xóa/move.
- Đầu ra: luồng file → LED chạy liên tục trên môi trường test, có phục hồi sau restart.
- Điều kiện qua: file gửi bù không làm lùi số hiện tại; một kênh mất cập nhật vẫn stale dù kênh khác còn chạy.

### P5 — Cấu hình, chẩn đoán và quản lý màn hình

- [ ] Operator window: chọn nguồn, mapping, precision/unit, ngưỡng, stale, branding và màn đích.
- [ ] Validate/atomic save/backup cấu hình, xác nhận thay đổi và diagnostics dễ đọc.
- [ ] Chọn và nhớ output, xử lý hotplug, Windows DPI và màn hình không còn tồn tại.
- [ ] Chặn hai instance, xử lý shutdown, DB/log hỏng, disk full, cache cũ và config lỗi.
- [ ] Không cho popup vận hành lọt sang LED công khai.
- Đầu ra: đủ chức năng vận hành tại chỗ, không cần sửa JSON bằng tay cho thao tác thường gặp.
- Điều kiện qua: thay cấu hình không gây nửa trạng thái cũ/mới; lỗi một nguồn không treo UI hoặc nguồn khác.

### P6 — Đóng gói và chạy bền Windows

- [ ] Đóng gói bằng cơ chế deploy QML của Qt hoặc `windeployqt --qmldir` tương ứng kit đã pin; có qwindows, QML imports, SQLite plugin và MSVC runtime cần thiết.
- [ ] Bản đầu ưu tiên thư mục Release portable/ZIP có version và checksum; không viết registry/service không cần thiết.
- [ ] Viết script/hướng dẫn Task Scheduler, rollback phiên bản và sao lưu cấu hình; chỉ cài khi được phép.
- [ ] Kiểm tra trên Windows sạch không có Qt/dev tools, đúng quyền tài khoản vận hành.
- [ ] Soak test ít nhất 72 giờ trên PC đích hoặc cấu hình tương đương, sau đó tiếp tục theo dõi pilot tại hiện trường.
- Đầu ra: gói chạy, dependency/license notices, kết quả build/test/soak, hướng dẫn triển khai.
- Điều kiện qua: không tăng RAM/log/DB vô hạn, không có I/O block UI, phục hồi sau reboot/crash và nguồn reconnect.

### P7 — Thử hiện trường, nghiệm thu và chuyển đổi

- [ ] Kiểm tra tín hiệu đúng 1920 × 1200, toàn vùng 1280 × 960 mm, không cắt/méo; xác nhận cách mapping nếu hai tỷ lệ khác nhau.
- [ ] Đối chiếu 12 kênh với file nguồn và thiết bị/biên bản đã được phép sử dụng, không chỉ so với app cũ.
- [ ] Thử đọc ở khoảng cách vận hành thống nhất, ban ngày/ban đêm; điều chỉnh kích thước chữ và độ sáng qua cấu hình được duyệt.
- [ ] Kiểm thử hiệu chuẩn/lỗi/mất file/stale bằng nguồn test hoặc thao tác được nhà máy phê duyệt, không cố gây lỗi cảm biến thật.
- [ ] Pilot trên luồng bản sao không tranh inbox với app cũ; chốt người phụ trách và cửa sổ chuyển đổi.
- [ ] Backup riêng app/config/database cũ, chuyển ứng dụng hiển thị, không đổi luồng truyền CEMS; rollback nếu có lỗi.
- [ ] Xác nhận đủ pH, đơn vị/ngưỡng/chú thích được duyệt trước khi nghiệm thu đủ nội dung.
- Đầu ra: biên bản test/duyệt hiển thị, cấu hình vận hành, gói bàn giao và phương án rollback.

## 11. Danh mục kiểm thử bắt buộc

### 11.1. Unit tests

- TT10: TAB/CRLF/LF/BOM; ngày giờ sai, năm nhuận, đúng UTC+7 khi máy khác timezone; ký hiệu/encoding/số lỗi; status thiếu/lạ; file một hoặc nhiều dòng.
- Profile: tên chuẩn không tiền tố, prefix legacy được bật riêng, nhóm 5 trường lặp chỉ khi cho phép; mapping sai trạm/nhập nhằng bị chặn.
- Chọn mới nhất: gửi bù, đảo thứ tự, trùng nội dung, khác nội dung cùng timestamp và timestamp tương lai.
- Trạng thái: mới hơn nhưng 01/02/invalid thay số tốt cũ; 00 không ngưỡng; 0 hợp lệ; NaN/Inf không hợp lệ.
- Ngưỡng: dưới/trên/bằng biên, pH theo khoảng, chưa có ngưỡng, sai unit, giá trị trước làm tròn; không suy ra so ngưỡng từ status thiết bị.
- Freshness: từng thông số độc lập, restart cache, đồng hồ nhảy, không file mới nhưng timer vẫn chuyển stale.
- Config/storage: schema version, atomic save, rollback config lỗi, transaction, database hỏng và checkpoint retention.

### 11.2. Integration và giao diện

- File write dở/khóa/rename/overwrite, mất quyền, UNC mất mạng, source reconnect và ngày mới tạo thư mục.
- App cũ tiêu thụ file: chứng minh nguồn bản sao/luồng phân phối đủ ổn định trước khi thử song song.
- QML smoke/snapshot các trạng thái, 12 vị trí cố định, số dài nhất, tiếng Việt và ký hiệu hóa học.
- Kiểm tra đủ package Windows, thiếu plugin/thiếu DLL có lỗi dễ chẩn đoán; chạy từ shortcut có Start in khác.
- 1/2 màn, Extend/Duplicate, DPI 100/125/150%, EDID converter, hotplug, sleep/wake, reboot và process crash.
- Theo dõi CPU/RAM/disk/UI latency trong soak test. Mục tiêu ban đầu: từ lúc file hoàn tất/được phát hiện đến render ≤5 giây trong điều kiện tải danh định; phải ghi rõ không phải độ trễ từ cảm biến đến file.
- Xác nhận không có listen port không cần thiết; quyền nguồn chỉ đọc; không in thông tin nhạy cảm vào log.

## 12. Điều kiện hoàn thành

- [ ] Đủ 12 thông số, đúng nhóm/thứ tự, không cuộn và đọc rõ trên LED thực.
- [ ] Chuẩn thiết kế 1920 × 1200, kiểm tra vùng vật lý 1280 × 960 mm và mapping được xác nhận.
- [ ] Giá trị/đơn vị/thời gian/status đối chiếu được với nguồn; không nhầm giờ nhận với giờ đo.
- [ ] Dữ liệu cũ, thiếu, hiệu chuẩn, lỗi, unit mismatch và conflict không xuất hiện như số bình thường đạt giới hạn.
- [ ] Ngưỡng và căn cứ áp dụng có người duyệt; thiếu ngưỡng không bị ghi đạt chuẩn.
- [ ] Nhập file chỉ đọc, không ảnh hưởng CEMS/receiver/bridge hoặc database cũ.
- [ ] Build/test Windows đạt, chạy bền và restart/reconnect đạt, bộ đóng gói chạy trên máy không có Qt.
- [ ] Có cấu hình, log, chẩn đoán, tài liệu vận hành và rollback; không mở rộng thành phần mềm logger/CEMS.
- [ ] Mã tái sử dụng có nguồn gốc/giấy phép rõ; repository data-logger và thay đổi chưa commit của người dùng được giữ nguyên.

## 13. Những thông tin còn phải xác nhận

Các mục này không ngăn việc xây khung/preview bằng fixture, nhưng là cổng kiểm soát trước tích hợp thật/nghiệm thu:

1. PC dùng Windows nào, 32/64-bit, GPU và có bao nhiêu màn? Có sẵn mode 1920 × 1200 qua chuỗi chuyển đổi không?
2. Thư mục nào đang nhận file MDATA, do phần mềm nào quản lý? App cũ có move/xóa file tại đó không?
3. Có thể cung cấp file khí/nước thực, mã trạm, nhịp xuất và trạng thái hiệu chuẩn/lỗi không?
4. pH lấy từ đâu? Nước thải sẽ đi qua nguồn file/bridge nào và trạng thái đã phản ánh đúng thiết bị chưa?
5. NH₄⁺ hay NH₄-N; PM theo mg/m³ hay mg/Nm³; NOx có sẵn hay cần quy đổi được duyệt?
6. Ngưỡng, hệ số/cơ sở đo, quy tắc bằng biên, tên quy chuẩn và lời chú thích nào được nhà máy phê duyệt?
7. Chu kỳ, stale timeout, dung sai giờ tương lai và xử lý correction cùng timestamp là gì?
8. Khoảng cách đọc/điều kiện ngày đêm, nội dung branding bắt buộc và tài khoản Windows vận hành là gì?

## 14. Tài liệu đã tham khảo khi lập kế hoạch

Trong workspace Cao Ngạn:

- `APP_MAN_LED_va_DATALOGGER_NUOC_THAI/LED DISPLAY.pdf`.
- `APP_MAN_LED_va_DATALOGGER_NUOC_THAI/PHAN_TICH_APP_MAN_LED.md`.
- `APP_MAN_LED_va_DATALOGGER_NUOC_THAI/README.md` và kết quả khảo sát bridge/WTW đã thảo luận trước đó.
- Thông số màn hình và sơ đồ kết nối người dùng đã xác nhận; tài liệu MDATA/hồ sơ thiết bị đã khảo sát ở các bước trước.

Trong dự án data-logger:

- `AGENTS.md`, `README.md`, `CHANGELOG.md`, CMake gốc/các layer và `packaging/qt_version.txt`.
- `docs/tt10-data-format.md`, các helper/model/writer/test được liệt kê trong mục 3.
- `shared/logger-ui-kit` — CMake, theme, components và LICENSE.
- Bản TT10 có sẵn tại `docs/catalogue/THAM_KHAO/thong tu 10-btnmt_2021.pdf`, Phụ lục 15, trang PDF 153–157; xác nhận 5 trường và mã trạng thái 00/01/02.

**Lưu ý:** báo cáo/app/bridge cũ là bằng chứng để hiểu hiện trạng, không phải chuẩn nghiệp vụ bắt buộc phải sao chép. Khi xung đột, ưu tiên phạm vi người dùng đã chốt, hợp đồng nguồn thực tế được duyệt và quy tắc không làm sai nghĩa số liệu.
