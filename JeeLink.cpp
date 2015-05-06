#include "JeeLink.h"

JeeLink::JeeLink() {
  m_ledEnabled = true;
}

void JeeLink::SwitchLed(boolean on) {
  byte LED_PIN = 9;
  if (m_ledEnabled) {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, !on);
  }
}

void JeeLink::Blink(byte ct) {
  if (ct > 0) {
    if (ct > 10) {
      ct = 10;
    }
    for (int i = 0; i < ct; i++) {
      SwitchLed(true);
      delay(50);
      SwitchLed(false);
      delay(50);
    }

  }
}

void JeeLink::EnableLED(bool enabled) {
  m_ledEnabled = enabled;

  if (!m_ledEnabled) {
    SwitchLed(false);
  }

}
