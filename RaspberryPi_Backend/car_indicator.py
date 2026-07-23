# Điều khiển LED xanh/đỏ + màn LCD1602A (qua I2C) trên breadboard,
# phản ánh trạng thái RUN/STOP mà Arduino xe báo lên qua Serial.

import RPi.GPIO as GPIO
from RPLCD.i2c import CharLCD

LED_GREEN_PIN = 17  # BCM
LED_RED_PIN = 27    # BCM

LCD_I2C_ADDRESS = 0x27  # Thường là 0x27 hoặc 0x3F tuỳ module PCF8574

_lcd = None
_last_running = None


def init():
    global _lcd

    GPIO.setmode(GPIO.BCM)
    GPIO.setup(LED_GREEN_PIN, GPIO.OUT, initial=GPIO.LOW)
    GPIO.setup(LED_RED_PIN, GPIO.OUT, initial=GPIO.HIGH)

    _lcd = CharLCD(i2c_expander='PCF8574', address=LCD_I2C_ADDRESS,
                   port=1, cols=16, rows=2)

    set_running(False)


def set_running(is_running):
    global _last_running
    if is_running == _last_running:
        return
    _last_running = is_running

    GPIO.output(LED_GREEN_PIN, GPIO.HIGH if is_running else GPIO.LOW)
    GPIO.output(LED_RED_PIN, GPIO.LOW if is_running else GPIO.HIGH)

    _lcd.clear()
    _lcd.write_string("Trang thai xe:")
    _lcd.cursor_pos = (1, 0)
    _lcd.write_string("CHAY (X)" if is_running else "DUNG (O)")


def handle_arduino_line(line):
    line = line.strip()
    if line == "STATE:RUN":
        set_running(True)
    elif line == "STATE:STOP":
        set_running(False)


def cleanup():
    if _lcd is not None:
        _lcd.clear()
        _lcd.close(clear=True)
    GPIO.cleanup()
