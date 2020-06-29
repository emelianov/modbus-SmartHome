#pragma once
#include <Run.h>
#include <ModbusIP_ESP8266.h>
#include <arduino_homekit_server.h>

#define CFG_HOMEKIT "/homekit.json"
#define MDNS_ANNOUNCE 5000
#define HOMEKIT_MAX_DEV 8
#define HOMEKIT_TEMP "Temperature"
#define HOMEKIT_LAMP "Lamp"

extern ModbusIP* mb;

typedef homekit_value_t (*hkGetter)();
typedef void (*hkSetter)(homekit_value_t);

extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t name;
extern "C" homekit_characteristic_t serial_number;
extern "C" void led_toggle();
extern "C" void accessory_init();
extern "C" void service_init();
extern "C" void homekit_server_reset();
extern "C" homekit_characteristic_t* addCT(char* name);
extern "C" homekit_characteristic_t* addON(char* name, void (*s)(homekit_value_t));
extern "C" homekit_characteristic_t* addTT(char* name, homekit_characteristic_t** ch,
                                            void (*s2)(homekit_value_t), homekit_characteristic_t** ch2,
                                            homekit_characteristic_t** ch3,
                                            void (*s4)(homekit_value_t), homekit_characteristic_t** ch4);

struct hk_map_t {
  TAddress reg = IREG(0);
  int8_t shift = 1;
  homekit_characteristic_t* ch = NULL;
};

hk_map_t hkMap[HOMEKIT_MAX_DEV];
uint8_t hkMapCount = 0;

float shift10(float t, int8_t s) {
  if (s == 0) return t;
  while (s) {
    t /= 10;
    s--;
  }
  return t;
}

int unshift10(float t, int8_t s) {
  if (s == 0) return t;
  while (s) {
    t *= 10;
    s--;
  }
  if (t > 32768)
    t = 32768;
  else if (t < -32767)
    t -32767;
  return t;
}

template <int N>
homekit_value_t temp_on_get() {
  int16_t t = 0;
  switch (hkMap[N].reg.type) {
  case TAddress::HREG:
    t = mb->Hreg(hkMap[N].reg.address);
    return HOMEKIT_FLOAT_CPP(shift10((int16_t)t, hkMap[N].shift));
  case TAddress::IREG:
    t = mb->Ireg(hkMap[N].reg.address);
    return HOMEKIT_FLOAT_CPP(shift10((int16_t)t, hkMap[N].shift));
  case TAddress::COIL:
    return HOMEKIT_BOOL_CPP(mb->Coil(hkMap[N].reg.address));
  case TAddress::ISTS:
    return HOMEKIT_BOOL_CPP(mb->Ists(hkMap[N].reg.address));
  }
  return HOMEKIT_NULL_CPP();
}

template <int N>
homekit_value_t int_on_get() {
  int16_t t = 0;
  switch (hkMap[N].reg.type) {
  case TAddress::HREG:
    t = mb->Hreg(hkMap[N].reg.address);
    return HOMEKIT_FLOAT_CPP(shift10((int16_t)t, hkMap[N].shift));
  case TAddress::IREG:
    t = mb->Ireg(hkMap[N].reg.address);
    return HOMEKIT_FLOAT_CPP(shift10((int16_t)t, hkMap[N].shift));
  case TAddress::COIL:
    return HOMEKIT_BOOL_(mb->Coil(hkMap[N].reg.address));
  case TAddress::ISTS:
    return HOMEKIT_BOOL_CPP(mb->Ists(hkMap[N].reg.address));
  }
  return HOMEKIT_NULL_CPP();
}

template <int N>
void temp_on_set(homekit_value_t v) {
  int16_t r = 0;
  switch (hkMap[N].reg.type) {
  case TAddress::COIL:
    if (v.format != homekit_format_bool) return;
    hkMap[N].ch->value = HOMEKIT_BOOL_CPP(v.bool_value);
    mb->Coil(hkMap[N].reg.address, v.bool_value);
  break;
  case TAddress::ISTS:
    if (v.format != homekit_format_bool) return;
    hkMap[N].ch->value = HOMEKIT_BOOL_CPP(v.bool_value);
    mb->Ists(hkMap[N].reg.address, v.bool_value);
  break;
  case TAddress::IREG:
    if (v.format == homekit_format_bool) {
      hkMap[N].ch->value = HOMEKIT_BOOL_CPP(v.bool_value);
      r = v.bool_value;
    }
    else if (v.format == homekit_format_float) {
      //int8_t s = 
      r = unshift10(v.float_value, hkMap[N].shift);
    }
    else if (v.format == homekit_format_int) {
      r = v.int_value;
      hkMap[N].ch->value = HOMEKIT_INT_CPP(v.int_value);
    }
    else
      break;
    mb->Ists(hkMap[N].reg.address, r);
  break;
  case TAddress::HREG:
    if (v.format == homekit_format_bool) {
      r = v.bool_value;
      hkMap[N].ch->value = HOMEKIT_BOOL_CPP(v.bool_value);
    }
    else if (v.format == homekit_format_float) {
      r = unshift10(v.float_value, hkMap[N].shift);
    }
    else if (v.format == homekit_format_int) {
      r = v.int_value;
      hkMap[N].ch->value = HOMEKIT_INT_CPP(v.int_value);
    }
    else
      break;
    mb->Hreg(hkMap[N].reg.address, r);
  break;
  }
}

hkGetter hkMapGetter(uint8_t idx) {
  switch (idx) {
  case 0:
    return temp_on_get<0>;
  case 1:
    return temp_on_get<1>;
  case 2:
    return temp_on_get<2>;
  case 3:
    return temp_on_get<3>;
  case 4:
    return temp_on_get<4>;
  case 5:
    return temp_on_get<5>;
  case 6:
    return temp_on_get<6>;
  case 7:
    return temp_on_get<7>;
  }
}

hkSetter hkMapSetter(uint8_t idx) {
  switch (idx) {
  case 0:
    return temp_on_set<0>;
  case 1:
    return temp_on_set<1>;
  case 2:
    return temp_on_set<2>;
  case 3:
    return temp_on_set<3>;
  case 4:
    return temp_on_set<4>;
  case 5:
    return temp_on_set<5>;
  case 6:
    return temp_on_set<6>;
  case 7:
    return temp_on_set<7>;
  }
}

uint16_t onRegInteger(TRegister* reg, uint16_t val) {
  if (reg->address.isIreg() && mb->Ireg(reg->address.address) == val) return val; // Skip if no change
  if (reg->address.isHreg() && mb->Hreg(reg->address.address) == val) return val; // Skip if no change
  uint8_t i;
  for (i = 0; i < HOMEKIT_MAX_DEV && hkMap[i].reg != reg->address; i++) ; // Find HomeKit mapping
  if (i >= HOMEKIT_MAX_DEV || !hkMap[i].ch) return val; // Skip if not found or no characteristic in mapping
  hkMap[i].ch->value = HOMEKIT_INT_CPP((int16_t)val);
  homekit_characteristic_notify(hkMap[i].ch, HOMEKIT_INT_CPP((int16_t)val));
  return val;
}

uint16_t onIreg(TRegister* reg, uint16_t val) {
  if (reg->address.isIreg() && mb->Ireg(reg->address.address) == val) return val; // Skip if no change
  if (reg->address.isHreg() && mb->Hreg(reg->address.address) == val) return val; // Skip if no change
  uint8_t i;
  for (i = 0; i < HOMEKIT_MAX_DEV && hkMap[i].reg != reg->address; i++) ; // Find HomeKit mapping
  if (i >= HOMEKIT_MAX_DEV || !hkMap[i].ch) return val; // Skip if not found or no characteristic in mapping
  hkMap[i].ch->value.float_value = shift10((int16_t)val, hkMap[i].shift);
  homekit_characteristic_notify(hkMap[i].ch, hkMap[i].ch->value);
  //homekit_characteristic_notify(hkMap[i].ch, HOMEKIT_FLOAT_CPP(shift10((int16_t)val, hkMap[i].shift)));
  return val;
}

bool addT(char* name, uint16_t ireg, int8_t shift = 1) {
  if (hkMapCount >= HOMEKIT_MAX_DEV) return false;
  if (!mb->addIreg(ireg)) return false;
  mb->onSetIreg(ireg, onIreg);
  hkMap[hkMapCount].reg = IREG(ireg);
  hkMap[hkMapCount].shift = shift;
  hkMap[hkMapCount].ch = addCT(name);
  hkMapCount++;
  return true;
}

bool addThermostat(char* name, uint16_t reg, int8_t shift = 1) {
  if (hkMapCount >= HOMEKIT_MAX_DEV) return false;
  if (!mb->addIreg(reg)) return false;
  if (!mb->addHreg(reg)) return false;
  if (!mb->addIreg(reg + 1)) return false;
  if (!mb->addHreg(reg + 1)) return false;
  mb->onSetIreg(reg, onIreg);
  mb->onSetHreg(reg, onIreg);
  mb->onSetIreg(reg + 1, onRegInteger);
  mb->onSetHreg(reg + 1, onRegInteger);
  
  hkMap[hkMapCount].reg = IREG(reg);  // Current T
  hkMap[hkMapCount].shift = shift;
  hkMap[hkMapCount + 1].reg = HREG(reg);  // Target T
  hkMap[hkMapCount + 1].shift = shift;
  reg++;
  hkMap[hkMapCount + 2].reg = IREG(reg);  // Current Mode
  hkMap[hkMapCount + 2].shift = shift;
  hkMap[hkMapCount + 3].reg = HREG(reg);  // Target Mode
  hkMap[hkMapCount + 3].shift = shift;

  addTT(name, &hkMap[hkMapCount].ch,
              hkMapSetter(hkMapCount + 1), &hkMap[hkMapCount + 1].ch,
              &hkMap[hkMapCount + 2].ch,
              hkMapSetter(hkMapCount + 3), &hkMap[hkMapCount + 3].ch);
              
  hkMapCount  += 4;
  return true;
}
                                            
uint16_t onCoil(TRegister* reg, uint16_t val) {
  if (mb->Coil(reg->address.address) == val) return val; // Skip if no change
  uint8_t i;
  for (i = 0; i < HOMEKIT_MAX_DEV && hkMap[i].reg != reg->address; i++) ; // Find HomeKit mapping
  if (i >= HOMEKIT_MAX_DEV || !hkMap[i].ch) return val; // Skip if not found or no characteristic in mapping
  homekit_characteristic_notify(hkMap[i].ch, HOMEKIT_BOOL_CPP(COIL_BOOL(val)));
  return val;
}

bool addO(char* name, uint16_t coil, int8_t shift = 1) {
  if (hkMapCount >= HOMEKIT_MAX_DEV) return false;
  if (!mb->addCoil(coil)) return false;
  mb->onSetCoil(coil, onCoil);
  hkMap[hkMapCount].reg = COIL(coil);
  hkMap[hkMapCount].shift = shift;
  hkMap[hkMapCount].ch = addON(name, hkMapSetter(hkMapCount));
  hkMapCount++;
  return true;
}

uint32_t next_heap_millis = 0;

uint32_t homekit_loop();

uint32_t homekit_mdns() {
  MDNS.announce();
  return MDNS_ANNOUNCE;
}

bool readHomekit();
uint32_t homekitInit() {
  //hkMap = malloc(sizeof(hk_map_t));
  //hkMap[0] = 
  service_init();
  //addON("Led", led_on_get, led_on_set);
  //addT("Temperature", 1);
  readHomekit();
  //addThermostat("Heater", 10, 2);
  accessory_init();
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  int name_len = snprintf(NULL, 0, "%s_%02X%02X%02X", name.value.string_value, mac[3], mac[4], mac[5]);
  char *name_value = (char*)malloc(name_len + 1);
  snprintf(name_value, name_len + 1, "%s_%02X%02X%02X", name.value.string_value, mac[3], mac[4], mac[5]);
  name.value = HOMEKIT_STRING_CPP(name_value);

  arduino_homekit_setup(&config);
  taskAdd(homekit_loop);
  taskAdd(homekit_mdns);
  return RUN_DELETE;
}

uint32_t homekit_loop() {
  arduino_homekit_loop();
  uint32_t time = millis();
  if (time > next_heap_millis) {
    INFO("heap: %d, sockets: %d", ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
    next_heap_millis = time + 10000;
  }
  return 100;
  //return 5;
}

void cliHKreset(Shell &shell, int argc, const ShellArguments &argv) {
  homekit_server_reset();
}
ShellCommand(hkreset, " - HomeKit: Reset pairings", cliHKreset);

char* regTypeToStr(TAddress reg);
void cliHKlist(Shell &shell, int argc, const ShellArguments &argv) {
  shell.printf("Reg\t\tShift\tType\tDescription\n");
  for (uint8_t i = 0; i < hkMapCount; i++)
    shell.printf("%s\t%d\t%d\t%s\t%s\n", regTypeToStr(hkMap[i].reg), hkMap[i].reg.address, hkMap[i].shift, hkMap[i].ch->type, hkMap[i].ch->description);
}
ShellCommand(hklist, " - HomeKit: List devices", cliHKlist);

bool addHomekit(const char* type, const char* name, TAddress reg, int16_t extra);

void cliHKlamp(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 2) {
    if (!addHomekit(HOMEKIT_LAMP, argv[2], COIL(atoi(argv[1])), 0))
      shell.printf("Error\n");
  }
}
ShellCommand(hklamp, "<Coil> <Name> - HomeKit: Map Coil to Switch", cliHKlamp);

void cliHKtemp(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 3) {
    if (!addHomekit(HOMEKIT_TEMP, argv[3], IREG(atoi(argv[1])), atoi(argv[2])))
      shell.printf("Error\n");
  }
}
ShellCommand(hktemp, "<Ireg> <shift> <Name> - HomeKit: Map Ireg to Temperature", cliHKtemp);



bool readHomekit() {
  cJSON* json = nullptr;
  cJSON* arr = nullptr;
  File configFile = SPIFFS.open(CFG_HOMEKIT, "r");
  if (configFile) {
    char* data = (char*)malloc(configFile.size() + 1);
    if (data) {
      if (configFile.read((uint8_t*)data, configFile.size()) == configFile.size()) {
        data[configFile.size()] = '/0';
        TDEBUG("Homekit: default %d bytes read\n", configFile.size());
        json = cJSON_Parse(data);
      }
      free(data);
    }
    configFile.close();
  }
  if (!json) return false;
  cJSON* address;
  cJSON* value;
  cJSON* shift;
  cJSON* entry;
  arr = cJSON_GetObjectItemCaseSensitive(json, HOMEKIT_TEMP);
  if (arr)
    cJSON_ArrayForEach(entry, arr) {
      address = cJSON_GetObjectItemCaseSensitive(entry, "address");
      value = cJSON_GetObjectItemCaseSensitive(entry, "name");
      shift = cJSON_GetObjectItemCaseSensitive(entry, "shift");
      if (value && address && shift && cJSON_IsString(value) && cJSON_IsNumber(address) && cJSON_IsNumber(shift)) {
        addT(value->valuestring, address->valueint, shift->valueint);
      }
    }
  arr = cJSON_GetObjectItemCaseSensitive(json, HOMEKIT_LAMP);
  if (arr)
    cJSON_ArrayForEach(entry, arr) {
      address = cJSON_GetObjectItemCaseSensitive(entry, "address");
      value = cJSON_GetObjectItemCaseSensitive(entry, "name");
      if (value && address && cJSON_IsString(value) && cJSON_IsNumber(address)) {
        addO(value->valuestring, address->valueint);
      }
  }
  cleanup:
  cJSON_Delete(json);
  return true;
}

bool addHomekit(const char* type, const char* name, TAddress reg, int16_t extra = 0) {
  cJSON* json = nullptr;
  cJSON* arr = nullptr;
  cJSON* address = nullptr;
  cJSON* value = nullptr;
  cJSON* entry = nullptr;
  cJSON* item = nullptr;
  bool update = false;
  bool result = false;

  File configFile = SPIFFS.open(CFG_HOMEKIT, "r");
  if (configFile) {
    char* data = (char*)malloc(configFile.size() + 1);
    if (data) {
      if (configFile.read((uint8_t*)data, configFile.size()) == configFile.size()) {
        data[configFile.size()] = '/0';
        TDEBUG("Homekit: default %d bytes read\n", configFile.size());
        json = cJSON_Parse(data);
      }
      free(data);
    }
    configFile.close();
  }
  if (!json)
    json = cJSON_CreateObject();
  if (!json) return false;
  arr = cJSON_GetObjectItemCaseSensitive(json, type);
  if (arr)
    cJSON_ArrayForEach(entry, arr) {  // Try to find and update entry
      address = cJSON_GetObjectItemCaseSensitive(entry, "address");
      value = cJSON_GetObjectItemCaseSensitive(entry, "name");
      if (value && address && cJSON_IsString(value) && strcmp(value->valuestring, name) == 0) {
        update = true;
      }
    }
   else {
    arr = cJSON_CreateArray();
    if (!arr) goto cleanup;
    cJSON_AddItemToObject(json, type, arr);
   }
    if (!update) {  // Append entry
      entry = cJSON_CreateObject();
      item = cJSON_CreateString(name);
      if (!item) goto cleanup;
      cJSON_AddItemToObject(entry, "name", item);
      item = cJSON_CreateNumber(reg.address);
      if (!item) goto cleanup;
      cJSON_AddItemToObject(entry, "address", item);
      if (strcmp(type, HOMEKIT_TEMP) == 0) {
        item = cJSON_CreateNumber(extra);
        if (!item) goto cleanup;
        cJSON_AddItemToObject(entry, "shift", item);
      }
      cJSON_AddItemToArray(arr, entry);
      entry = nullptr;
      update = true;
    }
    if (update) {
      configFile = SPIFFS.open(CFG_HOMEKIT, "w");
      if (configFile) {
        char* out = cJSON_Print(json);
        configFile.write((uint8_t*)out, strlen(out));
        free(out);
      }
      //configFile.close();
    }
  result = true;
  cleanup:
  if (configFile) configFile.close();
  if (entry) cJSON_Delete(entry);
  if (json) cJSON_Delete(json);
  return result;
}

uint32_t homeKitInit() {
  return RUN_DELETE;
}
