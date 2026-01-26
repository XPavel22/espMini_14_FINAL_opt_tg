#include "TimeModule.h"

TimeModule::TimeModule(AppState& appStateRef)
    : appState(appStateRef),
      timeClient(ntpUDP, "pool.ntp.org", offsetTime, 5000)
{
}

  void TimeModule::beginNTP() {
    timeClient.begin();
  }

  void TimeModule::updateTime() {time_t rawtime;
    struct tm timeinfo;

    rawtime = time(nullptr);if (localtime_r(&rawtime, &timeinfo) == NULL) {}

    bool validSystemTime = false;
    int year = timeinfo.tm_year + 1900;if (year >= 2025) {
      validSystemTime = true;}

    if (!validSystemTime) {char* loadedTime = loadTimeFromStorage();
      if (loadedTime != NULL && strlen(loadedTime) > 0) {memset(&timeinfo, 0, sizeof(timeinfo));
        char* result = strptime(loadedTime, "%Y-%m-%d %H:%M:%S", &timeinfo);
        if (result != NULL) {
          rawtime = mktime(&timeinfo);validSystemTime = true;
        } else {}
        free(loadedTime);
      } else {}
    }

    if (!validSystemTime) {memset(&timeinfo, 0, sizeof(timeinfo));
      timeinfo.tm_year = 2025 - 1900;
      timeinfo.tm_mon = 0;
      timeinfo.tm_mday = 1;
      timeinfo.tm_hour = 12;
      timeinfo.tm_min = 0;
      timeinfo.tm_sec = 0;
      rawtime = mktime(&timeinfo);}

     IPAddress ntpServerIP;

    if (WiFi.isConnected() && WiFi.getMode() == WIFI_STA) {bool ntpUpdated = timeClient.forceUpdate();
      if (ntpUpdated && timeClient.getEpochTime() > 0) {
        isInternetAvailable = true;

        time_t utcTime = timeClient.getEpochTime();localtime_r(&utcTime, &timeinfo);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        saveTimeToStorage(timeStr);

        rawtime = timeClient.getEpochTime();
      }
    } else {
      isInternetAvailable = false;}

    struct timeval tv = { .tv_sec = rawtime, .tv_usec = 0 };
    if (settimeofday(&tv, NULL) == 0) {} else {}

    time_t finalTime = time(nullptr);struct tm finalTimeInfo;
    if (localtime_r(&finalTime, &finalTimeInfo)) {
      char finalTimeStr[64];
      strftime(finalTimeStr, sizeof(finalTimeStr), "%Y-%m-%d %H:%M:%S", &finalTimeInfo);} else {}}

  char* TimeModule::loadTimeFromStorage() {
  #ifdef ESP32
    prefs.begin(STORAGE_NAMESPACE, true);
    String storedTime = prefs.getString("current_time", "");
    prefs.end();

    if (storedTime.length() > 0) {
      return strdup(storedTime.c_str());
    }
  #else
    EEPROM.begin(512);
    char buffer[64] = {0};
    for (int i = 0; i < 63; i++) {
      buffer[i] = EEPROM.read(STORAGE_ADDRESS + i);
      if (buffer[i] == '\0') break;
    }
    EEPROM.end();

    if (strlen(buffer) > 0) {
      return strdup(buffer);
    }
  #endif
    return NULL;
  }

  void TimeModule::saveTimeToStorage(const char* timeStr) {
  #ifdef ESP32
    prefs.begin(STORAGE_NAMESPACE, false);
    prefs.putString("current_time", timeStr);
    prefs.end();
  #else
    EEPROM.begin(512);
    for (int i = 0; i < strlen(timeStr); i++) {
      EEPROM.write(STORAGE_ADDRESS + i, timeStr[i]);
    }
    EEPROM.write(STORAGE_ADDRESS + strlen(timeStr), '\0');
    EEPROM.commit();
    EEPROM.end();
  #endif
  }

  time_t TimeModule::getCurrentTime() {
    return time(nullptr);
  }

  char* TimeModule::getFormattedTime(const char* format) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    static char buffer[64];
    strftime(buffer, sizeof(buffer), format, &timeinfo);
    return buffer;
  }

  bool TimeModule::setTimeManually(const String &date, const String &time) {

    if (date.length() < 10 || time.length() < 5) {return false;
    }

    int year = date.substring(0, 4).toInt();
    int month = date.substring(5, 7).toInt();
    int day = date.substring(8, 10).toInt();

    int hour = time.substring(0, 2).toInt();
    int minute = time.substring(3, 5).toInt();
    int second = (time.length() == 8) ? time.substring(6, 8).toInt() : 0;

    if (year < 2000 || year > 9100 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {return false;
    }

    struct tm newTime = {};
    newTime.tm_year = year - 1900;
    newTime.tm_mon = month - 1;
    newTime.tm_mday = day;
    newTime.tm_hour = hour;
    newTime.tm_min = minute;
    newTime.tm_sec = second;

    time_t newEpoch = mktime(&newTime);

    struct timeval tv = { .tv_sec = newEpoch, .tv_usec = 0 };
    if (settimeofday(&tv, NULL) == 0) {} else {return false;
    }

    struct tm timeinfo;
    if (!localtime_r(&newEpoch, &timeinfo)) return false;
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    saveTimeToStorage(timeStr);return true;
  }

  String TimeModule::getFormattedDateTime() {
    char timeBuffer[64];

    struct tm timeinfo;
    time_t now = time(nullptr);
    localtime_r(&now, &timeinfo);
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(timeBuffer);
  }
