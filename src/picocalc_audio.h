#pragma once
// PicoCalc speaker output. The PicoCalc low-pass filters a PWM signal on each channel into
// its AW8010 amps. The S3 has no DAC, so LEDC PWM drives both channels. The STM32 switches
// the amps on and off (headphone detect), so there is nothing else to enable here.

#include <Arduino.h>

namespace picocalc {

class Audio {
public:
    void begin();
    void tone(uint32_t hz, uint32_t ms);  // blocking square-wave beep on both channels
    void start(uint32_t hz, uint8_t volume = 255);  // non-blocking; plays until stop()
    void stop();

private:
    static constexpr uint8_t CH_L = 6;
    static constexpr uint8_t CH_R = 7;
    bool _ready = false;
};

}  // namespace picocalc
