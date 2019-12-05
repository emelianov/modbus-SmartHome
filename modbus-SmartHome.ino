//////////////////////////////////////////
// ESP8266/ESP32 Modbus SmartHome Device
// (c)2018, a.m.emelianov@gmail.com
//

#define BUSY ;
#define IDLE ;
#define VERSION "0.2  -  " __DATE__
#define TDEBUG(format, ...) Serial.printf_P(PSTR(format), ##__VA_ARGS__);
#define MEM_LOW 4096

extern String sysName;
#ifdef ESP8266
 #include <FS.h>
#else
 #include <SPIFFS.h>
#endif
#include <ModbusIP_ESP8266.h>
#include "wifi.h"
#include "web.h"
#include "mb.h"
#include "ds1820.h"
#include "gpio.h"
#include "cli.h"
#include "update.h"

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
  Serial.begin(115200);
 #endif
  SPIFFS.begin();
  wifiInit();    // Connect to WiFi network & Start discovery services
  webInit();     // Start Web-server
  modbusInit();
  taskAdd(cliInit);
  taskAdd(updateInit);
  taskAdd(dsInit);      // Start 1-Wire temperature sensors
  taskAdd(gpioInit);
}

void loop() { 
  taskExec();
  yield();
}
