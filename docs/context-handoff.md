# TÀI LIỆU CHUYỂN GIAO NGỮ CẢNH DỰ ÁN IoT (CONTEXT HANDOFF)

> **Hướng dẫn sử dụng cho AI tiếp nhận:** Hãy đọc toàn bộ tài liệu dưới đây để nắm bắt đầy đủ bối cảnh kỹ thuật, mã nguồn và tiến độ hiện tại của sinh viên. Tiếp tục đóng vai trò là người hướng dẫn kỹ thuật và cố vấn đồ án, giữ phong cách trả lời trực diện, thực tế, sát với kiến trúc hệ thống đã xây dựng.

---

## 1. Thông tin chung về Đề tài
* **Nhóm thực hiện:** Nhóm 16 (Số lượng: 2 thành viên)
* **STT Đề tài:** 21
* **Tên đề tài:** Hệ thống giám sát và cảnh báo quá tải điện năng thông minh
* **Tiến độ tổng thể:** Đạt khoảng **85%** (Đã hoàn thành triển khai kỹ thuật và báo cáo tiến độ đến hết **Tuần 11**, chuẩn bị tổng duyệt và bảo vệ ở **Tuần 12**).

---

## 2. Kiến trúc Hệ thống 4 Tầng & Tech Stack

### Tầng 1: Cảm biến & Thiết bị biên (Sensing / Physical Layer)
* **Nền tảng mô phỏng:** Wokwi (ESP32 DevKit v1).
* **Phần cứng mô phỏng:**
  * **Biến trở xoay (Potentiometer):** Nối vào chân ADC của ESP32, đóng vai trò là *phụ tải giả lập*. Vặn biến trở làm thay đổi điện áp ADC ($0 - 3.3\text{ V}$), ESP32 quy đổi sang công suất tiêu thụ $P \in [0, 2000]\text{ W}$. Điện áp lưới giả định ổn định $U = 220\text{ V}$, dòng điện tính theo công thức $I = \frac{P}{U}$.
  * **Cơ cấu chấp hành & Cảnh báo tại chỗ:** Module Relay (đóng/cắt tải), LED đỏ, Còi báo động (Buzzer).
* **Logic tại biên (Edge Safety & FSM):**
  * Máy trạng thái hữu hạn (FSM) gồm 3 mức: `NORMAL`, `WARNING` (tiệm cận ngưỡng, $800\text{ W} - 1200\text{ W}$), và `CRITICAL` (quá tải, $P > 1200\text{ W}$).
  * **An toàn độc lập:** Khi $P > 1200\text{ W}$, ESP32 tự động ngắt Relay (`relay: "OFF"`), bật LED đỏ và hú còi ngay lập tức tại biên mà không phụ thuộc vào độ trễ mạng.

### Tầng 2: Mạng & Truyền thông (Network / MQTT Layer)
* **MQTT Broker:** `broker.emqx.io` (Public Broker).
* **Định dạng dữ liệu:** JSON.
* **Các Topic sử dụng:**
  * **Uplink (Telemetry):** `nhom16/bai21/telemetry`
    * Ví dụ gói tin: `{"U": 220, "I": 1.97, "P": 433.7, "E": 0.00257, "relay": "ON", "state": "NORMAL"}`
  * **Downlink (Control):** `nhom16/bai21/control`
    * Lệnh ngắt khẩn cấp: `{"cmd": "relay_off"}`
    * Lệnh khôi phục nguồn: `{"cmd": "relay_on"}`

### Tầng 3: Xử lý trung gian & Tự động hóa (Middleware / Node-RED Layer)
* **Môi trường triển khai:** Node-RED chạy trên Docker Container (truy cập editor tại `http://localhost:1880`).
* **Các luồng xử lý (Flows) đã xây dựng:**
  1. **Luồng nhận và bóc tách Telemetry:**
     * `mqtt in` (`nhom16/bai21/telemetry`) $\rightarrow$ `json` (chuyển chuỗi sang Object) $\rightarrow$ Node `function` (**Tách U, I, P** có **5 cổng xuất**):
       * Cổng 1: Công suất $P$ (W)
       * Cổng 2: Điện áp $U$ (V)
       * Cổng 3: Dòng điện $I$ (A)
       * Cổng 4: Điện năng tích lũy $E$ (kWh, làm tròn 4 chữ số thập phân)
       * Cổng 5: Trạng thái lưới điện (`state`)
  2. **Luồng giám sát ngưỡng quá tải:**
     * Từ Cổng 1 ($P$) $\rightarrow$ Node `switch` (**Kiểm tra quá tải (>1200W)**) cấu hình điều kiện `> 1200` (kiểu dữ liệu `number`) $\rightarrow$ Kích hoạt cảnh báo (Debug / Toast Notification / Telegram Bot).
  3. **Luồng tính tiền điện bậc thang (Tuần 11):**
     * Từ Cổng 4 ($E$) $\rightarrow$ Node `function` (**Tính tiền điện**) áp dụng biểu giá điện sinh hoạt lũy tiến EVN (Bậc 1: 1.893 đ, Bậc 2: 1.956 đ, Bậc 3: 2.271 đ, Bậc 4+: 2.860 đ) $\rightarrow$ Xuất chuỗi định dạng tiền Việt (`xx.xxx VNĐ`).
  4. **Luồng lưu trữ dữ liệu lịch sử CSV (Tuần 11):**
     * Từ node `json` $\rightarrow$ Node `delay` (**limit 1 msg/30s**, chế độ *Rate Limit*, drop intermediate messages để chống nghẽn I/O) $\rightarrow$ Node `function` (**Định dạng CSV**: ghép `${time},${d.U},${d.I},${d.P},${d.E},${d.state}`) $\rightarrow$ Node `write file` (nhóm *storage*, ghi nối tiếp `append to file` vào `/data/power_log.csv`, bật `Add newline`).

### Tầng 4: Ứng dụng & Giao diện người dùng (Application / Dashboard Layer)
* **Giao diện Web:** Node-RED Dashboard (`http://localhost:1880/ui`).
* **Các thành phần UI thuộc Group `[Home] Giám sát phụ tải`:**
  * 2 Đồng hồ kim (`ui_gauge`): **Điện áp** ($0 - 250\text{ V}$) và **Dòng điện phụ tải** ($0 - 20\text{ A}$).
  * 1 Biểu đồ đường (`ui_chart`): **Công suất tiêu thụ (W)** theo thời gian thực.
  * 3 Thẻ hiển thị chữ (`ui_text`): **Điện năng tiêu thụ (E)** (`kWh`), **Tiền điện tạm tính** (`VNĐ`), và **Trạng thái lưới điện** (`NORMAL`/`WARNING`/`CRITICAL`).
  * 2 Nút điều khiển 2 chiều (`ui_button`): **CẮT TẢI KHẨN CẤP** (`relay_off`) và **BẬT / KHÔI PHỤC NGUỒN** (`relay_on`).
  * Cảnh báo đẩy: Pop-up `ui_toast` trên web và tin nhắn tự động qua Telegram Bot (`node-red-contrib-telegrambot`).

---

## 3. Lịch sử Tiến độ & Các lỗi kỹ thuật đã xử lý

### Tuần 10 (Hoàn thành 100%)
* **Công việc đã làm:** Nâng cấp node tách dữ liệu lên 5 cổng; bổ sung nút khôi phục nguồn (`relay_on`) hoàn thiện điều khiển 2 chiều; tích hợp cảnh báo Toast và Telegram Bot; hoàn thiện nội dung bảng báo cáo Excel Tuần 10.
* **Lỗi đã khắc phục:**
  * *Lỗi báo quá tải giả ở mức $433.7\text{ W}$:* Nguyên nhân do node `switch` để kiểu so sánh chuỗi (`a_z`) khiến `'433.7' > '1200'` theo bảng chữ cái, đồng thời dây đang cắm nhầm vào Cổng 2 (`<= 1200`). Đã chuyển sang kiểu số (`number`) và nối đúng Cổng 1 (`> 1200`).

### Tuần 11 (Hoàn thành 100% theo nguyên lý Pareto 80/20)
* **Công việc đã làm:**
  * Xây dựng module tính tiền điện bậc thang EVN hiển thị ra thẻ **Tiền điện tạm tính (VNĐ)**.
  * Xây dựng luồng tự động ghi log chuỗi thời gian ra `/data/power_log.csv` bằng node `write file` kết hợp bộ lọc `limit 1 msg/30s` để bảo vệ bộ nhớ Docker.
  * Hoàn thiện nội dung bảng báo cáo Excel Tuần 11 (6 đầu việc chi tiết, bảng kiểm tra kiến trúc 4 tầng, phân tích rủi ro nghẽn I/O / mất dữ liệu Docker Volume và giải pháp khắc phục).

---

## 4. Các việc cần làm tiếp theo (Tuần 12 - Nghiệm thu)
1. **Kiểm tra & Chụp ảnh minh chứng Tuần 11:** Đảm bảo đã bấm nút **Deploy** trên Node-RED; kiểm tra file `/data/power_log.csv` đã sinh dữ liệu sau 1 phút chạy; chụp ảnh Dashboard có mục tiền điện VNĐ và xuất file sao lưu `flows_week11.json`.
2. **Hoàn thiện Slide & Kịch bản Demo:** Chuẩn bị slide thuyết trình bám sát kiến trúc 4 tầng, quay video hoặc demo trực tiếp kịch bản: *Vận hành bình thường $\rightarrow$ Vặn biến trở quá $1200\text{ W}$ $\rightarrow$ ESP32 tự cắt Relay & hú còi + Báo động Dashboard/Telegram $\rightarrow$ Giảm tải & bấm nút khôi phục nguồn*.
3. **Luyện tập phản biện:** Chuẩn bị câu trả lời cho các câu hỏi về lý do dùng biến trở giả lập tải, cơ chế bảo vệ tại biên (Edge Safety) so với điều khiển qua Cloud/MQTT, và cách chống mất dữ liệu/nghẽn file log trên Docker.

---

## 5. Thông tin tham khảo thêm trong hội thoại
* Người dùng có hỏi về các dự án mã nguồn mở trên GitHub làm mô phỏng mạch điện / ESP32 dạng 3D trên web tương tự Wokwi:
  * **Velxio** (`davidmonterocrespo24/velxio` - >2.600 stars): Nền tảng mô phỏng đa bo mạch (ESP32, Arduino, STM32, RPi) mã nguồn mở thay thế Wokwi, hỗ trợ self-host bằng Docker.
  * **Kho lõi của Wokwi:** `wokwi/avr8js`, `wokwi/rp2040js`, `wokwi/wokwi-elements`.
  * **Simple Circuit Engine** (`SimonThuillier/simple-circuit-engine`): Thư viện mô phỏng mạch điện 3D bằng Three.js và TypeScript.