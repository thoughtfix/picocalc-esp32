// picocalc-esp32 example: in-PicoCalc self-test.
//
// Screen: color bars + gray ramp (display and color order), system info, STM32 firmware /
// battery / backlights, SD card listing, live key echo.
// Keys:   F1 tone, F2 LCD backlight, F3 keyboard backlight, F4 16/24-bit color,
//         F5 full-screen fill timing, ESC rotation, ENTER clears the typed line.
// Serial: the same actions as single letters (see printHelp), plus a text log of every
//         key event, so the test can be driven and read over USB.

#include <Arduino.h>
#include <PicoCalcESP32.h>

using namespace picocalc;

static Display lcd;
static Stm32 mcu;
static Audio audio;

static const int W = 320;
static const int ROW_H = 18;
static const int Y_TITLE = 0;
static const int Y_BARS = 20;
static const int Y_RAMP = 64;
static const int Y_INFO = 86;
static const int Y_FOOTER = 302;

enum InfoRow {
    ROW_CHIP, ROW_STM32, ROW_BATTERY, ROW_SD, ROW_FILE1, ROW_FILE2, ROW_FILE3, ROW_FILE4,
    ROW_KEY, ROW_TYPED, ROW_DISPLAY, ROW_COUNT
};

static String info[ROW_COUNT];
static String typed;
static uint8_t rotation = 0;
static uint8_t colorDepth = 16;
static float lastFps = 0;
static int lastSdDet = -1;

static void drawRow(int row) {
    int y = Y_INFO + row * ROW_H;
    lcd.fillRect(0, y, W, ROW_H, TFT_BLACK);
    lcd.setTextColor(row == ROW_KEY ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
    lcd.drawString(info[row], 4, y + 1);
}

static void setRow(int row, const String& text) {
    info[row] = text;
    drawRow(row);
    Serial.printf("  %s\n", text.c_str());
}

static void drawStatic() {
    lcd.fillScreen(TFT_BLACK);
    lcd.setFont(&fonts::Font2);

    lcd.fillRect(0, Y_TITLE, W, 18, TFT_NAVY);
    lcd.setTextColor(TFT_WHITE, TFT_NAVY);
    lcd.drawString("PICOCALC-ESP32 SELFTEST", 4, Y_TITLE + 1);
    lcd.drawRightString("TOP-RIGHT", W - 4, Y_TITLE + 1);

    // uint16_t matters: LovyanGFX treats a uint32_t color as RGB888, and TFT_* are RGB565.
    struct Bar { uint16_t color; const char* name; uint16_t text; };
    static const Bar bars[] = {
        {TFT_RED, "RED", TFT_WHITE},     {TFT_GREEN, "GRN", TFT_BLACK},
        {TFT_BLUE, "BLUE", TFT_WHITE},   {TFT_CYAN, "CYAN", TFT_BLACK},
        {TFT_MAGENTA, "MAG", TFT_WHITE}, {TFT_YELLOW, "YEL", TFT_BLACK},
        {TFT_WHITE, "WHT", TFT_BLACK},   {TFT_BLACK, "BLK", TFT_WHITE},
    };
    const int bw = W / 8;
    for (int i = 0; i < 8; i++) {
        lcd.fillRect(i * bw, Y_BARS, bw, 40, bars[i].color);
        lcd.setTextColor(bars[i].text, bars[i].color);
        lcd.drawCenterString(bars[i].name, i * bw + bw / 2, Y_BARS + 12);
    }
    lcd.drawRect(7 * bw, Y_BARS, bw, 40, TFT_DARKGREY);

    for (int x = 0; x < W; x++) {
        uint8_t v = x * 255 / (W - 1);
        lcd.drawFastVLine(x, Y_RAMP, 16, lcd.color888(v, v, v));
    }

    lcd.fillRect(0, Y_FOOTER, W, 18, TFT_DARKGREY);
    lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
    lcd.drawString("F1 tone F2 LCD F3 KBD F4 bits F5 fps ESC rot", 2, Y_FOOTER + 1);

    for (int i = 0; i < ROW_COUNT; i++) drawRow(i);
}

static void updateDisplayRow() {
    char buf[64];
    snprintf(buf, sizeof(buf), "LCD %d-bit rot %d  fill %.1f fps", colorDepth, rotation, lastFps);
    setRow(ROW_DISPLAY, buf);
}

static void updateChipRow() {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s %luMHz PSRAM %luK heap %luK", ESP.getChipModel(),
             (unsigned long)ESP.getCpuFreqMHz(), (unsigned long)(ESP.getPsramSize() / 1024),
             (unsigned long)(ESP.getFreeHeap() / 1024));
    setRow(ROW_CHIP, buf);
}

static void updateStm32Row() {
    int ver = mcu.firmwareVersion();
    char buf[64];
    if (ver < 0) {
        snprintf(buf, sizeof(buf), "STM32: NO RESPONSE (errors %lu)", (unsigned long)mcu.errorCount());
    } else {
        snprintf(buf, sizeof(buf), "STM32 fw 0x%02X  i2c delay %ums err %lu", ver, mcu.readDelayMs(),
                 (unsigned long)mcu.errorCount());
    }
    setRow(ROW_STM32, buf);
}

static void updateBatteryRow() {
    int bat = mcu.batteryRaw();
    int lcdBl = mcu.lcdBacklight();
    int kbdBl = mcu.kbdBacklight();
    char buf[64];
    if (bat < 0) {
        snprintf(buf, sizeof(buf), "Battery: read error");
    } else {
        snprintf(buf, sizeof(buf), "Batt %d%%%s LCD-BL %d KBD-BL %d", bat & 0x7F,
                 (bat & 0x80) ? " CHG" : "", lcdBl, kbdBl);
    }
    setRow(ROW_BATTERY, buf);
}

static void sdTest() {
    for (int r = ROW_FILE1; r <= ROW_FILE4; r++) setRow(r, "");
    int det = digitalRead(pins::SD_DET);
    lastSdDet = det;
    if (!sdBegin()) {
        setRow(ROW_SD, String("SD: reseat or insert card (DET=") + det + ")");
        Serial.printf("[sd] %s: %s\n", faultTitle(Fault::SdCard), faultAdvice(Fault::SdCard));
        return;
    }
    const char* type = "?";
    switch (SD.cardType()) {
        case CARD_MMC: type = "MMC"; break;
        case CARD_SD: type = "SDSC"; break;
        case CARD_SDHC: type = "SDHC"; break;
        default: break;
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "SD %s %.1fGB DET=%d", type, SD.cardSize() / 1e9, det);
    setRow(ROW_SD, buf);

    File root = SD.open("/");
    int row = ROW_FILE1, total = 0;
    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
        total++;
        if (row <= ROW_FILE4) {
            String line = String(" ") + (f.isDirectory() ? "[" : "") + f.name() + (f.isDirectory() ? "]" : "");
            if (!f.isDirectory()) line += String("  ") + (unsigned long)f.size();
            setRow(row++, line);
        } else {
            Serial.printf("   %s\n", f.name());
        }
        f.close();
    }
    root.close();
    if (total > 4) setRow(ROW_FILE4, info[ROW_FILE4] + String("  (+") + (total - 4) + " more)");
    if (total == 0) setRow(ROW_FILE1, " (card is empty)");
}

static void fillTest() {
    const uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_BLACK};
    const int frames = 20;
    uint32_t t0 = millis();
    for (int i = 0; i < frames; i++) lcd.fillScreen(colors[i % 5]);
    uint32_t dt = millis() - t0;
    lastFps = frames * 1000.0f / dt;
    Serial.printf("[fill] %d full-screen fills in %lu ms = %.1f fps (%d-bit)\n", frames,
                  (unsigned long)dt, lastFps, colorDepth);
    drawStatic();
    updateDisplayRow();
}

static void cycleLcdBacklight() {
    int cur = mcu.lcdBacklight();
    int next = (cur < 0 || cur >= 224) ? 32 : cur + 64;
    int applied = mcu.setLcdBacklight(next);
    Serial.printf("[lcd-bl] %d -> requested %d, STM32 reports %d\n", cur, next, applied);
    updateBatteryRow();
}

static void cycleKbdBacklight() {
    int cur = mcu.kbdBacklight();
    int next = (cur < 0 || cur >= 192) ? 0 : cur + 64;
    int applied = mcu.setKbdBacklight(next);
    Serial.printf("[kbd-bl] %d -> requested %d, STM32 reports %d\n", cur, next, applied);
    updateBatteryRow();
}

static void toggleDepth() {
    colorDepth = colorDepth == 16 ? 24 : 16;
    lcd.setColorDepth(colorDepth);
    drawStatic();
    updateDisplayRow();
}

static void cycleRotation() {
    rotation = (rotation + 1) & 7;
    lcd.setRotation(rotation);
    drawStatic();
    updateDisplayRow();
}

static void beep() {
    audio.tone(880, 120);
    audio.tone(1320, 120);
    Serial.println("[tone] 880 Hz + 1320 Hz, 120 ms each");
}

static void fullReport() {
    Serial.println();
    Serial.println("==================== PICOCALC-ESP32 SELFTEST ====================");
    updateChipRow();
    updateStm32Row();
    updateBatteryRow();
    sdTest();
    updateDisplayRow();
    Serial.println("=========================================================================");
}

static void printHelp() {
    Serial.println("Serial commands: t=tone l=LCD backlight k=kbd backlight d=16/24-bit f=fill fps");
    Serial.println("                 o=rotation s=SD retest r=full report 0-9=I2C read delay (ms x2)");
    Serial.println("                 x=preview the fault screen (r to return)");
}

static void handleKey(const KeyEvent& ev) {
    const char* name = keyName(ev.code);
    Serial.printf("[key] 0x%02X %-9s %s\n", ev.code, name, keyStateName(ev.state));
    char buf[64];
    snprintf(buf, sizeof(buf), "Key 0x%02X %s %s", ev.code, name, keyStateName(ev.state));
    info[ROW_KEY] = buf;
    drawRow(ROW_KEY);

    if (ev.state != KEY_PRESSED) return;
    switch (ev.code) {
        case key::F1: beep(); return;
        case key::F2: cycleLcdBacklight(); return;
        case key::F3: cycleKbdBacklight(); return;
        case key::F4: toggleDepth(); return;
        case key::F5: fillTest(); return;
        case key::ESC: cycleRotation(); return;
        case key::ENTER: typed = ""; break;
        case key::BACKSPACE: if (typed.length()) typed.remove(typed.length() - 1); break;
        default:
            if (ev.code >= 0x20 && ev.code < 0x7F && typed.length() < 36) typed += (char)ev.code;
            break;
    }
    info[ROW_TYPED] = String("> ") + typed + "_";
    drawRow(ROW_TYPED);
}

static void handleSerial() {
    while (Serial.available()) {
        char c = Serial.read();
        switch (c) {
            case 't': beep(); break;
            case 'l': cycleLcdBacklight(); break;
            case 'k': cycleKbdBacklight(); break;
            case 'd': toggleDepth(); break;
            case 'f': fillTest(); break;
            case 'o': cycleRotation(); break;
            case 's': sdTest(); break;
            case 'r': drawStatic(); fullReport(); break;
            case 'x': showFault(lcd, Fault::KeyboardController); break;
            case 'h': case '?': printHelp(); break;
            default:
                if (c >= '0' && c <= '9') {
                    mcu.setReadDelayMs((c - '0') * 2);
                    Serial.printf("[i2c] read delay now %u ms\n", mcu.readDelayMs());
                    updateStm32Row();
                }
                break;
        }
    }
}

void setup() {
    safePins();
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && millis() - start < 2000) delay(10);

    pinMode(pins::SD_DET, INPUT_PULLUP);

    lcd.init();
    lcd.setColorDepth(colorDepth);
    lcd.setRotation(rotation);
    info[ROW_KEY] = "Key: (press something)";
    info[ROW_TYPED] = "> _";
    drawStatic();

    bool i2cOk = mcu.begin(Wire);
    Serial.printf("[i2c] Wire.begin(SDA=%d, SCL=%d, 10 kHz) -> %s\n", pins::KBD_SDA, pins::KBD_SCL,
                  i2cOk ? "ok" : "FAILED");
    audio.begin();

    fullReport();
    beep();
    printHelp();
}

static void checkStm32Health() {
    static bool faultShown = false;
    if (!mcu.responding() && !faultShown) {
        showFault(lcd, Fault::KeyboardController);
        faultShown = true;
    } else if (mcu.responding() && faultShown) {
        Serial.println("[stm32] responding again");
        faultShown = false;
        drawStatic();
        fullReport();
    }
}

void loop() {
    static uint32_t lastBattery = 0, lastSdPoll = 0;

    KeyEvent ev;
    while (mcu.readKey(ev)) handleKey(ev);
    checkStm32Health();
    handleSerial();

    uint32_t now = millis();
    if (now - lastBattery > 10000) {
        lastBattery = now;
        updateBatteryRow();
    }
    if (now - lastSdPoll > 1000) {
        lastSdPoll = now;
        if (digitalRead(pins::SD_DET) != lastSdDet) {
            Serial.println("[sd] card-detect changed");
            delay(200);
            sdTest();
        }
    }
    delay(5);
}
