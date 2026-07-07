#include "RF24_Remote.h"

RF24 radio(PIN_SPI_CE, PIN_SPI_CSN);
const byte addresses[6] = "Free1";
int nrfDataRead[8];
bool nrfComplete = false;

bool nrf24L01Setup() {
  // NRF24L01
  if (radio.begin()) {                      // initialize RF24
    radio.setPALevel(RF24_PA_MAX);      // set power amplifier (PA) level
    radio.setDataRate(RF24_1MBPS);      // set data rate through the air
    radio.setRetries(0, 15);            // set the number and delay of retries
    radio.openWritingPipe(addresses);   // open a pipe for writing
    radio.openReadingPipe(1, addresses);// open a pipe for reading
    radio.startListening();             // start monitoringtart listening on the pipes opened

    return true;
  }
  return false;
}

// Must be called from loop() only - NRF24 reads use SPI and must never run
// from an ISR (previously done via FlexiTimer2), which could stall interrupts
// and freeze the whole board (servo/sonar/RF timing all depend on them).
void updateNrf24L01Data() {
  nrfComplete = false;
  
  /* --- ĐÃ COMMENT BỎ (KHÔNG DÙNG NRF24) ---
  if (radio.available()) {             // if receive the data
    while (radio.available()) {         // read all the data
      radio.read(nrfDataRead, sizeof(nrfDataRead));   // read data
    }
    nrfComplete = true;
  }
  ----------------------------------------- */
  
  // --- THÊM MỚI: Đọc dữ liệu với Thuật toán chốt đồng bộ (Sync Header) ---
  // Chống lỗi xe bị điên / còi kêu liên tục do mất đồng bộ byte khi chế độ né vật cản gây trễ
  while (Serial.available() >= 16) {
    if (Serial.read() == 0xAA) {           // Tìm byte 1 của cờ 0x55AA
      if (Serial.peek() == 0x55) {         // Tìm byte 2
        Serial.read();                     // Bỏ qua byte 0x55 trong buffer
        nrfDataRead[0] = 0x55AA;           // Khôi phục POT1
        Serial.readBytes((char*)&nrfDataRead[1], 14); // Đọc 14 byte còn lại
        nrfComplete = true;
      }
    }
  }
}

bool getNrf24L01Data()
{
  return nrfComplete;
}

void clearNrfFlag() {
  nrfComplete = 0;
}

void updateCarActionByNrfRemote() {
  int x = nrfDataRead[2] - 512;
  int y = nrfDataRead[3] - 512;
  int pwmL, pwmR;
  if (y < 0) {
    pwmL = (-y + x) / 2;
    pwmR = (-y - x) / 2;
  }
  else {
    pwmL = (-y - x) / 2;
    pwmR = (-y + x) / 2;
  }
  motorRun(pwmL, pwmR);

  if (nrfDataRead[4] == 0) {
    setBuzzer(true);
  }
  else {
    setBuzzer(false);
  }
}

void resetNrfDataBuf() {
  nrfDataRead[0] = 0;
  nrfDataRead[1] = 0;
  nrfDataRead[2] = 512;
  nrfDataRead[3] = 512;
  nrfDataRead[4] = 1;
  nrfDataRead[5] = 1;
  nrfDataRead[6] = 1;
  nrfDataRead[7] = 1;
}

u8 updateNrfCarMode() {
  // nrfDataRead [5 6 7] --> 111
  return ((nrfDataRead[5] == 1 ? 1 : 0) << 2) | ((nrfDataRead[6] == 1 ? 1 : 0) << 1) | ((nrfDataRead[7] == 1 ? 1 : 0) << 0);
}
