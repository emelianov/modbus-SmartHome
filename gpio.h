#pragma once

#define CFG_GPIO "/gpio.json"
#define GPIO_MAX_COUNT 16
#define GPIO_MAX_LOCAL_ISTS 16

extern ModbusIP<EthernetClient, EthernetServer>* mb;

struct gpio_t {
  String name = "";
  uint8_t mode = INPUT;
  uint8_t pin = 0;
  bool inverse = false;
  TAddress reg;
  uint16_t* flag = nullptr;
};

std::vector<gpio_t> gpios;
uint16_t click = 50;
uint16_t press = 250;
uint16_t longPress = 2000;
uint16_t veryLong = 10000;
enum swType {DIRECT, CLICK, PRESS, DOUBLE_CLICK, LONG_PRESS, TRIPLE_CLICK, VERY_LONG_PRESS};
struct intrPress {
  uint16_t flag = 0;
  uint32_t timeStamp = 0;
};

uint16_t ists[GPIO_MAX_LOCAL_ISTS];
/*
template <I>
void gpioHandler() {
  uint8_t state = digitalRead(ists[I]);
  if (state == LOW) {
    ists[I].timeStamp = millis();
    return;
  }
  timeStamp = millis() - timeStamp;
  ists[I].flag++;
};

template <I>
void gpioDirect() {
  uint8_t state = digitalRead(ists[I]);
  if (state == LOW) {
    ists[I].timeStamp = millis();
    return;
  }
  timeStamp = millis() - timeStamp;
  ists[I].flag++;
};
*/

uint16_t cbDigitalRead(TRegister* reg, uint16_t val) {
  std::vector<gpio_t>::iterator it = std::find_if(gpios.begin(), gpios.end(), [reg](const gpio_t& d) {return d.reg == reg->address;});
  if (it != gpios.end()) {
    switch (it->reg.type) {
    case TAddress::HREG:
      return 0;
    break;
    case TAddress::IREG:
      return 0;
    break;
    case TAddress::COIL:
      return COIL_VAL(digitalRead(it->pin) == HIGH);
    break;
    case TAddress::ISTS:
      return ISTS_VAL(digitalRead(it->pin) == HIGH);
    break;
    }
  }
  return COIL_VAL(false);
}

uint16_t cbDigitalWrite(TRegister* reg, uint16_t val) {
  std::vector<gpio_t>::iterator it = std::find_if(gpios.begin(), gpios.end(), [reg](const gpio_t& d) {return d.reg == reg->address;});
  if (it != gpios.end()) {
    switch (it->reg.type) {
    case TAddress::HREG:
      return 0;
    break;
    case TAddress::IREG:
      return 0;
    break;
    case TAddress::COIL:
      digitalWrite(it->pin, COIL_BOOL(val)?HIGH:LOW);
    break;
    case TAddress::ISTS:
      digitalWrite(it->pin, ISTS_BOOL(val)?HIGH:LOW);
    break;
    }
  }
  return COIL_VAL(false);
}

uint16_t addGpio(TAddress local, uint8_t pin, uint8_t mode) {
  gpio_t reg;
  reg.reg = local;
  reg.pin = pin;
  reg.mode = mode;
  pinMode(pin, mode);
  switch (local.type) {
  case TAddress::HREG:
    mb->addHreg(local.address);
  break;
  case TAddress::IREG:
    mb->addIreg(local.address);
  break;
  case TAddress::COIL:
    mb->addCoil(local.address);
    mb->onSetCoil(local.address, (mode==OUTPUT)?cbDigitalWrite:cbDigitalRead);
    mb->onGetCoil(local.address, cbDigitalRead);
  break;
  case TAddress::ISTS:
    mb->addIsts(local.address);
    mb->onSetIsts(local.address, (mode==OUTPUT)?cbDigitalWrite:cbDigitalRead);
    mb->onGetIsts(local.address, cbDigitalRead);    
  break;
  }
  gpios.push_back(reg);
  return true;
}

bool saveGpio() {
  File configFile = SPIFFS.open(CFG_GPIO, "w");
  if (configFile) {
    cJSON* json = cJSON_CreateObject();
    if (json) {
      cJSON* arr = cJSON_CreateArray();
      if (arr) {
        for (gpio_t& reg : gpios) {
            cJSON* item;
            cJSON* entry = cJSON_CreateObject();
            if (!entry) continue;
            item = cJSON_CreateNumber(reg.reg.type);
            if (!item) continue;
            cJSON_AddItemToObject(entry, "type", item);
            item = cJSON_CreateNumber(reg.reg.address);
            if (!item) continue;
            cJSON_AddItemToObject(entry, "address", item);
            item = cJSON_CreateNumber(reg.pin);
            if (!item) continue;
            cJSON_AddItemToObject(entry, "pin", item);                        
            item = cJSON_CreateNumber(reg.mode);
            if (!item) continue;
            cJSON_AddItemToObject(entry, "mode", item);                        
            item = cJSON_CreateBool(reg.inverse);
            if (!item) continue;
            cJSON_AddItemToObject(entry, "inverse", item);                        
            item = cJSON_CreateString(reg.name.c_str());
            if (item) cJSON_AddItemToObject(entry, "name", item);
            cJSON_AddItemToArray(arr, entry);
          }
          cJSON_AddItemToObject(json, "gpio", arr);
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

bool readGpio() {
  int16_t i;
  File configFile = SPIFFS.open(CFG_GPIO, "r");
  if (configFile) {
    char* data = (char*)malloc(configFile.size() + 1);
    if (data) {
      if (configFile.read((uint8_t*)data, configFile.size()) == configFile.size()) {
        data[configFile.size()] = '/0';
        TDEBUG("gpio: config %d bytes read\n", configFile.size());
        cJSON* json = cJSON_Parse(data);
        if (json) {
          TDEBUG("gpio: JSON parsed\n");
          cJSON* arr = NULL;
          cJSON* entry = NULL;
          cJSON* item;
          i = 0;
          if (cJSON_HasObjectItem(json, "gpio")) arr = cJSON_GetObjectItemCaseSensitive(json, "gpio");
          if (arr) {
            TDEBUG("gpio: registers:\n");
            cJSON_ArrayForEach(entry, arr) {
              gpio_t reg;
              if (i > GPIO_MAX_COUNT) break;
              i++;
              TDEBUG("gpio: #%d\n", i);
              cJSON* item = NULL;
              if (!cJSON_HasObjectItem(entry, "type")) continue;
              item = cJSON_GetObjectItemCaseSensitive(entry, "type");
              if (!item || !cJSON_IsNumber(item)) continue;
              reg.reg.type = (TAddress::RegType)item->valueint;
              if (!cJSON_HasObjectItem(entry, "address")) continue;
              item = cJSON_GetObjectItemCaseSensitive(entry, "address");
              if (!item || !cJSON_IsNumber(item)) continue;
              reg.reg.address = item->valueint;
              if (!cJSON_HasObjectItem(entry, "inverse")) continue;
              item = cJSON_GetObjectItemCaseSensitive(entry, "inverse");
              if (!item || !cJSON_IsBool(item)) continue;
              reg.inverse = item->valueint;
              if (!cJSON_HasObjectItem(entry, "pin")) continue;
              item = cJSON_GetObjectItemCaseSensitive(entry, "pin");
              if (!item || !cJSON_IsNumber(item)) continue;
              reg.pin = item->valueint;
              if (!cJSON_HasObjectItem(entry, "mode")) continue;
              item = cJSON_GetObjectItemCaseSensitive(entry, "mode");
              if (!item || !cJSON_IsNumber(item)) continue;
              reg.mode = item->valueint;
              if (cJSON_HasObjectItem(entry, "name")) {
                item = cJSON_GetObjectItemCaseSensitive(entry, "name");
                if (item && cJSON_IsString(item))
                  reg.name = item->valuestring;
              }
              addGpio(reg.reg, reg.pin, reg.mode);
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



uint32_t gpioInit() {
  readGpio();
  return RUN_DELETE;
}
