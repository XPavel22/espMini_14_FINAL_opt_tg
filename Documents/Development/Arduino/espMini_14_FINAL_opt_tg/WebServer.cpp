#include "WebServer.h"

WebServer::WebServer(Settings& settings,
                     DeviceManager& deviceManager,
                     AppState& appState,
                     TimeModule& timeModule,
                     Info& info,
                     Ota& ota,
                     WiFiManager& wifiManager
                    )
  : server(80),
    settings(settings),
    deviceManager(deviceManager),
    appState(appState),
    timeModule(timeModule),
    info(info),
    ota(ota),
    wifiManager(wifiManager)
{

}

void WebServer::stop() {
  server.end();
}

void WebServer::begin() {
if (!settings.ws.isWifiTurnedOn) {
return;
  }

  server.on("/", HTTP_GET, [this](AsyncWebServerRequest * request) {
    _webServerIsBusy = true;
    _forceLiveDataUpdate = true;
    request->send(getIndexResponse(request));
    _webServerIsBusy = false;
  });

  server.onNotFound([this](AsyncWebServerRequest * request) {

  _forceLiveDataUpdate = true;
    request->send(getIndexResponse(request));
  });

  server.on("/settings", HTTP_GET, [this](AsyncWebServerRequest * request) {
    handleGetSettings(request);
  });

  server.on("/device", HTTP_GET, [this](AsyncWebServerRequest * request) {
    handleGetDeviceSettings(request);
  });

  server.on("/live", HTTP_GET, [this](AsyncWebServerRequest * request) {
// добавить - если пришел параметр force то принудительно отправить данные
    handleGetLiveData(request);
  });

  server.on("/logs", HTTP_GET, [this](AsyncWebServerRequest * request) {
    handleGetLogs(request);
  });

  server.on("/saveSettings", HTTP_POST, [this](AsyncWebServerRequest * request) {
    handleSaveSettings(request);
  });

server.on("/saveDevice", HTTP_POST,

  [this](AsyncWebServerRequest * request) {

    if (request->hasHeader("Content-Length")) {
      size_t contentLength = request->header("Content-Length").toInt();

      size_t freeHeap = ESP.getFreeHeap();

      size_t requiredMemory = contentLength + SIZE_JSON_S + ESP8266_SAFETY_MARGIN_HEAP;

      Serial.printf("[MEMORY] Check: Required=%u, Free=%u\n", requiredMemory, freeHeap);

      if (freeHeap < requiredMemory) {
        Serial.printf("❌ ОШИБКА: Недостаточно памяти. Требуется: %u байт, доступно: %u байт. Отклоняю запрос.\n", requiredMemory, freeHeap);

        sendError(request, 507, "Insufficient Storage. Device memory is low. Try simplifying the configuration or rebooting the device.");
        request->client()->close();
        return;
      }
      Serial.println("[MEMORY] Памяти достаточно, продолжаю обработку.");

    } else {
      Serial.println("[MEMORY] Заголовок Content-Length отсутствую, не могу проверить память.");
    }
  },

  NULL,
  [this](AsyncWebServerRequest * request, uint8_t* data, size_t len, size_t index, size_t total) {
    handleSaveDeviceSettings(request, data, len, index, total);
  }
);

  server.on("/updateDevice", HTTP_POST, [this](AsyncWebServerRequest * request) {

    handleUpdateDeviceProperty(request);
  });

  server.on("/set-datetime", HTTP_POST, [this](AsyncWebServerRequest * request) {
    handleSaveDateTime(request);
  });

  server.on("/relay", HTTP_POST, [this](AsyncWebServerRequest * request) {
    handleControlRelay(request);
  });

  server.on("/clearLog", HTTP_POST, [this](AsyncWebServerRequest * request) {
    handleClearLogs(request);
  });

  server.on("/reboot", HTTP_POST, [this](AsyncWebServerRequest * request) {
    handleReboot(request);
  });

  server.on("/reset", HTTP_POST, [this](AsyncWebServerRequest * request) {
    handleFullReset(request);
  });

  server.on("/resetDevice", HTTP_POST, [this](AsyncWebServerRequest * request) {
    handleResetDevice(request);
  });

  server.on("/sysinfo", HTTP_POST, [this](AsyncWebServerRequest * request) {
    handleSysinfo(request);
  });

  server.on("/scan", HTTP_POST, [this](AsyncWebServerRequest * request) {
if (wifiManager.isScanInProgress()) {
sendError(request, 503, "Scanning already in progress, please wait.");
      return;
    }

    wifiManager.startScan();
    sendSuccess(request, "Scan started");
});

  server.on("/uploadFile", HTTP_POST,

  [this](AsyncWebServerRequest * request) {
    _webServerIsBusy = true;
request->onDisconnect([this]() {
      _webServerIsBusy = false;
ota.resetUploadState();
    });
  },

  [this](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    ota.handleFileUpload(request, filename, index, data, len, final);
  }
           );

  server.on("/download", HTTP_POST, [](AsyncWebServerRequest * request) {
    if (request->hasParam("file", true)) {
      String filename = request->getParam("file", true)->value();
      filename = "/" + filename;

#ifdef ESP32
      if (!SPIFFS.begin(true)) {
#elif defined(ESP8266)
      if (!SPIFFS.begin()) {
#endif
}

      if (!SPIFFS.exists(filename)) {
        request->send(404, "text/plain", "Файл не найден");
        return;
      }

      request->send(SPIFFS, filename, "application/octet-stream");
    } else {
      request->send(400, "text/plain", "Некорректный запрос");
    }
  });

  server.begin();
  yield();
}

bool WebServer::isBusy() const {
  return _webServerIsBusy;
}

void WebServer::printRequestParameters(AsyncWebServerRequest * request) {
  int params = request->params();
  for (int i = 0; i < params; i++) {
    const AsyncWebParameter* p = request->getParam(i);
    if (p->isPost()) {
} else {
}

}
}

void WebServer::sendError(AsyncWebServerRequest * request, int code, const String & message) {
  StaticJsonDocument<200> doc;
  doc["status"] = "error";
  doc["code"] = code;
  doc["message"] = message;
  sendJson(request, doc);
}

void WebServer::sendSuccess(AsyncWebServerRequest * request, const String & message) {
  StaticJsonDocument<200> doc;
  doc["status"] = "success";
  doc["message"] = message;
  sendJson(request, doc);
}

void WebServer::sendJson(AsyncWebServerRequest * request, JsonDocument & doc) {
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

AsyncWebServerResponse * WebServer::getIndexResponse(AsyncWebServerRequest * request) {
  bool hasHtml = SPIFFS.exists("/index.html");
  bool hasGz = SPIFFS.exists("/index.html.gz");
  time_t htmlTime = 0, gzTime = 0;
  time_t now = time(nullptr);

  if (hasHtml) {
    File htmlFile = SPIFFS.open("/index.html", "r");
    if (htmlFile) {
      htmlTime = htmlFile.getLastWrite();
      htmlFile.close();
    }
  }
  if (hasGz) {
    File gzFile = SPIFFS.open("/index.html.gz", "r");
    if (gzFile) {
      gzTime = gzFile.getLastWrite();
      gzFile.close();
    }
  }

  AsyncWebServerResponse *response = nullptr;
  String selectedFile = "";
  String reason = "";

  if (hasHtml && (!hasGz || htmlTime > gzTime)) {
    response = request->beginResponse(SPIFFS, "/index.html", "text/html");
    selectedFile = "/index.html";
    reason = (!hasGz) ? "GZ file not exists" : "HTML is newer";
  } else if (hasGz) {
    response = request->beginResponse(SPIFFS, "/index.html.gz", "text/html");
    response->addHeader("Content-Encoding", "gzip");
    selectedFile = "/index.html.gz";
    reason = (!hasHtml) ? "HTML file not exists" : "GZ is newer or equal";
  } else {
    response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    selectedFile = "EMBEDDED";
    reason = "No files in SPIFFS";
  }

  if (response) {

    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");

    response->addHeader("Vary", "Accept-Language");
    response->addHeader("Content-Language", "ru-RU");

    response->addHeader("Accept-Ranges", "none");

    response->addHeader("Content-Disposition", "inline");

    char dateStr[40];
    time_t now = time(nullptr);
    struct tm *tm_info = gmtime(&now);
    strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
    response->addHeader("Date", dateStr);

    response->addHeader("Server", "ESP32-AsyncWebServer");

    if (selectedFile != "EMBEDDED" && selectedFile != "") {
      File file = SPIFFS.open(selectedFile, "r");
      if (file) {
        time_t lastMod = file.getLastWrite();
        file.close();

        char lastModStr[40];
        struct tm *tm_mod = gmtime(&lastMod);
        strftime(lastModStr, sizeof(lastModStr), "%a, %d %b %Y %H:%M:%S GMT", tm_mod);
        response->addHeader("Last-Modified", lastModStr);

        char etag[30];
        snprintf(etag, sizeof(etag), "\"%lx\"", (unsigned long)lastMod);
        response->addHeader("ETag", etag);
      }
    }
  }

  return response;
}

void WebServer::handleGetSettings(AsyncWebServerRequest * request) {
  _webServerIsBusy = true;

String json = settings.serializeSettings(settings.ws);
  request->send(200, "application/json", json);

  _webServerIsBusy = false;
}

void WebServer::handleGetDeviceSettings(AsyncWebServerRequest * request) {
processRequestSetting = true;

  const Device& currentDevice = deviceManager.myDevices[deviceManager.currentDeviceIndex];

  String status = deviceManager.serializeDevice(currentDevice, nullptr, request);
  if (status != "sending") {
request->send(500, "text/plain", "Failed to start streaming");
  } else {
}

  processRequestSetting = false;
}

void WebServer::handleGetLiveData(AsyncWebServerRequest * request) {
  if (appState.isSaveWifiRequest || deviceManager.isSaveControl || processRequestSetting) {
    Serial.println("[WebServer:LiveData] Request rejected: Server is busy.");
    request->send(503, "text/plain", "Server Busy");
    return;
  }
  _webServerIsBusy = true;

  bool forceUpdate = _forceLiveDataUpdate;

  const Device& currentDevice = deviceManager.myDevices[deviceManager.currentDeviceIndex];

  static uint32_t lastSentRelayChecksum = 0;
  static uint32_t lastSentSensorChecksum = 0;
  static uint32_t lastSentTimerChecksum = 0;
  static uint32_t lastSentSettingsChecksum = 0;

  DynamicJsonDocument doc(4096);
  bool hasUpdates = false;

  uint32_t currentRelayChecksum = deviceManager.calculateOutputRelayChecksum();

  if (forceUpdate || currentRelayChecksum != lastSentRelayChecksum) {
    if (!forceUpdate) Serial.println("[WebServer:LiveData] Event: Relays data changed.");
    JsonObject relaysUpdate = doc.createNestedObject("relays_update");
    deviceManager.serializeRelaysForControlTab(relaysUpdate);
    lastSentRelayChecksum = currentRelayChecksum;
    hasUpdates = true;
  }

  uint32_t currentSensorChecksum = deviceManager.calculateSensorValuesChecksum();
  if (forceUpdate || currentSensorChecksum != lastSentSensorChecksum) {
    //if (!forceUpdate) Serial.println("[WebServer:LiveData] Event: Sensors data changed.");
    JsonObject sensorsUpdate = doc.createNestedObject("sensors_update");
    deviceManager.serializeSensorValues(sensorsUpdate);
    lastSentSensorChecksum = currentSensorChecksum;
    hasUpdates = true;
  }

  uint32_t currentTimerChecksum = deviceManager.calculateTimersProgressChecksum();

  if (currentDevice.isTimersEnabled && (forceUpdate || currentTimerChecksum != lastSentTimerChecksum)) {
    //if (!forceUpdate) Serial.println("[WebServer:LiveData] Event: Timers data changed.");
    JsonObject timersUpdate = doc.createNestedObject("timers_update");
    deviceManager.serializeTimersProgress(timersUpdate);
    lastSentTimerChecksum = currentTimerChecksum;
    hasUpdates = true;
  }

  uint32_t currentSettingsChecksum = deviceManager.calculateDeviceFlagsChecksum();
  if (forceUpdate || currentSettingsChecksum != lastSentSettingsChecksum) {
    if (!forceUpdate) Serial.println("[WebServer:LiveData] Event: Device settings flags changed.");
    JsonObject settingsUpdate = doc.createNestedObject("settings_update");
    deviceManager.serializeDeviceFlags(settingsUpdate);
    lastSentSettingsChecksum = currentSettingsChecksum;
    hasUpdates = true;
  }

  JsonObject staticInfo = doc.createNestedObject("static_info");
  staticInfo["freeHeap"] = ESP.getFreeHeap();
  staticInfo["systemLoad"] = settings.ws.systemLoading;
  staticInfo["uptime"] = millis() / 1000;
  staticInfo["dateTime"] = timeModule.getFormattedDateTime();
  staticInfo["wifiStatus"] = WiFi.status();
  staticInfo["localIP"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();

  String scanData = wifiManager.getScanResults();
  if (!scanData.isEmpty()) {
    Serial.println("[WebServer:LiveData] Event: Wi-Fi scan results available.");
    if (scanData.startsWith("\"status\":")) {
      doc["scan_status"] = scanData.substring(9, scanData.length() - 1);
    } else {
      DynamicJsonDocument scanDoc(2048);
      DeserializationError error = deserializeJson(scanDoc, scanData);
      if (!error) {
        doc["wifi_scan_results"] = scanDoc.as<JsonArray>();
      } else {
        Serial.printf("[WebServer:LiveData] Error parsing Wi-Fi scan JSON: %s\n", error.c_str());
        doc["scan_status"] = "parse_error";
      }
    }
  }

  sendJson(request, doc);

  _forceLiveDataUpdate = false;

  _webServerIsBusy = false;
}

void WebServer::handleGetLogs(AsyncWebServerRequest * request) {
  _webServerIsBusy = true;
StaticJsonDocument<200> doc;
  doc["message"] = "Logs endpoint not implemented yet";
  sendJson(request, doc);

  _webServerIsBusy = false;
}

void WebServer::handleSysinfo(AsyncWebServerRequest * request) {
  _webServerIsBusy = true;
StaticJsonDocument<800> doc;

  char statusBuffer[800];

  info.getSystemStatus(statusBuffer, sizeof(statusBuffer));

  doc["message"] = statusBuffer;

  sendJson(request, doc);

  _webServerIsBusy = false;
}

void WebServer::handleSaveSettings(AsyncWebServerRequest * request) {
  _webServerIsBusy = true;
if (request->hasParam("body", true)) {
    String body = request->getParam("body", true)->value();

    DynamicJsonDocument doc(SIZE_JSON_S);
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
      sendError(request, 400, "Invalid JSON: " + String(error.c_str()));
      _webServerIsBusy = false;
      return;
    }

    if (settings.deserializeSettings(doc.as<JsonObject>(), settings.ws)) {

      appState.isSaveWifiRequest = true;
      sendSuccess(request, "Settings saved");

    } else {
      sendError(request, 500, "Failed to apply settings");
    }
  } else {
    sendError(request, 400, "Missing body parameter");
  }

  _webServerIsBusy = false;
}

void WebServer::handleSaveDeviceSettings(AsyncWebServerRequest * request, uint8_t* data, size_t len, size_t index, size_t total) {
  processRequestSetting = true;

  static char* jsonBuffer = nullptr;
  static size_t expectedTotalSize = 0;

  if (index == 0) {

    if (jsonBuffer) {
      free(jsonBuffer);
      jsonBuffer = nullptr;
    }

    expectedTotalSize = total;

    jsonBuffer = (char*)malloc(expectedTotalSize + 1);

    if (!jsonBuffer) {
request->send(500, "application/json", R"({"error":"Memory allocation failed on server"})");
      processRequestSetting = false;
      return;
    }
}

  if (jsonBuffer) {
    memcpy(jsonBuffer + index, data, len);
  }

  if (index + len != expectedTotalSize) {
    return;
  }

  jsonBuffer[expectedTotalSize] = '\0';
{
    DynamicJsonDocument doc(SIZE_JSON_S);
    DeserializationError error = deserializeJson(doc, jsonBuffer);

    if (error) {
request->send(400, "application/json", R"({"error":"Invalid JSON content"})");
    } else {

      Device& currentDevice = deviceManager.myDevices[deviceManager.currentDeviceIndex];
      if (deviceManager.deserializeDevice(doc.as<JsonObject>(), currentDevice)) {
        appState.isSaveControlRequest = true;
request->send(200, "application/json", R"({"status":"ok", "message":"Настройки сохранены"})");
      } else {
request->send(500, "application/json", R"({"error":"Failed to apply settings"})");
      }
    }
  }

  if (jsonBuffer) {
    free(jsonBuffer);
    jsonBuffer = nullptr;
  }
processRequestSetting = false;
}

void WebServer::handleUpdateDeviceProperty(AsyncWebServerRequest * request) {
if (!request->hasParam("body", true)) {
request->send(400, "application/json", R"({"error":"Missing 'body' parameter in form data"})");
    return;
  }

  String jsonPayload = request->getParam("body", true)->value();
if (jsonPayload.length() == 0 || !jsonPayload.startsWith("{") || !jsonPayload.endsWith("}")) {
request->send(400, "application/json", R"({"error":"Invalid JSON format in 'body' parameter"})");
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, jsonPayload);

  if (error) {
request->send(400, "application/json", R"({"error":"Invalid JSON content"})");
    return;
  }

  if (deviceManager.currentDeviceIndex >= deviceManager.myDevices.size()) {
request->send(400, "application/json", R"({"error":"Invalid device index"})");
    return;
  }

  Device& currentDevice = deviceManager.myDevices[deviceManager.currentDeviceIndex];
  bool anyFieldUpdated = false;
  String updatedPropertiesList = "";

  for (JsonPair kv : doc.as<JsonObject>()) {
    const char* key = kv.key().c_str();
    JsonVariant value = kv.value();
    bool updated = false;

    String keyStr = String(key);
    int bracketStart = keyStr.indexOf('[');
    int bracketEnd = keyStr.indexOf(']');

    if (bracketStart != -1 && bracketEnd != -1 && bracketEnd > bracketStart) {
      String arrayName = keyStr.substring(0, bracketStart);

      String indexStr = keyStr.substring(bracketStart + 1, bracketEnd);
      bool isNumeric = true;
      for (unsigned int i = 0; i < indexStr.length(); i++) {
        if (!isDigit(indexStr.charAt(i))) {
          isNumeric = false;
          break;
        }
      }

      if (!isNumeric) {
continue;
      }

      int index = indexStr.toInt();

      if (index < 0) {
continue;
      }

      if (bracketEnd + 2 >= keyStr.length()) {
continue;
      }

      String propertyName = keyStr.substring(bracketEnd + 2);

      if (arrayName == "act") {

        if (index >= 0 && index < currentDevice.actions.size()) {
          Action& action = currentDevice.actions[index];
          if (propertyName == "use" && value.is<bool>()) {
            action.isUseSetting = value.as<bool>();
            updated = true;
          } else if (propertyName == "dsc" && value.is<String>()) {
            strlcpy(action.description, value.as<String>().c_str(), MAX_DESCRIPTION_LENGTH);
            updated = true;
          }
        } else {
          Serial.printf("⚠️ Пропускаем свойство с индексом за пределами массива actions: %s (индекс: %d, размер: %d)\n",
                        key, index, currentDevice.actions.size());
        }
      }
      else if (arrayName == "tmr") {

        if (index >= 0 && index < currentDevice.timers.size()) {
          Timer& timer = currentDevice.timers[index];
          if (propertyName == "use" && value.is<bool>()) {
            timer.isUseSetting = value.as<bool>();
            updated = true;
          }
        } else {
          Serial.printf("⚠️ Пропускаем свойство с индексом за пределами массива timers: %s (индекс: %d, размер: %d)\n",
                        key, index, currentDevice.timers.size());
        }
      }
      else if (arrayName == "sch") {

        if (index >= 0 && index < currentDevice.scheduleScenarios.size()) {
          ScheduleScenario& schedule = currentDevice.scheduleScenarios[index];
          if (propertyName == "use" && value.is<bool>()) {
            schedule.isUseSetting = value.as<bool>();
            updated = true;
          }
        } else {
          Serial.printf("⚠️ Пропускаем свойство с индексом за пределами массива scheduleScenarios: %s (индекс: %d, размер: %d)\n",
                        key, index, currentDevice.scheduleScenarios.size());
        }
      }
      else if (arrayName == "sen") {

        if (index >= 0 && index < currentDevice.sensors.size()) {
          Sensor& sensor = currentDevice.sensors[index];
          if (propertyName == "use" && value.is<bool>()) {
            sensor.isUseSetting = value.as<bool>();
            updated = true;
          }
        } else {
          Serial.printf("⚠️ Пропускаем свойство с индексом за пределами массива sensors: %s (индекс: %d, размер: %d)\n",
                        key, index, currentDevice.sensors.size());
        }
      }
      else if (arrayName == "rel") {

        if (index >= 0 && index < currentDevice.relays.size()) {
          Relay& relay = currentDevice.relays[index];
          if (propertyName == "man" && value.is<bool>()) {
            relay.manualMode = value.as<bool>();
            updated = true;
          } else if (propertyName == "stp" && value.is<bool>()) {
            relay.statePin = value.as<bool>();
            updated = true;
          }
        } else {
          Serial.printf("⚠️ Пропускаем свойство с индексом за пределами массива relays: %s (индекс: %d, размер: %d)\n",
                        key, index, currentDevice.relays.size());
        }
      }
      else if (arrayName == "pid") {

        if (index >= 0 && index < currentDevice.pids.size()) {
          Pid& pid = currentDevice.pids[index];
          if (propertyName == "Kp" && value.is<double>()) {
            pid.Kp = value.as<double>();
            updated = true;
          }
        } else {
          Serial.printf("⚠️ Пропускаем свойство с индексом за пределами массива pids: %s (индекс: %d, размер: %d)\n",
                        key, index, currentDevice.pids.size());
        }
      }
      else {
}
    }
    else if (strcmp(key, "ite") == 0 && value.is<bool>()) {
      currentDevice.isTimersEnabled = value.as<bool>();
      updated = true;
    } else if (strcmp(key, "iet") == 0 && value.is<bool>()) {
      currentDevice.isEncyclateTimers = value.as<bool>();
      updated = true;
    } else if (strcmp(key, "ise") == 0 && value.is<bool>()) {
      currentDevice.isScheduleEnabled = value.as<bool>();
      updated = true;
    } else if (strcmp(key, "iae") == 0 && value.is<bool>()) {
      currentDevice.isActionEnabled = value.as<bool>();
      updated = true;
    } else if (strcmp(key, "tmp.use") == 0 && value.is<bool>()) {
      currentDevice.temperature.isUseSetting = value.as<bool>();
      updated = true;
    } else {
}

    if (updated) {
      anyFieldUpdated = true;
      updatedPropertiesList += String(key) + " ";
    }
  }

  if (anyFieldUpdated) {
    String response = "{\"status\":\"ok\",\"message\":\"Properties updated: " + updatedPropertiesList + "\"}";
    request->send(200, "application/json", response);
} else {
    request->send(400, "application/json", R"({"error":"No valid properties were provided for update"})");
}
}

void WebServer::handleSaveDateTime(AsyncWebServerRequest * request) {

  printRequestParameters(request);

  _webServerIsBusy = true;
if (request->hasParam("body", true)) {
    String body = request->getParam("body", true)->value();

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
      sendError(request, 400, "Invalid JSON: " + String(error.c_str()));
      _webServerIsBusy = false;
      return;
    }

    String dateInput = doc["date"];
    String timeInput = doc["time"];
    int8_t timeZoneValue = doc["timeZone"].as<uint8_t>();
if (timeZoneValue < -12 || timeZoneValue > 14) {
      timeZoneValue = 3;
    }

    settings.ws.timeZone = timeZoneValue;

    bool success = false;
    if (!dateInput.isEmpty() && !timeInput.isEmpty()) {
      success = timeModule.setTimeManually(dateInput, timeInput);
    }

    if (success) {
      sendSuccess(request, "Time settings saved");
    } else {
      sendError(request, 400, "Invalid date/time format");
    }
  } else {
    sendError(request, 400, "Missing body parameter");
  }

  _webServerIsBusy = false;
}

void WebServer::handleControlRelay(AsyncWebServerRequest * request) {
  _webServerIsBusy = true;
if (request->hasParam("body", true)) {
    String body = request->getParam("body", true)->value();

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
      sendError(request, 400, "Invalid JSON: " + String(error.c_str()));
      _webServerIsBusy = false;
      return;
    }

    bool success = deviceManager.handleRelayCommand(doc.as<JsonObject>(), 0);

    if (success) {
      sendSuccess(request, "Relay command executed");
    } else {
      sendError(request, 400, "Failed to execute relay command");
    }
  } else {
    sendError(request, 400, "Missing body parameter");
  }

  _webServerIsBusy = false;
}

void WebServer::handleClearLogs(AsyncWebServerRequest * request) {
  _webServerIsBusy = true;
sendSuccess(request, "Logs cleared (not implemented)");

  _webServerIsBusy = false;
}

void WebServer::handleReboot(AsyncWebServerRequest * request) {
  _webServerIsBusy = true;
sendSuccess(request, "Rebooting...");
  appState.isReboot = true;

  _webServerIsBusy = false;
}

void WebServer::handleFullReset(AsyncWebServerRequest * request) {
  _webServerIsBusy = true;
sendSuccess(request, "Reset initiated");

  _webServerIsBusy = false;
  appState.isFormat = true;
}

void WebServer::handleResetDevice(AsyncWebServerRequest * request) {
  _webServerIsBusy = true;
if (SPIFFS.exists("/devices.json")) {
    bool success = SPIFFS.remove("/devices.json");
    if (success) {
      sendSuccess(request, "Reset Device executed");
      appState.isReboot = true;
    } else {
      sendError(request, 400, "Failed to execute ResetDevice");
    }
  }

  _webServerIsBusy = false;
}

void WebServer::loop() {

}
