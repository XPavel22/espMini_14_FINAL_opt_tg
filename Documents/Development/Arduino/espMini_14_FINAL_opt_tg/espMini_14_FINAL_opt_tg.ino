/*
  # Форматируем без изменения структуры
  prettier --print-width 120 --html-whitespace-sensitivity css --write index.html
  Далее сжатие gzip -k -9 index.html
  Перевод .gz в .h - % xxd -i index.html.gz > index_html_gz.h

  #ifndef INDEX_HTML_GZ_H
  #define INDEX_HTML_GZ_H

  #include <stdint.h>

  static const uint8_t index_html_gz[] PROGMEM = {
  ... 0x00
  };

  static const unsigned int index_html_gz_len = sizeof(index_html_gz);

  #endif
*/

/*
 * Исправить html сохранение сенсоров
 */

#include "WiFiManager.h"
#include "WebServer.h"
#include "ConfigSettings.h"
#include "TimeModule.h"
#include "DeviceManager.h"
#include "Info.h"
#include "Ota.h"
#include "AppState.h"
#include "Control.h"
#include "Logger.h"
#include "TelegramBot.h"
#include <locale.h>

#if defined(ESP8266)
// Для ESP-01 и других ESP8266 плат
#if defined(ARDUINO_ESP8266_ESP01) || defined(ESP01)
#define LED_PIN 2  // GPIO2 для ESP-01 (синий светодиод)
#else
#define LED_PIN 2  // По умолчанию для большинства ESP8266
#endif
#elif defined(ESP32)
// Для ESP32-S2 и других ESP32 вариаций
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(ARDUINO_ESP32S2_DEV)
#define LED_PIN 15 // GPIO15 для ESP32-S2
#else
#define LED_PIN 2  // По умолчанию для других ESP32
#endif
#endif

#define CONTROL_BUTTON

#define LONG_PRESS_TIME 3000
#define BUTTON_PIN 5
unsigned long buttonPressStartTime = 0;
bool wasButtonPressed = false;

AppState appState;
Settings settings(appState);
Info sysInfo;
Logger logger;
TimeModule timeModule(appState);
Ota ota(settings, appState);
DeviceManager deviceManager(appState);
Control control(deviceManager, logger, appState);
WiFiManager wifiManager(settings, timeModule, appState);
WebServer webServer(settings, deviceManager, appState, timeModule, sysInfo, ota, wifiManager);
TelegramBot telegramBot(settings, webServer, logger, appState, ota, sysInfo, deviceManager);
// -------------------------

void setup() {
  setlocale(LC_TIME, "ru_RU");

  Serial.begin(115200);

  delay(300);

#ifdef CONTROL_BUTTON
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Checking for control button press to format...");

  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Button is held. Starting 3-second timer for formatting...");
    unsigned long startTime = millis();

    while (digitalRead(BUTTON_PIN) == LOW) {

      if (millis() - startTime > LONG_PRESS_TIME) {
        Serial.println("3 seconds have passed. Formatting SPIFFS and restarting...");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        delay(100);
        SPIFFS.format();
        Serial.println("SPIFFS formatted.");
        ESP.restart();
      }
      delay(10);
    }
    Serial.println("Button was released before 3 seconds. Continuing normal startup.");
  }
#endif

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  digitalWrite(LED_PIN, HIGH);
  delay(300);
  digitalWrite(LED_PIN, LOW);

  settings.begin();

  Serial.printf("Free heap before LoadSettings: %d\n", ESP.getFreeHeap());

  if (!settings.loadSettings()) {
    Serial.println("Using default settings");
  }

  deviceManager.deviceInit();

  Serial.printf("Free heap after DeviceInit: %d\n", ESP.getFreeHeap());

  if (settings.ws.isWifiTurnedOn) {
    Serial.println("WiFi is turned ON in settings. Initializing network...");

    wifiManager.begin();

    if (!settings.ws.isAP) {
      Serial.println("Waiting for WiFi connection...");
      unsigned long startTime = millis();
      while (!wifiManager.isConnected() && millis() < (startTime + 5000)) {
        wifiManager.loop();
        delay(10);
      }
    } else {
      Serial.println("AP mode detected, skipping connection wait.");
    }

    timeModule.beginNTP();

  } else {

    Serial.println("WiFi is turned OFF. Skipping network initialization.");

#ifdef ESP32
    WiFi.mode(WIFI_MODE_NULL);
#elif defined(ESP8266)
    WiFi.mode(WIFI_OFF);
#endif
  }

  timeModule.updateTime();

  if (settings.ws.isWifiTurnedOn) {
    webServer.begin();
  }

  control.setup();

  digitalWrite(LED_PIN, LOW);
  delay(500);
  digitalWrite(LED_PIN, HIGH);
}

void loop() {

#ifdef CONTROL_BUTTON

  bool isPressed = control.checkTouchSensor(BUTTON_PIN);

  if (isPressed && !wasButtonPressed) {
    Serial.println("Button press started, timer...");
    buttonPressStartTime = millis();
  }

  if (!isPressed && wasButtonPressed) {
    unsigned long pressDuration = millis() - buttonPressStartTime;
    Serial.printf("Button released. Press duration: %lu ms\n", pressDuration);

    if (pressDuration > LONG_PRESS_TIME) {
      Serial.println("Long press detected! Toggling WiFi state and restarting...");

      settings.ws.isWifiTurnedOn = !settings.ws.isWifiTurnedOn;

      if (settings.saveSettings()) {
        Serial.println("Settings saved successfully.");
      } else {
        Serial.println("ERROR: Failed to save settings!");
      }

      if (!settings.ws.isWifiTurnedOn) {
        Serial.println("Shutting down WiFi before restart...");
        webServer.stop(); // Останавливаем сервер
        WiFi.disconnect(true);
        delay(100);
#ifdef ESP32
        WiFi.mode(WIFI_MODE_NULL);
#elif defined(ESP8266)
        WiFi.mode(WIFI_OFF);
#endif

        Serial.println("WiFi is off. Ready for reboot.");
      }

      ESP.restart();

    } else {
      Serial.println("Short press, ignoring.");
    }
  }

  wasButtonPressed = isPressed;

#endif

  if (appState.isSaveControlRequest && !ota.isUpdate && !appState.isStartWifi) {
    static unsigned long saveTimer = 0;

    if (saveTimer == 0) {
      saveTimer = millis();
    }

    if (millis() - saveTimer >= 200) {
      deviceManager.writeDevicesToFile(deviceManager.myDevices, "/devices.json");

      saveTimer = 0;
      Serial.println("Сохранение выполнено");

      deviceManager.saveDeviceFlagsState(); // сохраняем флаги
      deviceManager.deviceFlagsOff(); // сбрасываем в false
      delay(100);
      control.setupControl(true); // переинициализируем
      delay(100);
      deviceManager.restoreDeviceFlagsState(); // восстанавливаем флаги

      deviceManager.isSaveControl = false;
      appState.isSaveControlRequest = false;
    }
  }

  if (appState.isSaveWifiRequest && !appState.isStartWifi) {
    delay(10);
    settings.saveSettings();
    appState.isSaveWifiRequest = false;

    control.setupControl(true);
  }

  ota.loop();

  if (!ota.isUpdate && !appState.isSaveControlRequest && !appState.isStartWifi && !appState.isProcessWorkingJson) {
    control.loop();
  }

  if (settings.ws.isWifiTurnedOn) {
    wifiManager.loop();
    webServer.loop();

    if (!ota.isUpdate && !deviceManager.isSaveControl && !appState.isStartWifi && timeModule.isInternetAvailable) {
      telegramBot.loop();
    }

  }

  if (appState.isFormat) {
    static unsigned long formatTimer = 0;

    if (formatTimer == 0) {
      formatTimer = millis();
    }

    if (millis() - formatTimer >= 1000) {
      webServer.stop();
      settings.format(true);
      appState.isFormat = false;
    }
  }

  if (appState.isReboot) {
    settings.reboot();
    appState.isReboot = false;
  }

  delay(10);
}
