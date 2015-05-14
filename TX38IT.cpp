#include "TX38IT.h"

/*
* Technoline TX38-IT 17.241 868.3 MHz
* Message Format:
*
* .- [0] -. .- [1] -. .- [2] -. .- [3] -.
* |       | |       | |       | |       |
* SSDD.DDDD NWTT.TTTT TTTT.CCCC CCCC.____
* |||     | |||          | |       |
* |||     | |||          |  `---------------- CRC
* |||     | |||          |
* |||     | ||`------------ Temperature = (T * 0.1) - 40.0
* |||     | ||
* |||     | |`-------- weak battery
* |||     | |
* |||     | `-------  new battery
* |||     |
* ||`---------- ID
* `---- START = 3
*
*/


byte TX38IT::CalculateCRC(byte data[]) {
  int i, j;
  byte res = 0;

  int len = 3;
  int bits = 20;

  for (j = 0; j < len; j++)
  {
    uint8_t val = data[j];
    for (i = 0; i < 8; i++)
    {
      int bitNum = j * 8 + i;

      if(bitNum < bits)
      {
        uint8_t tmp = (uint8_t)((res ^ val) & 0x80);
        res <<= 1;
        if (0 != tmp) {
          res ^= 0x31;
        }
        val <<= 1;
      }
    }
  }
  return res;
}

void TX38IT::EncodeFrame(struct Frame *frame, byte bytes[4]) {
  for (int i = 0; i < 4; i++) { bytes[i] = 0; }

  // ID
  bytes[0] |= 0x3 << 6;
  bytes[0] |= frame->ID & 0x3F;

  // NewBatteryFlag
  bytes[1] |= frame->NewBatteryFlag << 7;
  bytes[1] |= frame->WeakBatteryFlag << 6;

  // Temperature
  long tempVal = (frame->Temperature + 40.0) * 10.0;

  bytes[1] |= (tempVal >> 4) & 0x3F;
  bytes[2] |= (tempVal << 4) & 0xF0;

  byte crc = CalculateCRC(bytes);

  bytes[2] |= (crc >> 4) & 0x0F;
  bytes[3] |= (crc << 4) & 0xF0;
  bytes[3] |= frame->miscBits & 0x0F;
}


void TX38IT::DecodeFrame(byte *bytes, struct Frame *frame) {
  frame->IsValid = true;

  frame->CRC = ((bytes[2] & 0xf) << 4) | (bytes[3] & 0xf0) >> 4;
  if (frame->CRC != CalculateCRC(bytes)) {
    frame->IsValid = false;
  }

  // * SSDD.DDDD NWTT.TTTT TTTT.CCCC CCCC.____
  frame->ID = (bytes[0] & 0x3F);

  frame->Header = (bytes[0] & 0xC0) >> 6;
  if (frame->Header != 3) {
    frame->IsValid = false;
  }

  frame->NewBatteryFlag  = (bytes[1] & 0x80) >> 7;
  frame->WeakBatteryFlag = (bytes[1] & 0x40) >> 6;

  long tempVal;

  tempVal = ((bytes[1] & 0x3F) << 4) | (bytes[2] & 0xf0) >> 4;

  frame->Temperature = (tempVal * 0.1) - 40.0;

  frame->miscBits = (bytes[3] & 0x0f);

  frame->Humidity = 106;

}


String TX38IT::GetFhemDataString(struct Frame *frame) {
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
  pBuf += "OK 9 ";
  pBuf += frame->ID;
  pBuf += ' ';

  // bogus check humidity + eval 2 channel TX25IT
  // TBD .. Dont understand the magic here!?
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
  if (frame->WeakBatteryFlag) {
    hum |= 0x80;
  }
  pBuf += hum;

  return pBuf;
}

bool TX38IT::DisplayFrame(byte *data, struct TX38IT::Frame &frame, bool fOnlyIfValid) {
  byte filter[5];
  filter[0] = 0;
  filter[1] = 0;
  filter[2] = 0;
  filter[3] = 0;
  filter[4] = 0;

  bool hideIt = false;
  for (int f = 0; f < 5; f++) {
    if (frame.ID == filter[f]) {
      hideIt = true;
      break;
    }
  }

  if (!hideIt && fOnlyIfValid && !frame.IsValid) {
	  hideIt = true;
  }

  if (!hideIt) {
    // MilliSeconds, raw data and crc ok
    static unsigned long lastMillis;
    SensorBase::DisplayFrame(lastMillis, "TX38IT", frame.IsValid, data, FRAME_LENGTH);

    if (frame.IsValid) {
      // Start
      Serial.print(" S:");
      Serial.print(frame.Header, DEC);

      // Sensor ID
      Serial.print(" ID:");
      Serial.print(frame.ID, DEC);

      // New battery flag
      Serial.print(" NewBatt:");
      Serial.print(frame.NewBatteryFlag, DEC);

      // Weak battery flag
      Serial.print(" WeakBatt:");
      Serial.print(frame.WeakBatteryFlag, DEC);

      // Temperature
      Serial.print(" Temp:");
      Serial.print(frame.Temperature);

      // CRC
      Serial.print(" CRC:");
      Serial.print(frame.CRC, DEC);
    }

    Serial.println();
  }

}

void TX38IT::AnalyzeFrame(byte *data, bool fOnlyIfValid) {
  struct Frame frame;
  DecodeFrame(data, &frame);
  DisplayFrame(data, frame, fOnlyIfValid);
}

bool TX38IT::TryHandleData(byte *data, bool fFhemDisplay) {
  if ((data[0] & 0xC0) == 0xC0) {
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
		     return DisplayFrame(data, frame);
	     }
    }
  }
  return false;
}