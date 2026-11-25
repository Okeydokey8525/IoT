# CHƯƠNG 2: PHÂN TÍCH YÊU CẦU HỆ THỐNG

## 2.1. Tác nhân

BANG: Bảng 2.1: Các tác nhân của hệ thống
| Tác nhân | Vai trò |
|---|---|
| Người dùng (chủ nhà) | Xem tình trạng tiêu thụ điện, nhận cảnh báo, bấm nút cắt hoặc bật nguồn từ xa |
| Thiết bị biên ESP32 | Đo tải, tự bảo vệ, gửi dữ liệu và nhận lệnh điều khiển |
| Broker MQTT | Trung chuyển bản tin giữa ESP32 và Node-RED, báo trạng thái kết nối của thiết bị |
| Máy chủ Node-RED | Xử lý dữ liệu, hiển thị Dashboard, tính tiền điện, lưu lịch sử |

## 2.2. Yêu cầu chức năng

BANG: Bảng 2.2: Danh sách yêu cầu chức năng
| Mã | Chức năng | Mô tả |
|---|---|---|
| CN01 | Đo và tính toán | Đọc tín hiệu tải mỗi 10 ms, lọc nhiễu, tính dòng điện, công suất và điện năng tích lũy |
| CN02 | Cảnh báo sớm | Bật LED đỏ khi công suất vượt ngưỡng cảnh báo, chưa ngắt tải |
| CN03 | Tự ngắt khi quá tải | Ngắt relay và hú còi khi quá tải kéo dài quá thời gian cho phép |
| CN04 | Tự phục hồi | Đóng lại relay khi công suất giảm xuống dưới ngưỡng phục hồi |
| CN05 | Gửi dữ liệu định kỳ | Gửi U, I, P, E và trạng thái lên broker mỗi 2 s |
| CN06 | Gửi cảnh báo sự cố | Gửi một bản tin cảnh báo mỗi lần hệ thống ngắt tải vì quá tải |
| CN07 | Báo trạng thái kết nối | Báo online khi kết nối, broker tự báo offline khi thiết bị mất kết nối |
| CN08 | Điều khiển từ xa | Nhận lệnh bật, tắt relay và đặt lại hệ thống từ người dùng |
| CN09 | Hiển thị Dashboard | Hiển thị điện áp, dòng điện, công suất theo thời gian, điện năng và trạng thái |
| CN10 | Tính tiền điện | Tính tiền điện tạm tính theo giá bậc thang từ điện năng tích lũy |
| CN11 | Lưu lịch sử | Ghi dữ liệu vào file CSV theo chu kỳ 30 s |

## 2.3. Các kịch bản sử dụng chính

### 2.3.1. Giám sát điện năng theo thời gian thực

Người dùng mở Dashboard trên trình duyệt. Mỗi 2 s, ESP32 gửi một bản tin dữ liệu, Node-RED tách từng đại lượng và cập nhật đồng hồ điện áp, đồng hồ dòng điện, biểu đồ công suất, ô điện năng, ô tiền điện tạm tính và ô trạng thái. Người dùng biết được thiết bị đang tiêu thụ bao nhiêu điện và đã dùng bao nhiêu tiền mà không cần có mặt tại chỗ.

### 2.3.2. Cảnh báo và tự ngắt khi quá tải

Khi người dùng bật thêm thiết bị làm tổng công suất vượt 1.200 W, ESP32 chuyển sang trạng thái cảnh báo: LED đỏ sáng, relay vẫn cấp điện. Nếu người dùng tắt bớt thiết bị và công suất xuống dưới 1.100 W, hệ thống trở lại bình thường. Nếu quá tải kéo dài quá 3 s, ESP32 ngắt relay, còi hú và gửi một bản tin cảnh báo. Dashboard hiển thị trạng thái CRITICAL. Khi tải được giảm xuống dưới 1.100 W, relay tự đóng lại.

### 2.3.3. Bảo vệ khi mất kết nối mạng

Khi Wi-Fi hoặc broker gặp sự cố, ESP32 vẫn tiếp tục đo và chạy logic bảo vệ như bình thường. Quá tải kéo dài vẫn làm relay ngắt ngay tại chỗ. Trong lúc đó ESP32 cứ 5 s thử kết nối lại một lần mà không làm gián đoạn việc đo. Khi có mạng trở lại, thiết bị tự kết nối lại và tiếp tục gửi dữ liệu.

### 2.3.4. Điều khiển relay từ xa

Người dùng bấm nút “Cắt tải khẩn cấp” trên Dashboard, ví dụ khi đi vắng mà nhớ ra quên tắt bàn ủi. Node-RED gửi lệnh relay_off qua broker, ESP32 ngắt relay và giữ nguyên trạng thái ngắt, không tự đóng lại. Khi muốn dùng lại, người dùng bấm nút “Bật / khôi phục nguồn” để gửi lệnh relay_on.

### 2.3.5. Tính tiền điện và lưu lịch sử

Từ điện năng tích lũy, Node-RED tính tiền điện tạm tính theo giá bậc thang và hiển thị trên Dashboard. Đồng thời, cứ 30 s Node-RED ghi một dòng dữ liệu gồm thời gian, U, I, P, E và trạng thái vào file CSV, để người dùng xem lại hoặc phân tích sau.

## 2.4. Quy tắc nghiệp vụ

BANG: Bảng 2.3: Quy tắc nghiệp vụ
| Mã | Quy tắc |
|---|---|
| QT1 | Công suất vượt 1.200 W thì chuyển sang cảnh báo, chưa ngắt tải |
| QT2 | Quá tải liên tục từ 3 s trở lên thì ngắt tải |
| QT3 | Chỉ trở về bình thường khi công suất xuống dưới 1.100 W |
| QT4 | Điện năng chỉ được cộng dồn khi relay đang đóng; dòng điện bằng 0 khi relay ngắt |
| QT5 | Bản tin cảnh báo chỉ gửi một lần cho mỗi lần ngắt tải |
| QT6 | Sau lệnh bật hoặc tắt từ xa, hệ thống không tự đổi trạng thái relay khi đang bình thường hoặc cảnh báo |
| QT7 | Quá tải kéo dài luôn được ưu tiên: relay bị ngắt kể cả khi đang điều khiển từ xa |
| QT8 | Khi phục hồi sau quá tải hoặc nhận lệnh reset, hệ thống trở lại chế độ tự động |

## 2.5. Yêu cầu phi chức năng

BANG: Bảng 2.4: Yêu cầu phi chức năng
| Mã | Yêu cầu | Mô tả |
|---|---|---|
| PCN1 | Độc lập với mạng | Chức năng bảo vệ không phụ thuộc Wi-Fi, broker hay máy chủ |
| PCN2 | Không chặn | Không dùng lệnh chờ chặn; kết nối lại mạng không được làm trễ việc đo và ngắt tải |
| PCN3 | Chống nhiễu | Lọc tín hiệu đo để không cảnh báo nhầm vì xung nhiễu ngắn |
| PCN4 | Chống đóng ngắt liên tục | Dùng thời gian trễ và hysteresis để relay không dao động quanh ngưỡng |
| PCN5 | Tự phục hồi kết nối | Tự kết nối lại Wi-Fi và broker mà không cần khởi động lại thiết bị |
| PCN6 | Gọn nhẹ | Bản tin JSON ngắn; file lịch sử không ghi quá dày để tránh đầy bộ nhớ |

## 2.6. Kết chương

Chương 2 đã xác định 4 tác nhân, 11 yêu cầu chức năng, 5 kịch bản sử dụng, các quy tắc nghiệp vụ về ngưỡng và quyền ưu tiên, cùng các yêu cầu phi chức năng. Yêu cầu quan trọng nhất là bảo vệ độc lập với mạng. Chương 3 sẽ trình bày thiết kế đáp ứng các yêu cầu này.
