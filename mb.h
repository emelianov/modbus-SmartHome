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
/*
struct register {
  String name;
  bool pull;
}

// Read configuration of Modbus Push/Pull targets
bool readModbus() {
  int16_t i;
  File configFile = SPIFFS.open(CFG_MODBUS, "r");
  if (configFile) {
   char* data = (char*)malloc(configFile.size() + 1);
   if (data) {
    configFile.read((uint8_t*)data, configFile.size());
    data[configFile.size()] = '/0';
    Serial.println("Modbus: config reading complete");
    cJSON* json = cJSON_Parse(data);
    if (json) {
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
        remote reg;
        cJSON_ArrayForEach(entry, arr) {
          if (i > DEVICE_MAX_COUNT) break;
          i++;
          Serial.println(i);
          cJSON* item = NULL;
          if (cJSON_HasObjectItem(entry, "interval")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "interval");
            if (item && cJSON_IsNumber(item))
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
              reg.reg = IREG(item->valuedouble);
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
              dev.addressFromString(item->valuestring);
              Serial.print(dev.addressToString());
              regs.push_back(reg);
            }
          }
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
*/
uint32_t modbusInit() {
  mb = new ModbusIP();
  mb->onConnect(connect);
  mb->slave();
  mb->master();
  mb->connect(IPAddress(192,168,30,13));
  taskAdd(modbusLoop);
  return RUN_DELETE;
}

