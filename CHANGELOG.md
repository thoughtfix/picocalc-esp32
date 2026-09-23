# Changelog

## 0.1.0 (unreleased)

- `sdBegin()`, `sdCardPresent()`, `sdSpi()`: SD card setup without knowing the bus and pins.
  Verified on hardware.
- STM32 link: I2C bus clear at startup, and up to 3 attempts per transaction. Verified on
  hardware: 0 errors across 5 consecutive ESP32 resets, including flash-then-reboot.
- `Stm32::responding()` and `showFault()`: tell the user to power-cycle when the keyboard
  controller stops answering, or to reseat/insert the SD card. The self-test uses both.
- Audio: builds on arduino-esp32 3.x (pin-based LEDC API) as well as 2.x. Compile-tested
  on 3.3.12.
- README: Cardputer comparison, radio and battery notes, build and flash instructions,
  API overview, disclaimer.
- Arduino IDE metadata (`library.properties`). Not yet tested in the Arduino IDE.

## 0.0.1

- First version, verified on a PicoCalc with a Waveshare ESP32-S3-Pico (HWTEST 01 PASS):
  display, keyboard, battery, backlights, SD, speaker.
