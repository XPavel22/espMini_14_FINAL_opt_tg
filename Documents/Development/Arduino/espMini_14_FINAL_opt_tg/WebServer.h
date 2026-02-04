#pragma once

#include "CommonTypes.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "ConfigSettings.h"
#include "DeviceManager.h"
#include "AppState.h"
#include "TimeModule.h"
#include "Info.h"
#include "Ota.h"
#include "WiFiManager.h"
#include "index_html_gz.h"

#define MAX_JSON_PAYLOAD_SIZE_ESP8266 3500
#define ESP8266_SAFETY_MARGIN_HEAP 5000

class WebServer {
public:
    WebServer(Settings& settings,
              DeviceManager& deviceManager,
              AppState& appState,
              TimeModule& timeModule,
              Info& info,
              Ota& ota,
              WiFiManager& wifiManager
              );

    void stop();
    void begin();

    void loop();
    bool isBusy() const;

private:
    AsyncWebServer server;
    Settings& settings;
    DeviceManager& deviceManager;
    AppState& appState;
    TimeModule& timeModule;
    Info& info;
    Ota& ota;
    WiFiManager& wifiManager;

    bool _webServerIsBusy = false;
    bool _forceLiveDataUpdate = false;
    bool processRequestSetting = false;

    AsyncWebServerResponse* getIndexResponse(AsyncWebServerRequest* request);

    void handleGetSettings(AsyncWebServerRequest* request);
    void handleGetDeviceSettings(AsyncWebServerRequest* request);
    void handleGetLiveData(AsyncWebServerRequest* request);
    void handleGetLogs(AsyncWebServerRequest* request);

    void handleSaveSettings(AsyncWebServerRequest* request);
    void handleSaveDeviceSettings(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
    void handleSaveDateTime(AsyncWebServerRequest* request);
    void handleUpdateDeviceProperty(AsyncWebServerRequest* request);

    void handleControlRelay(AsyncWebServerRequest* request);
    void handleClearLogs(AsyncWebServerRequest* request);
    void handleReboot(AsyncWebServerRequest* request);
    void handleFullReset(AsyncWebServerRequest* request);
    void handleSysinfo(AsyncWebServerRequest* request);
    void handleResetDevice(AsyncWebServerRequest* request);

    void printRequestParameters(AsyncWebServerRequest* request);

    void sendError(AsyncWebServerRequest* request, int code, const String& message);
    void sendSuccess(AsyncWebServerRequest* request, const String& message = "OK");
    void sendJson(AsyncWebServerRequest* request, JsonDocument& doc);

};
