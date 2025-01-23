#pragma once

#include "wled.h"
#include <functional>

class Button
{
private:
    int pin;
    int lastState = LOW;
    boolean longPressCalled = false;
    unsigned long lastPressTime = 0;
    unsigned long lastDebounceTime = 0;
    static constexpr unsigned long LONG_PRESS_DELAY = 1000;
    static constexpr unsigned long DEBOUNCE_DELAY = 50;
    std::function<void()> onClick;
    std::function<void()> onLongPress;

public:
    Button(int pin)
        : pin(pin) {};

    void setup(std::function<void()> onClickCallback)
    {
        onClick = onClickCallback;
        pinMode(pin, INPUT);
    }

    void setup(std::function<void()> onClickCallback, std::function<void()> onLongPressCallback)
    {
        onLongPress = onLongPressCallback;
        setup(onClickCallback);
    }

    void run()
    {
        int newState = digitalRead(pin);
        unsigned long currentTime = millis();

        if (lastState == LOW && newState == HIGH)
        {
            lastPressTime = currentTime;
            lastDebounceTime = currentTime;
        }

        if (newState == HIGH)
        {
            if (currentTime - lastPressTime > LONG_PRESS_DELAY && !longPressCalled)
            {
                longPressCalled = true;
                if (onLongPress)
                {
                    onLongPress();
                }
            }
        }

        if (lastState == HIGH && newState == LOW)
        {
            if (currentTime - lastPressTime < LONG_PRESS_DELAY && currentTime - lastDebounceTime > DEBOUNCE_DELAY)
            {
                if (onClick)
                {
                    onClick();
                }
            }
            longPressCalled = false;
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
    Button resetToDefaultPresetButton = Button(33);

    int currentFavoriteEffectIndex = 0;
    // Add favorite effects here
    std::vector<uint8_t> favoriteEffects = {FX_MODE_COLORTWINKLE, FX_MODE_PIXELWAVE, FX_MODE_PLASMOID, FX_MODE_RAINBOW_CYCLE, FX_MODE_GRAVCENTRIC, FX_MODE_METEOR, FX_MODE_STARBURST, FX_MODE_RIPPLEPEAK};

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

    void switchEffects()
    {
        effectCurrent = favoriteEffects[currentFavoriteEffectIndex];
        currentFavoriteEffectIndex = (currentFavoriteEffectIndex + 1) % favoriteEffects.size();
        colorUpdated(CALL_MODE_BUTTON);
        strip.show();
    }

    void loadBootPreset()
    {
        applyPreset(1);
        stateUpdated(CALL_MODE_BUTTON);
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

        resetToDefaultPresetButton.setup([this]()
                                         { switchEffects(); },
                                         [this]()
                                         { loadBootPreset(); });
    }

    void loop()
    {
        onOffButton.run();
        reduceBrightnessButton.run();
        increaseBrightnessButton.run();
        resetToDefaultPresetButton.run();
    }
};