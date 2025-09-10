#pragma once
#include <functional>
#include <thread>

class Button {
public:
    Button(int pin);
    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;
    void set_callback(std::function<void()> callback);
    void init();
private:
    int pin;
    std::function<void()> callback;
    std::thread button_thread;
};