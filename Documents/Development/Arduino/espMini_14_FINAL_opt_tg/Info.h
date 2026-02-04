#ifndef INFO__H
#define INFO__H

#include <cstddef>
#include <cstdio>
#include <cstdint>

#ifdef ESP32
#include <WiFi.h>
#include <esp_system.h>
#include <esp_chip_info.h>
#include <SPIFFS.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FS.h>
#endif

class Info
{
public:
    Info();

    void getChipModel(char* buffer, size_t bufferSize);

     size_t getSystemStatus(char* buffer, size_t bufferSize);

private:

};

#endif
