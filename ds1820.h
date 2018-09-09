//////////////////////////////////////////////////////
// ESP8266/ESP32 Modbus Temperature sensor
// (c)2018, a.m.emelianov@gmail.com
//

#pragma once

#define DEVICE_INTERVAL 10000

#define OW_PIN D2

#include <FS.h>
#include <Run.h>
#define CJSON_NESTING_LIMIT 10
#include <cJSON.h>
#include <DallasTemperature.h>

#define DEVICE_RESPONSE_WAIT 250

#define CFG_SENSORS "/sensors.json"
#define DEVICE_MAX_COUNT 16

struct sensor {
  DeviceAddress device;
  String        name;
  float         C = DEVICE_DISCONNECTED_C;        // Last temperature query result
  float         Ct;       // Hysteresis base
  float         H;        // Hysteresis delta
  bool          AF;       // Hysteresis mode
  int16_t       pin = -1;      // Affected PIN
  uint32_t      age = 0;      // Last success quiry time
  String addressToString() {
    char szRet[24];
    sprintf(szRet,"%02X%02X%02X%02X%02X%02X%02X%02X", device[0], device[1], device[2], device[3], device[4], device[5], device[6], device[7]);
    return String(szRet);
  }
  bool addressFromString(String str) {
    for (uint8_t j = 0; j < 8; j++)
      device[j] = strtol(str.substring(j*2, j*2+2).c_str(), NULL, 16);
  }
  bool operator==(const DeviceAddress dev) const {
    char szRet[24];
    return (memcmp(device, dev, sizeof(DeviceAddress)) == 0);
  }
};

OneWire * oneWire = NULL;
DallasTemperature * sensors = NULL;
std::vector<sensor> sens;
int16_t oneWirePin = OW_PIN;
uint32_t queryInterval = DEVICE_INTERVAL - DEVICE_RESPONSE_WAIT;

// Scan in-memory table for device with address
sensor* deviceFind(DeviceAddress address) {
  for (sensor& dev: sens) if (dev == address) return &dev;
  return nullptr;
}

uint32_t readTSensorsResponse();

// Query sensors to prepare data
uint32_t readTSensors() {
  sensors->requestTemperatures();
  taskAddWithDelay(readTSensorsResponse, DEVICE_RESPONSE_WAIT);
  return RUN_DELETE;  
}


bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  Serial.print("Write: ");
  Serial.println(event);
  return true;
}
uint16_t rReg = 0;
bool cbReg(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  Serial.printf("Read: %d, Value: %d\n", event, rReg);
  return true;
}
// Get temperature query responce
uint32_t readTSensorsResponse() {
 uint8_t i;
  for (sensor& dev : sens) {
    float t = sensors->getTempC(dev.device);
    if (t !=  DEVICE_DISCONNECTED_C) {
      dev.C = t;
      dev.age = millis();
      if (dev.pin >= 0) {
        mb->Ireg(dev.pin, t * 100);
        if (!mb->isConnected(IPAddress(192,168,30,13))) {
          mb->connect(IPAddress(192,168,30,13));
        }
        mb->writeHreg(IPAddress(192,168,30,13), 1, t * 100, cbWrite);
        mb->writeHreg(IPAddress(192,168,30,13), 2, -100);
        mb->readHreg(IPAddress(192,168,30,13), 2, &rReg, 1, cbReg);
      }
    }
  }
  taskAddWithDelay(readTSensors, queryInterval); 
  return RUN_DELETE;
}

// Scan 1-Wire bus for connected devices and fill in-memory table
bool scanSensors() {
  bool newDeviceAdded = false;
  Serial.print("Sensor found: ");
  Serial.println(sensors->getDeviceCount());
  for (uint8_t i = 0; i < sensors->getDeviceCount(); i++) {
    DeviceAddress deviceFound;
    sensors->getAddress(deviceFound, i);
    sensor* s = deviceFind(deviceFound);
    if (!s) {
      s = new sensor;
      memcpy(s->device, deviceFound, sizeof(DeviceAddress));
      s->name = "New device";
      sens.push_back(*s);
      newDeviceAdded = true;
      break;
    }
  }
  return newDeviceAdded;
}

// Save in-memory sensor's table to file system
bool saveSensors() {
   File configFile = SPIFFS.open(F(CFG_SENSORS), "w");
   if (configFile) {
    cJSON* json = cJSON_CreateObject();
    cJSON* parm;
    parm = cJSON_CreateNumber(oneWirePin);
    if (parm) cJSON_AddItemToObject(json, "pin", parm);
    parm = cJSON_CreateNumber(queryInterval + DEVICE_RESPONSE_WAIT);
    if (parm) cJSON_AddItemToObject(json, "interval", parm);
    cJSON* arr = cJSON_CreateArray();
    if (arr) {
      for (sensor& dev : sens) {
        cJSON* item;
        cJSON* entry = cJSON_CreateObject();
        if (!entry) continue;
        item = cJSON_CreateString(dev.addressToString().c_str());
        if (item) cJSON_AddItemToObject(entry, "address", item);
        item = cJSON_CreateString(dev.name.c_str());
        if (item) cJSON_AddItemToObject(entry, "name", item);
        item = cJSON_CreateNumber(dev.pin);
        if (item) cJSON_AddItemToObject(entry, "pin", item);
//        item = cJSON_CreateNumber(dev.H);
//        if (item) cJSON_AddItemToObject(entry, "H", item);
        cJSON_AddItemToArray(arr, entry);
      }
      cJSON_AddItemToObject(json, "sensors", arr);
      char* out = cJSON_Print(json);
      configFile.write((uint8_t*)out, strlen(out));
      cJSON_Delete(json);
      free(out);
    }
    configFile.close();
    return true;
   }
   return false;  
}

// Read sensors from file system
bool readSensors() {
  int16_t i;
  File configFile = SPIFFS.open(CFG_SENSORS, "r");
  if (configFile) {
   char* data = (char*)malloc(configFile.size() + 1);
   if (data) {
    configFile.read((uint8_t*)data, configFile.size());
    data[configFile.size()] = '/0';
    Serial.println("config reading complete");
    cJSON* json = cJSON_Parse(data);
    Serial.println("data parsed");
    if (json) {
      cJSON* arr = NULL;
      cJSON* entry = NULL;
      cJSON* item;
      if (cJSON_HasObjectItem(json, "pin")) {
        item = cJSON_GetObjectItemCaseSensitive(json, "pin");
        if (item && cJSON_IsNumber(item))
          oneWirePin = item->valuedouble;
      }
      if (cJSON_HasObjectItem(json, "interval")) {
        item = cJSON_GetObjectItemCaseSensitive(json, "interval");
        if (item && cJSON_IsNumber(item))
          queryInterval = item->valuedouble - DEVICE_RESPONSE_WAIT;
      }
      i = 0;
      Serial.println("ready to get array");
      if (cJSON_HasObjectItem(json, "sensors")) arr = cJSON_GetObjectItemCaseSensitive(json, "sensors");
      Serial.println("get array");
      if (arr) {
        sensor dev;
        cJSON_ArrayForEach(entry, arr) {
          if (i > DEVICE_MAX_COUNT) break;
          i++;
          Serial.println(i);
          cJSON* item = NULL;
          if (cJSON_HasObjectItem(entry, "name")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "name");
            if (item && cJSON_IsString(item))
              dev.name = item->valuestring;
          }
          if (cJSON_HasObjectItem(entry, "pin")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "pin");
            if (item && cJSON_IsNumber(item))
              dev.pin = item->valuedouble;
              if (dev.pin >= 0) mb->addIreg(dev.pin);
          }
//          if (cJSON_HasObjectItem(entry, "H")) {
//            item = cJSON_GetObjectItemCaseSensitive(entry, "H");
//            if (item && cJSON_IsNumber(item))
//              dev.H = item->valuedouble;
//          }
          if (cJSON_HasObjectItem(entry, "address")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "address");
            if (item && cJSON_IsString(item)) {
              dev.addressFromString(item->valuestring);
              Serial.print(dev.addressToString());
              Serial.println("Push from config");
              sens.push_back(dev);
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

// Send in-memory sensor's table 
void jsonWire() {
//  for (sensor& dev : sens) {
//    dev.age = 0;
//  }
  if (scanSensors()) {
    saveSensors();
  }
  server->sendHeader("Connection", "close");
  server->sendHeader("Cache-Control", "no-store, must-revalidate");
  cJSON* root = cJSON_CreateObject();
  if (root) {
    cJSON* item;
    item = cJSON_CreateNumber(oneWirePin);
    if (item) cJSON_AddItemToObject(root, "pin", item);
    item = cJSON_CreateNumber(queryInterval + DEVICE_RESPONSE_WAIT);
    if (item) cJSON_AddItemToObject(root, "interval", item);
    cJSON* json = cJSON_CreateArray();
    if (json) {
      cJSON_AddItemToObject(root, "sensors", json);
      for (sensor& dev : sens) {
        //cJSON* item;
        cJSON* entry = cJSON_CreateObject();
        if (!entry) continue;
        item = cJSON_CreateString(dev.addressToString().c_str());
        if (item) cJSON_AddItemToObject(entry, "address", item);
        item = cJSON_CreateString(dev.name.c_str());
        if (item) cJSON_AddItemToObject(entry, "name", item);
        item = cJSON_CreateNumber(dev.C);
        if (item) cJSON_AddItemToObject(entry, "C", item);
        item = cJSON_CreateNumber(dev.Ct);
        if (item) cJSON_AddItemToObject(entry, "Ct", item);
        item = cJSON_CreateNumber(dev.H);
        if (item) cJSON_AddItemToObject(entry, "H", item);
        item = cJSON_CreateNumber(dev.AF);
        if (item) cJSON_AddItemToObject(entry, "AF", item);
        item = cJSON_CreateNumber(dev.pin);
        if (item) cJSON_AddItemToObject(entry, "pin", item);
        item = cJSON_CreateNumber(dev.age);
        if (item) cJSON_AddItemToObject(entry, "seen", item);
        cJSON_AddItemToArray(json, entry);
      }
      char* out = cJSON_Print(root);
      server->send(200, "application/json", out);
      cJSON_Delete(root);
      free(out);
      return;
    }
    cJSON_Delete(root);
  }
  server->send_P(500, "text/plain", PSTR("Error"));
}

// Respond modify sensor name query
// 1wiremodify?json={[{{address=1122334455667788},{name="new name"}},{...}]}
void http1WireModify() {
  cJSON* root = nullptr;
  cJSON* entry = nullptr;
  cJSON* item = nullptr;
  cJSON* json = nullptr;
  String name = "";
  bool          AF = false;       // Hysteresis mode
  int16_t       pin = 0;      // Affected PIN
  server->sendHeader("Connection", "close");
  server->sendHeader("Cache-Control", "no-store, must-revalidate");
  if (!server->hasArg("json")) {
    server->send_P(500, "text/plain", PSTR("Error - Wrong parameters"));
    goto cleanup;
  }
  root = cJSON_Parse(server->urlDecode(server->arg("json")).c_str());
  if (!root) {
    server->send_P(500, "text/plain", PSTR("Error - Wrong parameters"));
    goto cleanup;
  }
  Serial.println(server->urlDecode(server->arg("json")));
  if (cJSON_HasObjectItem(root, "pin")) { // Assing 1-Wire pin
    item = cJSON_GetObjectItemCaseSensitive(root, "pin");
    if (item && cJSON_IsNumber(item)) oneWirePin = item->valuedouble;
  }
  if (cJSON_HasObjectItem(root, "interval")) {  // Set query interval
    item = cJSON_GetObjectItemCaseSensitive(root, "interval");
    if (item && cJSON_IsNumber(item)) queryInterval = item->valuedouble - DEVICE_RESPONSE_WAIT;
  }
  if (cJSON_HasObjectItem(root, "delete")) {  // Delete sensor
    item = cJSON_GetObjectItemCaseSensitive(root, "delete");
    if (item && cJSON_IsString(item)) {
       //= item->valuestring;
    }
  }
  if (cJSON_HasObjectItem(root, "sensors")) json = cJSON_GetObjectItem(root, "sensors");
    if (json) {
      cJSON_ArrayForEach(entry, json) {
        item = cJSON_GetObjectItemCaseSensitive(entry, "address");
        if (!item || !cJSON_IsString(item))
          continue;
        uint8_t i;
        sensor s;
        sensor *t;
        s.addressFromString(item->valuestring);
        t = deviceFind(s.device);
        Serial.println();
        if (!t) {
          server->send(403, "text/plain", "Error - Not found");
          goto cleanup;
        }
        name = t->name;
        AF = t->AF;
        pin = t->pin;
        item = cJSON_GetObjectItemCaseSensitive(entry, "name");
        if (item || cJSON_IsString(item)) name = item->valuestring;
        item = cJSON_GetObjectItemCaseSensitive(entry, "AF");
        if (item || cJSON_IsNumber(item)) AF = item->valuedouble;
        item = cJSON_GetObjectItemCaseSensitive(entry, "pin");
        if (item || cJSON_IsNumber(item)) pin = item->valuedouble;
        t->name = name;
        t->AF = AF;
        t->pin = pin;
        mb->addIreg(pin);
      }
    //cJSON_Delete(json);
      saveSensors();
      server->send(200, "text/plain", "Confirmation - Ok");
    }
  cleanup:
  if (root) cJSON_Delete(root);
  return;
}

// Initialize
uint32_t dsInit() {
  Serial.println(ESP.getFreeHeap());
  oneWire = new OneWire(OW_PIN);
  sensors = new DallasTemperature(oneWire);
  Serial.println("obj created");
  sensors->begin();
  sensors->setResolution(12);
  sensors->setWaitForConversion(false);
  SPIFFS.begin();
  readSensors();
  Serial.println("config read");
  if (scanSensors) {
    saveSensors();
  }
  Serial.println("bus scanned");
  server->on("/wire", jsonWire);
  server->on("/1wiremodify", http1WireModify);
  Serial.println("web inited");
  taskAdd(readTSensors);
  Serial.println(ESP.getFreeHeap());
  return RUN_DELETE;
}
