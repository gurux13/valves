#include "leds.h"
#include "pinout.h"

Leds::Leds() {
    leds.push_back(new Led(PIN_LED_G));
    leds.push_back(new Led(PIN_LED_Y));
    leds.push_back(new Led(PIN_LED_R));
}

Leds& Leds::instance() {
    static Leds instance;
    return instance;
}

Led& Leds::get(Color color) {
    return *leds.at(static_cast<size_t>(color));
}

const std::vector<Led*>& Leds::get_all() {
    return leds;
}

