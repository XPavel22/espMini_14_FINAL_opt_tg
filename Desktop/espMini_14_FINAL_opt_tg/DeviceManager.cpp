#include "DeviceManager.h"
#include <cstring>

DeviceManager::DeviceManager(AppState& appState)
    : appState(appState)
{

}

void DeviceManager::initializeDevice(const char* name, bool activ, bool isNewDevice) {if (!isNewDevice) {
    if (!myDevices.empty()) {
      myDevices.clear();
      myDevices.shrink_to_fit();
    }
  }

  myDevices.emplace_back();
  Device& newDevice = myDevices.back();

  strncpy_safe(newDevice.nameDevice, name, MAX_DESCRIPTION_LENGTH);
  newDevice.isSelected = activ;
  newDevice.isTimersEnabled = false;
  newDevice.isEncyclateTimers = true;
  newDevice.isScheduleEnabled = false;
  newDevice.isActionEnabled = false;

#ifdef ESP32
#ifdef CONFIG_IDF_TARGET_ESP32S2
  newDevice.pins = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 18, 21, 33};
#elif defined(CONFIG_IDF_TARGET_ESP32)

  newDevice.pins = { 0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33 };

#else
  newDevice.pins = {1, 2, 3, 4, 5, 6, 7, 8, 9};
#endif
#elif defined(ESP8266)
  newDevice.pins = {1, 2, 3, 4, 5, 12, 13, 14, 17};
#else
  newDevice.pins = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};
#endif

  int nextId = 0;

  const int output_pins[] = {3, 18, 19, 21};
  const bool output_manual_mode[] = {true, false, true, false};
  const bool output_state_pin[] = {true, false, true, false};

  for (int i = 0; i < 4; i++) {
    Relay relay;
    relay.id = nextId++;
    relay.pin = output_pins[i];
    relay.manualMode = output_manual_mode[i];
    relay.statePin = output_state_pin[i];
    relay.isOutput = true;
    relay.isDigital = true;
    relay.lastState = false;
    relay.isPwm = false;
    relay.pwm = 0;
    snprintf(relay.description, MAX_DESCRIPTION_LENGTH, "Выход_%d", i + 1);
    newDevice.relays.push_back(relay);
  }

  Relay dhtInput;
  dhtInput.id = nextId++;
  dhtInput.pin = 23;
  dhtInput.manualMode = false;
  dhtInput.isOutput = false;
  dhtInput.isDigital = true;
  dhtInput.statePin = false;
  dhtInput.lastState = false;
  strncpy_safe(dhtInput.description, "DHT-11 Датчик", MAX_DESCRIPTION_LENGTH);
  newDevice.relays.push_back(dhtInput);

  Relay currentInput;
  currentInput.id = nextId++;
  currentInput.pin = 33;
  currentInput.manualMode = false;
  currentInput.isOutput = false;
  currentInput.isDigital = false;
  currentInput.statePin = false;
  currentInput.lastState = false;
  strncpy_safe(currentInput.description, "Вход датчик тока", MAX_DESCRIPTION_LENGTH);
  newDevice.relays.push_back(currentInput);

  Sensor dhtSensor;
  strncpy_safe(dhtSensor.description, "Сенсор DHT11", MAX_DESCRIPTION_LENGTH);
  dhtSensor.isUseSetting = true;
  dhtSensor.sensorId = nextId++;
  dhtSensor.relayId = 4;
  dhtSensor.typeSensor.clear();
  dhtSensor.typeSensor.set(0, true);
  dhtSensor.serial_r = 20000;
  dhtSensor.thermistor_r = 10000;
  dhtSensor.currentValue = 0.0;
  dhtSensor.humidityValue = 0.0;
  dhtSensor.dht = nullptr;
  newDevice.sensors.push_back(dhtSensor);

  Sensor currentSensor;
  strncpy_safe(currentSensor.description, "Датчик тока", MAX_DESCRIPTION_LENGTH);
  currentSensor.isUseSetting = true;
  currentSensor.sensorId = nextId++;
  currentSensor.relayId = 5;
  currentSensor.typeSensor.clear();
  currentSensor.typeSensor.set(4, true);
  currentSensor.serial_r = 20000;
  currentSensor.thermistor_r = 10000;
  currentSensor.currentValue = 0.0;
  currentSensor.humidityValue = 0.0;
  newDevice.sensors.push_back(currentSensor);

  Action touchAction;
  strncpy_safe(touchAction.description, "Действие - превышение тока", MAX_DESCRIPTION_LENGTH);
  touchAction.isUseSetting = true;
  touchAction.targetRelayId = -1;
  touchAction.relayMustBeOn = true;
  touchAction.targetSensorId = currentSensor.sensorId;
  touchAction.triggerValueMax = 1;
  touchAction.triggerValueMin = 0.5;
  touchAction.isHumidity = false;
  touchAction.actionMoreOrEqual = true;
  touchAction.isReturnSetting = true;
  touchAction.wasTriggered = false;
  touchAction.collectionSettings.clear();
  touchAction.collectionSettings.set(1, true);
  touchAction.sendMsg = "";

  OutPower defaultTouchOutput;
  defaultTouchOutput.isUseSetting = true;
  defaultTouchOutput.relayId = newDevice.relays[0].id;
  defaultTouchOutput.statePin = false;
  defaultTouchOutput.lastState = false;
  defaultTouchOutput.isPwm = false;
  defaultTouchOutput.pwm = 0;
  defaultTouchOutput.isReturn = true;

  touchAction.outputs.push_back(defaultTouchOutput);
  newDevice.actions.push_back(touchAction);

  ScheduleScenario scenario;
  strncpy_safe(scenario.description, "Мой первый сценарий 1", MAX_DESCRIPTION_LENGTH);
  scenario.isUseSetting = false;
  scenario.isActive = false;
  scenario.collectionSettings.clear();
  scenario.collectionSettings.set(3, true);
  strncpy_safe(scenario.startDate, "2012-12-12", MAX_DATE_LENGTH);
  strncpy_safe(scenario.endDate, "2222-12-12", MAX_DATE_LENGTH);

  startEndTime timeInterval;
  strncpy_safe(timeInterval.startTime, "08:00", MAX_TIME_LENGTH);
  strncpy_safe(timeInterval.endTime, "18:00", MAX_TIME_LENGTH);
  scenario.startEndTimes.push_back(timeInterval);
  scenario.week.bits = 0x7F;
  scenario.months.bits = 0xFFF;

  scenario.initialStateRelay.isUseSetting = true;
  scenario.initialStateRelay.relayId = newDevice.relays[0].id;
  scenario.initialStateRelay.statePin = true;
  scenario.initialStateRelay.lastState = false;
  scenario.initialStateRelay.isPwm = false;
  scenario.initialStateRelay.pwm = 0;
  scenario.initialStateRelay.isReturn = false;

  scenario.endStateRelay.isUseSetting = false;
  scenario.endStateRelay.relayId = newDevice.relays[0].id;
  scenario.endStateRelay.statePin = false;
  scenario.endStateRelay.lastState = false;
  scenario.endStateRelay.isPwm = false;
  scenario.endStateRelay.pwm = 0;
  scenario.endStateRelay.isReturn = false;

  scenario.temperatureUpdated = false;
  scenario.timersExecuted = false;
  scenario.initialStateApplied = false;
  scenario.endStateApplied = false;
  scenario.scenarioProcessed = false;
  newDevice.scheduleScenarios.push_back(scenario);

  newDevice.temperature.isUseSetting = false;
  newDevice.temperature.relayId = newDevice.relays[0].id;
  newDevice.temperature.lastState = false;
  newDevice.temperature.sensorId = dhtSensor.sensorId;
  newDevice.temperature.setTemperature = 22;
  newDevice.temperature.currentTemp = 0.0;
  newDevice.temperature.isSmoothly = false;
  newDevice.temperature.isIncrease = true;
  newDevice.temperature.collectionSettings.clear();
  newDevice.temperature.collectionSettings.set(0, true);
  newDevice.temperature.selectedPidIndex = 0;
  newDevice.temperature.pidOutputMs = 0;
  newDevice.temperature.sensorPtr = nullptr;
  newDevice.temperature.relayPtr = nullptr;

  Pid pid1, pid2, pid3;

  strncpy_safe(pid1.description, "Стандартный", MAX_DESCRIPTION_LENGTH);
  pid1.Kp = 2.0; pid1.Ki = 0.5; pid1.Kd = 1.0;

  strncpy_safe(pid2.description, "Быстрый", MAX_DESCRIPTION_LENGTH);
  pid2.Kp = 1.5; pid2.Ki = 0.4; pid2.Kd = 0.9;

  strncpy_safe(pid3.description, "Плавный", MAX_DESCRIPTION_LENGTH);
  pid3.Kp = 1.0; pid3.Ki = 0.3; pid3.Kd = 0.8;

  newDevice.pids.push_back(pid1);
  newDevice.pids.push_back(pid2);
  newDevice.pids.push_back(pid3);

  Timer timer;
  timer.isUseSetting = true;
  strncpy_safe(timer.time, "00:00:05", MAX_TIME_LENGTH);
  timer.collectionSettings.clear();
  timer.collectionSettings.set(1, true);

  timer.initialStateRelay.isUseSetting = true;
  timer.initialStateRelay.relayId = newDevice.relays[0].id;
  timer.initialStateRelay.statePin = true;
  timer.initialStateRelay.lastState = false;
  timer.initialStateRelay.isPwm = false;
  timer.initialStateRelay.pwm = 0;
  timer.initialStateRelay.isReturn = false;

  timer.endStateRelay.isUseSetting = true;
  timer.endStateRelay.relayId = newDevice.relays[0].id;
  timer.endStateRelay.statePin = false;
  timer.endStateRelay.lastState = false;
  timer.endStateRelay.isPwm = false;
  timer.endStateRelay.pwm = 0;
  timer.endStateRelay.isReturn = false;

  newDevice.timers.push_back(timer);}

String DeviceManager::serializeDevice(const Device& device, const char* fileName, AsyncWebServerRequest *request) {

  appState.isProcessWorkingJson = true;

  DynamicJsonDocument doc(SIZE_JSON_S);

    if (strlen(device.nameDevice) > 0) {
        doc["nmd"] = device.nameDevice;
    } else {
        doc["nmd"] = "";
    }
    doc["isl"] = device.isSelected;

     JsonArray pins = doc.createNestedArray("pins");
  for (const auto& pin : device.pins) {
    pins.add(pin);
  }

    JsonArray rel = doc.createNestedArray("rel");
    for (const auto& relay : device.relays) {
        JsonObject relayObj = rel.createNestedObject();
        relayObj["id"] = relay.id;
        relayObj["pin"] = relay.pin;
        relayObj["man"] = relay.manualMode;
        relayObj["stp"] = relay.statePin;
        relayObj["out"] = relay.isOutput;
        relayObj["dig"] = relay.isDigital;
        relayObj["lst"] = relay.lastState;
        relayObj["dsc"] = relay.description;
    }

    JsonArray pinL = doc.createNestedArray("pinL");
    for (const auto& pin : device.pins) {
        pinL.add(pin);
    }

    JsonArray sen = doc.createNestedArray("sen");
    for (const auto& sensor : device.sensors) {
        JsonObject sensorObj = sen.createNestedObject();

        sensorObj["dsc"] =  sensor.description;
        sensorObj["use"] = sensor.isUseSetting;
        sensorObj["sid"] = sensor.sensorId;
        sensorObj["rid"] = sensor.relayId;

        JsonArray typ = sensorObj.createNestedArray("typ");
        for (int i = 0; i < 7; i++) {
            typ.add(sensor.typeSensor.get(i));
        }

        sensorObj["ser"] = sensor.serial_r;
        sensorObj["thm"] = sensor.thermistor_r;
    }

    JsonArray act = doc.createNestedArray("act");
    for (const auto& action : device.actions) {
        JsonObject actionObj = act.createNestedObject();

        actionObj["dsc"] = action.description;
        actionObj["use"] = action.isUseSetting;
        actionObj["trd"] = action.targetRelayId;
        actionObj["rmb"] = action.relayMustBeOn;
        actionObj["tsd"] = action.targetSensorId;
        actionObj["tvm"] = action.triggerValueMax;
        actionObj["tvi"] = action.triggerValueMin;
        actionObj["hum"] = action.isHumidity;
        actionObj["ame"] = action.actionMoreOrEqual;
        actionObj["irs"] = action.isReturnSetting;
        actionObj["wtr"] = action.wasTriggered;
        actionObj["msg"] = action.sendMsg;

        JsonArray cls = actionObj.createNestedArray("cls");
        for (int i = 0; i < 4; i++) {
            cls.add(action.collectionSettings.get(i));
        }

        JsonArray outL = actionObj.createNestedArray("outL");
        for (const auto& output : action.outputs) {
            JsonObject outputObj = outL.createNestedObject();
            outputObj["use"] = output.isUseSetting;
            outputObj["rid"] = output.relayId;
            outputObj["stp"] = output.statePin;
            outputObj["lst"] = output.lastState;
            outputObj["rtn"] = output.isReturn;

        }
    }

    JsonArray sch = doc.createNestedArray("sch");
    for (const auto& scenario : device.scheduleScenarios) {
        JsonObject scenarioObj = sch.createNestedObject();
        scenarioObj["use"] = scenario.isUseSetting;
        scenarioObj["dsc"] = scenario.description;
        scenarioObj["iac"] = scenario.isActive;

        JsonArray cls = scenarioObj.createNestedArray("cls");
        for (int i = 0; i < 4; i++) {
            cls.add(scenario.collectionSettings.get(i));
        }

        scenarioObj["sdt"] = scenario.startDate;
        scenarioObj["edt"] = scenario.endDate;

        JsonArray set = scenarioObj.createNestedArray("set");
        for (const auto& timeInterval : scenario.startEndTimes) {
            JsonObject intervalObj = set.createNestedObject();
            intervalObj["stm"] = timeInterval.startTime;
            intervalObj["etm"] = timeInterval.endTime;
        }

        JsonArray wek = scenarioObj.createNestedArray("wek");
        for (int i = 0; i < 7; i++) {
            wek.add(scenario.week.get(i));
        }

        JsonArray mon = scenarioObj.createNestedArray("mon");
        for (int i = 0; i < 12; i++) {
            mon.add(scenario.months.get(i));
        }

        JsonObject isr = scenarioObj.createNestedObject("isr");
        isr["use"] = scenario.initialStateRelay.isUseSetting;
        isr["rid"] = scenario.initialStateRelay.relayId;
        isr["stp"] = scenario.initialStateRelay.statePin;
        isr["lst"] = scenario.initialStateRelay.lastState;

        JsonObject esr = scenarioObj.createNestedObject("esr");
        esr["use"] = scenario.endStateRelay.isUseSetting;
        esr["rid"] = scenario.endStateRelay.relayId;
        esr["stp"] = scenario.endStateRelay.statePin;
        esr["lst"] = scenario.endStateRelay.lastState;

    }

    JsonObject tmp = doc.createNestedObject("tmp");
    tmp["use"] = device.temperature.isUseSetting;
    tmp["rid"] = device.temperature.relayId;
    tmp["lst"] = device.temperature.lastState;
    tmp["sid"] = device.temperature.sensorId;
    tmp["stT"] = device.temperature.setTemperature;
    tmp["ctp"] = device.temperature.currentTemp;
    tmp["smt"] = device.temperature.isSmoothly;
    tmp["inc"] = device.temperature.isIncrease;

    JsonArray tempCls = tmp.createNestedArray("cls");
    for (int i = 0; i < 4; i++) {
        tempCls.add(device.temperature.collectionSettings.get(i));
    }

    tmp["spi"] = device.temperature.selectedPidIndex;

    JsonArray pid = doc.createNestedArray("pid");
    for (const auto& pid_item : device.pids) {
        JsonObject pidObject = pid.createNestedObject();
        pidObject["dsc"] = pid_item.description;
        pidObject["Kp"] = pid_item.Kp;
        pidObject["Ki"] = pid_item.Ki;
        pidObject["Kd"] = pid_item.Kd;
    }

    JsonArray tmr = doc.createNestedArray("tmr");
    for (const auto& timer : device.timers) {
        JsonObject timerObj = tmr.createNestedObject();
        timerObj["use"] = timer.isUseSetting;
        timerObj["tim"] = timer.time;

        JsonArray cls = timerObj.createNestedArray("cls");
        for (int i = 0; i < 4; i++) {
            cls.add(timer.collectionSettings.get(i));
        }

        JsonObject isr = timerObj.createNestedObject("isr");
        isr["use"] = timer.initialStateRelay.isUseSetting;
        isr["rid"] = timer.initialStateRelay.relayId;
        isr["stp"] = timer.initialStateRelay.statePin;
        isr["lst"] = timer.initialStateRelay.lastState;

        JsonObject esr = timerObj.createNestedObject("esr");
        esr["use"] = timer.endStateRelay.isUseSetting;
        esr["rid"] = timer.endStateRelay.relayId;
        esr["stp"] = timer.endStateRelay.statePin;
        esr["lst"] = timer.endStateRelay.lastState;

    }

    doc["ite"] = device.isTimersEnabled;
    doc["iet"] = device.isEncyclateTimers;
    doc["ise"] = device.isScheduleEnabled;
    doc["iae"] = device.isActionEnabled;

    convertBooleansToIntegers(doc);

    if (request != nullptr) {

      AsyncResponseStream *response = request->beginResponseStream("application/json");
        serializeJson(doc, *response);
        request->send(response);
        appState.isProcessWorkingJson = false;
        return "sending";
    }

    if (fileName != nullptr) {

    String jsonString;
    serializeJson(doc, jsonString);

    File file = SPIFFS.open(fileName, "w");
    if (!file) {
      appState.isProcessWorkingJson = false;
      return "error";
    }
    if (file.print(jsonString) == 0) {
      file.close();

      appState.isProcessWorkingJson = false;
      return "error";
    }
    file.println();
    file.close();

    appState.isProcessWorkingJson = false;
    return "success";
  }

  String jsonString;
  serializeJson(doc, jsonString);

  appState.isProcessWorkingJson = false;
  return jsonString;
}

void DeviceManager::convertBooleansToIntegers(JsonVariant variant) {
  if (variant.is<JsonObject>()) {
    JsonObject obj = variant.as<JsonObject>();
    for (JsonPair kv : obj) {
      convertBooleansToIntegers(kv.value());
    }
  } else if (variant.is<JsonArray>()) {
    JsonArray arr = variant.as<JsonArray>();
    for (JsonVariant elem : arr) {
      convertBooleansToIntegers(elem);
    }
  } else if (variant.is<bool>()) {
    variant.set(variant.as<bool>() ? 1 : 0);
  }
}

bool DeviceManager::deserializeDevice(JsonObject doc, Device& device) {

 appState.isProcessWorkingJson = true;if (doc.containsKey("nmd")) {
        strncpy_safe(device.nameDevice, doc["nmd"], MAX_DESCRIPTION_LENGTH);
      }

  if (doc.containsKey("isl")) {
    device.isSelected = doc["isl"].as<bool>();
  }

  if (doc.containsKey("rel")) {
    device.relays.clear();
    JsonArray relays = doc["rel"];
    device.relays.reserve(relays.size());
    for (JsonObject relayObj : relays) {
      Relay relay;
      if (relayObj.containsKey("id")) relay.id = relayObj["id"].as<int>();
      if (relayObj.containsKey("pin")) relay.pin = relayObj["pin"].as<uint8_t>();
      if (relayObj.containsKey("man")) relay.manualMode = relayObj["man"].as<bool>();
      if (relayObj.containsKey("stp")) relay.statePin = relayObj["stp"].as<bool>();
      if (relayObj.containsKey("lst")) relay.lastState = relayObj["lst"].as<bool>();
      if (relayObj.containsKey("out")) relay.isOutput = relayObj["out"].as<bool>();
      if (relayObj.containsKey("isPwm")) relay.isPwm = relayObj["isPwm"].as<bool>();
      if (relayObj.containsKey("pwm")) relay.pwm = relayObj["pwm"].as<uint8_t>();
      if (relayObj.containsKey("dig")) relay.isDigital = relayObj["dig"].as<bool>();
      if (relayObj.containsKey("dsc")) {
        strncpy_safe(relay.description, relayObj["dsc"], MAX_DESCRIPTION_LENGTH);
      }
      device.relays.push_back(relay);
    }
  }

  if (doc.containsKey("pinL")) {
    device.pins.clear();
    JsonArray pins = doc["pinL"];
    device.pins.reserve(pins.size());
    for (JsonVariant pin : pins) {
      device.pins.push_back(pin.as<uint8_t>());
    }
  }

  if (doc.containsKey("sen")) {
    JsonArray sensorsJson = doc["sen"];
    std::vector<Sensor> newSensors;
    newSensors.reserve(sensorsJson.size());

    for (JsonObject sensorObj : sensorsJson) {
      Sensor sensor;

      if (sensorObj.containsKey("dsc")) {
        strncpy_safe(sensor.description, sensorObj["dsc"], MAX_DESCRIPTION_LENGTH);
      }
      if (sensorObj.containsKey("use")) sensor.isUseSetting = sensorObj["use"].as<bool>();
      if (sensorObj.containsKey("sid")) sensor.sensorId = sensorObj["sid"];
      if (sensorObj.containsKey("rid")) sensor.relayId = sensorObj["rid"];

      if (sensorObj.containsKey("typ")) {
        JsonArray typeSensor = sensorObj["typ"];
        for (int i = 0; i < 7 && i < typeSensor.size(); i++) {
          sensor.typeSensor.set(i, typeSensor[i].as<bool>());
        }
      }

      if (sensorObj.containsKey("ser")) sensor.serial_r = sensorObj["ser"];
      if (sensorObj.containsKey("thm")) sensor.thermistor_r = sensorObj["thm"];

      newSensors.push_back(sensor);
    }

    noInterrupts();
    device.sensors = std::move(newSensors);
    interrupts();
  }

  if (doc.containsKey("act")) {
    JsonArray actionsJson = doc["act"];
    std::vector<Action> newActions;
    newActions.reserve(actionsJson.size());

    for (JsonObject actionObj : actionsJson) {
      Action action;

      if (actionObj.containsKey("dsc")) {
        strncpy_safe(action.description, actionObj["dsc"], MAX_DESCRIPTION_LENGTH);
      }
      if (actionObj.containsKey("use")) action.isUseSetting = actionObj["use"].as<bool>();
      if (actionObj.containsKey("tsd")) action.targetSensorId = actionObj["tsd"].as<int>();;
      if (actionObj.containsKey("tvm")) action.triggerValueMax = actionObj["tvm"].as<float>();
      if (actionObj.containsKey("tvi")) action.triggerValueMin = actionObj["tvi"].as<float>();
      if (actionObj.containsKey("hum")) action.isHumidity = actionObj["hum"].as<bool>();
      if (actionObj.containsKey("ame")) action.actionMoreOrEqual = actionObj["ame"].as<bool>();
      if (actionObj.containsKey("irs")) action.isReturnSetting = actionObj["irs"].as<bool>();
      if (actionObj.containsKey("wtr")) action.wasTriggered = actionObj["wtr"].as<bool>();
      if (actionObj.containsKey("msg")) action.sendMsg = actionObj["msg"].as<String>();

      if (actionObj.containsKey("trd"))
        action.targetRelayId = actionObj["trd"];
      else
        action.targetRelayId = -1;

      if (actionObj.containsKey("rmb"))
        action.relayMustBeOn = actionObj["rmb"].as<bool>();
      else
        action.relayMustBeOn = true;

      if (actionObj.containsKey("cls")) {
        JsonArray collectionSettings = actionObj["cls"];
        for (int i = 0; i < 4 && i < collectionSettings.size(); i++) {
          action.collectionSettings.set(i, collectionSettings[i].as<bool>());
        }
      }

      if (actionObj.containsKey("outL")) {
        JsonArray outputsJson = actionObj["outL"];
        action.outputs.clear();
        action.outputs.reserve(outputsJson.size());
        for (JsonObject outputObj : outputsJson) {
          OutPower output;
          if (outputObj.containsKey("use")) output.isUseSetting = outputObj["use"].as<bool>();
          if (outputObj.containsKey("rid")) output.relayId = outputObj["rid"];
          if (outputObj.containsKey("stp")) output.statePin = outputObj["stp"].as<bool>();
          if (outputObj.containsKey("lst")) output.lastState = outputObj["lst"].as<bool>();
          if (outputObj.containsKey("rtn")) output.isReturn = outputObj["rtn"].as<bool>();

          action.outputs.push_back(output);
        }
      }

      newActions.push_back(action);
    }

    noInterrupts();
    device.actions = std::move(newActions);
    interrupts();
  }

  if (doc.containsKey("sch")) {
    device.scheduleScenarios.clear();
    JsonArray scenarios = doc["sch"];
    device.scheduleScenarios.reserve(scenarios.size());
    for (JsonObject scenarioObj : scenarios) {
      ScheduleScenario scenario;
      scenario.temperatureUpdated = false;
      scenario.timersExecuted = false;
      scenario.initialStateApplied = false;
      scenario.endStateApplied = false;
      scenario.scenarioProcessed = false;

      if (scenarioObj.containsKey("use")) scenario.isUseSetting = scenarioObj["use"].as<bool>();
      if (scenarioObj.containsKey("dsc")) {
        strncpy_safe(scenario.description, scenarioObj["dsc"], MAX_DESCRIPTION_LENGTH);
      }
      if (scenarioObj.containsKey("iac")) scenario.isActive = scenarioObj["iac"].as<bool>();

      if (scenarioObj.containsKey("cls")) {
        JsonArray collectionSettings = scenarioObj["cls"];
        for (int i = 0; i < 4 && i < collectionSettings.size(); i++) {
          scenario.collectionSettings.set(i, collectionSettings[i].as<bool>());
        }
      }

      if (scenarioObj.containsKey("sdt")) {
        strncpy_safe(scenario.startDate, scenarioObj["sdt"], MAX_DATE_LENGTH);
      }
      if (scenarioObj.containsKey("edt")) {
        strncpy_safe(scenario.endDate, scenarioObj["edt"], MAX_DATE_LENGTH);
      }

      if (scenarioObj.containsKey("set")) {
        JsonArray startEndTimes = scenarioObj["set"];
        for (JsonObject intervalObj : startEndTimes) {
          startEndTime timeInterval;
          if (intervalObj.containsKey("stm")) {
            strncpy_safe(timeInterval.startTime, intervalObj["stm"], MAX_TIME_LENGTH);
          }
          if (intervalObj.containsKey("etm")) {
            strncpy_safe(timeInterval.endTime, intervalObj["etm"], MAX_TIME_LENGTH);
          }
          scenario.startEndTimes.push_back(timeInterval);
        }
      }

      if (scenarioObj.containsKey("wek")) {
        JsonArray week = scenarioObj["wek"];
        for (int i = 0; i < 7 && i < week.size(); i++) {
          scenario.week.set(i, week[i].as<bool>());
        }
      }

      if (scenarioObj.containsKey("mon")) {
        JsonArray months = scenarioObj["mon"];
        for (int i = 0; i < 12 && i < months.size(); i++) {
          scenario.months.set(i, months[i].as<bool>());
        }
      }

      if (scenarioObj.containsKey("isr")) {
        JsonObject initialStateRelay = scenarioObj["isr"];
        if (initialStateRelay.containsKey("use")) scenario.initialStateRelay.isUseSetting = initialStateRelay["use"].as<bool>();
        if (initialStateRelay.containsKey("rid")) scenario.initialStateRelay.relayId = initialStateRelay["rid"];
        if (initialStateRelay.containsKey("stp")) scenario.initialStateRelay.statePin = initialStateRelay["stp"].as<bool>();
        if (initialStateRelay.containsKey("lst")) scenario.initialStateRelay.lastState = initialStateRelay["lst"].as<bool>();

      }

      if (scenarioObj.containsKey("esr")) {
        JsonObject endStateRelay = scenarioObj["esr"];
        if (endStateRelay.containsKey("use")) scenario.endStateRelay.isUseSetting = endStateRelay["use"].as<bool>();
        if (endStateRelay.containsKey("rid")) scenario.endStateRelay.relayId = endStateRelay["rid"];
        if (endStateRelay.containsKey("stp")) scenario.endStateRelay.statePin = endStateRelay["stp"].as<bool>();
        if (endStateRelay.containsKey("lst")) scenario.endStateRelay.lastState = endStateRelay["lst"].as<bool>();

      }

      device.scheduleScenarios.push_back(scenario);
    }
  }

  if (doc.containsKey("tmp")) {
    JsonObject temperature = doc["tmp"];
    if (temperature.containsKey("use")) device.temperature.isUseSetting = temperature["use"].as<bool>();
    if (temperature.containsKey("rid")) device.temperature.relayId = temperature["rid"];
    if (temperature.containsKey("lst")) device.temperature.lastState = temperature["lst"].as<bool>();
    if (temperature.containsKey("sid")) device.temperature.sensorId = temperature["sid"];
    if (temperature.containsKey("stT")) device.temperature.setTemperature = temperature["stT"];
    if (temperature.containsKey("ctp")) device.temperature.currentTemp = temperature["ctp"];
    if (temperature.containsKey("smt")) device.temperature.isSmoothly = temperature["smt"].as<bool>();
    if (temperature.containsKey("inc")) device.temperature.isIncrease = temperature["inc"].as<bool>();
    if (temperature.containsKey("cls")) {
      JsonArray collectionSettings = temperature["cls"];
      for (int i = 0; i < 4 && i < collectionSettings.size(); i++) {
        device.temperature.collectionSettings.set(i, collectionSettings[i].as<bool>());
      }
    }
    if (temperature.containsKey("spi")) device.temperature.selectedPidIndex = temperature["spi"];
  }

  if (doc.containsKey("pid")) {
    device.pids.clear();
    JsonArray pidsArray = doc["pid"].as<JsonArray>();
    device.pids.reserve(pidsArray.size());
    for (JsonObject pidObject : pidsArray) {
      Pid pid;
      if (pidObject.containsKey("dsc")) {
        strncpy_safe(pid.description, pidObject["dsc"], MAX_DESCRIPTION_LENGTH);
      }
      if (pidObject.containsKey("Kp")) pid.Kp = pidObject["Kp"];
      if (pidObject.containsKey("Ki")) pid.Ki = pidObject["Ki"];
      if (pidObject.containsKey("Kd")) pid.Kd = pidObject["Kd"];
      device.pids.push_back(pid);
    }
  }

  if (doc.containsKey("tmr")) {
    device.timers.clear();
    JsonArray timers = doc["tmr"];
    device.timers.reserve(timers.size());
    for (JsonObject timerObj : timers) {
      Timer timer;
      if (timerObj.containsKey("use")) timer.isUseSetting = timerObj["use"].as<bool>();
      if (timerObj.containsKey("tim")) {
        strncpy_safe(timer.time, timerObj["tim"], MAX_TIME_LENGTH);
      }
      if (timerObj.containsKey("cls")) {
        JsonArray collectionSettings = timerObj["cls"];
        for (int i = 0; i < 4 && i < collectionSettings.size(); i++) {
          timer.collectionSettings.set(i, collectionSettings[i].as<bool>());
        }
      }
      if (timerObj.containsKey("isr")) {
        JsonObject initialStateRelay = timerObj["isr"];
        if (initialStateRelay.containsKey("use")) timer.initialStateRelay.isUseSetting = initialStateRelay["use"].as<bool>();
        if (initialStateRelay.containsKey("rid")) timer.initialStateRelay.relayId = initialStateRelay["rid"];
        if (initialStateRelay.containsKey("stp")) timer.initialStateRelay.statePin = initialStateRelay["stp"].as<bool>();
        if (initialStateRelay.containsKey("lst")) timer.initialStateRelay.lastState = initialStateRelay["lst"].as<bool>();

      }
      if (timerObj.containsKey("esr")) {
        JsonObject endStateRelay = timerObj["esr"];
        if (endStateRelay.containsKey("use")) timer.endStateRelay.isUseSetting = endStateRelay["use"].as<bool>();
        if (endStateRelay.containsKey("rid")) timer.endStateRelay.relayId = endStateRelay["rid"];
        if (endStateRelay.containsKey("stp")) timer.endStateRelay.statePin = endStateRelay["stp"].as<bool>();
        if (endStateRelay.containsKey("lst")) timer.endStateRelay.lastState = endStateRelay["lst"].as<bool>();

      }
      device.timers.push_back(timer);
    }
  }

  if (doc.containsKey("ite")) device.isTimersEnabled = doc["ite"].as<bool>();
  if (doc.containsKey("iet")) device.isEncyclateTimers = doc["iet"].as<bool>();
  if (doc.containsKey("ise")) device.isScheduleEnabled = doc["ise"].as<bool>();
  if (doc.containsKey("iae")) device.isActionEnabled = doc["iae"].as<bool>();appState.isProcessWorkingJson = false;
  return true;
}

void DeviceManager::saveRelayStates(uint8_t targetRelayId) {
  for (auto& device : myDevices) {

    Relay* relay = findRelayById(device, targetRelayId);
    if (relay) {
      relay->lastState = relay->statePin;
    }

    if (device.temperature.relayId == targetRelayId) {
      if (relay) {
        device.temperature.lastState = relay->statePin;
      }
    }

    for (auto& timer : device.timers) {
      if (timer.initialStateRelay.relayId == targetRelayId) {
        timer.initialStateRelay.lastState = relay ? relay->statePin : false;
      }
      if (timer.endStateRelay.relayId == targetRelayId) {
        timer.endStateRelay.lastState = relay ? relay->statePin : false;
      }
    }

    for (auto& scenario : device.scheduleScenarios) {
      if (scenario.initialStateRelay.relayId == targetRelayId) {
        scenario.initialStateRelay.lastState = relay ? relay->statePin : false;
      }
      if (scenario.endStateRelay.relayId == targetRelayId) {
        scenario.endStateRelay.lastState = relay ? relay->statePin : false;
      }
    }
  }
}

void DeviceManager::restoreRelayStates(uint8_t targetRelayId) {
  for (auto& device : myDevices) {

    Relay* relay = findRelayById(device, targetRelayId);
    if (relay) {
      relay->statePin = relay->lastState;
    }

    if (device.temperature.relayId == targetRelayId) {
      if (relay) {
        relay->statePin = device.temperature.lastState;
      }
    }

    for (auto& timer : device.timers) {
      if (timer.initialStateRelay.relayId == targetRelayId) {
        if (relay) {
          relay->statePin = timer.initialStateRelay.lastState;
        }
      }
      if (timer.endStateRelay.relayId == targetRelayId) {
        if (relay) {
          relay->statePin = timer.endStateRelay.lastState;
        }
      }
    }

    for (auto& scenario : device.scheduleScenarios) {
      if (scenario.initialStateRelay.relayId == targetRelayId) {
        if (relay) {
          relay->statePin = scenario.initialStateRelay.lastState;
        }
      }
      if (scenario.endStateRelay.relayId == targetRelayId) {
        if (relay) {
          relay->statePin = scenario.endStateRelay.lastState;
        }
      }
    }
  }
}

Relay* DeviceManager::findRelayById(Device& device, uint8_t relayId) {
  for (auto& relay : device.relays) {
    if (relay.id == relayId) {
      return &relay;
    }
  }
  return nullptr;
}

bool DeviceManager::writeDevicesToFile(const std::vector<Device>& myDevices, const char* filename) {for (const auto& device : myDevices) {

    serializeDevice(device, filename, nullptr);
    appState.isProcessWorkingJson = false;
  }
  return true;
}

bool DeviceManager::readDevicesFromFile(std::vector<Device>& myDevices, const char* filename) {File file = SPIFFS.open(filename, "r");
    if (!file || !file.available()) {if (file) file.close();
        return false;
    }

    size_t fileSize = file.size();DynamicJsonDocument doc(fileSize * 1.5);

    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {return false;
    }

    myDevices.clear();

    JsonObject deviceObj = doc.as<JsonObject>();
    if (!deviceObj) {return false;
    }

    Device device;
    memset(&device, 0, sizeof(Device));

    if (deserializeDevice(deviceObj, device)) {
        myDevices.push_back(device);} else {return false;
    }return true;
}

int DeviceManager::getSelectedDeviceIndex(const std::vector<Device>& myDevices) {
  for (size_t i = 0; i < myDevices.size(); ++i) {
    if (myDevices[i].isSelected) {
      return i;
    }
  }
  return -1;
}

int DeviceManager::deviceInit() {
  if (!SPIFFS.exists("/devices.json")) {
    initializeDevice("MyDevice1", true);
    writeDevicesToFile(myDevices, "/devices.json");} else {
    if (readDevicesFromFile(myDevices, "/devices.json")) {
      return currentDeviceIndex = getSelectedDeviceIndex(myDevices);
    } else {initializeDevice("MyDevice1", true);
      return currentDeviceIndex = 0;
    }
  }
  return currentDeviceIndex = 0;
}

uint32_t DeviceManager::calculateDeviceFlagsChecksum() {
  if (myDevices.empty() || currentDeviceIndex >= myDevices.size()) {
    return 0;
  }
  const Device& device = myDevices[currentDeviceIndex];
  uint32_t hash = 5381;

  auto addToHash = [&hash](const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
      hash = ((hash << 5) + hash) + bytes[i];
    }
  };

  addToHash(device.nameDevice, strlen(device.nameDevice));
  addToHash(&device.isSelected, sizeof(device.isSelected));
  addToHash(&device.isTimersEnabled, sizeof(device.isTimersEnabled));
  addToHash(&device.isEncyclateTimers, sizeof(device.isEncyclateTimers));
  addToHash(&device.isScheduleEnabled, sizeof(device.isScheduleEnabled));
  addToHash(&device.isActionEnabled, sizeof(device.isActionEnabled));
  addToHash(&device.temperature.isUseSetting, sizeof(device.temperature.isUseSetting));

  return hash;
}

uint32_t DeviceManager::calculateOutputRelayChecksum() {
  Device& device = myDevices[currentDeviceIndex];
  uint32_t hash = 5381;

  auto addToHash = [&hash](const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
      hash = ((hash << 5) + hash) + bytes[i];
    }
  };

  for (const auto& relay : device.relays) {

    if (relay.isOutput) {

      addToHash(&relay.id, sizeof(relay.id));
      addToHash(&relay.pin, sizeof(relay.pin));
      addToHash(&relay.isOutput, sizeof(relay.isOutput));
      addToHash(&relay.isPwm, sizeof(relay.isPwm));
      addToHash(&relay.pwm, sizeof(relay.pwm));
      addToHash(&relay.manualMode, sizeof(relay.manualMode));
      addToHash(relay.description, strnlen(relay.description, MAX_DESCRIPTION_LENGTH));

      bool state = relay.statePin;
      addToHash(&state, sizeof(state));
    }
  }
  return hash;
}

uint32_t DeviceManager::calculateSensorValuesChecksum() {
  Device& device = myDevices[currentDeviceIndex];
  uint32_t hash = 5381;

  auto addToHash = [&hash](const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
      hash = ((hash << 5) + hash) + bytes[i];
    }
  };

  for (const auto& sensor : device.sensors) {

    if (sensor.isUseSetting) {

      addToHash(&sensor.currentValue, sizeof(sensor.currentValue));

      addToHash(&sensor.humidityValue, sizeof(sensor.humidityValue));
    }
  }
  return hash;
}

uint32_t DeviceManager::calculateTimersProgressChecksum() {
  if (myDevices.empty() || currentDeviceIndex >= myDevices.size()) {
    return 0;
  }
  const Device& device = myDevices[currentDeviceIndex];
  uint32_t hash = 5381;

  auto addToHash = [&hash](const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
      hash = ((hash << 5) + hash) + bytes[i];
    }
  };

  for (const auto& timer : device.timers) {
    if (timer.isUseSetting) {

      addToHash(&timer.progress.elapsedTime, sizeof(timer.progress.elapsedTime));
      addToHash(&timer.progress.remainingTime, sizeof(timer.progress.remainingTime));
      addToHash(&timer.progress.isRunning, sizeof(timer.progress.isRunning));
      addToHash(&timer.progress.isStopped, sizeof(timer.progress.isStopped));
    }
  }
  return hash;
}

bool DeviceManager::handleRelayCommand(const JsonObject& command, uint32_t clientNum) {

  if (myDevices.empty()) {return false;
  }Device& device = myDevices[currentDeviceIndex];

  int relayId = command["relay"];
  const char* action = command["action"];

  if (!action) {return false;
  }

  if (strcmp(action, "reset_all") == 0) {bool anyRelayFound = false;
    for (auto& relay : device.relays) {
      if (relay.isOutput) {
        relay.statePin = false;
        relay.manualMode = false;
        anyRelayFound = true;
      }
    }
    return anyRelayFound;
  }

  if (command["relay"].isNull()) {return false;
  }

  bool found = false;for (auto& relay : device.relays) {
    if (relay.id == relayId) {

      found = true;if (strcmp(action, "reset") == 0) {
        relay.manualMode = false;}

      else if (strcmp(action, "on") == 0 || strcmp(action, "off") == 0) {
        bool newState = (strcmp(action, "on") == 0);

        if (relay.statePin != newState || !relay.manualMode) {
          relay.statePin = newState;
          relay.manualMode = true;} else {}
      }
      else {found = false;
      }
      break;
    }
  }

  if (!found) {}

  return found;
}

void DeviceManager::serializeRelaysForControlTab(JsonObject& target) {
    if (myDevices.empty() || currentDeviceIndex >= myDevices.size()) {
        target["rel"] = JsonArray();
        return;
    }

    const Device& device = myDevices[currentDeviceIndex];
    JsonArray rel = target.createNestedArray("rel");

    for (const auto& relay : device.relays) {
        if (relay.isOutput) {
            JsonObject relayObj = rel.createNestedObject();
            relayObj["id"] = relay.id;
            relayObj["dsc"] = relay.description;
            relayObj["stp"] = relay.statePin;
            relayObj["man"] = relay.manualMode;
        }
    }
}

void DeviceManager::serializeSensorValues(JsonObject& target) {
    if (myDevices.empty() || currentDeviceIndex >= myDevices.size()) {
        target["sen"] = JsonArray();
        return;
    }

    const Device& device = myDevices[currentDeviceIndex];
    JsonArray sen = target.createNestedArray("sen");

    for (const auto& sensor : device.sensors) {
        if (sensor.isUseSetting) {
            JsonObject sensorObj = sen.createNestedObject();
            sensorObj["id"] = sensor.sensorId;
            sensorObj["cv"] = sensor.currentValue;
            sensorObj["hv"] = sensor.humidityValue;
        }
    }
}

void DeviceManager::serializeTimersProgress(JsonObject& target) {
   Serial.println("serializeTimersProgress");
    if (myDevices.empty() || currentDeviceIndex >= myDevices.size()) {
        target["tmr"] = JsonArray();
        return;
    }

    const Device& device = myDevices[currentDeviceIndex];
    JsonArray tmr = target.createNestedArray("tmr");

    for (size_t i = 0; i < device.timers.size(); ++i) {
        const Timer& timer = device.timers[i];
        const TimerInfo& progress = timer.progress;

        JsonObject timerObj = tmr.createNestedObject();

        timerObj["i"] = i;
        timerObj["e"] = timer.isUseSetting;
        timerObj["et"] = progress.elapsedTime;
        timerObj["rt"] = progress.remainingTime;
        timerObj["r"] = progress.isRunning;
        timerObj["s"] = progress.isStopped;

    }

}

void DeviceManager::serializeDeviceFlags(JsonObject& target) {
    if (myDevices.empty() || currentDeviceIndex >= myDevices.size()) {
        return;
    }

    const Device& device = myDevices[currentDeviceIndex];

    target["ite"] = device.isTimersEnabled;
    target["iet"] = device.isEncyclateTimers;
    target["ise"] = device.isScheduleEnabled;
    target["iae"] = device.isActionEnabled;
    target["tmp_use"] = device.temperature.isUseSetting;
}

size_t DeviceManager::currentStateSensors(char* buffer, size_t bufferSize, bool includeHeader) {
    if (myDevices.empty() || currentDeviceIndex >= myDevices.size()) {
        return snprintf(buffer, bufferSize, "Ошибка: Устройство не найдено или не настроено.");
    }

    Device& device = myDevices[currentDeviceIndex];
    size_t offset = 0;
    int written = 0;

    if (includeHeader) {
        written = snprintf(buffer + offset, bufferSize - offset, "🌡️ Сенсоры:\n");
        if (written < 0 || offset + written >= bufferSize) return offset;
        offset += written;
    }

    for (size_t i = 0; i < device.sensors.size(); i++) {
        const auto& sensor = device.sensors[i];
        if (!sensor.isUseSetting) {
            continue;
        }

        const char* sensorName = sensor.description;
        if (strlen(sensorName) == 0) {
            written = snprintf(buffer + offset, bufferSize - offset, "  ID:%d: ", sensor.sensorId);
        } else {
            written = snprintf(buffer + offset, bufferSize - offset, "  %s: ", sensorName);
        }
        if (written < 0 || offset + written >= bufferSize) return offset;
        offset += written;

        bool hasData = false;

        if (sensor.typeSensor.get(0) || sensor.typeSensor.get(1)) {
            if (!isnan(sensor.currentValue)) {
                written = snprintf(buffer + offset, bufferSize - offset, "T=%.1f°C", sensor.currentValue);
                hasData = true;
            } else {
                written = snprintf(buffer + offset, bufferSize - offset, "T=Ошибка");
            }
            if (written < 0 || offset + written >= bufferSize) return offset;
            offset += written;

            if (!isnan(sensor.humidityValue)) {
                written = snprintf(buffer + offset, bufferSize - offset, ", В=%.1f%%", sensor.humidityValue);
            } else if (hasData) {
                written = snprintf(buffer + offset, bufferSize - offset, ", В=Ошибка");
            }
            if (written < 0 || offset + written >= bufferSize) return offset;
            offset += written;
        }

        else if (sensor.typeSensor.get(2)) {
            if (!isnan(sensor.currentValue)) {
                written = snprintf(buffer + offset, bufferSize - offset, "NTC=%.1f°C", sensor.currentValue);
            } else {
                written = snprintf(buffer + offset, bufferSize - offset, "NTC=Ошибка");
            }
            if (written < 0 || offset + written >= bufferSize) return offset;
            offset += written;
        }

        else if (sensor.typeSensor.get(3)) {
            const char* state = (sensor.currentValue > 0.5) ? "НАЖАТО" : "ОТПУЩЕНО";
            written = snprintf(buffer + offset, bufferSize - offset, "Кнопка=%s", state);
            if (written < 0 || offset + written >= bufferSize) return offset;
            offset += written;
        }

        else if (sensor.typeSensor.get(4)) {
            if (!isnan(sensor.currentValue) && sensor.currentValue != -1.0f) {
                written = snprintf(buffer + offset, bufferSize - offset, "Аналог=%.0f", sensor.currentValue);
            } else {
                written = snprintf(buffer + offset, bufferSize - offset, "Аналог=Ошибка");
            }
            if (written < 0 || offset + written >= bufferSize) return offset;
            offset += written;
        } else {
             written = snprintf(buffer + offset, bufferSize - offset, "Не настроен");
             if (written < 0 || offset + written >= bufferSize) return offset;
             offset += written;
        }

        written = snprintf(buffer + offset, bufferSize - offset, "\n");
        if (written < 0 || offset + written >= bufferSize) return offset;
        offset += written;
    }
    return offset;
}

String DeviceManager::currentStateSensors() {
    constexpr size_t TEMP_BUFFER_SIZE = 1024;
    char tempBuffer[TEMP_BUFFER_SIZE];

    size_t len = currentStateSensors(tempBuffer, TEMP_BUFFER_SIZE);

    return String(tempBuffer, len);
}
