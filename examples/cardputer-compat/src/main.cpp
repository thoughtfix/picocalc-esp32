// Cardputer-style code running on the PicoCalc through the compatibility layer.
// Nothing below is PicoCalc-specific. The include/ folder routes <M5Cardputer.h> to
// <PicoCalcCardputer.h>.
//
// Type to echo text; Backspace deletes; Enter clears (with a beep). Arrow keys arrive as the
// Cardputer's ; . , / navigation keys. Esc (the Cardputer's `) cycles the screen brightness.
// Every keyboard change is logged over Serial.

#include <M5Cardputer.h>

static const int CARD_W = 240;
static const int CARD_H = 135;

M5Canvas canvas(&M5Cardputer.Display);
String line = "> ";
String lastNav = "-";
uint8_t brightness = 128;

void draw() {
    canvas.fillSprite(TFT_BLACK);
    canvas.drawRect(0, 0, CARD_W, CARD_H, TFT_DARKGREY);
    canvas.setTextColor(TFT_GREEN, TFT_BLACK);
    canvas.drawString("Cardputer compat test", 6, 6);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString(String("Battery ") + M5.Power.getBatteryLevel() + "%" +
                          (M5.Power.isCharging() == m5::Power_Class::is_charging ? " CHG" : ""),
                      6, 26);
    canvas.drawString(String("Nav: ") + lastNav + "   Bright: " + brightness, 6, 46);
    canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
    canvas.drawString(line + "_", 6, 90);
    canvas.pushSprite((M5Cardputer.Display.width() - CARD_W) / 2, (M5Cardputer.Display.height() - CARD_H) / 2);
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setBrightness(brightness);

    canvas.setColorDepth(16);
    canvas.createSprite(CARD_W, CARD_H);
    canvas.setFont(&fonts::Font2);

    M5.Speaker.tone(880, 100);
    Serial.println("[compat] Cardputer-style app started");
    draw();
}

void loop() {
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isChange()) {
        if (M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState st = M5Cardputer.Keyboard.keysState();

            String word;
            for (char c : st.word) word += c;
            Serial.printf("[compat] held=%u word='%s' del=%d enter=%d tab=%d shift=%d ctrl=%d alt=%d\n",
                          M5Cardputer.Keyboard.isPressed(), word.c_str(), st.del, st.enter, st.tab,
                          st.shift, st.ctrl, st.alt);

            if (M5Cardputer.Keyboard.isKeyPressed(';')) lastNav = "UP";
            else if (M5Cardputer.Keyboard.isKeyPressed('.')) lastNav = "DOWN";
            else if (M5Cardputer.Keyboard.isKeyPressed(',')) lastNav = "LEFT";
            else if (M5Cardputer.Keyboard.isKeyPressed('/')) lastNav = "RIGHT";

            if (M5Cardputer.Keyboard.isKeyPressed('`')) {
                brightness = brightness >= 224 ? 32 : brightness + 64;
                M5.Display.setBrightness(brightness);
                Serial.printf("[compat] brightness %u\n", brightness);
            } else if (st.enter) {
                M5.Speaker.tone(1320, 60);
                line = "> ";
            } else if (st.del) {
                if (line.length() > 2) line.remove(line.length() - 1);
            } else {
                line += word;
                if (line.length() > 30) line = "> " + word;
            }
        } else {
            Serial.println("[compat] all keys released");
        }
        draw();
    }

    static uint32_t lastBattery = 0;
    if (millis() - lastBattery > 10000) {
        lastBattery = millis();
        draw();
    }
    delay(5);
}
