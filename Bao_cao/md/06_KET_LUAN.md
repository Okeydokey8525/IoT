# KẾT LUẬN VÀ HƯỚNG PHÁT TRIỂN

## Kết quả đạt được

Đồ án đã xây dựng được hệ thống giám sát điện năng và tự ngắt tải khi quá tải cho thiết bị điện dân dụng, theo kiến trúc IoT 4 tầng:

- Thiết bị biên ESP32 đọc tín hiệu tải mỗi 10 ms, lọc nhiễu bằng trung bình trượt 8 mẫu, tính dòng điện, công suất và điện năng tích lũy.
- Máy trạng thái 3 mức NORMAL, WARNING, CRITICAL với cảnh báo sớm, thời gian trễ 3 s trước khi ngắt và ngưỡng phục hồi thấp hơn ngưỡng cảnh báo để relay không đóng ngắt liên tục.
- Chức năng bảo vệ chạy ở luồng riêng trên thiết bị, tách khỏi luồng mạng, nên vẫn ngắt tải đúng 3 s khi mất broker; thiết bị tự kết nối lại khi có mạng mà không phải khởi động lại.
- Truyền thông MQTT với 4 topic: dữ liệu định kỳ, cảnh báo một lần, lệnh điều khiển và trạng thái kết nối dùng Last Will.
- Luồng Node-RED tách dữ liệu, cảnh báo quá tải, tính tiền điện theo giá bậc thang và ghi lịch sử CSV; Dashboard hiển thị số liệu và cho phép cắt hoặc bật nguồn từ xa.
- Bảo mật theo tầng: broker riêng có tài khoản và mã hóa TLS, ESP32 kiểm tra chứng chỉ máy chủ; trang soạn luồng Node-RED và Dashboard có mật khẩu.

## Hạn chế

- Hệ thống mới được mô phỏng trên Wokwi, chưa lắp mạch thật và chưa đo với tải thật.
- Chưa đo trực tiếp dòng điện và điện áp: biến trở mô phỏng công suất, điện áp giả định cố định 220 V.
- Chưa có mức ngắt tức thời khi đoản mạch.
- Bảo mật tầng thiết bị (mã hóa flash, khởi động an toàn) chưa thực hiện được vì cần phần cứng thật. Broker EMQX Cloud đang dùng gói miễn phí, chưa phân quyền theo từng topic cho từng tài khoản.
- Cảnh báo quá tải trên Node-RED mới hiển thị ở trạng thái Dashboard và khung gỡ lỗi, chưa gửi thông báo tới điện thoại người dùng.
- Biểu giá điện trong hàm tính tiền được gộp từ bậc 4 trở lên và cần cập nhật khi EVN điều chỉnh giá.
- Chưa tích hợp Home Assistant.

## Hướng phát triển

- Lắp mạch thật với cảm biến dòng ACS712 hoặc SCT-013 và cảm biến điện áp, hiệu chỉnh công thức theo số đo thực tế.
- Bổ sung mức bảo vệ đoản mạch: ngắt ngay khi dòng vượt nhiều lần định mức, và cảnh báo khi điện áp quá cao hoặc quá thấp.
- Phân quyền topic cho từng tài khoản trên broker; bật mã hóa flash và khởi động an toàn khi lắp mạch thật.
- Gửi cảnh báo qua Telegram Bot hoặc ứng dụng điện thoại.
- Tích hợp Home Assistant để quản lý chung với các thiết bị khác trong nhà.
- Đọc biểu giá điện từ cấu hình thay vì viết cố định trong mã, lưu lịch sử vào cơ sở dữ liệu để thống kê theo ngày, tháng.

# TÀI LIỆU THAM KHẢO

REF: [1] OASIS, “MQTT Version 3.1.1,” OASIS Standard, Oct. 2014. [Online]. Available: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html
REF: [2] Espressif Systems, “ESP32 Series Datasheet.” [Online]. Available: https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
REF: [3] OpenJS Foundation, “Node-RED Documentation.” [Online]. Available: https://nodered.org/docs/
REF: [4] Wokwi, “Wokwi Documentation.” [Online]. Available: https://docs.wokwi.com/
REF: [5] N. O’Leary, “PubSubClient – A client library for the Arduino that provides support for MQTT.” [Online]. Available: https://github.com/knolleary/pubsubclient
REF: [6] B. Blanchon, “ArduinoJson – Efficient JSON serialization for embedded C++.” [Online]. Available: https://arduinojson.org/
REF: [7] Allegro MicroSystems, “ACS712 – Fully Integrated, Hall-Effect-Based Linear Current Sensor IC,” Datasheet.
REF: [8] PlatformIO Labs, “PlatformIO Documentation.” [Online]. Available: https://docs.platformio.org/
REF: [9] EMQ Technologies, “EMQX Cloud Serverless.” [Online]. Available: https://www.emqx.com/en/cloud/serverless-mqtt
REF: [10] Node-RED, “node-red-dashboard.” [Online]. Available: https://flows.nodered.org/node/node-red-dashboard
