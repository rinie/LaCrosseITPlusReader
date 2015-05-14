LaCrosse IT+ reader
taken from http://sourceforge.net/p/fhem/code/HEAD/tree/trunk/fhem/contrib/arduino/36_LaCrosse-LaCrosseITPlusReader.zip?format=raw

Works without any modification on jeelink with RFM12B.

WH1080 uses repeated packages
Unknown package: 10-6: A A6 D0 E9 2B 0 1 0 0 E 1D 

See http://www.sevenwatt.com/main/wh1080-protocol-v2-fsk/
and
http://www.sevenwatt.com/main/jeenode-rfm12b-configuration-commands/

008516 LaCrosse [98 45 40 6A A1 ] CRC:OK S:9 ID:33 NewBatt:0 Bit12:0 Temp:14.00 WeakBatt:0 Hum:106 CRC:161


048000 WH1080 [A6 D0 D5 28 1 4 0 0 0 35 ] CRC:OK #:6 S:A ID:6D Temp:21.30 Hum:40 Rain:0.0000000000 WindSpeed:0.3400000095 WindGust:1.3600000143 WindBearing:N   CRC:35

