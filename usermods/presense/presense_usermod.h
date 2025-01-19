#pragma once

#include "wled.h"
#include <APDS9930.h>
#include <RD_03E.h>

class PresenseUsermod : public Usermod
{
private:
  APDS9930 apds = APDS9930();
  RD_03E radar = RD_03E(Serial1);
  float thresholdBrightness = 15;

  // 0 -> both radar and light sensor, 1 -> only radar, 2 -> only light sensor
  int mode = 0;

  const float BRIGHTNESS_CONSTANT = 0.5;
  const unsigned long LOOP_DELAY = 1000;
  const unsigned long AMBIENT_LIGHT_CHECK_DELAY = 1000;
  const unsigned long RADAR_CHECK_DELAY = 100;
  float ambientLight = 0;
  bool enabled = true;
  unsigned long lastRadarRunCheck = 0;
  unsigned long lastAmbientLightCheck = 0;

  unsigned long radarTimer = 0;
  bool humanPresentFlag = false;
  unsigned long radarTimeout = 10000;

  String modeToString(int mode)
  {
    switch (mode)
    {
    case 0:
      return "RADAR_AND_LIGHT_SENSOR";
    case 1:
      return "ONLY_RADAR";
    case 2:
      return "ONLY_LIGHT_SENSOR";
    default:
      return "RADAR_AND_LIGHT_SENSOR";
    }
  }

  int stringToMode(String mode)
  {
    if (mode == "RADAR_AND_LIGHT_SENSOR")
    {
      return 0;
    }
    else if (mode == "ONLY_RADAR")
    {
      return 1;
    }
    else if (mode == "ONLY_LIGHT_SENSOR")
    {
      return 2;
    }
    else
    {
      return 0;
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
      return ambientLight < thresholdBrightness;
    }

    if (!apds.readAmbientLightLux(ambientLight))
    {
      Serial.println(F("Error reading light values"));
      return false;
    }

    lastAmbientLightCheck = millis();

    return ambientLight < thresholdBrightness;
  }

  bool isHumanPresent()
  {
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

    switch (mode)
    {
    case 0:
      mode1();
      break;
    case 1:
      mode2();
      break;
    case 2:
      mode3();
      break;
    default:
      mode1();
      break;
    }
  }

  // Add this to expose settings in the web UI
  void addToJsonState(JsonObject &root) override
  {
    JsonObject presenseUsermod = root.createNestedObject("presenseUsermod");
    presenseUsermod["enabled"] = enabled;
    presenseUsermod["radarTimeout"] = radarTimeout;
    presenseUsermod["mode"] = modeToString(mode);

    JsonObject status = presenseUsermod.createNestedObject("status");
    status["ambientLight"] = ambientLight;
    status["radarStatus"] = radar.getStatus();
    status["radarDistance"] = radar.getDistance();
    status["radarLastSucessfulRead"] = radar.getLastSucessfulRead();
  }

  void readFromJsonState(JsonObject &root) override
  {
    if (root.containsKey("presenseUsermod"))
    {
      JsonObject presenseUsermod = root["presenseUsermod"];
      if (presenseUsermod.containsKey("enabled"))
      {
        enabled = presenseUsermod["enabled"];
      }
      if (presenseUsermod.containsKey("radarTimeout"))
      {
        radarTimeout = presenseUsermod["radarTimeout"];
      }
      if (presenseUsermod.containsKey("mode"))
      {
        mode = stringToMode(presenseUsermod["mode"].as<String>());
      }
    }
  }

  // Add usermod settings to the info page in the web UI
  void addToJsonInfo(JsonObject &root) override
  {
    JsonObject usermod = root.createNestedObject("PresenseUsermod");
    usermod["enabled"] = enabled;
  }
};
