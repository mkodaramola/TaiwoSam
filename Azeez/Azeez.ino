/*
  ESP32 + PZEM-004T (v3.0 Modbus protocol) + 16x2 I2C LCD  -  FULL VERSION

  Measures: voltage, current, active power, active energy, frequency, power factor.
  No PZEM library needed - talks Modbus-RTU directly over Serial2.

  Wiring (verified with the diagnostic sketch):
    PZEM TX -> ESP32 RX2 (GPIO16)
    PZEM RX -> ESP32 TX2 (GPIO17)
    LCD SDA -> GPIO21, LCD SCL -> GPIO22
    PZEM 5V + GND connected, AC terminals connected to mains
    (without mains on the PZEM's AC side the chip stays silent).

  Library required (Library Manager):
    "LiquidCrystal I2C" by Frank de Brabander

  Serial Monitor (115200 baud):
    send 'r'  -> reset the PZEM's energy counter
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- Pins ----------
#define PZEM_RX_PIN 16   // ESP32 RX2 <- PZEM TX
#define PZEM_TX_PIN 17   // ESP32 TX2 -> PZEM RX
#define I2C_SDA     21
#define I2C_SCL     22

// ---------- PZEM ----------
#define PZEM_ADDR   0xF8 // general address (single device on the bus)
#define PZEM_BAUD   9600
#define PZEM_TRIES  3    // attempts per read before giving up

// Optional voltage correction. 1.000 = no change.
// Only change this after comparing against a trusted reference meter.
#define VOLT_CAL    1.000f

// ---------- LCD ----------
#define LCD_COLS 16
#define LCD_ROWS 2
LiquidCrystal_I2C *lcd = nullptr;

// ---------- Timing ----------
const unsigned long READ_INTERVAL_MS = 1000;
const unsigned long PAGE_INTERVAL_MS = 3000;
const uint8_t FAILS_BEFORE_ERROR = 3;   // LCD shows error after this many failed cycles
unsigned long lastRead = 0, lastPage = 0;
uint8_t page = 0;

struct PzemData {
  float voltage;    // V
  float current;    // A
  float power;      // W
  float energy;     // kWh
  float frequency;  // Hz
  float pf;         // 0.00 - 1.00
};

enum PzemStatus { PZEM_OK, PZEM_NO_REPLY, PZEM_BAD_FRAME, PZEM_BAD_CRC };

PzemData data = {0, 0, 0, 0, 0, 0};
bool valid = false;
uint8_t failCount = 0;

// ---------- Modbus CRC16 ----------
uint16_t crc16(const uint8_t *d, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= d[i];
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

// ---------- PZEM single read attempt ----------
// Reads 10 input registers from 0x0000 and decodes them.
// Response frame: addr, 0x04, 0x14, 20 data bytes, CRC lo, CRC hi (25 bytes).
PzemStatus pzemReadOnce(PzemData &out, uint8_t &bytesGot) {
  uint8_t req[8] = {PZEM_ADDR, 0x04, 0x00, 0x00, 0x00, 0x0A, 0, 0};
  uint16_t c = crc16(req, 6);
  req[6] = c & 0xFF;
  req[7] = c >> 8;

  while (Serial2.available()) Serial2.read();   // flush stale bytes
  Serial2.write(req, 8);

  uint8_t buf[25];
  uint8_t n = 0;
  unsigned long start = millis();
  while (n < 25 && millis() - start < 500) {
    if (Serial2.available()) buf[n++] = Serial2.read();
  }
  bytesGot = n;

  if (n == 0) return PZEM_NO_REPLY;
  if (n != 25 || buf[1] != 0x04 || buf[2] != 0x14) return PZEM_BAD_FRAME;

  uint16_t rc = crc16(buf, 23);
  if (buf[23] != (rc & 0xFF) || buf[24] != (rc >> 8)) return PZEM_BAD_CRC;

  // Register i sits at buf[3 + 2*i] (high byte) and buf[4 + 2*i] (low byte)
  auto reg = [&](uint8_t i) -> uint32_t {
    return ((uint32_t)buf[3 + 2 * i] << 8) | buf[4 + 2 * i];
  };

  out.voltage   = reg(0) / 10.0f * VOLT_CAL;
  out.current   = ((reg(2) << 16) | reg(1)) / 1000.0f;
  out.power     = ((reg(4) << 16) | reg(3)) / 10.0f;
  out.energy    = ((reg(6) << 16) | reg(5)) / 1000.0f;   // Wh -> kWh
  out.frequency = reg(7) / 10.0f;
  out.pf        = reg(8) / 100.0f;
  return PZEM_OK;
}

// Retries a few times before reporting failure
PzemStatus pzemRead(PzemData &out, uint8_t &bytesGot) {
  PzemStatus st = PZEM_NO_REPLY;
  for (uint8_t i = 0; i < PZEM_TRIES; i++) {
    st = pzemReadOnce(out, bytesGot);
    if (st == PZEM_OK) return st;
    delay(100);
  }
  return st;
}

// ---------- PZEM energy reset ----------
void pzemResetEnergy() {
  uint8_t req[4] = {PZEM_ADDR, 0x42, 0, 0};
  uint16_t c = crc16(req, 2);
  req[2] = c & 0xFF;
  req[3] = c >> 8;
  while (Serial2.available()) Serial2.read();
  Serial2.write(req, 4);
  delay(300);
  while (Serial2.available()) Serial2.read();
  Serial.println("Energy counter reset command sent");
}

// ---------- LCD helpers ----------
uint8_t findLcdAddr() {
  const uint8_t candidates[] = {0x27, 0x3F};
  for (uint8_t a : candidates) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) return a;
  }
  return 0x27;   // fall back to the most common address
}

// Print a line padded to full width so no old characters are left behind
void lcdLine(uint8_t row, const String &text) {
  String s = text;
  while (s.length() < LCD_COLS) s += ' ';
  lcd->setCursor(0, row);
  lcd->print(s.substring(0, LCD_COLS));
}

void updateLcd() {
  if (!valid) {
    lcdLine(0, "PZEM no reply");
    lcdLine(1, "Check wiring/AC");
    return;
  }

  switch (page) {
    case 0:
      lcdLine(0, "Volt: " + String(data.voltage, 1) + " V");
      lcdLine(1, "Curr: " + String(data.current, 3) + " A");
      break;
    case 1:
      lcdLine(0, "Pwr: " + String(data.power, 1) + " W");
      lcdLine(1, "PF:  " + String(data.pf, 2));
      break;
    case 2:
      lcdLine(0, "Eng: " + String(data.energy, 3) + "kWh");
      lcdLine(1, "Freq: " + String(data.frequency, 1) + " Hz");
      break;
  }
}

// ---------- Setup / loop ----------
void setup() {
  Serial.begin(115200);

  Serial2.begin(PZEM_BAUD, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);
  delay(100);

  Wire.begin(I2C_SDA, I2C_SCL);
  uint8_t addr = findLcdAddr();
  lcd = new LiquidCrystal_I2C(addr, LCD_COLS, LCD_ROWS);
  lcd->init();
  lcd->backlight();
  lcdLine(0, "Energy Monitor");
  lcdLine(1, "Starting...");
  Serial.printf("LCD at 0x%02X\n", addr);
  delay(1500);
  lcd->clear();
}

void loop() {
  unsigned long now = millis();

  if (Serial.available() && Serial.read() == 'r') pzemResetEnergy();

  if (now - lastRead >= READ_INTERVAL_MS) {
    lastRead = now;

    uint8_t got = 0;
    PzemStatus st = pzemRead(data, got);

    if (st == PZEM_OK) {
      failCount = 0;
      valid = true;
      Serial.printf("V: %.1f V | I: %.3f A | P: %.1f W | E: %.3f kWh | f: %.1f Hz | PF: %.2f\n",
                    data.voltage, data.current, data.power,
                    data.energy, data.frequency, data.pf);
    } else {
      if (failCount < 255) failCount++;
      if (failCount >= FAILS_BEFORE_ERROR) valid = false;

      switch (st) {
        case PZEM_NO_REPLY:
          Serial.println("PZEM: no reply (0 bytes) - check AC on PZEM, 5V/GND, RX/TX wiring");
          break;
        case PZEM_BAD_FRAME:
          Serial.printf("PZEM: bad frame (%u bytes) - noise or wrong protocol\n", got);
          break;
        case PZEM_BAD_CRC:
          Serial.println("PZEM: CRC mismatch - noise on the serial lines");
          break;
        default: break;
      }
    }
    updateLcd();
  }

  if (now - lastPage >= PAGE_INTERVAL_MS) {
    lastPage = now;
    page = (page + 1) % 3;
    updateLcd();
  }
}