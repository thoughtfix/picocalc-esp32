#include "PicoCalcCardputer.h"

#include <esp_timer.h>
#include <sys/time.h>
#include <time.h>

#include "picocalc_pins.h"

namespace {
picocalc::Display g_display;
picocalc::Stm32 g_mcu;
picocalc::Audio g_audio;
picocalc::BacklightLight g_light(g_mcu);

// Returns the Cardputer key code a PicoCalc key stands in for, or -1 if there isn't one.
int toCardputer(uint8_t code) {
    namespace key = picocalc::key;
    switch (code) {
        case key::UP: return ';';
        case key::DOWN: return '.';
        case key::LEFT: return ',';
        case key::RIGHT: return '/';
        case key::ESC: return '`';
        case key::BACKSPACE:
        case key::DEL: return KEY_BACKSPACE;
        case key::ENTER: return KEY_ENTER;
        case key::TAB: return KEY_TAB;
        case key::SHIFT_L:
        case key::SHIFT_R: return KEY_LEFT_SHIFT;
        case key::CTRL: return KEY_LEFT_CTRL;
        case key::ALT: return KEY_LEFT_ALT;
    }
    if (code >= 0x20 && code < 0x7F) return code;
    return -1;
}

template <typename T>
bool contains(const std::vector<T>& v, T x) {
    for (auto& e : v)
        if (e == x) return true;
    return false;
}

template <typename T>
void eraseAll(std::vector<T>& v, T x) {
    for (size_t i = 0; i < v.size();) {
        if (v[i] == x) v.erase(v.begin() + i);
        else i++;
    }
}
}  // namespace

m5::M5Unified M5;
m5::M5_CARDPUTER M5Cardputer;

// ---------------------------------------------------------------- Keyboard_Class

void Keyboard_Class::KeysState::reset() {
    tab = fn = shift = ctrl = opt = alt = del = enter = space = false;
    modifiers = 0;
    word.clear();
    hid_keys.clear();
    modifier_keys.clear();
}

void Keyboard_Class::updateKeyList() {
    // A key pressed and released within one poll would otherwise never be seen by code that
    // checks isPressed()/isKeyPressed() once per loop. Such releases are held back one poll.
    for (uint8_t code : _deferredRelease) eraseAll(_heldNative, code);
    _deferredRelease.clear();

    uint32_t now = millis();
    if (_mcu && now - _lastPollMs >= 10) {
        _lastPollMs = now;
        std::vector<uint8_t> pressedThisPoll;
        picocalc::KeyEvent ev;
        for (int i = 0; i < 32 && _mcu->readKey(ev); i++) {
            if (ev.state == picocalc::KEY_PRESSED) {
                if (!contains(_heldNative, ev.code)) _heldNative.push_back(ev.code);
                pressedThisPoll.push_back(ev.code);
            } else if (ev.state == picocalc::KEY_RELEASED) {
                if (contains(pressedThisPoll, ev.code)) _deferredRelease.push_back(ev.code);
                else eraseAll(_heldNative, ev.code);
            }
        }
    }

    _held.clear();
    for (uint8_t code : _heldNative) {
        int c = toCardputer(code);
        if (c >= 0 && !contains(_held, (uint8_t)c)) _held.push_back((uint8_t)c);
    }
}

bool Keyboard_Class::isChange() {
    // Same rule as M5Cardputer: "changed" means the number of held keys changed.
    uint8_t size = _held.size();
    if (size != _lastSize) {
        _lastSize = size;
        return true;
    }
    return false;
}

bool Keyboard_Class::isKeyPressed(char c) {
    return contains(_held, (uint8_t)c);
}

// Two passes, modifiers first, following M5Cardputer's Keyboard_Class::updateKeysState():
// https://github.com/m5stack/M5Cardputer/blob/1.1.1/src/utility/Keyboard/Keyboard.cpp
// (MIT, M5Stack). Characters come from the STM32 already shifted, so `word` holds them as-is.
void Keyboard_Class::updateKeysState() {
    _state.reset();
    for (uint8_t code : _held) {
        switch (code) {
            case KEY_LEFT_CTRL: _state.ctrl = true; break;
            case KEY_LEFT_SHIFT: _state.shift = true; break;
            case KEY_LEFT_ALT: _state.alt = true; break;
            default: continue;
        }
        _state.modifiers |= (1 << (code - 0x80));
        _state.modifier_keys.push_back(code);
    }
    for (uint8_t code : _held) {
        switch (code) {
            case KEY_LEFT_CTRL:
            case KEY_LEFT_SHIFT:
            case KEY_LEFT_ALT: continue;
            case KEY_TAB: _state.tab = true; _state.hid_keys.push_back(code); continue;
            case KEY_BACKSPACE: _state.del = true; _state.hid_keys.push_back(code); continue;
            case KEY_ENTER: _state.enter = true; _state.hid_keys.push_back(code); continue;
        }
        if (code == ' ') _state.space = true;
        _state.word.push_back((char)code);
    }
}

namespace m5 {

// ---------------------------------------------------------------- Power_Class

void Power_Class::refresh() {
    uint32_t now = millis();
    if (_fresh && now - _lastMs < 2000) return;
    if (!_mcu) return;
    int raw = _mcu->batteryRaw();
    if (raw >= 0) {
        _raw = raw;
        _fresh = true;
    }
    _lastMs = now;
}

std::int32_t Power_Class::getBatteryLevel() {
    refresh();
    return _raw < 0 ? -1 : (_raw & 0x7F);
}

Power_Class::is_charging_t Power_Class::isCharging() {
    refresh();
    if (_raw < 0) return charge_unknown;
    return (_raw & 0x80) ? is_charging : is_discharging;
}

int16_t Power_Class::getBatteryVoltage() {
    int32_t pct = getBatteryLevel();
    return pct < 0 ? 0 : (int16_t)(3300 + pct * 9);
}

// ---------------------------------------------------------------- Speaker_Class

void Speaker_Class::timerStop(void* arg) {
    static_cast<Speaker_Class*>(arg)->stop();
}

bool Speaker_Class::tone(float frequency, uint32_t duration, int, bool) {
    if (!_audio) return false;
    if (!_timer) {
        esp_timer_create_args_t args = {};
        args.callback = &Speaker_Class::timerStop;
        args.arg = this;
        args.name = "pc_tone";
        esp_timer_handle_t handle;
        if (esp_timer_create(&args, &handle) != ESP_OK) return false;
        _timer = handle;
    }
    esp_timer_stop((esp_timer_handle_t)_timer);
    _audio->start((uint32_t)frequency, _volume);
    _playing = true;
    if (duration != UINT32_MAX) esp_timer_start_once((esp_timer_handle_t)_timer, (uint64_t)duration * 1000);
    return true;
}

void Speaker_Class::stop() {
    if (_timer) esp_timer_stop((esp_timer_handle_t)_timer);
    if (_audio) _audio->stop();
    _playing = false;
}

// ---------------------------------------------------------------- RTC_Class / IMU_Class

rtc_datetime_t RTC_Class::getDateTime() {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    rtc_datetime_t dt;
    dt.date.year = t.tm_year + 1900;
    dt.date.month = t.tm_mon + 1;
    dt.date.date = t.tm_mday;
    dt.date.weekDay = t.tm_wday;
    dt.time.hours = t.tm_hour;
    dt.time.minutes = t.tm_min;
    dt.time.seconds = t.tm_sec;
    return dt;
}

void RTC_Class::setDateTime(const rtc_datetime_t& dt) {
    struct tm t = {};
    t.tm_year = dt.date.year - 1900;
    t.tm_mon = dt.date.month - 1;
    t.tm_mday = dt.date.date;
    t.tm_hour = dt.time.hours;
    t.tm_min = dt.time.minutes;
    t.tm_sec = dt.time.seconds;
    struct timeval tv = {mktime(&t), 0};
    settimeofday(&tv, nullptr);
}

bool IMU_Class::getAccel(float* x, float* y, float* z) {
    if (x) *x = 0;
    if (y) *y = 0;
    if (z) *z = 0;
    return false;
}

bool IMU_Class::getGyro(float* x, float* y, float* z) {
    return getAccel(x, y, z);
}

// ---------------------------------------------------------------- M5Unified / M5_CARDPUTER

M5Unified::M5Unified() : Display(g_display), Lcd(g_display), _mcu(g_mcu) {}

void M5Unified::begin(config_t cfg) {
    if (_begun) return;
    _begun = true;

    picocalc::safePins();
    if (cfg.serial_baudrate) Serial.begin(cfg.serial_baudrate);

    _mcu.begin(Wire);
    // Keep the STM32's proven 16 ms write-to-read gap (ClockworkPi's own driver uses it, and
    // HWTEST 01 verified it at 90/90 keys). A 0 ms delay starves the controller under load:
    // battery reads return -1 and the keyboard goes dead. Don't set it to 0 here.

    g_display.getPanel()->setLight(&g_light);
    g_display.init();
    g_display.setColorDepth(16);
    if (cfg.clear_display) g_display.fillScreen(TFT_BLACK);

    g_audio.begin();
    Power.attach(&_mcu);
    Speaker.attach(&g_audio);
}

M5_CARDPUTER::M5_CARDPUTER()
    : Display(M5.Display), Lcd(M5.Display), Power(M5.Power), Speaker(M5.Speaker) {}

void M5_CARDPUTER::begin(bool enableKeyboard) {
    begin(M5.config(), enableKeyboard);
}

void M5_CARDPUTER::begin(M5Unified::config_t cfg, bool enableKeyboard) {
    M5.begin(cfg);
    _enableKeyboard = enableKeyboard;
    Keyboard.attach(&M5.stm32());
    Keyboard.begin();
}

void M5_CARDPUTER::update() {
    M5.update();
    if (_enableKeyboard) {
        Keyboard.updateKeyList();
        Keyboard.updateKeysState();
    }
}

}  // namespace m5
