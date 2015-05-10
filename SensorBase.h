#ifndef _SENSORBASE_h
#define _SENSORBASE_h

#include "Arduino.h"

class SensorBase {
public:
  static byte UpdateCRC(byte res, uint8_t val);
  static byte CalculateCRC(byte *data, byte len);
  static void SetDebugMode(boolean mode);

protected:
  static bool m_debug;

};

#endif


