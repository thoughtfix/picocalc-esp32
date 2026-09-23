#include "picocalc_stm32.h"
#include "picocalc_pins.h"

namespace picocalc {

namespace {
constexpr uint8_t REG_VER = 0x01;
constexpr uint8_t REG_KEY = 0x04;
constexpr uint8_t REG_BKL = 0x05;
constexpr uint8_t REG_FIF = 0x09;
constexpr uint8_t REG_BK2 = 0x0A;
constexpr uint8_t REG_BAT = 0x0B;
constexpr uint8_t WRITE_MASK = 0x80;
}  // namespace

const char* keyName(uint8_t code) {
    switch (code) {
        case key::BACKSPACE: return "BACKSPACE";
        case key::TAB: return "TAB";
        case key::ENTER: return "ENTER";
        case key::ALT: return "ALT";
        case key::SHIFT_L: return "LSHIFT";
        case key::SHIFT_R: return "RSHIFT";
        case key::SYM: return "SYM";
        case key::CTRL: return "CTRL";
        case key::ESC: return "ESC";
        case key::LEFT: return "LEFT";
        case key::UP: return "UP";
        case key::DOWN: return "DOWN";
        case key::RIGHT: return "RIGHT";
        case key::CAPS_LOCK: return "CAPSLOCK";
        case key::BREAK: return "BREAK";
        case key::INSERT: return "INSERT";
        case key::HOME: return "HOME";
        case key::DEL: return "DEL";
        case key::END: return "END";
        case key::PAGE_UP: return "PGUP";
        case key::PAGE_DOWN: return "PGDN";
        case key::F1: return "F1";
        case key::F2: return "F2";
        case key::F3: return "F3";
        case key::F4: return "F4";
        case key::F5: return "F5";
        case key::F6: return "F6";
        case key::F7: return "F7";
        case key::F8: return "F8";
        case key::F9: return "F9";
        case key::F10: return "F10";
        case key::POWER: return "POWER";
        case ' ': return "SPACE";
    }
    static char buf[8];
    if (code >= 0x21 && code < 0x7F) {
        buf[0] = (char)code;
        buf[1] = 0;
    } else {
        snprintf(buf, sizeof(buf), "?%02X", code);
    }
    return buf;
}

const char* keyStateName(uint8_t state) {
    switch (state) {
        case KEY_PRESSED: return "pressed";
        case KEY_HOLD: return "hold";
        case KEY_RELEASED: return "released";
        default: return "idle";
    }
}

// If the ESP32 resets in the middle of a read, the STM32 can be left holding SDA low.
// Clocking SCL until it lets go is the standard I2C bus-clear procedure (I2C spec, section 3.1.16).
static void clearBus() {
    pinMode(pins::KBD_SDA, INPUT_PULLUP);
    pinMode(pins::KBD_SCL, INPUT_PULLUP);
    delayMicroseconds(10);
    for (int i = 0; i < 9 && digitalRead(pins::KBD_SDA) == LOW; i++) {
        pinMode(pins::KBD_SCL, OUTPUT_OPEN_DRAIN);
        digitalWrite(pins::KBD_SCL, LOW);
        delayMicroseconds(50);
        digitalWrite(pins::KBD_SCL, HIGH);
        delayMicroseconds(50);
    }
    pinMode(pins::KBD_SCL, INPUT_PULLUP);
}

bool Stm32::begin(TwoWire& wire, uint32_t hz) {
    _wire = &wire;
    clearBus();
    return _wire->begin(pins::KBD_SDA, pins::KBD_SCL, hz);
}

bool Stm32::transferOnce(uint8_t reg, bool write, uint8_t value, uint8_t out[2]) {
    _wire->beginTransmission(ADDRESS);
    _wire->write(write ? (reg | WRITE_MASK) : reg);
    if (write) _wire->write(value);
    if (_wire->endTransmission() != 0) return false;
    if (_readDelayMs) delay(_readDelayMs);
    if (_wire->requestFrom((int)ADDRESS, 2) != 2) return false;
    out[0] = _wire->read();
    out[1] = _wire->read();
    return true;
}

bool Stm32::transfer(uint8_t reg, bool write, uint8_t value, uint8_t out[2]) {
    if (!_wire) return false;
    for (int attempt = 0; attempt < 3; attempt++) {
        if (transferOnce(reg, write, value, out)) {
            _responding = true;
            return true;
        }
        _errors++;
        delay(5);
    }
    _responding = false;
    return false;
}

bool BacklightLight::init(uint8_t brightness) {
    setBrightness(brightness);
    return true;
}

void BacklightLight::setBrightness(uint8_t brightness) {
    _mcu.setLcdBacklight(brightness);
}

int Stm32::firmwareVersion() {
    uint8_t b[2];
    return transfer(REG_VER, false, 0, b) ? b[1] : -1;
}

bool Stm32::readKey(KeyEvent& ev) {
    uint8_t b[2];
    if (!transfer(REG_FIF, false, 0, b)) return false;
    ev.state = b[0];
    ev.code = b[1];
    return ev.state != KEY_IDLE;
}

int Stm32::keyStatus() {
    uint8_t b[2];
    return transfer(REG_KEY, false, 0, b) ? b[0] : -1;
}

int Stm32::batteryRaw() {
    uint8_t b[2];
    return transfer(REG_BAT, false, 0, b) ? b[1] : -1;
}

int Stm32::lcdBacklight() {
    uint8_t b[2];
    return transfer(REG_BKL, false, 0, b) ? b[1] : -1;
}

int Stm32::setLcdBacklight(uint8_t level) {
    uint8_t b[2];
    return transfer(REG_BKL, true, level, b) ? b[1] : -1;
}

int Stm32::kbdBacklight() {
    uint8_t b[2];
    return transfer(REG_BK2, false, 0, b) ? b[1] : -1;
}

int Stm32::setKbdBacklight(uint8_t level) {
    uint8_t b[2];
    return transfer(REG_BK2, true, level, b) ? b[1] : -1;
}

}  // namespace picocalc
