////////////////////////////////////////
// ESP8266/ESP32 Relay Module
// (c)2018, a.m.emelianov@gmail.com
//

#define BUSY ;
#define IDLE ;
#define VERSION "0.1"

extern String sysName;

#include "wifi.h"
#include "web.h"
#include "ds1820.h"

void setup(void)
{
 #ifdef ESP8266
  Serial.begin(74880);
 #else
  Serial.begin(115200);
 #endif
  wifiInit();    // Connect to WiFi network & Start discovery services
  webInit();     // Start Web-server
  taskAdd(dsInit);      // Start 1-Wire temperature sensors
}

void loop() {
  taskExec();
  yield();
}
