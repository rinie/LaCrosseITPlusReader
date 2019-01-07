#include "WS1600.h"
/*
http://www.g-romahn.de/ws1600/Datepakete_raw.txt

Data - organized in nibbles - are structured as follows (exammple with blanks added for clarity):

 a 5a 5 0 628 1 033 2 000 3 e00 4 000 bd

 data always start with "a"
 from next 1.5 nibbles (here 5a) the 6 msb are identifier of transmitter,
 bit 1 indicates acquisition/synchronizing phase (so 5a >> 58 thereafter)
 bit 0 will be 1 in case of error (e.g. no wind sensor 5a >> 5b)
 next nibble (here 5) is count of quartets to betransmitted
 up to 5 quartets of data follow
 each quartet starts with type indicator (here 0,1,2,3,4)
 0: temperature, 3 nibbles bcd coded tenth of °c plus 400 (here 628-400 = 22.8°C)
 1: humidity, 3 nibbles bcd coded (here 33 %rH), meaning of 1st nibble still unclear
 2: rain, 3 nibbles, counter of contact closures
 3: wind, first nibble direction of wind vane (multiply by 22.5 to obtain degrees,
    here 0xe*22.5 = 315 degrees)
	next two nibbles wind speed in m per sec (i.e. no more than 255 m/s; 9th bit still not found)
 4: gust, speed in m per sec (yes, TX23 sensor does measure gusts and data are transmitted
    but not displayed by WS1600), number of significant nibbles still unclear
 next two bytes (here bd) are crc.
 During acquisition/synchronizing phase (abt. 5 hours) all 5 quartets are sent, see examplke above. Thereafter
 data strings contain only a few ( 1 up ton 3) quartets, so data strings are not! always of
 equal length.

After powering on, the complete set of data will be transmitted every 4.5 secs for 5 hours during acquisition phase.
Lateron only selected sets of data will be transmitted.

Stream of received data follows:

1st line: raw data in hex format as received from sensors
2nd line: meteorological data from outdoor sensors decoded, Same values
	  are displayed on basestation (last duet is "calculated crc" - always 00).

a1250444109120003808401b89 00
Temp  044 Humi 91 Rain 000 Wind 028  Dir 180 Gust 097  ( 4.4 °C, 91 %rH, no rain, wind 2.8 km/h from south, gust 9.7 km/h)
*/

byte WS1600::CalculateCRC(byte data[], byte frameLength) {
  return SensorBase::CalculateCRC(data, frameLength - 1);
}

byte WS1600::DecodeFrame(byte *bytes, struct WS1600::Frame *frame) {
	byte *sbuf = bytes;
    uint8_t dataSets = sbuf[1] & 0xF;
  frame->IsValid = true;
  frame->Header = (bytes[0] & 0xF0) >> 4;
  frame->frameLength = dataSets * 2 + 2 + 1;

  frame->CRC = bytes[frame->frameLength-1];
  if (frame->CRC != CalculateCRC(bytes, frame->frameLength)) {
    frame->IsValid = false;
  }
  if (frame->Header != 0xA) {
    frame->IsValid = false;
  }
  if (!frame->IsValid) {
	  return 0;
  }
    static char *compass[] = {"N  ", "NNE", "NE ", "ENE", "E  ", "ESE", "SE ", "SSE", "S  ", "SSW", "SW ", "WSW", "W  ", "WNW", "NW ", "NNW"};
    static char *sensors[] = {"Temp", "Hum ", "Rain", "Wind", "Gust"};
    uint8_t windbearing = 0;
    // station id
    uint8_t stationid = ((sbuf[0] & 0x0F) << 6) | ((sbuf[1] & 0xC0) >>6);
    int8_t temp = 0;
    int8_t tempDeci = 0;
    //humidity
    uint8_t humidity = 0;
    //wind speed
    uint8_t windspeed = 0;
    //wind gust
    uint8_t windgust = 0;
    //rainfall
    uint16_t rain = 0;

    for (byte i = 0; i < dataSets; i++) {
		byte j = 2 + i*2;
		byte sensorType = (sbuf[j] & 0xF0) >> 4;
		switch (sensorType) { // e.g. a 5a 5 0 628 1 033 2 000 3 e00 4 000 bd
			case 0:	//  0: temperature, 3 nibbles bcd coded tenth of °c plus 400 (here 628-400 = 22.8°C)
				temp = BCD2bin(sbuf[j] & 0x0F) * 10 + BCD2bin((sbuf[j + 1] & 0xF0)>>4);
				temp = temp;
				tempDeci = BCD2bin((sbuf[j + 1] & 0x0F));
				frame->SensorType[i] = sensors[sensorType];
			    frame->Temperature = ((temp * 10 + tempDeci) - 400) / 10;
				break;
			case 1: // 1: humidity, 3 nibbles bcd coded (here 33 %rH), meaning of 1st nibble still unclear
				humidity = BCD2bin(sbuf[j + 1]);
				frame->SensorType[i] = sensors[sensorType];
			    frame->Humidity = humidity;
				break;
			case 2: // 2: rain, 3 nibbles, counter of contact closures
				rain = ((sbuf[j] & 0x0F) + (sbuf[j + 1]) * 100);
				frame->SensorType[i] = sensors[sensorType];
				frame->Rain = rain;
				break;
			case 3:	//3: wind, first nibble direction of wind vane (multiply by 22.5 to obtain degrees,
    				// here 0xe*22.5 = 315 degrees)
					// next two nibbles wind speed in m per sec (i.e. no more than 255 m/s; 9th bit still not found)
				windbearing = (sbuf[j] & 0x0F);
				windspeed = (sbuf[j + 1]);
				frame->SensorType[i] = sensors[sensorType];
			    frame->WindSpeed = windspeed;
			    frame->WindBearing = compass[windbearing];
				break;
			case 4: // 4: gust, speed in m per sec (yes, TX23 sensor does measure gusts and data are transmitted
    				// but not displayed by WS1600), number of significant nibbles still unclear
				windgust = ((sbuf[j] & 0x0F) * 256 + (sbuf[j + 1]));
				frame->SensorType[i] = sensors[sensorType];
			    frame->WindGust = windgust;
				break;
			default:
				frame->SensorType[i] = "Unkown";
				break;
		}

	}
  frame->ID = stationid;
  frame->DataSets = dataSets;
  return frame->frameLength;
}

String WS1600::GetFhemDataString(struct Frame *frame) {
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
  pBuf += "OK WS1600 ";
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

byte WS1600::DisplayFrame(byte *data, struct Frame *frame, bool fOnlyIfValid) {
  bool hideIt = false;

  if ((!hideIt) && fOnlyIfValid && (!frame->IsValid)) {
	  hideIt = true;
  }

  if (!hideIt) {
    // MilliSeconds and the raw data bytes
    static unsigned long lastMillis;
    SensorBase::DisplayFrame(lastMillis, "WS1600", frame->IsValid, data, frame->frameLength);

    if (frame->IsValid) {
      // Start
      Serial.print(" S:");
      Serial.print(frame->Header, HEX);


      // Sensor ID
      Serial.print(" ID:");
      Serial.print(frame->ID, HEX);

		// datasets
	  Serial.print(" Datasets:");
      Serial.print(frame->DataSets, DEC);

      // Temperature
      Serial.print(" Temp:");
      printDouble(frame->Temperature);

      // Humidity
      Serial.print(" Hum:");
      Serial.print(frame->Humidity, DEC);

      Serial.print(" WindSpeed:");
      if (frame->WindSpeed < 254.0) {
      	printDouble(frame->WindSpeed);
	  }
	  else {
		  Serial.print("-1");
	  }

      Serial.print(" WindGust:");
      printDouble(frame->WindGust);

      Serial.print(" Rain:");
      printDouble(frame->Rain);

      Serial.print(" WindBearing:");
      Serial.print(frame->WindBearing);

	  Serial.print(" Sensors:[");
		for (byte i = 0; i < frame->DataSets; i++) {
			if (i > 0) {
				Serial.print(", ");
			}
		  Serial.print(frame->SensorType[i]);
		}
      // CRC
      Serial.print("] CRC:");
      Serial.print(frame->CRC, HEX);
    }

    Serial.println();
  }
  return (!hideIt) ? frame->frameLength : 0;
}

void WS1600::AnalyzeFrame(byte *data, bool fOnlyIfValid) {
  struct WS1600::Frame frame;
  byte frameLength;
  DecodeFrame(data, &frame);
  frameLength = DisplayFrame(data, &frame, fOnlyIfValid);
}

byte WS1600::TryHandleData(byte *data, bool fFhemDisplay) {
    static struct WS1600::Frame frame;
    DecodeFrame(data, &frame);
    if (frame.IsValid) {
	  if (fFhemDisplay) {
          String fhemString = "";
          fhemString = GetFhemDataString(&frame);
          if (fhemString.length() > 0) {
            Serial.println(fhemString);
          }
          return (fhemString.length() > 0) ? frame.frameLength : 0;
  	     }
  	     else {
		     return DisplayFrame(data, &frame);
	     }
    }
  return 0;
}

