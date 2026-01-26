#pragma once

#ifdef ESP8266
  #include <NTPClient.h>
  #include <WiFiUdp.h>
  #include <EEPROM.h>
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <NTPClient.h>
  #include <WiFiUdp.h>
  #include <Preferences.h>
  #include <WiFi.h>
#endif

#include <time.h>
#include "AppState.h"

class TimeModule {
public:
    TimeModule( AppState& appState);
    void beginNTP();
    void updateTime();
    time_t getCurrentTime();
    char* getFormattedTime(const char* format = "%Y-%m-%d %H:%M:%S");
    bool setTimeManually(const String &date, const String &time);
    String getFormattedDateTime();

    uint16_t offsetTime = 3 * 3600;
    bool isInternetAvailable = false;

private:
    AppState& appState;

    char* loadTimeFromStorage();
    void saveTimeToStorage(const char* timeStr);

    WiFiUDP ntpUDP;
    NTPClient timeClient;

    #ifdef ESP32
    Preferences prefs;
    #define STORAGE_NAMESPACE "time_storage"
    #else
    #define STORAGE_ADDRESS 0
    #endif
};
