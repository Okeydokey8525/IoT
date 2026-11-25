# Kế hoạch slide báo cáo đồ án IoT – Nhóm 16

Canva: https://www.canva.com/design/DAHU5OiJ1zk (16:9, 1920×1080, mẫu HUIT). Nguồn nội dung: `Bao_cao/md/` (báo cáo đã hoàn thành 20/11/2026).

## 1. Rà soát 11 slide hiện có

| Trang | Hiện tại | Vấn đề | Đề xuất |
|---|---|---|---|
| 1 | Bìa: "Hệ thống IoT giám sát và cảnh báo bất thường điện năng…" | Tên khác báo cáo ("…tự ngắt máy móc khi quá tải") | Đổi tên cho khớp báo cáo/đề cương (chờ chốt) |
| 2 | Thành viên | Ổn | Giữ |
| 3 | Nội dung 5 phần | Ghi chú nói "dữ liệu và cách huấn luyện" (chép từ khóa luận) | Đổi 5 phần theo chương báo cáo, viết lại ghi chú |
| 3 (ảnh xem trước cũ) | Vấn đề thực tế (ảnh AI căn nhà) | Ảnh ổn; ảnh xem trước trang 3 có thể là bản cũ, cần kiểm tra lại | Giữ ảnh, viết lại ghi chú |
| 4 | Giải pháp đề xuất (ảnh AI) | Có app điện thoại, cloud, công tắc thông minh – hệ thống thật không có | Thay bằng sơ đồ kiến trúc thật (Hình 3.1) |
| 5 | Chức năng cốt lõi (ảnh AI, chữ tiếng Anh lặp 2 lần, "anomaly detection") | Không khớp: hệ thống phát hiện quá tải theo ngưỡng, không phải "anomaly detection" | Thay bằng sơ đồ 4 chức năng tự làm (Đo → Bảo vệ tại chỗ → Truyền MQTT → Giám sát/điều khiển) |
| 6 | Đối tượng và phạm vi (ảnh AI căn hộ) | Ảnh dùng được | Giữ ảnh, thêm 3–4 ý ngắn về phạm vi (dân dụng, mô phỏng Wokwi, ngưỡng 1.200 W) |
| 7 | Kiến trúc tổng thể (ảnh AI, cảm biến kẹp dòng, điện thoại) | Không khớp thiết kế thật | Thay bằng Hình 3.1 (draw.io) |
| 8 | Công nghệ và phần cứng (ảnh AI) | Có cảm biến kẹp dòng, máy chủ – không có trong mô phỏng | Thay bằng bảng công nghệ (Bảng 4.1 rút gọn) + mạch Wokwi (Hình 3.2) |
| 9 | Giới thiệu dụng cụ (bảng) | Có PZEM-004T, breadboard, "thiết bị điện tạo tải" – không dùng | Sửa bảng: ESP32, biến trở (thay cảm biến), LED đỏ, còi, relay 5V |
| 10 | Nguyên lý hoạt động (ảnh AI) | Có "Smart switch", điện thoại cảnh báo | Thay bằng máy trạng thái (Hình 3.3) |
| 11 | Dashboard dự kiến (ảnh AI số liệu giả) | Số liệu giả, không phải Dashboard thật | Thay bằng ảnh chụp Dashboard Node-RED thật |

Ghi chú: số trang trên slide bị lặp (hai trang "5", hai trang "8"); ghi chú lời nói trang 4–10 giống hệt nhau – sẽ viết lại riêng từng trang.

## 2. Bố cục đề xuất (~18 slide, khoảng 12–15 phút)

| # | Slide | Hình | Nguồn |
|---|---|---|---|
| 1 | Bìa | – | sửa chữ |
| 2 | Thành viên | – | giữ |
| 3 | Nội dung | – | sửa |
| 4 | Vấn đề thực tế | ảnh AI có sẵn | giữ |
| 5 | Mục tiêu và phạm vi | ảnh AI có sẵn (trang 7 cũ) | sửa |
| 6 | Kiến trúc 4 tầng | `hinh_3_1.png` | có sẵn |
| 7 | Công nghệ sử dụng | bảng + logo | gõ trong Canva |
| 8 | Mạch mô phỏng Wokwi + linh kiện | `hinh_3_2.png` + bảng | có sẵn |
| 9 | Máy trạng thái bảo vệ 3 mức | `hinh_3_3.png` | có sẵn |
| 10 | Firmware 2 luồng (điểm nhấn) | sơ đồ mới | **vẽ draw.io** |
| 11 | 4 topic MQTT | bảng ví dụ bản tin | gõ trong Canva |
| 12 | Luồng Node-RED + tính tiền điện | `hinh_3_4.png` | có sẵn |
| 13 | Dashboard thật | ảnh Dashboard khổ ngang | **chụp lại** (ảnh hiện tại khổ dọc 636×1932) |
| 14 | Ma trận kiểm thử T1–T11 | bảng tóm tắt | gõ trong Canva |
| 15 | Minh chứng T2–T3 | `hinh_5_2.png` | có sẵn |
| 16 | Minh chứng T5 + bài học tách luồng | `hinh_5_4.png` | có sẵn |
| 17 | Kết luận, hạn chế, hướng phát triển | – | gõ |
| 18 | Demo trực tiếp / Cảm ơn – Hỏi đáp | ảnh AI (tùy chọn) | ChatGPT |

## 3. Hình cần tạo mới

| Hình | Cách làm | Ai làm |
|---|---|---|
| Sơ đồ firmware 2 luồng | draw.io (đúng kỹ thuật, không dùng AI) | Claude |
| Sơ đồ 4 chức năng (slide 6 cũ) | draw.io | Claude |
| Dashboard khổ ngang 16:9 | chụp `http://localhost:1880/ui` cửa sổ 1600×900 khi Wokwi đang chạy | Claude |
| Ảnh bìa phụ / slide cảm ơn | ChatGPT (prompt bên dưới) | Bạn |
| (Tùy chọn) ảnh minh họa mục tiêu | ChatGPT | Bạn |

## 4. Prompt cho ChatGPT tạo ảnh

Quy tắc chung: ảnh không chứa chữ (AI viết tiếng Việt hay sai), không vẽ thiết bị mà hệ thống không có (điện thoại báo động, cloud, công tắc thông minh).

**Prompt A – Ảnh minh họa giải pháp (slide Mục tiêu, tùy chọn)**
```
Clean isometric 3D illustration, 16:9, white background, soft blue and orange accents.
A household power outlet feeding an electric kettle and an air conditioner through a small
DIY electronics box: an ESP32 development board, a blue 5V relay module, a small round buzzer
and a glowing red LED. Thin glowing data line from the box to a Wi-Fi symbol and then to a
laptop showing a simple gauge dashboard. Calm, educational, flat lighting.
No text, no letters, no logos, no smartphone, no cloud servers.
```

**Prompt B – Slide cảm ơn / hỏi đáp**
```
Minimal isometric illustration, 16:9, white background with soft blue gradient (#0B5CAD tones).
A cozy Vietnamese home living room at evening, warm light, appliances (fan, rice cooker, TV)
connected to a small protective electronics module on the wall with a green status LED,
conveying safety and energy awareness. Leave the right 40% of the image empty for text.
No text, no letters, no logos.
```

## 5. Quy trình chỉnh sửa
- Làm theo đợt 5–6 slide, mỗi đợt gửi ảnh xem trước, được duyệt mới lưu.
- Canva MCP không xóa / nhân bản trang được → bạn tự xóa hoặc nhân bản khi cần.
- Ghi chú lời nói (presenter notes) viết cho từng slide, lấy từ báo cáo.

## 6. Trạng thái 20/11/2026 (đã lưu trên Canva)
Nguyên tắc: slide chỉ chứa lý thuyết, thiết kế và phần dẫn vào demo; những gì demo trực tiếp được thì không đưa lên slide.

Thứ tự: 1 Bìa · 2 Thành viên · 3 Nội dung · 4 Vấn đề · 5 Giải pháp · 6 Cơ sở lý thuyết · 7 Kiến trúc 4 tầng · 8 Mạch Wokwi · 9 Máy trạng thái · 10 Firmware 2 luồng · 11 MQTT · 12 Luồng Node-RED · 13 Demo trực tiếp · 14 Kết quả kiểm thử 11/11 · 15 T5 · 16 T6 · 17 Kết quả · 18 Hạn chế, hướng phát triển · 19 Thank you (đã xóa 5 trang trùng demo; bản PDF: docs/slides/Slide_IoT_Nhom16.pdf).
Sơ đồ slide sinh bằng `Bao_cao/py/ve_so_do_drawio.py --slide` (slide_2_luong, slide_4_chuc_nang); ảnh Dashboard ghép 2 cột: `Bao_cao/hinh/slide_dashboard.png`.
