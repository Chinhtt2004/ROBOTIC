# Điều khiển LED xanh/đỏ + màn hình ma trận 8x8 (MAX7219) trên breadboard,
# phản ánh trạng thái RUN/STOP mà Arduino xe báo lên qua Serial.

import RPi.GPIO as GPIO
import spidev

LED_GREEN_PIN = 17  # BCM
LED_RED_PIN = 27    # BCM

_MAX7219_DECODE_MODE = 0x09
_MAX7219_INTENSITY = 0x0A
_MAX7219_SCAN_LIMIT = 0x0B
_MAX7219_SHUTDOWN = 0x0C
_MAX7219_DISPLAY_TEST = 0x0F

_PATTERN_X = [0b10000001, 0b01000010, 0b00100100, 0b00011000,
              0b00011000, 0b00100100, 0b01000010, 0b10000001]
_PATTERN_O = [0b00111100, 0b01000010, 0b10000001, 0b10000001,
              0b10000001, 0b10000001, 0b01000010, 0b00111100]

_spi = None
_last_running = None


def _matrix_write(register, data):
    _spi.xfer2([register, data])


def init():
    global _spi

    GPIO.setmode(GPIO.BCM)
    GPIO.setup(LED_GREEN_PIN, GPIO.OUT, initial=GPIO.LOW)
    GPIO.setup(LED_RED_PIN, GPIO.OUT, initial=GPIO.HIGH)

    _spi = spidev.SpiDev()
    _spi.open(0, 0)  # bus 0, CE0
    _spi.max_speed_hz = 1000000

    _matrix_write(_MAX7219_DISPLAY_TEST, 0x00)
    _matrix_write(_MAX7219_SCAN_LIMIT, 0x07)
    _matrix_write(_MAX7219_DECODE_MODE, 0x00)
    _matrix_write(_MAX7219_INTENSITY, 0x08)
    _matrix_write(_MAX7219_SHUTDOWN, 0x01)  # thoát chế độ tiết kiệm điện

    set_running(False)


def _draw(pattern):
    for row in range(8):
        _matrix_write(row + 1, pattern[row])


def set_running(is_running):
    global _last_running
    if is_running == _last_running:
        return
    _last_running = is_running

    GPIO.output(LED_GREEN_PIN, GPIO.HIGH if is_running else GPIO.LOW)
    GPIO.output(LED_RED_PIN, GPIO.LOW if is_running else GPIO.HIGH)
    _draw(_PATTERN_X if is_running else _PATTERN_O)


def handle_arduino_line(line):
    line = line.strip()
    if line == "STATE:RUN":
        set_running(True)
    elif line == "STATE:STOP":
        set_running(False)


def cleanup():
    if _spi is not None:
        _matrix_write(_MAX7219_SHUTDOWN, 0x00)
        _spi.close()
    GPIO.cleanup()
