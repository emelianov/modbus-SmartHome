/*
 * simple_led_accessory.c
 * Define the accessory in pure C language using the Macro in characteristics.h
 *
 *  Created on: 2020-02-08
 *      Author: Mixiaoxiao (Wang Bin)
 *  Edited on: 2020-03-01
 *      Edited by: euler271 (Jonas Linn)
 */

#include <Arduino.h>
//#include <arduino_homekit_server.h>
#include <homekit/types.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <stdio.h>
#include <port.h>

#define ACCESSORY_NAME  ("ESP8266_LED")
#define ACCESSORY_SN  ("SN_0123456")  //SERIAL_NUMBER
#define ACCESSORY_MANUFACTURER ("http://733666.ru")
#define ACCESSORY_MODEL  ("ESP8266")

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, ACCESSORY_NAME);
homekit_characteristic_t serial_number = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, ACCESSORY_SN);

void accessory_identify(homekit_value_t _value) {

}

homekit_service_t** svc;
homekit_accessory_t *accessories[2];
homekit_server_config_t config = {
		.accessories = accessories,
		.password = "111-11-111",
		//.on_event = on_homekit_event,
		.setupId = "ABCD"
};

homekit_characteristic_t* addCT(char* name, homekit_value_t (*f)) {
  uint8_t i = 0;
  while (svc[i] != NULL) i++;
  homekit_service_t** t = realloc(svc, sizeof(homekit_service_t*) * (i + 2));
  if (!t) return NULL;
  svc = t;
  homekit_characteristic_t temp = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0.00);
  *temp.min_value = -200;
  *temp.max_value = 200;
  homekit_characteristic_t* temp_on = homekit_characteristic_clone(&temp);  
  svc[i] = NEW_HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .primary=true,
              .characteristics=(homekit_characteristic_t*[]){
                NEW_HOMEKIT_CHARACTERISTIC(NAME, name),
                temp_on,
                NULL
              });
  svc[i + 1] = NULL;
  return temp_on;
}

homekit_characteristic_t* addTT(char* name, homekit_characteristic_t** ch,
                                            void (*s2)(homekit_value_t), homekit_characteristic_t** ch2,
                                            homekit_characteristic_t** ch3,
                                            void (*s4)(homekit_value_t), homekit_characteristic_t** ch4) {
  uint8_t i = 0;
  while (svc[i] != NULL) i++;
  homekit_service_t** t = realloc(svc, sizeof(homekit_service_t*) * (i + 2));
  if (!t) return NULL;
  svc = t;
  homekit_characteristic_t t1 = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 20.00);
  homekit_characteristic_t t2 = HOMEKIT_CHARACTERISTIC_(TARGET_TEMPERATURE, 20.00, .setter=s2);
  homekit_characteristic_t h1 = HOMEKIT_CHARACTERISTIC_(CURRENT_HEATING_COOLING_STATE, 0);
  homekit_characteristic_t h2 = HOMEKIT_CHARACTERISTIC_(TARGET_HEATING_COOLING_STATE, 0, .setter=s4);
  homekit_characteristic_t u = HOMEKIT_CHARACTERISTIC_(TEMPERATURE_DISPLAY_UNITS, 0); 
  homekit_characteristic_t* temp1 = homekit_characteristic_clone(&t1);  
  homekit_characteristic_t* temp2 = homekit_characteristic_clone(&t2);
  homekit_characteristic_t* heat1 = homekit_characteristic_clone(&h1);
  homekit_characteristic_t* heat2 = homekit_characteristic_clone(&h2);
  homekit_characteristic_t* units = homekit_characteristic_clone(&u);
  *ch = temp1;
  *ch2 = temp2;
  *ch3 = heat1;
  *ch4 = heat2;
  svc[i] = NEW_HOMEKIT_SERVICE(THERMOSTAT,// .primary=true,
              .characteristics=(homekit_characteristic_t*[]){
                NEW_HOMEKIT_CHARACTERISTIC(NAME, name),
                temp1,
                temp2,
                heat1,
                heat2,
                units,
                NULL
              });
  svc[i + 1] = NULL;
  return NULL;
}

homekit_characteristic_t* addON(char* name, void (*s(homekit_value_t))) {
  uint8_t i = 0;
  while (svc[i] != NULL) i++;
  homekit_service_t** t = realloc(svc, sizeof(homekit_service_t*) * (i + 2));
  if (!t) return NULL;
  svc = t;
  homekit_characteristic_t temp = HOMEKIT_CHARACTERISTIC_(ON, false, .setter=s);
  homekit_characteristic_t* temp_on = homekit_characteristic_clone(&temp);  
  svc[i] = NEW_HOMEKIT_SERVICE(LIGHTBULB, .primary=true,
              .characteristics=(homekit_characteristic_t*[]){
                  NEW_HOMEKIT_CHARACTERISTIC(NAME, name),
                  temp_on,
                 NULL
              });
  svc[i + 1] = NULL;
  return temp_on;
}

homekit_characteristic_t* addMotion(char* name) {
  uint8_t i = 0;
  while (svc[i] != NULL) i++;
  homekit_service_t** t = realloc(svc, sizeof(homekit_service_t*) * (i + 2));
  if (!t) return NULL;
  svc = t;
  homekit_characteristic_t temp = HOMEKIT_CHARACTERISTIC_(OCCUPANCY_DETECTED, 0);
  homekit_characteristic_t* temp_on = homekit_characteristic_clone(&temp);  
  svc[i] = NEW_HOMEKIT_SERVICE(MOTION_SENSOR,
              .characteristics=(homekit_characteristic_t*[]){
                  NEW_HOMEKIT_CHARACTERISTIC(NAME, name),
                  temp_on,
                 NULL
              });
  svc[i + 1] = NULL;
  return temp_on;
}

void service_init() {
  svc = malloc(sizeof(homekit_service_t*) * 2);
  svc[0] =      NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION,
              .characteristics=(homekit_characteristic_t*[]){
                &name,
                NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, ACCESSORY_MANUFACTURER),
                &serial_number,
                NEW_HOMEKIT_CHARACTERISTIC(MODEL, ACCESSORY_MODEL),
                NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
                NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, accessory_identify),
                NULL
              });
  svc[1] =        NULL;
}
void accessory_init() {
  accessories[0] = NEW_HOMEKIT_ACCESSORY(
            .id = 1,
            .category = homekit_accessory_category_lightbulb,
            .services=svc);
  accessories[1] =    NULL;
}
