#include "WH1080.h"
#include <Time.h>
/// FSK weather station receiver
/// Receive packets echoes to serial.
/// Updates DCF77 time.
/// Supports Alecto WS3000, WS4000, Fine Offset WH1080 and similar 868MHz stations
/// National Geographic 265 requires adaptation of frequency to 915MHz band.
/// input handler and send functionality in code, but not implemented or used.
/// @see http://jeelabs.org/2010/12/11/rf12-acknowledgements/
// 2013-03-03<info@sevenwatt.com> http://opensource.org/licenses/mit-license.php

/*
 * Message Format: http://www.sevenwatt.com/main/wh1080-protocol-v2-fsk/
 *
 * Package definition:
 * [
 * preample 3 bytes 0xAA    synchron word    payload 10 bytes  postample 11bits zero
 * 0xAA    0xAA    0xAA     0x2D    0xD4     nnnnn---nnnnnnnnn 0x00     0x0
 * 101010101010101010101010 0010110111010100 101.............. 00000000 000
 * ]
 * repeated six times (identical packages) per transmission every 48 seconds
 * There is no or hardly any spacing between the packages.
 * Spacing: to be confirmed.
 *
 * Payload definition:
 * Weather sensor reading Message Format:
 * AAAABBBBBBBBCCCCCCCCCCCCDDDDDDDDEEEEEEEEFFFFFFFFGGGGHHHHHHHHHHHHIIIIJJJJKKKKKKKK
 * 0xA4    0xF0    0x27    0x47    0x00    0x00    0x03    0xC6    0x0C    0xFE
 * 10100100111100000010011101000111000000000000000000000011110001100000110011111110
 *
 * with:
 * AAAA = 1010    Message type: 0xA: sensor readings
 * BBBBBBBB       Station ID / rolling code: Changes with battery insertion.
 * CCCCCCCCCCCC   Temperature*10 in celsius. Binary format MSB is sign
 * DDDDDDDD       Humidity in %. Binary format 0-100. MSB (bit 7) unused.
 * EEEEEEEE       Windspeed
 * FFFFFFFF       Wind gust
 * GGGG           Unknown
 * HHHHHHHHHHHH   Rainfall cumulative. Binary format, max = 0x3FF,
 * IIII           Status bits: MSB b3=low batt indicator.
 * JJJJ           Wind direction
 * KKKKKKKK       CRC8 - reverse Dallas One-wire CRC
 *
 * DCF Time Message Format:
 * AAAABBBBBBBBCCCCDDEEEEEEFFFFFFFFGGGGGGGGHHHHHHHHIIIJJJJJKKKKKKKKLMMMMMMMNNNNNNNN
 * Hours Minutes Seconds Year       MonthDay      ?      Checksum
 * 0xB4    0xFA    0x59    0x06    0x42    0x13    0x43    0x02    0x45    0x74
 *
 * with:
 * AAAA = 1011    Message type: 0xB: DCF77 time stamp
 * BBBBBBBB       Station ID / rolling code: Changes with battery insertion.
 * CCCC           Unknown
 * DD             Unknown
 * EEEEEE         Hours, BCD
 * FFFFFFFF       Minutes, BCD
 * GGGGGGGG       Seconds, BCD
 * HHHHHHHH       Year, last two digits, BCD
 * III            Unknown
 * JJJJJ          Month number, BCD
 * KKKKKKKK       Day in month, BCD
 * L              Unknown status bit
 * MMMMMMM        Unknown
 * NNNNNNNN       CRC8 - reverse Dallas One-wire CRC
 * The DCF code is transmitted five times with 48 second intervals between 3-6 minutes past a new hour. The sensor data transmission stops in the 59th minute. Then there are no transmissions for three minutes, apparently to be noise free to acquire the DCF77 signal. On similar OOK weather stations the DCF77 signal is only transmitted every two hours.
 */

byte WH1080::CalculateCRC(byte data[]) {
  return SensorBase::CalculateCRC(data, FRAME_LENGTH - 1);
}


void WH1080::DecodeFrame(byte *bytes, struct Frame *frame, bool isWS4000) {
	byte *sbuf = bytes;
  frame->IsValid = true;

  frame->CRC = bytes[9];
  if (frame->CRC != CalculateCRC(bytes)) {
    frame->IsValid = false;
  }

    static char *compass[] = {"N  ", "NNE", "NE ", "ENE", "E  ", "ESE", "SE ", "SSE", "S  ", "SSW", "SW ", "WSW", "W  ", "WNW", "NW ", "NNW"};
    uint8_t windbearing = 0;
    byte status= 0;
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
    byte unknown = (sbuf[6] & 0xF0) >> 4;
    //rainfall
    double rain = (((sbuf[6] & 0x0F) << 8) | sbuf[7]) * 0.3;
    if (isWS4000) {
      status = (sbuf[8] & 0xF0) >> 4;
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
  frame->Unknown = unknown;
  frame->Rain = rain;
  frame->Status = status;
  frame->WindBearing = compass[windbearing];
}

void printDigits(int digits){
  // utility function for digital clock display: leading 0
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

int BCD2bin(uint8_t BCD) {
  return (10 * (BCD >> 4 & 0xF) + (BCD & 0xF));
}

void printDouble( double val, byte precision=1){
  // formats val with number of decimal places determine by precision
  // precision is a number from 0 to 6 indicating the desired decimial places
 char ascii[10];
 uint8_t ascii_len = sizeof(ascii);
  snprintf(ascii,ascii_len,"%d",int(val));
  if( precision > 0) {
    strcat(ascii,".");
    unsigned long frac;
    unsigned long mult = 1;
    byte padding = precision -1;
    while(precision--)
       mult *=10;

    if(val >= 0)
      frac = (val - int(val)) * mult;
    else
      frac = (int(val)- val ) * mult;
    unsigned long frac1 = frac;
    while( frac1 /= 10 )
      padding--;
    while(  padding--)
      strcat(ascii,"0");
    char str[7];
    snprintf(str,sizeof(str),"%d",frac);
    strcat(ascii,str);
  }
  Serial.print(ascii);
}

void timestamp(bool fLong=false)
{
	if (fLong) {
	  Serial.print(year());
	  Serial.print("-");
	  printDigits(month());
	  Serial.print("-");
	  printDigits(day());
	  Serial.print(" ");
  }
  printDigits(hour());
  Serial.print(":");
  printDigits(minute());
  Serial.print(":");
  printDigits(second());
  Serial.print(" ");
}

void update_time(uint8_t* tbuf) {
  static unsigned long lastMillis;
  SensorBase::DisplayFrame(lastMillis, "WH1080Time", true, tbuf, WH1080::FRAME_LENGTH);
  Serial.print(' ');
  setTime(BCD2bin(tbuf[2] & 0x3F),BCD2bin(tbuf[3]),BCD2bin(tbuf[4]),BCD2bin(tbuf[7]),BCD2bin(tbuf[6] & 0x1F),BCD2bin(tbuf[5]));
  timestamp(true);
  Serial.println();
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

      // Temperature
      Serial.print(" Temp:");
      printDouble(frame->Temperature);

      // Humidity
      Serial.print(" Hum:");
      Serial.print(frame->Humidity, DEC);

      Serial.print(" WindSpeed:");
      printDouble(frame->WindSpeed);

      Serial.print(" WindGust:");
      printDouble(frame->WindGust);

      Serial.print(" Unknown:");
      Serial.print(frame->Unknown, HEX);

      Serial.print(" Rain:");
      printDouble(frame->Rain);

      Serial.print(" Status:");
      Serial.print(frame->Status, HEX);

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
  bool fWs4000;
  bool fTimePacket;
  byte startNibble = (data[0] & 0xF0)>>4;

	switch(startNibble) {
	case 0x5: // WS3000 weather
		fWs4000 = false;
		fTimePacket = false;
		break;
	case 0x6: // WS3000 time
		fWs4000 = false;
		fTimePacket = true;
		break;
	case 0xA: //WS4000 WH1080 weather
		fWs4000 = true;
		fTimePacket = false;
		break;
	case 0xB: //WS4000 WH1080 time
		fWs4000 = true;
		fTimePacket = true;
		break;
	default:
		return false;
	}

  if (!fTimePacket) {
    struct Frame frame;
    DecodeFrame(data, &frame, fWs4000);
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
  else if (CalculateCRC(data) == data[9]) {
	  update_time(data);
	  return true;
  }
  return false;
}

