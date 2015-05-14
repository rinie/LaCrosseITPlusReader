#ifndef _LEVELSENDERLIB_h
#define _LEVELSENDERLIB_h

#include "Arduino.h"
#include "SensorBase.h"

class LevelSenderLib : public SensorBase {
public:
  struct Frame {
    byte  Header;
    byte  ID;
    float Level;
    float Temperature;
    float Voltage;
    byte  CRC;
    bool  IsValid;
  };
  static byte CalculateCRC(byte *data);
  static const byte FRAME_LENGTH = 6;
  static void EncodeFrame(struct Frame *frame, byte *bytes);
  static void DecodeFrame(byte *data, struct Frame *frame);
  static void AnalyzeFrame(byte *data, bool fOnlyIfValid = false);
  static bool DisplayFrame(byte *data, struct Frame &frame, bool fOnlyIfValid = true);
  static bool TryHandleData(byte *data, bool fFhemDisplay = true);
  static String GetFhemDataString(struct LevelSenderLib::Frame *frame);


};

#endif


