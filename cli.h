//////////////////////////////////////////
// ESP8266/ESP32 Modbus SmartHome Device
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
ntp
cp
mv
md5
gpiomapname
gpiomapdelete
pullname
pullists
pushcoil
pushhreg
pulldelete
wllist (?)
wlconnect
ip
*/

#include <Wire.h>
#ifdef ESP8266
 #include <ESP8266WiFi.h>
 #include <FS.h>
 #include <ESP8266Ping.h>
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

#ifdef ESP8266
uint32_t pingWait() {
  int8_t p = Ping.ping();
  if (p == -1) return 1000;
  shell.printf((p > 0)?"Alive\n":"No response\n");
  return RUN_DELETE;
}
void cliPing(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 1) {
    IPAddress ip;
    ip.fromString(argv[1]);
    if (ip != IPADDR_NONE) {
      Ping.ping(ip, 3, false);
      taskAddWithDelay(pingWait, 1000);
    }
  }
}
ShellCommand(ping, "<ip> - Ping", cliPing);
#endif

/*
int8_t numNetworks = -2;
uint32_t wlScanWait() {
  numNetworks = WiFi.scanComplete();
  if (numNetworks > 0) {
    Serial.println("Connecting...");
    WiFi.begin();
    return RUN_DELETE;
  }
  Serial.print(".");
  return 1000;
}
void cliWlScan(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 1 && strcmp_P(argv[1], PSTR("force")) == 0) {
    WiFi.disconnect();
    WiFi.scanNetworks(true);
    taskAddWithDelay(wlScanWait, 1000);
    return;
  }
  for (int i = 0; i < numNetworks; i++) {
      shell.printf("%d: %s, Ch:%d (%ddBm) %s\n", i+1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
  }
  WiFi.scanDelete();
  numNetworks = -2;
}
ShellCommand(wlscan, "[ force] - Scan Wi-Fi networks", cliWlScan);
*/
/*
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
*/
/*
void cliExec(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 2) {
    char* buffer[128];
    shell.setBuffer(buffer, 128);
    shel.execute(&argv[1]);
  }
}
ShellCommand(exec, "Execute batch command", cliExec);
*/

// System - related
uint32_t cliLoop();

#if defined(SERIAL)
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
  return 20;
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
#endif

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

#define EXEC_TIME 50
#define EXEC_DELAY RUN_NOW
#define EXEC_BUF 128
File execFile;
uint16_t execId = 0;
char buf[EXEC_BUF];

uint32_t execRun() {  
  if (!execFile)
    return RUN_DELETE;
    
  uint32_t start = millis();
  bool res = true;
  ShellArguments* cmd = nullptr;
  String ln;

  while (millis() - start < EXEC_TIME) {
    char t = buf[strlen((char*)buf) - 1];
    if (!execFile.available()) {
      res = false;
      goto leave;
    }
    ln = execFile.readStringUntil('\n');
    if (ln.length() < 2) {
      res = false;
      goto leave;
    }
    while(strlen((char*)buf) > 0 && (t == '\n' || t == '\r' || t == ' ')) {
        buf[strlen((char*)buf) - 1] = 0;
        t = buf[strlen((char*)buf) - 1];
    }
    if (ln[0] == '&') {
      ln.concat(" ");
      ln.concat((char*)buf);
      //shell.printf("Read: |%s|\n", ln.c_str());
      cmd = new ShellArguments((char*)ln.c_str() + 1, ln.length() - 1);
    } else {
      //shell.printf("Read: |%s|\n", ln.c_str());
      cmd = new ShellArguments(((char*)ln.c_str()), ln.length());
    }
    //shell.printf("Cmd: %s\n", (*cmd)[0]);
    if ((*cmd)[0][0] == ':')
      return RUN_NOW;
    if ((*cmd)[0][0] == '@') {
      if (strcmp((*cmd)[0], "@delay") == 0) {      // @delay
        if (cmd->count() > 1) {
          uint32_t tm = atoi((*cmd)[1]);
          if (tm == 0)
            tm = 1;
          return tm;
        }
      }
      else if (strcmp((*cmd)[0], "@goto") == 0) {  // @goto
        if (cmd->count() > 1) {
          execFile.seek(0);
          while (execFile.available()) {
            String tmp = execFile.readStringUntil('\n');
             if (strcmp(tmp.c_str(), (*cmd)[1]) == 0)
              break;
          }
          if (!execFile.available())
            res = false;
          goto leave;
        }
      }
      else if (strcmp((*cmd)[0], "@add") == 0) {   // @add
        uint16_t sum = 0;
        for (uint8_t i = 1; i < cmd->count(); i++)
          sum += atoi((*cmd)[i]);
        memset(buf, 0, EXEC_BUF);
        shell.setOutput((char*)buf, 128);
        shell.println(sum);
        goto leave;
      }
      else if (strcmp((*cmd)[0], "@ifeq") == 0) {  // @ifne
        if (cmd->count() > 2) {
          //shell.printf("Read: |%s|%s|\n", (*cmd)[1], (char*)buf);
          if (strcmp((*cmd)[1], (char*)buf) == 0) {
            if ((*cmd)[2][0] != ':') {
              memset(buf, 0, EXEC_BUF);
              shell.setOutput((char*)buf, 128);
              shell.print((*cmd)[2]);
            } else {
              execFile.seek(0);
              while (execFile.available()) {
                ln = execFile.readStringUntil('\n');
                if (ln == (((*cmd))[2]))
                  break;
              }
              if (!execFile.available())
                res = false;
            }
          } else {
            if (cmd->count() > 3) {
              memset(buf, 0, EXEC_BUF);
              shell.setOutput((char*)buf, 128);
              shell.print((*cmd)[3]);
            }
          }
        }
        goto leave;
      }
      else if (strcmp((*cmd)[0], "@ifne") == 0) {  // @ifne
        if (cmd->count() > 2) {
          //shell.printf("Read: |%s|%s|\n", (*cmd)[1], (char*)buf);
          if (strcmp((*cmd)[1], (char*)buf) != 0) {
            execFile.seek(0);
            while (execFile.available()) {
              ln = execFile.readStringUntil('\n');
              if (ln == (((*cmd))[2]))
                break;
            }
            if (!execFile.available())
              res = false;
          }
        }
        goto leave;
      }
      else {
        res = false;
        goto leave;
      }
    }
    //shell.printf("Try: %s\n", cmd[0]);
    memset(buf, 0, EXEC_BUF);
    shell.setOutput((char*)buf, 128);
    shell.execute2((*cmd));
  leave:
  shell.setOutput();
  if (cmd)
    delete cmd;
  //shell.println((char*)buf);
  if (!res) {
    shell.println("EXEC: Syntax error");
    if (execFile)
      execFile.close();
    execId = 0;
    ln = "";
    return RUN_DELETE;
  }
  }
  return EXEC_DELAY;
}
void cliExec(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 1) {
    if (execFile)
      execFile.close();
    execFile = SPIFFS.open(argv[1], "r");
    if (execFile) {
      execId = taskAdd(execRun);
      shell.println(execId);
    }
  }
}
ShellCommand(exec, "<filename> - Execute batch", cliExec);

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
void cliFormat(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 1 && strcmp_P(argv[1], PSTR("force")) == 0) {
    if (SPIFFS.format()) shell.printf_P("Done\n");
    return;
  }
  shell.printf_P("All data to be lost. Use 'format force' to precess\n");
}
ShellCommand(format, "[ force] - SPIFFS: Format filesystem", cliFormat);

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
  shell.printf_P(PSTR("Used/Total (bytes): %lu/%lu\n"), fsInfo.usedBytes, fsInfo.totalBytes);
 #else
  dir.close();
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

void cliLine(Shell &shell, int argc, const ShellArguments &argv) {
  File file;
  uint16_t t = 0;
  if (argc < 2)
    return;
  if(SPIFFS.exists(argv[1])){
    if (argc > 3 || (argc > 2 && argv[2][0] == '-')) {
      file = SPIFFS.open(argv[1], "r+");
      if (!file)
        goto leave;
        t = abs(atoi(argv[2]));
      char data;
      uint16_t i = 1;
      file.seek(0);
      while (i < t && file.read((uint8_t*)&data, 1)) {
        if (data =='\n')
          i++;
      }
      size_t filePos = file.position();
      if (argv[2][0] != '+') {
        while (file.read((uint8_t*)&data, 1) && (data != '\n'));
      }
      size_t fileRest = file.size() - file.position();
      uint8_t buf[fileRest];
      file.read(buf, fileRest);
      file.seek(filePos);
      if (argv[2][0] != '-') {
          for (uint8_t j = 3; j < argc; j++) {
            file.write(argv[j]);
            if (j < argc - 1)
              file.write(" ");
          }
          file.write('\n');
      }
      file.write(buf, fileRest);
      file.truncate(file.position());
    } else {
      file = SPIFFS.open(argv[1], "r");
      if (!file)
        goto leave;
      if (argc > 2)
        t = atoi(argv[2]);
      char data;
      uint16_t i = 1;
      if (t == 0)
        shell.printf_P(PSTR("%d\t"), i);
      while (file.read((uint8_t*)&data, 1)) {
        if (data == '\r')
          continue;
        if (t == 0 || i == t)
          shell.write(data);
        if (data =='\n') {
          i++;
          if (t == 0)
            shell.printf_P(PSTR("%d\t"), i);
        }
        if (t != 0 && i > t)
          break;
      }
    }
  } else {
    if (argv[2][0] == '+') {
      file = SPIFFS.open(argv[1], "w");
    }
  }
  leave:
  if (!file) {
    shell.printf_P(PSTR("File not found"));
    return;
  }
  file.close();
}
ShellCommand(line, "<filename> [line#|+line#|-line#] [new text] - SPIFFS: View, replace or add line of file", cliLine);

void cliRm(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc > 0)
    shell.printf(SPIFFS.remove(argv[1])?"":"Failed\n");
}
ShellCommand(rm, "<filename> - SPIFFS: Delete file", cliRm);

void cliCp(Shell &shell, int argc, const ShellArguments &argv) {
  if (argc < 3) return;
  File src = SPIFFS.open(argv[1], "r");
  File dst = SPIFFS.open(argv[2], "w");
  if (!(src && dst)) {
    shell.printf_P(PSTR("IO error\n"));
  } else {
    uint8_t b;
    while (src.read(&b, 1) == 1) dst.write(&b, 1);
  }
  if (src) src.close();
  if (dst) dst.close(); 
}
ShellCommand(cp, "<source> <destination> - SPIFFS: Copy file", cliCp);

void cliVersion(Shell &shell, int argc, const ShellArguments &argv) {
  shell.printf_P("modbus-SmartHome  -  %s\n", VERSION);
}
ShellCommand(version, "- Show build version", cliVersion);

void cliDate(Shell &shell, int argc, const ShellArguments &argv) {
  const time_t t = time(nullptr);
  shell.print(ctime(&t));
}
ShellCommand(date, "- Show date/time", cliDate);

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
            //(Stream)client.setTimeout(50);
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
    return haveClient?50:200;
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
