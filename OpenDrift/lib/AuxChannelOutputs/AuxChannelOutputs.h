#pragma once

#include <Arduino.h>

#if defined(OPENDRIFT_BOARD_C3)

// The auxiliary-channel implementation uses MCPWM, which is unavailable on ESP32-C3.
class Settings;
class CrsfInput;

class AuxChannelOutputs
{
public:
    bool begin(Settings& settings)
    {
        (void)settings;
        return true;
    }

    void update(
        Settings& settings,
        const CrsfInput& crsf,
        bool signalValid
    )
    {
        (void)settings;
        (void)crsf;
        (void)signalValid;
    }

    static bool isPinAvailable(uint8_t gpio)
    {
        (void)gpio;
        return false;
    }
};

#else

#include "CrsfInput.h"
#include "Settings.h"


class AuxChannelOutputs
{
public:

    static constexpr uint8_t FIRST_GPIO = 1;
    static constexpr uint8_t LAST_GPIO = 8;
    static constexpr uint8_t OUTPUT_COUNT = 8;

    bool begin(
        Settings& settings
    );

    void update(
        Settings& settings,
        const CrsfInput& crsf,
        bool signalValid
    );

    static bool isPinAvailable(
        uint8_t gpio
    );


private:

    bool attached[OUTPUT_COUNT] = {false};
    uint16_t lastPulse[OUTPUT_COUNT] = {0};
    bool timerInitialized[2][3] = {{false}};

    bool attachOutput(
        uint8_t slot
    );

    void detachOutput(
        uint8_t slot
    );

    void writeOutput(
        uint8_t slot,
        uint16_t pulse
    );
};


#endif
