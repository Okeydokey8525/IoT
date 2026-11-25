# Việc cần làm để hoàn thiện báo cáo

Cập nhật 20/11/2026: đã chạy T1–T8, T10, T11; firmware đã tách luồng bảo vệ khỏi luồng mạng (sửa lỗi trễ ngắt ở T5); hình 4.1, 5.1–5.6 và Bảng 5.2 đã xong.

## Đã xong
- [x] Firmware mới (2 luồng) trong `src/main.cpp`, đã dán vào trình soạn Wokwi và chạy đạt
- [x] Chụp hinh_4_1, hinh_5_1 … hinh_5_6 (lưu trong `hinh/`)
- [x] Kết quả T1–T11 đã điền vào `md/05_KIEM_THU_DANH_GIA.md`

## Còn lại (bạn tự làm)
- [x] Wokwi đã Save bản firmware TLS + tab secrets.h (kiểm tra 24/11/2026)
- [x] **Phân công**: đã điền Bảng 1.2 ngày 22/11/2026 (Lương: mạch Wokwi, Node-RED; Trí: firmware đo/bảo vệ, firmware mạng, kiểm thử; chung: phân tích, báo cáo, slide, báo cáo tuần, demo)
- [ ] (Tùy chọn) Commit thay đổi; bản firmware cũ đã sao lưu trong `_Luu_Tru_Ngoai/`

## Bảo mật (24/11/2026)
- [x] Node-RED: Dashboard yêu cầu đăng nhập (dùng chung tài khoản admin)
- [x] Broker riêng EMQX Cloud Serverless (TLS 8883 + tài khoản), firmware mới, chạy lại T1–T8 đạt
- [x] Báo cáo mục 3.8 + chương 1, 4, 5, 6; slide bảo mật mới (trang 13)
- [ ] Sau buổi báo cáo: đổi mật khẩu `esp32_nhom16` trên EMQX (vì tab secrets.h trên Wokwi công khai)
