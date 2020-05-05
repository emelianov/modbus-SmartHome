#pragma once
#include <Ethernet.h>
#include <ModbusIP_ESP8266.h>
#include <FS.h>
#include <cJSON.h>
#include <Shell.h>
#include <LoginShell.h>
extern LoginShell shell;

#define CFG_MODBUS "/modbus.json"
#define CFG_REGS "/defaults.json"
#define MB_IDLE 10
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

bool connectRemote(IPAddress ip) {
  Serial.println(ESP.getFreeHeap());
  mb->connect(ip);
  Serial.println(ESP.getFreeHeap());
}

uint32_t queryRemoteIreg() {
  uint16_t thisTaskId = taskId();
  std::vector<register_t>::iterator it = std::find_if(regs.begin(), regs.end(), [thisTaskId](const register_t& d) {return d.taskId == thisTaskId;});
  if (it != regs.end()) {
    if (mb->isConnected(it->ip)) {
      mb->pullIreg(it->ip, it->remote.address, it->reg.address, it->count, cbPull);
    } else {
      connectRemote(it->ip);
    }
    return it->interval;
  }
  return RUN_DELETE;
}

uint32_t queryRemoteIsts() {
  uint16_t thisTaskId = taskId();
  std::vector<register_t>::iterator it = std::find_if(regs.begin(), regs.end(), [thisTaskId](const register_t& d) {return d.taskId == thisTaskId;});
  if (it != regs.end()) {
    if (mb->isConnected(it->ip)) {
      mb->pullIsts(it->ip, it->remote.address, it->reg.address, it->count, cbPull);
    } else {
      connectRemote(it->ip);
    }
    return it->interval;
  }
  return RUN_DELETE;
}

uint32_t queryRemoteHreg() {
  uint16_t thisTaskId = taskId();
  std::vector<register_t>::iterator it = std::find_if(regs.begin(), regs.end(), [thisTaskId](const register_t& d) {return d.taskId == thisTaskId;});
  if (it != regs.end()) {
    if (mb->isConnected(it->ip)) {
      if (it->pull) {
        mb->pullHregToIreg(it->ip, it->remote.address, it->reg.address, it->count, cbPull);
      } else {
        mb->pushIregToHreg(it->ip, it->remote.address, it->reg.address, it->count, cbPull);
      }
    } else {
      connectRemote(it->ip);
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
      connectRemote(it->ip);
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
    reg.taskId = taskAdd(queryRemoteIsts);
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
      parm = cJSON_CreateString(VERSION);
      if (parm) cJSON_AddItemToObject(json, "version", parm);
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
          //if (cJSON_HasObjectItem(json, "dummy")) {
          //  item = cJSON_GetObjectItemCaseSensitive(json, "dummy");
          //  if (item && cJSON_IsNumber(item))
          //    isMaster = item->valueint;
          //}
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
              addPull(reg.ip, reg.remote, reg.reg, reg.interval, reg.count, reg.pull);
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

char* regTypeToStr(TAddress reg);
bool saveDefaults(TAddress reg, uint16_t value, bool erase = false) {
  cJSON* json = nullptr;
  cJSON* arr = nullptr;
  File configFile = SPIFFS.open(CFG_REGS, "r");
  if (configFile) {
    char* data = (char*)malloc(configFile.size() + 1);
    if (data) {
      if (configFile.read((uint8_t*)data, configFile.size()) == configFile.size()) {
        data[configFile.size()] = '/0';
        TDEBUG("mb: config %d bytes read\n", configFile.size());
        json = cJSON_Parse(data);
      }
      free(data);
    }
    configFile.close();
  }
  if (!json) {
    json = cJSON_CreateObject();
    if (!json) return false;
    cJSON* parm;
    parm = cJSON_CreateString(VERSION);
    if (parm) cJSON_AddItemToObject(json, "version", parm);
  }
  arr = cJSON_GetObjectItemCaseSensitive(json, regTypeToStr(reg));
  if (!arr) {
    arr = cJSON_CreateArray();
    if (arr) cJSON_AddItemToObject(json, regTypeToStr(reg), arr);
  }
  if (arr) {
    char* out;
    cJSON* item;
    cJSON* entry = nullptr;
    for (uint16_t i = 0; i < cJSON_GetArraySize(arr); i++) {
      entry = cJSON_GetArrayItem(arr, i);
      if (!entry) continue;
      item = cJSON_GetObjectItemCaseSensitive(entry, "address");
      if (item && cJSON_IsNumber(item) && item->valueint == reg.address) {
        cJSON_DeleteItemFromArray(arr, i);
        break;
      }
    }
    entry = cJSON_CreateObject();
    if (!entry) goto cleanup;
    item = cJSON_CreateNumber(reg.address);
    if (!item) goto cleanup;
    cJSON_AddItemToObject(entry, "address", item);
    item = cJSON_CreateNumber(value);
    if (!item) goto cleanup;
    cJSON_AddItemToObject(entry, "value", item);
    if (!erase) cJSON_AddItemToArray(arr, entry);
    configFile = SPIFFS.open(CFG_REGS, "w");
    if (configFile) {
      out = cJSON_Print(json);
      configFile.write((uint8_t*)out, strlen(out));
      configFile.close();
      free(out);
    }
    cleanup:
    //cJSON_Delete(entry);
    cJSON_Delete(json);
    return true;
  }
  return false;
}

bool readDefaults() {
  cJSON* json = nullptr;
  cJSON* arr = nullptr;
  File configFile = SPIFFS.open(CFG_REGS, "r");
  if (configFile) {
    char* data = (char*)malloc(configFile.size() + 1);
    if (data) {
      if (configFile.read((uint8_t*)data, configFile.size()) == configFile.size()) {
        data[configFile.size()] = '/0';
        TDEBUG("mb: default %d bytes read\n", configFile.size());
        json = cJSON_Parse(data);
      }
      free(data);
    }
    configFile.close();
  }
  if (!json) return false;
  cJSON* address;
  cJSON* value;
  cJSON* entry;
  arr = cJSON_GetObjectItemCaseSensitive(json, "COIL");
  if (arr)
  cJSON_ArrayForEach(entry, arr) {
    address = cJSON_GetObjectItemCaseSensitive(entry, "address");
    value = cJSON_GetObjectItemCaseSensitive(entry, "value");
    if (value && address && cJSON_IsNumber(value) && cJSON_IsNumber(address)) {
      mb->addCoil(address->valueint, (value->valueint == 1));
    }
  }
  arr = cJSON_GetObjectItemCaseSensitive(json, "ISTS");
  if (arr)
  cJSON_ArrayForEach(entry, arr) {
    address = cJSON_GetObjectItemCaseSensitive(entry, "address");
    value = cJSON_GetObjectItemCaseSensitive(entry, "value");
    if (value && address && cJSON_IsNumber(value) && cJSON_IsNumber(address)) {
      mb->addIsts(address->valueint, (value->valueint == 1));
    }
  }
  arr = cJSON_GetObjectItemCaseSensitive(json, "HREG");
  if (arr)
  cJSON_ArrayForEach(entry, arr) {
    address = cJSON_GetObjectItemCaseSensitive(entry, "address");
    value = cJSON_GetObjectItemCaseSensitive(entry, "value");
    if (value && address && cJSON_IsNumber(value) && cJSON_IsNumber(address)) {
      mb->addHreg(address->valueint, value->valueint);
    }
  }
  arr = cJSON_GetObjectItemCaseSensitive(json, "IREG");
  if (arr)
  cJSON_ArrayForEach(entry, arr) {
    address = cJSON_GetObjectItemCaseSensitive(entry, "address");
    value = cJSON_GetObjectItemCaseSensitive(entry, "value");
    if (value && address && cJSON_IsNumber(value) && cJSON_IsNumber(address)) {
      mb->addIreg(address->valueint, value->valueint);
    }
  }
  cleanup:
  cJSON_Delete(json);
  return true;
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
  //mb = new ModbusIP*();
  mb = new ModbusIP();
  mb->onConnect(connect);
  readDefaults();
  readModbus();
  mb->slave();
  mb->master();
  //mb->addHreg(10, 0, 5);
  //mb->onSetHreg(10, cb10, 5);
//  taskAdd(readRemote);
  //mb->connect(IPAddress(192,168,30,13));
  taskAdd(modbusLoop);
  return RUN_DELETE;
}

char* regTypeToStr(TAddress reg) {
  char* s = "COIL";
  if (reg.isHreg()) {
    s = "HREG";
  } else if (reg.isIreg()) {
    s = "IREG";
  } else if (reg.isIsts()) {
    s = "ISTS";
  }
  return s;
}

void cliPullHreg(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 5) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  IPAddress ip;
  ip.fromString(argv[1]);
  shell.println(addPull(ip, HREG(atoi(argv[2])), IREG(atoi(argv[3])), atoi(argv[4])));
}
ShellCommand(pullhreg, "<ip> <slave_hreg> <local_ireg> <interval> - Modbus: Pull slave Hreg to local Ireg", cliPullHreg);

void cliPullIreg(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 5) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  IPAddress ip;
  ip.fromString(argv[1]);
  shell.println(addPull(ip, IREG(atoi(argv[2])), IREG(atoi(argv[3])), atoi(argv[4])));
}
ShellCommand(pullireg, "<ip> <slave_ireg> <local_ireg> <interval> - Modbus: Pull Ireg", cliPullIreg);

void cliPullCoil(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 5) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  IPAddress ip;
  ip.fromString(argv[1]);
  shell.println(addPull(ip, COIL(atoi(argv[2])), ISTS(atoi(argv[3])), atoi(argv[4])));
}
ShellCommand(pullcoil, "<ip> <slave_coil> <local_ists> <interval> - Modbus: Pull slave Coil to local Ists", cliPullCoil);

void cliPushIsts(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 5) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  IPAddress ip;
  ip.fromString(argv[2]);
  shell.println(addPull(ip, COIL(atoi(argv[3])), ISTS(atoi(argv[1])), atoi(argv[4]), 1, false));
}
ShellCommand(pushists, "<local_ists> <ip> <slave_coil> <interval> - Modbus: Push local Ists to slave Coil", cliPushIsts);

void cliPushIreg(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 5) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  IPAddress ip;
  ip.fromString(argv[2]);
  shell.println(addPull(ip, HREG(atoi(argv[3])), IREG(atoi(argv[1])), atoi(argv[4]), 1, false));
}
ShellCommand(pushireg, "<local_ireg> <ip> <slave_hreg> <interval> - Modbus: Push local Ireg to slave Hreg", cliPushIreg);

void cliPullSave(Shell &shell, int argc, const ShellArguments &argv) {
  shell.println(saveModbus()?"Done":"Error");
}
ShellCommand(pullsave, "- Modbus: Save Pull/Push registers", cliPullSave);

void cliHreg(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 2) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  uint16_t reg = atoi(argv[1]);
  bool del = false;
  uint16_t val = 0;
  if (argv.count() > 2) {
    if (strcmp(argv[2], "delete") == 0) {
      mb->removeHreg(reg);
      del = true;
    } else {
      val = atoi(argv[2]);
      mb->addHreg(reg, val);
      mb->Hreg(reg, val);
    }
    if (argv.count() > 3 && strcmp(argv[3], "save") == 0) saveDefaults(HREG(reg), val, del);
  }
  shell.println(mb->Hreg(reg));
}
ShellCommand(hreg, "<hreg>[ <value>|delete][ save] - Modbus: Hreg get/set/add/delete", cliHreg);

void cliCoil(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 2) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  uint16_t reg = atoi(argv[1]);
  bool del = false;
  bool val = false;
  if (argv.count() > 2) {
    if (strcmp(argv[2], "delete") == 0) {
      mb->removeCoil(reg);
      del = true;
    } else {
      val = atoi(argv[2]) != 0;
      mb->addCoil(reg, val);
      mb->Coil(reg, val);
    }
    if (argv.count() > 3 && strcmp(argv[3], "save") == 0) saveDefaults(COIL(reg), val, del);
  }
  shell.println(mb->Coil(reg));
}
ShellCommand(coil, "<coil>[ <0|1>|delete][ save] - Modbus: Coil get/set/add/delete", cliCoil);

void cliIsts(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 2) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  uint16_t reg = atoi(argv[1]);
  bool del = false;
  bool val = false;
  if (argv.count() > 2) {
    if (strcmp(argv[2], "delete") == 0) {
      mb->removeIsts(reg);
      del = true;      
    } else {
      val = atoi(argv[2]) != 0;
      mb->addIsts(reg, val);
      mb->Ists(reg, val);
    }
    if (argv.count() > 3 && strcmp(argv[3], "save") == 0) saveDefaults(ISTS(reg), val, del);
  }
  shell.println(mb->Ists(reg));
}
ShellCommand(ists, "<ists>[ <0|1>|delete][ save] - Modbus: Ists get/set/add/remove", cliIsts);

void cliIreg(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 2) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  uint16_t reg = atoi(argv[1]);
  bool del = false;
  uint16_t val = false;
  if (argv.count() > 2) {
    if (strcmp(argv[2], "delete") == 0) {
      mb->removeIreg(reg);
      del = true;      
    } else {
      val = atoi(argv[2]);
      mb->addIreg(reg, val);
      mb->Ireg(reg, val);
    }
    if (argv.count() > 3 && strcmp(argv[3], "save") == 0) saveDefaults(IREG(reg), val, del);
  }
  shell.println(mb->Ireg(reg));
}
ShellCommand(ireg, "<ireg>[ <value>|delete][ save] - Modbus: Ireg get/set/add/remove", cliIreg);

void cliSlave(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 2) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  IPAddress ip;
  ip.fromString(argv[1]);
  if (mb->isConnected(ip)) {
    shell.printf_P(PSTR("Modbus: Already connected\n"));
  }
  if (!mb->connect(ip))
    shell.printf_P(PSTR("Modbus: Connection error\n"));
}
ShellCommand(slave, "<ip> - Modbus: Connect to slave", cliSlave);

void cliSlaveDisconnect(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 2) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  IPAddress ip;
  ip.fromString(argv[1]);
  if (!mb->isConnected(ip)) {
    shell.printf_P(PSTR("Modbus: Not connected\n"));
  }
  mb->disconnect(ip);
}
ShellCommand(slavedisconnect, "<ip> - Modbus: Disconnect from slave", cliSlaveDisconnect);

uint16_t dataRead;
bool cbReadCli(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  if (event != Modbus::EX_SUCCESS) {
    shell.printf_P(PSTR("Modbus error: 0x%02X\n"), event);
    return true;
  }
  shell.println(dataRead);
  return true;
}
void cliSlaveHreg(Shell &shell, int argc, const ShellArguments &argv) {
  IPAddress ip;
  if (argc < 2) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  ip.fromString(argv[1]);
  if (argc > 3) {
    dataRead = atoi(argv[3]);
    mb->writeHreg(ip, atoi(argv[2]), atoi(argv[3]), cbReadCli);
    return;
  }
  mb->readHreg(ip, atoi(argv[2]), &dataRead, 1, cbReadCli);
}
ShellCommand(slavehreg, "<ip> <hreg>[ <value>] - Modbus: Read/Write slave Hreg", cliSlaveHreg);
void cliSlaveIreg(Shell &shell, int argc, const ShellArguments &argv) {
  IPAddress ip;
  if (argc < 2) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  ip.fromString(argv[1]);
  if (!mb->readIreg(ip, atoi(argv[2]), &dataRead, 1, cbReadCli)) {
    shell.printf_P("Modbus: read error\n");
  }
}
ShellCommand(slaveireg, "<ip> <ireg> - Modbus: Read slave Ireg", cliSlaveIreg);
void cliSlaveCoil(Shell &shell, int argc, const ShellArguments &argv) {
  IPAddress ip;
  if (argc < 2) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  ip.fromString(argv[1]);
  if (argc > 3) {
    dataRead = atoi(argv[3]);
    if (mb->writeCoil(ip, atoi(argv[2]), atoi(argv[3])!=0, cbReadCli)) {
      shell.println("Not connected");
      return;
    }
  }
  if (!mb->readCoil(ip, atoi(argv[2]), (bool*)&dataRead, 1, cbReadCli)) {
    shell.println("Not connected");
  }
}
ShellCommand(slavecoil, "<ip> <coil>[ <0|1>] - Modbus: Read/Write slave Coil", cliSlaveCoil);
void cliSlaveIsts(Shell &shell, int argc, const ShellArguments &argv) {
  IPAddress ip;
  if (argc < 2) {
    shell.printf_P(PSTR("Wrong arguments\n"));
    return;
  }
  ip.fromString(argv[1]);
  mb->readIsts(ip, atoi(argv[2]), (bool*)&dataRead, 1, cbReadCli);
}
ShellCommand(slaveists, "<ip> <ists> - Modbus: Read slave Ists", cliSlaveIsts);

void cliPullList(Shell &shell, int argc, const ShellArguments &argv) {
  shell.printf_P(PSTR("ID\t Address\tReg\t\tCount\t\tLocal\t\tInterval\n"));
  for(auto const& item: regs) {
    shell.printf_P(PSTR("%d\t%s%d.%d.%d.%d\t\%s\t%d\t%d\t %s \t%s\t%d\t%d\t'%s\n"),
                        item.taskId, (mb->isConnected(item.ip)?"*":" "),
                        item.ip[0], item.ip[1], item.ip[2], item.ip[3],
                        regTypeToStr(item.remote), item.remote.address, item.count, item.pull?"=>":"<=", regTypeToStr(item.reg), item.reg.address, item.interval, item.name.c_str());
  }
}
ShellCommand(pulllist, "- Modbus: List slave Pulls/Pushs", cliPullList);
