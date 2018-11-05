#pragma once
/*
TODO:
i2cscam[ <sda> <scl>][ <rate>]
i2cread <id> <bytes>
i2cwrite <id> <byte>
cp
mv
rm
*/
#include <Wire.h>
#include <ESP8266WiFi.h>
#ifdef ESP8266
 #include <FS.h>
#else
 #include <SPIFFS.h>
#endif
#include <Run.h>
#include <Shell.h>
#include <LoginShell.h>

WiFiServer console(23);
WiFiClient client;
bool haveClient = false;
LoginShell shell;

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

void cliDsList(Shell &shell, int argc, const ShellArguments &argv) {
  shell.printf_P(PSTR("ID\t\t\t\tLocal\n"));
  for (auto &s : sens) {
    shell.printf_P(PSTR("%s\t => \tIREG\t%d\n"), s.addressToString().c_str(), s.pin);
  }
}
ShellCommand(dslist, "- DS1820: List sensors", cliDsList);

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
  shell.println(addIregPull(ip, atoi(argv[2]), atoi(argv[3]), atoi(argv[4])));
}
ShellCommand(pullireg, "<ip> <reg> <local> <interval> - Modbus: Pull Ireg", cliPullIreg);

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
ShellCommand(hreg, "<reg>[ <value>] - Modbus: Hreg get/set/add", cliHreg);
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
ShellCommand(coil, "<reg>[ <0|1>] - Modbus: Coil get/set/add", cliCoil);
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
ShellCommand(ists, "<reg> - Modbus: Ists get/set/add", cliIsts);
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
ShellCommand(ireg, "<reg> - Modbus: Ireg get/set/add", cliIreg);

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
ShellCommand(slavehreg, "<ip> <reg> [<value>] - Modbus: Read/Write slave Hreg", cliSlaveHreg);
void cliSlaveIreg(Shell &shell, int argc, const ShellArguments &argv) {
  IPAddress ip;
  if (argc < 2) return;
  ip.fromString(argv[1]);
  mb->readIreg(ip, atoi(argv[2]), &dataRead, 1, cbReadCli);
}
ShellCommand(slaveireg, "<ip> <reg> - Modbus: Read slave Ireg", cliSlaveIreg);
void cliSlaveCoil(Shell &shell, int argc, const ShellArguments &argv) {
  IPAddress ip;
  if (argc < 2) return;
  ip.fromString(argv[1]);
  if (argc > 3) {
    dataRead = atoi(argv[3]);
    mb->writeCoil(ip, atoi(argv[2]), atoi(argv[3])!=0, cbReadCli);
    return;
  }
  mb->readCoil(ip, atoi(argv[2]), (bool*)&dataRead, 1, cbReadCli);
}
ShellCommand(slavecoil, "<ip> <reg>[ <0|1>] - Modbus: Read/Write slave Coil", cliSlaveCoil);
void cliSlaveIsts(Shell &shell, int argc, const ShellArguments &argv) {
  IPAddress ip;
  if (argc < 2) return;
  ip.fromString(argv[1]);
  mb->readIsts(ip, atoi(argv[2]), (bool*)&dataRead, 1, cbReadCli);
}
ShellCommand(slaveists, "<ip> <reg> - Modbus: Read slave Ists", cliSlaveIsts);
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
void cliSlaveList(Shell &shell, int argc, const ShellArguments &argv) {
  shell.printf_P(PSTR("ID\tAddress\t\tReg\t\t\tLocal\t\tInterval\n"));
  for(auto const& item: regs) {
    shell.printf_P(PSTR("%d\t%s%d.%d.%d.%d\t\%s\t%d\t => \t%s\t%d\t%d\n"),
                        item.taskId, (mb->isConnected(item.ip)?" ":"*"),
                        item.ip[0], item.ip[1], item.ip[2], item.ip[3],
                        regTypeToStr(item.remote), item.remote.address, regTypeToStr(item.reg), item.reg.address, item.interval);
  }
}
ShellCommand(slavelist, "- Modbus: List slave pulls", cliSlaveList);

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
      while (file.read(&data, 1))
        if (!col) {
          col = 16;
          //shell.printf("\n%04x\t", file.pos());
        }
        shell.printf("%02x ", data);
        col--;
      file.close();
    }
  }
}
ShellCommand(hexdump, "<filename> - SPIFFS: View file content in Hex", cliCat);

int passCheck(const char *username, const char *password) {
  return 0;
}

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

