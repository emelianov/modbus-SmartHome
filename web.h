#pragma once

#include <Run.h>
#include <ESP8266WebServer.h>

void handlePrivate();
ESP8266WebServer* server;

uint32_t webLoop() {
  server->handleClient();
  return 100;
}

uint32_t webInit() {
  server = new ESP8266WebServer(80);
  server->begin();
  server->on("/private", handlePrivate);
  taskAdd(webLoop);
  Serial.print("[HTTP started]");
  return RUN_DELETE;
}

void handlePrivate() {
  char data[400];
  char* xml = ("<?xml version = \"1.0\" encoding=\"UTF-8\" ?><ctrl><private><heap>%d</heap><rssi>%d</rssi><uptime>%ld</uptime></private></ctrl>");
  //sprintf(data, ("<?xml version = \"1.0\" encoding=\"UTF-8\" ?><ctrl><private><heap>%d</heap><rssi>%d</rssi><uptime>%ld</uptime></private></ctrl>"), ESP.getFreeHeap(), WiFi.RSSI(),(uint32_t)millis()/1000);
  sprintf(data, xml, ESP.getFreeHeap(), WiFi.RSSI(),millis()/1000);
  server->sendHeader("Connection", "close");
  server->sendHeader("Cache-Control", "no-store, must-revalidate");
  server->send(200, "text/xml", data);
}
