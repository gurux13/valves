#include "button.h"
#include <array>
class Buttons {
public:
    enum ButtonId {
        NORMAL = 0,
        BYPASS = 1,
        CLOSE = 2,
    };
    void init();
    static Buttons& instance();
    Button& get(ButtonId id);
private:
    Buttons();
    Buttons(const Buttons&) = delete;
    Buttons& operator=(const Buttons&) = delete;
    std::array<Button, 3> buttons;
};