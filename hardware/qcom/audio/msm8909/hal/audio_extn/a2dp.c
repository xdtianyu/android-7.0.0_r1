/*
* Copyright (c) 2015, The Linux Foundation. All rights reserved.
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
#define LOG_TAG "split_a2dp"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0
#include <errno.h>
#include <cutils/log.h>

#include "audio_hw.h"
#include "platform.h"
#include "platform_api.h"
#include <stdlib.h>
#include <cutils/str_parms.h>
#include <hardware/audio.h>
#include <hardware/hardware.h>

#ifdef SPLIT_A2DP_ENABLED

struct a2dp_data{
    struct audio_stream_out *a2dp_stream;
    struct audio_hw_device *a2dp_device;
    bool a2dp_started;
    bool a2dp_suspended;
};

struct a2dp_data a2dp;

#define AUDIO_PARAMETER_A2DP_STARTED "A2dpStarted"

static int open_a2dp_output()
{
    hw_module_t *mod;
    int      format = AUDIO_FORMAT_PCM_16_BIT;
    int rc=0;
    uint32_t channels = AUDIO_CHANNEL_OUT_STEREO;
    uint32_t sampleRate = DEFAULT_OUTPUT_SAMPLING_RATE;
    struct audio_config config;

    ALOGV("open_a2dp_output");

    config.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
    config.channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    config.format = AUDIO_FORMAT_PCM_16_BIT;

    if (a2dp.a2dp_device == NULL){
        rc = hw_get_module_by_class(AUDIO_HARDWARE_MODULE_ID, (const char*)"a2dp",
                                    (const hw_module_t**)&mod);
        if (rc != 0) {
            ALOGE("Could not get a2dp hardware module");
            return rc;
        }
        ALOGV("Opening A2DP device HAL for the first time");
        rc = audio_hw_device_open(mod, &a2dp.a2dp_device);
        if (rc != 0) {
            ALOGE("couldn't open a2dp audio hw device");
            return rc;
        }
    }

    rc = a2dp.a2dp_device->open_output_stream(a2dp.a2dp_device, 0,AUDIO_DEVICE_OUT_BLUETOOTH_A2DP,
                                    (audio_output_flags_t)AUDIO_OUTPUT_FLAG_NONE, &config, &a2dp.a2dp_stream, NULL);

    if( rc != 0 ) {
        ALOGE("Failed to open output stream for a2dp: status %d", rc);
    }

    a2dp.a2dp_suspended = false;
    return rc;
}

static int close_a2dp_output()
{

    ALOGV("close_a2dp_output");
    if(!a2dp.a2dp_device && !a2dp.a2dp_stream){
        ALOGE("No Active A2dp output found");
        return 0;
    }

    a2dp.a2dp_device->close_output_stream(a2dp.a2dp_device, a2dp.a2dp_stream);
    a2dp.a2dp_stream = NULL;
    a2dp.a2dp_started = false;
    a2dp.a2dp_suspended = true;

    return 0;
}

void audio_extn_a2dp_set_parameters(struct str_parms *parms)
{
     int ret, val;
     char value[32]={0};

     ret = str_parms_get_str(parms, AUDIO_PARAMETER_DEVICE_CONNECT, value,
                            sizeof(value));
     if( ret >= 0) {
         val = atoi(value);
         if (val & AUDIO_DEVICE_OUT_ALL_A2DP) {
             ALOGV("Received device connect request for A2DP");
             open_a2dp_output();
         }
     }

     ret = str_parms_get_str(parms, AUDIO_PARAMETER_DEVICE_DISCONNECT, value,
                         sizeof(value));

     if( ret >= 0) {
         val = atoi(value);
         if (val & AUDIO_DEVICE_OUT_ALL_A2DP) {
             ALOGV("Received device dis- connect request");
             close_a2dp_output();
         }
     }

     ret = str_parms_get_str(parms, "A2dpSuspended", value, sizeof(value));
     if (ret >= 0) {
         if (a2dp.a2dp_device && a2dp.a2dp_stream) {
             a2dp.a2dp_device->set_parameters(a2dp.a2dp_device, str_parms_to_str(parms));
             if (!strncmp(value,"true",sizeof(value))) {
                 a2dp.a2dp_suspended = true;
             } else {
                 a2dp.a2dp_suspended = false;
             }
         }
     }
}

void audio_extn_a2dp_start_playback()
{
    int ret = 0;
    char buf[20]={0};

    if (!a2dp.a2dp_started && a2dp.a2dp_device && a2dp.a2dp_stream) {

         snprintf(buf,sizeof(buf),"%s=true",AUDIO_PARAMETER_A2DP_STARTED);
        /* This call indicates BT HAL to start playback */
        ret =  a2dp.a2dp_device->set_parameters(a2dp.a2dp_device, buf);
        if (ret < 0 ) {
           ALOGE("BT controller start failed, retry on the next write");
           a2dp.a2dp_started = false;
        } else {
           a2dp.a2dp_started = true;
           ALOGV("Start playback successful to BT HAL");
        }
    }
}

void audio_extn_a2dp_stop_playback()
{
    int ret =0;
    char buf[20]={0};

    if ( a2dp.a2dp_started && a2dp.a2dp_device && a2dp.a2dp_stream) {

       snprintf(buf,sizeof(buf),"%s=false",AUDIO_PARAMETER_A2DP_STARTED);

        ret = a2dp.a2dp_device->set_parameters(a2dp.a2dp_device, buf);

        if (ret < 0)
            ALOGE("out_standby to BT HAL failed");
        else
            ALOGV("out_standby to BT HAL successful");

    }
    a2dp.a2dp_started = false;
    a2dp.a2dp_suspended = true;
}

void audio_extn_a2dp_init ()
{
  a2dp.a2dp_started = false;
  a2dp.a2dp_suspended = true;
  a2dp.a2dp_stream = NULL;
  a2dp.a2dp_device = NULL;
}
#endif // SPLIT_A2DP_ENABLED
