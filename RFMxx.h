#ifndef _RFMXX_h
#define _RFMXX_h

#include "Arduino.h"
#define USE_SPI_H
#define USE_TIME_H
#ifdef ESP32
    #define USE_SX127x
#endif

#define PAYLOADSIZE 64
#define IsRF69 (m_radioType == RFM69CW)

#define IsSX127x (m_radioType == SX127x)

class RFMxx {
public:
  enum RadioType {
    RFM12B = 1,
    RFM69CW = 2,
    SX127x = 3
  };

#ifndef USE_SPI_H
  RFMxx(byte mosi, byte miso, byte sck, byte ss, byte irq);
#else
  RFMxx(byte ss=SS, byte irqPin=2, byte reset=-1); // jeelink en moteino irq 0 / irqPin =2
#endif
  void init();
  bool PayloadIsReady();
  byte GetPayload(byte *data);
  void InitialzeLaCrosse();
  void SendArray(byte *data, byte length);
  void SetDataRate(unsigned long dataRate);
  unsigned long GetDataRate();
  void SetFrequency(unsigned long kHz);
  unsigned long GetFrequency();
  void EnableReceiver(bool enable, bool fClearFifo = true);
  void EnableTransmitter(bool enable);
  static byte CalculateCRC(byte data[], int len);
  void PowerDown();
  void SetDebugMode(boolean mode);
  byte GetTemperature();
  RadioType GetRadioType();
  String GetRadioName();
  void Receive();
  bool ReceiveGetPayloadWhenReady(byte *data, byte &length, byte &packetCount);
private:
  RadioType m_radioType;
#ifndef USE_SPI_H
  byte m_mosi, m_miso, m_sck, m_ss, m_irq;
#else
  byte m_ss, m_irqPin, m_reset;
#endif
  bool m_debug;
  unsigned long m_dataRate;
  unsigned long m_frequency;
  byte m_payloadPointer;
  unsigned long m_lastReceiveTime;
  bool m_payloadReady;
  byte m_payload_min_size;
  byte m_payload_max_size;
  byte m_payload_crc;
  byte m_payload[PAYLOADSIZE];

  byte spi8(byte);
  unsigned short spi16(unsigned short value);
  byte ReadReg(byte addr);
  void WriteReg(byte addr, byte value);
  byte GetByteFromFifo();
  bool ClearFifo();
  void SendByte(byte data);

};

#ifdef USE_SX127x // use official semtech defines
#include "sx1276Regs-Fsk.h"
#else
#include "RFM69.h"
#endif
#endif


