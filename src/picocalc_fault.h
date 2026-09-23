#pragma once
// User-facing hardware fault messages. When a driver reports a failure it can't recover
// from, show the user what to do instead of leaving the screen frozen.

#include <M5GFX.h>

namespace picocalc {

enum class Fault {
    KeyboardController,  // STM32 not answering on I2C: needs a full power cycle
    SdCard,              // no card, or card unreadable: reseat or insert
};

const char* faultTitle(Fault fault);
const char* faultAdvice(Fault fault);

// Full-screen message plus a Serial log line. Draws on any M5GFX display or sprite.
void showFault(lgfx::LovyanGFX& gfx, Fault fault);
void showFault(lgfx::LovyanGFX& gfx, const char* title, const char* advice);

}  // namespace picocalc
