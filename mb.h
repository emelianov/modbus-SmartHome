#pragma once
#include <ModbusIP_ESP8266.h>
#include <FS.h>
#include <cJSON.h>

#define CFG_MODBUS "/modbus.json"
#define MB_IDLE 100
#define PULL_MAX_COUNT 10

extern ModbusIP* mb;

uint32_t modbusLoop() {
  mb->task();
  return MB_IDLE;
}

bool connect(IPAddress ip) {
  Serial.println(ip);
  return true;
}

struct register_t {
  String name = "";
  bool pull = false;
  TAddress reg = COIL(0);
  IPAddress ip = IPADDR_NONE;
  uint32_t interval = 0xFFFFFFFF;
  uint16_t taskId = 0;
  uint16_t remote = 0;
};

std::vector<register_t> regs;

bool cbPull(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  Serial.print("Pull result: ");
  Serial.println(event);
  return true;
}

uint32_t queryRemoteIreg() {
  uint16_t thisTaskId = taskId();
  std::vector<register_t>::iterator it = std::find_if(regs.begin(), regs.end(), [thisTaskId](const register_t& d) {return d.taskId == thisTaskId;});
  if (it != regs.end()) {
    mb->pullHregToIreg(it->ip, it->remote, it->reg.address, 1, cbPull);
    return it->interval;
  }
  return RUN_DELETE;
}

uint16_t cb1(TRegister* reg, uint16_t val) {
  Serial.printf("Ireg: %d\n", val);
  return val;
}

// Read configuration of Modbus Push/Pull targets
bool readModbus() {
  int16_t i;
  bool isMaster = false;
  bool isSlave = false;
  File configFile = SPIFFS.open(CFG_MODBUS, "r");
  if (configFile) {
    Serial.println("mb: open");
   char* data = (char*)malloc(configFile.size() + 1);
   if (data) {
    configFile.read((uint8_t*)data, configFile.size());
    data[configFile.size()] = '/0';
    Serial.println("Modbus: config reading complete");
    cJSON* json = cJSON_Parse(data);
    if (json) {
      Serial.println("mb: parse");
      cJSON* arr = NULL;
      cJSON* entry = NULL;
      cJSON* item;
      if (cJSON_HasObjectItem(json, "master")) {
        item = cJSON_GetObjectItemCaseSensitive(json, "master");
        if (item && cJSON_IsNumber(item))
          isMaster = item->valuedouble;
      }
      if (cJSON_HasObjectItem(json, "slave")) {
        item = cJSON_GetObjectItemCaseSensitive(json, "slave");
        if (item && cJSON_IsNumber(item))
          isSlave = item->valuedouble;
      }
      i = 0;
      Serial.println("Modbus: Push/Pull");
      if (cJSON_HasObjectItem(json, "registers")) arr = cJSON_GetObjectItemCaseSensitive(json, "registers");
      if (arr) {
        Serial.println("mb: registers");
        cJSON_ArrayForEach(entry, arr) {
          register_t reg;
          task cb = nullptr;
          if (i > PULL_MAX_COUNT) break;
          i++;
          Serial.println(i);
          cJSON* item = NULL;
          if (cJSON_HasObjectItem(entry, "interval")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "interval");
            if (item && cJSON_IsNumber(item))
              Serial.println("mb: interval");
              reg.interval = item->valuedouble;
          }
          if (cJSON_HasObjectItem(entry, "remote")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "remote");
            if (item && cJSON_IsNumber(item))
              reg.remote = item->valuedouble;
          }
          if (cJSON_HasObjectItem(entry, "hreg")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "hreg");
            if (item && cJSON_IsNumber(item))
              reg.reg = HREG(item->valuedouble);
          }
          if (cJSON_HasObjectItem(entry, "ireg")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "ireg");
            if (item && cJSON_IsNumber(item))
              Serial.println("mb: ireg");
              reg.reg = IREG(item->valuedouble);
              mb->addIreg(item->valuedouble);
              mb->onSetIreg(item->valuedouble, cb1);
          }
          if (cJSON_HasObjectItem(entry, "coil")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "coil");
            if (item && cJSON_IsNumber(item))
              reg.reg = COIL(item->valuedouble);
          }
          if (cJSON_HasObjectItem(entry, "ists")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "ists");
            if (item && cJSON_IsNumber(item))
              reg.reg = ISTS(item->valuedouble);
          }
          if (cJSON_HasObjectItem(entry, "ip")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "ip");
            if (item && cJSON_IsString(item)) {
              Serial.println("mb: ip");
              reg.ip.fromString(item->valuestring);
              cb = queryRemoteIreg;
            }
          }
          if (cb) {
            reg.taskId = taskAddWithDelay(cb, reg.interval);
          }
          regs.push_back(reg);
        }
        //cJSON_Delete(arr);
      }
      cJSON_Delete(json);
    }
    free(data);
   }
   configFile.close();
   return true;
  }
  return false;
}

bool cbRead(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  Serial.print("Read 3: ");
  Serial.println(event);
  return true;
}

uint16_t cb10(TRegister* reg, uint16_t val) {
  Serial.printf("New value: %d\n", val);
  return val;
}

uint32_t readRemote() {
  mb->pullHreg(IPAddress(192,168,30,13), 528, 10, 4, cbRead);
  return 10000;
}

uint32_t modbusInit() {
  mb = new ModbusIP();
  mb->onConnect(connect);
  readModbus();
  mb->slave();
  mb->master();
  mb->addHreg(10, 0, 5);
  mb->onSetHreg(10, cb10, 5);
  taskAdd(readRemote);
  mb->connect(IPAddress(192,168,30,13));
  taskAdd(modbusLoop);
  return RUN_DELETE;
}

