#pragma once
// Simple update module
// To upload through terminal you can use: curl -F "image=@firmware.bin" ehsensor8.local/update
#ifdef ESP8266
 extern ESP8266WebServer* server;
#else
 extern ESPWebServer* server;
 #include <Update.h>
 #define ESP32_SKETCH_SIZE 1310720
#endif
extern uint32_t restartESP();

void updateHandle() {
 #ifdef UPLOADPASS
  if(!server->authenticate(adminUsername.c_str(), adminPassword.c_str())) {
    return server->requestAuthentication();
  }
 #endif
  BUSY
  server->sendHeader("Connection", "close");
  server->sendHeader("Refresh", "10; url=/");
  server->send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
  taskAddWithDelay(restartESP, 500);
}

void updateUploadHandle() {
  size_t sketchSpace;
  BUSY
  HTTPUpload& upload = server->upload();
  switch (upload.status) {
  case UPLOAD_FILE_START:
  //WiFiUDP::stopAll();
   #ifdef ESP8266
    sketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
   #else
    sketchSpace = ESP32_SKETCH_SIZE;
   #endif
    Serial.println(sketchSpace);
    if(!Update.begin(sketchSpace)){//start with max available size
      Update.printError(Serial);
    }
  break;
  case UPLOAD_FILE_WRITE:
    TDEBUG(".");
    if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
      Update.printError(Serial);
    }
    break;
  case UPLOAD_FILE_END:
    TDEBUG("Write");
    if(Update.end(true)){ //true to set the size to the current progress
      TDEBUG("Update Success: %u\nRebooting...\n", upload.totalSize);
    }
    break;
    default:
     TDEBUG("%d", upload.status);
     Update.printError(Serial);
  }
  IDLE
}

uint32_t updateInit() {
    server->on("/update", HTTP_POST, updateHandle, updateUploadHandle); //Update firmware
    return RUN_DELETE;
};
