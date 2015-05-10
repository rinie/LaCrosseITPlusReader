#include "LaCrosse.h"

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

bool LaCrosse::USE_OLD_ID_CALCULATION = false;

byte LaCrosse::CalculateCRC(byte data[]) {
  return SensorBase::CalculateCRC(data, FRAME_LENGTH - 1);
}


void LaCrosse::EncodeFrame(struct Frame *frame, byte bytes[5]) {
  for (int i = 0; i < 5; i++) { bytes[i] = 0; }

  // ID
  bytes[0] = 9 << 4;
  bytes[0] |= frame->ID >> 2;
  bytes[1] = (frame->ID & 0b00000011) << 6;

  // NewBatteryFlag
  bytes[1] |= frame->NewBatteryFlag << 5;

  // Bit12
  bytes[1] |= frame->Bit12 << 4;

  // Temperature
  float temp = frame->Temperature + 40.0;
  bytes[1] |= (int)(temp / 10);
  bytes[2] |= ((int)temp % 10) << 4;
  bytes[2] |= (int)(fmod(temp, 1) * 10 + 0.5);

  // Humidity
  bytes[3] = frame->Humidity;

  // WeakBatteryFlag
  bytes[3] |= frame->WeakBatteryFlag << 7;

  // CRC
  bytes[4] = CalculateCRC(bytes);
  
}


void LaCrosse::DecodeFrame(byte *bytes, struct Frame *frame) {
  frame->IsValid = true;

  frame->CRC = bytes[4];
  if (frame->CRC != CalculateCRC(bytes)) {
    frame->IsValid = false;
  }

  // SSSS.DDDD DDN_.TTTT TTTT.TTTT WHHH.HHHH CCCC.CCCC
  frame->ID = 0;
  frame->ID |= (bytes[0] & 0xF) << 2;
  if (USE_OLD_ID_CALCULATION) {
    // This is the way how the initial release calculated th ID
    // It's wrong because the two bits must be moved to the right
    frame->ID |= (bytes[1] & 0xC0);
  }
  else {
    // The new ID calculation. The order of the bits is respected
    frame->ID |= (bytes[1] & 0xC0) >> 6;
  }


  frame->Header = (bytes[0] & 0xF0) >> 4;
  if (frame->Header != 9) {
    frame->IsValid = false;
  }

  frame->NewBatteryFlag = (bytes[1] & 0x20) >> 5;

  frame->Bit12 = (bytes[1] & 0x10) >> 4;

  byte bcd[3];
  bcd[0] = bytes[1] & 0xF;
  bcd[1] = (bytes[2] & 0xF0) >> 4;
  bcd[2] = (bytes[2] & 0xF);
  float t = 0;
  t += bcd[0] * 100.0;
  t += bcd[1] * 10.0;
  t += bcd[2] * 1.0;
  t = t / 10;
  t -= 40;
  frame->Temperature = t;

  frame->WeakBatteryFlag = (bytes[3] & 0x80) >> 7;

  frame->Humidity = bytes[3] & 0b01111111;
}


String LaCrosse::GetFhemDataString(struct Frame *frame) {
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

void LaCrosse::AnalyzeFrame(byte *data) {
  struct Frame frame;
  DecodeFrame(data, &frame);

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

  if (!hideIt) {
    // MilliSeconds
    static unsigned long lastMillis;
    unsigned long now = millis();
    char div[16];
    sprintf(div, "%06d ", now - lastMillis);
    lastMillis = millis();
    Serial.print(div);

    // Show the raw data bytes
    Serial.print("LaCrosse [");
    for (int i = 0; i < FRAME_LENGTH; i++) {
      Serial.print(data[i], DEC);
      Serial.print(" ");
    }
    Serial.print("]");

    // Check CRC
    if (!frame.IsValid) {
      Serial.print(" CRC:WRONG");
    }
    else {
      Serial.print(" CRC:OK");

      // Start
      Serial.print(" S:");
      Serial.print(frame.Header, DEC);

      // Sensor ID
      Serial.print(" ID:");
      Serial.print(frame.ID, DEC);

      // New battery flag
      Serial.print(" NewBatt:");
      Serial.print(frame.NewBatteryFlag, DEC);

      // Bit 12
      Serial.print(" Bit12:");
      Serial.print(frame.Bit12, DEC);

      // Temperature
      Serial.print(" Temp:");
      Serial.print(frame.Temperature);

      // Weak battery flag
      Serial.print(" WeakBatt:");
      Serial.print(frame.WeakBatteryFlag, DEC);

      // Humidity
      Serial.print(" Hum:");
      Serial.print(frame.Humidity, DEC);

      // CRC
      Serial.print(" CRC:");
      Serial.print(frame.CRC, DEC);
    }

    Serial.println();
  }

}

bool LaCrosse::TryHandleData(byte *data) {
  String fhemString = "";

  if ((data[0] & 0xF0) >> 4 == 9) {
    struct Frame frame;
    DecodeFrame(data, &frame);
    if (frame.IsValid) {
      fhemString = GetFhemDataString(&frame);
    }

    if (fhemString.length() > 0) {
      Serial.println(fhemString);
    }
  }

  return fhemString.length() > 0;

}

