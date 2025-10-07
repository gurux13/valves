#pragma once
#include "valve.h"
struct ZigbeeState {
    struct RawValues {
        Valve::State inlet = Valve::State::INTERMEDIATE;
        Valve::State outlet = Valve::State::INTERMEDIATE;
        Valve::State bypass = Valve::State::INTERMEDIATE;
    };
    RawValues rawValues;
    enum class State {
        UNDEFINED,
        SOFTEN,
        BYPASS,
        FULL_CLOSE,
        ERROR,
        RAW_CONTROL
    }
    state = State::UNDEFINED;
    bool transitioning = false;
};