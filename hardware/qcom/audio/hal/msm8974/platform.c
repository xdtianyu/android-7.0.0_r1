/*
 * Copyright (C) 2013-2016 The Android Open Source Project
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
#define LOG_TAG "msm8974_platform"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <stdlib.h>
#include <dlfcn.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>
#include <audio_hw.h>
#include <platform_api.h>
#include "platform.h"
#include "audio_extn.h"
#include <linux/msm_audio.h>

#define MIXER_XML_DEFAULT_PATH "/system/etc/mixer_paths.xml"
#define MIXER_XML_BASE_STRING "/system/etc/mixer_paths"
#define TOMTOM_8226_SND_CARD_NAME "msm8226-tomtom-snd-card"
#define TOMTOM_MIXER_FILE_SUFFIX "wcd9330"

#define LIB_ACDB_LOADER "libacdbloader.so"
#define AUDIO_DATA_BLOCK_MIXER_CTL "HDMI EDID"
#define CVD_VERSION_MIXER_CTL "CVD Version"

#define min(a, b) ((a) < (b) ? (a) : (b))

/*
 * This file will have a maximum of 38 bytes:
 *
 * 4 bytes: number of audio blocks
 * 4 bytes: total length of Short Audio Descriptor (SAD) blocks
 * Maximum 10 * 3 bytes: SAD blocks
 */
#define MAX_SAD_BLOCKS      10
#define SAD_BLOCK_SIZE      3

#define MAX_CVD_VERSION_STRING_SIZE    100

/* EDID format ID for LPCM audio */
#define EDID_FORMAT_LPCM    1

/* Retry for delay in FW loading*/
#define RETRY_NUMBER 10
#define RETRY_US 500000
#define MAX_SND_CARD 8

#define MAX_SND_CARD_NAME_LEN 31

#define DEFAULT_APP_TYPE_RX_PATH  0x11130

#define TOSTRING_(x) #x
#define TOSTRING(x) TOSTRING_(x)

struct audio_block_header
{
    int reserved;
    int length;
};

enum {
    CAL_MODE_SEND           = 0x1,
    CAL_MODE_PERSIST        = 0x2,
    CAL_MODE_RTAC           = 0x4
};

#define PLATFORM_CONFIG_KEY_OPERATOR_INFO "operator_info"

struct operator_info {
    struct listnode list;
    char *name;
    char *mccmnc;
};

struct operator_specific_device {
    struct listnode list;
    char *operator;
    char *mixer_path;
    int acdb_id;
};

static struct listnode operator_info_list;
static struct listnode *operator_specific_device_table[SND_DEVICE_MAX];

/* Audio calibration related functions */
typedef void (*acdb_deallocate_t)();
typedef int  (*acdb_init_v2_cvd_t)(char *, char *, int);
typedef int  (*acdb_init_v2_t)(char *);
typedef int  (*acdb_init_t)();
typedef void (*acdb_send_audio_cal_t)(int, int);
typedef void (*acdb_send_voice_cal_t)(int, int);
typedef int (*acdb_reload_vocvoltable_t)(int);
typedef int (*acdb_send_gain_dep_cal_t)(int, int, int, int, int);

/* Audio calibration related functions */
struct platform_data {
    struct audio_device *adev;
    bool fluence_in_spkr_mode;
    bool fluence_in_voice_call;
    bool fluence_in_voice_comm;
    bool fluence_in_voice_rec;
    /* 0 = no fluence, 1 = fluence, 2 = fluence pro */
    int  fluence_type;
    int  source_mic_type;
    bool speaker_lr_swap;

    void *acdb_handle;
    acdb_deallocate_t          acdb_deallocate;
    acdb_send_audio_cal_t      acdb_send_audio_cal;
    acdb_send_voice_cal_t      acdb_send_voice_cal;
    acdb_reload_vocvoltable_t  acdb_reload_vocvoltable;
    acdb_send_gain_dep_cal_t   acdb_send_gain_dep_cal;
    struct csd_data *csd;
    char ec_ref_mixer_path[64];

    char *snd_card_name;
    int max_vol_index;
    int max_mic_count;

    void *hw_info;
};

static int pcm_device_table[AUDIO_USECASE_MAX][2] = {
    [USECASE_AUDIO_PLAYBACK_DEEP_BUFFER] = {DEEP_BUFFER_PCM_DEVICE,
                                            DEEP_BUFFER_PCM_DEVICE},
    [USECASE_AUDIO_PLAYBACK_LOW_LATENCY] = {LOWLATENCY_PCM_DEVICE,
                                            LOWLATENCY_PCM_DEVICE},
    [USECASE_AUDIO_PLAYBACK_MULTI_CH] = {MULTIMEDIA2_PCM_DEVICE,
                                         MULTIMEDIA2_PCM_DEVICE},
    [USECASE_AUDIO_PLAYBACK_OFFLOAD] = {PLAYBACK_OFFLOAD_DEVICE,
                                        PLAYBACK_OFFLOAD_DEVICE},
    [USECASE_AUDIO_PLAYBACK_TTS] = {MULTIMEDIA2_PCM_DEVICE,
                                        MULTIMEDIA2_PCM_DEVICE},
    [USECASE_AUDIO_PLAYBACK_ULL] = {MULTIMEDIA3_PCM_DEVICE,
                                    MULTIMEDIA3_PCM_DEVICE},

    [USECASE_AUDIO_RECORD] = {AUDIO_RECORD_PCM_DEVICE,
                              AUDIO_RECORD_PCM_DEVICE},
    [USECASE_AUDIO_RECORD_LOW_LATENCY] = {LOWLATENCY_PCM_DEVICE,
                                          LOWLATENCY_PCM_DEVICE},

    [USECASE_VOICE_CALL] = {VOICE_CALL_PCM_DEVICE,
                            VOICE_CALL_PCM_DEVICE},
    [USECASE_VOICE2_CALL] = {VOICE2_CALL_PCM_DEVICE, VOICE2_CALL_PCM_DEVICE},
    [USECASE_VOLTE_CALL] = {VOLTE_CALL_PCM_DEVICE, VOLTE_CALL_PCM_DEVICE},
    [USECASE_QCHAT_CALL] = {QCHAT_CALL_PCM_DEVICE, QCHAT_CALL_PCM_DEVICE},
    [USECASE_VOWLAN_CALL] = {VOWLAN_CALL_PCM_DEVICE, VOWLAN_CALL_PCM_DEVICE},
    [USECASE_VOICEMMODE1_CALL] = {VOICEMMODE1_CALL_PCM_DEVICE,
                                  VOICEMMODE1_CALL_PCM_DEVICE},
    [USECASE_VOICEMMODE2_CALL] = {VOICEMMODE2_CALL_PCM_DEVICE,
                                  VOICEMMODE2_CALL_PCM_DEVICE},

    [USECASE_INCALL_REC_UPLINK] = {AUDIO_RECORD_PCM_DEVICE,
                                   AUDIO_RECORD_PCM_DEVICE},
    [USECASE_INCALL_REC_DOWNLINK] = {AUDIO_RECORD_PCM_DEVICE,
                                     AUDIO_RECORD_PCM_DEVICE},
    [USECASE_INCALL_REC_UPLINK_AND_DOWNLINK] = {AUDIO_RECORD_PCM_DEVICE,
                                                AUDIO_RECORD_PCM_DEVICE},
    [USECASE_AUDIO_HFP_SCO] = {HFP_PCM_RX, HFP_SCO_RX},

    [USECASE_AUDIO_SPKR_CALIB_RX] = {SPKR_PROT_CALIB_RX_PCM_DEVICE, -1},
    [USECASE_AUDIO_SPKR_CALIB_TX] = {-1, SPKR_PROT_CALIB_TX_PCM_DEVICE},

    [USECASE_AUDIO_PLAYBACK_AFE_PROXY] = {AFE_PROXY_PLAYBACK_PCM_DEVICE,
                                          AFE_PROXY_RECORD_PCM_DEVICE},
    [USECASE_AUDIO_RECORD_AFE_PROXY] = {AFE_PROXY_PLAYBACK_PCM_DEVICE,
                                        AFE_PROXY_RECORD_PCM_DEVICE},
    [USECASE_AUDIO_DSM_FEEDBACK] = {QUAT_MI2S_PCM_DEVICE, QUAT_MI2S_PCM_DEVICE},

};

/* Array to store sound devices */
static const char * const device_table[SND_DEVICE_MAX] = {
    [SND_DEVICE_NONE] = "none",
    /* Playback sound devices */
    [SND_DEVICE_OUT_HANDSET] = "handset",
    [SND_DEVICE_OUT_SPEAKER] = "speaker",
    [SND_DEVICE_OUT_SPEAKER_REVERSE] = "speaker-reverse",
    [SND_DEVICE_OUT_SPEAKER_SAFE] = "speaker-safe",
    [SND_DEVICE_OUT_HEADPHONES] = "headphones",
    [SND_DEVICE_OUT_LINE] = "line",
    [SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES] = "speaker-and-headphones",
    [SND_DEVICE_OUT_SPEAKER_SAFE_AND_HEADPHONES] = "speaker-safe-and-headphones",
    [SND_DEVICE_OUT_SPEAKER_AND_LINE] = "speaker-and-line",
    [SND_DEVICE_OUT_SPEAKER_SAFE_AND_LINE] = "speaker-safe-and-line",
    [SND_DEVICE_OUT_VOICE_HANDSET] = "voice-handset",
    [SND_DEVICE_OUT_VOICE_HAC_HANDSET] = "voice-hac-handset",
    [SND_DEVICE_OUT_VOICE_SPEAKER] = "voice-speaker",
    [SND_DEVICE_OUT_VOICE_HEADPHONES] = "voice-headphones",
    [SND_DEVICE_OUT_VOICE_LINE] = "voice-line",
    [SND_DEVICE_OUT_HDMI] = "hdmi",
    [SND_DEVICE_OUT_SPEAKER_AND_HDMI] = "speaker-and-hdmi",
    [SND_DEVICE_OUT_BT_SCO] = "bt-sco-headset",
    [SND_DEVICE_OUT_BT_SCO_WB] = "bt-sco-headset-wb",
    [SND_DEVICE_OUT_VOICE_HANDSET_TMUS] = "voice-handset-tmus",
    [SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES] = "voice-tty-full-headphones",
    [SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES] = "voice-tty-vco-headphones",
    [SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET] = "voice-tty-hco-handset",
    [SND_DEVICE_OUT_VOICE_TX] = "voice-tx",
    [SND_DEVICE_OUT_SPEAKER_PROTECTED] = "speaker-protected",
    [SND_DEVICE_OUT_VOICE_SPEAKER_PROTECTED] = "voice-speaker-protected",
    [SND_DEVICE_OUT_VOICE_SPEAKER_HFP] = "voice-speaker-hfp",

    /* Capture sound devices */
    [SND_DEVICE_IN_HANDSET_MIC] = "handset-mic",
    [SND_DEVICE_IN_HANDSET_MIC_AEC] = "handset-mic",
    [SND_DEVICE_IN_HANDSET_MIC_NS] = "handset-mic",
    [SND_DEVICE_IN_HANDSET_MIC_AEC_NS] = "handset-mic",
    [SND_DEVICE_IN_HANDSET_DMIC] = "dmic-endfire",
    [SND_DEVICE_IN_HANDSET_DMIC_AEC] = "dmic-endfire",
    [SND_DEVICE_IN_HANDSET_DMIC_NS] = "dmic-endfire",
    [SND_DEVICE_IN_HANDSET_DMIC_AEC_NS] = "dmic-endfire",
    [SND_DEVICE_IN_HANDSET_DMIC_STEREO] = "dmic-endfire",

    [SND_DEVICE_IN_SPEAKER_MIC] = "speaker-mic",
    [SND_DEVICE_IN_SPEAKER_MIC_AEC] = "speaker-mic",
    [SND_DEVICE_IN_SPEAKER_MIC_NS] = "speaker-mic",
    [SND_DEVICE_IN_SPEAKER_MIC_AEC_NS] = "speaker-mic",
    [SND_DEVICE_IN_SPEAKER_DMIC] = "speaker-dmic-endfire",
    [SND_DEVICE_IN_SPEAKER_DMIC_AEC] = "speaker-dmic-endfire",
    [SND_DEVICE_IN_SPEAKER_DMIC_NS] = "speaker-dmic-endfire",
    [SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS] = "speaker-dmic-endfire",
    [SND_DEVICE_IN_SPEAKER_DMIC_STEREO] = "speaker-dmic-endfire",

    [SND_DEVICE_IN_HEADSET_MIC] = "headset-mic",
    [SND_DEVICE_IN_HEADSET_MIC_AEC] = "headset-mic",

    [SND_DEVICE_IN_HDMI_MIC] = "hdmi-mic",
    [SND_DEVICE_IN_BT_SCO_MIC] = "bt-sco-mic",
    [SND_DEVICE_IN_BT_SCO_MIC_NREC] = "bt-sco-mic",
    [SND_DEVICE_IN_BT_SCO_MIC_WB] = "bt-sco-mic-wb",
    [SND_DEVICE_IN_BT_SCO_MIC_WB_NREC] = "bt-sco-mic-wb",
    [SND_DEVICE_IN_CAMCORDER_MIC] = "camcorder-mic",

    [SND_DEVICE_IN_VOICE_DMIC] = "voice-dmic-ef",
    [SND_DEVICE_IN_VOICE_DMIC_TMUS] = "voice-dmic-ef-tmus",
    [SND_DEVICE_IN_VOICE_SPEAKER_MIC] = "voice-speaker-mic",
    [SND_DEVICE_IN_VOICE_SPEAKER_DMIC] = "voice-speaker-dmic-ef",
    [SND_DEVICE_IN_VOICE_SPEAKER_MIC_HFP] = "voice-speaker-mic-hfp",
    [SND_DEVICE_IN_VOICE_HEADSET_MIC] = "voice-headset-mic",
    [SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC] = "voice-tty-full-headset-mic",
    [SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC] = "voice-tty-vco-handset-mic",
    [SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC] = "voice-tty-hco-headset-mic",

    [SND_DEVICE_IN_VOICE_REC_MIC] = "voice-rec-mic",
    [SND_DEVICE_IN_VOICE_REC_MIC_NS] = "voice-rec-mic",
    [SND_DEVICE_IN_VOICE_REC_MIC_AEC] = "voice-rec-mic",
    [SND_DEVICE_IN_VOICE_REC_DMIC_STEREO] = "voice-rec-dmic-ef",
    [SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE] = "voice-rec-dmic-ef-fluence",
    [SND_DEVICE_IN_VOICE_REC_HEADSET_MIC] = "headset-mic",

    [SND_DEVICE_IN_UNPROCESSED_MIC] = "unprocessed-mic",
    [SND_DEVICE_IN_UNPROCESSED_STEREO_MIC] = "unprocessed-stereo-mic",
    [SND_DEVICE_IN_UNPROCESSED_THREE_MIC] = "unprocessed-three-mic",
    [SND_DEVICE_IN_UNPROCESSED_QUAD_MIC] = "unprocessed-quad-mic",
    [SND_DEVICE_IN_UNPROCESSED_HEADSET_MIC] = "unprocessed-headset-mic",

    [SND_DEVICE_IN_VOICE_RX] = "voice-rx",

    [SND_DEVICE_IN_THREE_MIC] = "three-mic",
    [SND_DEVICE_IN_QUAD_MIC] = "quad-mic",
    [SND_DEVICE_IN_CAPTURE_VI_FEEDBACK] = "vi-feedback",
    [SND_DEVICE_IN_HANDSET_TMIC] = "three-mic",
    [SND_DEVICE_IN_HANDSET_QMIC] = "quad-mic",
    [SND_DEVICE_IN_HANDSET_TMIC_AEC] = "three-mic",
    [SND_DEVICE_IN_HANDSET_QMIC_AEC] = "quad-mic",
};

/* ACDB IDs (audio DSP path configuration IDs) for each sound device */
static int acdb_device_table[SND_DEVICE_MAX] = {
    [SND_DEVICE_NONE] = -1,
    [SND_DEVICE_OUT_HANDSET] = 7,
    [SND_DEVICE_OUT_SPEAKER] = 15,
    [SND_DEVICE_OUT_SPEAKER_REVERSE] = 15,
    [SND_DEVICE_OUT_SPEAKER_SAFE] = 15,
    [SND_DEVICE_OUT_HEADPHONES] = 10,
    [SND_DEVICE_OUT_LINE] = 77,
    [SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES] = 10,
    [SND_DEVICE_OUT_SPEAKER_SAFE_AND_HEADPHONES] = 10,
    [SND_DEVICE_OUT_SPEAKER_AND_LINE] = 77,
    [SND_DEVICE_OUT_SPEAKER_SAFE_AND_LINE] = 77,
    [SND_DEVICE_OUT_VOICE_HANDSET] = ACDB_ID_VOICE_HANDSET,
    [SND_DEVICE_OUT_VOICE_SPEAKER] = ACDB_ID_VOICE_SPEAKER,
    [SND_DEVICE_OUT_VOICE_HAC_HANDSET] = 53,
    [SND_DEVICE_OUT_VOICE_HEADPHONES] = 10,
    [SND_DEVICE_OUT_VOICE_LINE] = 77,
    [SND_DEVICE_OUT_HDMI] = 18,
    [SND_DEVICE_OUT_SPEAKER_AND_HDMI] = 15,
    [SND_DEVICE_OUT_BT_SCO] = 22,
    [SND_DEVICE_OUT_BT_SCO_WB] = 39,
    [SND_DEVICE_OUT_VOICE_HANDSET_TMUS] = ACDB_ID_VOICE_HANDSET_TMUS,
    [SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES] = 17,
    [SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES] = 17,
    [SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET] = 37,
    [SND_DEVICE_OUT_VOICE_TX] = 45,
    [SND_DEVICE_OUT_SPEAKER_PROTECTED] = 124,
    [SND_DEVICE_OUT_VOICE_SPEAKER_PROTECTED] = 101,
    [SND_DEVICE_OUT_VOICE_SPEAKER_HFP] = ACDB_ID_VOICE_SPEAKER,

    [SND_DEVICE_IN_HANDSET_MIC] = 4,
    [SND_DEVICE_IN_HANDSET_MIC_AEC] = 106,
    [SND_DEVICE_IN_HANDSET_MIC_NS] = 107,
    [SND_DEVICE_IN_HANDSET_MIC_AEC_NS] = 108,
    [SND_DEVICE_IN_HANDSET_DMIC] = 41,
    [SND_DEVICE_IN_HANDSET_DMIC_AEC] = 109,
    [SND_DEVICE_IN_HANDSET_DMIC_NS] = 110,
    [SND_DEVICE_IN_HANDSET_DMIC_AEC_NS] = 111,
    [SND_DEVICE_IN_HANDSET_DMIC_STEREO] = 34,

    [SND_DEVICE_IN_SPEAKER_MIC] = 11,
    [SND_DEVICE_IN_SPEAKER_MIC_AEC] = 112,
    [SND_DEVICE_IN_SPEAKER_MIC_NS] = 113,
    [SND_DEVICE_IN_SPEAKER_MIC_AEC_NS] = 114,
    [SND_DEVICE_IN_SPEAKER_DMIC] = 43,
    [SND_DEVICE_IN_SPEAKER_DMIC_AEC] = 115,
    [SND_DEVICE_IN_SPEAKER_DMIC_NS] = 116,
    [SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS] = 117,
    [SND_DEVICE_IN_SPEAKER_DMIC_STEREO] = 35,

    [SND_DEVICE_IN_HEADSET_MIC] = ACDB_ID_HEADSET_MIC_AEC,
    [SND_DEVICE_IN_HEADSET_MIC_AEC] = ACDB_ID_HEADSET_MIC_AEC,

    [SND_DEVICE_IN_HDMI_MIC] = 4,
    [SND_DEVICE_IN_BT_SCO_MIC] = 21,
    [SND_DEVICE_IN_BT_SCO_MIC_NREC] = 21,
    [SND_DEVICE_IN_BT_SCO_MIC_WB] = 38,
    [SND_DEVICE_IN_BT_SCO_MIC_WB_NREC] = 38,
    [SND_DEVICE_IN_CAMCORDER_MIC] = 61,

    [SND_DEVICE_IN_VOICE_DMIC] = 41,
    [SND_DEVICE_IN_VOICE_DMIC_TMUS] = ACDB_ID_VOICE_DMIC_EF_TMUS,
    [SND_DEVICE_IN_VOICE_SPEAKER_MIC] = 11,
    [SND_DEVICE_IN_VOICE_SPEAKER_MIC_HFP] = 11,
    [SND_DEVICE_IN_VOICE_SPEAKER_DMIC] = 43,
    [SND_DEVICE_IN_VOICE_HEADSET_MIC] = ACDB_ID_HEADSET_MIC_AEC,
    [SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC] = 16,
    [SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC] = 36,
    [SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC] = 16,

    [SND_DEVICE_IN_VOICE_REC_MIC] = ACDB_ID_VOICE_REC_MIC,
    [SND_DEVICE_IN_VOICE_REC_MIC_NS] = 113,
    [SND_DEVICE_IN_VOICE_REC_MIC_AEC] = 112,
    [SND_DEVICE_IN_VOICE_REC_DMIC_STEREO] = 35,
    [SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE] = 43,
    [SND_DEVICE_IN_VOICE_REC_HEADSET_MIC] = ACDB_ID_HEADSET_MIC_AEC,

    [SND_DEVICE_IN_UNPROCESSED_MIC] = ACDB_ID_VOICE_REC_MIC,
    [SND_DEVICE_IN_UNPROCESSED_HEADSET_MIC] = ACDB_ID_HEADSET_MIC_AEC,
    [SND_DEVICE_IN_UNPROCESSED_STEREO_MIC] = 35,
    [SND_DEVICE_IN_UNPROCESSED_THREE_MIC] = 125,
    [SND_DEVICE_IN_UNPROCESSED_QUAD_MIC] = 125,

    [SND_DEVICE_IN_VOICE_RX] = 44,

    [SND_DEVICE_IN_THREE_MIC] = 46,
    [SND_DEVICE_IN_QUAD_MIC] = 46,
    [SND_DEVICE_IN_CAPTURE_VI_FEEDBACK] = 102,
    [SND_DEVICE_IN_HANDSET_TMIC] = 125,
    [SND_DEVICE_IN_HANDSET_QMIC] = 125,
    [SND_DEVICE_IN_HANDSET_TMIC_AEC] = 125, /* override this for new target to 140 */
    [SND_DEVICE_IN_HANDSET_QMIC_AEC] = 125, /* override this for new target to 140 */
};

struct name_to_index {
    char name[100];
    unsigned int index;
};

#define TO_NAME_INDEX(X)   #X, X

/* Used to get index from parsed string */
static const struct name_to_index snd_device_name_index[SND_DEVICE_MAX] = {
    /* out */
    {TO_NAME_INDEX(SND_DEVICE_OUT_HANDSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_REVERSE)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_SAFE)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_HEADPHONES)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_LINE)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_SAFE_AND_HEADPHONES)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_AND_LINE)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_SAFE_AND_LINE)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_HANDSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_SPEAKER)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_SPEAKER_HFP)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_HEADPHONES)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_LINE)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_HDMI)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_AND_HDMI)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_BT_SCO)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_BT_SCO_WB)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_HANDSET_TMUS)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_HAC_HANDSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET)},

    /* in */
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_PROTECTED)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_SPEAKER_PROTECTED)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_MIC_AEC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_MIC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_MIC_AEC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_DMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_DMIC_AEC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_DMIC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_DMIC_AEC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_DMIC_STEREO)},

    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_MIC_AEC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_MIC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_MIC_AEC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_DMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_DMIC_AEC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_DMIC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_DMIC_STEREO)},

    {TO_NAME_INDEX(SND_DEVICE_IN_HEADSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HEADSET_MIC_AEC)},

    {TO_NAME_INDEX(SND_DEVICE_IN_HDMI_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_BT_SCO_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_BT_SCO_MIC_NREC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_BT_SCO_MIC_WB)},
    {TO_NAME_INDEX(SND_DEVICE_IN_BT_SCO_MIC_WB_NREC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_CAMCORDER_MIC)},

    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_DMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_DMIC_TMUS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_SPEAKER_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_SPEAKER_MIC_HFP)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_SPEAKER_DMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_HEADSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC)},

    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_REC_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_REC_MIC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_REC_MIC_AEC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_REC_DMIC_STEREO)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_REC_HEADSET_MIC)},

    {TO_NAME_INDEX(SND_DEVICE_IN_UNPROCESSED_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_UNPROCESSED_HEADSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_UNPROCESSED_STEREO_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_UNPROCESSED_THREE_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_UNPROCESSED_QUAD_MIC)},

    {TO_NAME_INDEX(SND_DEVICE_IN_THREE_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_QUAD_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_CAPTURE_VI_FEEDBACK)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_TMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_QMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_TMIC_AEC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_QMIC_AEC)},
};

static char * backend_tag_table[SND_DEVICE_MAX] = {0};
static char * hw_interface_table[SND_DEVICE_MAX] = {0};

static const struct name_to_index usecase_name_index[AUDIO_USECASE_MAX] = {
    {TO_NAME_INDEX(USECASE_AUDIO_PLAYBACK_DEEP_BUFFER)},
    {TO_NAME_INDEX(USECASE_AUDIO_PLAYBACK_LOW_LATENCY)},
    {TO_NAME_INDEX(USECASE_AUDIO_PLAYBACK_MULTI_CH)},
    {TO_NAME_INDEX(USECASE_AUDIO_PLAYBACK_OFFLOAD)},
    {TO_NAME_INDEX(USECASE_AUDIO_PLAYBACK_TTS)},
    {TO_NAME_INDEX(USECASE_AUDIO_PLAYBACK_ULL)},
    {TO_NAME_INDEX(USECASE_AUDIO_RECORD)},
    {TO_NAME_INDEX(USECASE_AUDIO_RECORD_LOW_LATENCY)},
    {TO_NAME_INDEX(USECASE_VOICE_CALL)},
    {TO_NAME_INDEX(USECASE_VOICE2_CALL)},
    {TO_NAME_INDEX(USECASE_VOLTE_CALL)},
    {TO_NAME_INDEX(USECASE_QCHAT_CALL)},
    {TO_NAME_INDEX(USECASE_VOWLAN_CALL)},
    {TO_NAME_INDEX(USECASE_INCALL_REC_UPLINK)},
    {TO_NAME_INDEX(USECASE_INCALL_REC_DOWNLINK)},
    {TO_NAME_INDEX(USECASE_INCALL_REC_UPLINK_AND_DOWNLINK)},
    {TO_NAME_INDEX(USECASE_AUDIO_HFP_SCO)},
};

#define DEEP_BUFFER_PLATFORM_DELAY (29*1000LL)
#define LOW_LATENCY_PLATFORM_DELAY (13*1000LL)

static pthread_once_t check_op_once_ctl = PTHREAD_ONCE_INIT;
static bool is_tmus = false;

static void check_operator()
{
    char value[PROPERTY_VALUE_MAX];
    int mccmnc;
    property_get("gsm.sim.operator.numeric",value,"0");
    mccmnc = atoi(value);
    ALOGD("%s: tmus mccmnc %d", __func__, mccmnc);
    switch(mccmnc) {
    /* TMUS MCC(310), MNC(490, 260, 026) */
    case 310490:
    case 310260:
    case 310026:
    /* Add new TMUS MNC(800, 660, 580, 310, 270, 250, 240, 230, 220, 210, 200, 160) */
    case 310800:
    case 310660:
    case 310580:
    case 310310:
    case 310270:
    case 310250:
    case 310240:
    case 310230:
    case 310220:
    case 310210:
    case 310200:
    case 310160:
        is_tmus = true;
        break;
    }
}

bool is_operator_tmus()
{
    pthread_once(&check_op_once_ctl, check_operator);
    return is_tmus;
}

static char *get_current_operator()
{
    struct listnode *node;
    struct operator_info *info_item;
    char mccmnc[PROPERTY_VALUE_MAX];
    char *ret = NULL;

    property_get("gsm.sim.operator.numeric",mccmnc,"0");

    list_for_each(node, &operator_info_list) {
        info_item = node_to_item(node, struct operator_info, list);
        if (strstr(info_item->mccmnc, mccmnc) != NULL) {
            ret = info_item->name;
        }
    }

    return ret;
}

static struct operator_specific_device *get_operator_specific_device(snd_device_t snd_device)
{
    struct listnode *node;
    struct operator_specific_device *ret = NULL;
    struct operator_specific_device *device_item;
    char *operator_name;

    operator_name = get_current_operator();
    if (operator_name == NULL)
        return ret;

    list_for_each(node, operator_specific_device_table[snd_device]) {
        device_item = node_to_item(node, struct operator_specific_device, list);
        if (strcmp(operator_name, device_item->operator) == 0) {
            ret = device_item;
        }
    }

    return ret;
}


static int get_operator_specific_device_acdb_id(snd_device_t snd_device)
{
    struct operator_specific_device *device;
    int ret = acdb_device_table[snd_device];

    device = get_operator_specific_device(snd_device);
    if (device != NULL)
        ret = device->acdb_id;

    return ret;
}

static const char *get_operator_specific_device_mixer_path(snd_device_t snd_device)
{
    struct operator_specific_device *device;
    const char *ret = device_table[snd_device];

    device = get_operator_specific_device(snd_device);
    if (device != NULL)
        ret = device->mixer_path;

    return ret;
}

bool platform_send_gain_dep_cal(void *platform, int level)
{
    bool ret_val = false;
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;
    int acdb_dev_id, app_type;
    int acdb_dev_type = MSM_SNDDEV_CAP_RX;
    int mode = CAL_MODE_RTAC;
    struct listnode *node;
    struct audio_usecase *usecase;

    if (my_data->acdb_send_gain_dep_cal == NULL) {
        ALOGE("%s: dlsym error for acdb_send_gain_dep_cal", __func__);
        return ret_val;
    }

    if (!voice_is_in_call(adev)) {
        ALOGV("%s: Not Voice call usecase, apply new cal for level %d",
               __func__, level);
        app_type = DEFAULT_APP_TYPE_RX_PATH;

        // find the current active sound device
        list_for_each(node, &adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);

            if (usecase != NULL &&
                usecase->type == PCM_PLAYBACK &&
                (usecase->stream.out->devices == AUDIO_DEVICE_OUT_SPEAKER)) {

                ALOGV("%s: out device is %d", __func__,  usecase->out_snd_device);
                if (audio_extn_spkr_prot_is_enabled()) {
                    acdb_dev_id = audio_extn_spkr_prot_get_acdb_id(usecase->out_snd_device);
                } else {
                    acdb_dev_id = acdb_device_table[usecase->out_snd_device];
                }

                if (!my_data->acdb_send_gain_dep_cal(acdb_dev_id, app_type,
                                                     acdb_dev_type, mode, level)) {
                    // set ret_val true if at least one calibration is set successfully
                    ret_val = true;
                } else {
                    ALOGE("%s: my_data->acdb_send_gain_dep_cal failed ", __func__);
                }
            } else {
                ALOGW("%s: Usecase list is empty", __func__);
            }
        }
    } else {
        ALOGW("%s: Voice call in progress .. ignore setting new cal",
              __func__);
    }
    return ret_val;
}

void platform_set_echo_reference(struct audio_device *adev, bool enable, audio_devices_t out_device)
{
    struct platform_data *my_data = (struct platform_data *)adev->platform;
    snd_device_t snd_device = SND_DEVICE_NONE;

    if (strcmp(my_data->ec_ref_mixer_path, "")) {
        ALOGV("%s: diabling %s", __func__, my_data->ec_ref_mixer_path);
        audio_route_reset_and_update_path(adev->audio_route, my_data->ec_ref_mixer_path);
    }

    if (enable) {
        strcpy(my_data->ec_ref_mixer_path, "echo-reference");
        if (out_device != AUDIO_DEVICE_NONE) {
            snd_device = platform_get_output_snd_device(adev->platform, out_device);
            platform_add_backend_name(adev->platform, my_data->ec_ref_mixer_path, snd_device);
        }

        ALOGV("%s: enabling %s", __func__, my_data->ec_ref_mixer_path);
        audio_route_apply_and_update_path(adev->audio_route, my_data->ec_ref_mixer_path);
    }
}

static struct csd_data *open_csd_client(bool i2s_ext_modem)
{
    struct csd_data *csd = calloc(1, sizeof(struct csd_data));

    csd->csd_client = dlopen(LIB_CSD_CLIENT, RTLD_NOW);
    if (csd->csd_client == NULL) {
        ALOGE("%s: DLOPEN failed for %s", __func__, LIB_CSD_CLIENT);
        goto error;
    } else {
        ALOGV("%s: DLOPEN successful for %s", __func__, LIB_CSD_CLIENT);

        csd->deinit = (deinit_t)dlsym(csd->csd_client,
                                             "csd_client_deinit");
        if (csd->deinit == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_deinit", __func__,
                  dlerror());
            goto error;
        }
        csd->disable_device = (disable_device_t)dlsym(csd->csd_client,
                                             "csd_client_disable_device");
        if (csd->disable_device == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_disable_device",
                  __func__, dlerror());
            goto error;
        }
        csd->enable_device_config = (enable_device_config_t)dlsym(csd->csd_client,
                                               "csd_client_enable_device_config");
        if (csd->enable_device_config == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_enable_device_config",
                  __func__, dlerror());
            goto error;
        }
        csd->enable_device = (enable_device_t)dlsym(csd->csd_client,
                                             "csd_client_enable_device");
        if (csd->enable_device == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_enable_device",
                  __func__, dlerror());
            goto error;
        }
        csd->start_voice = (start_voice_t)dlsym(csd->csd_client,
                                             "csd_client_start_voice");
        if (csd->start_voice == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_start_voice",
                  __func__, dlerror());
            goto error;
        }
        csd->stop_voice = (stop_voice_t)dlsym(csd->csd_client,
                                             "csd_client_stop_voice");
        if (csd->stop_voice == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_stop_voice",
                  __func__, dlerror());
            goto error;
        }
        csd->volume = (volume_t)dlsym(csd->csd_client,
                                             "csd_client_volume");
        if (csd->volume == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_volume",
                  __func__, dlerror());
            goto error;
        }
        csd->mic_mute = (mic_mute_t)dlsym(csd->csd_client,
                                             "csd_client_mic_mute");
        if (csd->mic_mute == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_mic_mute",
                  __func__, dlerror());
            goto error;
        }
        csd->slow_talk = (slow_talk_t)dlsym(csd->csd_client,
                                             "csd_client_slow_talk");
        if (csd->slow_talk == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_slow_talk",
                  __func__, dlerror());
            goto error;
        }
        csd->start_playback = (start_playback_t)dlsym(csd->csd_client,
                                             "csd_client_start_playback");
        if (csd->start_playback == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_start_playback",
                  __func__, dlerror());
            goto error;
        }
        csd->stop_playback = (stop_playback_t)dlsym(csd->csd_client,
                                             "csd_client_stop_playback");
        if (csd->stop_playback == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_stop_playback",
                  __func__, dlerror());
            goto error;
        }
        csd->start_record = (start_record_t)dlsym(csd->csd_client,
                                             "csd_client_start_record");
        if (csd->start_record == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_start_record",
                  __func__, dlerror());
            goto error;
        }
        csd->stop_record = (stop_record_t)dlsym(csd->csd_client,
                                             "csd_client_stop_record");
        if (csd->stop_record == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_stop_record",
                  __func__, dlerror());
            goto error;
        }

        csd->get_sample_rate = (get_sample_rate_t)dlsym(csd->csd_client,
                                             "csd_client_get_sample_rate");
        if (csd->get_sample_rate == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_get_sample_rate",
                  __func__, dlerror());

            goto error;
        }

        csd->init = (init_t)dlsym(csd->csd_client, "csd_client_init");

        if (csd->init == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_init",
                  __func__, dlerror());
            goto error;
        } else {
            csd->init(i2s_ext_modem);
        }
    }
    return csd;

error:
    free(csd);
    csd = NULL;
    return csd;
}

void close_csd_client(struct csd_data *csd)
{
    if (csd != NULL) {
        csd->deinit();
        dlclose(csd->csd_client);
        free(csd);
        csd = NULL;
    }
}

static void platform_csd_init(struct platform_data *my_data)
{
#ifdef PLATFORM_MSM8084
    int32_t modems, (*count_modems)(void);
    const char *name = "libdetectmodem.so";
    const char *func = "count_modems";
    const char *error;

    my_data->csd = NULL;

    void *lib = dlopen(name, RTLD_NOW);
    error = dlerror();
    if (!lib) {
        ALOGE("%s: could not find %s: %s", __func__, name, error);
        return;
    }

    count_modems = NULL;
    *(void **)(&count_modems) = dlsym(lib, func);
    error = dlerror();
    if (!count_modems) {
        ALOGE("%s: could not find symbol %s in %s: %s",
              __func__, func, name, error);
        goto done;
    }

    modems = count_modems();
    if (modems < 0) {
        ALOGE("%s: count_modems failed\n", __func__);
        goto done;
    }

    ALOGD("%s: num_modems %d\n", __func__, modems);
    if (modems > 0)
        my_data->csd = open_csd_client(false /*is_i2s_ext_modem*/);

done:
    dlclose(lib);
#else
     my_data->csd = NULL;
#endif
}

static void set_platform_defaults(struct platform_data * my_data)
{
    int32_t dev;
    for (dev = 0; dev < SND_DEVICE_MAX; dev++) {
        backend_tag_table[dev] = NULL;
        hw_interface_table[dev] = NULL;
        operator_specific_device_table[dev] = NULL;
    }

    // To overwrite these go to the audio_platform_info.xml file.
    backend_tag_table[SND_DEVICE_IN_BT_SCO_MIC] = strdup("bt-sco");
    backend_tag_table[SND_DEVICE_IN_BT_SCO_MIC_NREC] = strdup("bt-sco");
    backend_tag_table[SND_DEVICE_OUT_BT_SCO] = strdup("bt-sco");
    backend_tag_table[SND_DEVICE_OUT_HDMI] = strdup("hdmi");
    backend_tag_table[SND_DEVICE_OUT_SPEAKER_AND_HDMI] = strdup("speaker-and-hdmi");
    backend_tag_table[SND_DEVICE_OUT_BT_SCO_WB] = strdup("bt-sco-wb");
    backend_tag_table[SND_DEVICE_IN_BT_SCO_MIC_WB] = strdup("bt-sco-wb");
    backend_tag_table[SND_DEVICE_IN_BT_SCO_MIC_WB_NREC] = strdup("bt-sco-wb");
    backend_tag_table[SND_DEVICE_OUT_VOICE_TX] = strdup("afe-proxy");
    backend_tag_table[SND_DEVICE_IN_VOICE_RX] = strdup("afe-proxy");

    hw_interface_table[SND_DEVICE_OUT_HANDSET] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_SPEAKER] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_SPEAKER_REVERSE] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_SPEAKER_SAFE] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_HEADPHONES] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_LINE] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_SPEAKER_SAFE_AND_HEADPHONES] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_SPEAKER_AND_LINE] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_SPEAKER_SAFE_AND_LINE] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_VOICE_HANDSET] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_VOICE_HAC_HANDSET] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_VOICE_SPEAKER] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_VOICE_HEADPHONES] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_VOICE_LINE] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_HDMI] = strdup("HDMI_RX");
    hw_interface_table[SND_DEVICE_OUT_SPEAKER_AND_HDMI] = strdup("SLIMBUS_0_RX-and-HDMI_RX");
    hw_interface_table[SND_DEVICE_OUT_BT_SCO] = strdup("SEC_AUX_PCM_RX");
    hw_interface_table[SND_DEVICE_OUT_BT_SCO_WB] = strdup("SEC_AUX_PCM_RX");
    hw_interface_table[SND_DEVICE_OUT_VOICE_HANDSET_TMUS] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_VOICE_TX] = strdup("AFE_PCM_RX");
    hw_interface_table[SND_DEVICE_OUT_SPEAKER_PROTECTED] = strdup("SLIMBUS_0_RX");
    hw_interface_table[SND_DEVICE_OUT_VOICE_SPEAKER_PROTECTED] = strdup("SLIMBUS_0_RX");

    my_data->max_mic_count = PLATFORM_DEFAULT_MIC_COUNT;
}

void get_cvd_version(char *cvd_version, struct audio_device *adev)
{
    struct mixer_ctl *ctl;
    int count;
    int ret = 0;

    ctl = mixer_get_ctl_by_name(adev->mixer, CVD_VERSION_MIXER_CTL);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",  __func__, CVD_VERSION_MIXER_CTL);
        goto done;
    }
    mixer_ctl_update(ctl);

    count = mixer_ctl_get_num_values(ctl);
    if (count > MAX_CVD_VERSION_STRING_SIZE)
        count = MAX_CVD_VERSION_STRING_SIZE - 1;

    ret = mixer_ctl_get_array(ctl, cvd_version, count);
    if (ret != 0) {
        ALOGE("%s: ERROR! mixer_ctl_get_array() failed to get CVD Version", __func__);
        goto done;
    }

done:
    return;
}

void *platform_init(struct audio_device *adev)
{
    char value[PROPERTY_VALUE_MAX];
    struct platform_data *my_data = NULL;
    int retry_num = 0, snd_card_num = 0, key = 0, ret = 0;
    bool dual_mic_config = false, use_default_mixer_path = true;
    const char *snd_card_name;
    char *cvd_version = NULL;
    char *snd_internal_name = NULL;
    char *tmp = NULL;
    char mixer_xml_file[MIXER_PATH_MAX_LENGTH]= {0};
    char platform_info_file[MIXER_PATH_MAX_LENGTH]= {0};
    struct snd_card_split *snd_split_handle = NULL;
    my_data = calloc(1, sizeof(struct platform_data));

    my_data->adev = adev;

    list_init(&operator_info_list);

    set_platform_defaults(my_data);

    while (snd_card_num < MAX_SND_CARD) {
        adev->mixer = mixer_open(snd_card_num);

        while (!adev->mixer && retry_num < RETRY_NUMBER) {
            usleep(RETRY_US);
            adev->mixer = mixer_open(snd_card_num);
            retry_num++;
        }

        if (!adev->mixer) {
            ALOGE("%s: Unable to open the mixer card: %d", __func__,
                   snd_card_num);
            retry_num = 0;
            snd_card_num++;
            continue;
        }

        snd_card_name = mixer_get_name(adev->mixer);
        my_data->hw_info = hw_info_init(snd_card_name);

        audio_extn_set_snd_card_split(snd_card_name);
        snd_split_handle = audio_extn_get_snd_card_split();

        /* Get the codec internal name from the sound card and/or form factor
         * name and form the mixer paths and platfor info file name dynamically.
         * This is generic way of picking any codec and forma factor name based
         * mixer and platform info files in future with no code change.

         * current code extends and looks for any of the exteneded mixer path and
         * platform info file present based on codec and form factor.

         * order of picking appropriate file is
         * <i>   mixer_paths_<codec_name>_<form_factor>.xml, if file not present
         * <ii>  mixer_paths_<codec_name>.xml, if file not present
         * <iii> mixer_paths.xml

         * same order is followed for audio_platform_info.xml as well
         */

        // need to carryforward old file name
        if (!strncmp(snd_card_name, TOMTOM_8226_SND_CARD_NAME,
                     min(strlen(TOMTOM_8226_SND_CARD_NAME), strlen(snd_card_name)))) {
            snprintf(mixer_xml_file, sizeof(mixer_xml_file), "%s_%s.xml",
                             MIXER_XML_BASE_STRING, TOMTOM_MIXER_FILE_SUFFIX );
        } else {

            snprintf(mixer_xml_file, sizeof(mixer_xml_file), "%s_%s_%s.xml",
                             MIXER_XML_BASE_STRING, snd_split_handle->snd_card,
                             snd_split_handle->form_factor);

            if (F_OK != access(mixer_xml_file, 0)) {
                memset(mixer_xml_file, 0, sizeof(mixer_xml_file));
                snprintf(mixer_xml_file, sizeof(mixer_xml_file), "%s_%s.xml",
                             MIXER_XML_BASE_STRING, snd_split_handle->snd_card);

                if (F_OK != access(mixer_xml_file, 0)) {
                    memset(mixer_xml_file, 0, sizeof(mixer_xml_file));
                    strlcpy(mixer_xml_file, MIXER_XML_DEFAULT_PATH, MIXER_PATH_MAX_LENGTH);
                }
            }

            snprintf(platform_info_file, sizeof(platform_info_file), "%s_%s_%s.xml",
                             PLATFORM_INFO_XML_BASE_STRING, snd_split_handle->snd_card,
                             snd_split_handle->form_factor);

            if (F_OK != access(platform_info_file, 0)) {
                memset(platform_info_file, 0, sizeof(platform_info_file));
                snprintf(platform_info_file, sizeof(platform_info_file), "%s_%s.xml",
                             PLATFORM_INFO_XML_BASE_STRING, snd_split_handle->snd_card);

                if (F_OK != access(platform_info_file, 0)) {
                    memset(platform_info_file, 0, sizeof(platform_info_file));
                    strlcpy(platform_info_file, PLATFORM_INFO_XML_PATH, MIXER_PATH_MAX_LENGTH);
                }
            }
        }

        /* Initialize platform specific ids and/or backends*/
        platform_info_init(platform_info_file, my_data);

        /* validate the sound card name
         * my_data->snd_card_name can contain
         *     <a> complete sound card name, i.e. <device>-<codec>-<form_factor>-snd-card
         *         example: msm8994-tomtom-mtp-snd-card
         *     <b> or sub string of the card name, i.e. <device>-<codec>
         *         example: msm8994-tomtom
         * snd_card_name is truncated to 32 charaters as per mixer_get_name() implementation
         * so use min of my_data->snd_card_name and snd_card_name length for comparison
         */

        if (my_data->snd_card_name != NULL &&
                strncmp(snd_card_name, my_data->snd_card_name,
                        min(strlen(snd_card_name), strlen(my_data->snd_card_name))) != 0) {
            ALOGI("%s: found valid sound card %s, but not primary sound card %s",
                   __func__, snd_card_name, my_data->snd_card_name);
            retry_num = 0;
            snd_card_num++;
            hw_info_deinit(my_data->hw_info);
            my_data->hw_info = NULL;
            continue;
        }
        ALOGI("%s: found sound card %s, primary sound card expeted is %s",
              __func__, snd_card_name, my_data->snd_card_name);

        ALOGD("%s: Loading mixer file: %s", __func__, mixer_xml_file);
        adev->audio_route = audio_route_init(snd_card_num, mixer_xml_file);

        if (!adev->audio_route) {
            ALOGE("%s: Failed to init audio route controls, aborting.", __func__);
            goto init_failed;
        }
        adev->snd_card = snd_card_num;
        ALOGD("%s: Opened sound card:%d", __func__, snd_card_num);
        break;
    }

    if (snd_card_num >= MAX_SND_CARD) {
        ALOGE("%s: Unable to find correct sound card, aborting.", __func__);
        goto init_failed;
    }

    //set max volume step for voice call
    property_get("ro.config.vc_call_vol_steps", value, TOSTRING(MAX_VOL_INDEX));
    my_data->max_vol_index = atoi(value);

    property_get("persist.audio.dualmic.config",value,"");
    if (!strcmp("endfire", value)) {
        dual_mic_config = true;
    }

    my_data->source_mic_type = SOURCE_DUAL_MIC;

    my_data->fluence_in_spkr_mode = false;
    my_data->fluence_in_voice_call = false;
    my_data->fluence_in_voice_comm = false;
    my_data->fluence_in_voice_rec = false;

    property_get("ro.qc.sdk.audio.fluencetype", value, "none");
    if (!strcmp("fluencepro", value)) {
        my_data->fluence_type = FLUENCE_PRO_ENABLE;
    } else if (!strcmp("fluence", value) || (dual_mic_config)) {
        my_data->fluence_type = FLUENCE_ENABLE;
    } else if (!strcmp("none", value)) {
        my_data->fluence_type = FLUENCE_DISABLE;
    }

    if (my_data->fluence_type != FLUENCE_DISABLE) {
        property_get("persist.audio.fluence.voicecall",value,"");
        if (!strcmp("true", value)) {
            my_data->fluence_in_voice_call = true;
        }

        property_get("persist.audio.fluence.voicecomm",value,"");
        if (!strcmp("true", value)) {
            my_data->fluence_in_voice_comm = true;
        }

        property_get("persist.audio.fluence.voicerec",value,"");
        if (!strcmp("true", value)) {
            my_data->fluence_in_voice_rec = true;
        }

        property_get("persist.audio.fluence.speaker",value,"");
        if (!strcmp("true", value)) {
            my_data->fluence_in_spkr_mode = true;
        }
    }

    // support max to mono, example if max count is 3, usecase supports Three, dual and mono mic
    switch (my_data->max_mic_count) {
        case 4:
            my_data->source_mic_type |= SOURCE_QUAD_MIC;
        case 3:
            my_data->source_mic_type |= SOURCE_THREE_MIC;
        case 2:
            my_data->source_mic_type |= SOURCE_DUAL_MIC;
        case 1:
            my_data->source_mic_type |= SOURCE_MONO_MIC;
            break;
        default:
            ALOGE("%s: max_mic_count (%d), is not supported, setting to default",
                   __func__, my_data->max_mic_count);
            my_data->source_mic_type = SOURCE_MONO_MIC|SOURCE_DUAL_MIC;
            break;
        }

    ALOGV("%s: Fluence_Type(%d) max_mic_count(%d) mic_type(0x%x) fluence_in_voice_call(%d)"
          " fluence_in_voice_comm(%d) fluence_in_voice_rec(%d) fluence_in_spkr_mode(%d) ",
          __func__, my_data->fluence_type, my_data->max_mic_count, my_data->source_mic_type,
          my_data->fluence_in_voice_call, my_data->fluence_in_voice_comm,
          my_data->fluence_in_voice_rec, my_data->fluence_in_spkr_mode);

    my_data->acdb_handle = dlopen(LIB_ACDB_LOADER, RTLD_NOW);
    if (my_data->acdb_handle == NULL) {
        ALOGE("%s: DLOPEN failed for %s", __func__, LIB_ACDB_LOADER);
    } else {
        ALOGV("%s: DLOPEN successful for %s", __func__, LIB_ACDB_LOADER);
        my_data->acdb_deallocate = (acdb_deallocate_t)dlsym(my_data->acdb_handle,
                                                    "acdb_loader_deallocate_ACDB");
        if (!my_data->acdb_deallocate)
            ALOGE("%s: Could not find the symbol acdb_loader_deallocate_ACDB from %s",
                  __func__, LIB_ACDB_LOADER);

        my_data->acdb_send_audio_cal = (acdb_send_audio_cal_t)dlsym(my_data->acdb_handle,
                                                    "acdb_loader_send_audio_cal");
        if (!my_data->acdb_send_audio_cal)
            ALOGE("%s: Could not find the symbol acdb_send_audio_cal from %s",
                  __func__, LIB_ACDB_LOADER);

        my_data->acdb_send_voice_cal = (acdb_send_voice_cal_t)dlsym(my_data->acdb_handle,
                                                    "acdb_loader_send_voice_cal");
        if (!my_data->acdb_send_voice_cal)
            ALOGE("%s: Could not find the symbol acdb_loader_send_voice_cal from %s",
                  __func__, LIB_ACDB_LOADER);

        my_data->acdb_reload_vocvoltable = (acdb_reload_vocvoltable_t)dlsym(my_data->acdb_handle,
                                                    "acdb_loader_reload_vocvoltable");
        if (!my_data->acdb_reload_vocvoltable)
            ALOGE("%s: Could not find the symbol acdb_loader_reload_vocvoltable from %s",
                  __func__, LIB_ACDB_LOADER);

        my_data->acdb_send_gain_dep_cal = (acdb_send_gain_dep_cal_t)dlsym(my_data->acdb_handle,
                                                    "acdb_loader_send_gain_dep_cal");
        if (!my_data->acdb_send_gain_dep_cal)
            ALOGV("%s: Could not find the symbol acdb_loader_send_gain_dep_cal from %s",
                  __func__, LIB_ACDB_LOADER);

#if defined (PLATFORM_MSM8994) || (PLATFORM_MSM8996)
        acdb_init_v2_cvd_t acdb_init;
        acdb_init = (acdb_init_v2_cvd_t)dlsym(my_data->acdb_handle,
                                              "acdb_loader_init_v2");
        if (acdb_init == NULL) {
            ALOGE("%s: dlsym error %s for acdb_loader_init_v2", __func__, dlerror());
            goto acdb_init_fail;
        }

        cvd_version = calloc(1, MAX_CVD_VERSION_STRING_SIZE);
        get_cvd_version(cvd_version, adev);
        if (!cvd_version)
            ALOGE("failed to allocate cvd_version");
        else
            acdb_init((char *)snd_card_name, cvd_version, 0);
        free(cvd_version);
#elif defined (PLATFORM_MSM8084)
        acdb_init_v2_t acdb_init;
        acdb_init = (acdb_init_v2_t)dlsym(my_data->acdb_handle,
                                          "acdb_loader_init_v2");
        if (acdb_init == NULL) {
            ALOGE("%s: dlsym error %s for acdb_loader_init_v2", __func__, dlerror());
            goto acdb_init_fail;
        }
        acdb_init((char *)snd_card_name);
#else
        acdb_init_t acdb_init;
        acdb_init = (acdb_init_t)dlsym(my_data->acdb_handle,
                                                    "acdb_loader_init_ACDB");
        if (acdb_init == NULL)
            ALOGE("%s: dlsym error %s for acdb_loader_init_ACDB", __func__, dlerror());
        else
            acdb_init();
#endif
    }

acdb_init_fail:

    audio_extn_spkr_prot_init(adev);

    audio_extn_hwdep_cal_send(adev->snd_card, my_data->acdb_handle);

    /* load csd client */
    platform_csd_init(my_data);

    return my_data;

init_failed:
    if (my_data)
        free(my_data);
    return NULL;
}

void platform_deinit(void *platform)
{
    int32_t dev;
    struct operator_info *info_item;
    struct operator_specific_device *device_item;
    struct listnode *node;

    struct platform_data *my_data = (struct platform_data *)platform;
    close_csd_client(my_data->csd);

    hw_info_deinit(my_data->hw_info);

    for (dev = 0; dev < SND_DEVICE_MAX; dev++) {
        if (backend_tag_table[dev])
            free(backend_tag_table[dev]);
        if (hw_interface_table[dev])
            free(hw_interface_table[dev]);
        if (operator_specific_device_table[dev]) {
            while (!list_empty(operator_specific_device_table[dev])) {
                node = list_head(operator_specific_device_table[dev]);
                list_remove(node);
                device_item = node_to_item(node, struct operator_specific_device, list);
                free(device_item->operator);
                free(device_item->mixer_path);
                free(device_item);
            }
            free(operator_specific_device_table[dev]);
        }
    }

    if (my_data->snd_card_name)
        free(my_data->snd_card_name);

    while (!list_empty(&operator_info_list)) {
        node = list_head(&operator_info_list);
        list_remove(node);
        info_item = node_to_item(node, struct operator_info, list);
        free(info_item->name);
        free(info_item->mccmnc);
        free(info_item);
    }

    free(platform);
}

const char *platform_get_snd_device_name(snd_device_t snd_device)
{
    if (snd_device >= SND_DEVICE_MIN && snd_device < SND_DEVICE_MAX) {
        if (operator_specific_device_table[snd_device] != NULL) {
            return get_operator_specific_device_mixer_path(snd_device);
        }
        return device_table[snd_device];
    } else
        return "none";
}

int platform_get_snd_device_name_extn(void *platform, snd_device_t snd_device,
                                      char *device_name)
{
    struct platform_data *my_data = (struct platform_data *)platform;

    if (platform == NULL || device_name == NULL) {
        ALOGW("%s: something wrong, use legacy get_snd_device name", __func__);
        device_name = platform_get_snd_device_name(snd_device);
    } else if (snd_device >= SND_DEVICE_MIN && snd_device < SND_DEVICE_MAX) {
        if (operator_specific_device_table[snd_device] != NULL) {
            strlcpy(device_name, get_operator_specific_device_mixer_path(snd_device),
                    DEVICE_NAME_MAX_SIZE);
        } else {
            strlcpy(device_name, device_table[snd_device], DEVICE_NAME_MAX_SIZE);
        }
        hw_info_append_hw_type(my_data->hw_info, snd_device, device_name);
    } else {
        strlcpy(device_name, "none", DEVICE_NAME_MAX_SIZE);
    }

    return 0;
}

void platform_add_backend_name(void *platform, char *mixer_path,
                               snd_device_t snd_device)
{
    struct platform_data *my_data = (struct platform_data *)platform;

    if ((snd_device < SND_DEVICE_MIN) || (snd_device >= SND_DEVICE_MAX)) {
        ALOGE("%s: Invalid snd_device = %d", __func__, snd_device);
        return;
    }

    const char * suffix = backend_tag_table[snd_device];

    if (suffix != NULL) {
        strcat(mixer_path, " ");
        strcat(mixer_path, suffix);
    }
}

bool platform_check_backends_match(snd_device_t snd_device1, snd_device_t snd_device2)
{
    bool result = true;

    ALOGV("%s: snd_device1 = %s, snd_device2 = %s", __func__,
                platform_get_snd_device_name(snd_device1),
                platform_get_snd_device_name(snd_device2));

    if ((snd_device1 < SND_DEVICE_MIN) || (snd_device1 >= SND_DEVICE_MAX)) {
        ALOGE("%s: Invalid snd_device = %s", __func__,
                platform_get_snd_device_name(snd_device1));
        return false;
    }
    if ((snd_device2 < SND_DEVICE_MIN) || (snd_device2 >= SND_DEVICE_MAX)) {
        ALOGE("%s: Invalid snd_device = %s", __func__,
                platform_get_snd_device_name(snd_device2));
        return false;
    }
    const char * be_itf1 = hw_interface_table[snd_device1];
    const char * be_itf2 = hw_interface_table[snd_device2];

    if (NULL != be_itf1 && NULL != be_itf2) {
        if ((NULL == strstr(be_itf2, be_itf1)) && (NULL == strstr(be_itf1, be_itf2)))
            result = false;
    }

    ALOGV("%s: be_itf1 = %s, be_itf2 = %s, match %d", __func__, be_itf1, be_itf2, result);
    return result;
}

int platform_get_pcm_device_id(audio_usecase_t usecase, int device_type)
{
    int device_id;
    if (device_type == PCM_PLAYBACK)
        device_id = pcm_device_table[usecase][0];
    else
        device_id = pcm_device_table[usecase][1];
    return device_id;
}

static int find_index(const struct name_to_index * table, int32_t len,
                      const char * name)
{
    int ret = 0;
    int32_t i;

    if (table == NULL) {
        ALOGE("%s: table is NULL", __func__);
        ret = -ENODEV;
        goto done;
    }

    if (name == NULL) {
        ALOGE("null key");
        ret = -ENODEV;
        goto done;
    }

    for (i=0; i < len; i++) {
        if (!strcmp(table[i].name, name)) {
            ret = table[i].index;
            goto done;
        }
    }
    ALOGE("%s: Could not find index for name = %s",
            __func__, name);
    ret = -ENODEV;
done:
    return ret;
}

int platform_get_snd_device_index(char *device_name)
{
    return find_index(snd_device_name_index, SND_DEVICE_MAX, device_name);
}

int platform_get_usecase_index(const char *usecase_name)
{
    return find_index(usecase_name_index, AUDIO_USECASE_MAX, usecase_name);
}

void platform_add_operator_specific_device(snd_device_t snd_device,
                                           const char *operator,
                                           const char *mixer_path,
                                           unsigned int acdb_id)
{
    struct operator_specific_device *device;

    if (operator_specific_device_table[snd_device] == NULL) {
        operator_specific_device_table[snd_device] =
            (struct listnode *)calloc(1, sizeof(struct listnode));
        list_init(operator_specific_device_table[snd_device]);
    }

    device = (struct operator_specific_device *)calloc(1, sizeof(struct operator_specific_device));

    device->operator = strdup(operator);
    device->mixer_path = strdup(mixer_path);
    device->acdb_id = acdb_id;

    list_add_tail(operator_specific_device_table[snd_device], &device->list);

    ALOGD("%s: device[%s] -> operator[%s] mixer_path[%s] acdb_id[%d]", __func__,
            platform_get_snd_device_name(snd_device), operator, mixer_path, acdb_id);

}

int platform_set_snd_device_acdb_id(snd_device_t snd_device, unsigned int acdb_id)
{
    int ret = 0;

    if ((snd_device < SND_DEVICE_MIN) || (snd_device >= SND_DEVICE_MAX)) {
        ALOGE("%s: Invalid snd_device = %d",
            __func__, snd_device);
        ret = -EINVAL;
        goto done;
    }

    ALOGV("%s: acdb_device_table[%s]: old = %d new = %d", __func__,
          platform_get_snd_device_name(snd_device), acdb_device_table[snd_device], acdb_id);
    acdb_device_table[snd_device] = acdb_id;
done:
    return ret;
}

int platform_get_snd_device_acdb_id(snd_device_t snd_device)
{
    if ((snd_device < SND_DEVICE_MIN) || (snd_device >= SND_DEVICE_MAX)) {
        ALOGE("%s: Invalid snd_device = %d", __func__, snd_device);
        return -EINVAL;
    }

    if (operator_specific_device_table[snd_device] != NULL)
        return get_operator_specific_device_acdb_id(snd_device);
    else
        return acdb_device_table[snd_device];
}

int platform_send_audio_calibration(void *platform, snd_device_t snd_device)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int acdb_dev_id, acdb_dev_type;

    acdb_dev_id = acdb_device_table[audio_extn_get_spkr_prot_snd_device(snd_device)];
    if (acdb_dev_id < 0) {
        ALOGE("%s: Could not find acdb id for device(%d)",
              __func__, snd_device);
        return -EINVAL;
    }
    if (my_data->acdb_send_audio_cal) {
        ALOGV("%s: sending audio calibration for snd_device(%d) acdb_id(%d)",
              __func__, snd_device, acdb_dev_id);
        if (snd_device >= SND_DEVICE_OUT_BEGIN &&
                snd_device < SND_DEVICE_OUT_END)
            acdb_dev_type = ACDB_DEV_TYPE_OUT;
        else
            acdb_dev_type = ACDB_DEV_TYPE_IN;
        my_data->acdb_send_audio_cal(acdb_dev_id, acdb_dev_type);
    }
    return 0;
}

int platform_switch_voice_call_device_pre(void *platform)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int ret = 0;

    if (my_data->csd != NULL &&
        voice_is_in_call(my_data->adev)) {
        /* This must be called before disabling mixer controls on APQ side */
        ret = my_data->csd->disable_device();
        if (ret < 0) {
            ALOGE("%s: csd_client_disable_device, failed, error %d",
                  __func__, ret);
        }
    }
    return ret;
}

int platform_switch_voice_call_enable_device_config(void *platform,
                                                    snd_device_t out_snd_device,
                                                    snd_device_t in_snd_device)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int acdb_rx_id, acdb_tx_id;
    int ret = 0;

    if (my_data->csd == NULL)
        return ret;

    if (out_snd_device == SND_DEVICE_OUT_VOICE_SPEAKER &&
        audio_extn_spkr_prot_is_enabled())
        acdb_rx_id = platform_get_snd_device_acdb_id(SND_DEVICE_OUT_SPEAKER_PROTECTED);
    else
        acdb_rx_id = platform_get_snd_device_acdb_id(out_snd_device);

    acdb_tx_id = platform_get_snd_device_acdb_id(in_snd_device);

    if (acdb_rx_id > 0 && acdb_tx_id > 0) {
        ret = my_data->csd->enable_device_config(acdb_rx_id, acdb_tx_id);
        if (ret < 0) {
            ALOGE("%s: csd_enable_device_config, failed, error %d",
                  __func__, ret);
        }
    } else {
        ALOGE("%s: Incorrect ACDB IDs (rx: %d tx: %d)", __func__,
              acdb_rx_id, acdb_tx_id);
    }

    return ret;
}

int platform_switch_voice_call_device_post(void *platform,
                                           snd_device_t out_snd_device,
                                           snd_device_t in_snd_device)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int acdb_rx_id, acdb_tx_id;

    if (my_data->acdb_send_voice_cal == NULL) {
        ALOGE("%s: dlsym error for acdb_send_voice_call", __func__);
    } else {
        if (out_snd_device == SND_DEVICE_OUT_VOICE_SPEAKER &&
            audio_extn_spkr_prot_is_enabled())
            out_snd_device = SND_DEVICE_OUT_VOICE_SPEAKER_PROTECTED;

        acdb_rx_id = platform_get_snd_device_acdb_id(out_snd_device);
        acdb_tx_id = platform_get_snd_device_acdb_id(in_snd_device);

        if (acdb_rx_id > 0 && acdb_tx_id > 0)
            my_data->acdb_send_voice_cal(acdb_rx_id, acdb_tx_id);
        else
            ALOGE("%s: Incorrect ACDB IDs (rx: %d tx: %d)", __func__,
                  acdb_rx_id, acdb_tx_id);
    }

    return 0;
}

int platform_switch_voice_call_usecase_route_post(void *platform,
                                                  snd_device_t out_snd_device,
                                                  snd_device_t in_snd_device)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int acdb_rx_id, acdb_tx_id;
    int ret = 0;

    if (my_data->csd == NULL)
        return ret;

    if (out_snd_device == SND_DEVICE_OUT_VOICE_SPEAKER &&
        audio_extn_spkr_prot_is_enabled())
        acdb_rx_id = platform_get_snd_device_acdb_id(SND_DEVICE_OUT_VOICE_SPEAKER_PROTECTED);
    else
        acdb_rx_id = platform_get_snd_device_acdb_id(out_snd_device);

    acdb_tx_id = platform_get_snd_device_acdb_id(in_snd_device);

    if (acdb_rx_id > 0 && acdb_tx_id > 0) {
        ret = my_data->csd->enable_device(acdb_rx_id, acdb_tx_id,
                                          my_data->adev->acdb_settings);
        if (ret < 0) {
            ALOGE("%s: csd_enable_device, failed, error %d", __func__, ret);
        }
    } else {
        ALOGE("%s: Incorrect ACDB IDs (rx: %d tx: %d)", __func__,
              acdb_rx_id, acdb_tx_id);
    }

    return ret;
}

int platform_start_voice_call(void *platform, uint32_t vsid)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int ret = 0;

    if (my_data->csd != NULL) {
        ret = my_data->csd->start_voice(vsid);
        if (ret < 0) {
            ALOGE("%s: csd_start_voice error %d\n", __func__, ret);
        }
    }
    return ret;
}

int platform_stop_voice_call(void *platform, uint32_t vsid)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int ret = 0;

    if (my_data->csd != NULL) {
        ret = my_data->csd->stop_voice(vsid);
        if (ret < 0) {
            ALOGE("%s: csd_stop_voice error %d\n", __func__, ret);
        }
    }
    return ret;
}

int platform_get_sample_rate(void *platform, uint32_t *rate)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int ret = 0;

    if (my_data->csd != NULL) {
        ret = my_data->csd->get_sample_rate(rate);
        if (ret < 0) {
            ALOGE("%s: csd_get_sample_rate error %d\n", __func__, ret);
        }
    }
    return ret;
}

void platform_set_speaker_gain_in_combo(struct audio_device *adev,
                                        snd_device_t snd_device,
                                        bool enable)
{
    const char* name;
    switch (snd_device) {
        case SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES:
            if (enable)
                name = "spkr-gain-in-headphone-combo";
            else
                name = "speaker-gain-default";
            break;
        case SND_DEVICE_OUT_SPEAKER_AND_LINE:
            if (enable)
                name = "spkr-gain-in-line-combo";
            else
                name = "speaker-gain-default";
            break;
        case SND_DEVICE_OUT_SPEAKER_SAFE_AND_HEADPHONES:
            if (enable)
                name = "spkr-safe-gain-in-headphone-combo";
            else
                name = "speaker-safe-gain-default";
            break;
        case SND_DEVICE_OUT_SPEAKER_SAFE_AND_LINE:
            if (enable)
                name = "spkr-safe-gain-in-line-combo";
            else
                name = "speaker-safe-gain-default";
            break;
        default:
            return;
    }

    audio_route_apply_and_update_path(adev->audio_route, name);
}

int platform_set_voice_volume(void *platform, int volume)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = "Voice Rx Gain";
    int vol_index = 0, ret = 0;
    uint32_t set_values[ ] = {0,
                              ALL_SESSION_VSID,
                              DEFAULT_VOLUME_RAMP_DURATION_MS};

    // Voice volume levels are mapped to adsp volume levels as follows.
    // 100 -> 5, 80 -> 4, 60 -> 3, 40 -> 2, 20 -> 1  0 -> 0
    // But this values don't changed in kernel. So, below change is need.
    vol_index = (int)percent_to_index(volume, MIN_VOL_INDEX, my_data->max_vol_index);
    set_values[0] = vol_index;

    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        return -EINVAL;
    }
    ALOGV("Setting voice volume index: %d", set_values[0]);
    mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));

    if (my_data->csd != NULL) {
        ret = my_data->csd->volume(ALL_SESSION_VSID, volume,
                                   DEFAULT_VOLUME_RAMP_DURATION_MS);
        if (ret < 0) {
            ALOGE("%s: csd_volume error %d", __func__, ret);
        }
    }
    return ret;
}

int platform_set_mic_mute(void *platform, bool state)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = "Voice Tx Mute";
    int ret = 0;
    uint32_t set_values[ ] = {0,
                              ALL_SESSION_VSID,
                              DEFAULT_MUTE_RAMP_DURATION_MS};

    if (adev->mode != AUDIO_MODE_IN_CALL &&
        adev->mode != AUDIO_MODE_IN_COMMUNICATION)
        return 0;

    if (adev->enable_hfp)
        mixer_ctl_name = "HFP Tx Mute";

    set_values[0] = state;
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        return -EINVAL;
    }
    ALOGV("Setting voice mute state: %d", state);
    mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));

    if (my_data->csd != NULL) {
        ret = my_data->csd->mic_mute(ALL_SESSION_VSID, state,
                                     DEFAULT_MUTE_RAMP_DURATION_MS);
        if (ret < 0) {
            ALOGE("%s: csd_mic_mute error %d", __func__, ret);
        }
    }
    return ret;
}

int platform_set_device_mute(void *platform, bool state, char *dir)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;
    struct mixer_ctl *ctl;
    char *mixer_ctl_name = NULL;
    int ret = 0;
    uint32_t set_values[ ] = {0,
                              ALL_SESSION_VSID,
                              0};
    if(dir == NULL) {
        ALOGE("%s: Invalid direction:%s", __func__, dir);
        return -EINVAL;
    }

    if (!strncmp("rx", dir, sizeof("rx"))) {
        mixer_ctl_name = "Voice Rx Device Mute";
    } else if (!strncmp("tx", dir, sizeof("tx"))) {
        mixer_ctl_name = "Voice Tx Device Mute";
    } else {
        return -EINVAL;
    }

    set_values[0] = state;
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        return -EINVAL;
    }

    ALOGV("%s: Setting device mute state: %d, mixer ctrl:%s",
          __func__,state, mixer_ctl_name);
    mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));

    return ret;
}

bool platform_can_split_snd_device(snd_device_t snd_device,
                                   int *num_devices,
                                   snd_device_t *new_snd_devices)
{
    bool status = false;

    if (NULL == num_devices || NULL == new_snd_devices) {
        ALOGE("%s: NULL pointer ..", __func__);
        return false;
    }

    /*
     * If wired headset/headphones/line devices share the same backend
     * with speaker/earpiece this routine returns false.
     */
    if (snd_device == SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES &&
        !platform_check_backends_match(SND_DEVICE_OUT_SPEAKER, SND_DEVICE_OUT_HEADPHONES)) {
        *num_devices = 2;
        new_snd_devices[0] = SND_DEVICE_OUT_SPEAKER;
        new_snd_devices[1] = SND_DEVICE_OUT_HEADPHONES;
        status = true;
    } else if (snd_device == SND_DEVICE_OUT_SPEAKER_AND_LINE &&
               !platform_check_backends_match(SND_DEVICE_OUT_SPEAKER, SND_DEVICE_OUT_LINE)) {
        *num_devices = 2;
        new_snd_devices[0] = SND_DEVICE_OUT_SPEAKER;
        new_snd_devices[1] = SND_DEVICE_OUT_LINE;
        status = true;
    } else if (snd_device == SND_DEVICE_OUT_SPEAKER_SAFE_AND_HEADPHONES &&
               !platform_check_backends_match(SND_DEVICE_OUT_SPEAKER_SAFE, SND_DEVICE_OUT_HEADPHONES)) {
        *num_devices = 2;
        new_snd_devices[0] = SND_DEVICE_OUT_SPEAKER_SAFE;
        new_snd_devices[1] = SND_DEVICE_OUT_HEADPHONES;
        status = true;
    } else if (snd_device == SND_DEVICE_OUT_SPEAKER_SAFE_AND_LINE &&
               !platform_check_backends_match(SND_DEVICE_OUT_SPEAKER_SAFE, SND_DEVICE_OUT_LINE)) {
        *num_devices = 2;
        new_snd_devices[0] = SND_DEVICE_OUT_SPEAKER_SAFE;
        new_snd_devices[1] = SND_DEVICE_OUT_LINE;
        status = true;
    }
    return status;
}

snd_device_t platform_get_output_snd_device(void *platform, audio_devices_t devices)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;
    audio_mode_t mode = adev->mode;
    snd_device_t snd_device = SND_DEVICE_NONE;

    ALOGV("%s: enter: output devices(%#x)", __func__, devices);
    if (devices == AUDIO_DEVICE_NONE ||
        devices & AUDIO_DEVICE_BIT_IN) {
        ALOGV("%s: Invalid output devices (%#x)", __func__, devices);
        goto exit;
    }

    if (popcount(devices) == 2) {
        if (devices == (AUDIO_DEVICE_OUT_WIRED_HEADPHONE |
                        AUDIO_DEVICE_OUT_SPEAKER) ||
                devices == (AUDIO_DEVICE_OUT_WIRED_HEADSET |
                            AUDIO_DEVICE_OUT_SPEAKER)) {
            snd_device = SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES;
        } else if (devices == (AUDIO_DEVICE_OUT_LINE |
                               AUDIO_DEVICE_OUT_SPEAKER)) {
            snd_device = SND_DEVICE_OUT_SPEAKER_AND_LINE;
        } else if (devices == (AUDIO_DEVICE_OUT_WIRED_HEADPHONE |
                               AUDIO_DEVICE_OUT_SPEAKER_SAFE) ||
                   devices == (AUDIO_DEVICE_OUT_WIRED_HEADSET |
                               AUDIO_DEVICE_OUT_SPEAKER_SAFE)) {
            snd_device = SND_DEVICE_OUT_SPEAKER_SAFE_AND_HEADPHONES;
        } else if (devices == (AUDIO_DEVICE_OUT_LINE |
                               AUDIO_DEVICE_OUT_SPEAKER_SAFE)) {
            snd_device = SND_DEVICE_OUT_SPEAKER_SAFE_AND_LINE;
        } else if (devices == (AUDIO_DEVICE_OUT_AUX_DIGITAL |
                               AUDIO_DEVICE_OUT_SPEAKER)) {
            snd_device = SND_DEVICE_OUT_SPEAKER_AND_HDMI;
        } else {
            ALOGE("%s: Invalid combo device(%#x)", __func__, devices);
            goto exit;
        }
        if (snd_device != SND_DEVICE_NONE) {
            goto exit;
        }
    }

    if (popcount(devices) != 1) {
        ALOGE("%s: Invalid output devices(%#x)", __func__, devices);
        goto exit;
    }

    if (voice_is_in_call(adev) || adev->enable_voicerx) {
        if (devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
            devices & AUDIO_DEVICE_OUT_WIRED_HEADSET ||
            devices & AUDIO_DEVICE_OUT_LINE) {
            if (voice_is_in_call(adev) &&
                (adev->voice.tty_mode == TTY_MODE_FULL))
                snd_device = SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES;
            else if (voice_is_in_call(adev) &&
                (adev->voice.tty_mode == TTY_MODE_VCO))
                snd_device = SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES;
            else if (voice_is_in_call(adev) &&
                (adev->voice.tty_mode == TTY_MODE_HCO))
                snd_device = SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET;
            else {
                if (devices & AUDIO_DEVICE_OUT_LINE)
                    snd_device = SND_DEVICE_OUT_VOICE_LINE;
                else
                    snd_device = SND_DEVICE_OUT_VOICE_HEADPHONES;
                }
        } else if (devices & AUDIO_DEVICE_OUT_ALL_SCO) {
            if (adev->bt_wb_speech_enabled) {
                snd_device = SND_DEVICE_OUT_BT_SCO_WB;
            } else {
                snd_device = SND_DEVICE_OUT_BT_SCO;
            }
        } else if (devices & (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_SPEAKER_SAFE)) {
            if (!adev->enable_hfp) {
                snd_device = SND_DEVICE_OUT_VOICE_SPEAKER;
            } else {
                snd_device = SND_DEVICE_OUT_VOICE_SPEAKER_HFP;
            }
        } else if (devices & AUDIO_DEVICE_OUT_EARPIECE) {
            if(adev->voice.hac)
                snd_device = SND_DEVICE_OUT_VOICE_HAC_HANDSET;
            else if (is_operator_tmus())
                snd_device = SND_DEVICE_OUT_VOICE_HANDSET_TMUS;
            else
                snd_device = SND_DEVICE_OUT_VOICE_HANDSET;
        } else if (devices & AUDIO_DEVICE_OUT_TELEPHONY_TX)
            snd_device = SND_DEVICE_OUT_VOICE_TX;

        if (snd_device != SND_DEVICE_NONE) {
            goto exit;
        }
    }

    if (devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
        devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
        snd_device = SND_DEVICE_OUT_HEADPHONES;
    } else if (devices & AUDIO_DEVICE_OUT_LINE) {
        snd_device = SND_DEVICE_OUT_LINE;
    } else if (devices & AUDIO_DEVICE_OUT_SPEAKER_SAFE) {
        snd_device = SND_DEVICE_OUT_SPEAKER_SAFE;
    } else if (devices & AUDIO_DEVICE_OUT_SPEAKER) {
        if (my_data->speaker_lr_swap)
            snd_device = SND_DEVICE_OUT_SPEAKER_REVERSE;
        else
            snd_device = SND_DEVICE_OUT_SPEAKER;
    } else if (devices & AUDIO_DEVICE_OUT_ALL_SCO) {
        if (adev->bt_wb_speech_enabled) {
            snd_device = SND_DEVICE_OUT_BT_SCO_WB;
        } else {
            snd_device = SND_DEVICE_OUT_BT_SCO;
        }
    } else if (devices & AUDIO_DEVICE_OUT_AUX_DIGITAL) {
        snd_device = SND_DEVICE_OUT_HDMI ;
    } else if (devices & AUDIO_DEVICE_OUT_EARPIECE) {
        /*HAC support for voice-ish audio (eg visual voicemail)*/
        if(adev->voice.hac)
            snd_device = SND_DEVICE_OUT_VOICE_HAC_HANDSET;
        else
            snd_device = SND_DEVICE_OUT_HANDSET;
    } else {
        ALOGE("%s: Unknown device(s) %#x", __func__, devices);
    }
exit:
    ALOGV("%s: exit: snd_device(%s)", __func__, device_table[snd_device]);
    return snd_device;
}

snd_device_t platform_get_input_snd_device(void *platform, audio_devices_t out_device)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;
    audio_source_t  source = (adev->active_input == NULL) ?
                                AUDIO_SOURCE_DEFAULT : adev->active_input->source;

    audio_mode_t    mode   = adev->mode;
    audio_devices_t in_device = ((adev->active_input == NULL) ?
                                    AUDIO_DEVICE_NONE : adev->active_input->device)
                                & ~AUDIO_DEVICE_BIT_IN;
    audio_channel_mask_t channel_mask = (adev->active_input == NULL) ?
                                AUDIO_CHANNEL_IN_MONO : adev->active_input->channel_mask;
    snd_device_t snd_device = SND_DEVICE_NONE;
    int channel_count = popcount(channel_mask);

    ALOGV("%s: enter: out_device(%#x) in_device(%#x) channel_count (%d) channel_mask (0x%x)",
          __func__, out_device, in_device, channel_count, channel_mask);
    if ((out_device != AUDIO_DEVICE_NONE) && voice_is_in_call(adev)) {
        if (adev->voice.tty_mode != TTY_MODE_OFF) {
            if (out_device & AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
                out_device & AUDIO_DEVICE_OUT_WIRED_HEADSET ||
                out_device & AUDIO_DEVICE_OUT_LINE) {
                switch (adev->voice.tty_mode) {
                case TTY_MODE_FULL:
                    snd_device = SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC;
                    break;
                case TTY_MODE_VCO:
                    snd_device = SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC;
                    break;
                case TTY_MODE_HCO:
                    snd_device = SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC;
                    break;
                default:
                    ALOGE("%s: Invalid TTY mode (%#x)", __func__, adev->voice.tty_mode);
                }
                goto exit;
            }
        }
        if (out_device & AUDIO_DEVICE_OUT_EARPIECE) {
            if (my_data->fluence_in_voice_call == false) {
                snd_device = SND_DEVICE_IN_HANDSET_MIC;
            } else {
                if (is_operator_tmus())
                    snd_device = SND_DEVICE_IN_VOICE_DMIC_TMUS;
                else
                    snd_device = SND_DEVICE_IN_VOICE_DMIC;
            }
        } else if (out_device & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
            snd_device = SND_DEVICE_IN_VOICE_HEADSET_MIC;
        } else if (out_device & AUDIO_DEVICE_OUT_ALL_SCO) {
            if (adev->bt_wb_speech_enabled) {
                if (adev->bluetooth_nrec)
                    snd_device = SND_DEVICE_IN_BT_SCO_MIC_WB_NREC;
                else
                    snd_device = SND_DEVICE_IN_BT_SCO_MIC_WB;
            } else {
                if (adev->bluetooth_nrec)
                    snd_device = SND_DEVICE_IN_BT_SCO_MIC_NREC;
                else
                    snd_device = SND_DEVICE_IN_BT_SCO_MIC;
            }
        } else if (out_device & AUDIO_DEVICE_OUT_SPEAKER ||
                   out_device & AUDIO_DEVICE_OUT_SPEAKER_SAFE ||
                   out_device & AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
                   out_device & AUDIO_DEVICE_OUT_LINE) {
            if (my_data->fluence_in_voice_call && my_data->fluence_in_spkr_mode) {
                if (my_data->source_mic_type & SOURCE_DUAL_MIC) {
                    snd_device = SND_DEVICE_IN_VOICE_SPEAKER_DMIC;
                } else {
                    snd_device = SND_DEVICE_IN_VOICE_SPEAKER_MIC;
                }
            }

            //select default
            if (snd_device == SND_DEVICE_NONE) {
                if (!adev->enable_hfp) {
                    snd_device = SND_DEVICE_IN_VOICE_SPEAKER_MIC;
                } else {
                    snd_device = SND_DEVICE_IN_VOICE_SPEAKER_MIC_HFP;
                    platform_set_echo_reference(adev, true, out_device);
                }
            }
        } else if (out_device & AUDIO_DEVICE_OUT_TELEPHONY_TX) {
            snd_device = SND_DEVICE_IN_VOICE_RX;
        }
    } else if (source == AUDIO_SOURCE_CAMCORDER) {
        if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC ||
            in_device & AUDIO_DEVICE_IN_BACK_MIC) {
            snd_device = SND_DEVICE_IN_CAMCORDER_MIC;
        }
    } else if (source == AUDIO_SOURCE_VOICE_RECOGNITION) {
        if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC) {
            if (my_data->fluence_in_voice_rec && channel_count == 1) {
                if ((my_data->fluence_type == FLUENCE_PRO_ENABLE) &&
                    (my_data->source_mic_type & SOURCE_QUAD_MIC)) {
                    if (adev->active_input->enable_aec)
                        snd_device = SND_DEVICE_IN_HANDSET_QMIC_AEC;
                    else
                        snd_device = SND_DEVICE_IN_HANDSET_QMIC;
                } else if ((my_data->fluence_type == FLUENCE_PRO_ENABLE) &&
                    (my_data->source_mic_type & SOURCE_THREE_MIC)) {
                    if (adev->active_input->enable_aec)
                        snd_device = SND_DEVICE_IN_HANDSET_TMIC_AEC;
                    else
                        snd_device = SND_DEVICE_IN_HANDSET_TMIC;
                } else if (((my_data->fluence_type == FLUENCE_PRO_ENABLE) ||
                    (my_data->fluence_type == FLUENCE_ENABLE)) &&
                    (my_data->source_mic_type & SOURCE_DUAL_MIC)) {
                    if (adev->active_input->enable_aec)
                        snd_device = SND_DEVICE_IN_HANDSET_DMIC_AEC;
                    else
                        snd_device = SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE;
                }
                platform_set_echo_reference(adev, true, out_device);
            } else if ((channel_mask == AUDIO_CHANNEL_IN_FRONT_BACK) &&
                       (my_data->source_mic_type & SOURCE_DUAL_MIC)) {
                snd_device = SND_DEVICE_IN_VOICE_REC_DMIC_STEREO;
            } else if (((int)channel_mask == AUDIO_CHANNEL_INDEX_MASK_3) &&
                       (my_data->source_mic_type & SOURCE_THREE_MIC)) {
                snd_device = SND_DEVICE_IN_THREE_MIC;
            } else if (((int)channel_mask == AUDIO_CHANNEL_INDEX_MASK_4) &&
                       (my_data->source_mic_type & SOURCE_QUAD_MIC)) {
                snd_device = SND_DEVICE_IN_QUAD_MIC;
            }
            if (snd_device == SND_DEVICE_NONE) {
                if (adev->active_input->enable_ns)
                    snd_device = SND_DEVICE_IN_VOICE_REC_MIC_NS;
                else if (adev->active_input->enable_aec) {
                    snd_device = SND_DEVICE_IN_VOICE_REC_MIC_AEC;
                    platform_set_echo_reference(adev, true, out_device);
               } else
                    snd_device = SND_DEVICE_IN_VOICE_REC_MIC;
            }
        } else if (in_device & AUDIO_DEVICE_IN_WIRED_HEADSET) {
            snd_device = SND_DEVICE_IN_VOICE_REC_HEADSET_MIC;
        }
    } else if (source == AUDIO_SOURCE_UNPROCESSED) {
        if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC) {
            if (((channel_mask == AUDIO_CHANNEL_IN_FRONT_BACK) ||
                 (channel_mask == AUDIO_CHANNEL_IN_STEREO)) &&
                       (my_data->source_mic_type & SOURCE_DUAL_MIC)) {
                snd_device = SND_DEVICE_IN_UNPROCESSED_STEREO_MIC;
            } else if (((int)channel_mask == AUDIO_CHANNEL_INDEX_MASK_3) &&
                       (my_data->source_mic_type & SOURCE_THREE_MIC)) {
                snd_device = SND_DEVICE_IN_UNPROCESSED_THREE_MIC;
            } else if (((int)channel_mask == AUDIO_CHANNEL_INDEX_MASK_4) &&
                       (my_data->source_mic_type & SOURCE_QUAD_MIC)) {
                snd_device = SND_DEVICE_IN_UNPROCESSED_QUAD_MIC;
            } else {
                snd_device = SND_DEVICE_IN_UNPROCESSED_MIC;
            }
        } else if (in_device & AUDIO_DEVICE_IN_WIRED_HEADSET) {
            snd_device = SND_DEVICE_IN_UNPROCESSED_HEADSET_MIC;
        }
    } else if (source == AUDIO_SOURCE_VOICE_COMMUNICATION ||
               mode == AUDIO_MODE_IN_COMMUNICATION) {
        if (out_device & (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_SPEAKER_SAFE))
            in_device = AUDIO_DEVICE_IN_BACK_MIC;
        if (adev->active_input) {
            if (adev->active_input->enable_aec &&
                    adev->active_input->enable_ns) {
                if (in_device & AUDIO_DEVICE_IN_BACK_MIC) {
                    if (my_data->fluence_in_spkr_mode &&
                            my_data->fluence_in_voice_comm &&
                            (my_data->source_mic_type & SOURCE_DUAL_MIC)) {
                        snd_device = SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS;
                    } else {
                        snd_device = SND_DEVICE_IN_SPEAKER_MIC_AEC_NS;
                    }
                } else if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC) {
                    if (my_data->fluence_in_voice_comm &&
                            (my_data->source_mic_type & SOURCE_DUAL_MIC)) {
                        snd_device = SND_DEVICE_IN_HANDSET_DMIC_AEC_NS;
                    } else {
                        snd_device = SND_DEVICE_IN_HANDSET_MIC_AEC_NS;
                    }
                } else if (in_device & AUDIO_DEVICE_IN_WIRED_HEADSET) {
                    snd_device = SND_DEVICE_IN_HEADSET_MIC_AEC;
                }
                platform_set_echo_reference(adev, true, out_device);
            } else if (adev->active_input->enable_aec) {
                if (in_device & AUDIO_DEVICE_IN_BACK_MIC) {
                    if (my_data->fluence_in_spkr_mode &&
                            my_data->fluence_in_voice_comm &&
                            (my_data->source_mic_type & SOURCE_DUAL_MIC)) {
                        snd_device = SND_DEVICE_IN_SPEAKER_DMIC_AEC;
                    } else {
                        snd_device = SND_DEVICE_IN_SPEAKER_MIC_AEC;
                    }
                } else if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC) {
                    if (my_data->fluence_in_voice_comm &&
                            (my_data->source_mic_type & SOURCE_DUAL_MIC)) {
                        snd_device = SND_DEVICE_IN_HANDSET_DMIC_AEC;
                    } else {
                        snd_device = SND_DEVICE_IN_HANDSET_MIC_AEC;
                    }
               } else if (in_device & AUDIO_DEVICE_IN_WIRED_HEADSET) {
                   snd_device = SND_DEVICE_IN_HEADSET_MIC_AEC;
               }
               platform_set_echo_reference(adev, true, out_device);
            } else if (adev->active_input->enable_ns) {
                if (in_device & AUDIO_DEVICE_IN_BACK_MIC) {
                    if (my_data->fluence_in_spkr_mode &&
                            my_data->fluence_in_voice_comm &&
                            (my_data->source_mic_type & SOURCE_DUAL_MIC)) {
                        snd_device = SND_DEVICE_IN_SPEAKER_DMIC_NS;
                    } else {
                        snd_device = SND_DEVICE_IN_SPEAKER_MIC_NS;
                    }
                } else if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC) {
                    if (my_data->fluence_in_voice_comm &&
                            (my_data->source_mic_type & SOURCE_DUAL_MIC)) {
                        snd_device = SND_DEVICE_IN_HANDSET_DMIC_NS;
                    } else {
                        snd_device = SND_DEVICE_IN_HANDSET_MIC_NS;
                    }
                }
            }
        }
    } else if (source == AUDIO_SOURCE_DEFAULT) {
        goto exit;
    }


    if (snd_device != SND_DEVICE_NONE) {
        goto exit;
    }

    if (in_device != AUDIO_DEVICE_NONE &&
            !(in_device & AUDIO_DEVICE_IN_VOICE_CALL) &&
            !(in_device & AUDIO_DEVICE_IN_COMMUNICATION)) {
        if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC) {
            if ((my_data->source_mic_type & SOURCE_QUAD_MIC) &&
                (int)channel_mask == AUDIO_CHANNEL_INDEX_MASK_4) {
                snd_device = SND_DEVICE_IN_QUAD_MIC;
            } else if ((my_data->source_mic_type & SOURCE_THREE_MIC) &&
                       (int)channel_mask == AUDIO_CHANNEL_INDEX_MASK_3) {
                snd_device = SND_DEVICE_IN_THREE_MIC;
            } else if ((my_data->source_mic_type & SOURCE_DUAL_MIC) &&
                       channel_count == 2) {
                snd_device = SND_DEVICE_IN_HANDSET_DMIC_STEREO;
            } else if ((my_data->source_mic_type & SOURCE_MONO_MIC) &&
                       channel_count == 1) {
                snd_device = SND_DEVICE_IN_HANDSET_MIC;
            } else {
                ALOGE("%s: something wrong (1): source type (%d) channel_count (%d) .."
                      " channel mask (0x%x) no combination found .. setting to mono", __func__,
                       my_data->source_mic_type, channel_count, channel_mask);
                snd_device = SND_DEVICE_IN_HANDSET_MIC;
            }
        } else if (in_device & AUDIO_DEVICE_IN_BACK_MIC) {
            if ((my_data->source_mic_type & SOURCE_DUAL_MIC) &&
                    channel_count == 2) {
                snd_device = SND_DEVICE_IN_SPEAKER_DMIC_STEREO;
            } else if ((my_data->source_mic_type & SOURCE_MONO_MIC) &&
                    channel_count == 1) {
                snd_device = SND_DEVICE_IN_SPEAKER_MIC;
            } else {
                ALOGE("%s: something wrong (2): source type (%d) channel_count (%d) .."
                      " no combination found .. setting to mono", __func__,
                       my_data->source_mic_type, channel_count);
                snd_device = SND_DEVICE_IN_SPEAKER_MIC;
            }
        } else if (in_device & AUDIO_DEVICE_IN_WIRED_HEADSET) {
            snd_device = SND_DEVICE_IN_HEADSET_MIC;
        } else if (in_device & AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET) {
            if (adev->bt_wb_speech_enabled) {
                if (adev->bluetooth_nrec)
                    snd_device = SND_DEVICE_IN_BT_SCO_MIC_WB_NREC;
                else
                    snd_device = SND_DEVICE_IN_BT_SCO_MIC_WB;
            } else {
                if (adev->bluetooth_nrec)
                    snd_device = SND_DEVICE_IN_BT_SCO_MIC_NREC;
                else
                    snd_device = SND_DEVICE_IN_BT_SCO_MIC;
            }
        } else if (in_device & AUDIO_DEVICE_IN_AUX_DIGITAL) {
            snd_device = SND_DEVICE_IN_HDMI_MIC;
        } else {
            ALOGE("%s: Unknown input device(s) %#x", __func__, in_device);
            ALOGW("%s: Using default handset-mic", __func__);
            snd_device = SND_DEVICE_IN_HANDSET_MIC;
        }
    } else {
        if (out_device & AUDIO_DEVICE_OUT_EARPIECE) {
            snd_device = SND_DEVICE_IN_HANDSET_MIC;
        } else if (out_device & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
            snd_device = SND_DEVICE_IN_HEADSET_MIC;
        } else if (out_device & AUDIO_DEVICE_OUT_SPEAKER ||
                   out_device & AUDIO_DEVICE_OUT_SPEAKER_SAFE ||
                   out_device & AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
                   out_device & AUDIO_DEVICE_OUT_LINE) {
            if ((my_data->source_mic_type & SOURCE_DUAL_MIC) &&
                    channel_count == 2) {
                snd_device = SND_DEVICE_IN_SPEAKER_DMIC_STEREO;
            } else if ((my_data->source_mic_type & SOURCE_MONO_MIC) &&
                          channel_count == 1) {
                snd_device = SND_DEVICE_IN_SPEAKER_MIC;
            } else {
                ALOGE("%s: something wrong (3): source type (%d) channel_count (%d) .."
                      " no combination found .. setting to mono", __func__,
                       my_data->source_mic_type, channel_count);
                snd_device = SND_DEVICE_IN_SPEAKER_MIC;
            }
        } else if (out_device & AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET) {
            if (adev->bt_wb_speech_enabled) {
                if (adev->bluetooth_nrec)
                    snd_device = SND_DEVICE_IN_BT_SCO_MIC_WB_NREC;
                else
                    snd_device = SND_DEVICE_IN_BT_SCO_MIC_WB;
            } else {
                if (adev->bluetooth_nrec)
                    snd_device = SND_DEVICE_IN_BT_SCO_MIC_NREC;
                else
                    snd_device = SND_DEVICE_IN_BT_SCO_MIC;
            }
        } else if (out_device & AUDIO_DEVICE_OUT_AUX_DIGITAL) {
            snd_device = SND_DEVICE_IN_HDMI_MIC;
        } else {
            ALOGE("%s: Unknown output device(s) %#x", __func__, out_device);
            ALOGW("%s: Using default handset-mic", __func__);
            snd_device = SND_DEVICE_IN_HANDSET_MIC;
        }
    }
exit:
    ALOGV("%s: exit: in_snd_device(%s)", __func__, device_table[snd_device]);
    return snd_device;
}

int platform_set_hdmi_channels(void *platform,  int channel_count)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;
    struct mixer_ctl *ctl;
    const char *channel_cnt_str = NULL;
    const char *mixer_ctl_name = "HDMI_RX Channels";
    switch (channel_count) {
    case 8:
        channel_cnt_str = "Eight"; break;
    case 7:
        channel_cnt_str = "Seven"; break;
    case 6:
        channel_cnt_str = "Six"; break;
    case 5:
        channel_cnt_str = "Five"; break;
    case 4:
        channel_cnt_str = "Four"; break;
    case 3:
        channel_cnt_str = "Three"; break;
    default:
        channel_cnt_str = "Two"; break;
    }
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        return -EINVAL;
    }
    ALOGV("HDMI channel count: %s", channel_cnt_str);
    mixer_ctl_set_enum_by_string(ctl, channel_cnt_str);
    return 0;
}

int platform_edid_get_max_channels(void *platform)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;
    char block[MAX_SAD_BLOCKS * SAD_BLOCK_SIZE];
    char *sad = block;
    int num_audio_blocks;
    int channel_count;
    int max_channels = 0;
    int i, ret, count;

    struct mixer_ctl *ctl;

    ctl = mixer_get_ctl_by_name(adev->mixer, AUDIO_DATA_BLOCK_MIXER_CTL);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, AUDIO_DATA_BLOCK_MIXER_CTL);
        return 0;
    }

    mixer_ctl_update(ctl);

    count = mixer_ctl_get_num_values(ctl);

    /* Read SAD blocks, clamping the maximum size for safety */
    if (count > (int)sizeof(block))
        count = (int)sizeof(block);

    ret = mixer_ctl_get_array(ctl, block, count);
    if (ret != 0) {
        ALOGE("%s: mixer_ctl_get_array() failed to get EDID info", __func__);
        return 0;
    }

    /* Calculate the number of SAD blocks */
    num_audio_blocks = count / SAD_BLOCK_SIZE;

    for (i = 0; i < num_audio_blocks; i++) {
        /* Only consider LPCM blocks */
        if ((sad[0] >> 3) != EDID_FORMAT_LPCM) {
            sad += 3;
            continue;
        }

        channel_count = (sad[0] & 0x7) + 1;
        if (channel_count > max_channels)
            max_channels = channel_count;

        /* Advance to next block */
        sad += 3;
    }

    return max_channels;
}

int platform_set_incall_recording_session_id(void *platform,
                                             uint32_t session_id, int rec_mode)
{
    int ret = 0;
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = "Voc VSID";
    int num_ctl_values;
    int i;

    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        ret = -EINVAL;
    } else {
        num_ctl_values = mixer_ctl_get_num_values(ctl);
        for (i = 0; i < num_ctl_values; i++) {
            if (mixer_ctl_set_value(ctl, i, session_id)) {
                ALOGV("Error: invalid session_id: %x", session_id);
                ret = -EINVAL;
                break;
            }
        }
    }

    if (my_data->csd != NULL) {
        ret = my_data->csd->start_record(ALL_SESSION_VSID, rec_mode);
        if (ret < 0) {
            ALOGE("%s: csd_client_start_record failed, error %d",
                  __func__, ret);
        }
    }

    return ret;
}

int platform_stop_incall_recording_usecase(void *platform)
{
    int ret = 0;
    struct platform_data *my_data = (struct platform_data *)platform;

    if (my_data->csd != NULL) {
        ret = my_data->csd->stop_record(ALL_SESSION_VSID);
        if (ret < 0) {
            ALOGE("%s: csd_client_stop_record failed, error %d",
                  __func__, ret);
        }
    }

    return ret;
}

int platform_start_incall_music_usecase(void *platform)
{
    int ret = 0;
    struct platform_data *my_data = (struct platform_data *)platform;

    if (my_data->csd != NULL) {
        ret = my_data->csd->start_playback(ALL_SESSION_VSID);
        if (ret < 0) {
            ALOGE("%s: csd_client_start_playback failed, error %d",
                  __func__, ret);
        }
    }

    return ret;
}

int platform_stop_incall_music_usecase(void *platform)
{
    int ret = 0;
    struct platform_data *my_data = (struct platform_data *)platform;

    if (my_data->csd != NULL) {
        ret = my_data->csd->stop_playback(ALL_SESSION_VSID);
        if (ret < 0) {
            ALOGE("%s: csd_client_stop_playback failed, error %d",
                  __func__, ret);
        }
    }

    return ret;
}

int platform_set_parameters(void *platform, struct str_parms *parms)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    char value[128];
    char *kv_pairs = str_parms_to_str(parms);
    int ret = 0, err;

    if (kv_pairs == NULL) {
        ret = -EINVAL;
        ALOGE("%s: key-value pair is NULL",__func__);
        goto done;
    }

    ALOGV("%s: enter: %s", __func__, kv_pairs);

    err = str_parms_get_str(parms, PLATFORM_CONFIG_KEY_SOUNDCARD_NAME,
                            value, sizeof(value));
    if (err >= 0) {
        str_parms_del(parms, PLATFORM_CONFIG_KEY_SOUNDCARD_NAME);
        my_data->snd_card_name = strdup(value);
        ALOGV("%s: sound card name %s", __func__, my_data->snd_card_name);
    }

    err = str_parms_get_str(parms, PLATFORM_CONFIG_KEY_OPERATOR_INFO,
                            value, sizeof(value));
    if (err >= 0) {
        struct operator_info *info;
        char *str = value;
        char *name;

        str_parms_del(parms, PLATFORM_CONFIG_KEY_OPERATOR_INFO);
        info = (struct operator_info *)calloc(1, sizeof(struct operator_info));
        name = strtok(str, ";");
        info->name = strdup(name);
        info->mccmnc = strdup(str + strlen(name) + 1);

        list_add_tail(&operator_info_list, &info->list);
        ALOGV("%s: add operator[%s] mccmnc[%s]", __func__, info->name, info->mccmnc);
    }

    memset(value, 0, sizeof(value));
    err = str_parms_get_str(parms, PLATFORM_CONFIG_KEY_MAX_MIC_COUNT,
                            value, sizeof(value));
    if (err >= 0) {
        str_parms_del(parms, PLATFORM_CONFIG_KEY_MAX_MIC_COUNT);
        my_data->max_mic_count = atoi(value);
        ALOGV("%s: max_mic_count %s/%d", __func__, value, my_data->max_mic_count);
    }

done:
    ALOGV("%s: exit with code(%d)", __func__, ret);
    if (kv_pairs != NULL)
        free(kv_pairs);

    return ret;
}

/* Delay in Us */
int64_t platform_render_latency(audio_usecase_t usecase)
{
    switch (usecase) {
        case USECASE_AUDIO_PLAYBACK_DEEP_BUFFER:
            return DEEP_BUFFER_PLATFORM_DELAY;
        case USECASE_AUDIO_PLAYBACK_LOW_LATENCY:
            return LOW_LATENCY_PLATFORM_DELAY;
        default:
            return 0;
    }
}

bool platform_check_and_set_capture_backend_cfg(struct audio_device* adev,
         struct audio_usecase *usecase, snd_device_t snd_device)
{
    enum pcm_format  in_pcm_format = PCM_FORMAT_S16_LE;

    if (adev && adev->active_input)
        in_pcm_format = adev->active_input->config.format;

    // allow 24 bit recording only if voice call is not active
    if (!voice_is_in_call(adev) &&
        adev->mode != AUDIO_MODE_IN_COMMUNICATION &&
        in_pcm_format == PCM_FORMAT_S24_LE) {
        audio_route_apply_and_update_path(adev->audio_route, "set-capture-format-24le");
    } else {
        audio_route_apply_and_update_path(adev->audio_route, "set-capture-format-default");
    }

    return true;
}

int platform_set_snd_device_backend(snd_device_t device, const char *backend_tag,
                                    const char * hw_interface)
{
    int ret = 0;

    if ((device < SND_DEVICE_MIN) || (device >= SND_DEVICE_MAX)) {
        ALOGE("%s: Invalid snd_device = %d",
            __func__, device);
        ret = -EINVAL;
        goto done;
    }

    ALOGV("%s: backend_tag_table[%s]: old = %s new = %s", __func__,
          platform_get_snd_device_name(device),
          backend_tag_table[device] != NULL ? backend_tag_table[device]: "null", backend_tag);
    if (backend_tag_table[device]) {
        free(backend_tag_table[device]);
    }
    backend_tag_table[device] = strdup(backend_tag);

    if (hw_interface != NULL) {
        if (hw_interface_table[device])
            free(hw_interface_table[device]);
        ALOGV("%s: hw_interface_table[%d] = %s", __func__, device, hw_interface);
        hw_interface_table[device] = strdup(hw_interface);
    }
done:
    return ret;
}

int platform_set_usecase_pcm_id(audio_usecase_t usecase, int32_t type, int32_t pcm_id)
{
    int ret = 0;
    if ((usecase <= USECASE_INVALID) || (usecase >= AUDIO_USECASE_MAX)) {
        ALOGE("%s: invalid usecase case idx %d", __func__, usecase);
        ret = -EINVAL;
        goto done;
    }

    if ((type != 0) && (type != 1)) {
        ALOGE("%s: invalid usecase type", __func__);
        ret = -EINVAL;
    }
    ALOGV("%s: pcm_device_table[%d][%d] = %d", __func__, usecase, type, pcm_id);
    pcm_device_table[usecase][type] = pcm_id;
done:
    return ret;
}

#define DEFAULT_NOMINAL_SPEAKER_GAIN 20
int ramp_speaker_gain(struct audio_device *adev, bool ramp_up, int target_ramp_up_gain) {
    // backup_gain: gain to try to set in case of an error during ramp
    int start_gain, end_gain, step, backup_gain, i;
    bool error = false;
    const struct mixer_ctl *ctl;
    const char *mixer_ctl_name_gain_left = "Left Speaker Gain";
    const char *mixer_ctl_name_gain_right = "Right Speaker Gain";
    struct mixer_ctl *ctl_left = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name_gain_left);
    struct mixer_ctl *ctl_right = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name_gain_right);
    if (!ctl_left || !ctl_right) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s or %s, not applying speaker gain ramp",
                      __func__, mixer_ctl_name_gain_left, mixer_ctl_name_gain_right);
        return -EINVAL;
    } else if ((mixer_ctl_get_num_values(ctl_left) != 1)
            || (mixer_ctl_get_num_values(ctl_right) != 1)) {
        ALOGE("%s: Unexpected num values for mixer cmd - %s or %s, not applying speaker gain ramp",
                              __func__, mixer_ctl_name_gain_left, mixer_ctl_name_gain_right);
        return -EINVAL;
    }
    if (ramp_up) {
        start_gain = 0;
        end_gain = target_ramp_up_gain > 0 ? target_ramp_up_gain : DEFAULT_NOMINAL_SPEAKER_GAIN;
        step = +1;
        backup_gain = end_gain;
    } else {
        // using same gain on left and right
        const int left_gain = mixer_ctl_get_value(ctl_left, 0);
        start_gain = left_gain > 0 ? left_gain : DEFAULT_NOMINAL_SPEAKER_GAIN;
        end_gain = 0;
        step = -1;
        backup_gain = start_gain;
    }
    for (i = start_gain ; i != (end_gain + step) ; i += step) {
        //ALOGV("setting speaker gain to %d", i);
        if (mixer_ctl_set_value(ctl_left, 0, i)) {
            ALOGE("%s: error setting %s to %d during gain ramp",
                    __func__, mixer_ctl_name_gain_left, i);
            error = true;
            break;
        }
        if (mixer_ctl_set_value(ctl_right, 0, i)) {
            ALOGE("%s: error setting %s to %d during gain ramp",
                    __func__, mixer_ctl_name_gain_right, i);
            error = true;
            break;
        }
        usleep(1000);
    }
    if (error) {
        // an error occured during the ramp, let's still try to go back to a safe volume
        if (mixer_ctl_set_value(ctl_left, 0, backup_gain)) {
            ALOGE("%s: error restoring left gain to %d", __func__, backup_gain);
        }
        if (mixer_ctl_set_value(ctl_right, 0, backup_gain)) {
            ALOGE("%s: error restoring right gain to %d", __func__, backup_gain);
        }
    }
    return start_gain;
}

int platform_swap_lr_channels(struct audio_device *adev, bool swap_channels)
{
    // only update if there is active pcm playback on speaker
    struct audio_usecase *usecase;
    struct listnode *node;
    struct platform_data *my_data = (struct platform_data *)adev->platform;

    if (my_data->speaker_lr_swap != swap_channels) {
        my_data->speaker_lr_swap = swap_channels;

        list_for_each(node, &adev->usecase_list) {
            usecase = node_to_item(node, struct audio_usecase, list);
            if (usecase->type == PCM_PLAYBACK &&
                    usecase->stream.out->devices & AUDIO_DEVICE_OUT_SPEAKER) {
                /*
                 * If acdb tuning is different for SPEAKER_REVERSE, it is must
                 * to perform device switch to disable the current backend to
                 * enable it with new acdb data.
                 */
                if (acdb_device_table[SND_DEVICE_OUT_SPEAKER] !=
                    acdb_device_table[SND_DEVICE_OUT_SPEAKER_REVERSE]) {
                    const int initial_skpr_gain = ramp_speaker_gain(adev, false /*ramp_up*/, -1);
                    select_devices(adev, usecase->id);
                    if (initial_skpr_gain != -EINVAL) {
                        ramp_speaker_gain(adev, true /*ramp_up*/, initial_skpr_gain);
                    }
                } else {
                    const char *mixer_path;
                    if (swap_channels) {
                        mixer_path = platform_get_snd_device_name(SND_DEVICE_OUT_SPEAKER_REVERSE);
                        audio_route_apply_and_update_path(adev->audio_route, mixer_path);
                    } else {
                        mixer_path = platform_get_snd_device_name(SND_DEVICE_OUT_SPEAKER);
                        audio_route_apply_and_update_path(adev->audio_route, mixer_path);
                    }
                }
                break;
            }
        }
    }
    return 0;
}
