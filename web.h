#pragma once

#include <Run.h>
#ifdef ESP8266
 #include <FS.h>
 #include <ESP8266WebServer.h>
 ESP8266WebServer* server;
#else
 #include <SPIFFS.h>
 #include <WebServer.h>
 WebServer* server;
#endif

#include <detail/RequestHandlersImpl.h> // for StaticRequestHandler::getContentType(path);

extern ModbusIP* mb;

void handlePrivate();

uint32_t webLoop() {
  server->handleClient();
  return 100;
}

String getContentType(String filename){
  if(server->hasArg(F("download"))) return "application/octet-stream";
  else if(filename.endsWith(F(".htm"))) return "text/html";
  else if(filename.endsWith(F(".html"))) return "text/html";
  else if(filename.endsWith(F(".css"))) return "text/css";
  else if(filename.endsWith(F(".js"))) return "application/javascript";
  else if(filename.endsWith(F(".png"))) return "image/png";
  else if(filename.endsWith(F(".gif"))) return "image/gif";
  else if(filename.endsWith(F(".jpg"))) return "image/jpeg";
  else if(filename.endsWith(F(".ico"))) return "image/x-icon";
  else if(filename.endsWith(F(".xml"))) return "text/xml";
  else if(filename.endsWith(F(".pdf"))) return "application/x-pdf";
  else if(filename.endsWith(F(".zip"))) return "application/x-zip";
  else if(filename.endsWith(F(".gz"))) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  if(path.endsWith("/")) path += "index.html";
  //String contentType = StaticRequestHandler::getContentType(path);
  String contentType = getContentType(path);
  //String pathWithGz = path + ".gz";
  if(SPIFFS.exists(path + ".gz"))
    path += ".gz";
  if(SPIFFS.exists(path)){
    server->sendHeader("Connection", "close");
    server->sendHeader("Cache-Control", "no-store, must-revalidate");
    server->sendHeader("Access-Control-Allow-Origin", "*");
    File file = SPIFFS.open(path, "r");
    size_t sent = server->streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleGenericFile() {
  if(!handleFileRead(server->uri()))
    server->send(404, "text/plain", "FileNotFound");
}

uint32_t webInit() {
 #ifdef ESP8266
  server = new ESP8266WebServer(80);
 #else
  server = new WebServer(80);
 #endif
  server->begin();
  server->onNotFound(handleGenericFile);
  server->on("/private", handlePrivate);
  taskAdd(webLoop);
  TDEBUG("[HTTP started]");
  return RUN_DELETE;
}

void handlePrivate() {
  char data[400];
  //char* xml = ("<?xml version = \"1.0\" encoding=\"UTF-8\" ?><ctrl><private><heap>%d</heap><rssi>%d</rssi><uptime>%ld</uptime><transactions>%d</transactions></private></ctrl>");
  //sprintf(data, xml, ESP.getFreeHeap(), WiFi.RSSI(),millis()/1000, mb->transactions());
  sprintf_P(data, PSTR("<?xml version = \"1.0\" encoding=\"UTF-8\" ?><ctrl><private><heap>%d</heap><rssi>%d</rssi><uptime>%ld</uptime><transactions>%d</transactions></private></ctrl>"), ESP.getFreeHeap(), WiFi.RSSI(),millis()/1000, 0);
  server->sendHeader("Connection", "close");
  server->sendHeader("Cache-Control", "no-store, must-revalidate");
  server->send(200, "text/xml", data);
}
