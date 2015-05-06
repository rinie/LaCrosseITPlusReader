#include "Transmitter.h"
#include "LaCrosse.h"

Transmitter::Transmitter(RFMxx *rfm) {
  m_rfm = rfm;
  m_enabled = false;
  m_dataRate = 17241ul;
  m_id = 0;
  m_humidity = 0;
  m_temperature = 0.0;
  m_interval = 4000;
  m_lastTransmit = 0;
  m_newBatteryFlag = false;
  m_newBatteryFlagResetTime = 0;

}

void Transmitter::Enable(bool enabled){
  m_enabled = enabled;
}

bool Transmitter::Transmit() {
  bool result = false;

  if (m_enabled && millis() >= m_lastTransmit + m_interval) {
    m_lastTransmit = millis();

    // Reset the NewBatteryFlag, if it's time to do it 
    if (m_newBatteryFlag && millis() >= m_newBatteryFlagResetTime) {
      m_newBatteryFlag = false;
    }
    
    // Build a message with the current transmit data
    LaCrosse::Frame frame;
    frame.ID = m_id;
    frame.NewBatteryFlag = m_newBatteryFlag;
    frame.Bit12 = false;
    frame.Temperature = m_temperature;
    frame.WeakBatteryFlag = false;
    frame.Humidity = m_humidity;

    // Set the data rate and send it
    unsigned long currentDataRate = m_rfm->GetDataRate();
    m_rfm->SetDataRate(m_dataRate);
    byte bytes[LaCrosse::FRAME_LENGTH];
    LaCrosse::EncodeFrame(&frame, bytes);
    m_rfm->SendArray(bytes, LaCrosse::FRAME_LENGTH);
    
    m_rfm->SetDataRate(currentDataRate);
    
    result = true;

  }

  return result;
}

void Transmitter::SetParameters(byte id, word interval, bool newBatteryFlag, unsigned long newBatteryFlagResetTime, unsigned long dataRate) {
  m_id = id;
  m_interval = interval;
  m_newBatteryFlag = newBatteryFlag;
  m_newBatteryFlagResetTime = newBatteryFlagResetTime;
  m_dataRate = dataRate;
}

void Transmitter::SetValues(float temperature, byte humidity) {
  m_temperature = temperature;
  m_humidity = humidity;
}