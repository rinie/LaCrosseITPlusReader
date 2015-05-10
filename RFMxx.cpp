#include "RFMxx.h"
#include "SensorBase.h"
#include "JeeLink.h"
extern JeeLink jeeLink;

void RFMxx::Receive() {
  if (IsRF69) {
    if (ReadReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY) {
      for (int i = 0; i < PAYLOADSIZE; i++) {
        byte bt = GetByteFromFifo();
        m_payload[i] = bt;
      }
      m_payloadReady = true;
    }
  }
  else {
    bool hasData = false;
    digitalWrite(m_ss, LOW);
    asm("nop");
    asm("nop");
    if (digitalRead(m_miso)) {
      hasData = true;
    }
    digitalWrite(m_ss, HIGH);

    if (hasData) {
        byte bt = GetByteFromFifo();
      m_payload[m_payloadPointer++] = bt;
      m_lastReceiveTime = millis();
      m_payload_crc = SensorBase::UpdateCRC(m_payload_crc, bt);
    }

    if ((m_payloadPointer >= 8 && m_payload_crc == 0) || (m_payloadPointer > 0 && millis() > m_lastReceiveTime + 50) || m_payloadPointer >= 32) {
      m_payloadReady = true;
    }
  }
}

byte RFMxx::GetPayload(byte *data) {
	byte payloadPointer = m_payloadPointer;
  m_payloadReady = false;
  m_payloadPointer = 0;
  m_payload_crc = 0;
  for (int i = 0; i < PAYLOADSIZE; i++) {
    data[i] = m_payload[i];
  }
  return payloadPointer;
}

bool RFMxx::ReceiveGetPayloadWhenReady(byte *data, byte &length, byte &packetCount) {
      byte payload[PAYLOADSIZE];
      byte payLoadSize;
//      byte packetCount;
	bool fAgain = false;
	bool fEnableReceiver = true;
	unsigned long lastReceiveTime = 0;
	byte count = 0;
	byte len;
	bool fPayloadIsReady = false;

	do {
		Receive();

		if (PayloadIsReady()) {
			payLoadSize = GetPayload(payload);
			if (!fPayloadIsReady) {
				  for (int i = 0; i < payLoadSize; i++) {
					data[i] = payload[i];
				  }
				length = payLoadSize;
				len = payLoadSize;
				count++;
				fPayloadIsReady = true;
				fAgain = (payLoadSize < 16);
			}
			else if (payLoadSize >= len) { // WH1080 sends 6 repeated packages
				int i;
				  for (i = 0; i < len; i++) {
					if (data[i] != payload[i]) {
						break;
					}
				  }
				  if (i >= len) {
					count++; // matching packet count
					fAgain = true;
				  }
			}
			else {
				fAgain = false;
			}
			packetCount = count;

			if (fAgain) {
				lastReceiveTime = millis();
				fEnableReceiver = true;
				EnableReceiver(fEnableReceiver, false);
			}
	  }
	  else {
		  fAgain = fAgain && (fPayloadIsReady) && (count < 8) && (millis() < lastReceiveTime + 50);
	  }
	} while (fAgain);

	if (fEnableReceiver && fPayloadIsReady) {
		fEnableReceiver = false;
		EnableReceiver(fEnableReceiver);
	}
	return (fPayloadIsReady);
}

void RFMxx::SetDataRate(unsigned long dataRate) {
  m_dataRate = dataRate;

  if (IsRF69) {
    word r = ((32000000UL + (m_dataRate / 2)) / m_dataRate);
    WriteReg(0x03, r >> 8);
    WriteReg(0x04, r & 0xFF);
  }
  else {
    byte bt;
    if (m_dataRate == 17241) {
      bt = 0x13;
    }
    else if (m_dataRate == 9579) {
      bt = 0x23;
    }
    else {
      bt = (byte)(344828UL / m_dataRate) - 1;
    }
    RFMxx::spi16(0xC600 | bt);
  }
}

void RFMxx::SetFrequency(unsigned long kHz) {
  m_frequency = kHz;

  if (IsRF69) {
    unsigned long f = (((kHz * 1000) << 2) / (32000000L >> 11)) << 6;
    WriteReg(0x07, f >> 16);
    WriteReg(0x08, f >> 8);
    WriteReg(0x09, f);
  }
  else {
    RFMxx::spi16(40960 + (m_frequency - 860000) / 5);
  }
}

void RFMxx::EnableReceiver(bool enable, bool fClearFifo){
  if (enable) {
    if (IsRF69) {
      WriteReg(REG_OPMODE, (ReadReg(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER);
    }
    else {
      spi16(0x82C8);
      spi16(0xCA81);
      spi16(0xCA83);
    }
  }
  else {
    if (IsRF69) {
      WriteReg(REG_OPMODE, (ReadReg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
    }
    else {
      spi16(0x8208);
    }
  }
  if (fClearFifo) {
  	ClearFifo();
 }
}

void RFMxx::EnableTransmitter(bool enable){
  if (enable) {
    if (IsRF69) {
      WriteReg(REG_OPMODE, (ReadReg(REG_OPMODE) & 0xE3) | RF_OPMODE_TRANSMITTER);
    }
    else {
      spi16(0x8238);
    }
  }
  else {
    if (IsRF69) {
      WriteReg(REG_OPMODE, (ReadReg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
    }
    else {
      spi16(0x8208);
    }
  }
}

byte RFMxx::GetByteFromFifo() {
  return IsRF69 ? ReadReg(0x00) : (byte)spi16(0xB000);
}

bool RFMxx::PayloadIsReady() {
  return m_payloadReady;
}


bool RFMxx::ClearFifo() {
  if (IsRF69) {
    WriteReg(REG_IRQFLAGS2, 16);
  }
  else {
    for (byte i = 0; i < PAYLOADSIZE; i++) {
      spi16(0xB000);
    }
  }

}

void RFMxx::PowerDown(){
  if (IsRF69) {
    WriteReg(REG_OPMODE, (ReadReg(REG_OPMODE) & 0xE3) | RF_OPMODE_SLEEP);
  }
  else {
    spi16(0x8201);
  }
}

void RFMxx::InitialzeLaCrosse() {
  if (m_debug) {
    Serial.print("Radio is: ");
    Serial.println(GetRadioName());
  }

  digitalWrite(m_ss, HIGH);
  EnableReceiver(false);

  if (IsRF69) {
    /* 0x01 */ WriteReg(REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY);
    /* 0x02 */ WriteReg(REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00);
    /* 0x05 */ WriteReg(REG_FDEVMSB, RF_FDEVMSB_90000);
    /* 0x06 */ WriteReg(REG_FDEVLSB, RF_FDEVLSB_90000);
    /* 0x11 */ WriteReg(REG_PALEVEL, RF_PALEVEL_PA0_ON | RF_PALEVEL_PA1_OFF | RF_PALEVEL_PA2_OFF | RF_PALEVEL_OUTPUTPOWER_11111);
    /* 0x13 */ WriteReg(REG_OCP, RF_OCP_OFF);
    /* 0x19 */ WriteReg(REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_16 | RF_RXBW_EXP_2);
    /* 0x28 */ WriteReg(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN);
    /* 0x29 */ WriteReg(REG_RSSITHRESH, 220);
    /* 0x2E */ WriteReg(REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_2 | RF_SYNC_TOL_0);
    /* 0x2F */ WriteReg(REG_SYNCVALUE1, 0x2D);
    /* 0x30 */ WriteReg(REG_SYNCVALUE2, 0xD4);
    /* 0x37 */ WriteReg(REG_PACKETCONFIG1, RF_PACKET1_CRCAUTOCLEAR_OFF);
    /* 0x38 */ WriteReg(REG_PAYLOADLENGTH, PAYLOADSIZE);
    /* 0x3C */ WriteReg(REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE);
    /* 0x3D */ WriteReg(REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS | RF_PACKET2_AUTORXRESTART_ON | RF_PACKET2_AES_OFF);
    /* 0x6F */ WriteReg(REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0);
  }
  else {
    spi16(0x8208);              // RX/TX off
    spi16(0x80E8);              // 80e8 CONFIGURATION EL,EF,868 band,12.5pF  (iT+ 915  80f8)
    spi16(0xC26a);              // DATA FILTER
    spi16(0xCA12);              // FIFO AND RESET  8,SYNC,!ff,DR
    spi16(0xCEd4);              // SYNCHRON PATTERN  0x2dd4
    spi16(0xC481);              // AFC during VDI HIGH
    spi16(0x94a0);              // RECEIVER CONTROL VDI Medium 134khz LNA max DRRSI 103 dbm
    spi16(0xCC77);              //
    spi16(0x9850);              // Deviation 90 kHz
    spi16(0xE000);              //
    spi16(0xC800);              //
    spi16(0xC040);              // 1.66MHz,2.2V
  }

  SetFrequency(m_frequency);
  SetDataRate(m_dataRate);

  ClearFifo();
}

byte RFMxx::GetTemperature() {
  byte result = 0;
  if (IsRF69) {
    byte receiverWasOn = ReadReg(REG_OPMODE) & 0x10;

    EnableReceiver(false);

    WriteReg(0x4E, 0x08);
    while ((ReadReg(0x4E) & 0x04));
    result = ~ReadReg(0x4F) - 90;

    if (receiverWasOn) {
      EnableReceiver(true);
    }
  }

  return result;
}


#define clrb(pin) (*portOutputRegister(digitalPinToPort(pin)) &= ~digitalPinToBitMask(pin))
#define setb(pin) (*portOutputRegister(digitalPinToPort(pin)) |= digitalPinToBitMask(pin))
byte RFMxx::spi8(byte value) {
  volatile byte *misoPort = portInputRegister(digitalPinToPort(m_miso));
  byte misoBit = digitalPinToBitMask(m_miso);
  for (byte i = 8; i; i--) {
    clrb(m_sck);
    if (value & 0x80) {
      setb(m_mosi);
    }
    else {
      clrb(m_mosi);
    }
    value <<= 1;
    setb(m_sck);
    if (*misoPort & misoBit) {
      value |= 1;
    }
  }
  clrb(m_sck);

  return value;
}

unsigned short RFMxx::spi16(unsigned short value) {
  volatile byte *misoPort = portInputRegister(digitalPinToPort(m_miso));
  byte misoBit = digitalPinToBitMask(m_miso);

  clrb(m_ss);
  for (byte i = 0; i < 16; i++) {
    if (value & 32768) {
      setb(m_mosi);
    }
    else {
      clrb(m_mosi);
    }
    value <<= 1;
    if (*misoPort & misoBit) {
      value |= 1;
    }
    setb(m_sck);
    asm("nop");
    asm("nop");
    clrb(m_sck);
  }
  setb(m_ss);
  return value;
}

byte RFMxx::ReadReg(byte addr) {
  digitalWrite(m_ss, LOW);
  spi8(addr & 0x7F);
  byte regval = spi8(0);
  digitalWrite(m_ss, HIGH);
  return regval;

}

void RFMxx::WriteReg(byte addr, byte value) {
  digitalWrite(m_ss, LOW);
  spi8(addr | 0x80);
  spi8(value);

  digitalWrite(m_ss, HIGH);
}

RFMxx::RadioType RFMxx::GetRadioType() {
  return m_radioType;
}

String RFMxx::GetRadioName() {
  switch (GetRadioType()) {
    case RFMxx::RFM12B:
      return String("RFM12B");
      break;
    case RFMxx::RFM69CW:
      return String("RFM69CW");
      break;
    default:
      return String("None");
  }
}

RFMxx::RFMxx(byte mosi, byte miso, byte sck, byte ss, byte irq) {
  m_mosi = mosi;
  m_miso = miso;
  m_sck = sck;
  m_ss = ss;
  m_irq = irq;

  m_debug = false;
  m_dataRate = 17241;
  m_frequency = 868300;
  m_payloadPointer = 0;
  m_lastReceiveTime = 0;
  m_payloadReady = false;
  m_payload_crc = 0;

  pinMode(m_mosi, OUTPUT);
  pinMode(m_miso, INPUT);
  pinMode(m_sck, OUTPUT);
  pinMode(m_ss, OUTPUT);
  pinMode(m_irq, INPUT);

  digitalWrite(m_ss, HIGH);

  m_radioType = RFMxx::RFM12B;
  WriteReg(REG_PAYLOADLENGTH, 0xA);
  if (ReadReg(REG_PAYLOADLENGTH) == 0xA) {
    WriteReg(REG_PAYLOADLENGTH, 0x40);
    if (ReadReg(REG_PAYLOADLENGTH) == 0x40) {
      m_radioType = RFMxx::RFM69CW;
    }
  }

}

void RFMxx::SetDebugMode(boolean mode) {
  m_debug = mode;
}
unsigned long RFMxx::GetDataRate() {
  return m_dataRate;
}

unsigned long RFMxx::GetFrequency() {
  return m_frequency;
}
void RFMxx::SendByte(byte data) {
  while (!(spi16(0x0000) & 0x8000)) {}
  RFMxx::spi16(0xB800 | data);
}


void RFMxx::SendArray(byte *data, byte length) {
if (IsRF69) {
    WriteReg(REG_PACKETCONFIG2, (ReadReg(REG_PACKETCONFIG2) & 0xFB) | RF_PACKET2_RXRESTART); // avoid RX deadlocks

    EnableReceiver(false);
    ClearFifo();

    noInterrupts();
    digitalWrite(m_ss, LOW);

    spi8(REG_FIFO | 0x80);
    for (byte i = 0; i < length; i++) {
      spi8(data[i]);
    }

    digitalWrite(m_ss, HIGH);
    interrupts();

    EnableTransmitter(true);

    // Wait until transmission is finished
    unsigned long txStart = millis();
    while (!(ReadReg(REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT) && millis() - txStart < 500);

    EnableTransmitter(false);
  }
  else {
    // Transmitter on
    EnableTransmitter(true);

    // Sync, sync, sync ...
    RFMxx::SendByte(0xAA);
    RFMxx::SendByte(0xAA);
    RFMxx::SendByte(0xAA);
    RFMxx::SendByte(0x2D);
    RFMxx::SendByte(0xD4);

    // Send the data
    for (int i = 0; i < length; i++) {
      RFMxx::SendByte(data[i]);
    }

    // Transmitter off
    delay(1);
    EnableTransmitter(false);
  }

  if (m_debug) {
    Serial.print("Sending data: ");
    for (int p = 0; p < length; p++) {
      Serial.print(data[p], DEC);
      Serial.print(" ");
    }
    Serial.println();
  }
}



