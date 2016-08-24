/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "hubconnection.h"
#include "eventnums.h"
#include "sensType.h"

#define LOG_TAG "nanohub"
#include <utils/Log.h>
#include <utils/SystemClock.h>

#include "file.h"
#include "JSONObject.h"

#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>

#include <cutils/properties.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <media/stagefright/foundation/ADebug.h>
#include <sys/inotify.h>

#define APP_ID_GET_VENDOR(appid)       ((appid) >> 24)
#define APP_ID_MAKE(vendor, app)       ((((uint64_t)(vendor)) << 24) | ((app) & 0x00FFFFFF))
#define APP_ID_VENDOR_GOOGLE           0x476f6f676cULL // "Googl"
#define APP_ID_APP_BMI160              2

#define SENS_TYPE_TO_EVENT(_sensorType) (EVT_NO_FIRST_SENSOR_EVENT + (_sensorType))

#define NANOHUB_FILE_PATH       "/dev/nanohub"
#define NANOHUB_LOCK_DIR        "/data/system/nanohub_lock"
#define NANOHUB_LOCK_FILE       NANOHUB_LOCK_DIR "/lock"
#define MAG_BIAS_FILE_PATH      "/sys/class/power_supply/battery/compass_compensation"

#define NANOHUB_LOCK_DIR_PERMS  (S_IRUSR | S_IWUSR | S_IXUSR)

#define SENSOR_RATE_ONCHANGE    0xFFFFFF01UL
#define SENSOR_RATE_ONESHOT     0xFFFFFF02UL

#define MIN_MAG_SQ              (10.0f * 10.0f)
#define MAX_MAG_SQ              (80.0f * 80.0f)

#define ACCEL_RAW_KSCALE        (8.0f * 9.81f / 32768.0f)

#define OS_LOG_EVENT            0x474F4C41  // ascii: ALOG

#ifdef LID_STATE_REPORTING_ENABLED
const char LID_STATE_PROPERTY[] = "sensors.contexthub.lid_state";
const char LID_STATE_UNKNOWN[]  = "unknown";
const char LID_STATE_OPEN[]     = "open";
const char LID_STATE_CLOSED[]   = "closed";
#endif  // LID_STATE_REPORTING_ENABLED

static const uint32_t delta_time_encoded = 1;
static const uint32_t delta_time_shift_table[2] = {9, 0};

namespace android {

// static
Mutex HubConnection::sInstanceLock;

// static
HubConnection *HubConnection::sInstance = NULL;

HubConnection *HubConnection::getInstance()
{
    Mutex::Autolock autoLock(sInstanceLock);
    if (sInstance == NULL) {
        sInstance = new HubConnection;
    }
    return sInstance;
}

HubConnection::HubConnection()
    : Thread(false /* canCallJava */),
      mRing(10 *1024),
      mActivityCbCookie(NULL),
      mActivityCb(NULL),
      mStepCounterOffset(0ull),
      mLastStepCount(0ull)
{
    mMagBias[0] = mMagBias[1] = mMagBias[2] = 0.0f;
    mMagAccuracy = SENSOR_STATUS_UNRELIABLE;
    mMagAccuracyRestore = SENSOR_STATUS_UNRELIABLE;
    mGyroBias[0] = mGyroBias[1] = mGyroBias[2] = 0.0f;

    memset(&mSensorState, 0x00, sizeof(mSensorState));
    mFd = open(NANOHUB_FILE_PATH, O_RDWR);
    mPollFds[0].fd = mFd;
    mPollFds[0].events = POLLIN;
    mPollFds[0].revents = 0;
    mNumPollFds = 1;

    initNanohubLock();

#ifdef USB_MAG_BIAS_REPORTING_ENABLED
    mUsbMagBias = 0;
    mMagBiasPollIndex = -1;
    int magBiasFd = open(MAG_BIAS_FILE_PATH, O_RDONLY);
    if (magBiasFd < 0) {
        ALOGW("Mag bias file open failed: %s", strerror(errno));
    } else {
        mPollFds[mNumPollFds].fd = magBiasFd;
        mPollFds[mNumPollFds].events = 0;
        mPollFds[mNumPollFds].revents = 0;
        mMagBiasPollIndex = mNumPollFds;
        mNumPollFds++;
    }
#endif  // USB_MAG_BIAS_REPORTING_ENABLED

    mSensorState[COMMS_SENSOR_ACCEL].sensorType = SENS_TYPE_ACCEL;
    mSensorState[COMMS_SENSOR_GYRO].sensorType = SENS_TYPE_GYRO;
    mSensorState[COMMS_SENSOR_GYRO].alt = COMMS_SENSOR_GYRO_UNCALIBRATED;
    mSensorState[COMMS_SENSOR_GYRO_UNCALIBRATED].sensorType = SENS_TYPE_GYRO;
    mSensorState[COMMS_SENSOR_GYRO_UNCALIBRATED].alt = COMMS_SENSOR_GYRO;
    mSensorState[COMMS_SENSOR_MAG].sensorType = SENS_TYPE_MAG;
    mSensorState[COMMS_SENSOR_MAG].alt = COMMS_SENSOR_MAG_UNCALIBRATED;
    mSensorState[COMMS_SENSOR_MAG_UNCALIBRATED].sensorType = SENS_TYPE_MAG;
    mSensorState[COMMS_SENSOR_MAG_UNCALIBRATED].alt = COMMS_SENSOR_MAG;
    mSensorState[COMMS_SENSOR_LIGHT].sensorType = SENS_TYPE_ALS;
    mSensorState[COMMS_SENSOR_PROXIMITY].sensorType = SENS_TYPE_PROX;
    mSensorState[COMMS_SENSOR_PRESSURE].sensorType = SENS_TYPE_BARO;
    mSensorState[COMMS_SENSOR_TEMPERATURE].sensorType = SENS_TYPE_TEMP;
    mSensorState[COMMS_SENSOR_ORIENTATION].sensorType = SENS_TYPE_ORIENTATION;
    mSensorState[COMMS_SENSOR_WINDOW_ORIENTATION].sensorType = SENS_TYPE_WIN_ORIENTATION;
    mSensorState[COMMS_SENSOR_WINDOW_ORIENTATION].rate = SENSOR_RATE_ONCHANGE;
    mSensorState[COMMS_SENSOR_STEP_DETECTOR].sensorType = SENS_TYPE_STEP_DETECT;
    mSensorState[COMMS_SENSOR_STEP_DETECTOR].rate = SENSOR_RATE_ONCHANGE;
    mSensorState[COMMS_SENSOR_STEP_COUNTER].sensorType = SENS_TYPE_STEP_COUNT;
    mSensorState[COMMS_SENSOR_SIGNIFICANT_MOTION].sensorType = SENS_TYPE_SIG_MOTION;
    mSensorState[COMMS_SENSOR_SIGNIFICANT_MOTION].rate = SENSOR_RATE_ONESHOT;
    mSensorState[COMMS_SENSOR_GRAVITY].sensorType = SENS_TYPE_GRAVITY;
    mSensorState[COMMS_SENSOR_LINEAR_ACCEL].sensorType = SENS_TYPE_LINEAR_ACCEL;
    mSensorState[COMMS_SENSOR_ROTATION_VECTOR].sensorType = SENS_TYPE_ROTATION_VECTOR;
    mSensorState[COMMS_SENSOR_GEO_MAG].sensorType = SENS_TYPE_GEO_MAG_ROT_VEC;
    mSensorState[COMMS_SENSOR_GAME_ROTATION_VECTOR].sensorType = SENS_TYPE_GAME_ROT_VECTOR;
    mSensorState[COMMS_SENSOR_HALL].sensorType = SENS_TYPE_HALL;
    mSensorState[COMMS_SENSOR_HALL].rate = SENSOR_RATE_ONCHANGE;
    mSensorState[COMMS_SENSOR_SYNC].sensorType = SENS_TYPE_VSYNC;
    mSensorState[COMMS_SENSOR_SYNC].rate = SENSOR_RATE_ONCHANGE;
    mSensorState[COMMS_SENSOR_ACTIVITY].sensorType = SENS_TYPE_ACTIVITY;
    mSensorState[COMMS_SENSOR_ACTIVITY].rate = SENSOR_RATE_ONCHANGE;
    mSensorState[COMMS_SENSOR_TILT].sensorType = SENS_TYPE_TILT;
    mSensorState[COMMS_SENSOR_TILT].rate = SENSOR_RATE_ONCHANGE;
    mSensorState[COMMS_SENSOR_GESTURE].sensorType = SENS_TYPE_GESTURE;
    mSensorState[COMMS_SENSOR_GESTURE].rate = SENSOR_RATE_ONESHOT;
    mSensorState[COMMS_SENSOR_DOUBLE_TWIST].sensorType = SENS_TYPE_DOUBLE_TWIST;
    mSensorState[COMMS_SENSOR_DOUBLE_TWIST].rate = SENSOR_RATE_ONCHANGE;
    mSensorState[COMMS_SENSOR_DOUBLE_TAP].sensorType = SENS_TYPE_DOUBLE_TAP;
    mSensorState[COMMS_SENSOR_DOUBLE_TAP].rate = SENSOR_RATE_ONCHANGE;

#ifdef LID_STATE_REPORTING_ENABLED
    initializeUinputNode();

    // set initial lid state
    if (property_set(LID_STATE_PROPERTY, LID_STATE_UNKNOWN) < 0) {
        ALOGE("could not set lid_state property");
    }

    // enable hall sensor for folio
    if (mFd >= 0) {
        queueActivate(COMMS_SENSOR_HALL, true /* enable */);
    }
#endif  // LID_STATE_REPORTING_ENABLED
}

HubConnection::~HubConnection()
{
    close(mFd);
}

void HubConnection::onFirstRef()
{
    run("HubConnection", PRIORITY_URGENT_DISPLAY);
}

status_t HubConnection::initCheck() const
{
    return mFd < 0 ? UNKNOWN_ERROR : OK;
}

status_t HubConnection::getAliveCheck()
{
    return OK;
}

static sp<JSONObject> readSettings(File *file) {
    off64_t size = file->seekTo(0, SEEK_END);
    file->seekTo(0, SEEK_SET);

    sp<JSONObject> root;

    if (size > 0) {
        char *buf = (char *)malloc(size);
        CHECK_EQ(file->read(buf, size), (ssize_t)size);
        file->seekTo(0, SEEK_SET);

        sp<JSONCompound> in = JSONCompound::Parse(buf, size);
        free(buf);
        buf = NULL;

        if (in != NULL && in->isObject()) {
            root = (JSONObject *)in.get();
        }
    }

    if (root == NULL) {
        root = new JSONObject;
    }

    return root;
}

static bool getCalibrationInt32(
        const sp<JSONObject> &settings, const char *key, int32_t *out,
        size_t numArgs) {
    sp<JSONArray> array;
    if (!settings->getArray(key, &array)) {
        return false;
    } else {
        for (size_t i = 0; i < numArgs; i++) {
            if (!array->getInt32(i, &out[i])) {
                return false;
            }
        }
    }
    return true;
}

static bool getCalibrationFloat(
        const sp<JSONObject> &settings, const char *key, float out[3]) {
    sp<JSONArray> array;
    if (!settings->getArray(key, &array)) {
        return false;
    } else {
        for (size_t i = 0; i < 3; i++) {
            if (!array->getFloat(i, &out[i])) {
                return false;
            }
        }
    }
    return true;
}

static void loadSensorSettings(sp<JSONObject>* settings,
                               sp<JSONObject>* saved_settings) {
    File settings_file(CONTEXTHUB_SETTINGS_PATH, "r");
    File saved_settings_file(CONTEXTHUB_SAVED_SETTINGS_PATH, "r");

    status_t err;
    if ((err = settings_file.initCheck()) != OK) {
        ALOGE("settings file open failed: %d (%s)",
              err,
              strerror(-err));

        *settings = new JSONObject;
    } else {
        *settings = readSettings(&settings_file);
    }

    if ((err = saved_settings_file.initCheck()) != OK) {
        ALOGE("saved settings file open failed: %d (%s)",
              err,
              strerror(-err));
        *saved_settings = new JSONObject;
    } else {
        *saved_settings = readSettings(&saved_settings_file);
    }
}

void HubConnection::saveSensorSettings() const {
    File saved_settings_file(CONTEXTHUB_SAVED_SETTINGS_PATH, "w");

    status_t err;
    if ((err = saved_settings_file.initCheck()) != OK) {
        ALOGE("saved settings file open failed %d (%s)",
              err,
              strerror(-err));
        return;
    }

    // Build a settings object.
    sp<JSONArray> magArray = new JSONArray;
#ifdef USB_MAG_BIAS_REPORTING_ENABLED
    magArray->addFloat(mMagBias[0] + mUsbMagBias);
#else
    magArray->addFloat(mMagBias[0]);
#endif  // USB_MAG_BIAS_REPORTING_ENABLED
    magArray->addFloat(mMagBias[1]);
    magArray->addFloat(mMagBias[2]);

    sp<JSONObject> settingsObject = new JSONObject;
    settingsObject->setArray("mag", magArray);

    // Write the JSON string to disk.
    AString serializedSettings = settingsObject->toString();
    size_t size = serializedSettings.size();
    if ((err = saved_settings_file.write(serializedSettings.c_str(), size)) != (ssize_t)size) {
        ALOGE("saved settings file write failed %d (%s)",
              err,
              strerror(-err));
    }
}

sensors_event_t *HubConnection::initEv(sensors_event_t *ev, uint64_t timestamp, uint32_t type, uint32_t sensor)
{
    memset(ev, 0x00, sizeof(sensors_event_t));
    ev->version = sizeof(sensors_event_t);
    ev->timestamp = timestamp;
    ev->type = type;
    ev->sensor = sensor;

    return ev;
}

void HubConnection::processSample(uint64_t timestamp, uint32_t type, uint32_t sensor, struct OneAxisSample *sample, __attribute__((unused)) bool highAccuracy)
{
    sensors_event_t nev[1];
    int cnt = 0;

    switch (sensor) {
    case COMMS_SENSOR_ACTIVITY:
        if (mActivityCb != NULL) {
            (*mActivityCb)(mActivityCbCookie, timestamp / 1000ull,
                false, /* is_flush */
                (float)(sample->idata & 0x7), 0.0, 0.0);
        }
        break;
    case COMMS_SENSOR_PRESSURE:
        initEv(&nev[cnt++], timestamp, type, sensor)->pressure = sample->fdata;
        break;
    case COMMS_SENSOR_TEMPERATURE:
        initEv(&nev[cnt++], timestamp, type, sensor)->temperature = sample->fdata;
        break;
    case COMMS_SENSOR_PROXIMITY:
        initEv(&nev[cnt++], timestamp, type, sensor)->distance = sample->fdata;
        break;
    case COMMS_SENSOR_LIGHT:
        initEv(&nev[cnt++], timestamp, type, sensor)->light = sample->fdata;
        break;
    case COMMS_SENSOR_STEP_COUNTER:
        // We'll stash away the last step count in case we need to reset
        // the hub. This last step count would then become the new offset.
        mLastStepCount = mStepCounterOffset + sample->idata;
        initEv(&nev[cnt++], timestamp, type, sensor)->u64.step_counter = mLastStepCount;
        break;
    case COMMS_SENSOR_STEP_DETECTOR:
    case COMMS_SENSOR_SIGNIFICANT_MOTION:
    case COMMS_SENSOR_TILT:
    case COMMS_SENSOR_DOUBLE_TWIST:
        initEv(&nev[cnt++], timestamp, type, sensor)->data[0] = 1.0f;
        break;
    case COMMS_SENSOR_GESTURE:
    case COMMS_SENSOR_SYNC:
        initEv(&nev[cnt++], timestamp, type, sensor)->data[0] = sample->idata;
        break;
    case COMMS_SENSOR_HALL:
#ifdef LID_STATE_REPORTING_ENABLED
        sendFolioEvent(sample->idata);
#endif  // LID_STATE_REPORTING_ENABLED
        break;
    case COMMS_SENSOR_WINDOW_ORIENTATION:
        initEv(&nev[cnt++], timestamp, type, sensor)->data[0] = sample->idata;
        break;
    default:
        break;
    }

    if (cnt > 0)
        mRing.write(nev, cnt);
}

void HubConnection::magAccuracyUpdate(float x, float y, float z)
{
    float magSq = x * x + y * y + z * z;

    if (magSq < MIN_MAG_SQ || magSq > MAX_MAG_SQ) {
        // save last good accuracy (either MEDIUM or HIGH)
        if (mMagAccuracy != SENSOR_STATUS_UNRELIABLE)
            mMagAccuracyRestore = mMagAccuracy;
        mMagAccuracy = SENSOR_STATUS_UNRELIABLE;
    } else if (mMagAccuracy == SENSOR_STATUS_UNRELIABLE) {
        // restore
        mMagAccuracy = mMagAccuracyRestore;
    }
}

void HubConnection::processSample(uint64_t timestamp, uint32_t type, uint32_t sensor, struct RawThreeAxisSample *sample, __attribute__((unused)) bool highAccuracy)
{
    sensors_vec_t *sv;
    sensors_event_t nev[2];
    int cnt = 0;

    switch (sensor) {
    case COMMS_SENSOR_ACCEL:
        sv = &initEv(&nev[cnt++], timestamp, type, sensor)->acceleration;
        sv->x = sample->ix * ACCEL_RAW_KSCALE;
        sv->y = sample->iy * ACCEL_RAW_KSCALE;
        sv->z = sample->iz * ACCEL_RAW_KSCALE;
        sv->status = SENSOR_STATUS_ACCURACY_HIGH;
        break;
    default:
        break;
    }

    if (cnt > 0)
        mRing.write(nev, cnt);
}

void HubConnection::processSample(uint64_t timestamp, uint32_t type, uint32_t sensor, struct ThreeAxisSample *sample, bool highAccuracy)
{
    sensors_vec_t *sv;
    uncalibrated_event_t *ue;
    sensors_event_t *ev;
    sensors_event_t nev[2];
    static const float heading_accuracy = M_PI / 6.0f;
    float w;
    int cnt = 0;

    switch (sensor) {
    case COMMS_SENSOR_ACCEL:
        sv = &initEv(&nev[cnt++], timestamp, type, sensor)->acceleration;
        sv->x = sample->x;
        sv->y = sample->y;
        sv->z = sample->z;
        sv->status = SENSOR_STATUS_ACCURACY_HIGH;
        break;
    case COMMS_SENSOR_GYRO:
        if (mSensorState[sensor].enable) {
            sv = &initEv(&nev[cnt++], timestamp, type, sensor)->gyro;
            sv->x = sample->x;
            sv->y = sample->y;
            sv->z = sample->z;
            sv->status = SENSOR_STATUS_ACCURACY_HIGH;
        }

        if (mSensorState[COMMS_SENSOR_GYRO_UNCALIBRATED].enable) {
            ue = &initEv(&nev[cnt++], timestamp,
                SENSOR_TYPE_GYROSCOPE_UNCALIBRATED,
                COMMS_SENSOR_GYRO_UNCALIBRATED)->uncalibrated_gyro;
            ue->x_uncalib = sample->x + mGyroBias[0];
            ue->y_uncalib = sample->y + mGyroBias[1];
            ue->z_uncalib = sample->z + mGyroBias[2];
            ue->x_bias = mGyroBias[0];
            ue->y_bias = mGyroBias[1];
            ue->z_bias = mGyroBias[2];
        }
        break;
    case COMMS_SENSOR_GYRO_BIAS:
        mGyroBias[0] = sample->x;
        mGyroBias[1] = sample->y;
        mGyroBias[2] = sample->z;
        break;
    case COMMS_SENSOR_MAG:
        magAccuracyUpdate(sample->x, sample->y, sample->z);

        if (mSensorState[sensor].enable) {
            sv = &initEv(&nev[cnt++], timestamp, type, sensor)->magnetic;
            sv->x = sample->x;
            sv->y = sample->y;
            sv->z = sample->z;
            sv->status = mMagAccuracy;
        }

        if (mSensorState[COMMS_SENSOR_MAG_UNCALIBRATED].enable) {
            ue = &initEv(&nev[cnt++], timestamp,
                SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED,
                COMMS_SENSOR_MAG_UNCALIBRATED)->uncalibrated_magnetic;
            ue->x_uncalib = sample->x + mMagBias[0];
            ue->y_uncalib = sample->y + mMagBias[1];
            ue->z_uncalib = sample->z + mMagBias[2];
            ue->x_bias = mMagBias[0];
            ue->y_bias = mMagBias[1];
            ue->z_bias = mMagBias[2];
        }
        break;
    case COMMS_SENSOR_MAG_BIAS:
        mMagAccuracy = highAccuracy ? SENSOR_STATUS_ACCURACY_HIGH : SENSOR_STATUS_ACCURACY_MEDIUM;
        mMagBias[0] = sample->x;
        mMagBias[1] = sample->y;
        mMagBias[2] = sample->z;

        saveSensorSettings();
        break;
    case COMMS_SENSOR_ORIENTATION:
    case COMMS_SENSOR_LINEAR_ACCEL:
    case COMMS_SENSOR_GRAVITY:
        sv = &initEv(&nev[cnt++], timestamp, type, sensor)->orientation;
        sv->x = sample->x;
        sv->y = sample->y;
        sv->z = sample->z;
        sv->status = mMagAccuracy;
        break;
    case COMMS_SENSOR_DOUBLE_TAP:
        ev = initEv(&nev[cnt++], timestamp, type, sensor);
        ev->data[0] = sample->x;
        ev->data[1] = sample->y;
        ev->data[2] = sample->z;
        break;
    case COMMS_SENSOR_ROTATION_VECTOR:
        ev = initEv(&nev[cnt++], timestamp, type, sensor);
        w = sample->x * sample->x + sample->y * sample->y + sample->z * sample->z;
        if (w < 1.0f)
            w = sqrt(1.0f - w);
        else
            w = 0.0f;
        ev->data[0] = sample->x;
        ev->data[1] = sample->y;
        ev->data[2] = sample->z;
        ev->data[3] = w;
        ev->data[4] = (4 - mMagAccuracy) * heading_accuracy;
        break;
    case COMMS_SENSOR_GEO_MAG:
    case COMMS_SENSOR_GAME_ROTATION_VECTOR:
        ev = initEv(&nev[cnt++], timestamp, type, sensor);
        w = sample->x * sample->x + sample->y * sample->y + sample->z * sample->z;
        if (w < 1.0f)
            w = sqrt(1.0f - w);
        else
            w = 0.0f;
        ev->data[0] = sample->x;
        ev->data[1] = sample->y;
        ev->data[2] = sample->z;
        ev->data[3] = w;
        break;
    default:
        break;
    }

    if (cnt > 0)
        mRing.write(nev, cnt);
}

void HubConnection::discardInotifyEvent() {
    // Read & discard an inotify event. We only use the presence of an event as
    // a trigger to perform the file existence check (for simplicity)
    if (mInotifyPollIndex >= 0) {
        char buf[sizeof(struct inotify_event) + NAME_MAX + 1];
        int ret = ::read(mPollFds[mInotifyPollIndex].fd, buf, sizeof(buf));
        ALOGD("Discarded %d bytes of inotify data", ret);
    }
}

void HubConnection::waitOnNanohubLock() {
    if (mInotifyPollIndex < 0) {
        return;
    }
    struct pollfd *pfd = &mPollFds[mInotifyPollIndex];

    // While the lock file exists, poll on the inotify fd (with timeout)
    while (access(NANOHUB_LOCK_FILE, F_OK) == 0) {
        ALOGW("Nanohub is locked; blocking read thread");
        int ret = poll(pfd, 1, 5000);
        if ((ret > 0) && (pfd->revents & POLLIN)) {
            discardInotifyEvent();
        }
    }
}

void HubConnection::restoreSensorState()
{
    Mutex::Autolock autoLock(mLock);

    sendCalibrationOffsets();

    for (int i = 0; i < NUM_COMMS_SENSORS_PLUS_1; i++) {
        if (mSensorState[i].sensorType && mSensorState[i].enable) {
            struct ConfigCmd cmd;

            initConfigCmd(&cmd, i);

            ALOGI("restoring: sensor=%d, handle=%d, enable=%d, period=%" PRId64 ", latency=%" PRId64,
                  cmd.sensorType, i, mSensorState[i].enable, frequency_q10_to_period_ns(mSensorState[i].rate),
                  mSensorState[i].latency);

            int ret = TEMP_FAILURE_RETRY(write(mFd, &cmd, sizeof(cmd)));
            if (ret != sizeof(cmd)) {
                ALOGE("failed to send config command to restore sensor %d\n", cmd.sensorType);
            }

            cmd.cmd = CONFIG_CMD_FLUSH;

            for (int j = 0; j < mSensorState[i].flushCnt; j++) {
                int ret = TEMP_FAILURE_RETRY(write(mFd, &cmd, sizeof(cmd)));
                if (ret != sizeof(cmd)) {
                    ALOGE("failed to send flush command to sensor %d\n", cmd.sensorType);
                }
            }
        }
    }

    mStepCounterOffset = mLastStepCount;
}

void HubConnection::postOsLog(uint8_t *buf, ssize_t len)
{
    // if len is less than 6, it's either an invalid or an empty log message.
    if (len < 6)
        return;

    buf[len] = 0x00;
    switch (buf[4]) {
    case 'E':
        ALOGE("osLog: %s", &buf[5]);
        break;
    case 'W':
        ALOGW("osLog: %s", &buf[5]);
        break;
    case 'I':
        ALOGI("osLog: %s", &buf[5]);
        break;
    case 'D':
        ALOGD("osLog: %s", &buf[5]);
        break;
    default:
        break;
    }
}

ssize_t HubConnection::processBuf(uint8_t *buf, ssize_t len)
{
    struct nAxisEvent *data = (struct nAxisEvent *)buf;
    uint32_t type, sensor, bias, currSensor;
    int i, numSamples;
    bool one, rawThree, three;
    sensors_event_t ev;
    uint64_t timestamp;
    ssize_t ret = 0;

    if (len >= 4) {
        ret = sizeof(data->evtType);
        one = three = rawThree = false;
        bias = 0;
        switch (data->evtType) {
        case OS_LOG_EVENT:
            postOsLog(buf, len);
            return 0;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_ACCEL):
            type = SENSOR_TYPE_ACCELEROMETER;
            sensor = COMMS_SENSOR_ACCEL;
            three = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_ACCEL_RAW):
            type = SENSOR_TYPE_ACCELEROMETER;
            sensor = COMMS_SENSOR_ACCEL;
            rawThree = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_GYRO):
            type = SENSOR_TYPE_GYROSCOPE;
            sensor = COMMS_SENSOR_GYRO;
            bias = COMMS_SENSOR_GYRO_BIAS;
            three = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_MAG):
            type = SENSOR_TYPE_MAGNETIC_FIELD;
            sensor = COMMS_SENSOR_MAG;
            bias = COMMS_SENSOR_MAG_BIAS;
            three = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_ALS):
            type = SENSOR_TYPE_LIGHT;
            sensor = COMMS_SENSOR_LIGHT;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_PROX):
            type = SENSOR_TYPE_PROXIMITY;
            sensor = COMMS_SENSOR_PROXIMITY;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_BARO):
            type = SENSOR_TYPE_PRESSURE;
            sensor = COMMS_SENSOR_PRESSURE;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_TEMP):
            type = SENSOR_TYPE_AMBIENT_TEMPERATURE;
            sensor = COMMS_SENSOR_TEMPERATURE;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_ORIENTATION):
            type = SENSOR_TYPE_ORIENTATION;
            sensor = COMMS_SENSOR_ORIENTATION;
            three = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_WIN_ORIENTATION):
            type = SENSOR_TYPE_DEVICE_ORIENTATION;
            sensor = COMMS_SENSOR_WINDOW_ORIENTATION;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_STEP_DETECT):
            type = SENSOR_TYPE_STEP_DETECTOR;
            sensor = COMMS_SENSOR_STEP_DETECTOR;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_STEP_COUNT):
            type = SENSOR_TYPE_STEP_COUNTER;
            sensor = COMMS_SENSOR_STEP_COUNTER;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_SIG_MOTION):
            type = SENSOR_TYPE_SIGNIFICANT_MOTION;
            sensor = COMMS_SENSOR_SIGNIFICANT_MOTION;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_GRAVITY):
            type = SENSOR_TYPE_GRAVITY;
            sensor = COMMS_SENSOR_GRAVITY;
            three = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_LINEAR_ACCEL):
            type = SENSOR_TYPE_LINEAR_ACCELERATION;
            sensor = COMMS_SENSOR_LINEAR_ACCEL;
            three = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_ROTATION_VECTOR):
            type = SENSOR_TYPE_ROTATION_VECTOR;
            sensor = COMMS_SENSOR_ROTATION_VECTOR;
            three = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_GEO_MAG_ROT_VEC):
            type = SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR;
            sensor = COMMS_SENSOR_GEO_MAG;
            three = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_GAME_ROT_VECTOR):
            type = SENSOR_TYPE_GAME_ROTATION_VECTOR;
            sensor = COMMS_SENSOR_GAME_ROTATION_VECTOR;
            three = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_HALL):
            type = 0;
            sensor = COMMS_SENSOR_HALL;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_VSYNC):
            type = SENSOR_TYPE_SYNC;
            sensor = COMMS_SENSOR_SYNC;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_ACTIVITY):
            type = 0;
            sensor = COMMS_SENSOR_ACTIVITY;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_TILT):
            type = SENSOR_TYPE_TILT_DETECTOR;
            sensor = COMMS_SENSOR_TILT;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_GESTURE):
            type = SENSOR_TYPE_PICK_UP_GESTURE;
            sensor = COMMS_SENSOR_GESTURE;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_DOUBLE_TWIST):
            type = SENSOR_TYPE_DOUBLE_TWIST;
            sensor = COMMS_SENSOR_DOUBLE_TWIST;
            one = true;
            break;
        case SENS_TYPE_TO_EVENT(SENS_TYPE_DOUBLE_TAP):
            type = SENSOR_TYPE_DOUBLE_TAP;
            sensor = COMMS_SENSOR_DOUBLE_TAP;
            three = true;
            break;
        case EVT_RESET_REASON:
            uint32_t resetReason;
            memcpy(&resetReason, data->buffer, sizeof(resetReason));
            ALOGI("Observed hub reset: 0x%08" PRIx32, resetReason);
            restoreSensorState();
            return 0;
        default:
            return 0;
        }
    }

    if (len >= 16) {
        ret += sizeof(data->referenceTime);
        timestamp = data->referenceTime;
        numSamples = data->firstSample.numSamples;
        for (i=0; i<numSamples; i++) {
            if (data->firstSample.biasPresent && data->firstSample.biasSample == i)
                currSensor = bias;
            else
                currSensor = sensor;

            if (one) {
                if (i > 0)
                    timestamp += ((uint64_t)data->oneSamples[i].deltaTime) << delta_time_shift_table[data->oneSamples[i].deltaTime & delta_time_encoded];
                processSample(timestamp, type, currSensor, &data->oneSamples[i], data->firstSample.highAccuracy);
                ret += sizeof(data->oneSamples[i]);
            } else if (rawThree) {
                if (i > 0)
                    timestamp += ((uint64_t)data->rawThreeSamples[i].deltaTime) << delta_time_shift_table[data->rawThreeSamples[i].deltaTime & delta_time_encoded];
                processSample(timestamp, type, currSensor, &data->rawThreeSamples[i], data->firstSample.highAccuracy);
                ret += sizeof(data->rawThreeSamples[i]);
            } else if (three) {
                if (i > 0)
                    timestamp += ((uint64_t)data->threeSamples[i].deltaTime) << delta_time_shift_table[data->threeSamples[i].deltaTime & delta_time_encoded];
                processSample(timestamp, type, currSensor, &data->threeSamples[i], data->firstSample.highAccuracy);
                ret += sizeof(data->threeSamples[i]);
            }
        }

        if (!numSamples)
            ret += sizeof(data->firstSample);

        for (i=0; i<data->firstSample.numFlushes; i++) {
            if (sensor == COMMS_SENSOR_ACTIVITY) {
                if (mActivityCb != NULL) {
                    (*mActivityCb)(mActivityCbCookie, 0ull, /* when_us */
                        true, /* is_flush */
                        0.0f, 0.0f, 0.0f);
                }
            } else {
                memset(&ev, 0x00, sizeof(sensors_event_t));
                ev.version = META_DATA_VERSION;
                ev.timestamp = 0;
                ev.type = SENSOR_TYPE_META_DATA;
                ev.sensor = 0;
                ev.meta_data.what = META_DATA_FLUSH_COMPLETE;
                if (mSensorState[sensor].alt && mSensorState[mSensorState[sensor].alt].flushCnt > 0) {
                    mSensorState[mSensorState[sensor].alt].flushCnt --;
                    ev.meta_data.sensor = mSensorState[sensor].alt;
                } else {
                    mSensorState[sensor].flushCnt --;
                    ev.meta_data.sensor = sensor;
                }

                mRing.write(&ev, 1);
                ALOGI("flushing %d", ev.meta_data.sensor);
            }
        }
    }

    return ret;
}

void HubConnection::sendCalibrationOffsets()
{
    sp<JSONObject> settings;
    sp<JSONObject> saved_settings;
    int32_t accel[3], gyro[3], proximity, proximity_array[4];
    float barometer, mag[3], light;

    loadSensorSettings(&settings, &saved_settings);

    if (getCalibrationInt32(settings, "accel", accel, 3))
        queueDataInternal(COMMS_SENSOR_ACCEL, accel, sizeof(accel));

    if (getCalibrationInt32(settings, "gyro", gyro, 3))
        queueDataInternal(COMMS_SENSOR_GYRO, gyro, sizeof(gyro));

    if (settings->getFloat("barometer", &barometer))
        queueDataInternal(COMMS_SENSOR_PRESSURE, &barometer, sizeof(barometer));

    if (settings->getInt32("proximity", &proximity))
        queueDataInternal(COMMS_SENSOR_PROXIMITY, &proximity, sizeof(proximity));

    if (getCalibrationInt32(settings, "proximity", proximity_array, 4))
        queueDataInternal(COMMS_SENSOR_PROXIMITY, proximity_array, sizeof(proximity_array));

    if (settings->getFloat("light", &light))
        queueDataInternal(COMMS_SENSOR_LIGHT, &light, sizeof(light));

    if (getCalibrationFloat(saved_settings, "mag", mag))
        queueDataInternal(COMMS_SENSOR_MAG, mag, sizeof(mag));
}

bool HubConnection::threadLoop() {
    ALOGI("threadLoop: starting");

    if (mFd < 0) {
        ALOGE("threadLoop: exiting prematurely: nanohub is unavailable");
        return false;
    }
    waitOnNanohubLock();

    sendCalibrationOffsets();

    while (!Thread::exitPending()) {
        ssize_t ret;

        do {
            ret = poll(mPollFds, mNumPollFds, -1);
        } while (ret < 0 && errno == EINTR);

        if (mInotifyPollIndex >= 0 && mPollFds[mInotifyPollIndex].revents & POLLIN) {
            discardInotifyEvent();
            waitOnNanohubLock();
        }

#ifdef USB_MAG_BIAS_REPORTING_ENABLED
        if (mMagBiasPollIndex >= 0 && mPollFds[mMagBiasPollIndex].revents & POLLERR) {
            // Read from mag bias file
            char buf[16];
            lseek(mPollFds[mMagBiasPollIndex].fd, 0, SEEK_SET);
            ::read(mPollFds[mMagBiasPollIndex].fd, buf, 16);
            float bias = atof(buf);
            mUsbMagBias = bias;
            queueUsbMagBias();
        }
#endif // USB_MAG_BIAS_REPORTING_ENABLED

        if (mPollFds[0].revents & POLLIN) {
            uint8_t recv[256];
            ssize_t len = ::read(mFd, recv, sizeof(recv));

            for (ssize_t offset = 0; offset < len;) {
                ret = processBuf(recv + offset, len - offset);

                if (ret > 0)
                    offset += ret;
                else
                    break;
            }
        }
    }

    return false;
}

ssize_t HubConnection::read(sensors_event_t *ev, size_t size) {
    return mRing.read(ev, size);
}

void HubConnection::setActivityCallback(
        void *cookie,
        void (*cb)(void *, uint64_t time_ms, bool, float x, float y, float z))
{
    Mutex::Autolock autoLock(mLock);
    mActivityCbCookie = cookie;
    mActivityCb = cb;
}

void HubConnection::initConfigCmd(struct ConfigCmd *cmd, int handle)
{
    uint8_t alt = mSensorState[handle].alt;

    memset(cmd, 0x00, sizeof(*cmd));

    cmd->evtType = EVT_NO_SENSOR_CONFIG_EVENT;
    cmd->sensorType = mSensorState[handle].sensorType;

    if (alt && mSensorState[alt].enable && mSensorState[handle].enable) {
        cmd->cmd = CONFIG_CMD_ENABLE;
        if (mSensorState[alt].rate > mSensorState[handle].rate)
            cmd->rate = mSensorState[alt].rate;
        else
            cmd->rate = mSensorState[handle].rate;
        if (mSensorState[alt].latency < mSensorState[handle].latency)
            cmd->latency = mSensorState[alt].latency;
        else
            cmd->latency = mSensorState[handle].latency;
    } else if (alt && mSensorState[alt].enable) {
        cmd->cmd = mSensorState[alt].enable ? CONFIG_CMD_ENABLE : CONFIG_CMD_DISABLE;
        cmd->rate = mSensorState[alt].rate;
        cmd->latency = mSensorState[alt].latency;
    } else { /* !alt || !mSensorState[alt].enable */
        cmd->cmd = mSensorState[handle].enable ? CONFIG_CMD_ENABLE : CONFIG_CMD_DISABLE;
        cmd->rate = mSensorState[handle].rate;
        cmd->latency = mSensorState[handle].latency;
    }
}

void HubConnection::queueActivate(int handle, bool enable)
{
    struct ConfigCmd cmd;
    int ret;

    Mutex::Autolock autoLock(mLock);

    if (mSensorState[handle].sensorType) {
        mSensorState[handle].enable = enable;

        initConfigCmd(&cmd, handle);

        ret = TEMP_FAILURE_RETRY(write(mFd, &cmd, sizeof(cmd)));
        if (ret == sizeof(cmd))
            ALOGI("queueActivate: sensor=%d, handle=%d, enable=%d",
                    cmd.sensorType, handle, enable);
        else
            ALOGE("queueActivate: failed to send command: sensor=%d, handle=%d, enable=%d",
                    cmd.sensorType, handle, enable);
    } else {
        ALOGI("queueActivate: unhandled handle=%d, enable=%d", handle, enable);
    }
}

void HubConnection::queueSetDelay(int handle, nsecs_t sampling_period_ns)
{
    struct ConfigCmd cmd;
    int ret;

    Mutex::Autolock autoLock(mLock);

    if (mSensorState[handle].sensorType) {
        if (sampling_period_ns > 0 &&
                mSensorState[handle].rate != SENSOR_RATE_ONCHANGE &&
                mSensorState[handle].rate != SENSOR_RATE_ONESHOT) {
            mSensorState[handle].rate = period_ns_to_frequency_q10(sampling_period_ns);
        }

        initConfigCmd(&cmd, handle);

        ret = TEMP_FAILURE_RETRY(write(mFd, &cmd, sizeof(cmd)));
        if (ret == sizeof(cmd))
            ALOGI("queueSetDelay: sensor=%d, handle=%d, period=%" PRId64,
                    cmd.sensorType, handle, sampling_period_ns);
        else
            ALOGE("queueSetDelay: failed to send command: sensor=%d, handle=%d, period=%" PRId64,
                    cmd.sensorType, handle, sampling_period_ns);
    } else {
        ALOGI("queueSetDelay: unhandled handle=%d, period=%" PRId64, handle, sampling_period_ns);
    }
}

void HubConnection::queueBatch(
        int handle,
        __attribute__((unused)) int flags,
        nsecs_t sampling_period_ns,
        nsecs_t max_report_latency_ns)
{
    struct ConfigCmd cmd;
    int ret;

    Mutex::Autolock autoLock(mLock);

    if (mSensorState[handle].sensorType) {
        if (sampling_period_ns > 0 &&
                mSensorState[handle].rate != SENSOR_RATE_ONCHANGE &&
                mSensorState[handle].rate != SENSOR_RATE_ONESHOT) {
            mSensorState[handle].rate = period_ns_to_frequency_q10(sampling_period_ns);
        }
        mSensorState[handle].latency = max_report_latency_ns;

        initConfigCmd(&cmd, handle);

        ret = TEMP_FAILURE_RETRY(write(mFd, &cmd, sizeof(cmd)));
        if (ret == sizeof(cmd))
            ALOGI("queueBatch: sensor=%d, handle=%d, period=%" PRId64 ", latency=%" PRId64,
                    cmd.sensorType, handle, sampling_period_ns, max_report_latency_ns);
        else
            ALOGE("queueBatch: failed to send command: sensor=%d, handle=%d, period=%" PRId64 ", latency=%" PRId64,
                    cmd.sensorType, handle, sampling_period_ns, max_report_latency_ns);
    } else {
        ALOGI("queueBatch: unhandled handle=%d, period=%" PRId64 ", latency=%" PRId64,
                handle, sampling_period_ns, max_report_latency_ns);
    }
}

void HubConnection::queueFlush(int handle)
{
    struct ConfigCmd cmd;
    int ret;

    Mutex::Autolock autoLock(mLock);

    if (mSensorState[handle].sensorType) {
        mSensorState[handle].flushCnt++;

        initConfigCmd(&cmd, handle);
        cmd.cmd = CONFIG_CMD_FLUSH;

        ret = TEMP_FAILURE_RETRY(write(mFd, &cmd, sizeof(cmd)));
        if (ret == sizeof(cmd))
            ALOGI("queueFlush: sensor=%d, handle=%d",
                    cmd.sensorType, handle);
        else
            ALOGE("queueFlush: failed to send command: sensor=%d, handle=%d",
                    cmd.sensorType, handle);
    } else {
        ALOGI("queueFlush: unhandled handle=%d", handle);
    }
}

void HubConnection::queueDataInternal(int handle, void *data, size_t length)
{
    struct ConfigCmd *cmd = (struct ConfigCmd *)malloc(sizeof(struct ConfigCmd) + length);
    size_t ret;

    if (cmd && mSensorState[handle].sensorType) {
        initConfigCmd(cmd, handle);
        memcpy(cmd->data, data, length);
        cmd->cmd = CONFIG_CMD_CFG_DATA;

        ret = TEMP_FAILURE_RETRY(write(mFd, cmd, sizeof(*cmd) + length));
        if (ret == sizeof(*cmd) + length)
            ALOGI("queueData: sensor=%d, length=%zu",
                    cmd->sensorType, length);
        else
            ALOGE("queueData: failed to send command: sensor=%d, length=%zu",
                    cmd->sensorType, length);
        free(cmd);
    } else {
        ALOGI("queueData: unhandled handle=%d", handle);
    }
}

void HubConnection::queueData(int handle, void *data, size_t length)
{
    Mutex::Autolock autoLock(mLock);
    queueDataInternal(handle, data, length);
}

void HubConnection::initNanohubLock() {
    // Create the lock directory (if it doesn't already exist)
    if (mkdir(NANOHUB_LOCK_DIR, NANOHUB_LOCK_DIR_PERMS) < 0 && errno != EEXIST) {
        ALOGE("Couldn't create Nanohub lock directory: %s", strerror(errno));
        return;
    }

    mInotifyPollIndex = -1;
    int inotifyFd = inotify_init1(IN_NONBLOCK);
    if (inotifyFd < 0) {
        ALOGE("Couldn't initialize inotify: %s", strerror(errno));
    } else if (inotify_add_watch(inotifyFd, NANOHUB_LOCK_DIR, IN_CREATE | IN_DELETE) < 0) {
        ALOGE("Couldn't add inotify watch: %s", strerror(errno));
        close(inotifyFd);
    } else {
        mPollFds[mNumPollFds].fd = inotifyFd;
        mPollFds[mNumPollFds].events = POLLIN;
        mPollFds[mNumPollFds].revents = 0;
        mInotifyPollIndex = mNumPollFds;
        mNumPollFds++;
    }
}

#ifdef USB_MAG_BIAS_REPORTING_ENABLED
void HubConnection::queueUsbMagBias()
{
    struct MsgCmd *cmd = (struct MsgCmd *)malloc(sizeof(struct MsgCmd) + sizeof(float));
    size_t ret;

    if (cmd) {
        cmd->evtType = EVT_APP_FROM_HOST;
        cmd->msg.appId = APP_ID_MAKE(APP_ID_VENDOR_GOOGLE, APP_ID_APP_BMI160);
        cmd->msg.dataLen = sizeof(float);
        memcpy((float *)(cmd+1), &mUsbMagBias, sizeof(float));

        ret = TEMP_FAILURE_RETRY(write(mFd, cmd, sizeof(*cmd) + sizeof(float)));
        if (ret == sizeof(*cmd) + sizeof(float))
            ALOGI("queueUsbMagBias: bias=%f\n", mUsbMagBias);
        else
            ALOGE("queueUsbMagBias: failed to send command: bias=%f\n", mUsbMagBias);
        free(cmd);
    }
}
#endif  // USB_MAG_BIAS_REPORTING_ENABLED

#ifdef LID_STATE_REPORTING_ENABLED
status_t HubConnection::initializeUinputNode()
{
    int ret = 0;

    // Open uinput dev node
    mUinputFd = TEMP_FAILURE_RETRY(open("/dev/uinput", O_WRONLY | O_NONBLOCK));
    if (mUinputFd < 0) {
        ALOGE("could not open uinput node: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }

    // Enable SW_LID events
    ret  = TEMP_FAILURE_RETRY(ioctl(mUinputFd, UI_SET_EVBIT, EV_SW));
    ret |= TEMP_FAILURE_RETRY(ioctl(mUinputFd, UI_SET_EVBIT, EV_SYN));
    ret |= TEMP_FAILURE_RETRY(ioctl(mUinputFd, UI_SET_SWBIT, SW_LID));
    if (ret < 0) {
        ALOGE("could not send ioctl to uinput node: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }

    // Create uinput node for SW_LID
    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-folio");
    uidev.id.bustype = BUS_SPI;
    uidev.id.vendor  = 0;
    uidev.id.product = 0;
    uidev.id.version = 0;

    ret = TEMP_FAILURE_RETRY(write(mUinputFd, &uidev, sizeof(uidev)));
    if (ret < 0) {
        ALOGE("write to uinput node failed: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }

    ret = TEMP_FAILURE_RETRY(ioctl(mUinputFd, UI_DEV_CREATE));
    if (ret < 0) {
        ALOGE("could not send ioctl to uinput node: %s", strerror(errno));
        return UNKNOWN_ERROR;
    }

    return OK;
}

void HubConnection::sendFolioEvent(int32_t data) {
    ssize_t ret = 0;
    struct input_event ev;

    memset(&ev, 0, sizeof(ev));

    ev.type = EV_SW;
    ev.code = SW_LID;
    ev.value =  data;
    ret = TEMP_FAILURE_RETRY(write(mUinputFd, &ev, sizeof(ev)));
    if (ret < 0) {
        ALOGE("write to uinput node failed: %s", strerror(errno));
        return;
    }

    // Force flush with EV_SYN event
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value =  0;
    ret = TEMP_FAILURE_RETRY(write(mUinputFd, &ev, sizeof(ev)));
    if (ret < 0) {
        ALOGE("write to uinput node failed: %s", strerror(errno));
        return;
    }

    // Set lid state property
    if (property_set(LID_STATE_PROPERTY,
                     (data ? LID_STATE_CLOSED : LID_STATE_OPEN)) < 0) {
        ALOGE("could not set lid_state property");
    }
}
#endif  // LID_STATE_REPORTING_ENABLED

} // namespace android
