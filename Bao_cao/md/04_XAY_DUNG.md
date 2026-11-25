# CHƯƠNG 4: XÂY DỰNG HỆ THỐNG

Chương này trình bày môi trường phát triển và cách hiện thực từng phần của hệ thống: firmware trên ESP32, luồng xử lý trên Node-RED và giao diện Dashboard.

## 4.1. Môi trường phát triển

BANG: Bảng 4.1: Công cụ và thư viện sử dụng
| Thành phần | Công cụ, phiên bản | Vai trò |
|---|---|---|
| Soạn và biên dịch firmware | Visual Studio Code, PlatformIO, nền tảng espressif32, framework Arduino | Viết code, quản lý thư viện, biên dịch |
| Mô phỏng mạch | Wokwi (bản web và tiện ích trong VS Code) | Chạy ESP32 ảo cùng biến trở, LED, còi, relay |
| Thư viện MQTT | PubSubClient 2.8 | Kết nối broker, gửi và nhận bản tin |
| Thư viện JSON | ArduinoJson 6.21 | Đóng gói và đọc dữ liệu JSON |
| Broker MQTT | EMQX Cloud Serverless, cổng 8883 (TLS) | Trung chuyển bản tin giữa ESP32 và Node-RED, có tài khoản và mã hóa |
| Xử lý trung gian | Node-RED chạy trong Docker, node-red-dashboard 3.6.6 | Xử lý dữ liệu, giao diện giám sát |
| Quản lý mã nguồn | Git | Lưu lịch sử thay đổi |

Firmware dùng hai thư viện chính: PubSubClient [5] cho MQTT và ArduinoJson [6] cho dữ liệu JSON. Mã nguồn firmware nằm trong một tệp duy nhất. Nhóm soạn và biên dịch trong VS Code bằng lệnh `pio run`, sau đó dán cùng tệp này lên dự án Wokwi web để chạy mô phỏng và demo. Ở lần biên dịch ngày 20/11/2026, firmware dùng 13,8% RAM (45.336 byte) và 58,8% bộ nhớ Flash (770.833 byte) của ESP32.

## 4.2. Firmware trên ESP32

Firmware chia thành hai luồng chạy song song nhờ hệ điều hành thời gian thực FreeRTOS có sẵn trên ESP32:

- Luồng bảo vệ: cứ 10 ms đọc cảm biến, chạy máy trạng thái và điều khiển LED, còi, relay. Luồng này có độ ưu tiên cao hơn và không gọi bất kỳ hàm mạng nào.
- Luồng mạng (vòng lặp chính của Arduino): duy trì Wi-Fi và MQTT, gửi dữ liệu mỗi 2 s, gửi cảnh báo, nhận lệnh và in nhật ký ra Serial.

Thiết kế ban đầu đặt cả hai phần trong cùng một vòng lặp. Khi chạy kịch bản T5, nhóm phát hiện lệnh kết nối tới broker của thư viện PubSubClient là lệnh chặn: nếu broker không tồn tại, mỗi lần thử làm vòng lặp đứng khoảng 5 s, kéo theo việc ngắt tải bị trễ. Tách luồng giải quyết vấn đề này: lệnh kết nối có đứng thì chỉ làm chậm việc gửi dữ liệu, còn luồng bảo vệ vẫn chạy đều mỗi 10 ms.

Hai luồng không gọi trực tiếp hàm của nhau mà trao đổi qua các biến cờ: luồng mạng đặt cờ lệnh điều khiển, luồng bảo vệ đặt cờ cảnh báo và ghi lại các lần chuyển trạng thái kèm thời điểm. Các số đo dùng chung được đọc ghi trong vùng găng để không bị đọc dở dang.

### 4.2.1. Đọc cảm biến và lọc nhiễu

Cứ mỗi 10 ms, firmware đọc giá trị ADC của chân nối biến trở và đưa vào bộ đệm vòng 8 phần tử. Trong 8 lần đọc đầu tiên, khi bộ đệm chưa đầy, giá trị trung bình chỉ tính trên số mẫu đã có, để công suất hiển thị đúng ngay khi khởi động. Từ giá trị trung bình, firmware tính công suất, dòng điện và cộng dồn điện năng theo các công thức ở mục 3.3. Phần cộng dồn điện năng dùng số thực độ chính xác kép, vì mỗi lần cộng chỉ thêm một lượng rất nhỏ cỡ phần triệu kWh.

### 4.2.2. Máy trạng thái bảo vệ tại chỗ

Máy trạng thái chạy trong luồng bảo vệ, được hiện thực bằng hai khối lựa chọn theo trạng thái nối tiếp nhau trong cùng một chu kỳ 10 ms. Khối thứ nhất xét công suất và thời gian để đổi trạng thái. Khối thứ hai điều khiển LED, còi và relay theo trạng thái mới.

Còi được điều khiển bằng PWM với tần số 1.500 Hz. Firmware ghi nhớ trạng thái còi lần trước và chỉ ra lệnh khi trạng thái thay đổi, để không gửi lặp lệnh PWM trong mỗi vòng lặp. Relay được điều khiển mức cao để đóng điện cho tải, mức thấp để ngắt.

Khi hệ thống vào CRITICAL lần đầu, luồng bảo vệ bật cờ “có cảnh báo chờ gửi” và ghi lại công suất lúc ngắt. Luồng mạng thấy cờ thì gửi đúng một bản tin cảnh báo lên broker; nếu lúc đó đang mất mạng, bản tin được gửi ngay khi kết nối lại. Cờ đánh dấu được xóa khi hệ thống quay về NORMAL, để lần quá tải sau lại được báo. Mỗi lần chuyển trạng thái cũng được in ra Serial kèm thời điểm, ví dụ “t=16447 ms: WARNING -> CRITICAL”, giúp đo được thời gian trễ ngắt khi kiểm thử.

### 4.2.3. Kết nối Wi-Fi và MQTT

ESP32 kết nối tới mạng Wi-Fi ảo Wokwi-GUEST của Wokwi. Hàm xử lý mạng chạy trong luồng mạng và làm việc theo thứ tự sau:

- Nếu chưa có Wi-Fi: cứ 5 s thử kết nối lại một lần rồi thoát ra.
- Nếu có Wi-Fi nhưng chưa nối được broker: cứ 5 s thử kết nối MQTT một lần, kèm đăng ký bản tin “offline” làm Last Will. Kết nối thành công thì gửi “online” và đăng ký nhận topic control. Thất bại thì in mã lỗi ra nhật ký.
- Nếu đã kết nối đủ: duy trì nhịp giữ kết nối với broker và nhận các bản tin gửi đến.

Kết nối tới broker dùng lớp kết nối có mã hóa TLS của ESP32 thay cho kết nối TCP thường. Firmware nạp sẵn chứng chỉ gốc DigiCert Global Root CA (hết hạn 10/11/2031) để kiểm tra máy chủ, và đăng nhập bằng tài khoản riêng của ESP32. Địa chỉ broker, tên và mật khẩu đăng nhập nằm trong file secrets.h tách riêng; file này không được đưa lên kho mã nguồn. Nếu sai mật khẩu, broker từ chối kết nối và firmware in mã lỗi ra nhật ký.

Vì việc đo và ngắt tải nằm ở luồng bảo vệ, khi mất mạng ESP32 vẫn đọc cảm biến và ngắt tải đúng 3 s sau khi quá tải. Khi có mạng trở lại, thiết bị tự kết nối lại mà không phải khởi động lại.

### 4.2.4. Gửi dữ liệu và nhận lệnh điều khiển

Cứ mỗi 2 s, firmware đóng gói U, I, P, E, trạng thái relay và trạng thái hệ thống thành một bản tin JSON và gửi lên topic telemetry. Các số được làm tròn trước khi gửi: dòng điện 2 chữ số thập phân, công suất 1 chữ số, điện năng 5 chữ số. Bộ đệm MQTT được nâng lên 512 byte để bản tin JSON không bị cắt. Cùng lúc đó, một dòng trạng thái được in ra Serial Monitor để theo dõi khi mô phỏng.

BANG: Bảng 4.2: Ví dụ bản tin gửi và nhận
| Topic | Ví dụ nội dung |
|---|---|
| telemetry | {"U":220,"I":1.25,"P":275.0,"E":0.00012,"relay":"ON","state":"NORMAL"} |
| alarm | {"event":"OVERLOAD_CRITICAL","P_trip":1477.7,"threshold":1200,"relay":"OFF","msg":"Qua tai keo dai qua 3 giay. Da ngat Relay!"} |
| control | {"cmd":"relay_off"} |
| status | online hoặc offline |

Khi nhận bản tin trên topic control, luồng mạng đọc trường cmd và đặt cờ lệnh; luồng bảo vệ thực hiện lệnh trong chu kỳ 10 ms kế tiếp. Có 3 lệnh. Lệnh relay_on và relay_off đóng hoặc ngắt relay, đồng thời bật chế độ điều khiển thủ công. Lệnh reset đưa hệ thống về NORMAL, tắt còi, đóng relay và trả quyền điều khiển cho máy trạng thái. Bản tin không đúng định dạng JSON sẽ bị bỏ qua và ghi lỗi ra nhật ký.

HINH: Hình 4.1: Nhật ký hoạt động của ESP32 trên Serial Monitor

## 4.3. Xử lý trên Node-RED

Node-RED được chạy bằng Docker trên máy của nhóm, giao diện soạn luồng ở địa chỉ http://localhost:1880. Toàn bộ luồng được xuất ra tệp flows.json để lưu cùng mã nguồn.

### 4.3.1. Nhận và tách dữ liệu

Nút MQTT đăng ký topic telemetry trên broker EMQX Cloud, kết nối qua cổng 8883 có TLS bằng tài khoản riêng của Node-RED. Bản tin nhận được đi qua nút JSON để thành đối tượng, rồi vào nút hàm “Tách U, I, P”. Nút hàm này có thêm hai lớp an toàn: nếu dữ liệu vẫn còn ở dạng chuỗi thì tự giải mã lại, và nếu thiếu trường nào thì dùng giá trị mặc định (ví dụ điện áp mặc định 220 V, trạng thái mặc định NORMAL). Điện năng được làm tròn 4 chữ số thập phân và gắn đơn vị kWh trước khi hiển thị.

### 4.3.2. Cảnh báo quá tải

Đầu ra công suất được nối vào nút so sánh với điều kiện lớn hơn 1.200, kiểu số. Bản tin vượt ngưỡng được đưa tới nút ghi nhật ký “Cảnh báo quá tải” ở thanh bên của Node-RED. Song song với đó, trạng thái WARNING hoặc CRITICAL do ESP32 gửi lên được hiển thị trực tiếp trên ô “Trạng thái lưới điện” của Dashboard.

### 4.3.3. Tính tiền điện theo bậc thang EVN

Nút hàm “Tính tiền điện” lấy số kWh tích lũy và tính tiền theo cách lũy tiến: phần điện năng thuộc bậc nào thì nhân với đơn giá của bậc đó, rồi cộng lại. Kết quả được làm tròn và định dạng theo kiểu tiền Việt Nam, ví dụ “15.200 VNĐ”.

BANG: Bảng 4.3: Đơn giá dùng để tính tiền điện
| Bậc | Mức sử dụng | Đơn giá (đồng/kWh) |
|---|---|---|
| 1 | 0 – 50 kWh | 1.893 |
| 2 | 51 – 100 kWh | 1.956 |
| 3 | 101 – 200 kWh | 2.271 |
| 4 | Trên 200 kWh | 2.860 |

Biểu giá điện sinh hoạt của EVN có 6 bậc. Nhóm gộp các bậc từ 201 kWh trở lên thành một bậc 2.860 đồng/kWh để đơn giản hóa, nên kết quả chỉ là tiền điện tạm tính, chưa gồm thuế giá trị gia tăng.

### 4.3.4. Ghi lịch sử ra file CSV

Từ nút JSON, dữ liệu đi qua nút giới hạn tốc độ 1 bản tin mỗi 30 s, các bản tin đến trong khoảng giữa bị bỏ. Nút hàm “Định dạng CSV” ghép thời gian theo định dạng Việt Nam cùng U, I, P, E và trạng thái thành một dòng, rồi nút ghi tệp nối dòng đó vào cuối file /data/power_log.csv. Thư mục /data nằm trong vùng lưu trữ của container Docker. Với tần suất này, mỗi giờ file chỉ tăng 120 dòng.

## 4.4. Giao diện Dashboard

Dashboard được mở ở địa chỉ http://localhost:1880/ui. Hai đồng hồ kim hiển thị điện áp và dòng điện, biểu đồ đường vẽ công suất theo thời gian, ba ô chữ hiển thị điện năng, tiền điện tạm tính và trạng thái hệ thống. Hai nút ở cuối trang cho phép người dùng chủ động cắt tải khẩn cấp hoặc khôi phục nguồn mà không cần có mặt tại thiết bị. Nút khôi phục nguồn có màu xanh lá để phân biệt rõ với nút cắt tải.

HINH: Hình 4.2: Giao diện Node-RED Dashboard

## 4.5. Kết chương

Chương 4 đã trình bày cách hiện thực hệ thống. Firmware tách luồng bảo vệ khỏi luồng mạng, nên việc ngắt tải không phụ thuộc vào kết nối. Node-RED đảm nhận tách dữ liệu, cảnh báo, tính tiền điện và ghi lịch sử, còn Dashboard cho phép giám sát và điều khiển từ xa. Chương 5 sẽ trình bày kết quả kiểm thử các chức năng này.
