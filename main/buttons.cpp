#include "buttons.h"
#include "pinout.h"
Buttons::Buttons() : buttons{Button(PIN_BTN_NORMAL), Button(PIN_BTN_BYPASS), Button(PIN_BTN_CLOSE)} {}
void Buttons::init() {
    for (auto& button : buttons) {
        button.init();
    }
}

Button& Buttons::get(ButtonId id) {
    return buttons.at(static_cast<size_t>(id));
}

Buttons& Buttons::instance() {
    static Buttons instance;
    return instance;
}