#pragma once
// PicoCalc microSD slot: SPI bus setup and card detect, for use with the stock Arduino SD library.

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

namespace picocalc {

// Starts the SD card's own SPI bus (separate from the LCD's) and mounts the card on `SD`.
// 20 MHz is reliable. The card's MISO goes through the ESP32-S3 GPIO matrix, where reads
// above ~40 MHz are not guaranteed.
bool sdBegin(uint32_t hz = 20000000, const char* mountpoint = "/sd");

// True when the card-detect switch reports a card.
bool sdCardPresent();

// The SPI bus the SD card is on, for code that wants to share or reconfigure it.
SPIClass& sdSpi();

}  // namespace picocalc
