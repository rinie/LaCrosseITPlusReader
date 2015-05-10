#include "WT440XH.h"


void WT440XH::DecodeFrame(byte *bytes, struct LaCrosse::Frame *frame) {

  // * H = House code (0 - 15)
  // * D = Device code (1 - 4)  if sensor is configured to 4 we receive 0
  // * T = Temperature vor dem Komma
  // * t = Temperature nach dem Komma
  // * H = Humidity
  // * b = Low battery indication
  // * C = Checksum. bit 1 is XOR of odd bits, bit 0 XOR of even bits in message
  // _Byte 0_ _Byte 1_ _Byte 2_ _Byte 3_ _Byte 4_ _Byte 5_
  // 76543210 76543210 76543210 76543210 76543210 76543210
  // Kennung  bbDDHHHH TTTTTTTT tttttttt HHHHHHHH CCCCCCCC
  // 01010001.01001011.01001100.00001001.00010111.11111000 HEX 51 4B 4C 9 17 F8 houseCode:11 deviceCode:4 Temp C:26,9 humidity:23 Batt:1
  // 01010001.01010001.01001100.00000001.01000000.11010001 HEX 51 51 4C 1 40 D1 houseCode :1 deviceCode:1 Temp C:26,1 humidity:64 Batt:1
  // 01010001.00010100.01001110.00000000.00100101.00101000 HEX 51 14 4E 0 25 28 houseCode :4 deviceCode:1 Temp C:28,0 humidity:37 Batt:0
  // 01010001.01001111.01001100.00001001.00011101.11101110 HEX 51 4F 4C 9 1D EE houseCode:15 deviceCode:4 Temp C:26,9 humidity:29 Batt:1

  frame->IsValid = true;
  frame->Header = bytes[0];
  frame->CRC = bytes[5];

  frame->IsValid = CrcIsValid(bytes);

  byte houseCode = (bytes[1] & 0xF);
  byte deviceCode = (bytes[1] & 0b00110000) >> 4;
  if (deviceCode == 0) {
    deviceCode = 4;
  }

  frame->ID = (deviceCode << 4) | houseCode;

  frame->Temperature = ((bytes[2] - 50) * 10.0 + bytes[3]) / 10.0;
  frame->Humidity = bytes[4];
  frame->WeakBatteryFlag = (bytes[1]) >> 6;

}


bool WT440XH::TryHandleData(byte *data) {
  String fhemString = "";

  if (data[0] == 0x51) {
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

bool WT440XH::CrcIsValid(byte *data) {
  byte crc = 0;
  for (int i = 0; i < FRAME_LENGTH; i++) {
    crc += data[i];
  }

  return crc == 0;
}


