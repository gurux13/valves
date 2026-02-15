#pragma once
#include <functional>
#include <thread>
#include "ids.h"
class Button {
public:
    Button(int pin, ButtonId id);
    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;
    void init();
    bool is_pressed();
private:
    int pin;
    ButtonId id;
    std::thread button_thread;
    bool pressed = false;
};