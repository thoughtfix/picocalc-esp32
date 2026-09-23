// Minimal picocalc-esp32 example: draw text, beep, and print key presses.

#include <Arduino.h>
#include <PicoCalcESP32.h>

picocalc::Display lcd;
picocalc::Stm32 mcu;
picocalc::Audio audio;

void setup() {
    picocalc::safePins();
    Serial.begin(115200);

    lcd.init();
    lcd.setColorDepth(16);
    lcd.fillScreen(TFT_BLACK);
    lcd.setFont(&fonts::Font4);
    lcd.drawString("Hello, PicoCalc!", 10, 10);

    mcu.begin(Wire);
    audio.begin();
    audio.tone(880, 100);
}

void loop() {
    picocalc::KeyEvent ev;
    while (mcu.readKey(ev)) {
        if (ev.state == picocalc::KEY_PRESSED) {
            Serial.printf("key %s\n", picocalc::keyName(ev.code));
        }
    }
    delay(10);
}
