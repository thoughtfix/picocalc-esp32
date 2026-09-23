#pragma once
// Waveshare ESP32-S3-Pico seated in a ClockworkPi PicoCalc: pin map.
//
// Derived from two schematics:
// - ClockworkPi PicoCalc mainboard V2.0, "PICO" block:
//   https://github.com/clockworkpi/PicoCalc/blob/f91519806d4b2e0a62c4638a9f695cd5162c5479/clockwork_Mainboard_V2.0_Schematic.pdf
// - Waveshare ESP32-S3-Pico, U6 "Pico no debug" header table:
//   https://files.waveshare.com/upload/a/a7/ESP32-S3-Pico-SCH.pdf
// The comment on each line gives the Pico GPIO that the PicoCalc was designed around.

namespace picocalc {
namespace pins {

// LCD, ST7365P on SPI (Pico SPI1)
constexpr int LCD_SCK  = 35;  // GP10
constexpr int LCD_MOSI = 36;  // GP11
constexpr int LCD_MISO = 37;  // GP12
constexpr int LCD_CS   = 38;  // GP13
constexpr int LCD_DC   = 39;  // GP14
constexpr int LCD_RST  = 40;  // GP15

// microSD on SPI (Pico SPI0)
constexpr int SD_MISO = 42;  // GP16
constexpr int SD_CS   = 41;  // GP17
constexpr int SD_SCK  = 1;   // GP18
constexpr int SD_MOSI = 2;   // GP19
constexpr int SD_DET  = 6;   // GP22

// STM32 keyboard / power controller on I2C, address 0x1F (Pico I2C1)
constexpr int KBD_SDA = 17;  // GP6
constexpr int KBD_SCL = 18;  // GP7

// Speaker PWM into the RC filter and AW8010 amps
constexpr int AUDIO_L = 7;   // GP26
constexpr int AUDIO_R = 8;   // GP27

// Onboard PicoCalc 8 MB PSRAM (ESP-PSRAM64H). The S3 does not use it. Hold CE high so it
// never drives the shared lines on the "Core GPIOs" header.
constexpr int PSRAM_CS = 4;  // GP20

// "Core GPIOs" external header, shared with the onboard PSRAM bus
constexpr int HDR_GP2  = 13;  // RAM_TX  (PSRAM SI)
constexpr int HDR_GP3  = 14;  // RAM_RX  (PSRAM SO)
constexpr int HDR_GP4  = 15;  // RAM_IO2
constexpr int HDR_GP5  = 16;  // RAM_IO3
constexpr int HDR_GP21 = 5;   // RAM_SCK
constexpr int HDR_GP28 = 9;   // ADC1_CH8

// "Mainboard GPIOs" header UARTs
constexpr int UART0_TX = 11;  // GP0, PicoCalc USB-C CH340 via the DIP-switch mux
constexpr int UART0_RX = 12;  // GP1
constexpr int UART1_TX = 33;  // GP8, also wired to STM32 UART3
constexpr int UART1_RX = 34;  // GP9

// Pico ADC_VREF pad, which is a plain GPIO on the S3. How the PicoCalc terminates it is
// unverified, so NEVER drive this pin.
constexpr int ADC_VREF_DO_NOT_DRIVE = 10;

// ESP32-S3-Pico onboard WS2812 (not on the header)
constexpr int RGB_LED = 21;

}  // namespace pins

// Put board-level pins into a known safe state. Call first thing in setup().
void safePins();

}  // namespace picocalc
