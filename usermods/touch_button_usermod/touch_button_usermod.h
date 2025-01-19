#pragma once

#include "wled.h"

#include <functional>

class Button
{
private:
    int pin;
    int lastState = LOW;
    std::function<void()> onClick;

public:
    Button(int pin)
        : pin(pin) {};

    void setup(std::function<void()> onClickCallback)
    {
        onClick = onClickCallback;
        pinMode(pin, INPUT);
    }

    void run()
    {
        int newState = digitalRead(pin);
        if (lastState == HIGH && newState == LOW)
        {
            if (onClick)
                onClick();
        }
        lastState = newState;
    }
};

class TouchButtonUsermod : public Usermod
{
private:
    Button onOffButton = Button(27);
    Button reduceBrightnessButton = Button(26);
    Button increaseBrightnessButton = Button(25);

    static constexpr unsigned long READ_DELAY = 100;
    unsigned long readTimer = 0;

public:
    void toggleStrip()
    {
        if (bri > 0)
        {
            briLast = bri;
            bri = 0;
        }
        else
        {
            bri = briLast > 0 ? briLast : 255;
        }
        stateUpdated(CALL_MODE_BUTTON);
        strip.show();
    }

    void reduceBrightness()
    {
        if (bri > 0)
        {
            bri = max(bri - 25, 0);
            stateUpdated(CALL_MODE_BUTTON);
            strip.show();
        }
    }

    void increaseBrightness()
    {
        if (bri < 255)
        {
            bri = min(bri + 25, 255);
            stateUpdated(CALL_MODE_BUTTON);
            strip.show();
        }
    }

public:
    void setup()
    {
        onOffButton.setup([this]()
                          { toggleStrip(); });

        reduceBrightnessButton.setup([this]()
                                     { reduceBrightness(); });

        increaseBrightnessButton.setup([this]()
                                       { increaseBrightness(); });
    }

    void loop()
    {
        if (millis() - readTimer > READ_DELAY)
        {
            onOffButton.run();
            reduceBrightnessButton.run();
            increaseBrightnessButton.run();
            readTimer = millis();
        }
    }
};