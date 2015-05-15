#ifndef _WS1600_h
#define _WS1600_h

#include "Arduino.h"
#include "SensorBase.h"
#include "WH1080.h"

class WS1600 : public SensorBase {
public:
  struct Frame {
    byte  Header;
    byte  ID;
    byte  DataSets;
    char  *SensorType[5];
    double Temperature;
    byte  Humidity;
    double WindSpeed;
    double WindGust;
    double Rain;
    char *WindBearing;
    byte  CRC;
    bool  IsValid;
    byte frameLength;
  };

  static const byte FRAME_LENGTH = 13;
  static byte CalculateCRC(byte data[], byte frameLength=FRAME_LENGTH);
  static byte DecodeFrame(byte *bytes, struct WS1600::Frame *frame);
  static byte DisplayFrame(byte *data, struct WS1600::Frame *frame, bool fOnlyIfValid = true);
  static void AnalyzeFrame(byte *data, bool fOnlyIfValid = false);
  static byte TryHandleData(byte *data, bool fFhemDisplay = true);
  static String GetFhemDataString(struct WS1600::Frame *frame);

};


#endif


