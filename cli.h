//////////////////////////////////////////
// ESP8266/ESP32 Modbus Temperature sensor
// (c)2018, a.m.emelianov@gmail.com
//
// CLI interface
//
#pragma once
/*
TODO:
i2cscan[ <sda> <scl>][ <rate>]
i2cread <id> <bytes>
i2cwrite <id> <byte>
cp
mv
format
tftp
version
*/
#include <Wire.h>
#ifdef ESP8266
 #include <ESP8266WiFi.h>
 #include <FS.h>
#else
 #include <WiFi.h>
 #include <SPIFFS.h>
#endif
#include <Run.h>
#include <Shell.h>
#include <LoginShell.h>

WiFiServer console(23);
WiFiClient client;
bool haveClient = false;
LoginShell shell;

char* regTypeToStr(TAddress reg) {
  char* s = "COIL";
  if (reg.type == TAddress::HREG) {
    s = "HREG";
  } else if (reg.type == TAddress::IREG) {
    s = "IREG";
  } else if (reg.type == TAddress::ISTS) {
    s = "ISTS";
  }
  return s;
}

void clii2cScan(Shell &shell, int argc, const ShellArguments &argv) {
  uint8_t error, address;
  uint8_t nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      shell.printf_P(PSTR("I2C device found at address %02X\n"), address);
      nDevices++;
    }
    else if (error == 4) {
      shell.printf_P(PSTR("Unknow error at address %02X\n"), address);
    }
  }
  if (nDevices == 0)
    shell.printf_P(PSTR("No I2C devices found\n"));
}
ShellCommand(i2cscan, "- I2C: Scan bus", clii2cScan);

void cliGpio(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 2) {
    digitalWrite(atoi(argv[1]), atoi(argv[2]));
  }
  if (argc > 1) {
    shell.println(digitalRead(atoi(argv[1])));
  }
}
ShellCommand(gpio, "<nr>[ <0|1>] - GPIO: Read/Write", cliGpio);
void cliGpioMode(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 2) {
    pinMode(atoi(argv[1]), strcmp(argv[2], "input") == 0?INPUT:OUTPUT);
  }
}
ShellCommand(gpiomode, "<nr> <input|output> - GPIO: Set mode", cliGpioMode);

void cliGpioMapIsts(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 2) {
    addGpio(ISTS(atoi(argv[2])), atoi(argv[1]), INPUT);
  }
}
ShellCommand(gpiomapists, "<pin> <local_ists> - GPIO: Map to Ists", cliGpioMapIsts);

void cliGpioMapCoil(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 2) {
    addGpio(COIL(atoi(argv[2])), atoi(argv[1]), OUTPUT);
  }
}
ShellCommand(gpiomapcoil, "<pin> <local_coil> - GPIO: Map to Coil", cliGpioMapCoil);

void cliGpioList(Shell &shell, int argc, const ShellArguments &argv) {
  shell.printf_P(PSTR("GPIO\t\tReg\n"));
  for (auto &s : gpios) {
    shell.printf_P(PSTR("%d\t %s \t%s\t%d\t'%s\n"), s.pin, (s.mode == OUTPUT)?"<=":"=>", regTypeToStr(s.reg), s.reg.address, s.name.c_str());
  }
}
ShellCommand(gpiolist, "- GPIO: List mappings", cliGpioList);

void cliGpioSave(Shell &shell, int argc, const ShellArguments &argv) {
  shell.println(saveGpio()?"Done":"Error");
}
ShellCommand(gpiosave, "- GPIO: Save mappings", cliGpioSave);

void cliDsList(Shell &shell, int argc, const ShellArguments &argv) {
  shell.printf_P(PSTR("ID\t\t\t\tLocal\n"));
  for (auto &s : sens) {
    shell.printf_P(PSTR("%s\t => \tIREG\t%d\t'%s\n"), s.addressToString().c_str(), s.pin, s.name.c_str());
  }
}
ShellCommand(dslist, "- DS1820: List sensors", cliDsList);

void cliDsSave(Shell &shell, int argc, const ShellArguments &argv) {
  shell.println(saveSensors()?"Done":"Error");
}
ShellCommand(dssave, "- DS1820: Save sensors", cliDsSave);

void cliDsScan(Shell &shell, int argc, const ShellArguments &argv) {
  shell.printf_P(PSTR("Found %d new sensors\n"), scanSensors());
}
ShellCommand(dsscan, "- DS1820: Scan for new sensors", cliDsScan);

void cliDsMap(Shell &shell, int argc, const ShellArguments &argv) {
  DeviceAddress addr;
  sensor* dev;
  char* tmp = "00";
  if (argc > 2) {
    for (uint8_t j = 0; j < 8; j++) {
      tmp[0] = argv[1][j*2];
      tmp[1] = argv[1][j*2 + 1];
      addr[j] = strtol(tmp, NULL, 16);
    }
    dev = deviceFind(addr);
    if (dev) {
      dev->pin = atoi(argv[2]);
    }
  }
}
ShellCommand(dsmap, "<ID> <ireg> - DS1820: Map sensor to IREG", cliDsMap);

void cliDsName(Shell &shell, int argc, const ShellArguments &argv) {
  DeviceAddress addr;
  sensor* dev;
  char* tmp = "00";
  if (argc > 2) {
    for (uint8_t j = 0; j < 8; j++) {
      tmp[0] = argv[1][j*2];
      tmp[1] = argv[1][j*2 + 1];
      addr[j] = strtol(tmp, NULL, 16);
    }
    dev = deviceFind(addr);
    if (dev) {
      dev->name = argv[2];
    }
  }
}
ShellCommand(dsname, "<ID> <name> - DS1820: Set sensor name", cliDsName);

void cliPullHreg(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 5) return;
  IPAddress ip;
  ip.fromString(argv[1]);
  shell.println(addPull(ip, HREG(atoi(argv[2])), IREG(atoi(argv[3])), atoi(argv[4])));
}
ShellCommand(pullhreg, "<ip> <slave_hreg> <local_ireg> <interval> - Modbus: Pull slave Hreg to local Ireg", cliPullHreg);

void cliPullIreg(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 5) return;
  IPAddress ip;
  ip.fromString(argv[1]);
  shell.println(addPull(ip, IREG(atoi(argv[2])), IREG(atoi(argv[3])), atoi(argv[4])));
}
ShellCommand(pullireg, "<ip> <slave_ireg> <local_ireg> <interval> - Modbus: Pull Ireg", cliPullIreg);

void cliPullCoil(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 5) return;
  IPAddress ip;
  ip.fromString(argv[1]);
  shell.println(addPull(ip, COIL(atoi(argv[2])), ISTS(atoi(argv[3])), atoi(argv[4])));
}
ShellCommand(pullcoil, "<ip> <slave_coil> <local_ists> <interval> - Modbus: Pull slave Coil to local Ists", cliPullCoil);

void cliPushIsts(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 5) return;
  IPAddress ip;
  ip.fromString(argv[2]);
  shell.println(addPull(ip, COIL(atoi(argv[3])), ISTS(atoi(argv[1])), atoi(argv[4]), 1, false));
}
ShellCommand(pushists, "<local_ists> <ip> <slave_coil> <interval> - Modbus: Push local Ists to slave Coil", cliPushIsts);

void cliPullSave(Shell &shell, int argc, const ShellArguments &argv) {
  shell.println(saveModbus()?"Done":"Error");
}
ShellCommand(pullsave, "- Modbus: Slave Pull registers", cliPullSave);

void cliHreg(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 2) return;
  uint16_t reg = atoi(argv[1]);
  if (argv.count() > 2) {
    uint16_t val = atoi(argv[2]);
    mb->addHreg(reg, val);
    mb->Hreg(reg, val);
  }
  shell.println(mb->Hreg(reg));
}
ShellCommand(hreg, "<hreg>[ <value>] - Modbus: Hreg get/set/add", cliHreg);
void cliCoil(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 2) return;
  uint16_t reg = atoi(argv[1]);
  if (argv.count() > 2) {
    bool val = atoi(argv[2]) != 0;
    mb->addCoil(reg, val);
    mb->Coil(reg, val);
  }
  shell.println(mb->Coil(reg));
}
ShellCommand(coil, "<coil>[ <0|1>] - Modbus: Coil get/set/add", cliCoil);
void cliIsts(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 2) return;
  uint16_t reg = atoi(argv[1]);
  if (argv.count() > 2) {
    bool val = atoi(argv[2]) != 0;
    mb->addIsts(reg, val);
    mb->Ists(reg, val);
  }
  shell.println(mb->Ists(reg));
}
ShellCommand(ists, "<ists>[ <0|1>] - Modbus: Ists get/set/add", cliIsts);
void cliIreg(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 2) return;
  uint16_t reg = atoi(argv[1]);
  if (argv.count() > 2) {
    uint16_t val = atoi(argv[2]);
    mb->addIreg(reg, val);
    mb->Ireg(reg, val);
  }
  shell.println(mb->Ireg(reg));
}
ShellCommand(ireg, "<ireg>[ <value>] - Modbus: Ireg get/set/add", cliIreg);

void cliConnect(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 1) {
    IPAddress ip;
    ip.fromString(argv[1]);
    if (mb->isConnected(ip)) {
      shell.printf_P(PSTR("Modbus: Already connected\n"));
    }
    if (!mb->connect(ip))
      shell.printf_P(PSTR("Modbus: Connection error\n"));
  }
}
ShellCommand(connect, "<ip> - Modbus: Connect to slave", cliConnect);

uint16_t dataRead;
bool cbReadCli(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  if (event != Modbus::EX_SUCCESS) {
    shell.printf_P(PSTR("Modbus error: 0x%02X\n"), event);
    return true;
  }
  shell.println(dataRead);
  return true;
}
void cliSlaveHreg(Shell &shell, int argc, const ShellArguments &argv) {
  IPAddress ip;
  if (argc < 2) return;
  ip.fromString(argv[1]);
  if (argc > 3) {
    dataRead = atoi(argv[3]);
    mb->writeHreg(ip, atoi(argv[2]), atoi(argv[3]), cbReadCli);
    return;
  }
  mb->readHreg(ip, atoi(argv[2]), &dataRead, 1, cbReadCli);
}
ShellCommand(slavehreg, "<ip> <hreg>[ <value>] - Modbus: Read/Write slave Hreg", cliSlaveHreg);
void cliSlaveIreg(Shell &shell, int argc, const ShellArguments &argv) {
  IPAddress ip;
  if (argc < 2) return;
  ip.fromString(argv[1]);
  mb->readIreg(ip, atoi(argv[2]), &dataRead, 1, cbReadCli);
}
ShellCommand(slaveireg, "<ip> <ireg> - Modbus: Read slave Ireg", cliSlaveIreg);
void cliSlaveCoil(Shell &shell, int argc, const ShellArguments &argv) {
  IPAddress ip;
  if (argc < 2) {
    shell.println("Wrong arguments count");
    return;
  }
  ip.fromString(argv[1]);
  if (argc > 3) {
    dataRead = atoi(argv[3]);
    if (mb->writeCoil(ip, atoi(argv[2]), atoi(argv[3])!=0, cbReadCli)) {
      shell.println("Not connected");
      return;
    }
  }
  if (!mb->readCoil(ip, atoi(argv[2]), (bool*)&dataRead, 1, cbReadCli)) {
    shell.println("Not connected");
  }
}
ShellCommand(slavecoil, "<ip> <coil>[ <0|1>] - Modbus: Read/Write slave Coil", cliSlaveCoil);
void cliSlaveIsts(Shell &shell, int argc, const ShellArguments &argv) {
  IPAddress ip;
  if (argc < 2) return;
  ip.fromString(argv[1]);
  mb->readIsts(ip, atoi(argv[2]), (bool*)&dataRead, 1, cbReadCli);
}
ShellCommand(slaveists, "<ip> <ists> - Modbus: Read slave Ists", cliSlaveIsts);

void cliPullList(Shell &shell, int argc, const ShellArguments &argv) {
  shell.printf_P(PSTR("ID\t Address\tReg\t\tCount\t\tLocal\t\tInterval\n"));
  for(auto const& item: regs) {
    shell.printf_P(PSTR("%d\t%s%d.%d.%d.%d\t\%s\t%d\t%d\t %s \t%s\t%d\t%d\t'%s\n"),
                        item.taskId, (mb->isConnected(item.ip)?"*":" "),
                        item.ip[0], item.ip[1], item.ip[2], item.ip[3],
                        regTypeToStr(item.remote), item.remote.address, item.count, item.pull?"=>":"<=", regTypeToStr(item.reg), item.reg.address, item.interval, item.name.c_str());
  }
}
ShellCommand(pulllist, "- Modbus: List slave pulls/pushs", cliPullList);

// System - related
uint32_t cliLoop();
uint32_t serial2cli() {
  uint8_t data;
  while (Serial.readBytes(&data, 1))
    shell.write(data);
  while (shell.readBytes(&data, 1)) {
    if (data == 0x1A) {
      taskAdd(cliLoop);
      shell.printf_P(PSTR("^Z"));
      return RUN_DELETE;
    }
    Serial.write(data);
  }
  return 100;
}
void cliSerial(Shell &shell, int argc, const ShellArguments &argv) {
  if (argv.count() > 1) {
    uint32_t b = atoi(argv[1]);
    if (b > 0) {
      Serial.end();
      Serial.begin(b);
    }
  }
  taskDel(cliLoop);
  taskAdd(serial2cli);
}
ShellCommand(serial, "[<Baudrate>] - Connect to serial (^Z to break)", cliSerial);

void cliPs(Shell &shell, int argc, const ShellArguments &argv) {
  shell.printf_P(PSTR("ID\tDelay\tRemind\n"));
  for (uint8_t i = 0; i < taskCount; i++) {
    shell.printf_P(PSTR("%d\t%lu\t%lu\n"), taskTasks[i].id, taskTasks[i].delay, millis() - taskTasks[i].lastRun);
  }
}
ShellCommand(ps, "- Display task list", cliPs);

void cliKill(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 1)
    taskDel(atoi(argv[1]));
}
ShellCommand(kill, "<id> - Kill task by ID", cliKill);


void cliMem(Shell &shell, int argc, const ShellArguments &argv) {
  shell.println(ESP.getFreeHeap());
}
ShellCommand(mem, "- System free heap", cliMem);

void cliReboot(Shell &shell, int argc, const ShellArguments &argv) {
  ESP.restart();
}
ShellCommand(reboot, "- System Reboot", cliReboot);

void cliUptime(Shell &shell, int argc, const ShellArguments &argv) {
  shell.println(millis()/1000);
}
ShellCommand(uptime, "- System uptime", cliUptime);

// SPIFFS - related
void cliLs(Shell &shell, int argc, const ShellArguments &argv) {
  String path = "/";  //argc?argv[1]:"/";
 #ifdef ESP8266
  Dir dir = SPIFFS.openDir(path);
  while(dir.next()){
    File entry = dir.openFile("r");
 #else
  File dir = SPIFFS.open(path);
  File entry;
  while (entry = dir.openNextFile()) {
 #endif
    shell.printf_P(PSTR("%s\t%d\n"), entry.name(), entry.size());
    entry.close();
  }
 #ifdef ESP8266
  FSInfo fsInfo;
  SPIFFS.info(fsInfo);
  shell.printf_P(PSTR("Free/Total (bytes): %lu/%lu\n"), fsInfo.usedBytes, fsInfo.totalBytes);
 #else
  shell.print(SPIFFS.usedBytes());
  shell.print("/");
  shell.print(SPIFFS.totalBytes());
 #endif
}
ShellCommand(ls, "- SPIFFS: List files", cliLs);

void cliCat(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 0 && argv.count() > 0) {
    if(SPIFFS.exists(argv[1])){
      File file = SPIFFS.open(argv[1], "r");
      if (!file) {
        shell.printf_P(PSTR("File not found"));
        return;
      }
      uint8_t data;
      while (file.read(&data, 1))
        shell.write(data);
      file.close();
    }
  }
}
ShellCommand(cat, "<filename> - SPIFFS: View file content", cliCat);

void cliHexdump(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 0 && argv.count() > 0) {
    if(SPIFFS.exists(argv[1])){
      File file = SPIFFS.open(argv[1], "r");
      if (!file) {
        shell.printf_P(PSTR("File not found"));
        return;
      }
      uint8_t data;
      uint8_t col = 0;
      while (file.read(&data, 1)) {
        if (!col) {
          col = 16;
          shell.printf("\n%04x\t", file.position() - 1);
        }
        shell.printf("%02x ", data);
        col--;
      } 
      file.close();
    }
  }
}
ShellCommand(hexdump, "<filename> - SPIFFS: View file content in Hex", cliHexdump);

void cliRm(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 0)
    shell.printf(SPIFFS.remove(argv[1])?"Done":"Failed");
}
ShellCommand(rm, "<filename> - SPIFFS: Delete file", cliRm);

int passCheck(const char *username, const char *password) {
  return 0;
}

extern uint32_t mem;

uint32_t cliLoop() {
    // Handle new/disconnecting clients.
    if (!haveClient) {
        // Check for new client connections.
        client = console.available();
        if (client) {
            haveClient = true;
            shell.begin(client, 5);
        }
    } else if (!client.connected()) {
        // The current client has been disconnected.  Shut down the shell.
        shell.end();
        client.stop();
        client = WiFiClient();
        haveClient = false;
    }

    // Perform periodic shell processing on the active client.
    shell.loop();
    return 100;
}

uint32_t cliInit() {
  Wire.begin();
  // Listen on port 23 for incoming telnet connections.
  console.begin();
  shell.setMachineName("ehcontrol4");
  shell.setPasswordCheckFunction(passCheck);
  taskAdd(cliLoop);
  return RUN_DELETE;
}
