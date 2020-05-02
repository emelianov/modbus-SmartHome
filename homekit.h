#pragma once
#include <Run.h>
#include <ModbusIP_ESP8266.h>
#include <arduino_homekit_server.h>

#define MDNS_ANNOUNCE 5000
#define HOMEKIT_MAX_DEV 8

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
extern "C" homekit_characteristic_t* addCT(char* name, homekit_value_t (*f)());
extern "C" homekit_characteristic_t* addON(char* name, homekit_value_t (*g)(), void (*s)(homekit_value_t));

struct hk_map_t {
  TAddress reg = IREG(0);
  int8_t shift = 1;
  //char* name = "";
  homekit_characteristic_t* ch = NULL;
};

hk_map_t hkMap[HOMEKIT_MAX_DEV];
uint8_t hkMapCount = 0;

template <int N>
homekit_value_t temp_on_get() {
  int16_t t = 0;
  switch (hkMap[N].reg.type) {
  case TAddress::HREG:
    t = mb->Hreg(hkMap[N].reg.address);
    return HOMEKIT_FLOAT_CPP( (hkMap[N].shift != 0)?((float)t / hkMap[N].shift):(float)t );
  case TAddress::IREG:
    t = mb->Ireg(hkMap[N].reg.address);
    return HOMEKIT_FLOAT_CPP( (hkMap[N].shift != 0)?((float)t / hkMap[N].shift):(float)t );
  case TAddress::COIL:
    return HOMEKIT_BOOL_CPP(mb->Coil(hkMap[N].reg.address));
  case TAddress::ISTS:
    return HOMEKIT_BOOL_CPP(mb->Ists(hkMap[N].reg.address));
  }
  return HOMEKIT_NULL_CPP();
}

template <int N>
void temp_on_set(homekit_value_t v) {
  switch (hkMap[N].reg.type) {
  case TAddress::COIL:
    if (v.format != homekit_format_bool) return;
    mb->Coil(hkMap[N].reg.address, v.bool_value);
  break;
  case TAddress::ISTS:
    if (v.format != homekit_format_bool) return;
    mb->Ists(hkMap[N].reg.address, v.bool_value);
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

uint16_t onIreg(TRegister* reg, uint16_t val) {
  if (mb->Ireg(reg->address.address) == val) return val; // Skip if no change
  uint8_t i;
  for (i = 0; i < HOMEKIT_MAX_DEV && hkMap[i].reg != reg->address; i++) ; // Find HomeKit mapping
  if (i >= HOMEKIT_MAX_DEV || !hkMap[i].ch) return val; // Skip if not found or no characteristic in mapping
  homekit_characteristic_notify(hkMap[i].ch, HOMEKIT_FLOAT_CPP((hkMap[i].shift != 0)?(float)val / hkMap[i].shift:(float)val));
  return val;
}

bool addT(char* name, uint16_t ireg, int8_t shift = 1) {
  if (hkMapCount >= HOMEKIT_MAX_DEV) return false;
  if (!mb->addIreg(ireg)) return false;
  mb->onSetIreg(ireg, onIreg);
  hkMap[hkMapCount].reg = IREG(ireg);
  hkMap[hkMapCount].shift = shift;
  hkMap[hkMapCount].ch = addCT(name, hkMapGetter(hkMapCount));
  hkMapCount++;
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
  hkMap[hkMapCount].ch = addON(name, hkMapGetter(hkMapCount), hkMapSetter(hkMapCount));
  hkMapCount++;
  return true;
}

uint32_t next_heap_millis = 0;

uint32_t homekit_loop();

uint32_t homekit_mdns() {
  MDNS.announce();
  return MDNS_ANNOUNCE;
}

uint32_t homekitInit() {
  //hkMap = malloc(sizeof(hk_map_t));
  //hkMap[0] = 
  service_init();
  //addON("Led", led_on_get, led_on_set);
  addT("Temperature", 1);
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

void cliHKmap(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 1) {
  }
}
ShellCommand(hkmap, "<Coil> <Name> - HomeKit: Map Coil to Switch", cliHKmap);

void cliHKtemp(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 1) {
  }
}
ShellCommand(hktemp, "<Ireg> <Name> - HomeKit: Map Ireg to Temperature", cliHKmap);

uint32_t homeKitInit() {
  return RUN_DELETE;
}
