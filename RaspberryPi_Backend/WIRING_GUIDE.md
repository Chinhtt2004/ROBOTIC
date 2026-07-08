# Hướng dẫn kết nối Raspberry Pi với Arduino Bridge (Cắm cáp USB)

Hướng đi này an toàn, dễ làm nhất và **không yêu cầu bạn phải tháo lắp hay đấu dây rườm rà**. Chiếc Raspberry Pi sẽ thay thế hoàn toàn Laptop của bạn.

### 1. Ý tưởng Kiến trúc (Autonomous Mode)
Bạn sẽ để con Raspberry Pi **nằm ngay trên chiếc xe** (hoặc để ngoài cũng được). 
Thay vì đấu dây trực tiếp cụm NRF24L01 vào Pi (như hướng dẫn trước gây lỗi phần cứng), chúng ta sẽ tận dụng lại chiếc **Arduino Bridge** (chính là cái Remote) cắm thẳng vào cổng USB của Raspberry Pi!

*   **Laptop/Điện thoại:** Chỉ dùng để mở trình duyệt Web (Wi-Fi).
*   **Raspberry Pi:** Cõng trên lưng xe, chạy Camera AI, tính toán tọa độ, rồi đẩy tín hiệu qua cáp USB xuống mạch Arduino Bridge.
*   **Arduino Bridge (Remote):** Nhận lệnh từ cáp USB của Pi, và nháy sóng Radio sang chiếc xe (dù 2 đứa cách nhau chỉ vài centimet). 
*   **Chiếc Xe:** Chạy code gốc `05.4`, nhận sóng Radio như bình thường.

### 2. Cách khởi chạy hệ thống bằng Python
1. Lấy sợi cáp USB xanh, cắm 1 đầu vào **Arduino Bridge** (Remote), đầu còn lại cắm vào **cổng USB của Raspberry Pi**.
2. Đảm bảo Raspberry Pi đã cài Python 3. Copy thư mục `RaspberryPi_Backend` này vào Pi.
3. Mở Terminal tại thư mục này và gõ lệnh tải 2 thư viện cần thiết:
   ```bash
   pip install pyserial websockets
   ```
4. Nếu trên Pi, cổng USB cắm vào Arduino nhận tên khác (Ví dụ `/dev/ttyUSB0` thay vì `/dev/ttyACM0`), bạn hãy dùng lệnh `ls /dev/tty*` để xem và mở file `server.py` sửa lại biến `SERIAL_PORT` nhé.
5. Chạy Backend:
   ```bash
   python3 server.py
   ```
6. Bạn sẽ thấy báo `[Serial] Connected to Arduino on /dev/tty...`. Lúc này, bạn chỉ cần mở Frontend trên Laptop, đổi `ws://localhost:8080` thành địa chỉ IP Wi-Fi của Pi (ví dụ `ws://192.168.1.50:8080`) là xong!

Đây là bước đệm tuyệt vời nhất. Toàn bộ logic chống kẹt phím, đổi tốc độ và ép kiểu 16-byte đều đã được mình viết gọn gàng bằng Python, sẵn sàng để bạn nhúng code OpenCV (Camera) vào `server.py` sau này!
