#ifndef BATTERY_H
#define BATTERY_H

#include <Adafruit_GFX.h>

#include <cstdint>

struct BM_BATTERY
{
    static std::uint16_t const width, height;
    static void draw(Adafruit_GFX & gfx, std::int16_t x, std::int16_t y, std::uint8_t percent = 100, std::uint16_t color = 1);
};

#endif
