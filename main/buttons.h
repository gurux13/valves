#pragma once
#include "button.h"
#include <array>
class Buttons {
public:

    void init();
    static Buttons& instance();
    Button& get(ButtonId id);
private:
    Buttons();
    Buttons(const Buttons&) = delete;
    Buttons& operator=(const Buttons&) = delete;
    std::array<Button, 3> buttons;
};