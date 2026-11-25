# CHƯƠNG 3: THIẾT KẾ HỆ THỐNG

Chương này trình bày thiết kế của hệ thống theo kiến trúc IoT 4 tầng: kiến trúc tổng thể, phần cứng mô phỏng, cách tính các đại lượng điện, máy trạng thái bảo vệ, truyền thông MQTT, luồng xử lý trên Node-RED và giao diện Dashboard.

## 3.1. Kiến trúc tổng thể 4 tầng

Hệ thống chia thành 4 tầng, mỗi tầng có một nhiệm vụ riêng.

- Tầng cảm biến và thiết bị biên: ESP32 đọc tín hiệu tải, tính dòng điện, công suất và điện năng, tự quyết định đóng hoặc ngắt relay, bật LED và còi tại chỗ.
- Tầng mạng: ESP32 kết nối Wi-Fi và gửi, nhận bản tin JSON qua giao thức MQTT với broker EMQX Cloud Serverless, cổng 8883 có mã hóa TLS và yêu cầu tài khoản.
- Tầng xử lý trung gian: Node-RED chạy trong Docker, nhận dữ liệu từ broker, tách từng đại lượng, kiểm tra quá tải, tính tiền điện và ghi lịch sử ra file CSV.
- Tầng ứng dụng: Node-RED Dashboard hiển thị đồng hồ, biểu đồ, trạng thái và có nút điều khiển relay từ xa.

Điểm quan trọng nhất của thiết kế là quyết định ngắt tải nằm ở tầng biên. ESP32 không cần hỏi Node-RED hay broker trước khi ngắt relay. Vì vậy khi mất Wi-Fi hoặc broker gặp sự cố, thiết bị vẫn tự bảo vệ được. Các tầng phía trên chỉ dùng để giám sát, lưu trữ và điều khiển bổ sung.

HINH: Hình 3.1: Kiến trúc tổng thể 4 tầng của hệ thống

## 3.2. Thiết kế phần cứng

### 3.2.1. Sơ đồ mạch mô phỏng

Mạch được mô phỏng trên nền tảng Wokwi, gồm bo ESP32 DevKit C v4, một biến trở, một LED đỏ, một còi buzzer và một module relay. Biến trở đóng vai trò phụ tải giả lập: vặn biến trở làm thay đổi điện áp đưa vào chân ADC của ESP32, tương ứng với mức tiêu thụ điện của thiết bị trong nhà. Khi làm phần cứng thật, biến trở sẽ được thay bằng cảm biến dòng điện như ACS712 hoặc SCT-013 mà không phải đổi cấu trúc phần mềm.

HINH: Hình 3.2: Sơ đồ mạch mô phỏng trên Wokwi

### 3.2.2. Bảng kết nối chân

BANG: Bảng 3.1: Kết nối chân linh kiện với ESP32
| Linh kiện | Chân linh kiện | Chân ESP32 | Chức năng |
|---|---|---|---|
| Biến trở | VCC / GND | 3V3 / GND | Cấp nguồn 3,3 V cho biến trở |
| Biến trở | SIG | GPIO 34 | Tín hiệu tải đưa vào bộ ADC 12 bit |
| LED đỏ | Anot / Katot | GPIO 25 / GND | Báo động bằng ánh sáng |
| Còi buzzer | Chân 2 / Chân 1 | GPIO 26 / GND | Phát âm báo động 1.500 Hz bằng PWM |
| Relay | IN | GPIO 33 | Mức cao đóng điện cho tải, mức thấp ngắt tải |
| Relay | VCC / GND | 5V / GND | Cấp nguồn cho cuộn hút relay |
| Serial Monitor | RX / TX | TX / RX | Xuất nhật ký hoạt động ra màn hình |

## 3.3. Công thức tính dòng, áp, công suất và điện năng

ESP32 lấy mẫu chân ADC mỗi 10 ms. Giá trị ADC nằm trong khoảng 0 đến 4.095. Để loại bỏ dao động và xung nhiễu, mỗi mẫu mới được đưa vào bộ lọc trung bình trượt 8 mẫu, nghĩa là giá trị dùng để tính luôn là trung bình của 80 ms gần nhất.

Từ giá trị ADC đã lọc, hệ thống tính các đại lượng như sau:

- Công suất tiêu thụ: P = (ADC trung bình / 4.095) × 3.000 W. Thang đo từ 0 đến 3.000 W, đủ bao phủ các thiết bị gia dụng phổ biến như nồi cơm điện, ấm đun, máy sấy tóc.
- Điện áp: U = 220 V, là điện áp danh định của lưới điện sinh hoạt. Mạch mô phỏng chưa có cảm biến điện áp nên giá trị này là hằng số.
- Dòng điện: I = P / U khi relay đang đóng, và I = 0 khi relay đã ngắt vì lúc đó tải không còn được cấp điện.
- Điện năng tích lũy: sau mỗi lần lấy mẫu, E được cộng thêm P × Δt, quy đổi ra kWh. Điện năng chỉ được cộng khi relay đang đóng, nên con số này phản ánh đúng lượng điện tải đã thực sự dùng.

BANG: Bảng 3.2: Các ngưỡng và thông số bảo vệ
| Thông số | Giá trị | Ý nghĩa |
|---|---|---|
| Chu kỳ lấy mẫu | 10 ms | Thời gian giữa hai lần đọc ADC |
| Bộ lọc | 8 mẫu | Trung bình trượt, chống nhiễu |
| Ngưỡng cảnh báo | 1.200 W (khoảng 5,45 A) | Vượt ngưỡng thì chuyển sang WARNING |
| Thời gian trễ | 3 s | Quá tải kéo dài quá thời gian này thì ngắt tải |
| Ngưỡng phục hồi | 1.100 W | Công suất xuống dưới mức này mới trở về NORMAL |
| Tần số còi | 1.500 Hz | Âm báo động khi ngắt tải |

## 3.4. Thiết kế máy trạng thái bảo vệ

Logic bảo vệ được thiết kế dưới dạng máy trạng thái hữu hạn với 3 trạng thái.

- NORMAL: tải hoạt động trong giới hạn an toàn. LED và còi tắt, relay đóng.
- WARNING: công suất vừa vượt 1.200 W. LED đỏ sáng để báo sớm, còi chưa kêu và relay vẫn đóng. Hệ thống bắt đầu đếm thời gian.
- CRITICAL: công suất vượt 1.200 W liên tục quá 3 s. Còi hú 1.500 Hz, LED sáng, relay ngắt để bảo vệ, và một bản tin cảnh báo được gửi lên broker.

Thời gian trễ 3 s giúp hệ thống không ngắt nhầm khi thiết bị vừa khởi động. Một số thiết bị như máy bơm hay tủ lạnh có dòng khởi động cao trong thời gian ngắn rồi giảm xuống, giống cơ chế ngắt chậm của aptomat.

Ngưỡng phục hồi 1.100 W thấp hơn ngưỡng cảnh báo 1.200 W. Khoảng chênh 100 W này gọi là hysteresis. Nếu dùng chung một ngưỡng, khi công suất dao động quanh 1.200 W thì relay sẽ đóng ngắt liên tục, gây hỏng tiếp điểm và thiết bị.

Trong mỗi chu kỳ 10 ms, máy trạng thái chạy theo hai bước. Bước một chỉ xét điều kiện và đổi trạng thái. Bước hai mới bật tắt LED, còi và relay theo trạng thái vừa có. Cách tách này giúp trạng thái ghi trong nhật ký luôn khớp với trạng thái thực của relay.

HINH: Hình 3.3: Sơ đồ chuyển trạng thái NORMAL – WARNING – CRITICAL

BANG: Bảng 3.3: Điều kiện chuyển trạng thái
| Trạng thái hiện tại | Điều kiện | Trạng thái mới | Hành động |
|---|---|---|---|
| NORMAL | P > 1.200 W | WARNING | Bật LED, bắt đầu đếm giờ |
| WARNING | P < 1.100 W | NORMAL | Tắt LED |
| WARNING | Quá tải liên tục ≥ 3 s | CRITICAL | Bật còi, ngắt relay, gửi cảnh báo |
| CRITICAL | P < 1.100 W | NORMAL | Tắt còi và LED, đóng lại relay |

## 3.5. Thiết kế truyền thông MQTT

Hệ thống dùng 4 topic, cùng tiền tố nhom16/bai21. Dữ liệu được đóng gói dạng JSON.

BANG: Bảng 3.4: Danh sách topic MQTT
| Topic | Hướng | Tần suất | Nội dung |
|---|---|---|---|
| nhom16/bai21/telemetry | ESP32 → Broker | 2 s một lần | U, I, P, E, trạng thái relay, trạng thái hệ thống |
| nhom16/bai21/alarm | ESP32 → Broker | Mỗi lần vào CRITICAL | Loại sự cố, công suất lúc ngắt, ngưỡng, thông báo |
| nhom16/bai21/control | Broker → ESP32 | Khi người dùng bấm nút | Lệnh relay_on, relay_off hoặc reset |
| nhom16/bai21/status | Broker giữ lại | Khi kết nối hoặc mất kết nối | online hoặc offline |

Bản tin cảnh báo được gửi đúng một lần mỗi khi hệ thống vào CRITICAL, không lặp lại mỗi chu kỳ. Nhờ vậy người dùng không bị dồn dập thông báo trùng.

Topic status dùng cơ chế Last Will and Testament của MQTT. Khi kết nối, ESP32 đăng ký với broker một bản tin “offline” và tự gửi “online”, cả hai đều được broker giữ lại. Nếu ESP32 mất nguồn đột ngột, broker tự công bố “offline”, nên bên giám sát biết thiết bị đã mất kết nối mà không cần chờ.

Thứ tự ưu tiên giữa điều khiển từ xa và bảo vệ tự động được thiết kế như sau:

- Khi nhận lệnh relay_on hoặc relay_off, ESP32 chuyển sang chế độ điều khiển thủ công. Ở chế độ này, máy trạng thái không tự đóng lại relay ở trạng thái NORMAL và WARNING, để lệnh của người dùng không bị ghi đè.
- Trạng thái CRITICAL luôn được ưu tiên cao nhất. Khi quá tải kéo dài, relay bị ngắt kể cả khi đang ở chế độ thủ công.
- Khi hệ thống phục hồi từ CRITICAL hoặc nhận lệnh reset, chế độ thủ công được hủy và máy trạng thái lấy lại quyền điều khiển.

## 3.6. Thiết kế luồng xử lý Node-RED

Luồng Node-RED gồm 3 nhánh xử lý dữ liệu và 1 nhánh điều khiển.

- Nhánh hiển thị: nút MQTT nhận topic telemetry, chuyển chuỗi JSON thành đối tượng, rồi nút hàm “Tách U, I, P” tách thành 5 đầu ra: công suất, điện áp, dòng điện, điện năng và trạng thái. Mỗi đầu ra nối tới một thành phần trên Dashboard.
- Nhánh cảnh báo: công suất đi qua nút so sánh, chỉ cho qua khi lớn hơn 1.200 W. Phép so sánh dùng kiểu số, vì so sánh kiểu chuỗi sẽ cho kết quả sai (ví dụ chuỗi “433.7” bị coi là lớn hơn “1200”).
- Nhánh lưu trữ: dữ liệu đi qua nút giới hạn tốc độ, chỉ giữ 1 bản tin mỗi 30 s và bỏ các bản tin ở giữa, rồi được ghép thành một dòng CSV gồm thời gian, U, I, P, E, trạng thái và ghi nối tiếp vào file /data/power_log.csv. Việc giới hạn tốc độ giúp file log không phình to và không ghi đĩa liên tục.
- Nhánh điều khiển: hai nút bấm trên Dashboard gửi lệnh relay_off và relay_on vào topic control với QoS 1, để lệnh được broker xác nhận đã nhận.

HINH: Hình 3.4: Luồng xử lý trên Node-RED

## 3.7. Thiết kế giao diện Dashboard

Dashboard có một trang “Home” với nhóm “Giám sát phụ tải”, gồm các thành phần sau.

BANG: Bảng 3.5: Thành phần giao diện Dashboard
| Thành phần | Loại | Nội dung hiển thị |
|---|---|---|
| Điện áp | Đồng hồ kim | 0 đến 250 V |
| Dòng điện phụ tải | Đồng hồ kim | 0 đến 20 A |
| Công suất tiêu thụ | Biểu đồ đường | Công suất theo thời gian, giữ dữ liệu 1 giờ gần nhất |
| Điện năng tiêu thụ | Ô chữ | Điện năng tích lũy, đơn vị kWh |
| Tiền điện tạm tính | Ô chữ | Số tiền theo giá bậc thang, đơn vị VNĐ |
| Trạng thái lưới điện | Ô chữ | NORMAL, WARNING hoặc CRITICAL |
| Cắt tải khẩn cấp | Nút bấm | Gửi lệnh relay_off |
| Bật / khôi phục nguồn | Nút bấm | Gửi lệnh relay_on |

## 3.8. Phân tích bảo mật theo 4 tầng

Hệ thống có chức năng điều khiển từ xa, nên ngoài việc đọc trộm dữ liệu, nguy cơ đáng lo hơn là người lạ gửi lệnh tắt hoặc bật nguồn. Nhóm xét lần lượt từng tầng của kiến trúc: tầng đó có thể bị tấn công thế nào, biện pháp thường dùng là gì và hệ thống hiện đã làm được đến đâu.

BANG: Bảng 3.6: Phân tích bảo mật theo 4 tầng
| Tầng | Nguy cơ | Biện pháp thường dùng | Hiện trạng của hệ thống |
|---|---|---|---|
| 1. Thiết bị biên | Kẻ xấu tiếp cận mạch: đọc firmware để lấy mật khẩu Wi-Fi và broker, nối tắt relay | Đặt mạch trong hộp kín, mã hóa bộ nhớ flash, khởi động an toàn (secure boot) | Chưa áp dụng vì Wokwi không mô phỏng các tính năng này; nguy cơ đòi hỏi tiếp cận vật lý nên ít xảy ra. Relay kích mức cao nên khi ESP32 hỏng hoặc mất nguồn, relay tự nhả và tải bị ngắt |
| 2. Mạng và truyền thông | Nghe lén bản tin; gửi lệnh giả vào topic control | Wi-Fi WPA2; broker có tài khoản và mật khẩu; mã hóa TLS (cổng 8883) | Đã áp dụng: broker EMQX Cloud yêu cầu tài khoản riêng cho ESP32 và Node-RED, mã hóa TLS cổng 8883, ESP32 kiểm tra chứng chỉ máy chủ. Firmware chỉ nhận đúng 3 lệnh hợp lệ |
| 3. Xử lý trung gian | Người lạ mở trang soạn luồng Node-RED để sửa hoặc xóa luồng | Đặt mật khẩu cho trang quản trị | Đã đặt tài khoản và mật khẩu (adminAuth) |
| 4. Ứng dụng | Người lạ mở Dashboard và bấm nút cắt tải | Đăng nhập trước khi xem Dashboard | Đã yêu cầu đăng nhập (httpNodeAuth) |

Tầng 2 là nơi dễ bị tấn công nhất, vì đây là kênh duy nhất cho phép điều khiển thiết bị từ xa. Ban đầu nhóm dùng broker công cộng broker.emqx.io, nên ai biết tên topic nhom16/bai21/control đều có thể gửi lệnh relay_off. Nhóm đã chuyển sang broker riêng EMQX Cloud Serverless với ba lớp bảo vệ. Thứ nhất, broker chỉ nhận kết nối có đúng tài khoản; ESP32 và Node-RED dùng hai tài khoản riêng. Thứ hai, mọi bản tin đi qua cổng 8883 được mã hóa TLS nên không thể nghe lén. Thứ ba, ESP32 nạp sẵn chứng chỉ gốc DigiCert Global Root CA và chỉ kết nối khi chứng chỉ của máy chủ do chứng chỉ gốc này ký, nên không bị lừa kết nối vào broker giả. Ngoài ra, thiết kế ưu tiên bảo vệ ở mục 3.5 vẫn giữ: lệnh relay_on không thể đóng điện lại khi hệ thống đang ở CRITICAL.

Với tầng 3 và 4, mật khẩu được khai báo trong file cấu hình settings.js của Node-RED, lưu dưới dạng băm bcrypt chứ không lưu chữ thường. Thông tin đăng nhập broker của ESP32 được tách ra file secrets.h riêng, không đưa lên kho mã nguồn.

## 3.9. Kết chương

Chương 3 đã trình bày thiết kế của hệ thống theo 4 tầng. Quyết định bảo vệ được đặt tại ESP32 bằng máy trạng thái có thời gian trễ và hysteresis, nên hệ thống vẫn an toàn khi mất mạng. Truyền thông dùng 4 topic MQTT, có cơ chế cảnh báo một lần và báo mất kết nối tự động. Node-RED đảm nhận hiển thị, cảnh báo, tính tiền và lưu lịch sử. Về bảo mật, tầng 2 dùng broker riêng có tài khoản và mã hóa TLS, tầng 3 và 4 có mật khẩu; tầng 1 cần phần cứng thật nên chỉ được phân tích. Chương 4 sẽ trình bày cách hiện thực các thiết kế này.
