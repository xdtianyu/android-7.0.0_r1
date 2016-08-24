/*
 * Copyright (C) 2008 The Android Open Source Project
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
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/wireless.h>

#include <openssl/evp.h>
#include <openssl/sha.h>

#define LOG_TAG "SoftapController"
#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <cutils/log.h>
#include <netutils/ifc.h>
#include <private/android_filesystem_config.h>
#include "wifi.h"
#include "ResponseCode.h"

#include "SoftapController.h"

using android::base::StringPrintf;
using android::base::WriteStringToFile;

static const char HOSTAPD_CONF_FILE[]    = "/data/misc/wifi/hostapd.conf";
static const char HOSTAPD_BIN_FILE[]    = "/system/bin/hostapd";

SoftapController::SoftapController()
    : mPid(0) {}

SoftapController::~SoftapController() {
}

int SoftapController::startSoftap() {
    pid_t pid = 1;

    if (mPid) {
        ALOGE("SoftAP is already running");
        return ResponseCode::SoftapStatusResult;
    }

    if (ensure_entropy_file_exists() < 0) {
        ALOGE("Wi-Fi entropy file was not created");
    }

    if ((pid = fork()) < 0) {
        ALOGE("fork failed (%s)", strerror(errno));
        return ResponseCode::ServiceStartFailed;
    }

    if (!pid) {
        ensure_entropy_file_exists();
        if (execl(HOSTAPD_BIN_FILE, HOSTAPD_BIN_FILE,
                  "-e", WIFI_ENTROPY_FILE,
                  HOSTAPD_CONF_FILE, (char *) NULL)) {
            ALOGE("execl failed (%s)", strerror(errno));
        }
        ALOGE("SoftAP failed to start");
        return ResponseCode::ServiceStartFailed;
    } else {
        mPid = pid;
        ALOGD("SoftAP started successfully");
        usleep(AP_BSS_START_DELAY);
    }
    return ResponseCode::SoftapStatusResult;
}

int SoftapController::stopSoftap() {

    if (mPid == 0) {
        ALOGE("SoftAP is not running");
        return ResponseCode::SoftapStatusResult;
    }

    ALOGD("Stopping the SoftAP service...");
    kill(mPid, SIGTERM);
    waitpid(mPid, NULL, 0);

    mPid = 0;
    ALOGD("SoftAP stopped successfully");
    usleep(AP_BSS_STOP_DELAY);
    return ResponseCode::SoftapStatusResult;
}

bool SoftapController::isSoftapStarted() {
    return (mPid != 0);
}

/*
 * Arguments:
 *  argv[2] - wlan interface
 *  argv[3] - SSID
 *  argv[4] - Broadcast/Hidden
 *  argv[5] - Channel
 *  argv[6] - Security
 *  argv[7] - Key
 */
int SoftapController::setSoftap(int argc, char *argv[]) {
    int hidden = 0;
    int channel = AP_CHANNEL_DEFAULT;

    if (argc < 5) {
        ALOGE("Softap set is missing arguments. Please use:");
        ALOGE("softap <wlan iface> <SSID> <hidden/broadcast> <channel> <wpa2?-psk|open> <passphrase>");
        return ResponseCode::CommandSyntaxError;
    }

    if (!strcasecmp(argv[4], "hidden"))
        hidden = 1;

    if (argc >= 5) {
        channel = atoi(argv[5]);
        if (channel <= 0)
            channel = AP_CHANNEL_DEFAULT;
    }

    std::string wbuf(StringPrintf("interface=%s\n"
            "driver=nl80211\n"
            "ctrl_interface=/data/misc/wifi/hostapd\n"
            "ssid=%s\n"
            "channel=%d\n"
            "ieee80211n=1\n"
            "hw_mode=%c\n"
            "ignore_broadcast_ssid=%d\n"
            "wowlan_triggers=any\n",
            argv[2], argv[3], channel, (channel <= 14) ? 'g' : 'a', hidden));

    std::string fbuf;
    if (argc > 7) {
        char psk_str[2*SHA256_DIGEST_LENGTH+1];
        if (!strcmp(argv[6], "wpa-psk")) {
            if (!generatePsk(argv[3], argv[7], psk_str)) {
                return ResponseCode::OperationFailed;
            }
            fbuf = StringPrintf("%swpa=3\nwpa_pairwise=TKIP CCMP\nwpa_psk=%s\n", wbuf.c_str(), psk_str);
        } else if (!strcmp(argv[6], "wpa2-psk")) {
            if (!generatePsk(argv[3], argv[7], psk_str)) {
                return ResponseCode::OperationFailed;
            }
            fbuf = StringPrintf("%swpa=2\nrsn_pairwise=CCMP\nwpa_psk=%s\n", wbuf.c_str(), psk_str);
        } else if (!strcmp(argv[6], "open")) {
            fbuf = wbuf;
        }
    } else if (argc > 6) {
        if (!strcmp(argv[6], "open")) {
            fbuf = wbuf;
        }
    } else {
        fbuf = wbuf;
    }

    if (!WriteStringToFile(fbuf, HOSTAPD_CONF_FILE, 0660, AID_SYSTEM, AID_WIFI)) {
        ALOGE("Cannot write to \"%s\": %s", HOSTAPD_CONF_FILE, strerror(errno));
        return ResponseCode::OperationFailed;
    }
    return ResponseCode::SoftapStatusResult;
}

/*
 * Arguments:
 *	argv[2] - interface name
 *	argv[3] - AP or P2P or STA
 */
int SoftapController::fwReloadSoftap(int argc, char *argv[])
{
    char *fwpath = NULL;

    if (argc < 4) {
        ALOGE("SoftAP fwreload is missing arguments. Please use: softap <wlan iface> <AP|P2P|STA>");
        return ResponseCode::CommandSyntaxError;
    }

    if (strcmp(argv[3], "AP") == 0) {
        fwpath = (char *)wifi_get_fw_path(WIFI_GET_FW_PATH_AP);
    } else if (strcmp(argv[3], "P2P") == 0) {
        fwpath = (char *)wifi_get_fw_path(WIFI_GET_FW_PATH_P2P);
    } else if (strcmp(argv[3], "STA") == 0) {
        fwpath = (char *)wifi_get_fw_path(WIFI_GET_FW_PATH_STA);
    } else {
        return ResponseCode::CommandParameterError;
    }
    if (!fwpath) {
        ALOGE("Softap fwReload - NULL path for %s", argv[3]);
        return ResponseCode::SoftapStatusResult;
    }
    if (wifi_change_fw_path((const char *)fwpath)) {
        ALOGE("Softap fwReload failed");
        return ResponseCode::OperationFailed;
    }
    else {
        ALOGD("Softap fwReload - Ok");
    }
    return ResponseCode::SoftapStatusResult;
}

bool SoftapController::generatePsk(char *ssid, char *passphrase, char *psk_str) {
    unsigned char psk[SHA256_DIGEST_LENGTH];

    // Use the PKCS#5 PBKDF2 with 4096 iterations
    if (PKCS5_PBKDF2_HMAC_SHA1(passphrase, strlen(passphrase),
                               reinterpret_cast<const unsigned char *>(ssid),
                               strlen(ssid), 4096, SHA256_DIGEST_LENGTH,
                               psk) != 1) {
        ALOGE("Cannot generate PSK using PKCS#5 PBKDF2");
        return false;
    }

    for (int j=0; j < SHA256_DIGEST_LENGTH; j++) {
        sprintf(&psk_str[j*2], "%02x", psk[j]);
    }

    return true;
}
