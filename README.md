# picocalc-esp32

**Run Cardputer apps, and potentially other ESP32 apps, on a [ClockworkPi PicoCalc](https://www.clockworkpi.com/picocalc)
with a [Waveshare ESP32-S3-Pico](https://www.waveshare.com/wiki/ESP32-S3-Pico) core.**

The Waveshare ESP32-S3-Pico is pin-compatible with the Raspberry Pi Pico. It drops into the
PicoCalc's Pico socket with no wiring changes, and replaces the RP2040/RP2350 with a 240 MHz
dual-core ESP32-S3 that has WiFi, Bluetooth LE, 16 MB of flash and 2 MB of PSRAM. The PicoCalc's
hardware still expects the pins a Pico uses. This library provides the drivers for the display,
keyboard, SD card, speaker, battery and backlights.

An example of it in use is **[Porkocalc](https://github.com/thoughtfix/porkocalc)** (coming soon),
a port of the excellent **[M5PorkChop](https://github.com/0ct0sec/M5PORKCHOP)** project by 0ct0sec.

> **Status: early (v0.1.0).** Display, keyboard, battery, backlights, SD, speaker, and GPS (raw
> feed) are verified on real hardware. A **Cardputer compatibility layer** ships too - code written
> for `M5Cardputer` / `M5Unified` compiles against it with a two-line shim (see
> [`examples/cardputer-compat`](examples/cardputer-compat)). See [KNOWN_ISSUES.md](KNOWN_ISSUES.md)
> for the honest punch-list.

## Why this is exciting

If you write for the M5Stack Cardputer, the PicoCalc with an ESP32-S3-Pico is the same chip with
a lot more room:

| | M5Stack Cardputer ADV | PicoCalc + Waveshare ESP32-S3-Pico |
|---|---|---|
| Chip | ESP32-S3 (S3FN8), 2 cores @ 240 MHz | ESP32-S3 (S3R2), 2 cores @ 240 MHz |
| RAM | 512 KB SRAM, **no PSRAM** | 512 KB SRAM **+ 2 MB PSRAM** |
| Flash | 8 MB | **16 MB** |
| Screen | 1.14", 240x135 | **4" IPS, 320x320** (3.2x the pixels) |
| Keyboard | 56 keys | **67 keys**, backlit, real arrow keys, F1-F10 |
| Battery | 1750 mAh | **Two 18650 cells** (user-supplied) |
| Audio | Speaker, mic, 3.5 mm out | Stereo speakers, 3.5 mm out (no mic) |
| Storage | microSD | microSD |

The same ESP32-S3 means Cardputer code already compiles for this chip. The extra PSRAM means
full-screen frame buffers and big data sets don't compete with WiFi for internal RAM, which is
the usual cause of out-of-memory crashes on the Cardputer. The Cardputer does have things the
PicoCalc lacks (IMU, IR emitter, microphone, Grove port), so apps that depend on those need
another plan.

**Radio caveat.** The ESP32-S3-Pico's antenna sits inside the PicoCalc, behind the LCD and next
to the batteries and mainboard. Expect weaker WiFi and Bluetooth reception than on an open board
or a Cardputer, which matters for wardriving and other scanning. In one informal comparison, the
strongest network read about 19 dB weaker with the board installed than on a desk. That wasn't a
controlled test, but plan for reduced range.

**Battery.** An ESP32-S3 with its radio on draws more current than an RP2040. Two 18650 cells
hold several times the Cardputer ADV's 1750 mAh, though, so expect long runtimes.

## What's included

- **Display**: the 320x320 ST7365P LCD as an [M5GFX](https://github.com/m5stack/M5GFX)/LovyanGFX
  device (16-bit color, 40 MHz SPI, about 24 full-screen redraws per second). Code that already
  uses `M5GFX` or `M5Canvas` can draw here.
- **Keyboard**: key events (pressed / held / released) from the PicoCalc's STM32 keyboard
  controller, with names for every key.
- **Battery and backlights**: battery percentage and charging state, LCD and keyboard backlight
  control, all through the same STM32.
- **SD card**: `sdBegin()` mounts the card with the stock Arduino `SD` library, plus card detect.
- **Speaker**: tones through both speakers.
- **Pin map**: every PicoCalc signal mapped to its ESP32-S3 GPIO, including the external headers.
- **Hardware tests**: flash-and-look acceptance tests for your setup.

## Quick start (PlatformIO)

`platformio.ini`:

```ini
[env:esp32s3pico]
platform = espressif32@6.12.0
board = esp32-s3-devkitc-1
framework = arduino
board_build.flash_mode = qio
board_build.arduino.memory_type = qio_qspi
board_upload.flash_size = 16MB
board_upload.maximum_size = 16777216
board_build.partitions = default_16MB.csv
build_flags =
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
lib_deps =
    https://github.com/thoughtfix/picocalc-esp32.git
monitor_speed = 115200
monitor_dtr = 0
monitor_rts = 0
```

`src/main.cpp`:

```cpp
#include <Arduino.h>
#include <PicoCalcESP32.h>

picocalc::Display lcd;
picocalc::Stm32 mcu;
picocalc::Audio audio;

void setup() {
    picocalc::safePins();
    Serial.begin(115200);

    lcd.init();
    lcd.setColorDepth(16);
    lcd.fillScreen(TFT_BLACK);
    lcd.setFont(&fonts::Font4);
    lcd.drawString("Hello, PicoCalc!", 10, 10);

    mcu.begin(Wire);
    audio.begin();
    audio.tone(880, 100);
}

void loop() {
    picocalc::KeyEvent ev;
    while (mcu.readKey(ev)) {
        if (ev.state == picocalc::KEY_PRESSED) {
            Serial.printf("key %s\n", picocalc::keyName(ev.code));
        }
    }
    delay(10);
}
```

This is also [`examples/hello-picocalc`](examples/hello-picocalc).

## Building and flashing the examples

Each folder in [`examples/`](examples) is a complete PlatformIO project.

**VS Code:** install the **PlatformIO IDE** extension, then use *File > Open Folder* on an example
folder (for example `examples/selftest`). Build with the checkmark button in the status bar and
upload with the right-arrow button. The serial monitor (plug icon) is already configured by the example's `platformio.ini`.

**Command line:**

```sh
cd examples/selftest
pio run                                   # build
pio run -t upload --upload-port <port>    # flash
pio device monitor -p <port>              # watch the output
```

**Which port.** Flash through the ESP32-S3-Pico's own USB-C port, not the PicoCalc's. The
PicoCalc's USB-C goes to a serial adapter that can't put the ESP32 into bootloader mode. The
board's USB-C appears as **two** serial ports, because of a USB hub on the board:

- The ESP32-S3's native USB (Espressif "USB JTAG/serial debug unit"). Use this one for flashing
  and for `Serial` output. macOS: `/dev/cu.usbmodem*`, Linux: `/dev/ttyACM*`, Windows: `COMx`.
- A WCH CH343 USB-serial chip wired to UART0. It can also flash, as a backup.

**If it gets stuck**, with uploads failing or the monitor showing `waiting for download`: press
**RESET** on the board, or hold **BOOT**, tap **RESET**, release **BOOT**, then upload again.

**Serial monitor settings matter.** Opening the native USB port with DTR/RTS asserted reboots the
chip into download mode. The examples set `monitor_dtr = 0` and `monitor_rts = 0`. Copy those
lines into your own projects.

**Arduino IDE** (not yet tested; PlatformIO is the supported path): board *ESP32S3 Dev Module*,
Flash Size *16MB*, PSRAM *QSPI PSRAM*, USB CDC On Boot *Enabled*, USB Mode *Hardware CDC and
JTAG*. Install M5GFX from the Library Manager and this library from its ZIP.

## Examples and hardware tests

| Example | What it does |
|---|---|
| [`hello-picocalc`](examples/hello-picocalc) | The quick start above. |
| [`hello`](examples/hello) | Reports chip, flash, PSRAM and heap over USB, runs a PSRAM check and a WiFi scan, and cycles the RGB LED. Touches no PicoCalc pins, so it's safe on a bare board. |
| [`selftest`](examples/selftest) | Full in-PicoCalc test: color bars, live key echo, battery and backlights, SD listing, speaker, display fill rate. Keys F1-F5 and Esc run the individual tests. |
| [`gps-dump`](examples/gps-dump) | Raw NMEA dump from a GPS module on the side header (GPIO9 RX @ 9600) to USB serial and the LCD. Wiring/among-the-living check for a NEO-6M. |

## API overview

Everything is in `namespace picocalc`. Include `<PicoCalcESP32.h>`.

| Call | What it does |
|---|---|
| `safePins()` | Call first in `setup()`. Holds the PicoCalc's unused onboard PSRAM chip deselected. |
| `Display` | An M5GFX `LGFX_Device`. Use it like any M5GFX display: `init()`, `fillScreen()`, `drawString()`, sprites, and so on. |
| `Stm32::begin(Wire)` | Starts the keyboard controller link. |
| `Stm32::readKey(ev)` | Next key event from the FIFO (`ev.state`, `ev.code`). Returns false when empty. |
| `keyName(code)`, `key::UP`, `key::F1`, ... | Key names and codes. |
| `Stm32::batteryRaw()` | Bits 0-6 are the percentage, bit 7 means charging. |
| `Stm32::setLcdBacklight(level)`, `setKbdBacklight(level)` | 0-255. |
| `Stm32::responding()` | False after a transaction fails all its retries. Use it to detect a stuck keyboard controller. |
| `showFault(lcd, Fault::KeyboardController)` | Full-screen message telling the user what to do (power-cycle, or reseat the SD card). |
| `sdBegin()`, `sdCardPresent()` | Mount the SD card on `SD`, and read card detect. |
| `Audio::begin()`, `Audio::tone(hz, ms)` | Square-wave tones on both speakers. |
| `pins::...` | The full pin map. |

## Pin map

| PicoCalc function | Pico GPIO | ESP32-S3 GPIO |
|---|---|---|
| LCD SCK / MOSI / MISO / CS | GP10 / 11 / 12 / 13 | 35 / 36 / 37 / 38 |
| LCD DC / RST | GP14 / 15 | 39 / 40 |
| SD MISO / CS / SCK / MOSI | GP16 / 17 / 18 / 19 | 42 / 41 / 1 / 2 |
| SD card detect | GP22 | 6 |
| Keyboard I2C SDA / SCL (STM32 at 0x1F) | GP6 / 7 | 17 / 18 |
| Speaker L / R (PWM) | GP26 / 27 | 7 / 8 |
| PicoCalc onboard PSRAM CS (unused, held high) | GP20 | 4 |
| "Core GPIOs" header GP2 / 3 / 4 / 5 / 21 / 28 | GP2 / 3 / 4 / 5 / 21 / 28 | 13 / 14 / 15 / 16 / 5 / 9 |
| "Mainboard GPIOs" header UART0 TX / RX | GP0 / 1 | 11 / 12 |
| "Mainboard GPIOs" header UART1 TX / RX (also wired to the STM32) | GP8 / 9 | 33 / 34 |

The full map with notes is in [`src/picocalc_pins.h`](src/picocalc_pins.h).

## Gotchas

- **Color types.** Pass `TFT_*` colors as `uint16_t`. M5GFX treats a `uint32_t` color as RGB888,
  so `TFT_RED` stored in a `uint32_t` draws green.
- **Big buffers go in PSRAM.** A full-screen 16-bit sprite is 200 KB. Call `setPsram(true)` on
  large `M5Canvas`/`LGFX_Sprite` objects before `createSprite()`.
- **GPIO10 is off-limits.** It sits on the Pico's ADC_VREF pad, and how the PicoCalc terminates
  that pad is unverified. Don't drive it.
- **If the keyboard controller stops answering**, power-cycle the PicoCalc: power off, unplug
  USB, then power on. It has happened once in testing, after many rapid ESP32 resets, and
  couldn't be reproduced. The self-test shows a fault screen when it happens.
- **The STM32 runs on PicoCalc power.** If the ESP32 is powered only through its own USB-C with the
  PicoCalc switched off, the display and SD card still work but the keyboard, battery and
  backlight calls fail. Switch the PicoCalc on.

## Disclaimer

- **Use at your own risk.** This is hobby software for modified hardware. Nothing here is
  endorsed by ClockworkPi, Waveshare or M5Stack, and there's no warranty (see [LICENSE](LICENSE)).
- **Take extra care when plugging in cores.** Power the PicoCalc off and unplug USB before
  swapping boards. Check the orientation (pin 1 to pin 1) and that no header pins are bent. A
  board inserted backwards or offset by a pin can put battery voltage where it doesn't belong.
- **Know and obey your local laws.** Some apps built on this library, such as Porkocalc, are
  capable of wireless security analysis. Only scan or test networks and devices you own or are
  explicitly authorized to test. You're responsible for how you use them.

## Credits

See [CREDITS.md](CREDITS.md) for the full list. In short: ClockworkPi for the PicoCalc and its
reference code, Waveshare for the ESP32-S3-Pico, lovyan03 and M5Stack for LovyanGFX/M5GFX,
[jblanked](https://www.youtube.com/@jblanked) ([Picoware](https://github.com/jblanked/Picoware))
and [skizzophrenic / TalkingSasquach](https://www.youtube.com/@TalkingSasquach) for inspiration,
[Valleytechsolutions](https://www.youtube.com/@Valleytechsolutions) for showcasing the community,
and 0ct0sec's [M5PorkChop](https://github.com/0ct0sec/M5PORKCHOP) for the reason this exists.

## License

MIT. See [LICENSE](LICENSE).

TL;DR: do what you want with it. Make T-shirts. Tattoo it on your ass if you like.
