#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#ifdef ESP32
#include <ESPmDNS.h>
#elif defined(ESP8266)
#include <ESP8266mDNS.h>
#endif

#include "ConfigSettings.h"
#include "TimeModule.h"
#include "AppState.h"
#include <ESPAsyncWebServer.h>

#ifdef ESP8266
typedef WiFiMode_t wifi_mode_t;
#endif

class WiFiManager {
public:
    WiFiManager(Settings& ws, TimeModule& tm,  AppState& appState);

    void begin();
    void loop();

    void startAccessPoint();
    void connectToWiFi();

    bool isConnected() const;
    String getStatusString() const;

    void startScan();
    void checkScanCompletion();
    String getScanResults();
    bool isScanInProgress() const;

    void setIpAdresesInConfig(NetworkSetting& net);

private:
    Settings& settings;
    TimeModule& timeModule;
    AppState& appState;

    bool isScanning = false;
    unsigned long scanStartTime = 0;
    wifi_mode_t originalMode;
    static const unsigned long SCAN_TIMEOUT_MS = 5000;

    String scanResultsBuffer;
    bool scanResultsReady = false;

    bool lastIsAP = false;
    bool isConnecting = false;
    unsigned long connectionStartTime = 0;
    const unsigned long connectionTimeout = 15000;

    bool isInFallbackAP = false;
    unsigned long apStartTime = 0;
    const unsigned long apTimeout = 300000;

    bool timeUpdateScheduled = false;
    unsigned long ipObtainedTime = 0;
    const unsigned long timeUpdateDelay = 3000;

    void setupStationMode(const NetworkSetting& net);
    void setupAccessPointMode();

    bool parseBssid(const String& bssidStr, uint8_t* bssidArray);
};

#endif
