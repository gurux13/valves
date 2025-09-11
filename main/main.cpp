#include "valves.h"
#include "esp_log.h"
#include "leds.h"
#include "buttons.h"
#include "led.h"
#include "zigbee.h"
extern "C" void app_main(void) {
    ESP_LOGI("main", "Starting valve control application");
    Valves& valves = Valves::get_instance();
    valves.init();
    // return;
    auto& buttons = Buttons::instance();
    buttons.init();
    auto& leds = Leds::instance();
    // leds.get(Leds::Color::GREEN).blink(500);
    // leds.get(Leds::Color::YELLOW).blink(250);
    leds.get(Leds::Color::RED).blink(100);

   init_zigbee();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}