#include "Ota.h"

volatile bool otaUpdateCompleted = false;
unsigned long otaRebootTime = 0;

Ota::Ota(Settings& settings, AppState& appState)
    : settings(settings),
      appState(appState),
      isUpdate(false),
      statusUpdate(""),
      previousStatus(""),
      statusUpdateChanged(false),
       _uploadHasError(false),
       _isFirmwareUpload(false)
{
}

void Ota::resetUploadState() {
    if (_uploadFile) {
        _uploadFile.close();
    }
    if (_isFirmwareUpload) {
#ifdef ESP32
        Update.abort();
#endif
    }
    _uploadFile = File();
    _isFirmwareUpload = false;
    _uploadHasError = false;
    Serial.println("[OTA] Upload state has been reset.");
}

void Ota::handleFileUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t *fileData, size_t len, bool final) {

    if (_uploadHasError) {
        return;
    }

    if (index == 0) {
        resetUploadState();
        _isFirmwareUpload = filename.endsWith(".bin");

        if (!_isFirmwareUpload && !settings.freeSpaceFS()) {
            _uploadHasError = true;
            request->send(500, "text/plain", "Not enough space");
            return;
        }

#ifdef ESP8266
        Update.runAsync(true);
#endif

        if (_isFirmwareUpload) {
            #ifdef ESP32
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
#elif defined(ESP8266)
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace, U_FLASH)) {
#endif
                _uploadHasError = true;
                request->send(500, "text/plain", "OTA Begin Failed");
                return;
            }
        } else {
            _uploadFile = SPIFFS.open("/" + filename, "w");
            if (!_uploadFile) {
                _uploadHasError = true;
                request->send(500, "text/plain", "File open error");
                return;
            }
        }
    }

    if (len > 0) {
        if (_isFirmwareUpload) {
            if (Update.write(fileData, len) != len) {
                _uploadHasError = true;
                request->send(500, "text/plain", "OTA Write Error");
                return;
            }
        } else if (_uploadFile) {
            if (_uploadFile.write(fileData, len) != len) {
                _uploadHasError = true;
                request->send(500, "text/plain", "SPIFFS Write Error");
                return;
            }
        }
    }

    if (final) {
        bool success = false;
        if (_isFirmwareUpload && !_uploadHasError) {
            if (Update.end(true)) {
                success = true;
                request->send(200, "text/plain", "OTA Complete");
                otaUpdateCompleted = true;
                otaRebootTime = millis();
            } else {
                _uploadHasError = true;
                request->send(500, "text/plain", "OTA End Failed");
            }
        } else if (_uploadFile && !_uploadHasError) {
            _uploadFile.close();
            success = true;
            request->send(200, "text/plain", "File Uploaded");
        }

        if (!success) {
            _uploadHasError = true;
        }

        resetUploadState();
    }
}

void Ota::loop() {
    if (otaUpdateCompleted && (millis() - otaRebootTime > 500)) {
        ESP.restart();
    }
}

String Ota::getFileNameFromUrl(String url) {
    int lastSlash = url.lastIndexOf('/');
    return (lastSlash == -1) ? "firmware.bin" : url.substring(lastSlash + 1);
}

Ota::ProcessStatus Ota::handleUpdate(const String& file_path, const String& fileName, bool debug) {
    const int MAX_ATTEMPTS = 3;
    const unsigned long TIMEOUT = 300000;

      auto checkMemory = [this]() -> ProcessStatus {
    uint32_t freeMemory = ESP.getFreeHeap();
#ifdef ESP8266
    if (freeMemory < MIN_FREE_MEMORY || ESP.getHeapFragmentation() > 50) {
#elif defined(ESP32)
    if (freeMemory < MIN_FREE_MEMORY || ESP.getMinFreeHeap() < MIN_FREE_MEMORY / 2) {
#endif
            delay(1000);
            ESP.restart();
            return HTTP_UPDATE_FAILED;
        }
        return HTTP_DOWNLOAD_IN_PROGRESS;
    };

    if (!dlState.isFirmware && !SPIFFS.exists("/")) {
        if (!SPIFFS.begin()) {
            dlState.cleanup();
            return HTTP_FILE_FAILED;
        }
    }

    if (dlState.attempt == 0) {

        dlState.otaStarted = false;
        dlState.file = File();

        int protocolIndex = file_path.indexOf("://");
        if (protocolIndex == -1) {

            return HTTP_UPDATE_FAILED;
        }

        int hostStart = protocolIndex + 3;
        int hostEnd   = file_path.indexOf('/', hostStart);
        if (hostEnd == -1) {
            dlState.host = file_path.substring(hostStart);
            dlState.path = "/";
        } else {
            dlState.host = file_path.substring(hostStart, hostEnd);
            dlState.path = file_path.substring(hostEnd);
        }

        if (fileName.isEmpty()) {

            int lastSlash = dlState.path.lastIndexOf('/');
            if (lastSlash != -1) {
                dlState.fileName = dlState.path.substring(lastSlash + 1);
            } else {
                dlState.fileName = "download";
            }
        } else {
            dlState.fileName = fileName;
        }

        dlState.isFirmware = dlState.fileName.endsWith(".bin");

    }

    switch (dlState.state) {
        case STATE_IDLE:

               dlState.attempt++;

            dlState.state = STATE_CONNECTING;
            dlState.startTime = millis();
            return HTTP_DOWNLOAD_IN_PROGRESS;

        case STATE_CONNECTING:
            dlState.client.setInsecure();
            dlState.client.setTimeout(180000);
            if (dlState.client.connect(dlState.host.c_str(), 443)) {

                String requestStr = "GET " + dlState.path + " HTTP/1.1\r\n" +
                   "Host: " + dlState.host + "\r\n" +
                   "User-Agent: ESP8266/ESP32\r\n" +
                   "Connection: close\r\n\r\n";

                dlState.client.print(requestStr);
                dlState.state = STATE_READING_HEADERS;
            } else {

                dlState.state = STATE_ERROR;
            }
             break;

        case STATE_READING_HEADERS: {
    while (dlState.client.available()) {
        String line = dlState.client.readStringUntil('\n');
        line.trim();

        if (line.startsWith("HTTP/1.1")) {
            int code = line.substring(9, 12).toInt();

            if (code != 200) {
                dlState.state = STATE_ERROR;
                break;
            }
        } else if (line.startsWith("Content-Length:")) {
            dlState.contentLength = line.substring(16).toInt();

        } else if (line == "") {

            if (dlState.isFirmware && !dlState.otaStarted) {
                if (Update.begin(dlState.contentLength)) {
                    dlState.otaStarted = true;
#ifdef ESP8266
                    Update.runAsync(true);
#endif

                } else {

                    dlState.state = STATE_ERROR;
                    break;
                }
            }
            dlState.state = STATE_DOWNLOADING;
            break;
        }
    }

    if (millis() - dlState.startTime > 5000) {

        dlState.state = STATE_ERROR;
    }
    return HTTP_DOWNLOAD_IN_PROGRESS;
}

        case STATE_DOWNLOADING: {
#ifdef ESP8266
            int bufferSize = constrain(ESP.getFreeHeap() / 8, 256, 512);
#else
            int bufferSize = constrain(ESP.getFreeHeap() / 4, 512, 1024);
#endif
            uint8_t buffer[bufferSize];

            if (!dlState.isFirmware && !dlState.file) {
                if (SPIFFS.exists("/" + dlState.fileName)) SPIFFS.remove("/" + dlState.fileName);
                dlState.file = SPIFFS.open("/" + dlState.fileName, "w");
                if (!dlState.file) {

                    dlState.state = STATE_ERROR;
                }
            }

            if (dlState.state == STATE_DOWNLOADING) {
                int bytesRemaining = dlState.contentLength - dlState.totalReceived;
                int bytesToRead = min(min(dlState.client.available(), bytesRemaining), bufferSize);

                if (bytesToRead > 0) {
                    int bytesRead = dlState.client.readBytes(buffer, bytesToRead);
                    if (bytesRead > 0) {
                        if (dlState.isFirmware) {
                            if (Update.write(buffer, bytesRead) != bytesRead) {

                                dlState.state = STATE_ERROR;
                            }
                        } else {
                            if (dlState.file.write(buffer, bytesRead) != bytesRead) {

                                dlState.file.close();
                                SPIFFS.remove("/" + dlState.fileName);
                                dlState.state = STATE_ERROR;
                            }
                            if (dlState.totalReceived % 4096 == 0) dlState.file.flush();
                        }
                        dlState.totalReceived += bytesRead;
                    }
                }

                if (debug && millis() - dlState.lastProgressTime >= 1000) {
                    int progress = (dlState.contentLength > 0) ?
                                    (dlState.totalReceived * 100) / dlState.contentLength : 0;
                    Serial.printf("Progress: %d%% | Received: %d/%d bytes\n",
                                  progress, dlState.totalReceived, dlState.contentLength);
                    dlState.lastProgressTime = millis();
                }

                if (dlState.totalReceived >= dlState.contentLength) {
                    if (!dlState.isFirmware && dlState.file) {
                        dlState.file.close();
                        if (debug) Serial.println("✅ File download completed");
                    }
                    dlState.state = STATE_COMPLETE;
                }

                if (millis() - dlState.startTime > TIMEOUT) {
                    if (debug) Serial.println("❌ Download timeout");
                    dlState.state = STATE_ERROR;
                }
            }
            return HTTP_DOWNLOAD_IN_PROGRESS;
        }

        case STATE_COMPLETE:
            if (dlState.isFirmware) {
                if (Update.end(true)) {
                    if (debug) Serial.println("✅ Firmware update successful! Restarting...");
                    delay(2000);
                    ESP.restart();
                    return HTTP_UPDATE_OK;
                } else {
                    if (debug) Serial.println("❌ OTA finalization failed");
                    dlState.state = STATE_ERROR;
                    return HTTP_DOWNLOAD_IN_PROGRESS;
                }
            } else {
                if (debug) Serial.println("✅ File download completed successfully");
                return HTTP_FILE_END;
            }
            dlState.cleanup();
            checkMemory();

            break;

        case STATE_ERROR:

    if (debug) {
        Serial.printf("💥 ERROR State: isFirmware=%s, attempt=%d/%d, otaStarted=%s\n",
            dlState.isFirmware ? "true" : "false",
            dlState.attempt, MAX_ATTEMPTS,
            dlState.otaStarted ? "true" : "false");
    }

    if (dlState.isFirmware) {
        if (debug) Serial.println("❌ Firmware update failed. Restarting...");
      #ifdef ESP32

        Update.abort();
      #elif defined(ESP8266)
        Serial.println("Перезагрузка ESP8266 для отката прошивки...");
        delay(2000);
        ESP.restart();
      #endif
        dlState.cleanup();
        return HTTP_UPDATE_FAILED;
    }

    if (dlState.attempt < MAX_ATTEMPTS) {
        if (debug) Serial.printf("⚠️  Retrying... Attempt %d/%d\n", dlState.attempt, MAX_ATTEMPTS);
        delay(2000);
        dlState.state = STATE_IDLE;
        return HTTP_DOWNLOAD_IN_PROGRESS;
    } else {
        if (debug) Serial.println("❌ File download failed after max attempts");
         dlState.cleanup();
        return HTTP_FILE_FAILED;
    }

    checkMemory();

    break;
    }

    return HTTP_DOWNLOAD_IN_PROGRESS;
}
