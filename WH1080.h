#ifndef _WH1080_h
#define _WH1080_h

#include "Arduino.h"
#include "SensorBase.h"

//WH1080 V2 protocol defines
#define MSG_WS4000 1
#define MSG_WS3000 2
#define LEN_WS4000 10
#define LEN_WS3000 9
#define LEN_MAX 10

class WH1080 : public SensorBase {
public:
  struct Frame {
    byte  Header;
    byte  ID;
    double Temperature;
    byte  Humidity;
    double WindSpeed;
    double WindGust;
    byte  Unknown;
    double Rain;
    byte  Status;
    char *WindBearing;
    byte  CRC;
    bool  IsValid;
    byte frameLength;
  };

  static const byte FRAME_LENGTH = LEN_WS4000;
  static byte CalculateCRC(byte data[], byte frameLength=FRAME_LENGTH);
  static byte DecodeFrame(byte *bytes, struct WH1080::Frame *frame);
  static byte DisplayFrame(byte *data, byte packetCount, struct WH1080::Frame *frame, bool fOnlyIfValid = true);
  static void AnalyzeFrame(byte *data, byte packetCount, bool fOnlyIfValid = false);
  static byte TryHandleData(byte *data, byte packetCount, bool fFhemDisplay = true);
  static String GetFhemDataString(struct WH1080::Frame *frame);

};

void printDigits(int digits);
int BCD2bin(uint8_t BCD);
void printDouble( double val, byte precision=1);


#endif


