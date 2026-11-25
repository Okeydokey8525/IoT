# CHƯƠNG 5: KIỂM THỬ VÀ ĐÁNH GIÁ

## 5.1. Phương pháp kiểm thử

Hệ thống được kiểm thử thủ công theo kịch bản trên mô phỏng Wokwi bản web. Mỗi kịch bản mô tả thao tác cụ thể (vặn biến trở, đổi cấu hình mạng, gửi lệnh) và kết quả mong đợi. Kết quả được quan sát đồng thời ở ba nơi:

- Serial Monitor của Wokwi: trạng thái máy trạng thái, công suất, dòng điện, điện năng, trạng thái relay, Wi-Fi và MQTT, in mỗi 2 s.
- Trạng thái linh kiện trên mạch mô phỏng: LED đỏ, còi, đèn báo của relay.
- Phía nhận bản tin: công cụ MQTT client đăng ký topic nhom16/bai21/# và Node-RED Dashboard.

Biến trở trong Wokwi có giá trị từ 0 đến 1.023, tương ứng công suất từ 0 đến 3.000 W. Ngưỡng 1.200 W ứng với khoảng 40% hành trình biến trở, ngưỡng phục hồi 1.100 W ứng với khoảng 36%.

## 5.2. Kịch bản kiểm thử

BANG: Bảng 5.1: Ma trận kiểm thử nghiệm thu
| Mã | Kịch bản | Thao tác | Kết quả mong đợi |
|---|---|---|---|
| T1 | Gửi dữ liệu định kỳ | Đăng ký topic nhom16/bai21/# trên MQTT client | Nhận bản tin JSON đủ 6 trường U, I, P, E, relay, state mỗi 2 s; topic status là online |
| T2 | Quá tải tức thời | Vặn biến trở quá 40% (P > 1.200 W) | LED đỏ sáng, relay vẫn đóng, state là WARNING |
| T3 | Quá tải kéo dài | Giữ mức quá tải hơn 3 s | Còi hú, relay ngắt, đúng 1 bản tin trên topic alarm |
| T4 | Phục hồi | Hạ biến trở dưới 36% (P < 1.100 W) | Trở về NORMAL, relay đóng lại |
| T5 | Mất mạng khi quá tải | Làm mất kết nối broker, giữ quá tải hơn 3 s | Relay vẫn ngắt tại chỗ, máy trạng thái vẫn chạy |
| T6 | Tự kết nối lại | Khôi phục kết nối mạng | In “Ket noi thanh cong!”, dữ liệu gửi lại bình thường, không khởi động lại thiết bị |
| T7 | Nhận lệnh tắt từ xa | Gửi {"cmd":"relay_off"} vào topic control | Relay ngắt và không tự đóng lại |
| T8 | Lệnh đặt lại từ xa | Gửi {"cmd":"reset"} vào topic control | Về NORMAL, relay đóng, hệ thống trở lại chế độ tự động |
| T9 | Hiển thị Dashboard | Mở http://localhost:1880/ui khi mô phỏng đang chạy | Đồng hồ, biểu đồ, điện năng, tiền điện, trạng thái cập nhật theo dữ liệu |
| T10 | Nút điều khiển trên Dashboard | Bấm “Cắt tải khẩn cấp” rồi “Bật / khôi phục nguồn” | Relay ngắt rồi đóng lại theo nút bấm |
| T11 | Ghi lịch sử CSV | Để hệ thống chạy vài phút | File /data/power_log.csv có thêm 1 dòng mỗi 30 s |

## 5.3. Kết quả kiểm thử

BANG: Bảng 5.2: Kết quả kiểm thử
| Mã | Kết quả | Ghi chú |
|---|---|---|
| T1 | Đạt | Nhận đủ 6 trường, topic status là online (Hình 5.1). Theo thời gian thực các bản tin cách nhau khoảng 5 s vì Wokwi chạy ở khoảng 40% tốc độ thật; theo thời gian mô phỏng là 2 s |
| T2 | Đạt | Biến trở 55%, P = 1.648,4 W: LED sáng, chuyển WARNING lúc t = 13.447 ms, relay vẫn ON (Hình 5.2) |
| T3 | Đạt | Chuyển CRITICAL lúc t = 16.447 ms, đúng 3.000 ms sau WARNING; relay OFF, MQTT client nhận đúng 1 bản tin alarm với P_trip = 1.648,4 (Hình 5.2) |
| T4 | Đạt | Hạ biến trở về 35%, P = 1.049,8 W: chuyển CRITICAL → NORMAL, relay ON (Hình 5.3) |
| T5 | Đạt | Đổi tạm tên broker thành tên không tồn tại: MQTT: NO, mỗi lần thử kết nối thất bại đứng khoảng 5 s ở luồng mạng, nhưng WARNING lúc t = 19.304 ms và CRITICAL lúc t = 22.304 ms, vẫn đúng 3.000 ms, relay OFF (Hình 5.4) |
| T6 | Đạt | Tạm dừng mô phỏng khoảng 30 s để broker hủy phiên. Khi chạy tiếp, ESP32 tự kết nối lại, điện năng E tiếp tục cộng dồn từ giá trị cũ, chứng tỏ thiết bị không khởi động lại (Hình 5.5) |
| T7 | Đạt | Gửi relay_off từ MQTTX: relay OFF và giữ OFF dù đang ở NORMAL (Hình 5.6) |
| T8 | Đạt | Gửi reset: Serial in “Da Reset he thong ve NORMAL”, relay ON, dòng điện 4,77 A trở lại |
| T9 | Đạt | Kiểm tra ngày 19/11/2026: tải khoảng 607 W, Dashboard hiện 220 V, 2,76 A, trạng thái NORMAL và biểu đồ công suất (Hình 4.2) |
| T10 | Đạt | Bấm “Cắt tải khẩn cấp”: Serial in “Nhan lenh: relay_off”, relay OFF. Bấm “Bật / khôi phục nguồn”: nhận relay_on, relay ON, dòng điện 4,77 A trở lại |
| T11 | Đạt | File /data/power_log.csv có 25 dòng, các dòng cuối cách nhau khoảng 30–40 s (theo thời gian thực) và ghi đúng trạng thái CRITICAL, NORMAL. Thời gian trong file theo múi giờ UTC của container Docker, lệch 7 giờ so với giờ Việt Nam |

Các kịch bản được chạy lại ngày 20/11/2026 với broker.emqx.io, bản tin được xem bằng ứng dụng MQTTX. Lần chạy đầu với firmware một vòng lặp cho thấy ở T5 việc ngắt tải bị trễ vì lệnh kết nối broker đứng khoảng 5 s mỗi lần thử. Nhóm đã tách luồng bảo vệ khỏi luồng mạng (mục 4.2) rồi chạy lại T2–T8 và T10 với bản firmware cuối; các hình 4.1 và 5.2–5.6 lấy từ lần chạy này. Hình 5.1 và kết quả T11 lấy từ lần chạy trước khi tách luồng; phần gửi dữ liệu và luồng Node-RED không thay đổi giữa hai bản.

Ngày 24/11/2026, sau khi chuyển sang broker EMQX Cloud có tài khoản và TLS (mục 3.8), nhóm chạy lại T1–T8, gửi lệnh và xem bản tin bằng một MQTT client đăng nhập vào broker mới. Kết quả đều đạt: T1 nhận đủ bản tin telemetry và status online; T2–T3 chuyển WARNING lúc t = 12.081 ms, CRITICAL lúc t = 15.081 ms, đúng 3.000 ms, bản tin alarm gửi đúng một lần; T4 về NORMAL lúc t = 21.371 ms khi P = 1.084,2 W; T5 với tên broker không tồn tại vẫn chuyển WARNING lúc t = 9.081 ms và CRITICAL lúc t = 12.081 ms; T6 tạm dừng mô phỏng, broker gửi offline theo Last Will, khi chạy tiếp ESP32 tự kết nối lại và điện năng cộng tiếp từ giá trị cũ; T7 và T8 nhận đúng lệnh relay_off và reset. Node-RED nhận dữ liệu qua broker mới và tiếp tục ghi file CSV. Khi thử đăng nhập bằng một tài khoản không tồn tại, broker từ chối với thông báo “Not authorized”. Các hình trong chương này lấy từ lần chạy ngày 20/11/2026 với broker công cộng; nhật ký của lần chạy mới chỉ khác ở địa chỉ broker.

HINH: Hình 5.1: T1 – Dữ liệu định kỳ nhận được trên MQTT client

HINH: Hình 5.2: T2, T3 – Chuyển từ WARNING sang CRITICAL, ngắt relay và gửi cảnh báo

HINH: Hình 5.3: T4 – Phục hồi về NORMAL khi giảm tải

HINH: Hình 5.4: T5 – Vẫn ngắt tải khi mất kết nối broker

HINH: Hình 5.5: T6 – Tự kết nối lại sau khi có mạng

HINH: Hình 5.6: T7 – Thực hiện lệnh tắt relay từ xa

## 5.4. Hướng dẫn cài đặt và chạy

Chạy mô phỏng trên Wokwi web:

- Mở dự án tại https://wokwi.com/projects/477826305121504257.
- Bấm nút Start để chạy. ESP32 tự kết nối Wi-Fi ảo Wokwi-GUEST và broker EMQX Cloud. Dự án Wokwi cần có thêm tab secrets.h chứa thông tin đăng nhập broker.
- Vặn biến trở để thay đổi mức tải, theo dõi kết quả trên Serial Monitor.

Biên dịch firmware trong Visual Studio Code:

- Cài tiện ích PlatformIO IDE và Wokwi Simulator.
- Mở thư mục dự án, chạy lệnh `pio run` để biên dịch. PlatformIO tự tải thư viện PubSubClient và ArduinoJson.
- Mở tệp diagram.json và chọn lệnh “Wokwi: Start Simulator” để mô phỏng trong VS Code.

Chạy Node-RED:

- Chạy Node-RED bằng Docker, mở trình soạn luồng tại http://localhost:1880.
- Cài gói node-red-dashboard, nhập tệp node-red/flows.json và bấm Deploy.
- Mở Dashboard tại http://localhost:1880/ui.

## 5.5. Đánh giá theo mục tiêu

BANG: Bảng 5.3: Đối chiếu kết quả với yêu cầu đề tài
| Yêu cầu | Mức đạt | Ghi chú |
|---|---|---|
| Đo dòng điện và điện áp | Một phần | Dòng điện được tính từ công suất mô phỏng; điện áp là hằng số 220 V, chưa có cảm biến điện áp |
| Tự ngắt khi quá tải | Đạt | Ngắt sau 3 s quá tải, có cảnh báo sớm và hysteresis |
| Tự ngắt khi đoản mạch | Chưa đạt | Chưa có mức ngắt tức thời khi dòng tăng đột biến; mọi trường hợp quá tải đều chờ 3 s |
| Relay và còi cảnh báo tại chỗ | Đạt | Relay, còi 1.500 Hz và LED đỏ hoạt động theo trạng thái |
| ESP32 → MQTT → Node-RED | Đạt | Dữ liệu, cảnh báo và lệnh điều khiển đi qua broker EMQX Cloud có tài khoản và TLS |
| Ứng dụng giám sát | Đạt | Dùng Node-RED Dashboard; chưa tích hợp Home Assistant như gợi ý trong đề |
| Bảo vệ khi mất mạng | Đạt | Luồng bảo vệ tách khỏi luồng mạng; khi không có broker vẫn ngắt đúng 3 s sau khi quá tải |
| Tính tiền điện, lưu lịch sử | Đạt | Tính theo giá bậc thang, ghi CSV mỗi 30 s |

## 5.6. Kết chương

Chương 5 đã trình bày phương pháp kiểm thử, 11 kịch bản kiểm thử, hướng dẫn cài đặt và đánh giá theo yêu cầu đề tài. Hệ thống đáp ứng các yêu cầu chính về giám sát, cảnh báo, tự ngắt khi quá tải và điều khiển từ xa. Các phần còn thiếu là đo điện áp thực, ngắt tức thời khi đoản mạch và tích hợp Home Assistant.
