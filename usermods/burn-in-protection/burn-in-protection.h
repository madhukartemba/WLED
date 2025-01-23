#include "wled.h"

class BurnInProtection : public Usermod
{
private:
    bool enabled = true;
    unsigned long resetLedTimer = 0;
    unsigned long resetLedDelay = 600000;

    static const char *_name;
    static const char *_enabled;
    static const char *_resetLedDelay;

private:
    bool shouldReset()
    {
        return strip.getBrightness() == 0 || effectCurrent == 0;
    };

public:
    void setup()
    {
        if (!enabled)
            return;

        resetLedTimer = millis();
    }

    void loop()
    {
        if (!enabled)
            return;

        if (millis() - resetLedTimer > resetLedDelay && shouldReset())
        {
            colorUpdated(CALL_MODE_BUTTON);
            strip.show();
            resetLedTimer = millis();
        }
    }

    void addToJsonState(JsonObject &root) override
    {
        JsonObject burnInProtection = root.createNestedObject(FPSTR(_name));
        burnInProtection[FPSTR(_enabled)] = enabled;
        burnInProtection[FPSTR(_resetLedDelay)] = resetLedDelay;
    }

    void readFromJsonState(JsonObject &root) override
    {
        JsonObject top = root[FPSTR(_name)];
        if (top.isNull())
        {
            return;
        }
        getJsonValue(top[FPSTR(_enabled)], enabled);
        getJsonValue(top[FPSTR(_resetLedDelay)], resetLedDelay);
    }

    void addToConfig(JsonObject &root) override
    {
        JsonObject burnInProtection = root.createNestedObject(FPSTR(_name));
        burnInProtection[FPSTR(_enabled)] = enabled;
        burnInProtection[FPSTR(_resetLedDelay)] = resetLedDelay;
    }

    bool readFromConfig(JsonObject &root) override
    {
        JsonObject top = root[FPSTR(_name)];
        bool configComplete = !top.isNull();
        configComplete &= getJsonValue(top[FPSTR(_enabled)], enabled);
        configComplete &= getJsonValue(top[FPSTR(_resetLedDelay)], resetLedDelay);
        return configComplete;
    }
};

const char *BurnInProtection::_name = "BurnInProtection";
const char *BurnInProtection::_enabled = "enabled";
const char *BurnInProtection::_resetLedDelay = "resetLedDelay";