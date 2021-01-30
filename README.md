# esp32HomeIoT-V2.0

This software is a github-port from my personal IoT-System. It uses an ESP32-Devboard, an DHT-Sensor, a relay and a IR-led.

I don't know if you find a good use for it so modify it as you like!
## Used Librarys

- [ThingerESP32](https://github.com/thinger-io/Arduino-Library) 
  - Used as online api.

- [WiFi, WiFiMulti, WiFiUdp](https://github.com/espressif/arduino-esp32/tree/master/libraries)
  - Used for wifi connection. WiFiUdp is a dependency of WakeOnLan.

- [WakeOnLan](https://github.com/a7md0/WakeOnLan) 
  - Used to turn on a WoL-Device.

- [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) 
  - Used to control ir led to make things like AV-receivers controllable from the internet.

- [Sinric.pro](https://github.com/sinricpro/esp8266-esp32-sdk) 
  - Amazon Echo sdk