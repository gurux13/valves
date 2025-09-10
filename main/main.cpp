#include "valves.h"
#include "esp_log.h"
#include "leds.h"
#include "buttons.h"
#include "led.h"
#include "zigbee.h"
extern "C" void app_main(void) {
    ESP_LOGI("main", "Starting valve control application");
    // Valves& valves = Valves::get_instance();
    // valves.init();
    // for (const auto& valve : valves.get_all_valves()) {
    //     ESP_LOGI("main", "Opening valve: %s", valve->get_name().c_str());
    //     valve->open();
    // }
    // ESP_LOGI("main", "Entering main loop");
    // while (true) {
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    //     for (const auto& valve : valves.get_all_valves()) {
    //         ESP_LOGI("main", "Valve %s is in state %s", valve->get_name().c_str(), Valve::state_to_string(valve->get_state()).c_str());
    //         if (valve->get_state() == Valve::State::OPEN) {
    //             valve->close();
    //         } else if (valve->get_state() == Valve::State::CLOSED) {
    //             valve->open();
    //         }

    //     }

    // }
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