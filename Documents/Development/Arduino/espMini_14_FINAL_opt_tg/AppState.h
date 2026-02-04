#pragma once
#include <Arduino.h>

    struct AppState {

    bool wifiConnected = false;
    bool isAP = false;
    bool isTemporaryAP = false;
    bool isScanning = false;
    bool isInternetAvailable = false;
    bool isStartWifi = false;

    bool isUpdating = false;
    bool isFormat = false;
    bool isReboot = false;
    bool isSaveControlRequest = false;
    bool isSaveWifiRequest = false;
    bool isProcessWorkingJson = false;
};
