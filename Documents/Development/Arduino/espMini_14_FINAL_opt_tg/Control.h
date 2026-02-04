#ifndef CONTROL_H
#define CONTROL_H

#include "CommonTypes.h"

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <PID_v1.h>
#include <DHT.h>
#include <time.h>
#include "DeviceManager.h"
#include "Logger.h"
#include "AppState.h"

class Control {
private:
 Logger& logger;
 DeviceManager& deviceManager;
 AppState& appState;

    std::vector<Device>& myDevices;
    uint8_t& currentDeviceIndex;

    std::unordered_map<uint8_t, TouchSensorState> touchStates;
    std::unique_ptr<PID> myPID;

    unsigned long lastPidTime;
    double outputPid;
    double inputPid;
    double setpointPid;

    const unsigned long pidWindowSize = 5000;
    unsigned long pidWindowStartTime;

    unsigned long lastUpdate = 0;

     double scalePidCoefficient(double userCoefficient);

    bool isNumeric(const String& str);
    bool isValidDateTime(const String& dateTime);
    int shiftWeekDay(int currentDay);
    time_t convertToTimeT(const String& date);
    time_t getCurrentTime();
    String formatDateTime(time_t rawTime);
    time_t convertOnlyDate(time_t rawTime);
    uint32_t timeStringToSeconds(const String& timeStr);
    String secondsToTimeString(uint32_t totalSeconds);

    void controlOutputs(OutPower& outPower);

    void setFlagsSettingsTimers(uint8_t selectedIndex, Timer& currentTimer);
    void collectionSettingsTimer(uint8_t currentTimerIndex);
    void executeTimers(bool state);

    void saveRelayStates(uint8_t relayIndex);
    void restoreRelayStates(uint8_t relayIndex);

    void collectionSettingsSchedule(bool start, ScheduleScenario& scenario);

    float readNTCTemperature(const Sensor& sensor);
    int readAnalog(const Sensor& sensor);

 struct {
    bool isActive = false;
    bool pidInitialized = false;
  } tempControlState;

    Relay* findRelayById(Device& device, int id);
    Sensor* findSensorById(Device& device, int id);

public:

     Control(DeviceManager& dm, Logger& lg, AppState& appState);

    void setup();
    void update();
    void loop();

    void setupControl(bool onlyDHT = false);
    void readSensors();
    void setTemperature();
    void setTimersExecute();
    void updatePins();
    void setSchedules();
    void setSensorActions();

    void processCommand(const String& command);

    bool checkTouchSensor(uint8_t pin);

    void reInitDhtSensors();

};

#endif
