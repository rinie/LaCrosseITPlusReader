#ifndef _TX38IT_h
#define _TX38IT_h

#include "Arduino.h"
#include "SensorBase.h"


class TX38IT : public SensorBase {
public:
  struct Frame {
    byte  Header;
    byte  ID;
    bool  NewBatteryFlag;
    bool  WeakBatteryFlag;
    float Temperature;
    byte  Humidity;
    byte  CRC;
    byte  miscBits;
    bool  IsValid;
  };

  static const byte FRAME_LENGTH = 4;
  static bool USE_OLD_ID_CALCULATION;
  static byte CalculateCRC(byte data[]);
  static void EncodeFrame(struct TX38IT::Frame *frame, byte bytes[FRAME_LENGTH]);
  static void DecodeFrame(byte *bytes, struct TX38IT::Frame *frame);
  static bool DisplayFrame(byte *data, struct TX38IT::Frame &frame, bool fOnlyIfValid = true);
  static void AnalyzeFrame(byte *data, bool fOnlyIfValid = false);
  static bool TryHandleData(byte *data, bool fFhemDisplay = true);
  static String GetFhemDataString(struct TX38IT::Frame *frame);

};


#endif


