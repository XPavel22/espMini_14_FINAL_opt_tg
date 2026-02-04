#pragma once
#include <ArduinoJson.h>
#include <Arduino.h>
#include <vector>
#include <memory>

#ifdef ESP32
#include <WiFi.h>
#include <SPIFFS.h>
#elif defined(ESP8266)
#include <FS.h>
#include <ESP8266WiFi.h>
#endif

#ifdef ESP32
    #define SIZE_JSON_S 8192
#elif defined(ESP8266)
    #define SIZE_JSON_S 6144
#endif
