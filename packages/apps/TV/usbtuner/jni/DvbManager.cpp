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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>

#define LOG_TAG "DvbManager"
#include "logging.h"

#include "DvbManager.h"

static double currentTimeMillis() {
    struct timeval tv;
    gettimeofday(&tv, (struct timezone *) NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

DvbManager::DvbManager(JNIEnv *env, jobject)
        : mFeFd(-1),
          mDemuxFd(-1),
          mDvrFd(-1),
          mPatFilterFd(-1),
          mFeHasLock(false) {
    jclass clazz = env->FindClass(
        "com/android/usbtuner/TunerHal");
    mOpenDvbFrontEndMethodID = env->GetMethodID(
        clazz, "openDvbFrontEndFd", "()I");
    mOpenDvbDemuxMethodID = env->GetMethodID(
        clazz, "openDvbDemuxFd", "()I");
    mOpenDvbDvrMethodID = env->GetMethodID(
        clazz, "openDvbDvrFd", "()I");
}

DvbManager::~DvbManager() {
    reset();
}

bool DvbManager::isFeLocked() {
    struct dvb_frontend_event kevent;
    memset(&kevent, 0, sizeof(kevent));
    if (ioctl(mFeFd, FE_READ_STATUS, &kevent.status) == 0) {
        return (kevent.status & FE_HAS_LOCK);
    }
    return false;
}

int DvbManager::tune(JNIEnv *env, jobject thiz,
        const int frequency, const char *modulationStr, int timeout_ms) {
    resetExceptFe();

    struct dvb_frontend_parameters feParams;
    memset(&feParams, 0, sizeof(struct dvb_frontend_parameters));
    feParams.frequency = frequency;
    if (strcmp(modulationStr, "8VSB") == 0) {
        feParams.u.vsb.modulation = VSB_8;
    } else if (strcmp(modulationStr, "QAM256") == 0) {
        feParams.u.vsb.modulation = QAM_256;
    } else {
        ALOGE("Unrecognized modulation mode : %s", modulationStr);
        return -1;
    }

    if (openDvbFe(env, thiz) != 0) {
        return -1;
    }

    feParams.inversion = INVERSION_AUTO;
    /* Check frontend capability */
    struct dvb_frontend_info feInfo;
    if (ioctl(mFeFd, FE_GET_INFO, &feInfo) != -1) {
        if (!(feInfo.caps & FE_CAN_INVERSION_AUTO)) {
            // FE can't do INVERSION_AUTO, trying INVERSION_OFF instead
            feParams.inversion = INVERSION_OFF;
        }
    }

    if (ioctl(mFeFd, FE_SET_FRONTEND, &feParams) != 0) {
        ALOGD("Can't set Frontend : %s", strerror(errno));
        return -1;
    }

    int lockSuccessCount = 0;
    double tuneClock = currentTimeMillis();
    while (currentTimeMillis() - tuneClock < timeout_ms) {
        usleep(FE_LOCK_CHECK_INTERNAL_US);

        bool lockStatus = isFeLocked();
        if (lockStatus) {
            lockSuccessCount++;
        } else {
            lockSuccessCount = 0;
        }
        ALOGI("Lock status : %s", lockStatus ? "true" : "false");
        if (lockSuccessCount >= FE_CONSECUTIVE_LOCK_SUCCESS_COUNT) {
            mFeHasLock = true;
            openDvbDvr(env, thiz);
            return 0;
        }
    }

    return -1;
}

int DvbManager::stopTune() {
    reset();
    usleep(DVB_TUNE_STOP_DELAY_MS);
    return 0;
}

int DvbManager::openDvbFeFromSystemApi(JNIEnv *env, jobject thiz) {
    int fd = (int) env->CallIntMethod(thiz, mOpenDvbFrontEndMethodID);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    return fd;
}

int DvbManager::openDvbDemuxFromSystemApi(JNIEnv *env, jobject thiz) {
    int fd = (int) env->CallIntMethod(thiz, mOpenDvbDemuxMethodID);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    return fd;
}

int DvbManager::openDvbDvrFromSystemApi(JNIEnv *env, jobject thiz) {
    int fd = (int) env->CallIntMethod(thiz, mOpenDvbDvrMethodID);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    return fd;
}

int DvbManager::openDvbFe(JNIEnv *env, jobject thiz) {
    if (mFeFd == -1) {
        if ((mFeFd = openDvbFeFromSystemApi(env, thiz)) < 0) {
            ALOGD("Can't open FE file : %s", strerror(errno));
            return -1;
        }
    }

    struct dvb_frontend_info info;
    if (ioctl(mFeFd, FE_GET_INFO, &info) == 0) {
        const char *types;
        switch (info.type) {
            case FE_QPSK:
                types = "DVB-S";
                break;
            case FE_QAM:
                types = "DVB-C";
                break;
            case FE_OFDM:
                types = "DVB-T";
                break;
            case FE_ATSC:
                types = "ATSC";
                break;
            default:
                types = "Unknown";
        }
        ALOGI("Using frontend \"%s\", type %s", info.name, types);
    }
    return 0;
}

int DvbManager::startTsPidFilter(JNIEnv *env, jobject thiz, int pid, int filterType) {
    Mutex::Autolock autoLock(mFilterLock);

    if (mPidFilters.find(pid) != mPidFilters.end() || (mPatFilterFd != -1 && pid == PAT_PID)) {
        return 0;
    }

    int demuxFd;
    if ((demuxFd = openDvbDemuxFromSystemApi(env, thiz)) < 0) {
        ALOGD("Can't open DEMUX file : %s", strerror(errno));
        return -1;
    }

    struct dmx_pes_filter_params filter;
    memset(&filter, 0, sizeof(filter));
    filter.pid = pid;
    filter.input = DMX_IN_FRONTEND;
    switch (filterType) {
        case FILTER_TYPE_AUDIO:
            filter.pes_type = DMX_PES_AUDIO;
            break;
        case FILTER_TYPE_VIDEO:
            filter.pes_type = DMX_PES_VIDEO;
            break;
        case FILTER_TYPE_PCR:
            filter.pes_type = DMX_PES_PCR;
            break;
        default:
            filter.pes_type = DMX_PES_OTHER;
            break;
    }
    filter.output = DMX_OUT_TS_TAP;
    filter.flags |= (DMX_CHECK_CRC | DMX_IMMEDIATE_START);

    // create a pes filter
    if (ioctl(demuxFd, DMX_SET_PES_FILTER, &filter)) {
        close(demuxFd);
        return -1;
    }

    if (pid != PAT_PID) {
        mPidFilters.insert(std::pair<int, int>(pid, demuxFd));
    } else {
        mPatFilterFd = demuxFd;
    }

    return 0;
}

void DvbManager::closeAllDvbPidFilter() {
    // Close all dvb pid filters except PAT filter to maintain the opening status of the device.
    Mutex::Autolock autoLock(mFilterLock);

    for (std::map<int, int>::iterator it(mPidFilters.begin());
                it != mPidFilters.end(); it++) {
        close(it->second);
    }
    mPidFilters.clear();
}

void DvbManager::closePatFilter() {
    Mutex::Autolock autoLock(mFilterLock);

    if (mPatFilterFd != -1) {
        close(mPatFilterFd);
        mPatFilterFd = -1;
    }
}

int DvbManager::openDvbDvr(JNIEnv *env, jobject thiz) {
    if ((mDvrFd = openDvbDvrFromSystemApi(env, thiz)) < 0) {
        ALOGD("Can't open DVR file : %s", strerror(errno));
        return -1;
    }
    return 0;
}

void DvbManager::closeDvbFe() {
    if (mFeFd != -1) {
        close(mFeFd);
        mFeFd = -1;
    }
}

void DvbManager::closeDvbDvr() {
    if (mDvrFd != -1) {
        close(mDvrFd);
        mDvrFd = -1;
    }
}

void DvbManager::reset() {
    mFeHasLock = false;
    closeDvbDvr();
    closeAllDvbPidFilter();
    closePatFilter();
    closeDvbFe();
}

void DvbManager::resetExceptFe() {
    mFeHasLock = false;
    closeDvbDvr();
    closeAllDvbPidFilter();
    closePatFilter();
}

int DvbManager::readTsStream(JNIEnv *env, jobject thiz,
        uint8_t *tsBuffer, int tsBufferSize, int timeout_ms) {
    if (!mFeHasLock) {
        usleep(DVB_ERROR_RETRY_INTERVAL_MS);
        return -1;
    }

    if (mDvrFd == -1) {
        openDvbDvr(env, thiz);
    }

    struct pollfd pollFd;
    pollFd.fd = mDvrFd;
    pollFd.events = POLLIN|POLLPRI|POLLERR;
    pollFd.revents = 0;
    int poll_result = poll(&pollFd, NUM_POLLFDS, timeout_ms);
    if (poll_result == 0) {
        return 0;
    } else if (poll_result == -1 || pollFd.revents & POLLERR) {
        ALOGD("Can't read DVR : %s", strerror(errno));
        // TODO: Find how to recover this situation correctly.
        closeDvbDvr();
        usleep(DVB_ERROR_RETRY_INTERVAL_MS);
        return -1;
    }
    return read(mDvrFd, tsBuffer, tsBufferSize);
}
