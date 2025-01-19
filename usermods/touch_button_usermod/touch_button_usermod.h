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
    Button button = Button(27);

public:
    void setup()
    {
        button.setup([]()
                           { Serial.println("Button clicked!"); });
    }

    void loop()
    {
        button.run();
    }
};