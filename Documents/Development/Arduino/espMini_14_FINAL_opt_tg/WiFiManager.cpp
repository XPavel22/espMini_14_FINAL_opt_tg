#include "WiFiManager.h"

WiFiManager::WiFiManager(Settings& ws, TimeModule& tm, AppState& appState)
  : settings(ws),
    timeModule(tm),
    appState(appState)
    {
       lastIsAP = settings.ws.isAP;
      }

void WiFiManager::begin() {
    if (!settings.ws.isWifiTurnedOn) {return;
    }

    appState.isStartWifi = true;

#ifdef ESP32
    WiFi.setHostname(settings.ws.mDNS.c_str());
#else
    WiFi.hostname(settings.ws.mDNS.c_str());
#endif

    if (settings.ws.isAP) {startAccessPoint();
        return;
    }connectToWiFi();

     lastIsAP = settings.ws.isAP;
}

void WiFiManager::loop() {
    checkScanCompletion();

    if (settings.ws.isAP != lastIsAP) {
        Serial.printf("WiFiManager: Mode changed from %s to %s. Applying new mode.\n",
                      lastIsAP ? "AP" : "STA",
                      settings.ws.isAP ? "AP" : "STA");

        if (settings.ws.isAP) {

            startAccessPoint();
        } else {

            connectToWiFi();
        }

        lastIsAP = settings.ws.isAP;
        return;
    }

#ifdef ESP8266
    MDNS.update();
#endif

    if (settings.ws.isAP) {
        return;
    }

    if (!settings.ws.isWifiTurnedOn) {
        return;
    }

    if (timeUpdateScheduled && (millis() - ipObtainedTime > timeUpdateDelay)) {timeModule.updateTime();
        timeUpdateScheduled = false;
    }

    if (isInFallbackAP) {
        if (millis() - apStartTime > apTimeout) {
          isInFallbackAP = false;
            connectToWiFi();
        }
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (isConnecting) {
            isConnecting = false;
            appState.isStartWifi = false;

            if (!settings.ws.networkSettings.empty()) {
                NetworkSetting& net = settings.ws.networkSettings[0];
                setIpAdresesInConfig(net);

            }

            timeUpdateScheduled = true;
            ipObtainedTime = millis();if (!MDNS.begin(settings.ws.mDNS.c_str())) {} else {
                MDNS.addService("http", "tcp", 80);}
        }
        return;
    }

    if (isConnecting) {
        if (millis() - connectionStartTime > connectionTimeout) {isConnecting = false;
            isInFallbackAP = true;
            apStartTime = millis();
            startAccessPoint();
        }
    } else {
      connectToWiFi();
    }

}

void WiFiManager::connectToWiFi() {
    timeUpdateScheduled = false;
    appState.isStartWifi = true;

    if (settings.ws.networkSettings.empty()) {
        Serial.println("WiFiManager: No network settings saved. Starting fallback AP.");
        isInFallbackAP = true;
        apStartTime = millis();
        startAccessPoint();
        return;
    }

    NetworkSetting& net = settings.ws.networkSettings[0];

    if (net.ssid.isEmpty()) {
        Serial.println("WiFiManager: SSID is empty. Starting fallback AP.");
        isInFallbackAP = true;
        apStartTime = millis();
        startAccessPoint();
        return;
    }

    setupStationMode(net);

    bool connectionStarted = false;
    if (!net.bssid.isEmpty()) {
        uint8_t bssid[6];
        if (parseBssid(net.bssid, bssid)) {
            Serial.printf("WiFiManager: Connecting to SSID '%s' using BSSID '%s' on channel %d\n", net.ssid.c_str(), net.bssid.c_str(), net.channel);

            WiFi.begin(net.ssid.c_str(), net.password.c_str(), net.channel, bssid);
            connectionStarted = true;
        } else {
            Serial.printf("WiFiManager: Failed to parse BSSID '%s'. Falling back to SSID-only connection.\n", net.bssid.c_str());
        }
    }

    if (!connectionStarted) {

        Serial.printf("WiFiManager: Connecting to SSID '%s' (no BSSID)\n", net.ssid.c_str());
        WiFi.begin(net.ssid.c_str(), net.password.c_str());
    }

    isConnecting = true;
    isInFallbackAP = false;
    connectionStartTime = millis();
}

void WiFiManager::startAccessPoint() {

    appState.isStartWifi = true;

    setupAccessPointMode();

    IPAddress apIP = WiFi.softAPIP();

       if (!MDNS.begin(settings.ws.mDNS.c_str())) {

        } else {
        MDNS.addService("http", "tcp", 80);}

    appState.isStartWifi = false;
}

bool WiFiManager::isConnected() const {
    return (WiFi.status() == WL_CONNECTED);
}

String WiFiManager::getStatusString() const {
    switch (WiFi.status()) {
        case WL_CONNECTED: return "Connected";
        case WL_NO_SHIELD: return "No WiFi Shield";
        case WL_IDLE_STATUS: return "Idle";
        case WL_NO_SSID_AVAIL: return "No SSID Available";
        case WL_SCAN_COMPLETED: return "Scan Completed";
        case WL_CONNECT_FAILED: return "Connection Failed";
        case WL_CONNECTION_LOST: return "Connection Lost";
        case WL_DISCONNECTED: return "Disconnected";
        default: return "Unknown Status";
    }
}

void WiFiManager::setupStationMode(const NetworkSetting& net) {
    WiFi.disconnect();

#ifdef ESP32
    WiFi.mode(WIFI_MODE_STA);
#else
    WiFi.mode(WIFI_STA);
#endif

#ifdef ESP8266
    WiFi.setAutoConnect(true);
#endif

    WiFi.setAutoReconnect(true);

#ifdef ESP32
    WiFi.setHostname(settings.ws.mDNS.c_str());
#else
    WiFi.hostname(settings.ws.mDNS.c_str());
#endif

    if (net.useStaticIP) {
      WiFi.config(net.staticIP, net.staticGateway, net.staticSubnet, net.staticDNS);
    } else {}
}

void WiFiManager::setupAccessPointMode() {
    WiFi.disconnect();

#ifdef ESP32
    WiFi.mode(WIFI_MODE_AP);
#else
    WiFi.mode(WIFI_AP);
#endif

    WiFi.softAPConfig(
        settings.ws.staticIpAP,
        settings.ws.staticIpAP,
        IPAddress(255, 255, 255, 0)
    );

    if (settings.ws.passwordAP.isEmpty()) {
        WiFi.softAP(settings.ws.ssidAP.c_str());
    } else {
        WiFi.softAP(settings.ws.ssidAP.c_str(), settings.ws.passwordAP.c_str());
        }
}

void WiFiManager::startScan() {scanResultsBuffer = "";
    scanResultsReady = false;

    originalMode = WiFi.getMode();

     WiFi.mode(WIFI_AP_STA);WiFi.scanDelete();
    WiFi.scanNetworks(true);
    isScanning = true;
    scanStartTime = millis();
}

void WiFiManager::checkScanCompletion() {
    if (!isScanning) {
        return;
    }

    auto restoreMode = [this]() {WiFi.mode(originalMode);
        isScanning = false;
    };

    if (millis() - scanStartTime > SCAN_TIMEOUT_MS) {restoreMode();
        WiFi.scanDelete();
        scanResultsBuffer = "\"status\":\"timeout\"";
        scanResultsReady = true;
        return;
    }

    int scanResult = WiFi.scanComplete();

    if (scanResult == WIFI_SCAN_RUNNING) {
        return;
    }

    restoreMode();

    if (scanResult == WIFI_SCAN_FAILED) {scanResultsBuffer = "\"status\":\"failed\"";
    } else if (scanResult >= 0) {String json = "[";
        for (int i = 0; i < scanResult; ++i) {
            if (i) json += ",";
            if (!WiFi.SSID(i) || WiFi.SSID(i).length() == 0) continue;
            json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
            json += "\"bssid\":\"" + WiFi.BSSIDstr(i) + "\",";
            json += "\"channel\":" + String(WiFi.channel(i)) + ",";
            json += "\"password\":\"\",";
            json += "\"signalLevel\":" + String(WiFi.RSSI(i)) + "}";
        }
        json += "]";
        scanResultsBuffer = json;
    }

    WiFi.scanDelete();
    scanResultsReady = true;
}

String WiFiManager::getScanResults() {
    if (scanResultsReady) {
        scanResultsReady = false;
        String results = scanResultsBuffer;
        scanResultsBuffer = "";
        return results;
    }
    return "";
}

bool WiFiManager::isScanInProgress() const {
    return isScanning;
}

void WiFiManager::setIpAdresesInConfig(NetworkSetting& net) {

    if (!net.useStaticIP) {
        net.staticIP = WiFi.localIP();
        net.staticGateway = WiFi.gatewayIP();
        net.staticSubnet = WiFi.subnetMask();
        net.staticDNS = WiFi.dnsIP();
        }
}

bool WiFiManager::parseBssid(const String& bssidStr, uint8_t* bssidArray) {
    if (bssidStr.length() != 17) {
        return false;
    }

    char* endPtr;
    for (int i = 0; i < 6; ++i) {
        bssidArray[i] = (uint8_t)strtol(bssidStr.substring(i * 3, i * 3 + 2).c_str(), &endPtr, 16);
        if (endPtr == bssidStr.substring(i * 3, i * 3 + 2).c_str()) {
            return false;
        }
    }
    return true;
}
