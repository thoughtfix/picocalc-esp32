#include "picocalc_audio.h"
#include "picocalc_pins.h"

// arduino-esp32 3.x replaced the channel-based LEDC API with a pin-based one.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
#define PICOCALC_LEDC_PIN_API 1
#endif

namespace picocalc {

void Audio::begin() {
#ifdef PICOCALC_LEDC_PIN_API
    ledcAttachChannel(pins::AUDIO_L, 1000, 10, CH_L);
    ledcAttachChannel(pins::AUDIO_R, 1000, 10, CH_R);
#else
    ledcSetup(CH_L, 1000, 10);
    ledcSetup(CH_R, 1000, 10);
    ledcAttachPin(pins::AUDIO_L, CH_L);
    ledcAttachPin(pins::AUDIO_R, CH_R);
#endif
    stop();
    _ready = true;
}

void Audio::tone(uint32_t hz, uint32_t ms) {
    if (!_ready) begin();
#ifdef PICOCALC_LEDC_PIN_API
    ledcWriteTone(pins::AUDIO_L, hz);
    ledcWriteTone(pins::AUDIO_R, hz);
#else
    ledcWriteTone(CH_L, hz);
    ledcWriteTone(CH_R, hz);
#endif
    delay(ms);
    stop();
}

void Audio::start(uint32_t hz, uint8_t volume) {
    if (!_ready) begin();
    if (volume == 0 || hz == 0) {
        stop();
        return;
    }
    // ledcWriteTone() sets 50% duty (512 of 1024), the loudest a square wave gets.
    // Lower duty is quieter.
    uint32_t duty = 512u * volume / 255u;
#ifdef PICOCALC_LEDC_PIN_API
    ledcWriteTone(pins::AUDIO_L, hz);
    ledcWriteTone(pins::AUDIO_R, hz);
    if (volume < 255) {
        ledcWrite(pins::AUDIO_L, duty);
        ledcWrite(pins::AUDIO_R, duty);
    }
#else
    ledcWriteTone(CH_L, hz);
    ledcWriteTone(CH_R, hz);
    if (volume < 255) {
        ledcWrite(CH_L, duty);
        ledcWrite(CH_R, duty);
    }
#endif
}

void Audio::stop() {
#ifdef PICOCALC_LEDC_PIN_API
    ledcWrite(pins::AUDIO_L, 0);
    ledcWrite(pins::AUDIO_R, 0);
#else
    ledcWrite(CH_L, 0);
    ledcWrite(CH_R, 0);
#endif
}

}  // namespace picocalc
