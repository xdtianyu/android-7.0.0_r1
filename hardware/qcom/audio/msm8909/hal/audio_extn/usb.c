/*
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 * Not a Contribution.
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

#define LOG_TAG "audio_hw_usb"
#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <system/audio.h>
#include <tinyalsa/asoundlib.h>

#ifdef USB_HEADSET_ENABLED
#define USB_LOW_LATENCY_OUTPUT_PERIOD_SIZE   512
#define USB_LOW_LATENCY_OUTPUT_PERIOD_COUNT  8
#define USB_DEFAULT_OUTPUT_SAMPLING_RATE     48000

#define USB_PROXY_DEFAULT_SAMPLING_RATE      48000
#define USB_PROXY_OPEN_RETRY_COUNT           100
#define USB_PROXY_OPEN_WAIT_TIME             20
#define USB_PROXY_PERIOD_SIZE                3072
#define USB_PROXY_RATE_8000                  8000
#define USB_PROXY_RATE_16000                 16000
#define USB_PROXY_RATE_48000                 48000
#define USB_PERIOD_SIZE                      2048
#define USB_BUFF_SIZE                        2048
#define AFE_PROXY_PERIOD_COUNT               32
#define AFE_PROXY_PLAYBACK_DEVICE            8
#define AFE_PROXY_CAPTURE_DEVICE             7

struct usb_module {
    uint32_t usb_card;
    uint32_t proxy_card;
    uint32_t usb_device_id;
    uint32_t proxy_device_id;

    int32_t channels_playback;
    int32_t sample_rate_playback;
    int32_t channels_record;
    int32_t sample_rate_record;

    bool is_playback_running;
    bool is_record_running;

    pthread_t usb_playback_thr;
    pthread_t usb_record_thr;
    pthread_mutex_t usb_playback_lock;
    pthread_mutex_t usb_record_lock;

    struct pcm *proxy_pcm_playback_handle;
    struct pcm *usb_pcm_playback_handle;
    struct pcm *proxy_pcm_record_handle;
    struct pcm *usb_pcm_record_handle;
    struct audio_device *adev;
};

static struct usb_module *usbmod = NULL;
static pthread_once_t alloc_usbmod_once_ctl = PTHREAD_ONCE_INIT;

struct pcm_config pcm_config_usbmod = {
    .channels = 2,
    .rate = USB_DEFAULT_OUTPUT_SAMPLING_RATE,
    .period_size = USB_LOW_LATENCY_OUTPUT_PERIOD_SIZE,
    .period_count = USB_LOW_LATENCY_OUTPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = USB_LOW_LATENCY_OUTPUT_PERIOD_SIZE / 4,
    .stop_threshold = INT_MAX,
    .avail_min = USB_LOW_LATENCY_OUTPUT_PERIOD_SIZE / 4,
};

static void usb_alloc()
{
    usbmod = calloc(1, sizeof(struct usb_module));
}

// Some USB audio accessories have a really low default volume set. Look for a suitable
// volume control and set the volume to default volume level.
static void initPlaybackVolume() {
    ALOGD("initPlaybackVolume");
    struct mixer *usbMixer = mixer_open(1);

    if (usbMixer) {
         struct mixer_ctl *ctl = NULL;
         unsigned int usbPlaybackVolume;
         unsigned int i;
         unsigned int num_ctls = mixer_get_num_ctls(usbMixer);

         // Look for the first control named ".*Playback Volume" that isn't for a microphone
         for (i = 0; i < num_ctls; i++) {
             ctl = mixer_get_ctl(usbMixer, i);
             if ((ctl) && (strstr((const char *)mixer_ctl_get_name(ctl), "Playback Volume") &&
                 !strstr((const char *)mixer_ctl_get_name(ctl), "Mic"))) {
                   break;
             }
         }
         if (ctl != NULL) {
            ALOGD("Found a volume control for USB: %s", mixer_ctl_get_name(ctl) );
            usbPlaybackVolume = mixer_ctl_get_value(ctl, 0);
            ALOGD("Value got from mixer_ctl_get is:%u", usbPlaybackVolume);
            if (mixer_ctl_set_value(ctl,0,usbPlaybackVolume) < 0) {
               ALOGE("Failed to set volume; default volume might be used");
            }
         } else {
            ALOGE("No playback volume control found; default volume will be used");
         }
         mixer_close(usbMixer);
    } else {
         ALOGE("Failed to open mixer for card 1");
    }
}

static int usb_get_numof_rates(char *rates_str)
{
    int i, size = 0;
    char *next_sr_string, *temp_ptr;
    next_sr_string = strtok_r(rates_str, " ,", &temp_ptr);

    if (next_sr_string == NULL) {
        ALOGE("%s: get_numof_rates: could not find rates string", __func__);
        return 0;
    }

    for (i = 1; next_sr_string != NULL; i++) {
        size ++;
        next_sr_string = strtok_r(NULL, " ,.-", &temp_ptr);
    }
    return size;
}

static int usb_get_capability(char *type, int32_t *channels,
                                    int32_t *sample_rate)
{
    ALOGD("%s: for %s", __func__, type);
    long unsigned file_size;
    FILE *fp;
    char *buffer;
    int32_t err = 1;
    int32_t size = 0;
    int32_t fd=-1, i, channels_playback;
    char *str_start, *channel_start, *rates_str_start, *next_sr_str,
         *next_sr_string, *temp_ptr;
    struct stat st;
    char *read_buf = NULL;
    char *rates_str = NULL;
    char *rates_str_for_val = NULL;
    int  *rates_supported = NULL;
    char path[128];
    int ret = 0;

    memset(&st, 0x0, sizeof(struct stat));
    *sample_rate = 0;
    snprintf(path, sizeof(path), "/proc/asound/card%u/stream0",
             usbmod->usb_card);

    fd = open(path, O_RDONLY);
    if (fd <0) {
        ALOGE("%s: error failed to open config file %s error: %d\n",
              __func__, path, errno);
        ret = -EINVAL;
        goto done;
    }

    if (fstat(fd, &st) < 0) {
        ALOGE("%s: error failed to stat %s error %d\n",
             __func__, path, errno);
        ret = -EINVAL;
        goto done;
    }

    file_size = st.st_size;

    read_buf = (char *)calloc(1, USB_BUFF_SIZE + 1);

    if (!read_buf) {
        ALOGE("Failed to create read_buf");
        ret = -ENOMEM;
        goto done;
    }

    err = read(fd, read_buf, USB_BUFF_SIZE);
    str_start = strstr(read_buf, type);
    if (str_start == NULL) {
        ALOGE("%s: error %s section not found in usb config file",
               __func__, type);
        ret = -EINVAL;
        goto done;
    }

    channel_start = strstr(str_start, "Channels:");
    if (channel_start == NULL) {
        ALOGE("%s: error could not find Channels information", __func__);
        ret = -EINVAL;
        goto done;
    }

    channel_start = strstr(channel_start, " ");
    if (channel_start == NULL) {
        ALOGE("%s: error channel section not found in usb config file",
               __func__);
        ret = -EINVAL;
        goto done;
    }

    channels_playback = atoi(channel_start);
    if (channels_playback == 1) {
        *channels = 1;
    } else {
        *channels = 2;
    }

    ALOGD("%s: channels supported by device: %d", __func__, *channels);
    rates_str_start = strstr(str_start, "Rates:");
    if (rates_str_start == NULL) {
        ALOGE("%s: error cant find rates information", __func__);
        ret = -EINVAL;
        goto done;
    }

    rates_str_start = strstr(rates_str_start, " ");
    if (rates_str_start == NULL) {
        ALOGE("%s: error channel section not found in usb config file",
               __func__);
        ret = -EINVAL;
        goto done;
    }

    char *target = strchr(rates_str_start, '\n');
    if (target == NULL) {
        ALOGE("%s: error end of line not found", __func__);
        ret = -EINVAL;
        goto done;
    }

    size = target - rates_str_start;
    if ((rates_str = (char *)malloc(size + 1)) == NULL) {
        ALOGE("%s: error unable to allocate memory to hold sample rate strings",
              __func__);
        ret = -EINVAL;
        goto done;
    }

    if ((rates_str_for_val = (char *)malloc(size + 1)) == NULL) {
        ALOGE("%s: error unable to allocate memory to hold sample rate string",
               __func__);
        ret = -EINVAL;
        goto done;
    }

    memcpy(rates_str, rates_str_start, size);
    memcpy(rates_str_for_val, rates_str_start, size);
    rates_str[size] = '\0';
    rates_str_for_val[size] = '\0';

    size = usb_get_numof_rates(rates_str);
    if (!size) {
        ALOGE("%s: error could not get rate size, returning", __func__);
        ret = -EINVAL;
        goto done;
    }

    rates_supported = (int *)malloc(sizeof(int) * size);

    if (!rates_supported) {
        ALOGE("couldn't allocate mem for rates_supported");
        ret = -EINVAL;
        goto done;
    }

    next_sr_string = strtok_r(rates_str_for_val, " ,", &temp_ptr);
    if (next_sr_string == NULL) {
        ALOGE("%s: error could not get first rate val", __func__);
        ret = -EINVAL;
        goto done;
    }

    rates_supported[0] = atoi(next_sr_string);
    ALOGD("%s: rates_supported[0] for playback: %d",
           __func__, rates_supported[0]);
    for (i = 1; i<size; i++) {
        next_sr_string = strtok_r(NULL, " ,.-", &temp_ptr);
        if (next_sr_string == NULL) {
            rates_supported[i] = -1; // fill in an invalid sr for the rest
            continue;
        }
        rates_supported[i] = atoi(next_sr_string);
        ALOGD("rates_supported[%d] for playback: %d",i, rates_supported[i]);
    }

    for (i = 0; i<size; i++) {
        if ((rates_supported[i] > *sample_rate) &&
            (rates_supported[i] <= 48000)) {
            /* Sample Rate should be one of the proxy supported rates only
               This is because proxy port is used to read from/write to DSP */
            if ((rates_supported[i] == USB_PROXY_RATE_8000) ||
                (rates_supported[i] == USB_PROXY_RATE_16000) ||
                (rates_supported[i] == USB_PROXY_RATE_48000)) {
                *sample_rate = rates_supported[i];
            }
        }
    }
    ALOGD("%s: sample_rate: %d", __func__, *sample_rate);

done:
    if (fd >= 0) close(fd);
    if (rates_str_for_val) free(rates_str_for_val);
    if (rates_str) free(rates_str);
    if (rates_supported) free(rates_supported);
    if (read_buf) free(read_buf);
    return ret;
}

static int32_t usb_playback_entry(void *adev)
{
    unsigned char usbbuf[USB_PROXY_PERIOD_SIZE] = {0};
    int32_t ret, bytes, proxy_open_retry_count;

    ALOGD("%s: entry", __func__);
    /* update audio device pointer */
    usbmod->adev = (struct audio_device*)adev;
    proxy_open_retry_count = USB_PROXY_OPEN_RETRY_COUNT;

    /* get capabilities */
    pthread_mutex_lock(&usbmod->usb_playback_lock);
    ret = usb_get_capability((char *)"Playback:",
            &usbmod->channels_playback, &usbmod->sample_rate_playback);
    if (ret) {
        ALOGE("%s: could not get playback capabilities from usb device",
               __func__);
        pthread_mutex_unlock(&usbmod->usb_playback_lock);
        return -EINVAL;
    }
    /* update config for usb
       1 pcm frame(sample)= 4 bytes since two channels*/
    pcm_config_usbmod.period_size = USB_PERIOD_SIZE/4;
    pcm_config_usbmod.channels = usbmod->channels_playback;
    pcm_config_usbmod.rate = usbmod->sample_rate_playback;
    ALOGV("%s: usb device %u:period %u:channels %u:sample", __func__,
          pcm_config_usbmod.period_size, pcm_config_usbmod.channels,
          pcm_config_usbmod.rate);

    usbmod->usb_pcm_playback_handle = pcm_open(usbmod->usb_card, \
                                    usbmod->usb_device_id, PCM_OUT |
                                    PCM_MMAP | PCM_NOIRQ , &pcm_config_usbmod);

    if ((usbmod->usb_pcm_playback_handle \
        && !pcm_is_ready(usbmod->usb_pcm_playback_handle))
        || (!usbmod->is_playback_running)) {
        ALOGE("%s: failed: %s", __func__,
               pcm_get_error(usbmod->usb_pcm_playback_handle));
        pcm_close(usbmod->usb_pcm_playback_handle);
        usbmod->usb_pcm_playback_handle = NULL;
        pthread_mutex_unlock(&usbmod->usb_playback_lock);
        return -ENOMEM;
    }
    ALOGD("%s: USB configured for playback", __func__);

    /* update config for proxy*/
    pcm_config_usbmod.period_size = USB_PROXY_PERIOD_SIZE/3;
    pcm_config_usbmod.rate = usbmod->sample_rate_playback;
    pcm_config_usbmod.channels = usbmod->channels_playback;
    pcm_config_usbmod.period_count = AFE_PROXY_PERIOD_COUNT;
    usbmod->proxy_device_id = AFE_PROXY_PLAYBACK_DEVICE;
    ALOGD("%s: proxy device %u:period %u:channels %u:sample", __func__,
          pcm_config_usbmod.period_size, pcm_config_usbmod.channels,
          pcm_config_usbmod.rate);

    while(proxy_open_retry_count){
        usbmod->proxy_pcm_playback_handle = pcm_open(usbmod->proxy_card,
                                            usbmod->proxy_device_id, PCM_IN |
                                     PCM_MMAP | PCM_NOIRQ, &pcm_config_usbmod);
        if(usbmod->proxy_pcm_playback_handle
            && !pcm_is_ready(usbmod->proxy_pcm_playback_handle)){
                     pcm_close(usbmod->proxy_pcm_playback_handle);
                     proxy_open_retry_count--;
                     usleep(USB_PROXY_OPEN_WAIT_TIME * 1000);
                     ALOGE("%s: pcm_open for proxy failed retrying = %d",
                            __func__, proxy_open_retry_count);
                }
                else{
                  break;
                }
    }

    if ((usbmod->proxy_pcm_playback_handle
        && !pcm_is_ready(usbmod->proxy_pcm_playback_handle))
        || (!usbmod->is_playback_running)) {
        ALOGE("%s: failed: %s", __func__,
               pcm_get_error(usbmod->proxy_pcm_playback_handle));
        pcm_close(usbmod->proxy_pcm_playback_handle);
        usbmod->proxy_pcm_playback_handle = NULL;
        pthread_mutex_unlock(&usbmod->usb_playback_lock);
        return -ENOMEM;
    }
    ALOGD("%s: PROXY configured for playback", __func__);
    pthread_mutex_unlock(&usbmod->usb_playback_lock);

    ALOGD("Init USB volume");
    initPlaybackVolume();
    /* main loop to read from proxy and write to usb */
    while (usbmod->is_playback_running) {
        /* read data from proxy */
        ret = pcm_mmap_read(usbmod->proxy_pcm_playback_handle,
                                 (void *)usbbuf, USB_PROXY_PERIOD_SIZE);
        /* Write to usb */
        ret = pcm_mmap_write(usbmod->usb_pcm_playback_handle,
                                (void *)usbbuf, USB_PROXY_PERIOD_SIZE);
        if(!usbmod->is_playback_running)
            break;

        memset(usbbuf, 0, USB_PROXY_PERIOD_SIZE);
    } /* main loop end */

    ALOGD("%s: exiting USB playback thread",__func__);
    return 0;
}

static void* usb_playback_launcher(void *adev)
{
    int32_t ret;

    usbmod->is_playback_running = true;
    ret = usb_playback_entry(adev);

    if (ret) {
        ALOGE("%s: failed with err:%d", __func__, ret);
        usbmod->is_playback_running = false;
    }
    return NULL;
}

static int32_t usb_record_entry(void *adev)
{
    unsigned char usbbuf[USB_PROXY_PERIOD_SIZE] = {0};
    int32_t ret, bytes, proxy_open_retry_count;
    ALOGD("%s: entry", __func__);

    /* update audio device pointer */
    usbmod->adev = (struct audio_device*)adev;
    proxy_open_retry_count = USB_PROXY_OPEN_RETRY_COUNT;

    /* get capabilities */
    pthread_mutex_lock(&usbmod->usb_record_lock);
    ret = usb_get_capability((char *)"Capture:",
            &usbmod->channels_record, &usbmod->sample_rate_record);
    if (ret) {
        ALOGE("%s: could not get capture capabilities from usb device",
               __func__);
        pthread_mutex_unlock(&usbmod->usb_record_lock);
        return -EINVAL;
    }
    /* update config for usb
       1 pcm frame(sample)= 4 bytes since two channels*/
    pcm_config_usbmod.period_size = USB_PERIOD_SIZE/4;
    pcm_config_usbmod.channels = usbmod->channels_record;
    pcm_config_usbmod.rate = usbmod->sample_rate_record;
    ALOGV("%s: usb device %u:period %u:channels %u:sample", __func__,
          pcm_config_usbmod.period_size, pcm_config_usbmod.channels,
          pcm_config_usbmod.rate);

    usbmod->usb_pcm_record_handle = pcm_open(usbmod->usb_card, \
                                    usbmod->usb_device_id, PCM_IN |
                                    PCM_MMAP | PCM_NOIRQ , &pcm_config_usbmod);

    if ((usbmod->usb_pcm_record_handle \
        && !pcm_is_ready(usbmod->usb_pcm_record_handle))
        || (!usbmod->is_record_running)) {
        ALOGE("%s: failed: %s", __func__,
               pcm_get_error(usbmod->usb_pcm_record_handle));
        pcm_close(usbmod->usb_pcm_record_handle);
        usbmod->usb_pcm_record_handle = NULL;
        pthread_mutex_unlock(&usbmod->usb_record_lock);
        return -ENOMEM;
    }
    ALOGD("%s: USB configured for capture", __func__);

    /* update config for proxy*/
    pcm_config_usbmod.period_size = USB_PROXY_PERIOD_SIZE/4;
    pcm_config_usbmod.rate = usbmod->sample_rate_record;
    pcm_config_usbmod.channels = usbmod->channels_record;
    pcm_config_usbmod.period_count = AFE_PROXY_PERIOD_COUNT * 2;
    usbmod->proxy_device_id = AFE_PROXY_CAPTURE_DEVICE;
    ALOGV("%s: proxy device %u:period %u:channels %u:sample", __func__,
          pcm_config_usbmod.period_size, pcm_config_usbmod.channels,
          pcm_config_usbmod.rate);

    while(proxy_open_retry_count){
        usbmod->proxy_pcm_record_handle = pcm_open(usbmod->proxy_card,
                                            usbmod->proxy_device_id, PCM_OUT |
                                     PCM_MMAP | PCM_NOIRQ, &pcm_config_usbmod);
        if(usbmod->proxy_pcm_record_handle
            && !pcm_is_ready(usbmod->proxy_pcm_record_handle)){
                     pcm_close(usbmod->proxy_pcm_record_handle);
                     proxy_open_retry_count--;
                     usleep(USB_PROXY_OPEN_WAIT_TIME * 1000);
                     ALOGE("%s: pcm_open for proxy(recording) failed retrying = %d",
                            __func__, proxy_open_retry_count);
                }
                else{
                  break;
                }
    }
    if ((usbmod->proxy_pcm_record_handle
        && !pcm_is_ready(usbmod->proxy_pcm_record_handle))
        || (!usbmod->is_record_running)) {
        ALOGE("%s: failed: %s", __func__,
               pcm_get_error(usbmod->proxy_pcm_record_handle));
        pcm_close(usbmod->proxy_pcm_record_handle);
        usbmod->proxy_pcm_record_handle = NULL;
        pthread_mutex_unlock(&usbmod->usb_record_lock);
        return -ENOMEM;
    }
    ALOGD("%s: PROXY configured for capture", __func__);
    pthread_mutex_unlock(&usbmod->usb_record_lock);

    /* main loop to read from usb and write to proxy */
    while (usbmod->is_record_running) {
        /* read data from usb */
        ret = pcm_mmap_read(usbmod->usb_pcm_record_handle,
                                 (void *)usbbuf, USB_PROXY_PERIOD_SIZE);
        /* Write to proxy */
        ret = pcm_mmap_write(usbmod->proxy_pcm_record_handle,
                                (void *)usbbuf, USB_PROXY_PERIOD_SIZE);
        if(!usbmod->is_record_running)
            break;

        memset(usbbuf, 0, USB_PROXY_PERIOD_SIZE);
    } /* main loop end */

    ALOGD("%s: exiting USB capture thread",__func__);
    return 0;
}

static void* usb_capture_launcher(void *adev)
{
    int32_t ret;

    usbmod->is_record_running = true;
    ret = usb_record_entry(adev);

    if (ret) {
        ALOGE("%s: failed with err:%d", __func__, ret);
        usbmod->is_record_running = false;
    }
    return NULL;
}

void audio_extn_usb_init(void *adev)
{
    pthread_once(&alloc_usbmod_once_ctl, usb_alloc);

    usbmod->is_playback_running = false;
    usbmod->is_record_running = false;

    usbmod->usb_pcm_playback_handle = NULL;
    usbmod->proxy_pcm_playback_handle = NULL;

    usbmod->usb_pcm_record_handle = NULL;
    usbmod->proxy_pcm_record_handle = NULL;

    usbmod->usb_card = 1;
    usbmod->usb_device_id = 0;
    usbmod->proxy_card = 0;
    usbmod->proxy_device_id = AFE_PROXY_PLAYBACK_DEVICE;
    usbmod->adev = (struct audio_device*)adev;

     pthread_mutex_init(&usbmod->usb_playback_lock,
                        (const pthread_mutexattr_t *) NULL);
     pthread_mutex_init(&usbmod->usb_record_lock,
                        (const pthread_mutexattr_t *) NULL);
}

void audio_extn_usb_deinit()
{
    if (NULL != usbmod){
        free(usbmod);
        usbmod = NULL;
    }
}

void audio_extn_usb_set_proxy_sound_card(uint32_t sndcard_idx)
{
    /* Proxy port and USB headset are related to two different sound cards */
    if (sndcard_idx == usbmod->usb_card) {
        usbmod->usb_card = usbmod->proxy_card;
    }

    usbmod->proxy_card = sndcard_idx;
}

void audio_extn_usb_start_playback(void *adev)
{
    int32_t ret;

    if (NULL == usbmod){
        ALOGE("%s: USB device object is NULL", __func__);
        return;
    }

    if (usbmod->is_playback_running){
        ALOGE("%s: USB playback thread already running", __func__);
        return;
    }

    ALOGD("%s: creating USB playback thread", __func__);
    ret = pthread_create(&usbmod->usb_playback_thr, NULL,
                         usb_playback_launcher, (void*)adev);
    if (ret)
        ALOGE("%s: failed to create USB playback thread with err:%d",
              __func__, ret);
}

void audio_extn_usb_stop_playback()
{
    int32_t ret;
    ALOGD("%s: entry", __func__);

    usbmod->is_playback_running = false;
    if (NULL != usbmod->proxy_pcm_playback_handle)
        pcm_stop(usbmod->proxy_pcm_playback_handle);

    if (NULL != usbmod->usb_pcm_playback_handle)
        pcm_stop(usbmod->usb_pcm_playback_handle);

    if(usbmod->usb_playback_thr) {
        ret = pthread_join(usbmod->usb_playback_thr,NULL);
        ALOGE("%s: return for pthread_join = %d", __func__, ret);
        usbmod->usb_playback_thr = (pthread_t)NULL;
    }

    pthread_mutex_lock(&usbmod->usb_playback_lock);
    if (NULL != usbmod->usb_pcm_playback_handle){
        pcm_close(usbmod->usb_pcm_playback_handle);
        usbmod->usb_pcm_playback_handle = NULL;
    }

    if (NULL != usbmod->proxy_pcm_playback_handle){
        pcm_close(usbmod->proxy_pcm_playback_handle);
        usbmod->proxy_pcm_playback_handle = NULL;
    }
    pthread_mutex_unlock(&usbmod->usb_playback_lock);

    ALOGD("%s: exiting",__func__);
}

void audio_extn_usb_start_capture(void *adev)
{
    int32_t ret;

    if (NULL == usbmod){
        ALOGE("%s: USB device object is NULL", __func__);
        return;
    }

    if (usbmod->is_record_running){
        ALOGE("%s: USB capture thread already running", __func__);
        return;
    }

    ALOGD("%s: creating USB capture thread", __func__);
    ret = pthread_create(&usbmod->usb_record_thr, NULL,
                         usb_capture_launcher, (void*)adev);
    if (ret)
        ALOGE("%s: failed to create USB capture thread with err:%d",
              __func__, ret);
}

void audio_extn_usb_stop_capture()
{
    int32_t ret;
    ALOGD("%s: entry", __func__);

    usbmod->is_record_running = false;
    if (NULL != usbmod->proxy_pcm_record_handle)
        pcm_stop(usbmod->proxy_pcm_record_handle);

    if (NULL != usbmod->usb_pcm_record_handle)
        pcm_stop(usbmod->usb_pcm_record_handle);

    if(usbmod->usb_record_thr) {
        ret = pthread_join(usbmod->usb_record_thr,NULL);
        ALOGE("%s: return for pthread_join = %d", __func__, ret);
        usbmod->usb_record_thr = (pthread_t)NULL;
    }

    pthread_mutex_lock(&usbmod->usb_record_lock);
    if (NULL != usbmod->usb_pcm_record_handle){
        pcm_close(usbmod->usb_pcm_record_handle);
        usbmod->usb_pcm_record_handle = NULL;
    }

    if (NULL != usbmod->proxy_pcm_record_handle){
        pcm_close(usbmod->proxy_pcm_record_handle);
        usbmod->proxy_pcm_record_handle = NULL;
    }
    pthread_mutex_unlock(&usbmod->usb_record_lock);

    ALOGD("%s: exiting",__func__);
}

bool audio_extn_usb_is_proxy_inuse()
{
    if( usbmod->is_record_running || usbmod->is_playback_running)
        return true;
    else
        return false;
}
#endif /*USB_HEADSET_ENABLED end*/
