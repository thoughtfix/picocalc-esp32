#include "picocalc_sd.h"
#include "picocalc_pins.h"

namespace picocalc {

SPIClass& sdSpi() {
    static SPIClass bus(HSPI);
    return bus;
}

bool sdCardPresent() {
    pinMode(pins::SD_DET, INPUT_PULLUP);
    return digitalRead(pins::SD_DET) == LOW;
}

bool sdBegin(uint32_t hz, const char* mountpoint) {
    static bool busStarted = false;
    if (!busStarted) {
        sdSpi().begin(pins::SD_SCK, pins::SD_MISO, pins::SD_MOSI, pins::SD_CS);
        busStarted = true;
    }
    SD.end();
    return SD.begin(pins::SD_CS, sdSpi(), hz, mountpoint);
}

}  // namespace picocalc
