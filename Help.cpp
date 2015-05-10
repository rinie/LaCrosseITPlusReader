#include "Help.h"

//- helpText ---------------------------------------------------------------------------------------
const char helpText[] PROGMEM =
"\n"
"Available commands:" "\n"
"  <n>a                     - activity LED (0=off, 1=on)" "\n"
"  <t10>,<t1>,<t0>,<hum>c   - set temperature and humidity for transmit" "\n"
"  <n>d                     - DEBUG mode (0=suppress TX and bad packets)" "\n"
"  <nnnnnn>f                - frequency (5 kHz steps e.g. 868315)" "\n"
"  <id>,<int>,<nbt>,<dr>i   - set the parameters for the transmit loop" "\n"
"  <n>r                     - data rate (0=17.241 kbps, 1=9.579 kbps)" "\n"
"  b1,b2,b3,b4s             - send the passed bytes plus the calculated CRC" "\n"
"  <n>t                     - toggle data rate intervall (0=no toggle, >0=seconds)" "\n"
"  <n>v                     - version and configuration report" "\n"
"  <n>y                     - Relay (0=no relay, 1=Relay received packets)" "\n"
"  <n>x                     - used for tests" "\n"
;


void Help::Show() {
  PGM_P s = helpText;
  for (;;) {
    char c = pgm_read_byte(s++);
    if (c == 0)
      break;
    if (c == '\n')
      Serial.print('\r');
    Serial.print(c);
  }
  Serial.println();
}



