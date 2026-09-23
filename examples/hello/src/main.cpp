// picocalc-esp32 example: Hello World / capability report.
//
// Prints what the chip reports about itself over native USB CDC, runs a PSRAM
// read/write check and a WiFi scan, and cycles the onboard WS2812.
//
// Pin safety: this sketch configures NO Pico-header GPIO. Only GPIO21 (the
// onboard WS2812, not routed to the header) is driven. Header pins stay in
// their reset state, so it is safe to run inside the PicoCalc.
// GPIO21 = RGB LED per the Waveshare ESP32-S3-Pico schematic:
// https://files.waveshare.com/upload/a/a7/ESP32-S3-Pico-SCH.pdf

#include <Arduino.h>
#include <WiFi.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_system.h>

static const uint8_t RGB_LED_PIN = 21;
static const uint8_t LED_LEVEL = 16;

static const char *resetReasonName(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON:   return "power-on";
        case ESP_RST_EXT:       return "external pin";
        case ESP_RST_SW:        return "software";
        case ESP_RST_PANIC:     return "panic";
        case ESP_RST_INT_WDT:   return "interrupt watchdog";
        case ESP_RST_TASK_WDT:  return "task watchdog";
        case ESP_RST_WDT:       return "other watchdog";
        case ESP_RST_DEEPSLEEP: return "deep sleep wake";
        case ESP_RST_BROWNOUT:  return "BROWNOUT";
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "unknown";
    }
}

static const char *flashModeName(FlashMode_t m) {
    switch (m) {
        case FM_QIO:  return "QIO";
        case FM_QOUT: return "QOUT";
        case FM_DIO:  return "DIO";
        case FM_DOUT: return "DOUT";
        default:      return "unknown";
    }
}

static void psramTest() {
    if (!psramFound()) {
        Serial.println("  PSRAM test:      SKIPPED (no PSRAM found)");
        return;
    }
    const size_t len = 1024 * 1024;
    uint32_t *buf = (uint32_t *)ps_malloc(len);
    if (!buf) {
        Serial.println("  PSRAM test:      FAIL (ps_malloc 1 MB returned NULL)");
        return;
    }
    const size_t words = len / sizeof(uint32_t);
    uint32_t t0 = micros();
    for (size_t i = 0; i < words; i++) buf[i] = i * 2654435761u;
    uint32_t tWrite = micros() - t0;
    size_t errors = 0;
    t0 = micros();
    for (size_t i = 0; i < words; i++) {
        if (buf[i] != i * 2654435761u) errors++;
    }
    uint32_t tRead = micros() - t0;
    free(buf);
    Serial.printf("  PSRAM test:      %s, 1 MB, %u errors, write %.1f MB/s, read %.1f MB/s\n",
                  errors ? "FAIL" : "PASS", (unsigned)errors,
                  len / (float)tWrite, len / (float)tRead);
}

static void wifiScan() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    int n = WiFi.scanNetworks();
    if (n < 0) {
        Serial.printf("  WiFi scan:       FAIL (code %d)\n", n);
    } else {
        int best = -127, bestCh = 0;
        for (int i = 0; i < n; i++) {
            if (WiFi.RSSI(i) > best) {
                best = WiFi.RSSI(i);
                bestCh = WiFi.channel(i);
            }
        }
        Serial.printf("  WiFi scan:       %d networks", n);
        if (n > 0) Serial.printf(", strongest %d dBm on ch %d", best, bestCh);
        Serial.println();
    }
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);
}

static void printReport() {
    esp_chip_info_t chip;
    esp_chip_info(&chip);

    uint32_t jedec = 0;
    esp_flash_read_id(NULL, &jedec);
    uint32_t physFlash = (jedec & 0xFF) ? (1u << (jedec & 0xFF)) : 0;

    uint64_t mac = ESP.getEfuseMac();

    Serial.println();
    Serial.println("==================== PICOCALC-ESP32 HELLO ====================");
    Serial.printf("  Chip:            %s rev %d, %d cores @ %lu MHz\n", ESP.getChipModel(),
                  ESP.getChipRevision(), ESP.getChipCores(), (unsigned long)ESP.getCpuFreqMHz());
    Serial.printf("  Features:        %s%s%s%s\n",
                  (chip.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi-b/g/n " : "",
                  (chip.features & CHIP_FEATURE_BLE) ? "BLE " : "",
                  (chip.features & CHIP_FEATURE_EMB_FLASH) ? "embedded-flash " : "",
                  (chip.features & CHIP_FEATURE_EMB_PSRAM) ? "embedded-PSRAM" : "");
    Serial.printf("  MAC (efuse):     %02X:%02X:%02X:%02X:%02X:%02X\n",
                  (uint8_t)(mac), (uint8_t)(mac >> 8), (uint8_t)(mac >> 16),
                  (uint8_t)(mac >> 24), (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
    Serial.printf("  SDK / Arduino:   IDF %s / arduino-esp32 %d.%d.%d\n", ESP.getSdkVersion(),
                  ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
    Serial.printf("  Reset reason:    %s\n", resetReasonName(esp_reset_reason()));
    Serial.printf("  Flash:           JEDEC 0x%06lX, physical %lu MB, configured %lu MB, %s @ %lu MHz\n",
                  (unsigned long)jedec, (unsigned long)(physFlash >> 20),
                  (unsigned long)(ESP.getFlashChipSize() >> 20),
                  flashModeName(ESP.getFlashChipMode()),
                  (unsigned long)(ESP.getFlashChipSpeed() / 1000000));
    Serial.printf("  Sketch:          %lu KB used, %lu KB free in app partition\n",
                  (unsigned long)(ESP.getSketchSize() / 1024),
                  (unsigned long)(ESP.getFreeSketchSpace() / 1024));
    Serial.printf("  Internal heap:   %lu KB total, %lu KB free, %lu KB largest block\n",
                  (unsigned long)(ESP.getHeapSize() / 1024), (unsigned long)(ESP.getFreeHeap() / 1024),
                  (unsigned long)(ESP.getMaxAllocHeap() / 1024));
    Serial.printf("  PSRAM:           %s, %lu KB total, %lu KB free\n",
                  psramFound() ? "found" : "NOT FOUND",
                  (unsigned long)(ESP.getPsramSize() / 1024), (unsigned long)(ESP.getFreePsram() / 1024));
    psramTest();
    Serial.printf("  Die temperature: %.1f C\n", temperatureRead());
    wifiScan();
    Serial.println("  Press any key in the monitor to print this report again.");
    Serial.println("=====================================================================");
}

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && millis() - start < 3000) delay(10);
    delay(500);
    printReport();
}

void loop() {
    static uint8_t phase = 0;
    static uint32_t lastBeat = 0;

    switch (phase++ % 3) {
        case 0: neopixelWrite(RGB_LED_PIN, LED_LEVEL, 0, 0); break;
        case 1: neopixelWrite(RGB_LED_PIN, 0, LED_LEVEL, 0); break;
        case 2: neopixelWrite(RGB_LED_PIN, 0, 0, LED_LEVEL); break;
    }

    if (Serial.available()) {
        while (Serial.available()) Serial.read();
        printReport();
    }

    if (millis() - lastBeat >= 5000) {
        lastBeat = millis();
        Serial.printf("[hello] uptime %lus, heap free %lu KB, PSRAM free %lu KB\n",
                      (unsigned long)(millis() / 1000), (unsigned long)(ESP.getFreeHeap() / 1024),
                      (unsigned long)(ESP.getFreePsram() / 1024));
    }
    delay(500);
}
