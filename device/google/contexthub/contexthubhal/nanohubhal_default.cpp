/*
 * Copyright (c) 2016, Google. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_TAG "NanohubHAL"
#include <hardware/context_hub.h>
#include "nanohub_perdevice.h"
#include "nanohubhal.h"
#include <utils/Log.h>

namespace android {

namespace nanohub {

#define DEVICE "Default"
#define DEVICE_TAG (DEVICE[0])

static const connected_sensor_t mSensors[] = {
    {
        .sensor_id = ((int)DEVICE_TAG << 8) + 1,
        .physical_sensor = {
            .name = "i'll get to this later",
        },
    },
    {
        .sensor_id = ((int)DEVICE_TAG << 8) + 2,
        .physical_sensor = {
            .name = "i'll get to this later as well",
        },
    },
};

static const context_hub_t mHub = {
    .name = "Google System Nanohub on " DEVICE,
    .vendor = "Google/StMicro",
    .toolchain = "gcc-arm-none-eabi",
    .platform_version = 1,
    .toolchain_version = 0x04080000, //4.8
    .hub_id = 0,

    .peak_mips = 16,
    .stopped_power_draw_mw = 0.010 * 1.800,
    .sleep_power_draw_mw   = 0.080 * 1.800,
    .peak_power_draw_mw    = 3.000 * 1.800,

    .connected_sensors = mSensors,
    .num_connected_sensors = sizeof(mSensors) / sizeof(*mSensors),

    .max_supported_msg_len = MAX_RX_PACKET,
    .os_app_name = { .id = 0 },
};

const char *get_devnode_path(void)
{
    return "/dev/nanohub_comms";
}

const context_hub_t* get_hub_info(void)
{
    return &mHub;
}

}; // namespace nanohub

}; // namespace android
