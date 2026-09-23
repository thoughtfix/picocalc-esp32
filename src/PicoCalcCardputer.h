#pragma once
// Cardputer compatibility layer: `M5` and `M5Cardputer` objects backed by the PicoCalc,
// so code written for the M5Stack Cardputer builds and runs with few or no changes.
//
// Use it INSTEAD of M5Unified and M5Cardputer. Don't list them in lib_deps. Put these two
// one-line files in your project's include/ folder:
//     include/M5Unified.h   ->  #include <PicoCalcCardputer.h>
//     include/M5Cardputer.h ->  #include <PicoCalcCardputer.h>
// examples/cardputer-compat shows the setup.
//
// The API shapes (class names, method signatures, KeysState fields, key codes) mirror
// M5Stack's MIT-licensed libraries so existing code compiles:
//   M5Unified:   https://github.com/m5stack/M5Unified
//   M5Cardputer: https://github.com/m5stack/M5Cardputer (Keyboard_Class, Keyboard_def.h)
// Everything behind them is PicoCalc code.
//
// Only what Cardputer apps commonly use is covered: Display, Keyboard, Power (battery),
// Speaker (tones), Rtc (system clock), Imu (stub, the PicoCalc has none). Anything missing
// fails at compile time, which makes gaps obvious.

#include <Arduino.h>
#include <M5GFX.h>
#include <vector>

#include "picocalc_audio.h"
#include "picocalc_display.h"
#include "picocalc_stm32.h"

// Cardputer key codes, same values as M5Cardputer's utility/Keyboard/Keyboard_def.h
#ifndef KEY_BACKSPACE
#define KEY_LEFT_CTRL 0x80
#define KEY_LEFT_SHIFT 0x81
#define KEY_LEFT_ALT 0x82
#define KEY_FN 0xff
#define KEY_OPT 0x00
#define KEY_BACKSPACE 0x2a
#define KEY_TAB 0x2b
#define KEY_ENTER 0x28
#endif

// Keyboard with the M5Cardputer Keyboard_Class API, fed by the PicoCalc STM32.
// PicoCalc keys are translated to what the Cardputer would report:
//   arrows -> ; . , /  (the Cardputer's arrow keys: up, down, left, right)
//   Esc -> `            Backspace and Del -> KEY_BACKSPACE    Enter -> KEY_ENTER
//   Tab -> KEY_TAB      Shift/Ctrl/Alt -> KEY_LEFT_SHIFT/CTRL/ALT
//   printable characters -> themselves (the STM32 already applies Shift and Caps Lock)
// F1-F10 and other PicoCalc-only keys have no Cardputer equivalent. Read them with
// picoCalcKeysHeld().
class Keyboard_Class {
public:
    struct KeysState {
        bool tab = false;
        bool fn = false;
        bool shift = false;
        bool ctrl = false;
        bool opt = false;
        bool alt = false;
        bool del = false;
        bool enter = false;
        bool space = false;
        uint8_t modifiers = 0;

        std::vector<char> word;
        std::vector<uint8_t> hid_keys;       // special keys only (tab/del/enter), no ASCII->HID table
        std::vector<uint8_t> modifier_keys;

        void reset();
    };

    void begin() {}
    void attach(picocalc::Stm32* mcu) { _mcu = mcu; }

    uint8_t isPressed() { return _held.size(); }
    bool isChange();
    bool isKeyPressed(char c);

    void updateKeyList();
    void updateKeysState();
    KeysState& keysState() { return _state; }

    bool capslocked() { return _capsLocked; }
    void setCapsLocked(bool isLocked) { _capsLocked = isLocked; }

    // PicoCalc native key codes currently held (picocalc::key::*, e.g. F1-F10).
    const std::vector<uint8_t>& picoCalcKeysHeld() const { return _heldNative; }

private:
    picocalc::Stm32* _mcu = nullptr;
    std::vector<uint8_t> _heldNative;       // PicoCalc codes
    std::vector<uint8_t> _deferredRelease;  // released in the same poll they were pressed
    std::vector<uint8_t> _held;             // Cardputer codes
    KeysState _state;
    uint8_t _lastSize = 0;
    uint32_t _lastPollMs = 0;
    bool _capsLocked = false;
};

namespace m5 {

using board_t = m5gfx::board_t;

struct rtc_time_t {
    std::int8_t hours;
    std::int8_t minutes;
    std::int8_t seconds;
};

struct rtc_date_t {
    std::int16_t year;
    std::int8_t month;
    std::int8_t date;
    std::int8_t weekDay;
};

struct rtc_datetime_t {
    rtc_date_t date;
    rtc_time_t time;
};

// Battery via the STM32. The PicoCalc reports a percentage only, so the voltage is an estimate.
class Power_Class {
public:
    enum is_charging_t { is_discharging = 0, is_charging, charge_unknown };

    void attach(picocalc::Stm32* mcu) { _mcu = mcu; }
    std::int32_t getBatteryLevel();
    is_charging_t isCharging();
    int16_t getBatteryVoltage();  // estimated from percent: 3300 mV + 9 mV per %
    int16_t getVBUSVoltage() { return -1; }

private:
    void refresh();
    picocalc::Stm32* _mcu = nullptr;
    int _raw = -1;
    uint32_t _lastMs = 0;
    bool _fresh = false;
};

// Non-blocking square-wave tones on both PicoCalc speakers. There's no WAV/PCM playback.
class Speaker_Class {
public:
    void attach(picocalc::Audio* audio) { _audio = audio; }
    bool begin() { return true; }
    void end() { stop(); }
    bool tone(float frequency, uint32_t duration = UINT32_MAX, int channel = -1, bool stop_current_sound = true);
    void stop();
    void stop(uint8_t) { stop(); }
    bool isPlaying() const { return _playing; }
    void setVolume(uint8_t volume) { _volume = volume; }
    uint8_t getVolume() const { return _volume; }

private:
    static void timerStop(void* arg);
    picocalc::Audio* _audio = nullptr;
    void* _timer = nullptr;
    volatile bool _playing = false;
    uint8_t _volume = 255;  // M5Unified defaults to 64, which is too quiet on a square wave
};

// System clock (set it with NTP or GPS). Year 1970 means "not set".
class RTC_Class {
public:
    bool isEnabled() const { return true; }
    rtc_datetime_t getDateTime();
    rtc_date_t getDate() { return getDateTime().date; }
    rtc_time_t getTime() { return getDateTime().time; }
    void setDateTime(const rtc_datetime_t& dt);
};

// The PicoCalc has no IMU. Reads return false and zeros.
class IMU_Class {
public:
    bool isEnabled() const { return false; }
    bool update() { return false; }
    bool getAccel(float* x, float* y, float* z);
    bool getGyro(float* x, float* y, float* z);
};

// Outside the class so it can be a default argument inside it (GCC rule for nested initializers).
struct m5_config_t {
    uint32_t serial_baudrate = 115200;
    bool clear_display = true;
    bool output_power = true;
    bool internal_imu = true;
    bool internal_rtc = true;
    bool internal_spk = true;
    bool internal_mic = true;
    bool external_imu = false;
    bool external_rtc = false;
    uint8_t led_brightness = 0;
};

class M5Unified {
public:
    using config_t = m5_config_t;

    M5Unified();
    config_t config() const { return config_t(); }
    void begin(config_t cfg = config_t());
    void update() {}
    board_t getBoard() const { return board_t::board_unknown; }

    picocalc::Display& Display;
    picocalc::Display& Lcd;
    Power_Class Power;
    Speaker_Class Speaker;
    RTC_Class Rtc;
    IMU_Class Imu;

    picocalc::Stm32& stm32() { return _mcu; }

private:
    picocalc::Stm32& _mcu;
    bool _begun = false;
};

class M5_CARDPUTER {
public:
    void begin(bool enableKeyboard = true);
    void begin(M5Unified::config_t cfg, bool enableKeyboard = true);
    void update();

    picocalc::Display& Display;
    picocalc::Display& Lcd;
    Power_Class& Power;
    Speaker_Class& Speaker;
    Keyboard_Class Keyboard;

    M5_CARDPUTER();

private:
    bool _enableKeyboard = true;
};

}  // namespace m5

extern m5::M5Unified M5;
extern m5::M5_CARDPUTER M5Cardputer;
