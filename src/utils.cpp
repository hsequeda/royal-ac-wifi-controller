#include <cstdint>

uint8_t reverse_bits_n(uint8_t val, uint8_t len) {
  uint8_t resp = 0;
  for (uint8_t i = 0; i < len; i++) {
    if ((val & ((1 << i))) == 0) continue;
    resp |= (1 << (len - 1 - i));
  }

  return resp;
}

uint8_t reverse_bits_n_v2(uint8_t val, uint8_t len) {
  uint8_t resp = 0;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t bit = val & ((1 << i));
    resp |= (bit << (len - 1 - i));
  }

  return resp;
}
