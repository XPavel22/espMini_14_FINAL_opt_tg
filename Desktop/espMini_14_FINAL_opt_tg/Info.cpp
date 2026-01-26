#include "Info.h"

namespace {
    size_t appendToBuffer(char* buffer, size_t bufferSize, size_t offset, const char* format, ...) {
        if (bufferSize <= offset) return offset;

        va_list args;
        va_start(args, format);
        int written = vsnprintf(buffer + offset, bufferSize - offset, format, args);
        va_end(args);

        if (written > 0) {
            return offset + written;
        }
        return offset;
    }
}

Info::Info()
{

}

void Info::getChipModel(char* buffer, size_t bufferSize) {
#ifdef ESP32
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    switch (chip_info.model) {
        case CHIP_ESP32:    snprintf(buffer, bufferSize, "ESP32"); break;
        case CHIP_ESP32S2:  snprintf(buffer, bufferSize, "ESP32-S2"); break;
        case CHIP_ESP32S3:  snprintf(buffer, bufferSize, "ESP32-S3"); break;
        case CHIP_ESP32C3:  snprintf(buffer, bufferSize, "ESP32-C3"); break;
        case CHIP_ESP32C6:  snprintf(buffer, bufferSize, "ESP32-C6"); break;
        default:            snprintf(buffer, bufferSize, "Unknown (0x%X)", chip_info.model); break;
    }
#elif defined(ESP8266)
    snprintf(buffer, bufferSize, "ESP8266");
#endif
}

size_t Info::getSystemStatus(char* buffer, size_t bufferSize) {
    size_t offset = 0;

    char chipModel[20];
    getChipModel(chipModel, sizeof(chipModel));
    offset = appendToBuffer(buffer, bufferSize, offset, "Модель процессора: %s\n", chipModel);

#ifdef ESP32
    uint32_t cpuFreq = getCpuFrequencyMhz();
#elif defined(ESP8266)
    uint32_t cpuFreq = ESP.getCpuFreqMHz();
#endif
    offset = appendToBuffer(buffer, bufferSize, offset, "Частота процессора: %u MHz\n", cpuFreq);

#ifdef ESP32
    offset = appendToBuffer(buffer, bufferSize, offset, "Объем ОЗУ: %u KB\n", ESP.getHeapSize() / 1024);
    offset = appendToBuffer(buffer, bufferSize, offset, "Свободная память ОЗУ: %u KB\n", ESP.getFreeHeap() / 1024);
#elif defined(ESP8266)
    uint32_t freeHeap = ESP.getFreeHeap();
    offset = appendToBuffer(buffer, bufferSize, offset, "Общий объем ОЗУ: ~80 KB\n");
    offset = appendToBuffer(buffer, bufferSize, offset, "Занято: %u KB\n", (80 - freeHeap/1024));
    offset = appendToBuffer(buffer, bufferSize, offset, "Свободно: %u KB\n", freeHeap/1024);
#endif

#if defined(ESP8266)
    FSInfo fs_info;
    if(SPIFFS.info(fs_info)) {
        offset = appendToBuffer(buffer, bufferSize, offset, "Общий объем SPIFFS: %u KB\n", fs_info.totalBytes / 1024);
        offset = appendToBuffer(buffer, bufferSize, offset, "Используемый объем SPIFFS: %u KB\n", fs_info.usedBytes / 1024);
        offset = appendToBuffer(buffer, bufferSize, offset, "Свободный объем SPIFFS: %u KB\n", (fs_info.totalBytes - fs_info.usedBytes) / 1024);
    } else {
        offset = appendToBuffer(buffer, bufferSize, offset, "Ошибка получения информации SPIFFS\n");
    }
#elif defined(ESP32)
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    offset = appendToBuffer(buffer, bufferSize, offset, "Общий объем SPIFFS: %u KB\n", totalBytes / 1024);
    offset = appendToBuffer(buffer, bufferSize, offset, "Используемый объем SPIFFS: %u KB\n", usedBytes / 1024);
    offset = appendToBuffer(buffer, bufferSize, offset, "Свободный объем SPIFFS: %u KB\n", (totalBytes - usedBytes) / 1024);
#endif

    WiFiMode_t mode = WiFi.getMode();

    if (mode == WIFI_AP) {
        offset = appendToBuffer(buffer, bufferSize, offset, "Режим: Точка доступа (AP)\n");
        offset = appendToBuffer(buffer, bufferSize, offset, "SSID точки доступа: %s\n", WiFi.softAPSSID().c_str());

        IPAddress apIP = WiFi.softAPIP();
        offset = appendToBuffer(buffer, bufferSize, offset, "IP адрес точки доступа: %u.%u.%u.%u\n", apIP[0], apIP[1], apIP[2], apIP[3]);

        uint8_t mac[6];
        WiFi.softAPmacAddress(mac);
        offset = appendToBuffer(buffer, bufferSize, offset, "MAC адрес AP: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        offset = appendToBuffer(buffer, bufferSize, offset, "Число подключенных клиентов: %u\n", WiFi.softAPgetStationNum());

    } else if (mode == WIFI_STA) {
        offset = appendToBuffer(buffer, bufferSize, offset, "Режим: Клиент (STA)\n");
        if (WiFi.status() == WL_CONNECTED) {
            offset = appendToBuffer(buffer, bufferSize, offset, "SSID сети: %s\n", WiFi.SSID().c_str());

            IPAddress localIP = WiFi.localIP();
            offset = appendToBuffer(buffer, bufferSize, offset, "IP адрес: %u.%u.%u.%u\n", localIP[0], localIP[1], localIP[2], localIP[3]);

            uint8_t mac[6];
            WiFi.macAddress(mac);
            offset = appendToBuffer(buffer, bufferSize, offset, "MAC адрес STA: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

            IPAddress gw = WiFi.gatewayIP();
            offset = appendToBuffer(buffer, bufferSize, offset, "Шлюз: %u.%u.%u.%u\n", gw[0], gw[1], gw[2], gw[3]);

            IPAddress sn = WiFi.subnetMask();
            offset = appendToBuffer(buffer, bufferSize, offset, "Маска подсети: %u.%u.%u.%u\n", sn[0], sn[1], sn[2], sn[3]);

            IPAddress dns = WiFi.dnsIP(0);
            offset = appendToBuffer(buffer, bufferSize, offset, "DNS1: %u.%u.%u.%u\n", dns[0], dns[1], dns[2], dns[3]);

            offset = appendToBuffer(buffer, bufferSize, offset, "RSSI: %d dBm\n", WiFi.RSSI());
            offset = appendToBuffer(buffer, bufferSize, offset, "Доступ: http://%u.%u.%u.%u\n", localIP[0], localIP[1], localIP[2], localIP[3]);
        } else {
            offset = appendToBuffer(buffer, bufferSize, offset, "Нет активного подключения\n");
        }
    } else if (mode == WIFI_AP_STA) {
        offset = appendToBuffer(buffer, bufferSize, offset, "Режим: Точка доступа + Клиент\n");

        offset = appendToBuffer(buffer, bufferSize, offset, "AP SSID: %s\n", WiFi.softAPSSID().c_str());
        IPAddress apIP = WiFi.softAPIP();
        offset = appendToBuffer(buffer, bufferSize, offset, "AP IP: %u.%u.%u.%u\n", apIP[0], apIP[1], apIP[2], apIP[3]);
        offset = appendToBuffer(buffer, bufferSize, offset, "AP клиентов: %u\n", WiFi.softAPgetStationNum());

        if (WiFi.status() == WL_CONNECTED) {
            offset = appendToBuffer(buffer, bufferSize, offset, "STA SSID: %s\n", WiFi.SSID().c_str());
            IPAddress localIP = WiFi.localIP();
            offset = appendToBuffer(buffer, bufferSize, offset, "STA IP: %u.%u.%u.%u\n", localIP[0], localIP[1], localIP[2], localIP[3]);
        } else {
            offset = appendToBuffer(buffer, bufferSize, offset, "STA: не подключен\n");
        }
    } else {
        offset = appendToBuffer(buffer, bufferSize, offset, "Режим WiFi: Выключен\n");
    }

    return offset;
}
