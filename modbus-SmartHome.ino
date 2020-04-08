//////////////////////////////////////////
// ESP8266/ESP32 Modbus SmartHome Device
// (c)2018, a.m.emelianov@gmail.com
//

#define VERSION "0.2  -  " __DATE__
#define SERIAL
//#define WS2812
//#define LCD
//#define JSONRPC
#define HOMEKIT
#define DS1820
#define MEM_LOW 4096
#define DEFAULT_NAME "mbh1"
#define DEFAULT_PASSWORD "P@ssw0rd"

#define TDEBUG(format, ...) //Serial.printf_P(PSTR(format), ##__VA_ARGS__);
#define BUSY ;
#define IDLE ;

String sysName = DEFAULT_NAME;
String sysPassword = DEFAULT_PASSWORD;

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
#include "gpio.h"
#include "update.h"
#if defined(DS1820)
 #include "ds1820.h"
#endif
#if defined(WS2812) && defined(ESP8266)
 #undef SERIAL
 #include "leds.h"
#endif
#if defined(LCD)
 #include "lcd.h"
#endif
#if defined(JSONRPC)
 #include "jsonrpc.h"
#endif
#if defined(HOMEKIT)
 #include "homekit.h"
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
 #if defined(SERIAL)
 #if defined(ESP32)
  Serial.begin(115200);
 #else
  Serial.begin(74880);
 #endif
 #endif
  SPIFFS.begin();
  wifiInit();    // Connect to WiFi network & Start discovery services
  webInit();     // Start Web-server
  modbusInit(); // Initialize Modbus subsystem
  taskAdd(cliInit);
  taskAdd(updateInit);
 #if defined(DS1820)
  taskAdd(dsInit);      // Start 1-Wire temperature sensors
 #endif
  taskAdd(gpioInit);
 #if defined(WS2812) && defined(ESP8266)
  taskAdd(ledsInit); // WS2812 led stripes
 #endif
 #if defined(JSONRPC)
  taskAdd(jsonRpcInit);
 #endif
 #if defined(HOMEKIT)
  taskAdd(homekitInit);
 #endif
}

void loop() { 
  taskExec();
  yield();
}
