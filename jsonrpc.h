#pragma once
#include <Run.h>
#ifdef ESP8266
 #include <ESP8266WebServer.h>
 extern ESP8266WebServer* server;
#else
 #include <WebServer.h>
 extern WebServer* server;
#endif

#include <cmdJson.h>
#include <MacAddress.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

class Cmd : public JsonServer {
    virtual int16_t queue_f(MacAddress mac) {
        return false;
    }
    virtual data_t data_f(MacAddress mac) {
        data_t data;
        return data;
    }
    virtual int16_t user_f(MacAddress mac) {
        return false;
    }
    virtual int16_t cancel_f(MacAddress mac) {
        return false;
    }
};

Cmd cm;

void handleJsonRpc() {
  if (server->hasHeader(X_SIGNED_HEADER)) {
    char* signCalc = cm.sign(server->arg("plain").c_str());
    Serial.printf("Got: %s, Calc: %s\n", server->header(X_SIGNED_HEADER).c_str(), signCalc);
  }
  String r = cm.process(server->arg("plain").c_str());
  server->sendHeader("Connection", "close");
  server->sendHeader("Cache-Control", "no-store, must-revalidate");
  server->sendHeader(X_SIGNED_HEADER, (char*)cm.sign(r.c_str()));
  server->send(200, "text/json", r.c_str());
}

uint32_t jsonRpcInit() {
  cm.setKey("123");
  Serial.printf("Key: %s, Sign: %s\n", 
  server->on("/rpc", handleJsonRpc);
  return RUN_DELETE;
}
