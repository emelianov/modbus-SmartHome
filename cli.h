#pragma once

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

void cliDs1820(Shell &shell, int argc, const ShellArguments &argv) {
  shell.println("ID\t\tIreg\tTemp");
  for (auto &s : sens) {
    shell.printf("%s\t%d\t%s\n", s.addressToString().c_str(), s.pin, String(s.C).c_str());
  }
}
ShellCommand(ds1820, "- DS1820: List sensors", cliDs1820);


void cliHreg(Shell &shell, int argc, const ShellArguments &argv) {
  if (argv.count() > 1) {
    uint16_t reg = atoi(argv[1]);
    if (argv.count() > 2) {
      uint16_t val = atoi(argv[2]);
      mb->addHreg(reg, val);
      mb->Hreg(reg, val);
    }
    shell.println(mb->Hreg(reg));
  }
}
ShellCommand(hreg, "<reg>[ <value>] - Modbus: Hreg get/set/add", cliHreg);

void cliConnect(Shell &shell, int argc, const ShellArguments &argv) {
  if (argv.count() > 1) {
    IPAddress ip;
    ip.fromString(argv[1]);
    if (!mb->connect(ip))
      shell.println("Modbus: Connection error");
  }
}
ShellCommand(connect, "<ip> - Modbus: Connect to slave", cliConnect);

uint16_t dataRead;
bool cbReadCli(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  if (event != Modbus::EX_SUCCESS) {
    shell.printf("Modbus error: %d\n", event);
    return true;
  }
  shell.println(dataRead);
  return true;
}
void cliReadHreg(Shell &shell, int argc, const ShellArguments &argv) {
  if (argv.count() > 2) {
    IPAddress ip;
    ip.fromString(argv[1]);
    uint16_t reg = atoi(argv[2]);
    mb->readHreg(ip, reg, &dataRead, 1, cbReadCli);
  }
}
ShellCommand(hregread, "<ip> <reg> - Modbus: Read Hreg from slave", cliReadHreg);
void cliReadIreg(Shell &shell, int argc, const ShellArguments &argv) {
  if (argv.count() > 2) {
    IPAddress ip;
    ip.fromString(argv[1]);
    uint16_t reg = atoi(argv[2]);
    mb->readIreg(ip, reg, &dataRead, 1, cbReadCli);
  }
}
ShellCommand(iregred, "<ip> <reg> - Modbus: Read Ireg from slave", cliReadIreg);
void cliReadCoil(Shell &shell, int argc, const ShellArguments &argv) {
  if (argv.count() > 2) {
    IPAddress ip;
    ip.fromString(argv[1]);
    uint16_t reg = atoi(argv[2]);
    mb->readCoil(ip, reg, (bool*)&dataRead, 1, cbReadCli);
  }
}
ShellCommand(coilread, "<ip> <reg> - Modbus: Read Coil from slave", cliReadCoil);
void cliReadIsts(Shell &shell, int argc, const ShellArguments &argv) {
  if (argv.count() > 2) {
    IPAddress ip;
    ip.fromString(argv[1]);
    uint16_t reg = atoi(argv[2]);
    mb->readIsts(ip, reg, (bool*)&dataRead, 1, cbReadCli);
  }
}
ShellCommand(istsread, "<ip> <reg> - Modbus: Read Ists from slave", cliReadIsts);


uint32_t cliLoop();
uint32_t serial2cli() {
  uint8_t data;
  while (Serial.readBytes(&data, 1))
    shell.write(data);
  while (shell.readBytes(&data, 1)) {
    if (data == 0x1A) {
      taskAdd(cliLoop);
      shell.println("^Z");
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
  shell.printf("ID\t\tDelay\tRemind\n");
  for (uint8_t i = 0; i < taskCount; i++) {
    shell.printf("%d\t%lu\t%lu\n", taskTasks[i].id, taskTasks[i].delay, millis() - taskTasks[i].lastRun);
  }
}
ShellCommand(ps, "- Display task list", cliPs);

void cliMem(Shell &shell, int argc, const ShellArguments &argv) {
  shell.println(ESP.getFreeHeap());
}
ShellCommand(mem, "- Display free memory", cliMem);

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
    shell.println(entry.name());
    entry.close();
  }
 #ifndef ESP8266
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
        shell.println("File not found");
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
    // Listen on port 23 for incoming telnet connections.
    console.begin();
    shell.setMachineName("ehcontrol4");
    shell.setPasswordCheckFunction(passCheck);
    taskAdd(cliLoop);
    return RUN_DELETE;
}

