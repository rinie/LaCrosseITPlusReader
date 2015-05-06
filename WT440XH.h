#ifndef _WT440XH_h
#define _WT440XH_h

#include "Arduino.h"
#include "SensorBase.h"
#include "LaCrosse.h"


class WT440XH : public LaCrosse {
public:
  static const byte FRAME_LENGTH = 6;
  static void DecodeFrame(byte *bytes, struct LaCrosse::Frame *frame);
  static bool TryHandleData(byte *data);
  static bool CrcIsValid(byte *data);
  
};


#endif

