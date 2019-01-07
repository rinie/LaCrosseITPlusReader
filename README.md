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

Now on ESP32 LORA (SX1276 is same as RFM96) 
See https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series

[LaCrosseWh1080ITPlusReader.00.2h (None) @AutoToggleWH1080 30 Seconds 17241 kbps / 868300 kHz]
042577 LaCrosse [98 45 96 6A 3F ] CRC:OK S:9 ID:33 NewBatt:0 Bit12:0 Temp:19.60 WeakBatt:0 Hum:106 CRC:63
98 Size:10 #:1
047997 Unknown [24 8B F7 61 DC 63 34 8 10 C7 0 0 0 0 0 ] CRC:WRONG Size:15 #:1
24 Size:15 #:1
008516 LaCrosse [98 45 96 6A 3F ] CRC:OK S:9 ID:33 NewBatt:0 Bit12:0 Temp:19.60 WeakBatt:0 Hum:106 CRC:63
98 Size:10 #:1
047999 WH1080 [A8 80 98 38 0 0 0 0 16 92 ] CRC:OK #:2 S:A ID:88 Temp:15.2 Hum:56 WindSpeed:0.0 WindGust:0.0 Unknown:0 Rain:0.0 Status:1 WindBearing:SE  CRC:92
A8 Size:10 #:2



