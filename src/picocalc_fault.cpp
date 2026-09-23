#include "picocalc_fault.h"

namespace picocalc {

const char* faultTitle(Fault fault) {
    switch (fault) {
        case Fault::KeyboardController: return "Keyboard controller not responding";
        case Fault::SdCard: return "SD card not readable";
    }
    return "Hardware fault";
}

const char* faultAdvice(Fault fault) {
    switch (fault) {
        case Fault::KeyboardController:
            return "Power off the PicoCalc, unplug USB, wait a few seconds, then power on again.";
        case Fault::SdCard:
            return "Reseat or insert the SD card. It must be formatted FAT32.";
    }
    return "Power off the PicoCalc, unplug USB, then power on again.";
}

void showFault(lgfx::LovyanGFX& gfx, Fault fault) {
    showFault(gfx, faultTitle(fault), faultAdvice(fault));
}

void showFault(lgfx::LovyanGFX& gfx, const char* title, const char* advice) {
    Serial.printf("[FAULT] %s: %s\n", title, advice);

    const uint16_t bg = 0x6000;  // dark red
    gfx.fillScreen(bg);
    gfx.setTextWrap(true, false);
    gfx.setTextColor(TFT_WHITE, bg);

    gfx.setFont(&fonts::Font4);
    gfx.setCursor(10, 20);
    gfx.println("Hardware problem");

    gfx.setFont(&fonts::Font2);
    gfx.setCursor(10, gfx.getCursorY() + 16);
    gfx.println(title);
    gfx.setCursor(10, gfx.getCursorY() + 12);
    gfx.println(advice);
}

}  // namespace picocalc
