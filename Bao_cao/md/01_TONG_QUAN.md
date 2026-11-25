# CHƯƠNG 1: TỔNG QUAN VÀ CƠ SỞ LÝ THUYẾT

## 1.1. Đặt vấn đề

Trong mỗi gia đình có nhiều thiết bị điện công suất lớn như nồi cơm điện, ấm đun nước, bàn ủi, máy sấy tóc, máy giặt. Khi nhiều thiết bị cùng chạy trên một ổ cắm hoặc một nhánh dây, dòng điện có thể vượt quá khả năng chịu đựng của dây dẫn và ổ cắm. Dây nóng lên kéo dài làm lão hóa lớp cách điện, dễ dẫn tới chập điện và cháy nổ. Ngoài ra, việc quên tắt thiết bị khi ra khỏi nhà cũng gây lãng phí điện và tiềm ẩn nguy hiểm.

Aptomat trong nhà có thể ngắt mạch khi quá tải, nhưng thường được chọn theo tổng công suất của cả nhánh, không theo dõi được từng thiết bị. Aptomat cũng không cho người dùng biết đang tiêu thụ bao nhiêu điện, không cảnh báo sớm trước khi ngắt và không cho phép điều khiển từ xa. Người dùng chỉ biết có sự cố khi điện đã mất.

Từ thực tế đó, nhóm thực hiện đề tài “Hệ thống giám sát và cảnh báo quá tải điện năng thông minh”, áp dụng cho các thiết bị điện dân dụng trong gia đình.

## 1.2. Giải pháp đề xuất

Nhóm đề xuất một hệ thống IoT gồm hai phần chính:

- Thiết bị biên dùng vi điều khiển ESP32 gắn tại ổ cắm hoặc thiết bị cần bảo vệ. ESP32 liên tục đo mức tiêu thụ, tính dòng điện, công suất và điện năng. Khi phát hiện quá tải kéo dài, ESP32 tự ngắt relay, bật đèn và còi báo động ngay tại chỗ mà không cần chờ lệnh từ máy chủ.
- Hệ thống giám sát từ xa: ESP32 gửi dữ liệu qua giao thức MQTT lên một broker. Node-RED nhận dữ liệu, hiển thị trên Dashboard, tính tiền điện tạm tính và lưu lịch sử. Người dùng có thể xem tình trạng và bấm nút cắt hoặc bật nguồn từ xa.

Điểm chính của giải pháp là chức năng bảo vệ nằm hoàn toàn ở thiết bị biên. Khi mất Wi-Fi hoặc máy chủ gặp sự cố, thiết bị vẫn tự ngắt tải khi quá tải. Phần giám sát từ xa chỉ bổ sung thêm thông tin và khả năng điều khiển.

Do điều kiện thời gian và chi phí, đồ án được thực hiện bằng mô phỏng trên nền tảng Wokwi. Một biến trở được dùng thay cho cảm biến dòng điện để giả lập mức tải thay đổi.

## 1.3. Mục tiêu đề tài

BANG: Bảng 1.1: Mục tiêu của đề tài
| Mã | Mục tiêu | Cách đánh giá |
|---|---|---|
| MT1 | Đo và tính dòng điện, công suất, điện năng tiêu thụ của tải | Giá trị trên Serial Monitor và Dashboard thay đổi đúng theo mức tải |
| MT2 | Cảnh báo sớm khi tải vượt ngưỡng an toàn | LED đỏ sáng khi công suất vượt 1.200 W |
| MT3 | Tự ngắt tải khi quá tải kéo dài | Relay ngắt và còi hú sau 3 s quá tải liên tục |
| MT4 | Vẫn bảo vệ được khi mất kết nối mạng | Ngắt tải đúng khi Wi-Fi hoặc broker không hoạt động |
| MT5 | Gửi dữ liệu và cảnh báo lên máy chủ qua MQTT | Nhận được bản tin trên broker và Node-RED |
| MT6 | Giám sát và điều khiển từ xa qua Dashboard | Dashboard hiển thị số liệu, nút bấm điều khiển được relay |
| MT7 | Tính tiền điện tạm tính và lưu lịch sử | Dashboard hiện số tiền, file CSV có dữ liệu |

## 1.4. Đối tượng và phạm vi

Đối tượng của đề tài là các thiết bị điện dân dụng dùng điện lưới sinh hoạt 220 V trong gia đình, với công suất trong khoảng 0 đến 3.000 W.

Phạm vi thực hiện:

- Phần cứng được mô phỏng trên Wokwi, gồm ESP32, biến trở thay cho cảm biến dòng điện, LED, còi buzzer và module relay. Nhóm chưa lắp mạch thật và chưa đo trên tải thật.
- Điện áp lưới được giả định ổn định ở 220 V, chưa đo điện áp thực tế.
- Truyền thông qua broker MQTT EMQX Cloud Serverless (gói miễn phí), có tài khoản đăng nhập và mã hóa TLS.
- Node-RED và Dashboard chạy trên máy tính cá nhân bằng Docker, có đặt mật khẩu cho trang quản trị và Dashboard.

## 1.5. Phương pháp thực hiện và phân công

Nhóm thực hiện đồ án theo từng tuần, mỗi tuần hoàn thành một phần của kiến trúc 4 tầng rồi kiểm thử trước khi làm phần tiếp theo: mô phỏng mạch và đọc cảm biến, máy trạng thái bảo vệ, kết nối MQTT, luồng Node-RED và Dashboard, sau cùng là tính tiền điện và lưu lịch sử. Mỗi tuần nhóm lập báo cáo tiến độ gồm công việc, người phụ trách, minh chứng và rủi ro.

BANG: Bảng 1.2: Phân công công việc
| STT | Thành viên | Công việc |
|---|---|---|
| 1 | Lê Đức Lương (2001230490) | Thiết kế mạch mô phỏng trên Wokwi; xây dựng luồng Node-RED và Dashboard (tính tiền điện, ghi lịch sử CSV, chạy trong Docker). Cùng Trí: phân tích đề tài, viết báo cáo, làm slide, báo cáo tiến độ hằng tuần, demo và thuyết trình. |
| 2 | Trần Lê Đức Trí (2001230486) | Viết firmware đo và bảo vệ (lọc tín hiệu ADC, máy trạng thái 3 mức, điều khiển LED, còi, relay); viết firmware mạng (Wi-Fi, MQTT, tách hai luồng FreeRTOS); kiểm thử 11 kịch bản và chụp minh chứng. Cùng Lương: phân tích đề tài, viết báo cáo, làm slide, báo cáo tiến độ hằng tuần, demo và thuyết trình. |

## 1.6. Cơ sở lý thuyết và công nghệ

### 1.6.1. Kiến trúc IoT 4 tầng

Một hệ thống IoT thường được chia thành 4 tầng. Tầng cảm biến và thiết bị biên thu thập dữ liệu từ môi trường và tác động lại bằng cơ cấu chấp hành. Tầng mạng truyền dữ liệu giữa thiết bị và máy chủ. Tầng xử lý trung gian nhận, xử lý, lưu trữ dữ liệu và chạy các quy tắc tự động. Tầng ứng dụng là nơi người dùng xem thông tin và ra lệnh. Cách chia này giúp mỗi tầng có thể thay đổi công nghệ mà ít ảnh hưởng tới tầng khác.

### 1.6.2. Vi điều khiển ESP32

ESP32 là vi điều khiển của hãng Espressif, có hai nhân xử lý, tích hợp sẵn Wi-Fi và Bluetooth [2]. ESP32 có bộ chuyển đổi tương tự sang số 12 bit, cho giá trị từ 0 đến 4.095, và có bộ tạo xung PWM dùng để điều khiển còi. Nhờ có Wi-Fi tích hợp và giá thành thấp, ESP32 được dùng rộng rãi trong các thiết bị IoT. Đồ án lập trình ESP32 bằng framework Arduino.

### 1.6.3. Cảm biến dòng điện và cách mô phỏng bằng biến trở

Để đo dòng điện xoay chiều trong thực tế, có thể dùng cảm biến hiệu ứng Hall như ACS712 [7], mắc nối tiếp với tải, hoặc cảm biến biến dòng không tiếp xúc như SCT-013, kẹp bên ngoài dây dẫn. Cả hai loại đều cho ra một điện áp tỉ lệ với dòng điện, được đưa vào chân ADC của vi điều khiển.

Trong mô phỏng, nhóm dùng một biến trở nối giữa 3,3 V và GND, chân giữa đưa vào ADC. Vặn biến trở làm điện áp vào ADC thay đổi, tương tự đầu ra của cảm biến khi dòng tải thay đổi. Firmware quy đổi giá trị ADC thành công suất tiêu thụ. Khi chuyển sang phần cứng thật, chỉ cần thay biến trở bằng cảm biến và sửa công thức quy đổi.

### 1.6.4. Relay, còi báo và LED cảnh báo

Relay là công tắc điều khiển bằng điện: một tín hiệu nhỏ từ vi điều khiển làm đóng hoặc mở tiếp điểm chịu được dòng lớn, nhờ đó ESP32 có thể cấp hoặc ngắt điện cho thiết bị. Còi buzzer phát âm thanh khi được cấp xung PWM ở một tần số nhất định. LED đỏ dùng để báo hiệu bằng ánh sáng. Ba thành phần này tạo thành cơ cấu chấp hành và cảnh báo tại chỗ.

### 1.6.5. Giao thức MQTT

MQTT là giao thức truyền tin nhẹ theo mô hình xuất bản và đăng ký, được chuẩn hóa bởi OASIS [1]. Thiết bị không gửi tin trực tiếp cho nhau mà qua một máy chủ trung gian gọi là broker. Bên gửi xuất bản bản tin vào một chủ đề (topic), bên nhận đăng ký các topic mình quan tâm và broker chuyển tin tới. Một số khái niệm được dùng trong đồ án:

- QoS (Quality of Service): mức bảo đảm chuyển tin. QoS 0 gửi một lần không xác nhận, QoS 1 bảo đảm tin tới nơi ít nhất một lần.
- Retain: broker giữ lại bản tin cuối cùng của topic và gửi ngay cho bên mới đăng ký.
- Last Will and Testament (LWT): bản tin thiết bị đăng ký trước với broker khi kết nối. Nếu thiết bị mất kết nối đột ngột, broker tự gửi bản tin này để báo cho các bên khác.

Đồ án dùng broker EMQX Cloud Serverless [9] ở gói miễn phí, bắt buộc tài khoản và mã hóa TLS, cùng thư viện PubSubClient [5] trên ESP32.

### 1.6.6. Node-RED và Node-RED Dashboard

Node-RED là công cụ lập trình theo luồng, chạy trên Node.js [3]. Người dùng kéo thả các nút (node) như nhận MQTT, chuyển đổi JSON, hàm xử lý, ghi file, rồi nối chúng thành luồng xử lý dữ liệu. Các nút hàm cho phép viết thêm mã JavaScript khi cần xử lý riêng. Gói node-red-dashboard [10] bổ sung các nút giao diện như đồng hồ, biểu đồ, ô chữ, nút bấm, tạo thành trang web giám sát mà không cần lập trình giao diện riêng.

### 1.6.7. Nền tảng mô phỏng Wokwi và PlatformIO

Wokwi là trình mô phỏng vi điều khiển chạy trên trình duyệt, hỗ trợ ESP32 cùng nhiều linh kiện như biến trở, LED, còi, relay [4]. Mạch được mô tả trong tệp diagram.json. Wokwi có mạng Wi-Fi ảo kết nối ra Internet, nên ESP32 mô phỏng có thể giao tiếp với broker MQTT thật. Nhóm dùng Wokwi bản web để chạy và trình diễn, đồng thời dùng PlatformIO [8] trong Visual Studio Code để soạn, quản lý thư viện và biên dịch firmware.

### 1.6.8. Máy trạng thái hữu hạn, lọc trung bình trượt và hysteresis

Máy trạng thái hữu hạn mô tả hệ thống bằng một số trạng thái cố định và các điều kiện chuyển giữa chúng. Ở mỗi trạng thái, hệ thống có hành vi riêng. Cách này giúp logic bảo vệ rõ ràng, dễ kiểm tra từng trường hợp.

Bộ lọc trung bình trượt lấy trung bình N mẫu gần nhất thay cho từng mẫu riêng lẻ, giúp loại bỏ các xung nhiễu ngắn của tín hiệu đo.

Hysteresis là việc dùng hai ngưỡng khác nhau cho chiều tăng và chiều giảm. Khi giá trị đo dao động quanh một ngưỡng duy nhất, đầu ra sẽ bật tắt liên tục. Dùng ngưỡng phục hồi thấp hơn ngưỡng cảnh báo tạo ra một khoảng đệm, giúp relay không đóng ngắt liên tục.

## 1.7. Kết chương

Chương 1 đã nêu vấn đề quá tải điện trong gia đình, giải pháp IoT với chức năng bảo vệ đặt tại thiết bị biên, mục tiêu, phạm vi và các công nghệ sử dụng gồm ESP32, MQTT, Node-RED và Wokwi. Chương 2 sẽ phân tích các yêu cầu cụ thể của hệ thống.
