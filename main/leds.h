#pragma once
#include <vector>
#include "led.h"
class Leds {
    public:
        enum Color {GREEN = 0, YELLOW = 1, RED = 2};
        static Leds& instance();
        Leds();
        Led& get(Color color);
        const std::vector<Led*>& get_all();
        Leds(const Leds&) = delete;
        Leds& operator=(const Leds&) = delete;
    private:
        std::vector<Led*> leds;
};