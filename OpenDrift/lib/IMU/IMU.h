#pragma once

#include <Arduino.h>
#include <Wire.h>

#if !defined(OPENDRIFT_BOARD_HEADLESS)
#include "SensorQMI8658.hpp"
#else
#include <MPU6050.h>
#endif


class IMU
{
public:

    IMU() {}
    IMU(const IMU&) {}
    IMU& operator=(const IMU&) { return *this; }

    bool begin();
    bool isReady() const;

    bool setGyroLpfMode(uint8_t mode);
    uint8_t getGyroLpfMode() const;

    void update();


    float getGyroX();
    float getGyroY();
    float getYawRate();

    float getAccelX();
    float getAccelY();
    float getAccelZ();
    float getAccelMagnitude();
    float getAccelDelta();
    float getTiltRate();
    float getSurfaceDisturbanceScore();


private:

    #if !defined(OPENDRIFT_BOARD_HEADLESS)
    SensorQMI8658 qmi;
    #else
    // GY-521 / MPU6050 wired to the standard ESP32 I2C pins.
    MPU6050 mpu;
    #endif

    float gyroX = 0;
    float gyroY = 0;
    float gyroZ = 0;

    float accelX = 0;
    float accelY = 0;
    float accelZ = 0;

    float slowAccelX = 0;
    float slowAccelY = 0;
    float slowAccelZ = 0;

    float accelMagnitude = 0;
    float accelDelta = 0;
    float tiltRate = 0;
    float surfaceDisturbanceScore = 0;

    bool accelFilterReady = false;

    bool initialized = false;

    uint8_t gyroLpfMode = 0;

    uint32_t lastUpdateMicros = 0;


    #if defined(OPENDRIFT_BOARD_HEADLESS)
        #if defined(OPENDRIFT_BOARD_C3)
        // ESP32-C3: GPIO 6 (SDA) / GPIO 7 (SCL) for I2C
        static constexpr int SDA_PIN = 6;
        static constexpr int SCL_PIN = 7;
        #else
        // Generic ESP32: GPIO 21 (SDA) / GPIO 22 (SCL) for I2C
        static constexpr int SDA_PIN = 21;
        static constexpr int SCL_PIN = 22;
        #endif
    #elif defined(OPENDRIFT_BOARD_AMOLED_164)
    static constexpr int SDA_PIN = 47;
    static constexpr int SCL_PIN = 48;
    #else
    static constexpr int SDA_PIN = 6;
    static constexpr int SCL_PIN = 7;
    #endif
};
