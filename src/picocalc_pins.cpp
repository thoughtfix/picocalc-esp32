#include <Arduino.h>
#include "picocalc_pins.h"

namespace picocalc {

void safePins() {
    pinMode(pins::PSRAM_CS, OUTPUT);
    digitalWrite(pins::PSRAM_CS, HIGH);
}

}  // namespace picocalc
