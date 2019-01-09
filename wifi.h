#pragma once

#include <Run.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266LLMNR.h>
#include <time.h>

uint32_t wifiWait() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
   
    if (!MDNS.begin(sysName.c_str())) {
      TDEBUG("[mDNS: failed]");
    } else {
      MDNS.addService("http", "tcp", 80);  // Add service to MDNS-SD
      TDEBUG("[mDNS: started]");
    }
    // LLMNR
   #ifdef ESP8266
    LLMNR.begin(sysName.c_str());
    TDEBUG("[LLMNR: started]");
   #endif

    return RUN_DELETE;
  }
  TDEBUG(".");
  return 500;
}

uint32_t wifiInit() {
    WiFi.begin();
    taskAdd(wifiWait);
    return RUN_DELETE;
}
