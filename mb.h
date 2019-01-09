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
  bool pull = true;
  TAddress reg = COIL(0);
  IPAddress ip;// = IPADDR_NONE;
  uint8_t count = 1;
  uint32_t interval = 0xFFFFFFFF;
  uint16_t taskId = 0;
  TAddress remote = COIL(0);
};

std::vector<register_t> regs;

bool cbPull(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  //Serial.printf_P("Pull result: 0x%02X\n", event);
  return true;
}

uint32_t queryRemoteIreg() {
  uint16_t thisTaskId = taskId();
  std::vector<register_t>::iterator it = std::find_if(regs.begin(), regs.end(), [thisTaskId](const register_t& d) {return d.taskId == thisTaskId;});
  if (it != regs.end()) {
    mb->pullIreg(it->ip, it->remote.address, it->reg.address, it->count, cbPull);
    return it->interval;
  }
  return RUN_DELETE;
}

uint32_t queryRemoteHreg() {
  uint16_t thisTaskId = taskId();
  std::vector<register_t>::iterator it = std::find_if(regs.begin(), regs.end(), [thisTaskId](const register_t& d) {return d.taskId == thisTaskId;});
  if (it != regs.end()) {
    if (mb->isConnected(it->ip)) {
      mb->pullHregToIreg(it->ip, it->remote.address, it->reg.address, it->count, cbPull);
    } else {
      Serial.println(ESP.getFreeHeap());
      mb->connect(it->ip);
      Serial.println(ESP.getFreeHeap());
    }
    return it->interval;
  }
  return RUN_DELETE;
}

uint32_t queryRemoteCoil() {
  uint16_t thisTaskId = taskId();
  std::vector<register_t>::iterator it = std::find_if(regs.begin(), regs.end(), [thisTaskId](const register_t& d) {return d.taskId == thisTaskId;});
  if (it != regs.end()) {
    if (mb->isConnected(it->ip)) {
      if (it->pull) {
        mb->pullCoilToIsts(it->ip, it->remote.address, it->reg.address, it->count, cbPull);
      } else {
        mb->pushIstsToCoil(it->ip, it->remote.address, it->reg.address, it->count, cbPull);
      }
    } else {
      Serial.println(ESP.getFreeHeap());
      mb->connect(it->ip);
      Serial.println(ESP.getFreeHeap());
    }
    return it->interval;
  }
  return RUN_DELETE;
}

uint16_t addIregPull(IPAddress ip, uint16_t remote, uint16_t local, uint32_t interval) {
  register_t reg;
  mb->addIreg(local);
  reg.ip = ip;
  reg.reg = IREG(local);
  reg.remote = IREG(remote);
  reg.interval = interval;
  reg.taskId = taskAdd(queryRemoteIreg);
  regs.push_back(reg);
  return reg.taskId;
}

uint16_t addPull(IPAddress ip, TAddress remote, TAddress local, uint32_t interval, uint8_t count = 1, bool pull = true) {
  register_t reg;
  switch (local.type) {
  case TAddress::HREG:
    mb->addHreg(local.address);
  break;
  case TAddress::IREG:
    mb->addIreg(local.address);
  break;
  case TAddress::COIL:
    mb->addCoil(local.address);
  break;
  case TAddress::ISTS:
    mb->addIsts(local.address);
  break;
  }
  reg.reg = local;
  reg.ip = ip;
  reg.remote = remote;
  switch (remote.type) {
  case TAddress::HREG:
    reg.taskId = taskAdd(queryRemoteHreg);
  break;
  case TAddress::IREG:
    reg.taskId = taskAdd(queryRemoteIreg);
  break;
  case TAddress::COIL:
    reg.taskId = taskAdd(queryRemoteCoil);
  break;
  case TAddress::ISTS:
  //  reg.taskId = taskAdd(queryRemoteIsts);
  break;
  }
  reg.pull = pull;
  reg.count = count;
  reg.interval = interval;
  regs.push_back(reg);
  return reg.taskId;
}

uint16_t cb1(TRegister* reg, uint16_t val) {
  Serial.printf("Ireg: %d\n", val);
  return val;
}

bool saveModbus() {
  File configFile = SPIFFS.open(CFG_MODBUS, "w");
  if (configFile) {
    Serial.println("mb: create");
    cJSON* json = cJSON_CreateObject();
    if (json) {
      cJSON* parm;
      parm = cJSON_CreateNumber(0);
      if (parm) cJSON_AddItemToObject(json, "dummy", parm);
      cJSON* arr = cJSON_CreateArray();
      if (arr) {
        for (register_t& reg : regs) {
            if (reg.taskId == 0) continue;
            cJSON* item;
            cJSON* entry = cJSON_CreateObject();
            if (!entry) continue;
            item = cJSON_CreateString(reg.ip.toString().c_str());
            if (!item) continue;
            cJSON_AddItemToObject(entry, "ip", item);
            item = cJSON_CreateNumber(reg.reg.type);
            if (!item) continue;
            cJSON_AddItemToObject(entry, "ltype", item);
            item = cJSON_CreateNumber(reg.reg.address);
            if (!item) continue;
            cJSON_AddItemToObject(entry, "laddress", item);
            item = cJSON_CreateString(reg.ip.toString().c_str());
            if (!item) continue;
            cJSON_AddItemToObject(entry, "rip", item);
            item = cJSON_CreateNumber(reg.remote.type);
            if (!item) continue;
            cJSON_AddItemToObject(entry, "rtype", item);
            item = cJSON_CreateNumber(reg.remote.address);
            if (!item) continue;
            cJSON_AddItemToObject(entry, "raddress", item);
            item = cJSON_CreateNumber(reg.count);
            if (!item) continue;
            cJSON_AddItemToObject(entry, "count", item);                        
            item = cJSON_CreateNumber(reg.interval);
            if (!item) continue;
            cJSON_AddItemToObject(entry, "interval", item);                        
            item = cJSON_CreateBool(reg.pull);
            if (!item) continue;
            cJSON_AddItemToObject(entry, "pull", item);                        
            item = cJSON_CreateString(reg.name.c_str());
            if (item) cJSON_AddItemToObject(entry, "name", item);
            cJSON_AddItemToArray(arr, entry);
          }
          cJSON_AddItemToObject(json, "registers", arr);
          char* out = cJSON_Print(json);
          configFile.write((uint8_t*)out, strlen(out));
          cJSON_Delete(json);
          free(out);
      }
      configFile.close();
      return true;
    }
    configFile.close();
    return false;
  }
  return false;
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
      if (configFile.read((uint8_t*)data, configFile.size()) == configFile.size()) {
        data[configFile.size()] = '/0';
        TDEBUG("mb: config %d bytes read\n", configFile.size());
        cJSON* json = cJSON_Parse(data);
        if (json) {
          TDEBUG("mb: JSON parsed\n");
          cJSON* arr = NULL;
          cJSON* entry = NULL;
          cJSON* item;
          if (cJSON_HasObjectItem(json, "dummy")) {
            item = cJSON_GetObjectItemCaseSensitive(json, "dummy");
            if (item && cJSON_IsNumber(item))
              isMaster = item->valueint;
          }
          i = 0;
          if (cJSON_HasObjectItem(json, "registers")) arr = cJSON_GetObjectItemCaseSensitive(json, "registers");
          if (arr) {
            TDEBUG("mb: registers:\n");
            cJSON_ArrayForEach(entry, arr) {
              register_t reg;
              if (i > PULL_MAX_COUNT) break;
              i++;
              TDEBUG("mb: #%d\n", i);
              cJSON* item = NULL;
              if (!cJSON_HasObjectItem(entry, "interval")) continue;
              item = cJSON_GetObjectItemCaseSensitive(entry, "interval");
              if (!item || !cJSON_IsNumber(item)) continue;
              reg.interval = item->valueint;
              if (!cJSON_HasObjectItem(entry, "rtype")) continue;
              item = cJSON_GetObjectItemCaseSensitive(entry, "rtype");
              if (!item || !cJSON_IsNumber(item)) continue;
              reg.remote.type = (TAddress::RegType)item->valueint;
              if (!cJSON_HasObjectItem(entry, "raddress")) continue;
              item = cJSON_GetObjectItemCaseSensitive(entry, "raddress");
              if (!item || !cJSON_IsNumber(item)) continue;
              reg.remote.address = item->valueint;
              if (!cJSON_HasObjectItem(entry, "ltype")) continue;
              item = cJSON_GetObjectItemCaseSensitive(entry, "ltype");
              if (!item || !cJSON_IsNumber(item)) continue;
              reg.reg.type = (TAddress::RegType)item->valueint;
              if (!cJSON_HasObjectItem(entry, "laddress")) continue;
              item = cJSON_GetObjectItemCaseSensitive(entry, "laddress");
              if (!item || !cJSON_IsNumber(item)) continue;
              reg.reg.address = item->valueint;
              if (cJSON_HasObjectItem(entry, "count")) {
                item = cJSON_GetObjectItemCaseSensitive(entry, "count");
                if (item && cJSON_IsNumber(item))
                  reg.count = item->valueint;
              }
              if (!cJSON_HasObjectItem(entry, "pull")) continue;
              item = cJSON_GetObjectItemCaseSensitive(entry, "pull");
              if (!item || !cJSON_IsBool(item)) continue;
              reg.pull = item->valueint;
              if (!cJSON_HasObjectItem(entry, "ip")) continue;
              item = cJSON_GetObjectItemCaseSensitive(entry, "ip");
              if (!item || !cJSON_IsString(item)) continue;
              TDEBUG("mb: ip %s", item->valuestring);
              reg.ip.fromString(item->valuestring);
              addPull(reg.ip, reg.remote, reg.reg, reg.interval);
              //regs.push_back(reg);
            }
            //cJSON_Delete(arr);
          }
          cJSON_Delete(json);
        }
        free(data);
        configFile.close();
        return true;
      }
      configFile.close();
      return false;
    }
    return false;
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
//  taskAdd(readRemote);
  mb->connect(IPAddress(192,168,30,13));
  taskAdd(modbusLoop);
  return RUN_DELETE;
}
