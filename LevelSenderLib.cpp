#include "LevelSenderLib.h"

// Message-Format
// --------------
//  .- [0] -.  .- [1] -.  .- [2] -.  .- [3] -.  .- [4] -.  .- [5] -.
//  |       |  |       |  |       |  |       |  |       |  |       |
//  SSSS.DDDD  LLLL.LLLL  LLLL.TTTT  TTTT.TTTT  VVVV.VVVV  CCCC.CCCC
//  |  | |  |  |  | |  |  |  | |  |  |  | |  |  |  | |  |  |       |
//  |  | |  |  |  | |  |  |  | |  |  |  | |  |  |  | |  |  `--------- CRC
//  |  | |  |  |  | |  |  |  | |  |  |  | |  |  |  | |  |
//  |  | |  |  |  | |  |  |  | |  |  |  | |  |  |  | `----- Voltage * 0.1V
//  |  | |  |  |  | |  |  |  | |  |  |  | |  |  `---------- Voltage * 1V 
//  |  | |  |  |  | |  |  |  | |  |  |  | |  |
//  |  | |  |  |  | |  |  |  | |  |  |  | `----- Temperature T *  0.1  \
//  |  | |  |  |  | |  |  |  | |  |  `---------- Temperature T *  1    |  +40 °C
//  |  | |  |  |  | |  |  |  | `---------------- Temperature T * 10    /
//  |  | |  |  |  | |  |  |  | 
//  |  | |  |  |  | |  |  `----- Level *   1  \
//  |  | |  |  |  | `----------- Level *  10  |  0,5cm steps
//  |  | |  |  `---------------- Level * 100  /
//  |  | |  |
//  |  | `----- ID (0 .. 15)
//  |  | 
//  `----- START (IT+=9, LevelSender=11)



void LevelSenderLib::DecodeFrame(byte *data, struct Frame *frame){
  // SSSS.DDDD  LLLL.LLLL  LLLL.TTTT  TTTT.TTTT  VVVV.VVVV  CCCC.CCCC

  frame->IsValid = true;

  frame->CRC = data[5];
  if (frame->CRC != CalculateCRC(data)) {
    if (m_debug) { Serial.println("## CRC FAIL ##"); }
    frame->IsValid = false;
  }

  frame->ID = (data[0] & 0x0F);

  frame->Header = (data[0] & 0xF0) >> 4;
  if (frame->Header != 11) {
    if (m_debug) { Serial.println("No valid start"); }
    frame->IsValid = false;
  }

  frame->Level = ((data[1] & 0xF0) >> 4) * 100;
  frame->Level += (data[1] & 0x0F) * 10;
  frame->Level += ((data[2] & 0xF0) >> 4);
  frame->Level *= 0.5;

  frame->Temperature = (data[2] & 0xF) * 10;
  frame->Temperature += ((data[3] & 0xF0) >> 4);
  frame->Temperature += (data[3] & 0xF) * 0.1;
  frame->Temperature -= 40;

  frame->Voltage = (data[4] & 0xF0) >> 4;
  frame->Voltage += (data[4] & 0x0F) * 0.1;


  // Check if the data can be valid
  if (frame->Temperature < -40.0 || frame->Temperature > 60.0) {
    frame->IsValid = false;
    if (m_debug) {
      Serial.print("No valid Temperature: ");
      Serial.println(frame->Temperature);
    }
  }
  if (frame->Level < 2.0 || frame->Level > 300) {
    frame->IsValid = false;
    if (m_debug) {
      Serial.print("No valid Level: ");
      Serial.println(frame->Level);
    }
  }
  if (frame->Voltage < 2.0 || frame->Voltage > 13.0) {
    frame->IsValid = false;
    if (m_debug) {
      Serial.print("No valid Voltage: ");
      Serial.println(frame->Voltage);
    }
  }
}
  
void LevelSenderLib::EncodeFrame(struct Frame *frame, byte *bytes) {
  // SSSS.DDDD  LLLL.LLLL  LLLL.TTTT  TTTT.TTTT  VVVV.VVVV  CCCC.CCCC
  for (int i = 0; i < FRAME_LENGTH; i++) { bytes[i] = 0; }

  // Start and ID
  bytes[0] |= frame->Header << 4;
  bytes[0] |= frame->ID;

  // Level
  float levelSteps = frame->Level / 0.5;
  bytes[1] |= ((int)(levelSteps / 100)) << 4;
  bytes[1] |= ((int)levelSteps % 100) / 10;
  bytes[2] |= ((int)fmod(((int)levelSteps % 100) % 10, 10)) << 4;

  // Temperature
  float temp = frame->Temperature + 40.0;
  bytes[2] |= (int)(temp / 10);
  bytes[3] |= ((int)temp % 10) << 4;
  bytes[3] |= (int)(fmod(temp, 1) * 10 + 0.5);

  // Voltage
  bytes[4] |= ((int)frame->Voltage % 10) << 4;
  bytes[4] |= (int)(fmod(frame->Voltage, 1) * 10 + 0.5);

  // CRC
  bytes[FRAME_LENGTH - 1] = CalculateCRC(bytes);
}

void LevelSenderLib::AnalyzeFrame(byte *data) {
  struct Frame frame;
  DecodeFrame(data, &frame);

  // MilliSeconds
  static unsigned long lastMillis;
  unsigned long now = millis();
  char div[16];
  sprintf(div, "%06d ", now - lastMillis);
  lastMillis = millis();
  Serial.print(div);

  // Show the raw data bytes
  Serial.print("LevelSender [");
  for (int i = 0; i < FRAME_LENGTH; i++) {
    Serial.print(data[i], HEX);
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

    // Level
    Serial.print(" Level:");
    Serial.print(frame.Level);

    // Temperature
    Serial.print(" Temp:");
    Serial.print(frame.Temperature);

    // Voltage
    Serial.print(" Volt:");
    Serial.print(frame.Voltage);

    // CRC
    Serial.print(" CRC:");
    Serial.print(frame.CRC, DEC);
  }

  Serial.println();

}


byte LevelSenderLib::CalculateCRC(byte data[]) {
  return SensorBase::CalculateCRC(data, FRAME_LENGTH - 1);
}


String LevelSenderLib::GetFhemDataString(struct Frame *frame) {
  // Format
  // 
  // OK LS 1  0   5   100 4   191 60      =  38,0cm    21,5°C   6,0V
  // OK LS 1  0   8   167 4   251 57      = 121,5cm    27,5°C   5,7V   
  // OK LS ID X   XXX XXX XXX XXX XXX
  // |   | |  |    |   |   |   |   |
  // |   | |  |    |   |   |   |   `--- Voltage * 10
  // |   | |  |    |   |   |   `------- Temp. * 10 + 1000 LSB
  // |   | |  |    |   |   `----------- Temp. * 10 + 1000 MSB
  // |   | |  |    |   `--------------- Level * 10 + 1000 MSB
  // |   | |  |    `------------------- Level * 10 + 1000 LSB
  // |   | |  `------------------------ Sensor type fix 0 at the moment
  // |   | `--------------------------- Sensor ID ( 0 .. 15)
  // |   `----------------------------- fix "11"
  // `--------------------------------- fix "LS"

  String result;
  result += "OK LS ";
  result += frame->ID;
  result += " 0 ";

  // Level
  int level = (int)(frame->Level * 10 + 1000);
  result += (byte)(level >> 8);
  result += " ";
  result += (byte)(level);
  result += " ";

  // Temperature
  int temp = (int)(frame->Temperature * 10 + 1000);
  result += (byte)(temp >> 8);
  result += " ";
  result += (byte)(temp);
  result += " ";

  // Voltage
  result += (int)(frame->Voltage * 10);

  return result;
}

bool LevelSenderLib::TryHandleData(byte *data) {
  String fhemString = "";

  struct Frame frame;
  DecodeFrame(data, &frame);
  if (frame.IsValid) {
    fhemString = GetFhemDataString(&frame);
  }

  if (fhemString.length() > 0) {
    Serial.println(fhemString);
  }

  return fhemString.length() > 0;
}