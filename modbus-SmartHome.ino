//////////////////////////////////////////
// ESP8266/ESP32 Modbus SmartHome Device
// (c)2018, a.m.emelianov@gmail.com
//

#define BUSY ;
#define IDLE ;
#define VERSION "0.2  -  " __DATE__
#define TDEBUG(format, ...) //Serial.printf_P(PSTR(format), ##__VA_ARGS__);
#define MEM_LOW 4096
#define WS2812
#define DEFAULT_NAME "MR1"
#define DEFAULT_PASSWORD "P@ssw0rd"
String sysName = DEFAULT_NAME;
String sysPassword = DEFAULT_PASSWORD;

extern String sysName;
#ifdef ESP8266
 #include <FS.h>
#else
 #include <SPIFFS.h>
#endif
#include <Run.h>
#include <ModbusIP_ESP8266.h>
#include "wifi.h"
#include "web.h"
#include "mb.h"
#include "cli.h"
#include "ds1820.h"
#include "gpio.h"
#include "update.h"
#if defined(WS2812) && defined(ESP8266)
 #include "leds.h"
#endif


ModbusIP* mb;
uint32_t mem = 0;
uint32_t tm = 0;
uint32_t restartESP() {
  ESP.restart();
  return RUN_DELETE;
}

uint32_t watchDog() {
  if (ESP.getFreeHeap() < MEM_LOW) ESP.restart();
  return 10000;
}

void setup(void)
{
 #ifdef TDEBUG
  //Serial.begin(74880);
 #endif
  SPIFFS.begin();
  wifiInit();    // Connect to WiFi network & Start discovery services
  webInit();     // Start Web-server
  modbusInit();
  taskAdd(cliInit);
  taskAdd(updateInit);
  taskAdd(dsInit);      // Start 1-Wire temperature sensors
  taskAdd(gpioInit);
  #if defined(WS2812) && defined(ESP8266)
   taskAdd(ledsInit);
  #endif
}

void loop() { 
  taskExec();
  yield();
}
