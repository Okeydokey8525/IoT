# HỆ THỐNG IOT GIÁM SÁT VÀ CẢNH BÁO BẤT THƯỜNG ĐIỆN NĂNG CỦA THIẾT BỊ DÂN DỤNG

> **Trường:** Đại học Công Thương TP. Hồ Chí Minh (HUIT)  
> **Giảng viên hướng dẫn:** ThS. Võ Hoàng Hải  
> **Nhóm thực hiện:** Nhóm 16  
> - **Lê Đức Lương** - MSSV: `2001230490`  
> - **Trần Lê Đức Trí** - MSSV: `2001230486`  

---

## 1. Giới thiệu Đề tài

Dự án nghiên cứu và triển khai mô hình IoT giám sát mức tiêu thụ điện năng của các thiết bị dân dụng (nồi cơm điện, quạt, tivi, bình đun, máy giặt...) trong thời gian thực. Hệ thống tự động phân tích và phát hiện các tình huống bất thường (quá tải, chập cháy, quên tắt thiết bị) nhằm đưa ra cảnh báo kịp thời bằng âm thanh, ánh sáng và dữ liệu log.

---

## 2. Sơ đồ phần cứng & Nối dây (Wokwi Simulation)

Dự án được mô phỏng trên nền tảng **Wokwi** với các linh kiện được định nghĩa trong file [`diagram.json`](diagram.json):

| Linh kiện | Loại linh kiện (Wokwi Type) | Chân linh kiện | Chân ESP32 kết nối | Ghi chú chức năng |
| :--- | :--- | :--- | :--- | :--- |
| **ESP32** | `board-esp32-devkit-c-v4` | - | - | Vi điều khiển trung tâm |
| **Biến trở (Pot)** | `wokwi-potentiometer` | `GND` | `GND.1` | Cấp mass |
| | | `VCC` | `3V3` | Cấp nguồn 3.3V |
| | | `SIG` | `GPIO 34` | Mô phỏng tải tiêu thụ điện ($0 \to 2000\text{ W}$) |
| **Đèn LED đỏ** | `wokwi-led` | Katot (`C`) | `GND.2` | Cực âm |
| | | Anot (`A`) | `GPIO 25` | Báo động thị giác khi có bất thường |
| **Còi Buzzer** | `wokwi-buzzer` | Chân `1` | `GND.2` | Cực âm |
| | | Chân `2` | `GPIO 26` | Phát âm thanh báo động tần số cao qua PWM |
| **Serial Monitor** | `$serialMonitor` | `RX` | `esp:TX` (GPIO 1) | Xuất log dữ liệu ra Terminal |
| | | `TX` | `esp:RX` (GPIO 3) | Nhận lệnh điều khiển |

---

## 3. Nguyên lý Hoạt động & Công thức Tính toán

Mỗi giây ($1\text{s}$), hệ thống đọc và xử lý một chu kỳ mẫu:

1. **Đọc cảm biến dòng/công suất (Biến trở ADC):**
   $$\text{rawADC} = \text{analogRead}(34) \quad (0 \le \text{rawADC} \le 4095)$$
2. **Tính công suất tiêu thụ tức thời ($P$):**
   $$P = \frac{\text{rawADC}}{4095} \times 2000.0\text{ (W)}$$
3. **Tính dòng điện tải ($I$) với điện áp danh định $U = 220\text{ V}$:**
   $$I = \frac{P}{U} = \frac{P}{220.0}\text{ (A)}$$
4. **Tính điện năng tiêu thụ tích lũy ($E$ theo kWh):**
   $$E_{t} = E_{t-1} + \frac{P \times \Delta t}{3600 \times 1000}\text{ (kWh)}$$

---

## 4. Các Tình huống Cảnh báo Bất thường (Anomaly Detection)

| Trạng thái | Điều kiện kích hoạt | Phản ứng phần cứng | Thông báo Serial |
| :--- | :--- | :--- | :--- |
| **BÌNH THƯỜNG (NORMAL)** | $P \le 1200\text{ W}$ và $t \le 25\text{s}$ | - LED Đỏ: **TẮT**<br>- Còi Buzzer: **TẮT** | `[OK] BINH THUONG` |
| **QUÁ TẢI (OVERLOAD)** | $P > 1200\text{ W}$ | - LED Đỏ: **SÁNG LIÊN TỤC**<br>- Còi: **KÊU LIÊN TỤC ($1500\text{ Hz}$)** | `[!] QUA TAI (OVERLOAD)`<br>*Nguy cơ chập cháy!* |
| **CHẠY QUÁ LÂU (OVERTIME)** | $P > 200\text{ W}$ liên tục $> 25\text{s}$ | - LED Đỏ: **CHỚP NHÁY**<br>- Còi: **KÊU NGẮT QUÃNG ($800\text{ Hz}$)** | `[*] CANH BAO: CHAY QUA LAU`<br>*Kiểm tra quên tắt thiết bị!* |

---

## 5. Hướng dẫn Cài đặt & Chạy Mô phỏng (Dành cho Người mới)

### Bước 1: Chuẩn bị Môi trường
Cần cài đặt sẵn trên máy tính:
- **Visual Studio Code** (hoặc Antigravity IDE).
- Extension **PlatformIO IDE** (để build code ESP32).
- Extension **Wokwi Simulator** (để chạy mạch mô phỏng).

> **Lưu ý về License Wokwi:** Khi dùng lần đầu, nhấn `Ctrl+Shift+P` $\to$ chọn `Wokwi: Request a 30-day Free License` và làm theo hướng dẫn trên web để kích hoạt.

### Bước 2: Biên dịch Firmware (Build)
Mở Terminal trong thư mục dự án (`c:\LeDucLuong\HK VII\IoT\Buoi04`), chạy lệnh:
```powershell
pio run
```
Khi thấy dòng `[SUCCESS]` nghĩa là file nạp `.pio/build/esp32dev/firmware.bin` và `firmware.elf` đã được tạo thành công.

### Bước 3: Khởi chạy Wokwi Simulator
1. Mở file [`diagram.json`](diagram.json).
2. Nhấn `Ctrl + Shift + P` (hoặc `F1`).
3. Gõ tìm kiếm: **`Wokwi: Start Simulator`** $\to$ nhấn **Enter**.
4. Tab mô phỏng Wokwi sẽ hiện ra kèm bảng mạch ESP32, biến trở, đèn LED và còi buzzer.

### Bước 4: Xem kết quả trên Terminal
Bạn có thể xem dữ liệu in ra theo 2 cách:
- **Cách 1 (Tự động):** Nhìn trực tiếp vào khung **"Wokwi Terminal"** ở tab Terminal phía dưới của IDE.
- **Cách 2 (PlatformIO Monitor qua RFC2217):** Mở một tab Terminal mới và chạy lệnh:
  ```powershell
  pio device monitor --port rfc2217://localhost:4444 --baud 115200
  ```

### Bước 5: Thử nghiệm tương tác
- Dùng chuột **xoay núm vặn của biến trở (Potentiometer)** trên màn hình:
  - Vặn lên mức cao ($> 60\%$) $\to$ Công suất vượt quá $1200\text{ W}$, đèn LED sáng đỏ và còi Buzzer kêu rú.
  - Vặn về mức trung bình ($\approx 500\text{ W}$) và giữ nguyên $> 25$ giây $\to$ Hệ thống kích hoạt báo động "Quên tắt thiết bị" (LED nhấp nháy, còi kêu từng hồi).
  - Vặn về mức $0\text{ W}$ $\to$ Hệ thống trở lại trạng thái an toàn.

---

## 6. Mẫu Dữ liệu Hiển thị (Serial Monitor)

```text
+=============================================================+
|      HE THONG GIAM SAT & CANH BAO BAT THUONG DIEN NANG      |
|              TRUONG DAI HOC CONG THUONG TP.HCM              |
|           Nhom 16: Le Duc Luong - Tran Le Duc Tri           |
+=============================================================+

+----------------------- BAN TIN GIAM SAT --------------------+
| Thoi gian chay: 00:00:15  |  Mau do: #15    | ADC: 2750 / 4095 |
+-------------------------------------------------------------+
|  Dien ap nguon (U)     :  220.0 V                             |
|  Dong dien tieu thu (I):   6.10 A                             |
|  Cong suat hien tai (P): 1343.1 W  (Max: 2000 W)              |
|  Dien nang tich luy    :   0.00559 kWh                        |
+-------------------------------------------------------------+
|  TRANG THAI            : [!] QUA TAI (OVERLOAD)             |
|  CHI TIET CANH BAO     : P > 1200W! Nguy co chap chay!      |
|  BAO DONG              : COI BUZZER & DEN LED DANG KICH HOAT|
+=============================================================+
```

---

## 7. Các Lỗi Thường Gặp & Cách Khắc Phục (Troubleshooting)

1. **Lỗi Serial Monitor không in chữ gì:**
   - **Nguyên nhân:** Wokwi chưa nối đường UART vào `$serialMonitor`.
   - **Khắc phục:** Đã bổ sung 2 dòng nối dây trong `diagram.json`:
     `["esp:TX", "$serialMonitor:RX", "", []]` và `["esp:RX", "$serialMonitor:TX", "", []]`.
2. **Lỗi cổng `listen EADDRINUSE :4000`:**
   - **Nguyên nhân:** Cổng 4000 bị trùng với tiến trình khác.
   - **Khắc phục:** Đã cấu hình sang cổng `4444` trong `wokwi.toml` (`rfc2217ServerPort = 4444`).
3. **Lỗi đỏ `LEDC is not initialized`:**
   - **Nguyên nhân:** Dùng hàm `noTone()` của Arduino gây lỗi driver khi còi chưa kêu.
   - **Khắc phục:** Chuyển sang dùng chuẩn PWM gốc của ESP32 (`ledcSetup`, `ledcAttachPin`, `ledcWriteTone`), triệt tiêu 100% lỗi đỏ.
4. **Lỗi chữ bị lệch bậc thang (stair-stepping):**
   - **Nguyên nhân:** In bằng `printf` chỉ có `\n` thiếu `\r`.
   - **Khắc phục:** Chuẩn hóa toàn bộ xuống dòng thành `\r\n`.
