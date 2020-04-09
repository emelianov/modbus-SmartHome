#pragma once
#include <Run.h>
#include <arduino_homekit_server.h>

extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t name;
extern "C" void led_toggle();
extern "C" void accessory_init();
extern "C" void homekit_server_reset();

uint32_t next_heap_millis = 0;
uint32_t homekit_loop();

uint32_t homekitInit() {
  accessory_init();
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  int name_len = snprintf(NULL, 0, "%s_%02X%02X%02X", name.value.string_value, mac[3], mac[4], mac[5]);
  char *name_value = (char*)malloc(name_len + 1);
  snprintf(name_value, name_len + 1, "%s_%02X%02X%02X", name.value.string_value, mac[3], mac[4], mac[5]);
  name.value = HOMEKIT_STRING_CPP(name_value);

  arduino_homekit_setup(&config);
  taskAdd(homekit_loop); 
  return RUN_DELETE;
}

uint32_t homekit_loop() {
  MDNS.announce();
  arduino_homekit_loop();
  uint32_t time = millis();
  if (time > next_heap_millis) {
    INFO("heap: %d, sockets: %d", ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
    next_heap_millis = time + 10000;
  }
  return 5;
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
