#include "WH1080.h"
/// FSK weather station receiver
/// Receive packets echoes to serial.
/// Updates DCF77 time.
/// Supports Alecto WS3000, WS4000, Fine Offset WH1080 and similar 868MHz stations
/// National Geographic 265 requires adaptation of frequency to 915MHz band.
/// input handler and send functionality in code, but not implemented or used.
/// @see http://jeelabs.org/2010/12/11/rf12-acknowledgements/
// 2013-03-03<info@sevenwatt.com> http://opensource.org/licenses/mit-license.php

/*
* Message Format:
*
* .- [0] -. .- [1] -. .- [2] -. .- [3] -. .- [4] -.
* |       | |       | |       | |       | |       |
* SSSS.DDDD DDN_.TTTT TTTT.TTTT WHHH.HHHH CCCC.CCCC
* |  | |     ||  |  | |  | |  | ||      | |       |
* |  | |     ||  |  | |  | |  | ||      | `--------- CRC
* |  | |     ||  |  | |  | |  | |`-------- Humidity
* |  | |     ||  |  | |  | |  | |
* |  | |     ||  |  | |  | |  | `---- weak battery
* |  | |     ||  |  | |  | |  |
* |  | |     ||  |  | |  | `----- Temperature T * 0.1
* |  | |     ||  |  | |  |
* |  | |     ||  |  | `---------- Temperature T * 1
* |  | |     ||  |  |
* |  | |     ||  `--------------- Temperature T * 10
* |  | |     | `--- new battery
* |  | `---------- ID
* `---- START = 9
*
*/

byte WH1080::CalculateCRC(byte data[]) {
  return SensorBase::CalculateCRC(data, FRAME_LENGTH - 1);
}


void WH1080::EncodeFrame(struct Frame *frame, byte bytes[FRAME_LENGTH]) {
  for (int i = 0; i < FRAME_LENGTH; i++) { bytes[i] = 0; }

  // ID
  bytes[0] = 9 << 4;
  bytes[0] |= frame->ID >> 2;
  bytes[1] = (frame->ID & 0b00000011) << 6;
#if 0
  // NewBatteryFlag
  bytes[1] |= frame->NewBatteryFlag << 5;

  // Bit12
  bytes[1] |= frame->Bit12 << 4;
#endif
  // Temperature
  float temp = frame->Temperature + 40.0;
  bytes[1] |= (int)(temp / 10);
  bytes[2] |= ((int)temp % 10) << 4;
  bytes[2] |= (int)(fmod(temp, 1) * 10 + 0.5);

  // Humidity
  bytes[3] = frame->Humidity;

#if 0
  // WeakBatteryFlag
  bytes[3] |= frame->WeakBatteryFlag << 7;
#endif
  // CRC
  bytes[FRAME_LENGTH-1] = CalculateCRC(bytes);

}


void WH1080::DecodeFrame(byte *bytes, struct Frame *frame) {
	bool isWS4000=true;
	byte *sbuf = bytes;
  frame->IsValid = true;

  frame->CRC = bytes[9];
  if (frame->CRC != CalculateCRC(bytes)) {
    frame->IsValid = false;
  }

  // SSSS.DDDD DDN_.TTTT TTTT.TTTT WHHH.HHHH CCCC.CCCC

    static char *compass[] = {"N  ", "NNE", "NE ", "ENE", "E  ", "ESE", "SE ", "SSE", "S  ", "SSW", "SW ", "WSW", "W  ", "WNW", "NW ", "NNW"};
    uint8_t windbearing = 0;
    // station id
    uint8_t stationid = (sbuf[0] << 4) | (sbuf[1] >>4);
    // temperature
    uint8_t sign = (sbuf[1] >> 3) & 1;
    int16_t temp = ((sbuf[1] & 0x07) << 8) | sbuf[2];
    if (sign)
      temp = (~temp)+sign;
    double temperature = temp * 0.1;
    //humidity
    uint8_t humidity = sbuf[3] & 0x7F;
    //wind speed
    double windspeed = sbuf[4] * 0.34;
    //wind gust
    double windgust = sbuf[5] * 0.34;
    //rainfall
    double rain = (((sbuf[6] & 0x0F) << 8) | sbuf[7]) * 0.3;
    if (isWS4000) {
      //wind bearing
      windbearing = sbuf[8] & 0x0F;
    }

  frame->ID = stationid;

  frame->Header = (bytes[0] & 0xF0) >> 4;
  if (frame->Header != 0x0A) {
    frame->IsValid = false;
  }

  frame->Temperature = temperature;

//  frame->WeakBatteryFlag = (bytes[3] & 0x80) >> 7;

  frame->Humidity = humidity;
  frame->WindSpeed = windspeed;
  frame->WindGust = windgust;
  frame->Rain = rain;
  frame->WindBearing = compass[windbearing];
}


String WH1080::GetFhemDataString(struct Frame *frame) {
  // Format
  //
  // OK 9 56 1   4   156 37     ID = 56  T: 18.0  H: 37  no NewBatt
  // OK 9 49 1   4   182 54     ID = 49  T: 20.6  H: 54  no NewBatt
  // OK 9 55 129 4 192 56       ID = 55  T: 21.6  H: 56  WITH NewBatt
  // OK 9 ID XXX XXX XXX XXX
  // |  | |  |   |   |   |
  // |  | |  |   |   |   --- Humidity incl. WeakBatteryFlag
  // |  | |  |   |   |------ Temp * 10 + 1000 LSB
  // |  | |  |   |---------- Temp * 10 + 1000 MSB
  // |  | |  |-------------- Sensor type (1 or 2) +128 if NewBatteryFlag
  // |  | |----------------- Sensor ID
  // |  |------------------- fix "9"
  // |---------------------- fix "OK"

  String pBuf;
  pBuf += "OK WH1080 ";
  pBuf += frame->ID;
  pBuf += ' ';

  // bogus check humidity + eval 2 channel TX25IT
  // TBD .. Dont understand the magic here!?
#if 0
  if ((frame->Humidity >= 0 && frame->Humidity <= 99)
    || frame->Humidity == 106
    || (frame->Humidity >= 128 && frame->Humidity <= 227)
    || frame->Humidity == 234) {
    pBuf += frame->NewBatteryFlag ? 129 : 1;
    pBuf += ' ';
  }
  else if (frame->Humidity == 125 || frame->Humidity == 253) {
    pBuf += 2 | frame->NewBatteryFlag ? 130 : 2;
    pBuf += ' ';
  }
  else {
    return "";
  }
#endif

  // add temperature
  uint16_t pTemp = (uint16_t)(frame->Temperature * 10 + 1000);
  pBuf += (byte)(pTemp >> 8);
  pBuf += ' ';
  pBuf += (byte)(pTemp);
  pBuf += ' ';

  // bogus check temperature
  if (frame->Temperature >= 60 || frame->Temperature <= -40)
    return "";

  // add humidity
  byte hum = frame->Humidity;
//  if (frame->WeakBatteryFlag) {
//    hum |= 0x80;
//  }
  pBuf += hum;

  return pBuf;
}

bool WH1080::DisplayFrame(byte *data,  byte packetCount, struct Frame *frame, bool fOnlyIfValid) {
  bool hideIt = false;

  if (!hideIt && fOnlyIfValid && !frame->IsValid) {
	  hideIt = true;
  }

  if (!hideIt) {
    // MilliSeconds and the raw data bytes
    static unsigned long lastMillis;
    SensorBase::DisplayFrame(lastMillis, "WH1080", frame->IsValid, data, FRAME_LENGTH);

    if (frame->IsValid) {
      // Repeat/Package count
      Serial.print(" #:");
      Serial.print(packetCount, DEC);

      // Start
      Serial.print(" S:");
      Serial.print(frame->Header, HEX);

      // Sensor ID
      Serial.print(" ID:");
      Serial.print(frame->ID, HEX);
#if 0
      // New battery flag
      Serial.print(" NewBatt:");
      Serial.print(frame->NewBatteryFlag, DEC);

      // Bit 12
      Serial.print(" Bit12:");
      Serial.print(frame->Bit12, DEC);
#endif
      // Temperature
      Serial.print(" Temp:");
      Serial.print(frame->Temperature);

#if 0
      // Weak battery flag
      Serial.print(" WeakBatt:");
      Serial.print(frame->WeakBatteryFlag, DEC);
#endif
      // Humidity
      Serial.print(" Hum:");
      Serial.print(frame->Humidity, DEC);

      Serial.print(" Rain:");
      Serial.print(frame->Rain, DEC);

      Serial.print(" WindSpeed:");
      Serial.print(frame->WindSpeed, DEC);

      Serial.print(" WindGust:");
      Serial.print(frame->WindGust, DEC);

      Serial.print(" WindBearing:");
      Serial.print(frame->WindBearing);

      // CRC
      Serial.print(" CRC:");
      Serial.print(frame->CRC, HEX);
    }

    Serial.println();
  }
  return !hideIt;
}

void WH1080::AnalyzeFrame(byte *data, byte packetCount, bool fOnlyIfValid) {
  struct Frame frame;
  bool fOk;
  DecodeFrame(data, &frame);
  fOk = DisplayFrame(data, packetCount, &frame, fOnlyIfValid);
}

bool WH1080::TryHandleData(byte *data, byte packetCount, bool fFhemDisplay) {

  if ((data[0] & 0xF0) >> 4 == 0x0A) {
    struct Frame frame;
    DecodeFrame(data, &frame);
    if (frame.IsValid) {
	  if (fFhemDisplay) {
          String fhemString = "";
          fhemString = GetFhemDataString(&frame);
          if (fhemString.length() > 0) {
            Serial.println(fhemString);
          }
          return fhemString.length() > 0;
  	     }
  	     else {
		     return DisplayFrame(data, packetCount, &frame);
	     }
    }
  }
  return false;
}

