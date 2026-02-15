#include "valves.h"
#include "esp_log.h"
#include "leds.h"
#include "buttons.h"
#include "led.h"
#include "zigbee.h"
#include "logic.h"
extern "C" void app_main(void) {
    ESP_LOGI("main", "Starting valve control application");
    Valves& valves = Valves::get_instance();
    valves.init();
    auto& buttons = Buttons::instance();
    buttons.init();
    auto& leds = Leds::instance();
    Logic logic(leds, buttons, valves);
    zb_set_logic(&logic);
    
    logic.start();
    init_zigbee(logic.should_factory_reset);

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        valves.step();
        logic.step();
    }
}