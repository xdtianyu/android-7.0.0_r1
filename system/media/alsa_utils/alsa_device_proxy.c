/*
 * Copyright (C) 2014 The Android Open Source Project
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

#define LOG_TAG "alsa_device_proxy"
/*#define LOG_NDEBUG 0*/
/*#define LOG_PCM_PARAMS 0*/

#include <log/log.h>

#include <errno.h>

#include "include/alsa_device_proxy.h"

#include "include/alsa_logging.h"

#define DEFAULT_PERIOD_SIZE     1024
#define DEFAULT_PERIOD_COUNT    2

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const unsigned format_byte_size_map[] = {
    2, /* PCM_FORMAT_S16_LE */
    4, /* PCM_FORMAT_S32_LE */
    1, /* PCM_FORMAT_S8 */
    4, /* PCM_FORMAT_S24_LE */
    3, /* PCM_FORMAT_S24_3LE */
};

void proxy_prepare(alsa_device_proxy * proxy, alsa_device_profile* profile,
                   struct pcm_config * config)
{
    ALOGV("proxy_prepare(c:%d, d:%d)", profile->card, profile->device);

    proxy->profile = profile;

#ifdef LOG_PCM_PARAMS
    log_pcm_config(config, "proxy_setup()");
#endif

    if (config->format != PCM_FORMAT_INVALID && profile_is_format_valid(profile, config->format)) {
        proxy->alsa_config.format = config->format;
    } else {
        ALOGW("Invalid format %d - using default %d.",
              config->format, profile->default_config.format);
        proxy->alsa_config.format = profile->default_config.format;
    }

    if (config->rate != 0 && profile_is_sample_rate_valid(profile, config->rate)) {
        proxy->alsa_config.rate = config->rate;
    } else {
        ALOGW("Invalid sample rate %u - using default %u.",
              config->rate, profile->default_config.rate);
        proxy->alsa_config.rate = profile->default_config.rate;
    }

    if (config->channels != 0 && profile_is_channel_count_valid(profile, config->channels)) {
        proxy->alsa_config.channels = config->channels;
    } else {
        ALOGW("Invalid channel count %u - using default %u.",
              config->channels, profile->default_config.channels);
        proxy->alsa_config.channels = profile->default_config.channels;

    }

    proxy->alsa_config.period_count = profile->default_config.period_count;
    proxy->alsa_config.period_size =
            profile_get_period_size(proxy->profile, proxy->alsa_config.rate);

    // Hack for USB accessory audio.
    // Here we set the correct value for period_count if tinyalsa fails to get it from the
    // f_audio_source driver.
    if (proxy->alsa_config.period_count == 0) {
        proxy->alsa_config.period_count = 4;
    }

    proxy->pcm = NULL;
    // config format should be checked earlier against profile.
    if (config->format >= 0 && (size_t)config->format < ARRAY_SIZE(format_byte_size_map)) {
        proxy->frame_size = format_byte_size_map[config->format] * proxy->alsa_config.channels;
    } else {
        proxy->frame_size = 1;
    }
}

int proxy_open(alsa_device_proxy * proxy)
{
    alsa_device_profile* profile = proxy->profile;
    ALOGV("proxy_open(card:%d device:%d %s)", profile->card, profile->device,
          profile->direction == PCM_OUT ? "PCM_OUT" : "PCM_IN");

    if (profile->card < 0 || profile->device < 0) {
        return -EINVAL;
    }

    proxy->pcm = pcm_open(profile->card, profile->device,
            profile->direction | PCM_MONOTONIC, &proxy->alsa_config);
    if (proxy->pcm == NULL) {
        return -ENOMEM;
    }

    if (!pcm_is_ready(proxy->pcm)) {
        ALOGE("  proxy_open() pcm_open() failed: %s", pcm_get_error(proxy->pcm));
#if defined(LOG_PCM_PARAMS)
        log_pcm_config(&proxy->alsa_config, "config");
#endif
        pcm_close(proxy->pcm);
        proxy->pcm = NULL;
        return -ENOMEM;
    }

    return 0;
}

void proxy_close(alsa_device_proxy * proxy)
{
    ALOGV("proxy_close() [pcm:%p]", proxy->pcm);

    if (proxy->pcm != NULL) {
        pcm_close(proxy->pcm);
        proxy->pcm = NULL;
    }
}

/*
 * Sample Rate
 */
unsigned proxy_get_sample_rate(const alsa_device_proxy * proxy)
{
    return proxy->alsa_config.rate;
}

/*
 * Format
 */
enum pcm_format proxy_get_format(const alsa_device_proxy * proxy)
{
    return proxy->alsa_config.format;
}

/*
 * Channel Count
 */
unsigned proxy_get_channel_count(const alsa_device_proxy * proxy)
{
    return proxy->alsa_config.channels;
}

/*
 * Other
 */
unsigned int proxy_get_period_size(const alsa_device_proxy * proxy)
{
    return proxy->alsa_config.period_size;
}

unsigned int proxy_get_period_count(const alsa_device_proxy * proxy)
{
    return proxy->alsa_config.period_count;
}

unsigned proxy_get_latency(const alsa_device_proxy * proxy)
{
    return (proxy_get_period_size(proxy) * proxy_get_period_count(proxy) * 1000)
               / proxy_get_sample_rate(proxy);
}

int proxy_get_presentation_position(const alsa_device_proxy * proxy,
        uint64_t *frames, struct timespec *timestamp)
{
    int ret = -EPERM; // -1
    unsigned int avail;
    if (proxy->pcm != NULL
            && pcm_get_htimestamp(proxy->pcm, &avail, timestamp) == 0) {
        const size_t kernel_buffer_size =
                proxy->alsa_config.period_size * proxy->alsa_config.period_count;
        if (avail > kernel_buffer_size) {
            ALOGE("available frames(%u) > buffer size(%zu)", avail, kernel_buffer_size);
        } else {
            int64_t signed_frames = proxy->transferred - kernel_buffer_size + avail;
            // It is possible to compensate for additional driver and device delay
            // by changing signed_frames.  Example:
            // signed_frames -= 20 /* ms */ * proxy->alsa_config.rate / 1000;
            if (signed_frames >= 0) {
                *frames = signed_frames;
                ret = 0;
            }
        }
    }
    return ret;
}

/*
 * I/O
 */
int proxy_write(alsa_device_proxy * proxy, const void *data, unsigned int count)
{
    int ret = pcm_write(proxy->pcm, data, count);
    if (ret == 0) {
        proxy->transferred += count / proxy->frame_size;
    }
    return ret;
}

int proxy_read(const alsa_device_proxy * proxy, void *data, unsigned int count)
{
    return pcm_read(proxy->pcm, data, count);
}
