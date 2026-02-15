#include "buttons.h"
#include "pinout.h"
#include "ids.h"
Buttons::Buttons() : buttons{Button(PIN_BTN_NORMAL, ButtonId::NORMAL), Button(PIN_BTN_BYPASS, ButtonId::BYPASS), Button(PIN_BTN_CLOSE, ButtonId::CLOSE)} {}
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