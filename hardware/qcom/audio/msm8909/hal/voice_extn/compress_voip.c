/*
 * Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 * Not a contribution.
 *
 * Copyright (C) 2013 The Android Open Source Project
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

#define LOG_TAG "compress_voip"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <math.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>

#include "audio_hw.h"
#include "platform_api.h"
#include "platform.h"
#include "voice_extn.h"

#define COMPRESS_VOIP_IO_BUF_SIZE_NB 320
#define COMPRESS_VOIP_IO_BUF_SIZE_WB 640

struct pcm_config pcm_config_voip_nb = {
    .channels = 1,
    .rate = 8000, /* changed when the stream is opened */
    .period_size = COMPRESS_VOIP_IO_BUF_SIZE_NB/2,
    .period_count = 10,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_voip_wb = {
    .channels = 1,
    .rate = 16000, /* changed when the stream is opened */
    .period_size = COMPRESS_VOIP_IO_BUF_SIZE_WB/2,
    .period_count = 10,
    .format = PCM_FORMAT_S16_LE,
};

struct voip_data {
    struct pcm *pcm_rx;
    struct pcm *pcm_tx;
    struct stream_out *out_stream;
    uint32_t out_stream_count;
    uint32_t in_stream_count;
    uint32_t sample_rate;
};

#define MODE_IS127              0x2
#define MODE_4GV_NB             0x3
#define MODE_4GV_WB             0x4
#define MODE_AMR                0x5
#define MODE_AMR_WB             0xD
#define MODE_PCM                0xC
#define MODE_4GV_NW             0xE

#define AUDIO_PARAMETER_KEY_VOIP_RATE               "voip_rate"
#define AUDIO_PARAMETER_KEY_VOIP_EVRC_RATE_MIN      "evrc_rate_min"
#define AUDIO_PARAMETER_KEY_VOIP_EVRC_RATE_MAX      "evrc_rate_max"
#define AUDIO_PARAMETER_KEY_VOIP_DTX_MODE           "dtx_on"
#define AUDIO_PARAMETER_VALUE_VOIP_TRUE             "true"
#define AUDIO_PARAMETER_KEY_VOIP_CHECK              "voip_flag"
#define AUDIO_PARAMETER_KEY_VOIP_OUT_STREAM_COUNT   "voip_out_stream_count"
#define AUDIO_PARAMETER_KEY_VOIP_SAMPLE_RATE        "voip_sample_rate"

static struct voip_data voip_data = {
  .pcm_rx = NULL,
  .pcm_tx = NULL,
  .out_stream = NULL,
  .out_stream_count = 0,
  .in_stream_count = 0,
  .sample_rate = 0
};

static int voip_set_volume(struct audio_device *adev, int volume);
static int voip_set_mic_mute(struct audio_device *adev, bool state);
static int voip_set_mode(struct audio_device *adev, int format);
static int voip_set_rate(struct audio_device *adev, int rate);
static int voip_set_evrc_min_max_rate(struct audio_device *adev, int min_rate,
                               int max_rate);
static int voip_set_dtx(struct audio_device *adev, bool enable);
static int voip_stop_call(struct audio_device *adev);
static int voip_start_call(struct audio_device *adev,
                           struct pcm_config *voip_config);

static int audio_format_to_voip_mode(int format)
{
    int mode;

    switch(format) {
        case AUDIO_FORMAT_PCM_16_BIT:
            mode = MODE_PCM;
            break;
        case AUDIO_FORMAT_AMR_NB:
            mode = MODE_AMR;
            break;
        case AUDIO_FORMAT_AMR_WB:
            mode = MODE_AMR_WB;
            break;
        case AUDIO_FORMAT_EVRC:
            mode = MODE_IS127;
            break;
        case AUDIO_FORMAT_EVRCB:
            mode = MODE_4GV_NB;
            break;
        case AUDIO_FORMAT_EVRCWB:
            mode = MODE_4GV_WB;
            break;
        case AUDIO_FORMAT_EVRCNW:
            mode = MODE_4GV_NW;
            break;
        default:
            mode = MODE_PCM;
    }
    return mode;
}

static int voip_set_volume(struct audio_device *adev, int volume)
{
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = "Voip Rx Gain";
    int vol_index = 0;
    uint32_t set_values[ ] = {0,
                              DEFAULT_VOLUME_RAMP_DURATION_MS};

    ALOGV("%s: enter", __func__);

    /* Voice volume levels are mapped to adsp volume levels as follows.
     * 100 -> 5, 80 -> 4, 60 -> 3, 40 -> 2, 20 -> 1  0 -> 0
     * But this values don't changed in kernel. So, below change is need.
     */
    vol_index = (int)percent_to_index(volume, MIN_VOL_INDEX, MAX_VOL_INDEX);
    set_values[0] = vol_index;

    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        return -EINVAL;
    }
    ALOGV("%s: Setting voip volume index: %d", __func__, set_values[0]);
    mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));

    ALOGV("%s: exit", __func__);
    return 0;
}

static int voip_set_mic_mute(struct audio_device *adev, bool state)
{
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = "Voip Tx Mute";
    uint32_t set_values[ ] = {0,
                              DEFAULT_VOLUME_RAMP_DURATION_MS};

    ALOGV("%s: enter, state=%d", __func__, state);

    if (adev->mode == AUDIO_MODE_IN_COMMUNICATION) {
        set_values[0] = state;
        ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
        if (!ctl) {
            ALOGE("%s: Could not get ctl for mixer cmd - %s",
                  __func__, mixer_ctl_name);
            return -EINVAL;
        }
        mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));
    }

    ALOGV("%s: exit", __func__);
    return 0;
}

static int voip_set_mode(struct audio_device *adev, int format)
{
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = "Voip Mode Config";
    uint32_t set_values[ ] = {0};
    int mode;

    ALOGD("%s: enter, format=%d", __func__, format);

    mode = audio_format_to_voip_mode(format);
    ALOGD("%s: Derived mode = %d", __func__, mode);

    set_values[0] = mode;
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
               __func__, mixer_ctl_name);
        return -EINVAL;
    }
    mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));

    ALOGV("%s: exit", __func__);
    return 0;
}

static int voip_set_rate(struct audio_device *adev, int rate)
{
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = "Voip Rate Config";
    uint32_t set_values[ ] = {0};

    ALOGD("%s: enter, rate=%d", __func__, rate);

    set_values[0] = rate;
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
               __func__, mixer_ctl_name);
        return -EINVAL;
    }
    mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));

    ALOGV("%s: exit", __func__);
    return 0;
}

static int voip_set_evrc_min_max_rate(struct audio_device *adev, int min_rate,
                               int max_rate)
{
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = "Voip Evrc Min Max Rate Config";
    uint32_t set_values[ ] = {0, 0};

    ALOGD("%s: enter, min_rate=%d, max_rate=%d",
          __func__, min_rate, max_rate);

    set_values[0] = min_rate;
    set_values[1] = max_rate;
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
               __func__, mixer_ctl_name);
        return -EINVAL;
    }
    mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));

    ALOGV("%s: exit", __func__);
    return 0;
}

static int voip_set_dtx(struct audio_device *adev, bool enable)
{
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = "Voip Dtx Mode";
    uint32_t set_values[ ] = {0};

    ALOGD("%s: enter, enable=%d", __func__, enable);

    set_values[0] = enable;
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
               __func__, mixer_ctl_name);
        return -EINVAL;
    }
    mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));

    ALOGV("%s: exit", __func__);
    return 0;
}

static int voip_stop_call(struct audio_device *adev)
{
    int i, ret = 0;
    struct audio_usecase *uc_info;

    ALOGD("%s: enter, out_stream_count=%d, in_stream_count=%d",
           __func__, voip_data.out_stream_count, voip_data.in_stream_count);

    if (!voip_data.out_stream_count && !voip_data.in_stream_count) {
        voip_data.sample_rate = 0;
        uc_info = get_usecase_from_list(adev, USECASE_COMPRESS_VOIP_CALL);
        if (uc_info == NULL) {
            ALOGE("%s: Could not find the usecase (%d) in the list",
                  __func__, USECASE_COMPRESS_VOIP_CALL);
            return -EINVAL;
        }

        /* 1. Close the PCM devices */
        if (voip_data.pcm_rx) {
            pcm_close(voip_data.pcm_rx);
            voip_data.pcm_rx = NULL;
        }
        if (voip_data.pcm_tx) {
            pcm_close(voip_data.pcm_tx);
            voip_data.pcm_tx = NULL;
        }

        /* 2. Get and set stream specific mixer controls */
        disable_audio_route(adev, uc_info);

        /* 3. Disable the rx and tx devices */
        disable_snd_device(adev, uc_info->out_snd_device);
        disable_snd_device(adev, uc_info->in_snd_device);

        list_remove(&uc_info->list);
        free(uc_info);
    } else
        ALOGV("%s: NO-OP because out_stream_count=%d, in_stream_count=%d",
               __func__, voip_data.out_stream_count, voip_data.in_stream_count);

    ALOGV("%s: exit: status(%d)", __func__, ret);
    return ret;
}

static int voip_start_call(struct audio_device *adev,
                           struct pcm_config *voip_config)
{
    int i, ret = 0;
    struct audio_usecase *uc_info;
    int pcm_dev_rx_id, pcm_dev_tx_id;

    ALOGD("%s: enter", __func__);

    uc_info = get_usecase_from_list(adev, USECASE_COMPRESS_VOIP_CALL);
    if (uc_info == NULL) {
        ALOGV("%s: voip usecase is added to the list", __func__);
        uc_info = (struct audio_usecase *)calloc(1, sizeof(struct audio_usecase));

        if (!uc_info) {
            ALOGE("failed to allocate voip usecase mem");
            return -ENOMEM;
        }

        uc_info->id = USECASE_COMPRESS_VOIP_CALL;
        uc_info->type = VOIP_CALL;
        if (voip_data.out_stream)
            uc_info->stream.out = voip_data.out_stream;
        else
            uc_info->stream.out = adev->primary_output;
        uc_info->in_snd_device = SND_DEVICE_NONE;
        uc_info->out_snd_device = SND_DEVICE_NONE;

        list_add_tail(&adev->usecase_list, &uc_info->list);

        select_devices(adev, USECASE_COMPRESS_VOIP_CALL);

        pcm_dev_rx_id = platform_get_pcm_device_id(uc_info->id, PCM_PLAYBACK);
        pcm_dev_tx_id = platform_get_pcm_device_id(uc_info->id, PCM_CAPTURE);

        if (pcm_dev_rx_id < 0 || pcm_dev_tx_id < 0) {
            ALOGE("%s: Invalid PCM devices (rx: %d tx: %d) for the usecase(%d)",
                  __func__, pcm_dev_rx_id, pcm_dev_tx_id, uc_info->id);
            ret = -EIO;
            goto error_start_voip;
        }

        ALOGD("%s: Opening PCM playback device card_id(%d) device_id(%d)",
              __func__, adev->snd_card, pcm_dev_rx_id);
        voip_data.pcm_rx = pcm_open(adev->snd_card,
                                    pcm_dev_rx_id,
                                    PCM_OUT, voip_config);
        if (voip_data.pcm_rx && !pcm_is_ready(voip_data.pcm_rx)) {
            ALOGE("%s: %s", __func__, pcm_get_error(voip_data.pcm_rx));
            pcm_close(voip_data.pcm_rx);
            voip_data.pcm_rx = NULL;
            ret = -EIO;
            goto error_start_voip;
        }

        ALOGD("%s: Opening PCM capture device card_id(%d) device_id(%d)",
              __func__, adev->snd_card, pcm_dev_tx_id);
        voip_data.pcm_tx = pcm_open(adev->snd_card,
                                    pcm_dev_tx_id,
                                    PCM_IN, voip_config);
        if (voip_data.pcm_tx && !pcm_is_ready(voip_data.pcm_tx)) {
            ALOGE("%s: %s", __func__, pcm_get_error(voip_data.pcm_tx));
            pcm_close(voip_data.pcm_rx);
            voip_data.pcm_tx = NULL;
            if (voip_data.pcm_rx) {
                pcm_close(voip_data.pcm_rx);
                voip_data.pcm_rx = NULL;
            }
            ret = -EIO;
            goto error_start_voip;
        }
        pcm_start(voip_data.pcm_rx);
        pcm_start(voip_data.pcm_tx);

        voice_extn_compress_voip_set_volume(adev, adev->voice.volume);

        if (ret < 0) {
            ALOGE("%s: error %d\n", __func__, ret);
            goto error_start_voip;
        }
    } else {
        ALOGV("%s: voip usecase is already enabled", __func__);
        if (voip_data.out_stream)
            uc_info->stream.out = voip_data.out_stream;
        else
            uc_info->stream.out = adev->primary_output;
        select_devices(adev, USECASE_COMPRESS_VOIP_CALL);
    }

    return 0;

error_start_voip:
    voip_stop_call(adev);

    ALOGV("%s: exit: status(%d)", __func__, ret);
    return ret;
}

int voice_extn_compress_voip_set_parameters(struct audio_device *adev,
                                             struct str_parms *parms)
{
    char *str;
    char value[32]={0};
    int ret = 0, err, rate;
    int min_rate, max_rate;
    bool flag;
    char *kv_pairs = str_parms_to_str(parms);

    ALOGV_IF(kv_pairs != NULL, "%s: enter: %s", __func__, kv_pairs);

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_VOIP_RATE,
                            value, sizeof(value));
    if (err >= 0) {
        rate = atoi(value);
        voip_set_rate(adev, rate);
        voip_set_evrc_min_max_rate(adev, rate, rate);
    }

    memset(value, 0, sizeof(value));
    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_VOIP_EVRC_RATE_MIN,
                            value, sizeof(value));
    if (err >= 0) {
        min_rate = atoi(value);
        str_parms_del(parms, AUDIO_PARAMETER_KEY_VOIP_EVRC_RATE_MIN);
        memset(value, 0, sizeof(value));
        err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_VOIP_EVRC_RATE_MAX,
                                value, sizeof(value));
        if (err >= 0) {
            max_rate = atoi(value);
            voip_set_evrc_min_max_rate(adev, min_rate, max_rate);
        } else {
            ALOGE("%s: AUDIO_PARAMETER_KEY_VOIP_EVRC_RATE_MAX not found", __func__);
            ret = -EINVAL;
            goto done;
        }
    }

    memset(value, 0, sizeof(value));
    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_VOIP_DTX_MODE,
                            value, sizeof(value));
    if (err >= 0) {
        flag = false;
        if (strcmp(value, AUDIO_PARAMETER_VALUE_VOIP_TRUE) == 0)
            flag = true;
        voip_set_dtx(adev, flag);
    }

done:
    ALOGV("%s: exit", __func__);
    free(kv_pairs);
    return ret;
}

void voice_extn_compress_voip_get_parameters(struct str_parms *query,
                                             struct str_parms *reply)
{
    int ret;
    char value[32]={0};
    char *str = NULL;

    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_VOIP_OUT_STREAM_COUNT,
                            value, sizeof(value));
    if (ret >= 0) {
        str_parms_add_int(reply, AUDIO_PARAMETER_KEY_VOIP_OUT_STREAM_COUNT,
                          voip_data.out_stream_count);
    }

    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_VOIP_SAMPLE_RATE,
                            value, sizeof(value));
    if (ret >= 0) {
        str_parms_add_int(reply, AUDIO_PARAMETER_KEY_VOIP_SAMPLE_RATE,
                          voip_data.sample_rate);
    }
}

void voice_extn_compress_voip_out_get_parameters(struct stream_out *out,
                                                 struct str_parms *query,
                                                 struct str_parms *reply)
{
    int ret, val;
    char value[32]={0};

    ALOGD("%s: enter", __func__);

    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_VOIP_CHECK, value, sizeof(value));

    if (ret >= 0) {
        if (out->usecase == USECASE_COMPRESS_VOIP_CALL)
            str_parms_add_int(reply, AUDIO_PARAMETER_KEY_VOIP_CHECK, true);
        else
            str_parms_add_int(reply, AUDIO_PARAMETER_KEY_VOIP_CHECK, false);
    }

    ALOGV("%s: exit", __func__);
}

void voice_extn_compress_voip_in_get_parameters(struct stream_in *in,
                                                struct str_parms *query,
                                                struct str_parms *reply)
{
    int ret, val;
    char value[32]={0};
    char *kv_pairs = NULL;

    ALOGV("%s: enter", __func__);

    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_VOIP_CHECK, value, sizeof(value));

    if (ret >= 0) {
        if (in->usecase == USECASE_COMPRESS_VOIP_CALL)
            str_parms_add_int(reply, AUDIO_PARAMETER_KEY_VOIP_CHECK, true);
        else
            str_parms_add_int(reply, AUDIO_PARAMETER_KEY_VOIP_CHECK, false);
    }

    kv_pairs = str_parms_to_str(reply);
    ALOGD_IF(kv_pairs != NULL, "%s: exit: return - %s", __func__, kv_pairs);
    free(kv_pairs);
}

int voice_extn_compress_voip_out_get_buffer_size(struct stream_out *out)
{
    if (out->config.rate == 16000)
        return COMPRESS_VOIP_IO_BUF_SIZE_WB;
    else
        return COMPRESS_VOIP_IO_BUF_SIZE_NB;
}

int voice_extn_compress_voip_in_get_buffer_size(struct stream_in *in)
{
    if (in->config.rate == 16000)
        return COMPRESS_VOIP_IO_BUF_SIZE_WB;
    else
        return COMPRESS_VOIP_IO_BUF_SIZE_NB;
}

int voice_extn_compress_voip_start_output_stream(struct stream_out *out)
{
    int ret = 0;
    struct audio_device *adev = out->dev;
    struct audio_usecase *uc_info;
    int snd_card_status = get_snd_card_state(adev);

    ALOGD("%s: enter", __func__);

    if (SND_CARD_STATE_OFFLINE == snd_card_status) {
        ret = -ENETRESET;
        ALOGE("%s: sound card is not active/SSR returning error %d ", __func__, ret);
        goto error;
    }

    if (!voip_data.out_stream_count)
        ret = voice_extn_compress_voip_open_output_stream(out);

    ret = voip_start_call(adev, &out->config);
    out->pcm = voip_data.pcm_rx;
    uc_info = get_usecase_from_list(adev, USECASE_COMPRESS_VOIP_CALL);
    if (uc_info) {
        uc_info->stream.out = out;
        uc_info->devices = out->devices;
    } else {
        ret = -EINVAL;
        ALOGE("%s: exit(%d): failed to get use case info", __func__, ret);
        goto error;
    }

error:
    ALOGV("%s: exit: status(%d)", __func__, ret);
    return ret;
}

int voice_extn_compress_voip_start_input_stream(struct stream_in *in)
{
    int ret = 0;
    struct audio_usecase *uc_info;
    struct audio_device *adev = in->dev;
    int snd_card_status = get_snd_card_state(adev);

    ALOGD("%s: enter", __func__);

    if (SND_CARD_STATE_OFFLINE == snd_card_status) {
        ret = -ENETRESET;
        ALOGE("%s: sound card is not active/SSR returning error %d ", __func__, ret);
        goto error;
    }

    if (!voip_data.in_stream_count)
        ret = voice_extn_compress_voip_open_input_stream(in);

    adev->active_input = in;
    ret = voip_start_call(adev, &in->config);
    in->pcm = voip_data.pcm_tx;

error:
    ALOGV("%s: exit: status(%d)", __func__, ret);
    return ret;
}

int voice_extn_compress_voip_close_output_stream(struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->dev;
    int ret = 0;

    ALOGD("%s: enter", __func__);
    if (voip_data.out_stream_count > 0) {
        voip_data.out_stream_count--;
        ret = voip_stop_call(adev);
        voip_data.out_stream = NULL;
        out->pcm = NULL;
    }

    ALOGV("%s: exit: status(%d)", __func__, ret);
    return ret;
}

int voice_extn_compress_voip_open_output_stream(struct stream_out *out)
{
    int mode, ret;

    ALOGD("%s: enter", __func__);

    out->supported_channel_masks[0] = AUDIO_CHANNEL_OUT_MONO;
    out->channel_mask = AUDIO_CHANNEL_OUT_MONO;
    out->usecase = USECASE_COMPRESS_VOIP_CALL;
    if (out->sample_rate == 16000)
        out->config = pcm_config_voip_wb;
    else
        out->config = pcm_config_voip_nb;

    voip_data.out_stream = out;
    voip_data.out_stream_count++;
    voip_data.sample_rate = out->sample_rate;
    ret = voip_set_mode(out->dev, out->format);

    ALOGV("%s: exit", __func__);
    return ret;
}

int voice_extn_compress_voip_close_input_stream(struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->dev;
    int status = 0;

    ALOGD("%s: enter", __func__);

    if(voip_data.in_stream_count > 0) {
       adev->active_input = NULL;
       voip_data.in_stream_count--;
       status = voip_stop_call(adev);
       in->pcm = NULL;
    }

    ALOGV("%s: exit: status(%d)", __func__, status);
    return status;
}

int voice_extn_compress_voip_open_input_stream(struct stream_in *in)
{
    int sample_rate;
    int buffer_size,frame_size;
    int mode, ret;

    ALOGD("%s: enter", __func__);

    if ((voip_data.sample_rate != 0) &&
        (voip_data.sample_rate != in->config.rate)) {
        ret = -ENOTSUP;
        goto done;
    } else {
        voip_data.sample_rate = in->config.rate;
    }

    in->usecase = USECASE_COMPRESS_VOIP_CALL;
    if (in->config.rate == 16000)
        in->config = pcm_config_voip_wb;
    else
        in->config = pcm_config_voip_nb;

    voip_data.in_stream_count++;
    ret = voip_set_mode(in->dev, in->format);

done:
    ALOGV("%s: exit, ret=%d", __func__, ret);
    return ret;
}

int voice_extn_compress_voip_set_volume(struct audio_device *adev, float volume)
{
    int vol, err = 0;

    ALOGV("%s: enter", __func__);

    if (volume < 0.0) {
        volume = 0.0;
    } else if (volume > 1.0) {
        volume = 1.0;
    }

    vol = lrint(volume * 100.0);

    /* Voice volume levels from android are mapped to driver volume levels as follows.
     * 0 -> 5, 20 -> 4, 40 ->3, 60 -> 2, 80 -> 1, 100 -> 0
     * So adjust the volume to get the correct volume index in driver
     */
    vol = 100 - vol;

    err = voip_set_volume(adev, vol);

    ALOGV("%s: exit: status(%d)", __func__, err);

    return err;
}

int voice_extn_compress_voip_set_mic_mute(struct audio_device *adev, bool state)
{
    int err = 0;

    ALOGV("%s: enter", __func__);

    err = voip_set_mic_mute(adev, state);

    ALOGV("%s: exit: status(%d)", __func__, err);
    return err;
}

bool voice_extn_compress_voip_pcm_prop_check()
{
    char prop_value[PROPERTY_VALUE_MAX] = {0};

    property_get("use.voice.path.for.pcm.voip", prop_value, "0");
    if (!strncmp("true", prop_value, sizeof("true")))
    {
        ALOGD("%s: VoIP PCM property is enabled", __func__);
        return true;
    }
    else
        return false;
}

bool voice_extn_compress_voip_is_active(struct audio_device *adev)
{
    struct audio_usecase *voip_usecase = NULL;
    voip_usecase = get_usecase_from_list(adev, USECASE_COMPRESS_VOIP_CALL);

    if (voip_usecase != NULL)
        return true;
    else
        return false;
}

bool voice_extn_compress_voip_is_format_supported(audio_format_t format)
{
    switch (format) {
    case AUDIO_FORMAT_PCM_16_BIT:
        if (voice_extn_compress_voip_pcm_prop_check())
            return true;
        else
            return false;
    case AUDIO_FORMAT_AMR_NB:
    case AUDIO_FORMAT_AMR_WB:
    case AUDIO_FORMAT_EVRC:
    case AUDIO_FORMAT_EVRCB:
    case AUDIO_FORMAT_EVRCWB:
    case AUDIO_FORMAT_EVRCNW:
        return true;
    default:
        return false;
    }
}

bool voice_extn_compress_voip_is_config_supported(struct audio_config *config)
{
    bool ret = false;

    ret = voice_extn_compress_voip_is_format_supported(config->format);
    if (ret) {
        if ((popcount(config->channel_mask) == 1) &&
            (config->sample_rate == 8000 || config->sample_rate == 16000))
            ret = ((voip_data.sample_rate == 0) ? true:
                    (voip_data.sample_rate == config->sample_rate));
        else
            ret = false;
    }
    return ret;
}
