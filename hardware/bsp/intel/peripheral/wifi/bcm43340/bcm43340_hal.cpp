/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "wifi_hal_bcm43340"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <hardware_brillo/wifi_driver_hal.h>

const char kStationDeviceName[] = "wlan0";

static bool write_file(const char *filename, const char *content) {
    int fd = open(filename, O_WRONLY);
    if (fd < 0) {
        ALOGE("Cannot open %s for writing", filename);
        return false;
    }

    ssize_t write_count = strlen(content);
    ssize_t actual_count = write(fd, content, write_count);
    close(fd);
    
    if (actual_count != write_count) {
        ALOGE("Expected to write %d bytes to %s but write returns %d",
            write_count, filename, actual_count);
        return false;
    }
    return true;
}

/* Our HAL needs to set the AP/Station mode prior to actually initializing
 * the wifi. We use a dummy function for the initialize.
 */
static wifi_driver_error wifi_driver_initialize_bcm43340(void) {
    return WIFI_SUCCESS;
}

static wifi_driver_error wifi_driver_initialize_bcm43340_internal(void) {
   struct ifreq req;
   int rc, socketfd;

   socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
   if (socketfd < 0) {
	ALOGE("%s: unable to open control socket\n", __func__);
	return WIFI_ERROR_UNKNOWN;
   }

   strcpy (req.ifr_name, kStationDeviceName);
   rc = ioctl(socketfd, SIOCGIFFLAGS, &req);
   if (rc < 0) {
	ALOGE("%s: unable to query interface wlan0\n", __func__);
	return WIFI_ERROR_UNKNOWN;
   }

   req.ifr_flags &= ~(IFF_UP|IFF_RUNNING);
   rc = ioctl(socketfd, SIOCSIFFLAGS, &req);
   if (rc < 0) {
	ALOGE("%s: unable to down interface wlan0\n", __func__);
	return WIFI_ERROR_UNKNOWN;
   }

   req.ifr_flags |= IFF_UP|IFF_RUNNING;
   rc = ioctl(socketfd, SIOCSIFFLAGS, &req);
   if (rc < 0) {
	ALOGE("%s: unable to up interface wlan0\n", __func__);
	return WIFI_ERROR_UNKNOWN;
   }

    return WIFI_SUCCESS;
}

static wifi_driver_error
wifi_driver_set_mode_bcm43340(
    wifi_driver_mode mode,
    char* wifi_device_name,
    size_t wifi_device_name_size) {

    const char *firmware_path = nullptr;

    switch (mode) {
    case WIFI_MODE_AP:
      firmware_path = WIFI_DRIVER_FW_PATH_AP;
      break;

    case WIFI_MODE_STATION:
      firmware_path = WIFI_DRIVER_FW_PATH_STA;
      break;

#ifdef WIFI_MODE_P2P
    case WIFI_MODE_P2P:
      firmware_path = WIFI_DRIVER_FW_PATH_P2P;
      break;
#endif

    default:
      ALOGE("Unkonwn WiFi driver mode %d", mode);
      return WIFI_ERROR_INVALID_ARGS;
    }

    if (true != write_file(WIFI_DRIVER_NVRAM_PATH_PARAM, WIFI_DRIVER_NVRAM_PATH))
	return WIFI_ERROR_UNKNOWN;
    if (true != write_file(WIFI_DRIVER_FW_PATH_PARAM, firmware_path))
	return WIFI_ERROR_UNKNOWN;

    strlcpy(wifi_device_name, kStationDeviceName, wifi_device_name_size);
    return wifi_driver_initialize_bcm43340_internal();
}


static int
close_bcm43340_driver(struct hw_device_t *device) {
    wifi_driver_device_t *dev = (wifi_driver_device_t *)device;
    if (dev)
        free(dev);
    return 0;
}

static int
open_bcm43340_driver(const struct hw_module_t* module, const char*, struct hw_device_t** device) {
    wifi_driver_device_t* dev = (wifi_driver_device_t *)calloc(1, sizeof(wifi_driver_device_t));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = WIFI_DRIVER_DEVICE_API_VERSION_0_1;

    // We're forced into this cast by the existing API.  This pattern is
    // common among users of the HAL.
    dev->common.module = (hw_module_t *)module;

    dev->common.close = close_bcm43340_driver;
    dev->wifi_driver_initialize = wifi_driver_initialize_bcm43340;
    dev->wifi_driver_set_mode = wifi_driver_set_mode_bcm43340;

    *device = &dev->common;

    return 0;
}

static struct hw_module_methods_t bcm43340_driver_module_methods = {
    open:		open_bcm43340_driver
};

hw_module_t HAL_MODULE_INFO_SYM = {
  tag: 			HARDWARE_MODULE_TAG,
  version_major:	1,
  version_minor:	0,
  id:			WIFI_DRIVER_HARDWARE_MODULE_ID,
  name:			"BCM43340 / Edison module",
  author:		"Intel",
  methods:		&bcm43340_driver_module_methods,
  dso:			NULL,
  reserved:		{0},
};
