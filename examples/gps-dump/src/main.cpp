// picocalc-esp32 example: GPS raw serial dump.
//
// Reads a GPS module on the PicoCalc's external "Core GPIOs" side header and dumps its raw NMEA
// stream to USB serial, and to the LCD so you can see it without a computer. No parsing: this is
// a wiring/among-the-living check for a module (e.g. a u-blox NEO-6M).
//
// Wiring (Core GPIOs side header):
//   GPS TX -> header GP28 = ESP32-S3 GPIO9   (UART RX in)
//   GPS RX <- header GP5  = ESP32-S3 GPIO16  (UART TX out; a NEO-6M doesn't need it to stream)
//   GPS VCC -> header 3V3_OUT,  GPS GND -> header GND
//
// A NEO-6M defaults to 9600 baud and streams NMEA sentences ($GPGGA, $GPRMC, ...) once powered,
// even with no fix (fields are just empty until it locks). Nothing on screen usually means a
// wiring/power/baud problem, or TX/RX swapped.

#include <Arduino.h>
#include <PicoCalcESP32.h>

static const int GPS_RX = picocalc::pins::HDR_GP28;  // GPIO9, from GPS TX
static const int GPS_TX = picocalc::pins::HDR_GP5;   // GPIO16, to GPS RX
static const uint32_t GPS_BAUD = 9600;

picocalc::Display lcd;
HardwareSerial GPS(1);  // UART1

static char line[128];
static size_t lineLen = 0;
static int screenY = 24;
static uint32_t byteCount = 0;
static uint32_t lastStatus = 0;

void setup() {
    picocalc::safePins();
    Serial.begin(115200);
    GPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);

    lcd.init();
    lcd.setColorDepth(16);
    lcd.fillScreen(TFT_BLACK);
    lcd.setFont(&fonts::Font2);
    lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    lcd.drawString("GPS raw dump  RX=GPIO9 @9600", 4, 4);
    Serial.printf("[gps-dump] UART1 RX=%d TX=%d @ %lu baud\n", GPS_RX, GPS_TX, (unsigned long)GPS_BAUD);
}

static void showLine(const char* s) {
    if (screenY > 300) {
        lcd.fillRect(0, 24, 320, 320 - 24, TFT_BLACK);
        screenY = 24;
    }
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.drawString(s, 4, screenY);
    screenY += 15;
}

void loop() {
    while (GPS.available()) {
        char c = (char)GPS.read();
        byteCount++;
        Serial.write(c);  // raw passthrough to USB
        if (c == '\n' || lineLen >= sizeof(line) - 1) {
            line[lineLen] = 0;
            if (lineLen > 1) showLine(line);
            lineLen = 0;
        } else if (c != '\r') {
            line[lineLen++] = c;
        }
    }

    if (millis() - lastStatus > 3000) {
        lastStatus = millis();
        if (byteCount == 0) {
            Serial.println("[gps-dump] no data yet (check power, wiring, TX/RX, baud)");
            lcd.setTextColor(TFT_RED, TFT_BLACK);
            lcd.fillRect(0, 4, 320, 18, TFT_BLACK);
            lcd.drawString("waiting for GPS data...", 4, 4);
        } else {
            Serial.printf("[gps-dump] %lu bytes received\n", (unsigned long)byteCount);
        }
    }
}
