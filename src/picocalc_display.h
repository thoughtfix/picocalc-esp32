#pragma once
// PicoCalc 320x320 ST7365P LCD as an M5GFX/LovyanGFX device.

#include <M5GFX.h>
#include <lgfx/v1/panel/Panel_LCD.hpp>

namespace picocalc {

// The ST7365P is a Sitronix ST7796-family controller that speaks standard MIPI DCS, so it
// can reuse the generic Panel_LCD. Only the vendor init sequence is specific to it.
// Class layout follows LovyanGFX's Panel_ST7789 (lovyan03, FreeBSD license):
// https://github.com/lovyan03/LovyanGFX/blob/master/src/lgfx/v1/panel/Panel_ST7789.hpp
struct Panel_ST7365P : public lgfx::Panel_LCD {
    Panel_ST7365P();

protected:
    const uint8_t* getInitCommands(uint8_t listno) const override;
};

class Display : public lgfx::LGFX_Device {
public:
    explicit Display(uint32_t spiWriteHz = 40000000);

private:
    lgfx::Bus_SPI _bus;
    Panel_ST7365P _panel;
};

}  // namespace picocalc
