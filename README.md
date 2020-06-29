# modbus-SmartHome

This sketch is 

## Supported CLI features

* ModbusIP Server
* ModbusIP Client. Pull/push registers from server by schedule.
* 1-Wire DS18B20 mapping to Modbus registers. Query schedule.
* GPIO mapping to Modbus registers (not implemented completry yet)
* Apple HomeKit (mapping Modbus registers to)
* Configuration is storing in JSON and aplaied at boot
* Modbus interactive read/write local and remote registers
* GPIO interactive read/write/mode change
* 1-Wire bus scan
* I2C bus scan
* File operations (list, remove, view content)
* System operarions (memory information, reboot, uptime, show date/time)
* Ping

## Based on following progects

* [Arduino](https://github.com/arduino/Arduino)
* [ESP8266 core for Arduino](https://github.com/esp8266/Arduino)
* [Run 2018.2](https://github.com/emelianov/Run)
* [Ultralightweight JSON parser in ANSI C](https://github.com/DaveGamble/cJSON)
* [Arduino plug and go library for the Maxim (previously Dallas) DS18B20 (and similar) temperature ICs](https://github.com/milesburton/Arduino-Temperature-Control-Library)
* [ESP8266Ping with Async extension](https://github.com/emelianov/ESP8266Ping)
* [Shell](https://github.com/emelianov/Shell)
* [NeoPixelBus](https://github.com/Makuna/NeoPixelBus)
* [Native Apple HomeKit accessory implementation for the ESP8266/ESP32 Arduino core](https://github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266)

## Telnet CLI commands

### File system

* format
* ls
* rm
* cat
* hexdump
* line

### Modbus (slave)

* coil
* hreg
* ists
* ireg

### ModbusIP (master)

* slave
* slavecoil
* slavehreg
* slaveists
* slaveireg
* pullcoil
* pullhreg
* pullists
* pullireg
* pushists
* pilllist

### 1-Wire

* dslist
* dsscan
* dsname
* dsmap
* dssave

### I2C

* i2cscan

### GPIO

* gpio
* gpiomode
* gpiomapcoil
* gpiomapists
* gpiolist
* gpiosave

### HomeKit

* hkreset
* hklist
* khtemp
* hklamp

### WS2812

* ledcolor
* ledon
* ledoff
* ledshift
* ledrotate
* ledshow

### LCD

* lcdtext
* lcdnum
* lcdclear

### Batch

* exec
* @delay
* @goto
* &
* @add
* @ifeq
* @ifgt
* @ifle
* :<label>

### System

* reboot
* serial
* mem
* ps
* kill
* uptime
* ping (ESP8266 only)
* ?
