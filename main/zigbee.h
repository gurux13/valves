#pragma once
#include "state.h"
void init_zigbee(bool factory_reset);
void zb_report_state(const ZigbeeState& state);
class Logic;
void zb_set_logic(Logic* logic);