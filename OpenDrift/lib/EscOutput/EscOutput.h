#pragma once

#include <Arduino.h>


// ESC pulses deliberately use LEDC instead of ESP32Servo. On ESP32-S3 the
// current ESP32Servo MCPWM allocator can route a second frequency through the
// first timer's signal, causing the throttle pin to mirror steering output.
class EscOutput
{
public:

    bool begin(
        int outputPin,
        int frequencyHz = 50
    );

    void end();

    void writeMicroseconds(
        int pulseUs
    );

    void configure(
        int centerPulse,
        bool reversed,
        int travelPercent,
        int quietBand
    );

    bool isActive() const;


private:

    // LEDC channel for the ESC output. ESP32-C3 exposes only 6 LEDC channels
    // (0-5): channel 7 does not exist there and LEDC_SETUP silently fails.
    // On C3 the servo (ESP32Servo) occupies channel 0 (timer 0) at the control
    // rate, so the ESC must run on a different timer: channels 2/3 -> timer 1,
    // channels 4/5 -> timer 2. Channel 2 (timer 1) keeps the 50 Hz ESC signal
    // independent of the servo timer. Other boards keep channel 7 as before.
    #if defined(OPENDRIFT_BOARD_C3)
    static constexpr uint8_t LEDC_CHANNEL = 2;
    #else
    static constexpr uint8_t LEDC_CHANNEL = 7;
    #endif
    // ESP32-C3 LEDC supports at most 14-bit duty resolution in this Arduino
    // core. At 50 Hz this still resolves an ESC pulse to about 1.22 us.
    static constexpr uint8_t LEDC_RESOLUTION_BITS = 14;

    int pin = -1;
    int frequency = 50;
    int currentPulse = 1500;
    int center = 1500;
    bool reversed = false;
    int travel = 100;
    int quiet = 0;
    bool active = false;
};
