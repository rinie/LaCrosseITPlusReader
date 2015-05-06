#ifndef _TRANSMITTER_h
#define _TRANSMITTER_h

#include "Arduino.h"
#include "RFMxx.h"


class Transmitter {
 private:
   RFMxx *m_rfm;
   bool m_enabled;
   unsigned long m_dataRate;
   byte m_id;
   word m_interval;
   unsigned long m_newBatteryFlagResetTime;
   bool m_newBatteryFlag;
   float m_temperature;
   byte m_humidity;
   unsigned long m_lastTransmit;


 public:
   Transmitter(RFMxx *rfm);
   void Enable(bool enabled);
   bool Transmit();
   void SetParameters(byte id, word interval, bool newBatteryFlag, unsigned long newBatteryFlagResetTime, unsigned long dataRate);
   void SetValues(float temperature, byte humidity);
};

#endif

