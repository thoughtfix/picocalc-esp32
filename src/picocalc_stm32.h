#pragma once
// Link to the PicoCalc's STM32 "keyboard" MCU. It owns the keyboard matrix, the LCD and
// keyboard backlights, and the AXP2101 PMU (battery, power off).
//
// Register protocol from ClockworkPi's keyboard firmware and Pico driver:
// https://github.com/clockworkpi/PicoCalc/blob/f91519806d4b2e0a62c4638a9f695cd5162c5479/Code/picocalc_keyboard/reg.h
// https://github.com/clockworkpi/PicoCalc/blob/f91519806d4b2e0a62c4638a9f695cd5162c5479/Code/picocalc_keyboard/picocalc_keyboard.ino#L109-L222
// https://github.com/clockworkpi/PicoCalc/blob/f91519806d4b2e0a62c4638a9f695cd5162c5479/Code/picocalc_helloworld/i2ckbd/i2ckbd.c
// Each transaction writes a register byte (bit 7 set = write, plus one value byte), then
// reads back 2 bytes.

#include <Arduino.h>
#include <Wire.h>
#include <M5GFX.h>

namespace picocalc {

enum KeyState : uint8_t {
    KEY_IDLE = 0,
    KEY_PRESSED = 1,
    KEY_HOLD = 2,
    KEY_RELEASED = 3,
};

// Special key codes, from picocalc_keyboard/keyboard.h (same repo and commit as above).
// Printable keys report their ASCII value.
namespace key {
constexpr uint8_t BACKSPACE = 0x08;
constexpr uint8_t TAB = 0x09;
constexpr uint8_t ENTER = 0x0A;
constexpr uint8_t ALT = 0xA1;
constexpr uint8_t SHIFT_L = 0xA2;
constexpr uint8_t SHIFT_R = 0xA3;
constexpr uint8_t SYM = 0xA4;
constexpr uint8_t CTRL = 0xA5;
constexpr uint8_t ESC = 0xB1;
constexpr uint8_t LEFT = 0xB4;
constexpr uint8_t UP = 0xB5;
constexpr uint8_t DOWN = 0xB6;
constexpr uint8_t RIGHT = 0xB7;
constexpr uint8_t CAPS_LOCK = 0xC1;
constexpr uint8_t BREAK = 0xD0;
constexpr uint8_t INSERT = 0xD1;
constexpr uint8_t HOME = 0xD2;
constexpr uint8_t DEL = 0xD4;
constexpr uint8_t END = 0xD5;
constexpr uint8_t PAGE_UP = 0xD6;
constexpr uint8_t PAGE_DOWN = 0xD7;
constexpr uint8_t F1 = 0x81;
constexpr uint8_t F2 = 0x82;
constexpr uint8_t F3 = 0x83;
constexpr uint8_t F4 = 0x84;
constexpr uint8_t F5 = 0x85;
constexpr uint8_t F6 = 0x86;
constexpr uint8_t F7 = 0x87;
constexpr uint8_t F8 = 0x88;
constexpr uint8_t F9 = 0x89;
constexpr uint8_t F10 = 0x90;
constexpr uint8_t POWER = 0x91;
}  // namespace key

struct KeyEvent {
    uint8_t state;
    uint8_t code;
};

// Human-readable key name; printable ASCII comes back as the character itself.
const char* keyName(uint8_t code);
const char* keyStateName(uint8_t state);

class Stm32;

// Lets M5GFX's setBrightness() drive the PicoCalc LCD backlight, which the STM32 controls.
// Attach with display.getPanel()->setLight(&light) before display.init().
class BacklightLight : public lgfx::ILight {
public:
    explicit BacklightLight(Stm32& mcu) : _mcu(mcu) {}
    bool init(uint8_t brightness) override;
    void setBrightness(uint8_t brightness) override;

private:
    Stm32& _mcu;
};

class Stm32 {
public:
    static constexpr uint8_t ADDRESS = 0x1F;

    // ClockworkPi's Pico driver runs this bus at 10 kHz and waits 16 ms between the
    // register write and the read. We start with the same values.
    bool begin(TwoWire& wire = Wire, uint32_t hz = 10000);
    void setReadDelayMs(uint16_t ms) { _readDelayMs = ms; }
    uint16_t readDelayMs() const { return _readDelayMs; }
    // Failed attempts, including ones a retry recovered from.
    uint32_t errorCount() const { return _errors; }
    // False once a transaction has failed all its retries. True again after the next success.
    bool responding() const { return _responding; }

    int firmwareVersion();              // BIOS version byte, or -1
    bool readKey(KeyEvent& ev);         // false when the FIFO is empty or on error
    int keyStatus();                    // FIFO count | caps (0x20) | num (0x40), or -1
    int batteryRaw();                   // bits 0-6 percent, bit 7 charging, or -1
    int lcdBacklight();                 // 0-255, or -1
    int setLcdBacklight(uint8_t level); // returns the level the STM32 applied, or -1
    int kbdBacklight();
    int setKbdBacklight(uint8_t level);

private:
    bool transfer(uint8_t reg, bool write, uint8_t value, uint8_t out[2]);
    bool transferOnce(uint8_t reg, bool write, uint8_t value, uint8_t out[2]);

    TwoWire* _wire = nullptr;
    uint16_t _readDelayMs = 16;
    uint32_t _errors = 0;
    bool _responding = true;
};

}  // namespace picocalc
