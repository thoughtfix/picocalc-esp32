#include "picocalc_display.h"
#include "picocalc_pins.h"

namespace picocalc {

Panel_ST7365P::Panel_ST7365P() {
    _cfg.panel_width = _cfg.memory_width = 320;
    _cfg.panel_height = 320;
    _cfg.memory_height = 480;
}

const uint8_t* Panel_ST7365P::getInitCommands(uint8_t listno) const {
    // Register values from ClockworkPi's PICOCALC display init in their PicoMite patch
    // (PicoMite by Geoff Graham and Peter Mather; PicoCalc changes by ClockworkPi):
    // https://github.com/clockworkpi/PicoCalc/blob/f91519806d4b2e0a62c4638a9f695cd5162c5479/Code/PicoMite/PicoMite.patch#L1163-L1241
    // MADCTL, COLMOD and INVON are left out here because Panel_LCD writes them from the
    // config (rotation, color depth, invert).
    static constexpr uint8_t list0[] = {
        0xF0, 1, 0xC3,  // command set control: enable part 1
        0xF0, 1, 0x96,  // command set control: enable part 2
        0xB4, 1, 0x00,  // display inversion control
        0xB7, 1, 0xC6,  // entry mode set
        0xB9, 2, 0x02, 0xE0,
        0xC0, 2, 0x80, 0x06,  // power control 1
        0xC1, 1, 0x15,        // power control 2
        0xC2, 1, 0xA7,        // power control 3
        0xC5, 1, 0x04,        // VCOM
        0xE8, 8, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xAA, 0x33,
        0xE0, 14, 0xF0, 0x06, 0x0F, 0x05, 0x04, 0x20, 0x37, 0x33, 0x4C, 0x37, 0x13, 0x14, 0x2B, 0x31,
        0xE1, 14, 0xF0, 0x11, 0x1B, 0x11, 0x0F, 0x0A, 0x37, 0x43, 0x4C, 0x37, 0x13, 0x13, 0x2C, 0x32,
        0xF0, 1, 0x3C,  // command set control: disable part 1
        0xF0, 1, 0x69,  // command set control: disable part 2
        CMD_SLPOUT, 0 + CMD_INIT_DELAY, 120,
        CMD_DISPON, 0 + CMD_INIT_DELAY, 20,
        0xFF, 0xFF,
    };
    return listno == 0 ? list0 : nullptr;
}

Display::Display(uint32_t spiWriteHz) {
    {
        auto cfg = _bus.config();
        cfg.spi_host = SPI2_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = spiWriteHz;
        cfg.freq_read = 16000000;
        cfg.spi_3wire = false;
        cfg.use_lock = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = pins::LCD_SCK;
        cfg.pin_mosi = pins::LCD_MOSI;
        cfg.pin_miso = pins::LCD_MISO;
        cfg.pin_dc = pins::LCD_DC;
        _bus.config(cfg);
        _panel.setBus(&_bus);
    }
    {
        auto cfg = _panel.config();
        cfg.pin_cs = pins::LCD_CS;
        cfg.pin_rst = pins::LCD_RST;
        cfg.pin_busy = -1;
        cfg.panel_width = 320;
        cfg.panel_height = 320;
        cfg.memory_width = 320;
        cfg.memory_height = 480;
        cfg.offset_x = 0;
        cfg.offset_y = 0;
        // Rotation 0 must produce MADCTL = MX | BGR (0x48), matching ClockworkPi's init.
        // Panel_LCD's internal rotation 6 is MX|MH; offset 6 maps user rotation 0 onto it.
        cfg.offset_rotation = 6;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits = 1;
        cfg.readable = false;
        cfg.invert = true;
        cfg.rgb_order = false;
        cfg.dlen_16bit = false;
        cfg.bus_shared = false;
        _panel.config(cfg);
    }
    setPanel(&_panel);
}

}  // namespace picocalc
