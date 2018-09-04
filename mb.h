#pragma once
#include <ModbusIP_ESP8266.h>

#define MB_IDLE 100

extern ModbusIP* mb;

uint32_t modbusLoop() {
  mb->task();
  return MB_IDLE;
}

bool connect(IPAddress ip) {
  Serial.println(ip);
  return true;
}

uint32_t modbusInit() {
  mb = new ModbusIP();
  mb->onConnect(connect);
  mb->slave();
  taskAdd(modbusLoop);
  return RUN_DELETE;
}

