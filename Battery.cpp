#include "Battery.h"

#include <algorithm>

std::uint16_t const BM_BATTERY::width  = 16;
std::uint16_t const BM_BATTERY::height = 8;

void BM_BATTERY::draw(Adafruit_GFX & gfx, std::int16_t x, std::int16_t y, std::uint8_t percent, std::uint16_t color)
{
    gfx.drawRect(x, y, 15, 8, color);
    gfx.fillRect(x + 15, y + 3, 1, 2, color);
    percent = std::min(percent, std::uint8_t {99}) / 20;
    for (std::uint8_t p = 0; p < percent; ++p)
    {
        gfx.fillRect(x + 2 + (p * 3), y + 2, 2, 4, color);
    }
}
