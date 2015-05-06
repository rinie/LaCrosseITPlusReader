#ifndef _EMT7110_h
#define _EMT7110_h

#include "Arduino.h"
#include "SensorBase.h"

class EMT7110 : public SensorBase {

public:
  struct Frame {
    byte  Header1;
    byte  Header2;
    word  ID;
    bool ConsumersConnected;
    bool PairingFlag;
    float Voltage;
    float Current;
    float Power;
    float AccumulatedPower;
    bool Byte9_6;
    bool Byte9_7;
    byte  CRC;
    bool  IsValid;
  };

  static bool CrcIsValid(byte *data);
  static const byte FRAME_LENGTH = 12;
  static void DecodeFrame(byte *data, struct EMT7110::Frame *frame);
  static void AnalyzeFrame(byte *data);
  static bool TryHandleData(byte *data);
  static String GetFhemDataString(struct EMT7110::Frame *frame);


};


#endif

