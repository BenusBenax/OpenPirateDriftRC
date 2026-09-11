#include "IMU.h"

#include <math.h>


#if defined(OPENDRIFT_BOARD_HEADLESS)
// GY-521 / MPU6050 configured for +/-1000 deg/s (matching the QMI8658 full
// scale as closely as possible) and +/-4g of acceleration.
static constexpr float MPU_GYRO_RESOLUTION_DPS = 1000.0f / 32768.0f;
static constexpr float MPU_ACCEL_RESOLUTION_G = 4.0f / 32768.0f;
#endif


bool IMU::begin()
{
    initialized = false;

    // Weak pullups keep an unpopulated I2C bus deterministic while the
    // address probe below prevents initialize() from running without a sensor.
    pinMode(SDA_PIN, INPUT_PULLUP);
    pinMode(SCL_PIN, INPUT_PULLUP);
    delay(10);

    Wire.begin(SDA_PIN, SCL_PIN);


    #if defined(OPENDRIFT_BOARD_HEADLESS)

    // MPU6050 (GY-521) has no begin() helper; probe the bus before configuring
    // it. A missing sensor must not reach initialize(), which can otherwise
    // leave the ESP waiting on an unacknowledged I2C transaction.
    Wire.setTimeOut(100);
    Wire.beginTransmission(MPU6050_DEFAULT_ADDRESS);
    bool mpuPresent =
        Wire.endTransmission() == 0;

    if(!mpuPresent || !mpu.testConnection())
    {
        return false;
    }

    mpu.initialize();

    if(!mpu.testConnection())
    {
        return false;
    }

    mpu.setFullScaleGyroRange(
        MPU6050_GYRO_FS_1000
    );

    mpu.setFullScaleAccelRange(
        MPU6050_ACCEL_FS_4
    );

    gyroLpfMode = 0;
    initialized = true;

    return true;

    #else

    if (!qmi.begin(
        Wire,
        QMI8658_L_SLAVE_ADDRESS,
        SDA_PIN,
        SCL_PIN))
    {
        return false;
    }


    if(!qmi.configAccelerometer(
        SensorQMI8658::ACC_RANGE_4G,
        SensorQMI8658::ACC_ODR_1000Hz,
        SensorQMI8658::LPF_MODE_0
    ))
    {
        return false;
    }


    if(!qmi.configGyroscope(
        SensorQMI8658::GYR_RANGE_1024DPS,
        SensorQMI8658::GYR_ODR_896_8Hz,
        SensorQMI8658::LPF_MODE_0
    ))
    {
        return false;
    }

    gyroLpfMode = 0;


    qmi.enableAccelerometer();
    qmi.enableGyroscope();

    initialized = true;

    return true;

    #endif
}


bool IMU::isReady() const
{
    return initialized;
}


bool IMU::setGyroLpfMode(uint8_t mode)
{
    if(!initialized)
    {
        return false;
    }

    mode = constrain(mode, 0, 2);

    #if defined(OPENDRIFT_BOARD_HEADLESS)

    // MPU6050 has no user-facing hardware LPF selection exposed here; keep the
    // requested mode for telemetry/blackbox compatibility and apply nothing.
    gyroLpfMode = mode;
    return true;

    #else

    if(mode == gyroLpfMode)
    {
        return true;
    }

    SensorQMI8658::LpfMode sensorMode =
        mode == 1
        ? SensorQMI8658::LPF_MODE_3
        : (mode == 2
            ? SensorQMI8658::LPF_OFF
            : SensorQMI8658::LPF_MODE_0);

    if(!qmi.configGyroscope(
        SensorQMI8658::GYR_RANGE_1024DPS,
        SensorQMI8658::GYR_ODR_896_8Hz,
        sensorMode
    ))
    {
        return false;
    }

    gyroLpfMode = mode;
    return true;

    #endif
}


uint8_t IMU::getGyroLpfMode() const
{
    return gyroLpfMode;
}



void IMU::update()
{
    if(!initialized)
    {
        return;
    }

    uint32_t now = micros();

    float dt = 0.01f;

    if(lastUpdateMicros != 0)
    {
        dt =
            (now - lastUpdateMicros)
            /
            1000000.0f;

        dt = constrain(
            dt,
            0.001f,
            0.05f
        );
    }

    lastUpdateMicros = now;


    #if defined(OPENDRIFT_BOARD_HEADLESS)

    // MPU6050 returns raw ADC counts; scale them into engineering units.
    // Gyro is already in deg/s (DPS), matching what the PID core expects.
    int16_t rawX = 0;
    int16_t rawY = 0;
    int16_t rawZ = 0;

    mpu.getRotation(
        &rawX,
        &rawY,
        &rawZ
    );

    gyroX = (float)rawX * MPU_GYRO_RESOLUTION_DPS;
    gyroY = (float)rawY * MPU_GYRO_RESOLUTION_DPS;
    gyroZ = (float)rawZ * MPU_GYRO_RESOLUTION_DPS;

    int16_t rawAx = 0;
    int16_t rawAy = 0;
    int16_t rawAz = 0;

    mpu.getAcceleration(
        &rawAx,
        &rawAy,
        &rawAz
    );

    accelX = (float)rawAx * MPU_ACCEL_RESOLUTION_G;
    accelY = (float)rawAy * MPU_ACCEL_RESOLUTION_G;
    accelZ = (float)rawAz * MPU_ACCEL_RESOLUTION_G;

    #else

    qmi.getGyroscope(
        gyroX,
        gyroY,
        gyroZ
    );

    if(!qmi.getAccelerometer(
        accelX,
        accelY,
        accelZ
    ))
    {
        return;
    }

    #endif


    accelMagnitude = sqrtf(
        (accelX * accelX) +
        (accelY * accelY) +
        (accelZ * accelZ)
    );

    tiltRate = sqrtf(
        (gyroX * gyroX) +
        (gyroY * gyroY)
    );

    if(!accelFilterReady)
    {
        slowAccelX = accelX;
        slowAccelY = accelY;
        slowAccelZ = accelZ;
        accelFilterReady = true;
    }

    float slowAmount =
        1.0f - expf(-dt / 0.25f);

    slowAccelX +=
        (accelX - slowAccelX) * slowAmount;
    slowAccelY +=
        (accelY - slowAccelY) * slowAmount;
    slowAccelZ +=
        (accelZ - slowAccelZ) * slowAmount;

    float deltaX = accelX - slowAccelX;
    float deltaY = accelY - slowAccelY;
    float deltaZ = accelZ - slowAccelZ;

    accelDelta = sqrtf(
        (deltaX * deltaX) +
        (deltaY * deltaY) +
        (deltaZ * deltaZ)
    );

    // Terrain detector used to release settled-drift features during a hard
    // compression, unload, or pitch/roll impulse. It never creates steering
    // correction directly.
    float accelerationScore = constrain(
        (accelDelta - 0.06f) / 0.50f,
        0.0f,
        1.0f
    );

    float tiltScore = constrain(
        (tiltRate - 15.0f) / 180.0f,
        0.0f,
        1.0f
    );

    float unloadScore = constrain(
        (0.75f - accelMagnitude) / 0.55f,
        0.0f,
        1.0f
    );

    float scoreTarget = max(
        accelerationScore,
        max(
            tiltScore * 0.80f,
            unloadScore
        )
    );

    float scoreTimeConstant =
        scoreTarget > surfaceDisturbanceScore
        ?
        0.04f
        :
        0.20f;

    float scoreAmount =
        1.0f - expf(-dt / scoreTimeConstant);

    surfaceDisturbanceScore +=
        (scoreTarget - surfaceDisturbanceScore)
        *
        scoreAmount;

    surfaceDisturbanceScore = constrain(
        surfaceDisturbanceScore,
        0.0f,
        1.0f
    );
}



float IMU::getGyroX()
{
    return gyroX;
}



float IMU::getGyroY()
{
    return gyroY;
}



float IMU::getYawRate()
{
    return gyroZ;
}


float IMU::getAccelX()
{
    return accelX;
}


float IMU::getAccelY()
{
    return accelY;
}


float IMU::getAccelZ()
{
    return accelZ;
}


float IMU::getAccelMagnitude()
{
    return accelMagnitude;
}


float IMU::getAccelDelta()
{
    return accelDelta;
}


float IMU::getTiltRate()
{
    return tiltRate;
}


float IMU::getSurfaceDisturbanceScore()
{
    return surfaceDisturbanceScore;
}
