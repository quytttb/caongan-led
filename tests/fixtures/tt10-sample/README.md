# Fixture TT10 mẫu

Các file trong thư mục này chỉ dùng cho unit test và smoke test. Không phải dữ liệu vận hành.

- `TNg_NDCN_KHITO1_20260101120000.txt`: 8 thông số khí thải, đủ 5 trường TAB, trạng thái `00`.
- `TNg_NDCN_NUONT1_20260101120000.txt`: 4 thông số nước thải, đủ 5 trường TAB, trạng thái `00`.
- `1_TNg_NDCN_KHITO1_20260101120500.txt`: file có tiền tố legacy `1_` (nguồn phải bật `allowLegacyPrefix`).
- `TNg_NDCN_KHITO1_20260101121000.txt`: một dòng có 10 trường (2 nhóm 5 trường), chỉ đọc khi nguồn bật `groupedFiveColumn`.

Thời gian trong tên file và trong bản ghi là giờ Việt Nam UTC+7.
