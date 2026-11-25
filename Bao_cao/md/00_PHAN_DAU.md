<!-- Phần đầu báo cáo. Mỗi khối @@ là một trang trong Word, mỗi dòng là một đoạn. -->

@@TOM_TAT
Đồ án xây dựng hệ thống giám sát điện năng và tự ngắt tải khi quá tải cho các thiết bị điện dân dụng trong gia đình, theo kiến trúc IoT 4 tầng. Thiết bị biên dùng vi điều khiển ESP32, được mô phỏng trên nền tảng Wokwi với biến trở thay cho cảm biến dòng điện, cùng LED đỏ, còi báo và module relay.
ESP32 đọc tín hiệu tải mỗi 10 ms, lọc nhiễu bằng trung bình trượt, tính dòng điện, công suất và điện năng tích lũy. Logic bảo vệ là máy trạng thái 3 mức: cảnh báo sớm khi công suất vượt 1.200 W, ngắt relay và hú còi khi quá tải kéo dài quá 3 s, tự phục hồi khi công suất giảm dưới 1.100 W. Logic này chạy ngay trên thiết bị nên vẫn bảo vệ được khi mất kết nối mạng. Dữ liệu được gửi qua giao thức MQTT tới broker EMQX Cloud có tài khoản và mã hóa TLS; Node-RED nhận dữ liệu, hiển thị trên Dashboard, tính tiền điện theo giá bậc thang, ghi lịch sử ra file CSV và cho phép người dùng cắt hoặc bật nguồn từ xa.
Báo cáo trình bày cơ sở lý thuyết, phân tích yêu cầu, thiết kế, cách xây dựng và kết quả kiểm thử của hệ thống, cùng các hạn chế và hướng phát triển.
Từ khóa: IoT, ESP32, MQTT, Node-RED, quá tải điện, Wokwi.

@@LOI_CAM_ON
Nhóm xin gửi lời cảm ơn chân thành đến ThS. Võ Hoàng Hải, giảng viên học phần, đã hướng dẫn và góp ý cho nhóm trong suốt quá trình thực hiện đồ án.
Nhóm cũng cảm ơn quý thầy cô Khoa Công nghệ Thông tin, Trường Đại học Công Thương TP. Hồ Chí Minh đã truyền đạt kiến thức nền tảng để nhóm hoàn thành đề tài.
Do thời gian và kinh nghiệm còn hạn chế, báo cáo khó tránh khỏi thiếu sót. Nhóm mong nhận được góp ý của quý thầy cô để hoàn thiện hơn.
Nhóm xin chân thành cảm ơn.

@@LOI_CAM_DOAN
Nhóm sinh viên thực hiện đồ án “Hệ thống giám sát và cảnh báo quá tải điện năng thông minh” xin cam đoan:
1. Nội dung phân tích, thiết kế, mã nguồn và kết quả trình bày trong báo cáo do nhóm thực hiện.
2. Các tài liệu, thư viện và nguồn tham khảo được sử dụng đều được trích dẫn trong danh mục tài liệu tham khảo.
3. Nhóm chịu hoàn toàn trách nhiệm về tính trung thực của các nội dung trong báo cáo.

@@VIET_TAT
| Viết tắt | Tiếng Anh | Tiếng Việt |
|---|---|---|
| ADC | Analog-to-Digital Converter | Bộ chuyển đổi tương tự sang số |
| CSV | Comma-Separated Values | Tệp dữ liệu phân cách bằng dấu phẩy |
| EVN | Vietnam Electricity | Tập đoàn Điện lực Việt Nam |
| FSM | Finite State Machine | Máy trạng thái hữu hạn |
| GPIO | General Purpose Input/Output | Chân vào ra đa dụng |
| IoT | Internet of Things | Internet vạn vật |
| JSON | JavaScript Object Notation | Định dạng dữ liệu dạng văn bản |
| LED | Light-Emitting Diode | Đèn bán dẫn phát quang |
| LWT | Last Will and Testament | Bản tin di chúc của MQTT |
| MQTT | Message Queuing Telemetry Transport | Giao thức truyền tin nhẹ cho IoT |
| PWM | Pulse Width Modulation | Điều chế độ rộng xung |
| QoS | Quality of Service | Mức bảo đảm chuyển tin |
| UI | User Interface | Giao diện người dùng |
