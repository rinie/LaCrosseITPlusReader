#include "SensorBase.h"

bool SensorBase::m_debug = false;

byte SensorBase::CalculateCRC(byte *data, byte len) {
  int i, j;
  byte res = 0;
  for (j = 0; j < len; j++) {
    uint8_t val = data[j];
    for (i = 0; i < 8; i++) {
      uint8_t tmp = (uint8_t)((res ^ val) & 0x80);
      res <<= 1;
      if (0 != tmp) {
        res ^= 0x31;
      }
      val <<= 1;
    }
  }
  return res;
}

void SensorBase::SetDebugMode(boolean mode) {
  m_debug = mode;
}

