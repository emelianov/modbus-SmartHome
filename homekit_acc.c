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

#define PIN_LED  2//D4

bool led_power = false; //true or flase
void led_update();

homekit_value_t led_on_get() {
	return HOMEKIT_BOOL(led_power);
}

void led_on_set(homekit_value_t value) {
	if (value.format != homekit_format_bool) {
		printf("Invalid on-value format: %d\n", value.format);
		return;
	}
	led_power = value.bool_value;
	led_update();
}

extern homekit_value_t temp_on_get();

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, ACCESSORY_NAME);
homekit_characteristic_t serial_number = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, ACCESSORY_SN);
homekit_characteristic_t led_on = HOMEKIT_CHARACTERISTIC_(ON, false, .getter=led_on_get, .setter=led_on_set);
homekit_characteristic_t temp_on = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0.00, .getter=temp_on_get);

void led_update() {
	if (led_power) {
   printf("ON\n");
   digitalWrite(PIN_LED, LOW);
	} else {
		printf("OFF\n");
		digitalWrite(PIN_LED, HIGH);
	}
}

void led_toggle() {
	led_on.value.bool_value = !led_on.value.bool_value;
	led_on.setter(led_on.value);
	homekit_characteristic_notify(&led_on, led_on.value);
}

void accessory_identify(homekit_value_t _value) {
	printf("accessory identify\n");
	for (int j = 0; j < 3; j++) {
		led_power = true;
		led_update();
		delay(100);
		led_power = false;
		led_update();
		delay(100);
	}
}

homekit_service_t** svc;
homekit_accessory_t *accessories[2];
homekit_server_config_t config = {
		.accessories = accessories,
		.password = "111-11-111",
		//.on_event = on_homekit_event,
		.setupId = "ABCD"
};

//homekit_value_t on_get() {
//  return HOMEKIT_FLOAT_CPP(30.0);
//}

homekit_characteristic_t* addCT(char* name, homekit_value_t (*f)) {
  uint8_t i = 0;
  while (svc[i] != NULL) i++;
  homekit_service_t** t = realloc(svc, sizeof(homekit_service_t*) * (i + 2));
  if (!t) return NULL;
  svc = t;
  homekit_characteristic_t temp = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0.00, .getter=f);
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

homekit_characteristic_t* addON(char* name, homekit_value_t (*g), void (*s(homekit_value_t))) {
  uint8_t i = 0;
  while (svc[i] != NULL) i++;
  homekit_service_t** t = realloc(svc, sizeof(homekit_service_t*) * (i + 2));
  if (!t) return NULL;
  svc = t;
  homekit_characteristic_t temp = HOMEKIT_CHARACTERISTIC_(ON, false, .getter=g, .setter=s);
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
/*
  svc[1] =        NEW_HOMEKIT_SERVICE(LIGHTBULB, .primary=true,
              .characteristics=(homekit_characteristic_t*[]){
                  NEW_HOMEKIT_CHARACTERISTIC(NAME, "Led"),
                  &led_on,
                 NULL
              });
              
  svc[2] =        NEW_HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .primary=true,
              .characteristics=(homekit_characteristic_t*[]){
                NEW_HOMEKIT_CHARACTERISTIC(NAME, "Temperature"),
                &temp_on,
                NULL
              });
*/
  svc[1] =        NULL;
  addON("Led", led_on_get, led_on_set);
  //addCT("Temperature", on_get);
}
void accessory_init() {
  accessories[0] = NEW_HOMEKIT_ACCESSORY(
            .id = 1,
            .category = homekit_accessory_category_lightbulb,
            .services=svc);
  accessories[1] =    NULL;

	pinMode(PIN_LED, OUTPUT);
	led_update();
}
