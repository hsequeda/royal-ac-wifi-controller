#include "ac_state.h"
#include "utils.h"
#include <cstdint>

// TODO: Check how make it private
const uint8_t BASE_FRAME[14] = {
    0xC4, 0xD3, 0x64, 0x80, 0x00, 0x26, 0xC0,
    0xF0, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00,
};

ACState new_ac_state() {
  return ACState{
      .power = POWER_TO_OFF, // don't modify this field directly
      .temp = 16,      // don't modify this field directly
      .healthy = false,
      .fan_mode = FAN_AUTO, // don't modify this field directly
      .display = false,
      .swing = false,
      .super = false,
  };
}

void set_temperature(ACState *ac_state, uint8_t new_temp) {
  ac_state->temp = new_temp;
  ac_state->super = false;
  ac_state->power = POWER_ON;
}

void set_fan_mode(ACState *ac_state, FanMode new_fan_mode) {
  ac_state->fan_mode = new_fan_mode;
  ac_state->super = false;
  ac_state->power = POWER_ON;
}

void set_super(ACState *ac_state, bool active) {
  ac_state->super = active;
  if (ac_state->super) {
    ac_state->temp = 16;
    ac_state->fan_mode = FAN_AUTO;
  }
  ac_state->power = POWER_ON;
}

void set_swing(ACState *ac_state, bool active) {
  ac_state->swing = active;
  ac_state->power = POWER_ON;
}

void set_display(ACState *ac_state, bool active) {
  ac_state->display = active;
  ac_state->power = POWER_ON;
}

void set_healthy(ACState *ac_state, bool active){
  ac_state->healthy = active;
  ac_state->power = POWER_ON;
}

void set_power(ACState *ac_state, PowerMode pm){
  ac_state->power = pm;
  if (ac_state->power != POWER_ON) {
    ac_state-> super = false;
  }
}

bool handle_power(ACState *ac_state, uint8_t *frame) {
  switch (ac_state->power) {
  case POWER_TO_ON:
    frame[5] |= (1 << 5);
    frame[8] |= (1 << 1);
    return true;
  case POWER_TO_OFF:
    frame[5] &= ~(1 << 5);
    frame[8] |= (1 << 1);
    return true;
  default:
    frame[5] |= (1 << 5);
    frame[8] &= ~(1 << 1);
    return false;
  }
}

bool handle_temperature(ACState *ac_state, uint8_t *frame) {
  uint8_t diff = (31 - ac_state->temp) & 0x0F;
  frame[7] = reverse_bits_n(diff, 4) << 4 | (frame[7] & 0x0F);
  return false;
}

bool handle_healthy(ACState *ac_state, uint8_t *frame) {
  frame[6] &= ~(1 << 3); // disable as default
  // if enable then activate the bit
  if (ac_state->healthy) {
    frame[6] |= (1 << 3);
  }

  return false;
}

bool handle_fan_mode(ACState *ac_state, uint8_t *frame) {
  switch (ac_state->fan_mode) {
  case FAN_LOW:
    frame[8] &= ~(1 << 7); // set first bit to 0
    frame[8] |= (1 << 6);  // set second bit to 1
    frame[8] &= ~(1 << 5); // set third bit to 0
    break;
  case FAN_MIDDLE:
    frame[8] |= (1 << 7);  // set first bit to 1
    frame[8] |= (1 << 6);  // set second bit to 1
    frame[8] &= ~(1 << 5); // set third bit to 0
    break;
  case FAN_HIGH:
    frame[8] |= (1 << 7);  // set first bit to 1
    frame[8] &= ~(1 << 6); // set second bit to 0
    frame[8] |= (1 << 5);  // set third bit to 1
    break;
  default:
    frame[8] &= ~(1 << 7); // set first bit to 0
    frame[8] &= ~(1 << 6); // set first bit to 0
    frame[8] &= ~(1 << 5); // set third bit to 0
    break;
  }

  return false;
}

bool handle_display(ACState *ac_state, uint8_t *frame) {
  // set off by default
  frame[5] |= (1 << 1);
  if (ac_state->display) {
    frame[5] &= ~(1 << 1);
  }

  return false;
}

bool handle_swing(ACState *ac_state, uint8_t *frame) {
  frame[8] &= ~(1 << 2); // set 4th bit to 0
  frame[8] &= ~(1 << 3); // set 5th bit to 0
  frame[8] &= ~(1 << 4); // set 6th bit to 0
  if (ac_state->swing) {
    frame[8] |= (1 << 2); // set 4th bit to 1
    frame[8] |= (1 << 3); // set 5th bit to 1
    frame[8] |= (1 << 4); // set 6th bit to 1
  }

  return false;
}

bool handle_super(ACState *ac_state, uint8_t *frame) {
  frame[6] &= ~(1 << 1); // disable as default
  if (ac_state->super) {
    frame[6] |= (1 << 1); // enable super bit
    return true;
  }

  return false;
}

void build_checksum(uint8_t *frame) {
  int sum = 0;
  for (int i = 0; i <= 12; i++) {
    sum += reverse_bits_n(frame[i], 8);
  }

  frame[13] = reverse_bits_n(sum &= 0xFF, 8);
  return;
}

uint8_t *build_frame(ACState *ac_state) {
  uint8_t *arr = new uint8_t[14];
  for (uint8_t i = 0; i < 14; i++) {
    arr[i] = BASE_FRAME[i];
  }

  if (handle_temperature(ac_state, arr)) {
    build_checksum(arr);
    return arr;
  }
  if (handle_healthy(ac_state, arr)) {
    build_checksum(arr);
    return arr;
  }
  if (handle_fan_mode(ac_state, arr)) {
    build_checksum(arr);
    return arr;
  }
  if (handle_display(ac_state, arr)) {
    build_checksum(arr);
    return arr;
  }
  if (handle_swing(ac_state, arr)) {
    build_checksum(arr);
    return arr;
  }
  if (handle_super(ac_state, arr)) {
    build_checksum(arr);
    return arr;
  }
  if (handle_power(ac_state, arr)) {
    build_checksum(arr);
    return arr;
  }

  build_checksum(arr);
  return arr;
}
