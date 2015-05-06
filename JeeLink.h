#ifndef _JEELINK_h
#define _JEELINK_h

#include "Arduino.h"

class JeeLink {
private:
  bool m_ledEnabled;
  void SwitchLed(boolean on);

public:
  JeeLink();
  void EnableLED(bool enabled);
  void Blink(byte ct);
};

#endif

