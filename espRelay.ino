////////////////////////////////////////
// ESP8266/ESP32 Temperature sensor
// (c)2018, a.m.emelianov@gmail.com
//

#define BUSY ;
#define IDLE ;
#define VERSION "0.1"
#define TDEBUG(...) Serial.printf(##__VA_ARGS__);

extern String sysName;

#include "wifi.h"
#include "web.h"
#include "ds1820.h"

void setup(void)
{
 #ifdef TDEBUG
 #ifdef ESP8266
  Serial.begin(74880);
 #else
  Serial.begin(115200);
 #endif
 #endif
  wifiInit();    // Connect to WiFi network & Start discovery services
  webInit();     // Start Web-server
  taskAdd(dsInit);      // Start 1-Wire temperature sensors
}

void loop() {
  taskExec();
  yield();
}
