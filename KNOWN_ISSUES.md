# Known issues & TODO

`picocalc-esp32` is early (v0.1.0). The core drivers are verified on hardware; a few things are
unfinished or hardware-dependent. This is the honest list.

## Open

- **Battery depends on the PicoCalc's STM32 firmware.** The keyboard MCU only reports a real
  battery level on recent firmware (its version register reads non-zero). On older firmware it
  returns a hardcoded value - check `Stm32::firmwareVersion()` before trusting `batteryRaw()`. Even
  with good firmware, the AXP2101 fuel gauge needs one full charge→discharge cycle to calibrate.
  There is no per-cell voltage over the STM32 bus (percentage only).
- **GPS: verified as a raw feed, not a full lock.** The `gps-dump` example confirms NMEA streaming
  on the side header; a full satellite fix needs open sky and hasn't been captured in CI/on-desk.
- **Arduino IDE path untested.** `library.properties` is present but PlatformIO is the supported/
  tested path; the Arduino IDE board settings in the README haven't been verified.
- **SD MISO is on the GPIO matrix**, so SD SPI reads are only guaranteed to ~40 MHz; `sdBegin()`
  defaults to a safe 20 MHz.

## Design notes (not bugs)

- **No audio DAC** on the PicoCalc - audio is PWM square-wave tones only, not PCM.
- **`GPIO10` is left untouched** (it sits on the Pico's ADC_VREF pad; PicoCalc termination is
  unverified). Don't drive it.
- The **onboard 8 MB PSRAM** on the PicoCalc mainboard is *not* usable by the ESP32-S3; the S3 uses
  its own in-package PSRAM. `safePins()` holds that chip deselected.

## Ideas

- A BLE-scan helper so apps get a device list without wrangling NimBLE directly.
- Optional higher SD clock with a per-board tested ceiling.
- Verify and document the Arduino IDE workflow.
