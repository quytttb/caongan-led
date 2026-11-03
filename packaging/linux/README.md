# Linux: build và đóng gói DEB

Gói `caongan-led_<version>_amd64.deb` tự chứa Qt 6.11.1 (thư viện, plugin nền tảng
`xcb`/`wayland`/`offscreen` và QML import) trong `/opt/caongan-led`. Lệnh
`/usr/bin/caongan-led` và desktop entry được cài kèm. Gói chỉ dành cho **amd64**,
build trên Ubuntu 24.04 với glibc ≥ 2.39.

## Build

Yêu cầu: source đầy đủ (kể cả submodule), Qt 6.11.1 `gcc_64`, CMake ≥ 3.24, Ninja,
`patchelf` và `dpkg-deb`.

```sh
CAONGAN_SOURCE_DIR="$PWD" \
CAONGAN_PACKAGE_VERSION=1.0.0 \
CAONGAN_PACKAGE_BUILD_DIR=/tmp/caongan-deb-build \
CAONGAN_PACKAGE_OUTPUT_DIR="$PWD/dist" \
QT_ROOT_DIR="$HOME/Qt/6.11.1/gcc_64" \
bash packaging/linux/build-deb.sh
```

`build-deb.sh` dùng project đóng gói riêng trong `packaging/linux`, thêm source tại
`CAONGAN_SOURCE_DIR` với `EXCLUDE_FROM_ALL`, build target `caongan_led` cấu hình
Release rồi chạy `cpack -G DEB`. `CAONGAN_PACKAGE_VERSION` phải trùng phiên bản ứng
dụng; CI lấy phiên bản từ CMake cache và kiểm tra trước khi build gói. Script từ chối
ghi đè `.deb`/`.sha256` đã có, nên dùng build/output directory mới cho mỗi lần build.

## Kiểm tra

```sh
bash packaging/linux/smoke-deb.sh \
  dist/caongan-led_1.0.0_amd64.deb /tmp/caongan-deb-smoke 1.0.0
```

Script giải nén gói vào thư mục tạm, kiểm tra version/architecture, sự hiện diện của
`caongan_led`, `qt.conf`, plugin `xcb`/`wayland`/`offscreen`/SQLite, chạy `ldd` để phát
hiện thư viện thiếu hoặc trỏ về Qt trên máy build, rồi chạy app offscreen với Qt chỉ
lấy từ gói. CI còn cài và gỡ gói trên Ubuntu 24.04 sạch trong Docker, chạy cả nền tảng
`offscreen` và `xcb` (qua Xvfb), và xác nhận dữ liệu ứng dụng không bị xóa khi gỡ.

## Giới hạn

- Chỉ amd64, Ubuntu 24.04 trở lên; không phải gói portable cho distribution khác.
- Chưa ký gói và chưa có APT repository; cài trực tiếp bằng
  `sudo apt install ./caongan-led_<version>_amd64.deb`.
- Gói phân phối Qt theo license hiện hành; xem `THIRD_PARTY_NOTICES.md` và
  `licenses/` trong gói Windows. Chưa thay thế kiểm tra trên máy Linux đích, màn hình
  LED/EDID hoặc chạy bền 72 giờ.
