//////////////////////////////////////////////////////
// ESP8266/ESP32 Relay Module
// (c)2018, a.m.emelianov@gmail.com
//

#pragma once

#define DEFAULT_NAME "MR1"
#define DEFAULT_PASSWORD "P@ssw0rd"
#define DEVICE_INTERVAL 10000

 #define OW_PIN D2

String sysName = DEFAULT_NAME;
String sysPassword = DEFAULT_PASSWORD;

#include <FS.h>
#include <Run.h>
#define CJSON_NESTING_LIMIT 10
#include <cJSON.h>
#include <DallasTemperature.h>
#include <detail/RequestHandlersImpl.h> // for StaticRequestHandler::getContentType(path);

#define DEVICE_RESPONSE_WAIT 250

#define CFG_SENSORS "/sensors.json"
#define DEVICE_MAX_COUNT 16

struct sensor {
  DeviceAddress device;
  String        name;
  float         C;        // Last temperature query result
  float         Ct;       // Hysteresis base
  float         H;        // Hysteresis delta
  bool          AF;       // Hysteresis mode
  int16_t       pin;      // Affecred PIN
  uint32_t      age;      // Last success quiry time
  String addressToString() {
    char szRet[24];
    sprintf(szRet,"%02X%02X%02X%02X%02X%02X%02X%02X", device[0], device[1], device[2], device[3], device[4], device[5], device[6], device[7]);
    return String(szRet);
  }
  bool addressFromString(String str) {
    for (uint8_t j = 0; j < 8; j++)
      device[j] = strtol(str.substring(j*2, j*2+2).c_str(), NULL, 16);
  }
};

OneWire * oneWire = NULL;
DallasTemperature * sensors = NULL;
sensor sens[DEVICE_MAX_COUNT];
int16_t oneWirePin = OW_PIN;
uint32_t queryInterval = DEVICE_INTERVAL - DEVICE_RESPONSE_WAIT;

// Scan in-memory table for device with address
int16_t deviceFind(DeviceAddress address) {
  for (uint8_t i = 0; i < DEVICE_MAX_COUNT; i++) {
    if (memcmp(sens[i].device, address, sizeof(DeviceAddress)) == 0) {
      return i;
    }
  }
  return -1;
}

uint32_t readTSensorsResponse();

// Query sensors to prepare data
uint32_t readTSensors() {
  sensors->requestTemperatures();
  taskAddWithDelay(readTSensorsResponse, DEVICE_RESPONSE_WAIT);
  return RUN_DELETE;  
}

// Get temperature query responce
uint32_t readTSensorsResponse() {
 uint8_t i;
 DeviceAddress zerro;
 memset(zerro, 0, sizeof(DeviceAddress));
  for (i = 0; i < DEVICE_MAX_COUNT; i++) {
   if (memcmp(sens[i].device, zerro, sizeof(DeviceAddress)) != 0) {
    float t = sensors->getTempC(sens[i].device);
    if (t !=  DEVICE_DISCONNECTED_C) {
      sens[i].C = t;
      sens[i].age = millis();
      if (sens[i].Ct != DEVICE_DISCONNECTED_C) {
        double delta = abs(sens[i].C - sens[i].Ct);
        if ((sens[i].AF && delta > sens[i].H / 2) || (!sens[i].AF && delta < sens[i].H / 2)) {
         // digitalWrite(sens[i].pin, HIGH);
        } else {
         // digitalWrite(sens[i].pin, LOW);
        }
      }
    }
   }
  }
  taskAddWithDelay(readTSensors, queryInterval); 
  return RUN_DELETE;
}

// Scan 1-Wire bus for connected devices and fill in-memory table
bool scanSensors() {
  bool newDeviceAdded = false;
  for (uint8_t i = 0; i < sensors->getDeviceCount(); i++) {
    DeviceAddress deviceFound;
    sensors->getAddress(deviceFound, i);
    int16_t j = deviceFind(deviceFound);
    if (j != -1) {
      sens[i].age = millis();
    } else {
      DeviceAddress zerro;
      memset(zerro, 0, sizeof(DeviceAddress));
      for (j = 0; j < DEVICE_MAX_COUNT; j++) {
        if (memcmp(sens[j].device, zerro, sizeof(DeviceAddress)) == 0) {
          memcpy(sens[j].device, deviceFound, sizeof(DeviceAddress));
          sens[j].name = "New device " + String(i);
          newDeviceAdded = true;
          break;
        }
      }
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
      for (uint8_t i = 0; i < DEVICE_MAX_COUNT; i++) {
        cJSON* item;
        cJSON* entry = cJSON_CreateObject();
        if (!entry) continue;
        item = cJSON_CreateString(sens[i].addressToString().c_str());
        if (item) cJSON_AddItemToObject(entry, "address", item);
        item = cJSON_CreateString(sens[i].name.c_str());
        if (item) cJSON_AddItemToObject(entry, "name", item);
        item = cJSON_CreateNumber(sens[i].Ct);
        if (item) cJSON_AddItemToObject(entry, "Ct", item);
        item = cJSON_CreateNumber(sens[i].H);
        if (item) cJSON_AddItemToObject(entry, "H", item);
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
  for (i = 0; i < DEVICE_MAX_COUNT; i++) {
    sens[i].C = DEVICE_DISCONNECTED_C;
    sens[i].Ct = DEVICE_DISCONNECTED_C;
    sens[i].H = DEVICE_DISCONNECTED_C;;
    sens[i].name = String(i);
    sens[i].pin = -1;
    sens[i].age = 0;
    memset(sens[i].device, 0, sizeof(DeviceAddress));    //Fill device id with 0
  }
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
        cJSON_ArrayForEach(entry, arr) {
          if (i > DEVICE_MAX_COUNT) break;
          Serial.println(i);
          cJSON* item = NULL;
          if (cJSON_HasObjectItem(entry, "address")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "address");
            if (item && cJSON_IsString(item))
              sens[i].addressFromString(item->valuestring);
          }
          if (cJSON_HasObjectItem(entry, "name")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "name");
            if (item && cJSON_IsString(item))
              sens[i].name = item->valuestring;
          }
          if (cJSON_HasObjectItem(entry, "Ct")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "Ct");
            if (item && cJSON_IsNumber(item))
              sens[i].Ct = item->valuedouble;
          }
          if (cJSON_HasObjectItem(entry, "H")) {
            item = cJSON_GetObjectItemCaseSensitive(entry, "H");
            if (item && cJSON_IsNumber(item))
              sens[i].H = item->valuedouble;
          }
          i++;
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
  for (uint8_t i = 0; i < DEVICE_MAX_COUNT; i++) {
    sens[i].age = 0;
  }
  scanSensors();
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
      for (uint8_t i = 0; i < DEVICE_MAX_COUNT; i++) {
        //cJSON* item;
        cJSON* entry = cJSON_CreateObject();
        if (!entry) continue;
        item = cJSON_CreateString(sens[i].addressToString().c_str());
        if (item) cJSON_AddItemToObject(entry, "address", item);
        item = cJSON_CreateString(sens[i].name.c_str());
        if (item) cJSON_AddItemToObject(entry, "name", item);
        item = cJSON_CreateNumber(sens[i].C);
        if (item) cJSON_AddItemToObject(entry, "C", item);
        item = cJSON_CreateNumber(sens[i].Ct);
        if (item) cJSON_AddItemToObject(entry, "Ct", item);
        item = cJSON_CreateNumber(sens[i].H);
        if (item) cJSON_AddItemToObject(entry, "H", item);
        item = cJSON_CreateNumber(sens[i].AF);
        if (item) cJSON_AddItemToObject(entry, "AF", item);
        item = cJSON_CreateNumber(sens[i].pin);
        if (item) cJSON_AddItemToObject(entry, "pin", item);
        item = cJSON_CreateNumber(sens[i].age);
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
  server->send(500, "text/plain", "Error");
}

// Convert F to C
double FtoC(double F) {
  return roundf((F - 32) / 1.8 * 100) / 100;
}

// Convert C to F
double CtoF(double C) {
  return C * 1.8 + 32;
}

// Parse incoming get query URL to comply access parameters and return sensor index
int16_t getSensorFromURL() {
  uint8_t i = -1;
  if (!oneWire || !sensors) {
    server->send(500, "text/plain", "Error – no 1Wire pin is configured on this board: " + sysName);
  }
  if (!server->hasArg("name") || server->arg("name") != sysName) {
    server->send(403, "text/plain", "Error - Access dinided: " + sysName);
  }
  if (!server->hasArg("pp") || server->urlDecode(server->arg("pp")) != sysPassword) {
    server->send(403, "text/plain", "Error - Access denided: " + sysName);
  }
  if (!server->hasArg("u")) {
    server->send(404, "text/plain", "Error - Object not found: " + sysName);
  }
  String u = server->urlDecode(server->arg("u"));
  for (i = 0; i < DEVICE_MAX_COUNT && sens[i].name != u; i++) {
    
    //Nothing
  }
  if (i >= DEVICE_MAX_COUNT) {
    server->send(404, "text/plain", "Error – no such device /“" + u + "/” is configured  check spelling");
    i = -1;
  }
  return i;
}

// Respond read temperature query
void http1WireRead() {
  server->sendHeader("Connection", "close");
  server->sendHeader("Cache-Control", "no-store, must-revalidate");
Serial.println(ESP.getFreeHeap());
  String reply = "";
  uint8_t i = getSensorFromURL();
  if (i == -1) return;
  reply += sysName + ", " + sens[i].name;
  reply = "Confirmation, " + reply + ", tf=" + String(CtoF(sens[i].C)) + ", tc=" + String(sens[i].C);
  server->send(200, "text/plain", reply);
}

// Respond change sensor settings query
void http1WireSet() {
  server->sendHeader("Connection", "close");
  server->sendHeader("Cache-Control", "no-store, must-revalidate");

  String reply = "";
  double thf = DEVICE_DISCONNECTED_F, thc = DEVICE_DISCONNECTED_C, hyst = DEVICE_DISCONNECTED_C;

  uint8_t i = getSensorFromURL();
  if (i == -1) return;
  reply += sysName + ", " + sens[i].name;

  if (server->hasArg("thf") && server->hasArg("thc")) {
    server->send(409, "text/plain", "Error - Wrong parameters");
    return;          
  }
  if (server->hasArg("thf")) {
    thf = atof(server->arg("thf").c_str());
    sens[i].Ct = FtoC(thf);
    reply += ", thf=" + String(thf);
    if (server->hasArg("hyst")) {
      hyst = atof(server->arg("hyst").c_str());
      sens[i].H = FtoC(hyst);
      reply += ", hyst=" + String(hyst);
    }
  }
  if (server->hasArg("thc")) {
    thc = atof(server->arg("thc").c_str());
    sens[i].Ct = thc;
    reply += ", thc=" + String(thc);
    if (server->hasArg("hyst")) {
      hyst = atof(server->arg("hyst").c_str());
      sens[i].H = hyst;
      reply += ", hyst=" + String(hyst);
    }
  }
  reply = "Confirmation, " + reply;
  saveSensors();
  server->send(200, "text/plain", reply);
}

// Respond delete sensor query
// 1wiredelete?address=1122334455667788
void http1WireDelete() {
  server->sendHeader("Connection", "close");
  server->sendHeader("Cache-Control", "no-store, must-revalidate");
  if (!server->hasArg("address")) {
    server->send(500, "text/plain", "Error - Wrong parameters");
    return;
  }
  uint8_t i;
  sensor s;
  s.addressFromString(server->arg("address"));
  for (i = 0; i < DEVICE_MAX_COUNT && !(memcmp(sens[i].device, s.device, sizeof(DeviceAddress)) == 0); i++) {
    //Nothing
  }
  if (i >= DEVICE_MAX_COUNT) {
    server->send(403, "text/plain", "Error - Not found");
    return;    
  }
  memset(sens[i].device, 0, sizeof(DeviceAddress));    //Fill device id with 0
  saveSensors();
  server->send(200, "text/plain", "Confirmation - Ok");
}

// Respond modify sensor name query
// 1wiremodify?json={[{{address=1122334455667788},{name="new name"}},{...}]}
void http1WireModify() {
  server->sendHeader("Connection", "close");
  server->sendHeader("Cache-Control", "no-store, must-revalidate");
  if (!server->hasArg("json")) {
    server->send(500, "text/plain", "Error - Wrong parameters");
    return;
  }
  cJSON* root = cJSON_Parse(server->urlDecode(server->arg("json")).c_str());
  Serial.println(server->urlDecode(server->arg("json")));
  cJSON* entry;
  cJSON* item;
  cJSON* json = NULL;
  String name;
  bool          AF;       // Hysteresis mode
  int16_t       pin;      // Affected PIN
  if (root) {
    if (cJSON_HasObjectItem(root, "pin")) {
      item = cJSON_GetObjectItemCaseSensitive(root, "pin");
      if (item && cJSON_IsNumber(item))
        oneWirePin = item->valuedouble;
    }
    if (cJSON_HasObjectItem(root, "interval")) {
      item = cJSON_GetObjectItemCaseSensitive(root, "interval");
      Serial.println(100);
      if (item && cJSON_IsNumber(item)) {
        //Serial.println(item->valuedouble);
        queryInterval = item->valuedouble - DEVICE_RESPONSE_WAIT;
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
          s.addressFromString(item->valuestring);
          for (i = 0; i < DEVICE_MAX_COUNT && !(memcmp(sens[i].device, s.device, sizeof(DeviceAddress)) == 0); i++) {
            //Nothing
            //Serial.println(item->valuestring);
            //Serial.println(s.addressToString());
            //Serial.println(sens[i].addressToString());
          }
          Serial.println();
          if (i >= DEVICE_MAX_COUNT) {
            server->send(403, "text/plain", "Error - Not found");
            return;    
          }
          name = sens[i].name;
          AF = sens[i].AF;
          pin = sens[i].pin;
          item = cJSON_GetObjectItemCaseSensitive(entry, "name");
          if (item || cJSON_IsString(item))
            name = item->valuestring;
          item = cJSON_GetObjectItemCaseSensitive(entry, "AF");
          if (item || cJSON_IsNumber(item))
            AF = item->valuedouble;
          item = cJSON_GetObjectItemCaseSensitive(entry, "pin");
          if (item || cJSON_IsNumber(item))
           pin = item->valuedouble;
          sens[i].name = name;
          sens[i].AF = AF;
          sens[i].pin = pin;
        }
      //cJSON_Delete(json);
          saveSensors();
          server->send(200, "text/plain", "Confirmation - Ok");
      }
  }
  cJSON_Delete(root);
}

  bool handleFileRead(String path){
    if(path.endsWith("/")) path += "index.html";
    String contentType = StaticRequestHandler::getContentType(path);
    String pathWithGz = path + ".gz";
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    if(SPIFFS.exists(path)){
      server->sendHeader("Connection", "close");
      server->sendHeader("Cache-Control", "no-store, must-revalidate");
      server->sendHeader("Access-Control-Allow-Origin", "*");
      File file = SPIFFS.open(path, "r");
      size_t sent = server->streamFile(file, contentType);
      file.close();
      return true;
    }
    return false;
  }

void handleGenericFile() {
  if(!handleFileRead(server->uri()))
    server->send(404, "text/plain", "FileNotFound");
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
  server->on("/1wireread", http1WireRead);
  server->on("/1wireset", http1WireSet);
  server->on("/1wiredelete", http1WireDelete);
  server->on("/1wiremodify", http1WireModify);
  server->onNotFound(handleGenericFile);
  Serial.println("web inited");
  taskAdd(readTSensors);
  Serial.println(ESP.getFreeHeap());
  return RUN_DELETE;
}
