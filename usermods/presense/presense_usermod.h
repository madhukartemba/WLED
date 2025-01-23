#pragma once

#include "wled.h"
#include <APDS9930.h>
#include <RD_03E.h>

class PresenseUsermod : public Usermod
{
private:
  APDS9930 apds = APDS9930();
  RD_03E radar = RD_03E(Serial1);

  float ambientLightThresholdBrightness = 15;

  // 0 -> both radar and light sensor, 1 -> only radar, 2 -> only light sensor
  int mode = 0;

  const unsigned long AMBIENT_LIGHT_CHECK_DELAY = 1000;
  const unsigned long RADAR_CHECK_DELAY = 100;
  float ambientLight = 0;
  bool enabled = true;
  bool lastStripState;
  unsigned long lastRadarRunCheck = 0;
  unsigned long lastAmbientLightCheck = 0;

  unsigned long radarTimer = 0;
  bool humanPresentFlag = false;
  unsigned long radarTimeout = 10000;

  // Strings to reduce memory usage (used more than twice)
  static const char _name[];
  static const char _enabled[];
  static const char _radarTimeout[];
  static const char _mode[];
  static const char _ambientLightThreshold[];
  static const char _ambientLight[];
  static const char _radarStatus[];
  static const char _radarDistance[];
  static const char _radarLastSucessfulRead[];
  static const char *const _modes[];
  static const char *const _radarStatuses[];

  String modeToString(int mode)
  {
    switch (mode)
    {
    case 0:
      return FPSTR(_modes[0]);
    case 1:
      return FPSTR(_modes[1]);
    case 2:
      return FPSTR(_modes[2]);
    default:
      return FPSTR(_modes[0]);
    }
  }

  int stringToMode(String mode)
  {
    if (mode == FPSTR(_modes[0]))
    {
      return 0;
    }
    else if (mode == FPSTR(_modes[1]))
    {
      return 1;
    }
    else if (mode == FPSTR(_modes[2]))
    {
      return 2;
    }
    else
    {
      return 0;
    }
  }

  String radarStatusToString(int status)
  {
    switch (status)
    {
    case 0:
      return String(FPSTR(_radarStatuses[0]));
    case 1:
      return String(FPSTR(_radarStatuses[1]));
    case 2:
      return String(FPSTR(_radarStatuses[2]));
    default:
      return String(FPSTR(_radarStatuses[0]));
    }
  }

  void refreshRadar()
  {
    radar.run();
  }

  boolean isDark()
  {
    if (millis() - lastAmbientLightCheck < AMBIENT_LIGHT_CHECK_DELAY)
    {
      return ambientLight < ambientLightThresholdBrightness;
    }

    if (!apds.readAmbientLightLux(ambientLight))
    {
      Serial.println(F("Error reading light values"));
      return false;
    }

    lastAmbientLightCheck = millis();

    return ambientLight < ambientLightThresholdBrightness;
  }

  bool isHumanPresent()
  {
    refreshRadar();
    bool presence = radar.isHumanPresent();
    unsigned long currentTime = millis();

    if (presence)
    {
      radarTimer = currentTime;
      humanPresentFlag = true;
    }
    else if (
        currentTime - radarTimer > radarTimeout)
    {
      humanPresentFlag = false;
      radarTimer = currentTime;
    }

    return humanPresentFlag;
  }

  void switchStrip(bool switchOn)
  {

    if (switchOn == lastStripState)
    {
      return;
    }

    lastStripState = switchOn;

    if (bri && switchOn)
      return; // If already on and trying to switch on, do nothing
    if (!bri && !switchOn)
      return; // If already off and trying to switch off, do nothing

    if (switchOn)
    {
      // Turn on the strip
      bri = briLast > 0 ? briLast : 255; // Restore last brightness or use maximum brightness
      stateUpdated(CALL_MODE_BUTTON);
    }
    else
    {
      // Turn off the strip
      briLast = bri; // Save the current brightness
      bri = 0;       // Set brightness to 0 (off)
      stateUpdated(CALL_MODE_BUTTON);
    }

    strip.show();
  }

  void mode1()
  {
    if (isDark())
    {
      if (isHumanPresent())
      {
        switchStrip(true);
      }
      else
      {
        switchStrip(false);
      }
    }
    else
    {
      switchStrip(false);
    }
  }

  void mode2()
  {
    if (isHumanPresent())
    {
      switchStrip(true);
    }
    else
    {
      switchStrip(false);
    }
  }

  void mode3()
  {
    if (isDark())
    {
      switchStrip(true);
    }
    else
    {
      switchStrip(false);
    }
  }

public:
  void setup()
  {
    if (!enabled)
      return; // Skip setup if disabled

    Serial.println("Hello from presense usermod!");

    // Initialize APDS-9930 (configure I2C and initial values)
    if (apds.init())
    {
      Serial.println(F("APDS-9930 initialization complete"));
    }
    else
    {
      Serial.println(F("Something went wrong during APDS-9930 init!"));
    }

    // Start running the APDS-9930 light sensor (no interrupts)
    if (apds.enableLightSensor(false))
    {
      Serial.println(F("Light sensor is now running"));
    }
    else
    {
      Serial.println(F("Something went wrong during light sensor init!"));
    }

    // Initialize the radar sensor
    radar.begin(18, 19); // RX, TX
  }

  void loop()
  {
    if (!enabled)
      return;

    if (mode == 0)
    {
      mode1();
    }
    else if (mode == 1)
    {
      mode2();
    }
    else if (mode == 2)
    {
      mode3();
    }
  }

  // Add this to expose settings in the web UI
  void addToJsonState(JsonObject &root) override
  {
    JsonObject presenseUsermod = root.createNestedObject(FPSTR(_name));
    presenseUsermod[FPSTR(_enabled)] = enabled;
    presenseUsermod[FPSTR(_radarTimeout)] = radarTimeout;
    presenseUsermod[FPSTR(_mode)] = modeToString(mode);

    JsonObject status = presenseUsermod.createNestedObject(F("status"));
    status[FPSTR(_ambientLight)] = ambientLight;
    status[FPSTR(_radarStatus)] = radarStatusToString(radar.getStatus());
    status[FPSTR(_radarDistance)] = radar.getDistance();
    status[FPSTR(_radarLastSucessfulRead)] = radar.getLastSucessfulRead();
  }

  void readFromJsonState(JsonObject &root) override
  {
    if (root.containsKey(FPSTR(_name)))
    {
      JsonObject presenseUsermod = root[FPSTR(_name)];
      if (presenseUsermod.containsKey(FPSTR(_enabled)))
      {
        enabled = presenseUsermod[FPSTR(_enabled)];
      }
      if (presenseUsermod.containsKey(FPSTR(_radarTimeout)))
      {
        radarTimeout = presenseUsermod[FPSTR(_radarTimeout)];
      }
      if (presenseUsermod.containsKey(FPSTR(_mode)))
      {
        mode = stringToMode(presenseUsermod[FPSTR(_mode)].as<String>());
      }
    }
  }

  void addToJsonInfo(JsonObject &root) override
  {
    JsonObject user = root["u"];
    if (user.isNull())
    {
      user = root.createNestedObject("u");
    }

    JsonArray infoArr = user.createNestedArray(FPSTR(_name));

    // Add toggle button with spacing
    String uiDomString = F("<div style=\"margin-bottom: 10px;\">");
    uiDomString += F("<button class=\"btn btn-xs\" onclick=\"requestJson({");
    uiDomString += FPSTR(_name);
    uiDomString += F(":{");
    uiDomString += FPSTR(_enabled);
    uiDomString += enabled ? F(":false}});\">") : F(":true}});\">");
    uiDomString += F("<i class=\"icons");
    uiDomString += enabled ? F(" on") : F(" off");
    uiDomString += F("\">&#xe08f;</i>");
    uiDomString += F("</button>");
    uiDomString += F("</div>");
    infoArr.add(uiDomString);

    if (enabled)
    {
      // Add mode selection dropdown with spacing
      uiDomString = F("<div style=\"margin-bottom: 10px;\">");
      uiDomString += F("<select class=\"form-control\" style=\"display: inline-block; width: auto;\" onchange=\"requestJson({");
      uiDomString += FPSTR(_name);
      uiDomString += F(":{");
      uiDomString += FPSTR(_mode);
      uiDomString += F(":this.value}});\">");

      // Radar and Light Sensor mode
      uiDomString += F("<option value=\"");
      uiDomString += FPSTR(_modes[0]);
      uiDomString += F("\"");
      uiDomString += (mode == 0) ? F(" selected") : F("");
      uiDomString += F(">Radar and Light Sensor</option>");

      // Only Radar mode
      uiDomString += F("<option value=\"");
      uiDomString += FPSTR(_modes[1]);
      uiDomString += F("\"");
      uiDomString += (mode == 1) ? F(" selected") : F("");
      uiDomString += F(">Radar Only</option>");

      // Only Light Sensor mode
      uiDomString += F("<option value=\"");
      uiDomString += FPSTR(_modes[2]);
      uiDomString += F("\"");
      uiDomString += (mode == 2) ? F(" selected") : F("");
      uiDomString += F(">Light Sensor Only</option>");
      uiDomString += F("</select></div>");

      // Add status information
      uiDomString += F("<div style=\"margin-bottom: 5px;\">");
      uiDomString += F("<strong>Ambient Light:</strong> ");
      uiDomString += String(ambientLight);
      uiDomString += F("</div>");
      uiDomString += F("<div style=\"margin-bottom: 5px;\">");
      uiDomString += F("<strong>Radar Status:</strong> ");
      uiDomString += radarStatusToString(radar.getStatus());
      uiDomString += F("</div>");
      infoArr.add(uiDomString);
    }
  }

  void addToConfig(JsonObject &root) override
  {
    JsonObject presenseUsermod = root.createNestedObject(FPSTR(_name));
    presenseUsermod[FPSTR(_enabled)] = enabled;
    presenseUsermod[FPSTR(_radarTimeout)] = radarTimeout;
    presenseUsermod[FPSTR(_mode)] = modeToString(mode);
    presenseUsermod[FPSTR(_ambientLightThreshold)] = ambientLightThresholdBrightness;
  }

  bool readFromConfig(JsonObject &root) override
  {
    JsonObject top = root[FPSTR(_name)];
    bool configComplete = !top.isNull();
    configComplete &= getJsonValue(top[FPSTR(_enabled)], enabled);
    configComplete &= getJsonValue(top[FPSTR(_radarTimeout)], radarTimeout);
    configComplete &= getJsonValue(top[FPSTR(_mode)], mode);
    configComplete &= getJsonValue(top[FPSTR(_ambientLightThreshold)], ambientLightThresholdBrightness);

    return configComplete;
  }
};

const char PresenseUsermod::_name[] PROGMEM = "Presense";
const char PresenseUsermod::_enabled[] PROGMEM = "enabled";
const char PresenseUsermod::_radarTimeout[] PROGMEM = "radarTimeout";
const char PresenseUsermod::_mode[] PROGMEM = "mode";
const char PresenseUsermod::_ambientLightThreshold[] PROGMEM = "ambientLightThreshold";
const char PresenseUsermod::_ambientLight[] PROGMEM = "ambientLight";
const char PresenseUsermod::_radarStatus[] PROGMEM = "radarStatus";
const char PresenseUsermod::_radarDistance[] PROGMEM = "radarDistance";
const char PresenseUsermod::_radarLastSucessfulRead[] PROGMEM = "radarLastSucessfulRead";
const char *const PresenseUsermod::_modes[] PROGMEM = {"RADAR_AND_LIGHT", "RADAR_ONLY", "LIGHT_ONLY"};
const char *const PresenseUsermod::_radarStatuses[] PROGMEM = {"HUMAN_ABSENT", "HUMAN_MOVING", "HUMAN_PRESENT"};
