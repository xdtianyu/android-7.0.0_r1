/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>
#include <float.h>

#include <eventnums.h>
#include <heap.h>
#include <hostIntf.h>
#include <i2c.h>
#include <nanohubPacket.h>
#include <sensors.h>
#include <seos.h>
#include <timer.h>

#define BMP280_APP_ID APP_ID_MAKE(APP_ID_VENDOR_GOOGLE, 5)

#define I2C_BUS_ID                      0
#define I2C_SPEED                       400000
#define I2C_ADDR                        0x76

#define BOSCH_BMP280_ID                 0x58

#define BOSCH_BMP280_REG_RESET          0x60
#define BOSCH_BMP280_REG_DIG_T1         0x88
#define BOSCH_BMP280_REG_ID             0xd0
#define BOSCH_BMP280_REG_CTRL_MEAS      0xf4
#define BOSCH_BMP280_REG_CONFIG         0xf5
#define BOSCH_BMP280_REG_PRES_MSB       0xf7

// temp: 2x oversampling, baro: 16x oversampling, power: normal
#define CTRL_ON    ((2 << 5) | (5 << 2) | 3)
// temp: 2x oversampling, baro: 16x oversampling, power: sleep
#define CTRL_SLEEP ((2 << 5) | (5 << 2))

enum BMP280SensorEvents
{
    EVT_SENSOR_I2C = EVT_APP_START + 1,
    EVT_SENSOR_BARO_TIMER,
    EVT_SENSOR_TEMP_TIMER,
};

enum BMP280TaskState
{
    STATE_RESET,
    STATE_VERIFY_ID,
    STATE_AWAITING_COMP_PARAMS,
    STATE_CONFIG,
    STATE_FINISH_INIT,
    STATE_IDLE,
    STATE_ENABLING_BARO,
    STATE_ENABLING_TEMP,
    STATE_DISABLING_BARO,
    STATE_DISABLING_TEMP,
    STATE_SAMPLING,
};

struct BMP280CompParams
{
    uint16_t dig_T1;
    int16_t dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
} __attribute__((packed));

static struct BMP280Task
{
    struct BMP280CompParams comp;

    uint32_t id;
    uint32_t baroHandle;
    uint32_t tempHandle;
    uint32_t baroTimerHandle;
    uint32_t tempTimerHandle;

    float offset;

    uint8_t txrxBuf[24];

    bool baroOn;
    bool tempOn;
    bool baroReading;
    bool baroCalibrating;
    bool tempReading;
} mTask;

struct CalibrationData {
    struct HostHubRawPacket header;
    struct SensorAppEventHeader data_header;
    float value;
} __attribute__((packed));

static const uint32_t tempSupportedRates[] =
{
    SENSOR_HZ(0.1),
    SENSOR_HZ(1),
    SENSOR_HZ(5),
    SENSOR_HZ(10),
    SENSOR_HZ(25),
    0,
};

static const uint64_t rateTimerValsTemp[] = //should match "supported rates in length" and be the timer length for that rate in nanosecs
{
    10 * 1000000000ULL,
     1 * 1000000000ULL,
    1000000000ULL / 5,
    1000000000ULL / 10,
    1000000000ULL / 25,
};

static const uint32_t baroSupportedRates[] =
{
    SENSOR_HZ(0.1),
    SENSOR_HZ(1),
    SENSOR_HZ(5),
    SENSOR_HZ(10),
    0
};

static const uint64_t rateTimerValsBaro[] = //should match "supported rates in length" and be the timer length for that rate in nanosecs
{
    10 * 1000000000ULL,
     1 * 1000000000ULL,
    1000000000ULL / 5,
    1000000000ULL / 10,
};

/* sensor callbacks from nanohub */

static void i2cCallback(void *cookie, size_t tx, size_t rx, int err)
{
    if (err == 0)
        osEnqueuePrivateEvt(EVT_SENSOR_I2C, cookie, NULL, mTask.id);
    else
        osLog(LOG_INFO, "BMP280: i2c error (%d)\n", err);
}

static void baroTimerCallback(uint32_t timerId, void *cookie)
{
    osEnqueuePrivateEvt(EVT_SENSOR_BARO_TIMER, cookie, NULL, mTask.id);
}

static void tempTimerCallback(uint32_t timerId, void *cookie)
{
    osEnqueuePrivateEvt(EVT_SENSOR_TEMP_TIMER, cookie, NULL, mTask.id);
}

static void setMode(bool on, void *cookie)
{
    mTask.txrxBuf[0] = BOSCH_BMP280_REG_CTRL_MEAS;
    mTask.txrxBuf[1] = (on) ? CTRL_ON : CTRL_SLEEP;
    i2cMasterTx(I2C_BUS_ID, I2C_ADDR, mTask.txrxBuf, 2, &i2cCallback,
                cookie);
}

static void sendCalibrationResult(uint8_t status, float value) {
    struct CalibrationData *data = heapAlloc(sizeof(struct CalibrationData));
    if (!data) {
        osLog(LOG_WARN, "Couldn't alloc cal result pkt");
        return;
    }

    data->header.appId = BMP280_APP_ID;
    data->header.dataLen = (sizeof(struct CalibrationData) - sizeof(struct HostHubRawPacket));
    data->data_header.msgId = SENSOR_APP_MSG_ID_CAL_RESULT;
    data->data_header.sensorType = SENS_TYPE_BARO;
    data->data_header.status = status;

    data->value = value;

    if (!osEnqueueEvtOrFree(EVT_APP_TO_HOST, data, heapFree))
        osLog(LOG_WARN, "Couldn't send cal result evt");
}

// TODO: only turn on the timer when enabled
static bool sensorPowerBaro(bool on, void *cookie)
{
    bool oldMode = mTask.baroOn || mTask.tempOn;
    bool newMode = on || mTask.tempOn;

    if (!on && mTask.baroTimerHandle) {
        timTimerCancel(mTask.baroTimerHandle);
        mTask.baroTimerHandle = 0;
        mTask.baroReading = false;
    }

    if (oldMode != newMode)
        setMode(newMode, (void*)(on ? STATE_ENABLING_BARO : STATE_DISABLING_BARO));
    else
        sensorSignalInternalEvt(mTask.baroHandle, SENSOR_INTERNAL_EVT_POWER_STATE_CHG, on, 0);

    mTask.baroOn = on;

    return true;
}

static bool sensorFirmwareBaro(void *cookie)
{
    return sensorSignalInternalEvt(mTask.baroHandle, SENSOR_INTERNAL_EVT_FW_STATE_CHG, 1, 0);
}

static bool sensorRateBaro(uint32_t rate, uint64_t latency, void *cookie)
{
    if (mTask.baroTimerHandle)
        timTimerCancel(mTask.baroTimerHandle);
    mTask.baroTimerHandle = timTimerSet(sensorTimerLookupCommon(baroSupportedRates, rateTimerValsBaro, rate), 0, 50, baroTimerCallback, NULL, false);
    return sensorSignalInternalEvt(mTask.baroHandle, SENSOR_INTERNAL_EVT_RATE_CHG, rate, latency);
}

static bool sensorFlushBaro(void *cookie)
{
    return osEnqueueEvt(sensorGetMyEventType(SENS_TYPE_BARO), SENSOR_DATA_EVENT_FLUSH, NULL);
}

static bool sensorCalibrateBaro(void *cookie)
{
    if (mTask.baroOn || mTask.tempOn) {
        osLog(LOG_ERROR, "BMP280: cannot calibrate while baro or temp are active\n");
        sendCalibrationResult(SENSOR_APP_EVT_STATUS_BUSY, 0.0f);
        return false;
    }

    if (mTask.baroTimerHandle)
        timTimerCancel(mTask.baroTimerHandle);
    mTask.baroTimerHandle = timTimerSet(100000000ull, 0, 50, baroTimerCallback, NULL, false);

    mTask.offset = 0.0f;
    mTask.baroOn = true;
    mTask.baroCalibrating = true;

    mTask.txrxBuf[0] = BOSCH_BMP280_REG_CTRL_MEAS;
    mTask.txrxBuf[1] = CTRL_ON;
    i2cMasterTx(I2C_BUS_ID, I2C_ADDR, mTask.txrxBuf, 2, &i2cCallback, (void*)STATE_IDLE);

    return true;
}

static bool sensorCfgDataBaro(void *data, void *cookie)
{
    mTask.offset = *((float*)data) * 100.0f; // offset is given in hPa, but used as Pa in compensation
    return true;
}

static bool sensorPowerTemp(bool on, void *cookie)
{
    bool oldMode = mTask.baroOn || mTask.tempOn;
    bool newMode = on || mTask.baroOn;

    if (!on && mTask.tempTimerHandle) {
        timTimerCancel(mTask.tempTimerHandle);
        mTask.tempTimerHandle = 0;
        mTask.tempReading = false;
    }

    if (oldMode != newMode)
        setMode(newMode, (void*)(on ? STATE_ENABLING_TEMP : STATE_DISABLING_TEMP));
    else
        sensorSignalInternalEvt(mTask.tempHandle, SENSOR_INTERNAL_EVT_POWER_STATE_CHG, on, 0);

    mTask.tempOn = on;

    return true;
}

static bool sensorFirmwareTemp(void *cookie)
{
    sensorSignalInternalEvt(mTask.tempHandle, SENSOR_INTERNAL_EVT_FW_STATE_CHG, 1, 0);
    return true;
}

static bool sensorRateTemp(uint32_t rate, uint64_t latency, void *cookie)
{
    if (mTask.tempTimerHandle)
        timTimerCancel(mTask.tempTimerHandle);
    mTask.tempTimerHandle = timTimerSet(sensorTimerLookupCommon(tempSupportedRates, rateTimerValsTemp, rate), 0, 50, tempTimerCallback, NULL, false);
    sensorSignalInternalEvt(mTask.tempHandle, SENSOR_INTERNAL_EVT_RATE_CHG, rate, latency);
    return true;
}

static bool sensorFlushTemp(void *cookie)
{
    return osEnqueueEvt(sensorGetMyEventType(SENS_TYPE_TEMP), SENSOR_DATA_EVENT_FLUSH, NULL);
}

static const struct SensorInfo sensorInfoBaro =
{
    .sensorName = "Pressure",
    .supportedRates = baroSupportedRates,
    .sensorType = SENS_TYPE_BARO,
    .numAxis = NUM_AXIS_EMBEDDED,
    .interrupt = NANOHUB_INT_NONWAKEUP,
    .minSamples = 300
};

static const struct SensorOps sensorOpsBaro =
{
    .sensorPower = sensorPowerBaro,
    .sensorFirmwareUpload = sensorFirmwareBaro,
    .sensorSetRate = sensorRateBaro,
    .sensorFlush = sensorFlushBaro,
    .sensorCalibrate = sensorCalibrateBaro,
    .sensorCfgData = sensorCfgDataBaro,
};

static const struct SensorInfo sensorInfoTemp =
{
    .sensorName = "Temperature",
    .supportedRates = tempSupportedRates,
    .sensorType = SENS_TYPE_TEMP,
    .numAxis = NUM_AXIS_EMBEDDED,
    .interrupt = NANOHUB_INT_NONWAKEUP,
    .minSamples = 20
};

static const struct SensorOps sensorOpsTemp =
{
    .sensorPower = sensorPowerTemp,
    .sensorFirmwareUpload = sensorFirmwareTemp,
    .sensorSetRate = sensorRateTemp,
    .sensorFlush = sensorFlushTemp,
};

// Returns temperature in units of 0.01 degrees celsius.
static int32_t compensateTemp( int32_t adc_T, int32_t *t_fine)
{
    int32_t var1 =
        (((adc_T >> 3) - ((int32_t)mTask.comp.dig_T1 << 1))
            * (int32_t)mTask.comp.dig_T2) >> 11;

    int32_t tmp = (adc_T >> 4) - (int32_t)mTask.comp.dig_T1;

    int32_t var2 = (((tmp * tmp) >> 12) * (int32_t)mTask.comp.dig_T3) >> 14;

    int32_t sum = var1 + var2;

    *t_fine = sum;

    return (sum * 5 + 128) >> 8;
}

static float compensateBaro(int32_t t_fine, int32_t adc_P)
{
    float f = t_fine - 128000, fSqr = f * f;
    float a = 1048576 - adc_P;
    float v1, v2, p, pSqr;

    v2 = fSqr * mTask.comp.dig_P6 + f * mTask.comp.dig_P5 * (float)(1ULL << 17) + mTask.comp.dig_P4 * (float)(1ULL << 35);
    v1 = fSqr * mTask.comp.dig_P1 * mTask.comp.dig_P3 * (1.0f/(1ULL << 41)) + f * mTask.comp.dig_P1 * mTask.comp.dig_P2 * (1.0f/(1ULL << 21)) + mTask.comp.dig_P1 * (float)(1ULL << 14);

    p = (a * (float)(1ULL << 31) - v2) * 3125 / v1;
    pSqr = p * p;

    return pSqr * mTask.comp.dig_P9 * (1.0f/(1ULL << 59)) + p * (mTask.comp.dig_P8 * (1.0f/(1ULL << 19)) + 1) * (1.0f/(1ULL << 8)) + 16.0f * mTask.comp.dig_P7;
}

static void getTempAndBaro(const uint8_t *tmp, float *pressure_Pa, float *temp_centigrade)
{
    int32_t pres_adc = ((int32_t)tmp[0] << 12) | ((int32_t)tmp[1] << 4) | (tmp[2] >> 4);
    int32_t temp_adc = ((int32_t)tmp[3] << 12) | ((int32_t)tmp[4] << 4) | (tmp[5] >> 4);

    int32_t T_fine;
    int32_t temp = compensateTemp(temp_adc, &T_fine);
    float pres = compensateBaro(T_fine, pres_adc);

    *temp_centigrade = (float)temp * 0.01f;
    *pressure_Pa = pres * (1.0f / 256.0f) + mTask.offset;
}

static void handleI2cEvent(enum BMP280TaskState state)
{
    union EmbeddedDataPoint sample;

    switch (state) {
        case STATE_RESET: {
            mTask.txrxBuf[0] = BOSCH_BMP280_REG_ID;
            i2cMasterTxRx(I2C_BUS_ID, I2C_ADDR, mTask.txrxBuf, 1,
                            mTask.txrxBuf, 1, &i2cCallback,
                            (void*)STATE_VERIFY_ID);
            break;
        }

        case STATE_VERIFY_ID: {
            /* Check the sensor ID */
            if (mTask.txrxBuf[0] != BOSCH_BMP280_ID) {
                osLog(LOG_INFO, "BMP280: not detected\n");
                break;
            }

            /* Get compensation parameters */
            mTask.txrxBuf[0] = BOSCH_BMP280_REG_DIG_T1;
            i2cMasterTxRx(I2C_BUS_ID, I2C_ADDR, mTask.txrxBuf, 1,
                            (uint8_t*)&mTask.comp, 24, &i2cCallback,
                            (void*)STATE_AWAITING_COMP_PARAMS);

            break;
        }

        case STATE_AWAITING_COMP_PARAMS: {
            mTask.txrxBuf[0] = BOSCH_BMP280_REG_CTRL_MEAS;
            mTask.txrxBuf[1] = CTRL_SLEEP;
            i2cMasterTx(I2C_BUS_ID, I2C_ADDR, mTask.txrxBuf, 2,
                              &i2cCallback, (void*)STATE_CONFIG);
            break;
        }

        case STATE_CONFIG: {
            mTask.txrxBuf[0] = BOSCH_BMP280_REG_CONFIG;
            // standby time: 62.5ms, IIR filter coefficient: 4
            mTask.txrxBuf[1] = (1 << 5) | (2 << 2);
            i2cMasterTx(I2C_BUS_ID, I2C_ADDR, mTask.txrxBuf, 2,
                              &i2cCallback, (void*)STATE_FINISH_INIT);
        }

        case STATE_ENABLING_BARO: {
            sensorSignalInternalEvt(mTask.baroHandle, SENSOR_INTERNAL_EVT_POWER_STATE_CHG, true, 0);
            break;
        }

        case STATE_ENABLING_TEMP: {
            sensorSignalInternalEvt(mTask.tempHandle, SENSOR_INTERNAL_EVT_POWER_STATE_CHG, true, 0);
            break;
        }

        case STATE_DISABLING_BARO: {
            sensorSignalInternalEvt(mTask.baroHandle, SENSOR_INTERNAL_EVT_POWER_STATE_CHG, false, 0);
            break;
        }

        case STATE_DISABLING_TEMP: {
            sensorSignalInternalEvt(mTask.tempHandle, SENSOR_INTERNAL_EVT_POWER_STATE_CHG, false, 0);
            break;
        }

        case STATE_FINISH_INIT: {
            sensorRegisterInitComplete(mTask.baroHandle);
            sensorRegisterInitComplete(mTask.tempHandle);
            osLog(LOG_INFO, "BMP280: idle\n");
            break;
        }

        case STATE_SAMPLING: {
            float pressure_Pa, temp_centigrade;
            getTempAndBaro(mTask.txrxBuf, &pressure_Pa, &temp_centigrade);

            if (mTask.baroOn && mTask.baroReading) {
                if (mTask.baroCalibrating) {
                    sendCalibrationResult(SENSOR_APP_EVT_STATUS_SUCCESS, pressure_Pa * 0.01f);

                    if (mTask.baroTimerHandle)
                        timTimerCancel(mTask.baroTimerHandle);

                    mTask.baroOn = false;
                    mTask.baroCalibrating = false;

                    mTask.txrxBuf[0] = BOSCH_BMP280_REG_CTRL_MEAS;
                    mTask.txrxBuf[1] = CTRL_SLEEP;
                    i2cMasterTx(I2C_BUS_ID, I2C_ADDR, mTask.txrxBuf, 2, &i2cCallback, (void*)STATE_IDLE);
                } else {
                    sample.fdata = pressure_Pa * 0.01f;
                    osEnqueueEvt(sensorGetMyEventType(SENS_TYPE_BARO), sample.vptr, NULL);
                }
            }

            if (mTask.tempOn && mTask.tempReading) {
                sample.fdata = temp_centigrade;
                osEnqueueEvt(sensorGetMyEventType(SENS_TYPE_TEMP), sample.vptr, NULL);
            }

            mTask.baroReading = false;
            mTask.tempReading = false;

            break;
        }

        default:
            break;
    }
}

static void handleEvent(uint32_t evtType, const void* evtData)
{
    switch (evtType) {
        case EVT_APP_START:
        {
            osEventUnsubscribe(mTask.id, EVT_APP_START);
            i2cMasterRequest(I2C_BUS_ID, I2C_SPEED);

            /* Reset chip */
            mTask.txrxBuf[0] = BOSCH_BMP280_REG_RESET;
            mTask.txrxBuf[1] = 0xB6;
            i2cMasterTx(I2C_BUS_ID, I2C_ADDR, mTask.txrxBuf, 2,
                        &i2cCallback, (void*)STATE_RESET);
            break;
        }

        case EVT_SENSOR_I2C:
        {
            handleI2cEvent((enum BMP280TaskState)evtData);
            break;
        }

        case EVT_SENSOR_BARO_TIMER:
        {
            /* Start sampling for a value */
            if (!mTask.baroReading && !mTask.tempReading) {
                mTask.txrxBuf[0] = BOSCH_BMP280_REG_PRES_MSB;
                i2cMasterTxRx(I2C_BUS_ID, I2C_ADDR, mTask.txrxBuf, 1,
                              mTask.txrxBuf, 6, &i2cCallback,
                              (void*)STATE_SAMPLING);
            }

            mTask.baroReading = true;
            break;
        }

        case EVT_SENSOR_TEMP_TIMER:
        {
            /* Start sampling for a value */
            if (!mTask.baroReading && !mTask.tempReading) {
                mTask.txrxBuf[0] = BOSCH_BMP280_REG_PRES_MSB;
                i2cMasterTxRx(I2C_BUS_ID, I2C_ADDR, mTask.txrxBuf, 1,
                              mTask.txrxBuf, 6, &i2cCallback,
                              (void*)STATE_SAMPLING);

            }

            mTask.tempReading = true;
            break;
        }
    }
}

static bool startTask(uint32_t taskId)
{
    osLog(LOG_INFO, "BMP280: task starting\n");

    mTask.id = taskId;
    mTask.offset = 0.0f;

    /* Register sensors */
    mTask.baroHandle = sensorRegister(&sensorInfoBaro, &sensorOpsBaro, NULL, false);
    mTask.tempHandle = sensorRegister(&sensorInfoTemp, &sensorOpsTemp, NULL, false);

    osEventSubscribe(taskId, EVT_APP_START);

    return true;
}

static void endTask(void)
{

}

INTERNAL_APP_INIT(BMP280_APP_ID, 0, startTask, endTask, handleEvent);
