import asyncio
import json
import struct
import time
import serial
import websockets

import car_indicator

# --- Khởi tạo Serial Port (Kết nối Pi với Arduino qua cáp USB) ---
# Trên Raspberry Pi, cổng USB thường là /dev/ttyACM0 hoặc /dev/ttyUSB0
SERIAL_PORT = '/dev/ttyACM0'  
BAUD_RATE = 9600

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    is_serial_connected = True
    print(f"[Serial] Connected to Arduino on {SERIAL_PORT}")
except Exception as e:
    is_serial_connected = False
    print(f"[Serial Error] Could not open port {SERIAL_PORT}: {e}")

# --- Biến toàn cục quản lý trạng thái xe ---
last_command_time = time.time()
current_mode = {"s1": 1, "s2": 1, "s3": 1} # Mặc định Manual
target_x = 0
target_y = 0
joy_z = 1 # 1 = nhả, 0 = bấm

# Danh sách clients đang kết nối WebSocket
connected_clients = set()

# --- Logic Đóng gói & Gửi dữ liệu qua cáp USB (Chạy liên tục mỗi 50ms) ---
async def serial_loop():
    global target_x, target_y
    while True:
        # Cơ chế fail-safe: Nếu mất kết nối/không nhận được lệnh trong 300ms, tự dừng xe
        if time.time() - last_command_time > 0.3:
            target_x = 0
            target_y = 0
        
        # Giới hạn x, y trong khoảng -512 đến +511
        x = max(-512, min(511, target_x))
        y = max(-512, min(511, target_y))

        # Phục hồi giá trị Analog gốc (0 đến 1023)
        analog_x = round(x + 512)
        analog_y = round(y + 512)

        # Đóng gói 18 bytes:
        # - 2 bytes đầu (0xAA, 0x55) làm Cờ đồng bộ (Sync Header) không xâm lấn dữ liệu
        # - 16 bytes sau (8 số short/int16) là dữ liệu gốc của xe (POT1, POT2, JoyX, JoyY, JoyZ, S1, S2, S3)
        pot1_val = 0 # Sau này có thể gán giá trị chỉnh màu LED
        pot2_val = 0 # Sau này có thể gán giá trị chỉnh tốc độ LED
        
        payload = struct.pack('<BBhhhhhhhh', 
                              0xAA, 0x55, 
                              pot1_val, pot2_val, analog_x, analog_y, joy_z, 
                              current_mode["s1"], current_mode["s2"], current_mode["s3"])
        
        success = False
        if is_serial_connected and ser.is_open:
            try:
                # Đọc sạch dữ liệu Arduino gửi lên (ví dụ các lệnh Serial.println)
                # Nếu không đọc, bộ đệm bị đầy sẽ làm đứng cả Arduino lẫn Pi!
                if ser.in_waiting > 0:
                    arduino_msg = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                    # In ra log để bạn xem kết quả debug từ Arduino
                    if arduino_msg.strip():
                        print(f"[Arduino] {arduino_msg.strip()}")
                    for line in arduino_msg.splitlines():
                        car_indicator.handle_arduino_line(line)
                
                # Gửi lệnh xuống Arduino
                ser.write(payload)
                success = True
            except Exception as e:
                print(f"[Serial Error] {e}")
        
        # Gửi Telemetry về cho web
        telemetry_data = {
            "type": "telemetry",
            "connected": success,
            "battery_percent": 100 if success else 0
        }
        if connected_clients:
            msg = json.dumps(telemetry_data)
            websockets.broadcast(connected_clients, msg)
            
        await asyncio.sleep(0.05) # Nghỉ 50ms (Tương đương 20 FPS)

# --- Xử lý sự kiện giả lập nhấn Joystick Z ---
async def simulate_joy_z_click():
    global joy_z
    joy_z = 0 # Bấm
    await asyncio.sleep(0.15) # Giữ 150ms
    joy_z = 1 # Nhả

# --- Xử lý kết nối WebSocket ---
async def ws_handler(websocket):
    global last_command_time, target_x, target_y, current_mode
    print("[WebSocket] Client connected")
    connected_clients.add(websocket)
    try:
        async for message in websocket:
            try:
                print(message)
                command = json.loads(message)
                if command.get("type") == "drive":
                    last_command_time = time.time()
                    target_x = command.get("x", 0)
                    target_y = command.get("y", 0)
                
                elif command.get("type") == "mode":
                    mode_str = command.get("mode")
                    if mode_str == "Manual": current_mode = {"s1": 1, "s2": 1, "s3": 1}
                    elif mode_str == "LineFollow": current_mode = {"s1": 1, "s2": 0, "s3": 1}
                    elif mode_str == "Obstacle": current_mode = {"s1": 0, "s2": 1, "s3": 1}
                    elif mode_str == "Auto": current_mode = {"s1": 0, "s2": 1, "s3": 0}
                    elif mode_str == "LEDControl": current_mode = {"s1": 1, "s2": 1, "s3": 0} # 1-1-0
                    else: current_mode = {"s1": 1, "s2": 1, "s3": 1} # Các chức năng chưa có -> Mặc định về Manual
                    
                    asyncio.create_task(simulate_joy_z_click())
                    
            except json.JSONDecodeError:
                print("[WebSocket] Error parsing JSON")
    finally:
        print("[WebSocket] Client disconnected")
        connected_clients.remove(websocket)
        target_x = 0
        target_y = 0

async def main():
    server = await websockets.serve(ws_handler, "0.0.0.0", 8080)
    print("[WebSocket] Server running on ws://0.0.0.0:8080")
    
    await asyncio.gather(
        server.wait_closed(),
        serial_loop()
    )

if __name__ == "__main__":
    car_indicator.init()
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        if is_serial_connected and ser.is_open:
            ser.close()
        print("Shutting down...")
    finally:
        car_indicator.cleanup()
