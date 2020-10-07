#pragma once
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#define LCD_CFG "/lcd.json"
#ifndef LCD_ID
// #define LCD_ID    0x27
#define LCD_ID 0x3F
#endif
#ifndef LCD_HEIGHT
#define LCD_HEIGHT 2
#endif
#ifndef LCD_WIDTH
#define LCD_WIDTH 16
#endif

LiquidCrystal_I2C *lcd;

uint32_t lcdInit() {
  uint8_t address = LCD_ID;
  uint8_t width = LCD_WIDTH;
  uint8_t height = LCD_HEIGHT;
  cJSON *json = nullptr;
  cJSON *arr = nullptr;
  cJSON *entry = nullptr;
  cJSON *item = nullptr;
  /*
      File configFile = SPIFFS.open(CFG_LCD, "r");
      if (!configFile)
          goto cleunup;
      char* data = (char*)malloc(configFile.size() + 1);
      if (!data)
          goto cleunup;
      if (configFile.read((uint8_t*)data, configFile.size()) !=
     configFile.size()) goto cleunup; data[configFile.size()] = '/0';
      TDEBUG("lcd: config %d bytes read\n", configFile.size());
      json = cJSON_Parse(data);
      if (!json)
          goto cleunup;
      TDEBUG("lcd: JSON parsed\n");
      if (!cJSON_HasObjectItem(json, "address"))
          goto cleanup;
      item = cJSON_GetObjectItemCaseSensitive(json, "address");
      if (item && cJSON_IsNumber(item))
          address = item->valueint;
          }
      if (cJSON_HasObjectItem(json, "width")) {
          item = cJSON_GetObjectItemCaseSensitive(json, "width");
          if (item && cJSON_IsNumber(item))
            width = item->valueint;
      }
      if (cJSON_HasObjectItem(json, "height")) {
          item = cJSON_GetObjectItemCaseSensitive(json, "height");
          if (item && cJSON_IsNumber(item))
            height = item->valueint;
      }
  */
  lcd = new LiquidCrystal_I2C(address, width, height);
  lcd->begin();

cleanup:
  if (json)
    cJSON_Delete(json);
//  if (data)
//    free(data);
//  if (configFile)
//    configFile.close();
  return RUN_DELETE;
}

void lcdText(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 3) {
    lcd->setCursor(atoi(argv[1]), atoi(argv[2]));
    lcd->print(argv[3]);
  }
}
ShellCommand(lcdtext, "<row> <col> <text> - LCD: Text output", lcdText);

/*
void lcdInt(Shell &shell, int argc, const ShellArguments &argv) {
  String number;
  uint8_t pos = 0;
  uint8_t width = 0;
  if (argc > 5) {
    pos = atoi(argv[3]);
    width = atoi(argv[4]);
    number = argv[5];
  } else if (argc == 4) {
    pos = atoi(argv[3]);
    number = argv[4];
  } else if (argc == 3) {
    number = argv[3];
  }
  if (pos) {
    number =
  }
}
ShellCommand(lcdnum, "<row> <col> [decimal pos] [width] <number> - LCD: Number output", lcdInt);
*/
