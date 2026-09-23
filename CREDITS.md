# Credits

This library stands on other people's work. Wherever code or a method came from somewhere else,
the source file has a comment linking to the original. If any attribution is wrong or missing,
please open an issue - credit matters.

## Hardware knowledge

- **ClockworkPi - [PicoCalc](https://github.com/clockworkpi/PicoCalc)**. The hardware, schematics,
  and reference firmware. The display init values, the STM32 keyboard/battery register protocol,
  the key codes, and the pin assignments all come from their published code and schematic.
  - PicoMite display init for the ST7365P (PicoMite by Geoff Graham and Peter Mather; PicoCalc
    changes by ClockworkPi): `Code/PicoMite/PicoMite.patch`
  - STM32 keyboard firmware: `Code/picocalc_keyboard/`
  - Pico keyboard driver: `Code/picocalc_helloworld/i2ckbd/`
- **Waveshare - [ESP32-S3-Pico](https://www.waveshare.com/wiki/ESP32-S3-Pico)** and its
  [schematic](https://files.waveshare.com/upload/a/a7/ESP32-S3-Pico-SCH.pdf), which gave us the
  header-to-GPIO mapping.

## Software

- **lovyan03 - [LovyanGFX](https://github.com/lovyan03/LovyanGFX)**
  ([FreeBSD](https://github.com/lovyan03/LovyanGFX/blob/master/license.txt)), via M5Stack's
  **[M5GFX](https://github.com/m5stack/M5GFX)** fork (MIT): the graphics engine. Our ST7365P panel
  class follows the structure of LovyanGFX's `Panel_ST7789`.
- **M5Stack - [M5Unified](https://github.com/m5stack/M5Unified) /
  [M5Cardputer](https://github.com/m5stack/M5Cardputer)** (MIT): the `M5` / `M5Cardputer` API
  shapes that our Cardputer compatibility layer mirrors so existing code compiles.

## Inspiration & the community

- **jblanked - [Picoware](https://github.com/jblanked/Picoware)** (GPL-3.0) · YouTube
  **[jblanked](https://www.youtube.com/@jblanked)**. Picoware showed one codebase can target both
  the PicoCalc and the Cardputer - the idea behind this whole library. Used as reference only; no
  GPL code is included here.
- **skizzophrenic - [M5PORKCHOP_DualScreen](https://github.com/skizzophrenic/M5PORKCHOP_DualScreen)**
  (MIT) · YouTube **[TalkingSasquach](https://www.youtube.com/@TalkingSasquach)**. Great work on
  scaling this class of firmware to a bigger screen.
- **0ct0sec - [M5PORKCHOP](https://github.com/0ct0sec/M5PORKCHOP)** (MIT). Wanting to run it on a
  PicoCalc is why this library exists; [Porkocalc](https://github.com/thoughtfix/porkocalc) is the
  example app.
- **[Valleytechsolutions](https://www.youtube.com/@Valleytechsolutions)** - for showcasing so much
  of what this community builds, and helping people discover these projects.
- **kaust149 - [esp32S3Pico_Zephyr](https://github.com/kaust149/esp32S3Pico_Zephyr)** - an early
  ESP32-S3-on-PicoCalc experiment, useful for cross-checking the pin map.

## Bundled dependency

- **M5GFX** (`m5stack/M5GFX`) is fetched as a library dependency; see its own license.
