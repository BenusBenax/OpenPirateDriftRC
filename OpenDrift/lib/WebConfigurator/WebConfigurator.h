#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "Settings.h"
#include "GyroController.h"
#include "RadioInput.h"
#include "BlackboxLogger.h"
#include "IMU.h"
#include "CrsfInput.h"
#include "Servo.h"
#include "EscOutput.h"


class WebConfigurator
{
public:

    WebConfigurator();

    void begin(
        Settings& settings,
        GyroController& gyro,
        RadioInput& steeringRadio,
        RadioInput& gainRadio,
        RadioInput& throttleRadio,
        BlackboxLogger& blackbox,
        IMU* imuRef = nullptr,
        bool imuReady = false,
        CrsfInput* crsfRef = nullptr,
        ServoOutput* servoRef = nullptr,
        EscOutput* escRef = nullptr,
        volatile bool* hwTestFlag = nullptr,
        uint8_t motorOutputPin = 0
    );

    void update();

    bool isRunning();


private:

    WebServer server;

    Settings* settings = nullptr;

    GyroController* gyro = nullptr;

    RadioInput* steeringRadio = nullptr;

    RadioInput* gainRadio = nullptr;

    RadioInput* throttleRadio = nullptr;

    BlackboxLogger* blackbox = nullptr;

    IMU* imu = nullptr;

    bool imuOk = false;

    CrsfInput* crsf = nullptr;

    bool running = false;

    ServoOutput* servoOut = nullptr;

    EscOutput* escOut = nullptr;

    // Shared with main.cpp so the control/gyro loop stops writing the servo
    // and throttle while a bench hardware test is running.
    volatile bool* hardwareTestFlag = nullptr;

    uint8_t motorOutputPin = 0;

    volatile bool hwTestRunning = false;

    TaskHandle_t hwTestTaskHandle = nullptr;

    void handleRoot();

    void handleLiveStatus();

    void handleSave();

    void handleProfileCreate();

    void handleProfileActivate();

    void handleProfileDelete();

    void handleLogDownload();

    void handleLogClear();

    void handleTestServo();

    void handleTestMotor();

    void startHardwareTest(bool motor);

    static void hardwareTestTask(void* param);

    void handleNotFound();

    String input(
        const char* label,
        const char* name,
        String value,
        const char* type = "number",
        const char* step = "1"
    );

    String checkbox(
        const char* label,
        const char* name,
        bool checked
    );

    int getIntArg(
        const char* name,
        int fallback
    );

    float getFloatArg(
        const char* name,
        float fallback
    );
};
