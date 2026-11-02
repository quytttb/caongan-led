# Mẫu để thử, không phải số liệu quan trắc

`caongan-led.example.json` là cấu hình mặc định không có thư mục nguồn, không chọn màn và tắt toàn bộ ngưỡng. `inputs/` chứa file TT10 khí/nước giả với timestamp cố định **2026-01-01 12:00:00 UTC+7**, nên sẽ cũ theo đồng hồ thực tế. Chỉ đọc từ bản sao trong môi trường test; không phát như số liệu thật trên LED. Xem `docs/walkthrough.md` và `docs/input-contract.md`.
