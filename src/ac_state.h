#include <cstdint>

enum PowerMode {
  POWER_TO_ON,
  POWER_TO_OFF,
  POWER_ON,
};

enum FanMode {
  FAN_AUTO,
  FAN_LOW,
  FAN_MIDDLE,
  FAN_HIGH,
};

struct ACState {
  PowerMode power;
  uint8_t temp;
  bool healthy;
  FanMode fan_mode;
  bool display;
  bool swing;
  bool super;
};

ACState new_ac_state();

void set_temperature(ACState *ac_state, uint8_t new_temp);
void set_fan_mode(ACState *ac_state, FanMode new_fan_mode);
void set_super(ACState *ac_state, bool active);
void set_swing(ACState *ac_state, bool active);
void set_display(ACState *ac_state, bool active);
void set_healthy(ACState *ac_state, bool active);
void set_power(ACState *ac_state, PowerMode pm);

uint8_t *build_frame(ACState *ac_state);
