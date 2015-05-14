#ifndef _LACROSSE_h
#define _LACROSSE_h

#include "Arduino.h"
#include "SensorBase.h"


class LaCrosse : public SensorBase {
public:
  struct Frame {
    byte  Header;
    byte  ID;
    bool  NewBatteryFlag;
    bool  Bit12;
    float Temperature;
    bool  WeakBatteryFlag;
    byte  Humidity;
    byte  CRC;
    bool  IsValid;
  };

  static const byte FRAME_LENGTH = 5;
  static bool USE_OLD_ID_CALCULATION;
  static byte CalculateCRC(byte data[]);
  static void EncodeFrame(struct LaCrosse::Frame *frame, byte bytes[FRAME_LENGTH]);
  static void DecodeFrame(byte *bytes, struct LaCrosse::Frame *frame);
  static void AnalyzeFrame(byte *data, bool fOnlyIfValid = false);
  static bool DisplayFrame(byte *data, struct Frame &frame, bool fOnlyIfValid = true);
  static bool TryHandleData(byte *data, bool fFhemDisplay = true);
  static String GetFhemDataString(struct LaCrosse::Frame *frame);

};


#endif


