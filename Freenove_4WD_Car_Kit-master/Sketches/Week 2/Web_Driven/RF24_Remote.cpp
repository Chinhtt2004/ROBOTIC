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
  
  // --- THÊM MỚI: Đọc dữ liệu với Thuật toán chốt đồng bộ (Sync Header) 18 bytes ---
  // Giải thích thuật toán:
  // - Khung truyền mới có 18 byte: [0xAA] [0x55] [16 byte dữ liệu gốc (POT1, POT2, JoyX...)]
  // - Nếu bộ đệm bị tràn và có các byte rác (do xe bị trễ ở chế độ né vật cản), vòng lặp while 
  //   sẽ liên tục dùng Serial.read() để vứt bỏ từng byte rác một.
  // - Nó chỉ dừng vứt khi "mò" thấy đúng 2 byte liên tiếp là 0xAA và 0x55.
  // - Khi đã thấy 0xAA và 0x55, nó tự tin đọc trọn vẹn 16 byte tiếp theo, đảm bảo khớp 100%.
  while (Serial.available() >= 18) {
    if (Serial.read() == 0xAA) {           // Tìm byte 1 của cờ (0xAA)
      if (Serial.peek() == 0x55) {         // Nhìn thử xem byte tiếp theo có phải 0x55 không
        Serial.read();                     // Đúng là 0x55 rồi, đọc để lấy nó ra khỏi buffer luôn
        // Đọc trọn vẹn 16 byte gốc vào mảng nrfDataRead (Bảo toàn được POT1 và POT2)
        Serial.readBytes((char*)nrfDataRead, 16); 
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
