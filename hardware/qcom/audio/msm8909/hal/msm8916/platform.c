/*
 * Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "msm8916_platform"
/*#define LOG_NDEBUG 0*/
#define LOG_NDDEBUG 0

#include <stdlib.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>
#include <audio_hw.h>
#include <platform_api.h>
#include "platform.h"
#include "audio_extn.h"
#include "voice_extn.h"
#include "sound/msmcal-hwdep.h"
#include <dirent.h>
#define SOUND_TRIGGER_DEVICE_HANDSET_MONO_LOW_POWER_ACDB_ID (100)
#define MAX_MIXER_XML_PATH  100
#define MIXER_XML_PATH "/system/etc/mixer_paths.xml"
#define MIXER_XML_PATH_MTP "/system/etc/mixer_paths_mtp.xml"
#define MIXER_XML_PATH_SBC "/system/etc/mixer_paths_sbc.xml"
#define MIXER_XML_PATH_MSM8909_PM8916 "/system/etc/mixer_paths_msm8909_pm8916.xml"
#define MIXER_XML_PATH_QRD_SKUH "/system/etc/mixer_paths_qrd_skuh.xml"
#define MIXER_XML_PATH_QRD_SKUI "/system/etc/mixer_paths_qrd_skui.xml"
#define MIXER_XML_PATH_QRD_SKUHF "/system/etc/mixer_paths_qrd_skuhf.xml"
#define MIXER_XML_PATH_QRD_SKUT "/system/etc/mixer_paths_qrd_skut.xml"
#define MIXER_XML_PATH_SKUK "/system/etc/mixer_paths_skuk.xml"
#define MIXER_XML_PATH_SKUA "/system/etc/mixer_paths_skua.xml"
#define MIXER_XML_PATH_SKUC "/system/etc/mixer_paths_skuc.xml"
#define MIXER_XML_PATH_SKUE "/system/etc/mixer_paths_skue.xml"
#define MIXER_XML_PATH_SKUL "/system/etc/mixer_paths_skul.xml"
#define MIXER_XML_PATH_AUXPCM "/system/etc/mixer_paths_auxpcm.xml"
#define MIXER_XML_PATH_AUXPCM "/system/etc/mixer_paths_auxpcm.xml"
#define MIXER_XML_PATH_WCD9306 "/system/etc/mixer_paths_wcd9306.xml"
#define MIXER_XML_PATH_WCD9330 "/system/etc/mixer_paths_wcd9330.xml"
#define MIXER_XML_PATH_WCD9326 "/system/etc/mixer_paths_wcd9326_i2s.xml"
#define PLATFORM_INFO_XML_PATH      "/system/etc/audio_platform_info.xml"
#define LIB_ACDB_LOADER "libacdbloader.so"
#define AUDIO_DATA_BLOCK_MIXER_CTL "HDMI EDID"
#define CVD_VERSION_MIXER_CTL "CVD Version"

#define MAX_COMPRESS_OFFLOAD_FRAGMENT_SIZE (256 * 1024)
#define MIN_COMPRESS_OFFLOAD_FRAGMENT_SIZE (2 * 1024)
#define COMPRESS_OFFLOAD_FRAGMENT_SIZE_FOR_AV_STREAMING (2 * 1024)
#define COMPRESS_OFFLOAD_FRAGMENT_SIZE (32 * 1024)
/* Used in calculating fragment size for pcm offload */
#define PCM_OFFLOAD_BUFFER_DURATION_FOR_AV 2000 /* 2 secs */
#define PCM_OFFLOAD_BUFFER_DURATION_FOR_AV_STREAMING 100 /* 100 millisecs */

/* MAX PCM fragment size cannot be increased  further due
 * to flinger's cblk size of 1mb,and it has to be a multiple of
 * 24 - lcm of channels supported by DSP
 */
#define MAX_PCM_OFFLOAD_FRAGMENT_SIZE (240 * 1024)
#define MIN_PCM_OFFLOAD_FRAGMENT_SIZE (32 * 1024)

#define ALIGN( num, to ) (((num) + (to-1)) & (~(to-1)))
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

/* fallback app type if the default app type from acdb loader fails */
#define DEFAULT_APP_TYPE  0x11130

/* Retry for delay in FW loading*/
#define RETRY_NUMBER 20
#define RETRY_US 500000
#define MAX_SND_CARD 8

#define SAMPLE_RATE_8KHZ  8000
#define SAMPLE_RATE_16KHZ 16000

#define AUDIO_PARAMETER_KEY_FLUENCE_TYPE  "fluence"
#define AUDIO_PARAMETER_KEY_SLOWTALK      "st_enable"
#define AUDIO_PARAMETER_KEY_HD_VOICE      "hd_voice"
#define AUDIO_PARAMETER_KEY_VOLUME_BOOST  "volume_boost"
#define MAX_CAL_NAME 20
#define APP_TYPE_SYSTEM_SOUNDS 0x00011131
#define APP_TYPE_GENERAL_RECORDING 0x00011132

char cal_name_info[WCD9XXX_MAX_CAL][MAX_CAL_NAME] = {
        [WCD9XXX_ANC_CAL] = "anc_cal",
        [WCD9XXX_MBHC_CAL] = "mbhc_cal",
        [WCD9XXX_MAD_CAL] = "mad_cal",
};

#define AUDIO_PARAMETER_KEY_REC_PLAY_CONC "rec_play_conc_on"

#define  AUDIO_PARAMETER_IS_HW_DECODER_SESSION_ALLOWED  "is_hw_dec_session_allowed"

char * dsp_only_decoders_mime[] = {
    "audio/x-ms-wma" /* wma*/ ,
    "audio/x-ms-wma-lossless" /* wma lossless */ ,
    "audio/x-ms-wma-pro" /* wma prop */ ,
    "audio/amr-wb-plus" /* amr wb plus */ ,
    "audio/alac"  /*alac */ ,
    "audio/x-ape" /*ape */,
};

enum {
	VOICE_FEATURE_SET_DEFAULT,
	VOICE_FEATURE_SET_VOLUME_BOOST
};

struct audio_block_header
{
    int reserved;
    int length;
};

/* Audio calibration related functions */
typedef void (*acdb_deallocate_t)();
typedef int  (*acdb_init_t)(char *, char *, int);
typedef void (*acdb_send_audio_cal_t)(int, int, int, int);
typedef void (*acdb_send_voice_cal_t)(int, int);
typedef int (*acdb_reload_vocvoltable_t)(int);
typedef int  (*acdb_get_default_app_type_t)(void);
typedef int (*acdb_loader_get_calibration_t)(char *attr, int size, void *data);
acdb_loader_get_calibration_t acdb_loader_get_calibration;

struct platform_data {
    struct audio_device *adev;
    bool fluence_in_spkr_mode;
    bool fluence_in_voice_call;
    bool fluence_in_voice_rec;
    bool fluence_in_audio_rec;
    int  fluence_type;
    char fluence_cap[PROPERTY_VALUE_MAX];
    int  fluence_mode;
    bool slowtalk;
    bool hd_voice;
    bool ec_ref_enabled;
    bool is_acdb_initialized;
    bool is_wsa_speaker;
    /* Audio calibration related functions */
    void                       *acdb_handle;
    int                        voice_feature_set;
    acdb_init_t                acdb_init;
    acdb_deallocate_t          acdb_deallocate;
    acdb_send_audio_cal_t      acdb_send_audio_cal;
    acdb_send_voice_cal_t      acdb_send_voice_cal;
    acdb_reload_vocvoltable_t  acdb_reload_vocvoltable;
    acdb_get_default_app_type_t acdb_get_default_app_type;
#ifdef RECORD_PLAY_CONCURRENCY
    bool rec_play_conc_set;
#endif
    void *hw_info;
    struct csd_data *csd;
};

static bool is_external_codec = false;
static const int pcm_device_table_of_ext_codec[AUDIO_USECASE_MAX][2] = {
   [USECASE_QCHAT_CALL] = {QCHAT_CALL_PCM_DEVICE_OF_EXT_CODEC, QCHAT_CALL_PCM_DEVICE_OF_EXT_CODEC}
};

/* List of use cases that has different PCM device ID's for internal and external codecs */
static const int misc_usecase[AUDIO_USECASE_MAX] = { USECASE_QCHAT_CALL };

static const int pcm_device_table[AUDIO_USECASE_MAX][2] = {
    [USECASE_AUDIO_PLAYBACK_DEEP_BUFFER] = {DEEP_BUFFER_PCM_DEVICE,
                                            DEEP_BUFFER_PCM_DEVICE},
    [USECASE_AUDIO_PLAYBACK_LOW_LATENCY] = {LOWLATENCY_PCM_DEVICE,
                                           LOWLATENCY_PCM_DEVICE},
    [USECASE_AUDIO_PLAYBACK_MULTI_CH] = {MULTIMEDIA2_PCM_DEVICE,
                                        MULTIMEDIA2_PCM_DEVICE},
    [USECASE_AUDIO_PLAYBACK_OFFLOAD] =
                     {PLAYBACK_OFFLOAD_DEVICE, PLAYBACK_OFFLOAD_DEVICE},
    [USECASE_AUDIO_RECORD] = {AUDIO_RECORD_PCM_DEVICE, AUDIO_RECORD_PCM_DEVICE},
    [USECASE_AUDIO_RECORD_COMPRESS] = {COMPRESS_CAPTURE_DEVICE, COMPRESS_CAPTURE_DEVICE},
    [USECASE_AUDIO_RECORD_LOW_LATENCY] = {LOWLATENCY_PCM_DEVICE,
                                          LOWLATENCY_PCM_DEVICE},
    [USECASE_AUDIO_RECORD_FM_VIRTUAL] = {MULTIMEDIA2_PCM_DEVICE,
                                  MULTIMEDIA2_PCM_DEVICE},
    [USECASE_AUDIO_PLAYBACK_FM] = {FM_PLAYBACK_PCM_DEVICE, FM_CAPTURE_PCM_DEVICE},
#ifdef ASM_LOOPBACK_RX_ENABLED
    [USECASE_AUDIO_HFP_SCO] = {HFP_ASM_RX_TX_SESSION2, HFP_ASM_RX_TX_SESSION2},
    [USECASE_AUDIO_HFP_SCO_WB] = {HFP_ASM_RX_TX_SESSION2, HFP_ASM_RX_TX_SESSION2},
#else
    [USECASE_AUDIO_HFP_SCO] = {HFP_PCM_RX, HFP_SCO_RX},
    [USECASE_AUDIO_HFP_SCO_WB] = {HFP_PCM_RX, HFP_SCO_RX},
#endif
    [USECASE_VOICE_CALL] = {VOICE_CALL_PCM_DEVICE, VOICE_CALL_PCM_DEVICE},
    [USECASE_VOICE2_CALL] = {VOICE2_CALL_PCM_DEVICE, VOICE2_CALL_PCM_DEVICE},
    [USECASE_VOLTE_CALL] = {VOLTE_CALL_PCM_DEVICE, VOLTE_CALL_PCM_DEVICE},
    [USECASE_QCHAT_CALL] = {QCHAT_CALL_PCM_DEVICE, QCHAT_CALL_PCM_DEVICE},
    [USECASE_VOWLAN_CALL] = {VOWLAN_CALL_PCM_DEVICE, VOWLAN_CALL_PCM_DEVICE},
    [USECASE_COMPRESS_VOIP_CALL] = {COMPRESS_VOIP_CALL_PCM_DEVICE, COMPRESS_VOIP_CALL_PCM_DEVICE},
    [USECASE_INCALL_REC_UPLINK] = {AUDIO_RECORD_PCM_DEVICE,
                                   AUDIO_RECORD_PCM_DEVICE},
    [USECASE_INCALL_REC_DOWNLINK] = {AUDIO_RECORD_PCM_DEVICE,
                                     AUDIO_RECORD_PCM_DEVICE},
    [USECASE_INCALL_REC_UPLINK_AND_DOWNLINK] = {AUDIO_RECORD_PCM_DEVICE,
                                                AUDIO_RECORD_PCM_DEVICE},
    [USECASE_INCALL_REC_UPLINK_COMPRESS] = {COMPRESS_CAPTURE_DEVICE,
                                            COMPRESS_CAPTURE_DEVICE},
    [USECASE_INCALL_REC_DOWNLINK_COMPRESS] = {COMPRESS_CAPTURE_DEVICE,
                                              COMPRESS_CAPTURE_DEVICE},
    [USECASE_INCALL_REC_UPLINK_AND_DOWNLINK_COMPRESS] = {COMPRESS_CAPTURE_DEVICE,
                                                         COMPRESS_CAPTURE_DEVICE},
    [USECASE_INCALL_MUSIC_UPLINK] = {INCALL_MUSIC_UPLINK_PCM_DEVICE,
                                     INCALL_MUSIC_UPLINK_PCM_DEVICE},
    [USECASE_INCALL_MUSIC_UPLINK2] = {INCALL_MUSIC_UPLINK2_PCM_DEVICE,
                                      INCALL_MUSIC_UPLINK2_PCM_DEVICE},
    [USECASE_AUDIO_SPKR_CALIB_RX] = {SPKR_PROT_CALIB_RX_PCM_DEVICE, -1},
    [USECASE_AUDIO_SPKR_CALIB_TX] = {-1, SPKR_PROT_CALIB_TX_PCM_DEVICE},
};

/* Array to store sound devices */
static const char * const device_table[SND_DEVICE_MAX] = {
    [SND_DEVICE_NONE] = "none",
    /* Playback sound devices */
    [SND_DEVICE_OUT_HANDSET] = "handset",
    [SND_DEVICE_OUT_SPEAKER] = "speaker",
    [SND_DEVICE_OUT_SPEAKER_WSA] = "wsa-speaker",
    [SND_DEVICE_OUT_SPEAKER_REVERSE] = "speaker-reverse",
    [SND_DEVICE_OUT_HEADPHONES] = "headphones",
    [SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES] = "speaker-and-headphones",
    [SND_DEVICE_OUT_VOICE_HANDSET] = "voice-handset",
    [SND_DEVICE_OUT_VOICE_SPEAKER] = "voice-speaker",
    [SND_DEVICE_OUT_VOICE_SPEAKER_WSA] = "wsa-voice-speaker",
    [SND_DEVICE_OUT_VOICE_HEADPHONES] = "voice-headphones",
    [SND_DEVICE_OUT_HDMI] = "hdmi",
    [SND_DEVICE_OUT_SPEAKER_AND_HDMI] = "speaker-and-hdmi",
    [SND_DEVICE_OUT_BT_SCO] = "bt-sco-headset",
    [SND_DEVICE_OUT_BT_SCO_WB] = "bt-sco-headset-wb",
    [SND_DEVICE_OUT_BT_A2DP] = "bt-a2dp",
    [SND_DEVICE_OUT_SPEAKER_AND_BT_A2DP] = "speaker-and-bt-a2dp",
    [SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES] = "voice-tty-full-headphones",
    [SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES] = "voice-tty-vco-headphones",
    [SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET] = "voice-tty-hco-handset",
    [SND_DEVICE_OUT_AFE_PROXY] = "afe-proxy",
    [SND_DEVICE_OUT_USB_HEADSET] = "usb-headphones",
    [SND_DEVICE_OUT_SPEAKER_AND_USB_HEADSET] = "speaker-and-usb-headphones",
    [SND_DEVICE_OUT_TRANSMISSION_FM] = "transmission-fm",
    [SND_DEVICE_OUT_ANC_HEADSET] = "anc-headphones",
    [SND_DEVICE_OUT_ANC_FB_HEADSET] = "anc-fb-headphones",
    [SND_DEVICE_OUT_VOICE_ANC_HEADSET] = "voice-anc-headphones",
    [SND_DEVICE_OUT_VOICE_ANC_FB_HEADSET] = "voice-anc-fb-headphones",
    [SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET] = "speaker-and-anc-headphones",
    [SND_DEVICE_OUT_ANC_HANDSET] = "anc-handset",
    [SND_DEVICE_OUT_SPEAKER_PROTECTED] = "speaker-protected",
#ifdef RECORD_PLAY_CONCURRENCY
    [SND_DEVICE_OUT_VOIP_HANDSET] = "voip-handset",
    [SND_DEVICE_OUT_VOIP_SPEAKER] = "voip-speaker",
    [SND_DEVICE_OUT_VOIP_HEADPHONES] = "voip-headphones",
#endif

    /* Capture sound devices */
    [SND_DEVICE_IN_HANDSET_MIC] = "handset-mic",
    [SND_DEVICE_IN_HANDSET_MIC_AEC] = "handset-mic",
    [SND_DEVICE_IN_HANDSET_MIC_NS] = "handset-mic",
    [SND_DEVICE_IN_HANDSET_MIC_AEC_NS] = "handset-mic",
    [SND_DEVICE_IN_HANDSET_DMIC] = "dmic-endfire",
    [SND_DEVICE_IN_HANDSET_DMIC_AEC] = "dmic-endfire",
    [SND_DEVICE_IN_HANDSET_DMIC_NS] = "dmic-endfire",
    [SND_DEVICE_IN_HANDSET_DMIC_AEC_NS] = "dmic-endfire",
    [SND_DEVICE_IN_SPEAKER_MIC] = "speaker-mic",
    [SND_DEVICE_IN_SPEAKER_MIC_AEC] = "speaker-mic",
    [SND_DEVICE_IN_SPEAKER_MIC_NS] = "speaker-mic",
    [SND_DEVICE_IN_SPEAKER_MIC_AEC_NS] = "speaker-mic",
    [SND_DEVICE_IN_SPEAKER_DMIC] = "speaker-dmic-endfire",
    [SND_DEVICE_IN_SPEAKER_DMIC_AEC] = "speaker-dmic-endfire",
    [SND_DEVICE_IN_SPEAKER_DMIC_NS] = "speaker-dmic-endfire",
    [SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS] = "speaker-dmic-endfire",
    [SND_DEVICE_IN_HEADSET_MIC] = "headset-mic",
    [SND_DEVICE_IN_HEADSET_MIC_FLUENCE] = "headset-mic",
    [SND_DEVICE_IN_VOICE_SPEAKER_MIC] = "voice-speaker-mic",
    [SND_DEVICE_IN_VOICE_HEADSET_MIC] = "voice-headset-mic",
    [SND_DEVICE_IN_HDMI_MIC] = "hdmi-mic",
    [SND_DEVICE_IN_BT_SCO_MIC] = "bt-sco-mic",
    [SND_DEVICE_IN_BT_SCO_MIC_NREC] = "bt-sco-mic",
    [SND_DEVICE_IN_BT_SCO_MIC_WB] = "bt-sco-mic-wb",
    [SND_DEVICE_IN_BT_SCO_MIC_WB_NREC] = "bt-sco-mic-wb",
    [SND_DEVICE_IN_CAMCORDER_MIC] = "camcorder-mic",
    [SND_DEVICE_IN_VOICE_DMIC] = "voice-dmic-ef",
    [SND_DEVICE_IN_VOICE_SPEAKER_DMIC] = "voice-speaker-dmic-ef",
    [SND_DEVICE_IN_VOICE_SPEAKER_QMIC] = "voice-speaker-qmic",
    [SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC] = "voice-tty-full-headset-mic",
    [SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC] = "voice-tty-vco-handset-mic",
    [SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC] = "voice-tty-hco-headset-mic",
    [SND_DEVICE_IN_VOICE_REC_MIC] = "voice-rec-mic",
    [SND_DEVICE_IN_VOICE_REC_MIC_NS] = "voice-rec-mic",
    [SND_DEVICE_IN_VOICE_REC_DMIC_STEREO] = "voice-rec-dmic-ef",
    [SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE] = "voice-rec-dmic-ef-fluence",
    [SND_DEVICE_IN_USB_HEADSET_MIC] = "usb-headset-mic",
    [SND_DEVICE_IN_CAPTURE_FM] = "capture-fm",
    [SND_DEVICE_IN_AANC_HANDSET_MIC] = "aanc-handset-mic",
    [SND_DEVICE_IN_QUAD_MIC] = "quad-mic",
    [SND_DEVICE_IN_HANDSET_STEREO_DMIC] = "handset-stereo-dmic-ef",
    [SND_DEVICE_IN_SPEAKER_STEREO_DMIC] = "speaker-stereo-dmic-ef",
    [SND_DEVICE_IN_CAPTURE_VI_FEEDBACK] = "vi-feedback",
    [SND_DEVICE_IN_VOICE_SPEAKER_DMIC_BROADSIDE] = "voice-speaker-dmic-broadside",
    [SND_DEVICE_IN_SPEAKER_DMIC_BROADSIDE] = "speaker-dmic-broadside",
    [SND_DEVICE_IN_SPEAKER_DMIC_AEC_BROADSIDE] = "speaker-dmic-broadside",
    [SND_DEVICE_IN_SPEAKER_DMIC_NS_BROADSIDE] = "speaker-dmic-broadside",
    [SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS_BROADSIDE] = "speaker-dmic-broadside",
    [SND_DEVICE_IN_VOICE_FLUENCE_DMIC_AANC] = "aanc-fluence-dmic-handset",
    [SND_DEVICE_IN_HANDSET_QMIC] = "quad-mic",
    [SND_DEVICE_IN_SPEAKER_QMIC_AEC] = "quad-mic",
    [SND_DEVICE_IN_SPEAKER_QMIC_NS] = "quad-mic",
    [SND_DEVICE_IN_SPEAKER_QMIC_AEC_NS] = "quad-mic",
};

/* ACDB IDs (audio DSP path configuration IDs) for each sound device */
static int acdb_device_table[SND_DEVICE_MAX] = {
    [SND_DEVICE_NONE] = -1,
    [SND_DEVICE_OUT_HANDSET] = 7,
    [SND_DEVICE_OUT_SPEAKER] = 14,
    [SND_DEVICE_OUT_SPEAKER_WSA] = 135,
    [SND_DEVICE_OUT_SPEAKER_REVERSE] = 14,
    [SND_DEVICE_OUT_HEADPHONES] = 10,
    [SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES] = 10,
    [SND_DEVICE_OUT_VOICE_HANDSET] = 7,
    [SND_DEVICE_OUT_VOICE_SPEAKER] = 14,
    [SND_DEVICE_OUT_VOICE_SPEAKER_WSA] = 135,
    [SND_DEVICE_OUT_VOICE_HEADPHONES] = 10,
    [SND_DEVICE_OUT_HDMI] = 18,
    [SND_DEVICE_OUT_SPEAKER_AND_HDMI] = 14,
    [SND_DEVICE_OUT_BT_SCO] = 22,
    [SND_DEVICE_OUT_BT_SCO_WB] = 39,
    [SND_DEVICE_OUT_BT_A2DP] = 20,
    [SND_DEVICE_OUT_SPEAKER_AND_BT_A2DP] = 14,
    [SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES] = 17,
    [SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES] = 17,
    [SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET] = 37,
    [SND_DEVICE_OUT_AFE_PROXY] = 0,
    [SND_DEVICE_OUT_USB_HEADSET] = 45,
    [SND_DEVICE_OUT_SPEAKER_AND_USB_HEADSET] = 14,
    [SND_DEVICE_OUT_TRANSMISSION_FM] = 0,
    [SND_DEVICE_OUT_ANC_HEADSET] = 26,
    [SND_DEVICE_OUT_ANC_FB_HEADSET] = 27,
    [SND_DEVICE_OUT_VOICE_ANC_HEADSET] = 26,
    [SND_DEVICE_OUT_VOICE_ANC_FB_HEADSET] = 27,
    [SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET] = 26,
    [SND_DEVICE_OUT_ANC_HANDSET] = 103,
    [SND_DEVICE_OUT_SPEAKER_PROTECTED] = 101,
#ifdef RECORD_PLAY_CONCURRENCY
    [SND_DEVICE_OUT_VOIP_HANDSET] = 133,
    [SND_DEVICE_OUT_VOIP_SPEAKER] = 132,
    [SND_DEVICE_OUT_VOIP_HEADPHONES] = 134,
#endif

    [SND_DEVICE_IN_HANDSET_MIC] = 4,
    [SND_DEVICE_IN_HANDSET_MIC_AEC] = 106,
    [SND_DEVICE_IN_HANDSET_MIC_NS] = 107,
    [SND_DEVICE_IN_HANDSET_MIC_AEC_NS] = 108,
    [SND_DEVICE_IN_HANDSET_DMIC] = 41,
    [SND_DEVICE_IN_HANDSET_DMIC_AEC] = 109,
    [SND_DEVICE_IN_HANDSET_DMIC_NS] = 110,
    [SND_DEVICE_IN_HANDSET_DMIC_AEC_NS] = 111,
    [SND_DEVICE_IN_SPEAKER_MIC] = 11,
    [SND_DEVICE_IN_SPEAKER_MIC_AEC] = 112,
    [SND_DEVICE_IN_SPEAKER_MIC_NS] = 113,
    [SND_DEVICE_IN_SPEAKER_MIC_AEC_NS] = 114,
    [SND_DEVICE_IN_SPEAKER_DMIC] = 43,
    [SND_DEVICE_IN_SPEAKER_DMIC_AEC] = 115,
    [SND_DEVICE_IN_SPEAKER_DMIC_NS] = 116,
    [SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS] = 117,
    [SND_DEVICE_IN_HEADSET_MIC] = 8,
    [SND_DEVICE_IN_HEADSET_MIC_FLUENCE] = 47,
    [SND_DEVICE_IN_VOICE_SPEAKER_MIC] = 11,
    [SND_DEVICE_IN_VOICE_HEADSET_MIC] = 8,
    [SND_DEVICE_IN_HDMI_MIC] = 4,
    [SND_DEVICE_IN_BT_SCO_MIC] = 21,
    [SND_DEVICE_IN_BT_SCO_MIC_NREC] = 122,
    [SND_DEVICE_IN_BT_SCO_MIC_WB] = 38,
    [SND_DEVICE_IN_BT_SCO_MIC_WB_NREC] = 123,
    [SND_DEVICE_IN_CAMCORDER_MIC] = 4,
    [SND_DEVICE_IN_VOICE_DMIC] = 41,
    [SND_DEVICE_IN_VOICE_SPEAKER_DMIC] = 43,
    [SND_DEVICE_IN_VOICE_SPEAKER_QMIC] = 19,
    [SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC] = 16,
    [SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC] = 36,
    [SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC] = 16,
    [SND_DEVICE_IN_VOICE_REC_MIC] = 4,
    [SND_DEVICE_IN_VOICE_REC_MIC_NS] = 107,
    [SND_DEVICE_IN_VOICE_REC_DMIC_STEREO] = 34,
    [SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE] = 41,
    [SND_DEVICE_IN_USB_HEADSET_MIC] = 44,
    [SND_DEVICE_IN_CAPTURE_FM] = 0,
    [SND_DEVICE_IN_AANC_HANDSET_MIC] = 104,
    [SND_DEVICE_IN_QUAD_MIC] = 46,
    [SND_DEVICE_IN_HANDSET_STEREO_DMIC] = 34,
    [SND_DEVICE_IN_SPEAKER_STEREO_DMIC] = 35,
    [SND_DEVICE_IN_CAPTURE_VI_FEEDBACK] = 102,
    [SND_DEVICE_IN_VOICE_SPEAKER_DMIC_BROADSIDE] = 12,
    [SND_DEVICE_IN_SPEAKER_DMIC_BROADSIDE] = 12,
    [SND_DEVICE_IN_SPEAKER_DMIC_AEC_BROADSIDE] = 119,
    [SND_DEVICE_IN_SPEAKER_DMIC_NS_BROADSIDE] = 121,
    [SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS_BROADSIDE] = 120,
    [SND_DEVICE_IN_VOICE_FLUENCE_DMIC_AANC] = 135,
    [SND_DEVICE_IN_HANDSET_QMIC] = 125,
    [SND_DEVICE_IN_SPEAKER_QMIC_AEC] = 126,
    [SND_DEVICE_IN_SPEAKER_QMIC_NS] = 127,
    [SND_DEVICE_IN_SPEAKER_QMIC_AEC_NS] = 129,
};

struct snd_device_index {
    char name[100];
    unsigned int index;
};

#define TO_NAME_INDEX(X)   #X, X

/* Used to get index from parsed sting */
struct snd_device_index snd_device_name_index[SND_DEVICE_MAX] = {
    {TO_NAME_INDEX(SND_DEVICE_OUT_HANDSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_WSA)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_REVERSE)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_HEADPHONES)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_HANDSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_SPEAKER)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_SPEAKER_WSA)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_HEADPHONES)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_HDMI)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_AND_HDMI)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_BT_SCO)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_BT_SCO_WB)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_BT_A2DP)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_AND_BT_A2DP)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_AFE_PROXY)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_USB_HEADSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_AND_USB_HEADSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_TRANSMISSION_FM)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_ANC_HEADSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_ANC_FB_HEADSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_ANC_HEADSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOICE_ANC_FB_HEADSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_ANC_HANDSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_SPEAKER_PROTECTED)},
#ifdef RECORD_PLAY_CONCURRENCY
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOIP_HANDSET)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOIP_SPEAKER)},
    {TO_NAME_INDEX(SND_DEVICE_OUT_VOIP_HEADPHONES)},
#endif
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_MIC_AEC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_MIC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_MIC_AEC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_DMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_DMIC_AEC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_DMIC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_DMIC_AEC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_MIC_AEC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_MIC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_MIC_AEC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_DMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_DMIC_AEC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_DMIC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HEADSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HEADSET_MIC_FLUENCE)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_SPEAKER_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_HEADSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HDMI_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_BT_SCO_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_BT_SCO_MIC_NREC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_BT_SCO_MIC_WB)},
    {TO_NAME_INDEX(SND_DEVICE_IN_BT_SCO_MIC_WB_NREC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_CAMCORDER_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_DMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_SPEAKER_DMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_SPEAKER_QMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_TTY_FULL_HEADSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_TTY_VCO_HANDSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_TTY_HCO_HEADSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_REC_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_REC_MIC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_REC_DMIC_STEREO)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE)},
    {TO_NAME_INDEX(SND_DEVICE_IN_USB_HEADSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_CAPTURE_FM)},
    {TO_NAME_INDEX(SND_DEVICE_IN_AANC_HANDSET_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_QUAD_MIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_STEREO_DMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_STEREO_DMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_CAPTURE_VI_FEEDBACK)},
    {TO_NAME_INDEX(SND_DEVICE_IN_VOICE_FLUENCE_DMIC_AANC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_HANDSET_QMIC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_QMIC_AEC)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_QMIC_NS)},
    {TO_NAME_INDEX(SND_DEVICE_IN_SPEAKER_QMIC_AEC_NS)},
};

#define NO_COLS 2
static int msm_be_id_array_len;
static int (*msm_device_to_be_id)[];

/* Below table lists output device to BE_ID mapping*/
/* Update the table based on the board configuration*/

static int msm_device_to_be_id_internal_codec [][NO_COLS] = {
       {AUDIO_DEVICE_OUT_EARPIECE                       ,       34},
       {AUDIO_DEVICE_OUT_SPEAKER                        ,       34},
       {AUDIO_DEVICE_OUT_WIRED_HEADSET                  ,       34},
       {AUDIO_DEVICE_OUT_WIRED_HEADPHONE                ,       34},
       {AUDIO_DEVICE_OUT_BLUETOOTH_SCO                  ,       11},
       {AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET          ,       11},
       {AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT           ,       11},
       {AUDIO_DEVICE_OUT_BLUETOOTH_A2DP                 ,       -1},
       {AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES      ,       -1},
       {AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER         ,       -1},
       {AUDIO_DEVICE_OUT_AUX_DIGITAL                    ,       4},
       {AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET              ,       9},
       {AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET              ,       9},
       {AUDIO_DEVICE_OUT_USB_ACCESSORY                  ,       -1},
       {AUDIO_DEVICE_OUT_USB_DEVICE                     ,       -1},
       {AUDIO_DEVICE_OUT_REMOTE_SUBMIX                  ,       9},
       {AUDIO_DEVICE_OUT_PROXY                          ,       9},
       {AUDIO_DEVICE_OUT_FM                             ,       7},
       {AUDIO_DEVICE_OUT_FM_TX                          ,       8},
       {AUDIO_DEVICE_OUT_ALL                            ,      -1},
       {AUDIO_DEVICE_NONE                               ,      -1},
       {AUDIO_DEVICE_OUT_DEFAULT                        ,      -1},
};

static int msm_device_to_be_id_external_codec [][NO_COLS] = {
       {AUDIO_DEVICE_OUT_EARPIECE                       ,       2},
       {AUDIO_DEVICE_OUT_SPEAKER                        ,       2},
       {AUDIO_DEVICE_OUT_WIRED_HEADSET                  ,       2},
       {AUDIO_DEVICE_OUT_WIRED_HEADPHONE                ,       2},
       {AUDIO_DEVICE_OUT_BLUETOOTH_SCO                  ,       11},
       {AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET          ,       11},
       {AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT           ,       11},
       {AUDIO_DEVICE_OUT_BLUETOOTH_A2DP                 ,       -1},
       {AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES      ,       -1},
       {AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER         ,       -1},
       {AUDIO_DEVICE_OUT_AUX_DIGITAL                    ,       4},
       {AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET              ,       9},
       {AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET              ,       9},
       {AUDIO_DEVICE_OUT_USB_ACCESSORY                  ,       -1},
       {AUDIO_DEVICE_OUT_USB_DEVICE                     ,       -1},
       {AUDIO_DEVICE_OUT_REMOTE_SUBMIX                  ,       9},
       {AUDIO_DEVICE_OUT_PROXY                          ,       9},
       {AUDIO_DEVICE_OUT_FM                             ,       7},
       {AUDIO_DEVICE_OUT_FM_TX                          ,       8},
       {AUDIO_DEVICE_OUT_ALL                            ,      -1},
       {AUDIO_DEVICE_NONE                               ,      -1},
       {AUDIO_DEVICE_OUT_DEFAULT                        ,      -1},
};


#define DEEP_BUFFER_PLATFORM_DELAY (29*1000LL)
#define LOW_LATENCY_PLATFORM_DELAY (13*1000LL)

static bool is_misc_usecase(audio_usecase_t usecase) {
     bool ret = false;
     int i;

     for (i = 0; i < AUDIO_USECASE_MAX; i++) {
          if(usecase == misc_usecase[i]) {
             ret = true;
             break;
          }
     }
     return ret;
}

static void update_codec_type(const char *snd_card_name) {

     if (!strncmp(snd_card_name, "msm8939-tapan-snd-card",
                  sizeof("msm8939-tapan-snd-card")) ||
         !strncmp(snd_card_name, "msm8939-tapan9302-snd-card",
                  sizeof("msm8939-tapan9302-snd-card"))||
         !strncmp(snd_card_name, "msm8939-tomtom9330-snd-card",
                  sizeof("msm8939-tomtom9330-snd-card")) ||
         !strncmp(snd_card_name, "msm8x09-tasha9326-snd-card",
                  sizeof("msm8x09-tasha9326-snd-card"))) {
         ALOGI("%s: snd_card_name: %s",__func__,snd_card_name);
         is_external_codec = true;
     }
}

static void query_platform(const char *snd_card_name,
                                      char *mixer_xml_path)
{
    if (!strncmp(snd_card_name, "msm8x16-snd-card-mtp",
                 sizeof("msm8x16-snd-card-mtp"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_MTP,
                sizeof(MIXER_XML_PATH_MTP));

        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_internal_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8x16-snd-card-sbc",
                 sizeof("msm8x16-snd-card-sbc"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_SBC,
                sizeof(MIXER_XML_PATH_SBC));

        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_internal_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8x16-skuh-snd-card",
                 sizeof("msm8x16-skuh-snd-card"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_QRD_SKUH,
                sizeof(MIXER_XML_PATH_QRD_SKUH));

        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_internal_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8x16-skui-snd-card",
                 sizeof("msm8x16-skui-snd-card"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_QRD_SKUI,
                sizeof(MIXER_XML_PATH_QRD_SKUI));

        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_internal_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8x16-skuhf-snd-card",
                 sizeof("msm8x16-skuhf-snd-card"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_QRD_SKUHF,
                sizeof(MIXER_XML_PATH_QRD_SKUHF));

        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_internal_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8939-snd-card-mtp",
                 sizeof("msm8939-snd-card-mtp"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_MTP,
                sizeof(MIXER_XML_PATH_MTP));

        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_internal_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8939-snd-card-skuk",
                 sizeof("msm8939-snd-card-skuk"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_SKUK,
                sizeof(MIXER_XML_PATH_SKUK));
        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_internal_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8939-tapan-snd-card",
                 sizeof("msm8939-tapan-snd-card"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_WCD9306,
                sizeof(MIXER_XML_PATH_WCD9306));
        msm_device_to_be_id = msm_device_to_be_id_external_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_external_codec) / sizeof(msm_device_to_be_id_external_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8939-tapan9302-snd-card",
                 sizeof("msm8939-tapan9302-snd-card"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_WCD9306,
                sizeof(MIXER_XML_PATH_WCD9306));

        msm_device_to_be_id = msm_device_to_be_id_external_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_external_codec) / sizeof(msm_device_to_be_id_external_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8939-tomtom9330-snd-card",
                 sizeof("msm8939-tomtom9330-snd-card"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_WCD9330,
                sizeof(MIXER_XML_PATH_WCD9330));
        msm_device_to_be_id = msm_device_to_be_id_external_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_external_codec) / sizeof(msm_device_to_be_id_external_codec[0]);
    } else if (!strncmp(snd_card_name, "msm8x09-tasha9326-snd-card",
                 sizeof("msm8x09-tasha9326-snd-card"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_WCD9326,
               MAX_MIXER_XML_PATH);
        msm_device_to_be_id = msm_device_to_be_id_external_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_external_codec) / sizeof(msm_device_to_be_id_external_codec[0]);
    } else if (!strncmp(snd_card_name, "msm8909-skua-snd-card",
                sizeof("msm8909-skua-snd-card"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_SKUA,
                sizeof(MIXER_XML_PATH_SKUA));
        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_internal_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8909-skuc-snd-card",
                 sizeof("msm8909-skuc-snd-card"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_SKUC,
                sizeof(MIXER_XML_PATH_SKUC));
        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_internal_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8909-skut-snd-card",
                 sizeof("msm8909-skut-snd-card"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_QRD_SKUT,
                sizeof(MIXER_XML_PATH_QRD_SKUT));
        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_internal_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8909-pm8916-snd-card",
                 sizeof("msm8909-pm8916-snd-card"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_MSM8909_PM8916,
                sizeof(MIXER_XML_PATH_MSM8909_PM8916));
        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_internal_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8909-skue-snd-card",
                 sizeof("msm8909-skue-snd-card"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_SKUE,
                sizeof(MIXER_XML_PATH_SKUE));
        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_internal_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);

    } else if (!strncmp(snd_card_name, "msm8939-snd-card-skul",
                 sizeof("msm8939-snd-card-skul"))) {
        strlcpy(mixer_xml_path, MIXER_XML_PATH_SKUL,
                sizeof(MIXER_XML_PATH_SKUL));
        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_external_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);
    } else {
        strlcpy(mixer_xml_path, MIXER_XML_PATH,
                sizeof(MIXER_XML_PATH));

        msm_device_to_be_id = msm_device_to_be_id_internal_codec;
        msm_be_id_array_len  =
            sizeof(msm_device_to_be_id_internal_codec) / sizeof(msm_device_to_be_id_internal_codec[0]);

    }
}

void platform_set_echo_reference(void *platform, bool enable)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;

    if (my_data->ec_ref_enabled) {
        my_data->ec_ref_enabled = false;
        ALOGD("%s: disabling echo-reference", __func__);
        audio_route_reset_and_update_path(adev->audio_route, "echo-reference");
    }

    if (enable) {
        my_data->ec_ref_enabled = true;
        ALOGD("%s: enabling echo-reference", __func__);
        audio_route_apply_and_update_path(adev->audio_route, "echo-reference");
    }

}

static struct csd_data *open_csd_client()
{
    struct csd_data *csd = calloc(1, sizeof(struct csd_data));
    if (!csd) {
        ALOGE("failed to allocate csd_data mem");
        return NULL;
    }

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
        csd->set_lch = (set_lch_t)dlsym(csd->csd_client, "csd_client_set_lch");
        if (csd->set_lch == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_set_lch",
                  __func__, dlerror());
            /* Ignore the error as this is not mandatory function for
             * basic voice call to work.
             */
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
        csd->init = (init_t)dlsym(csd->csd_client, "csd_client_init");

        if (csd->init == NULL) {
            ALOGE("%s: dlsym error %s for csd_client_init",
                  __func__, dlerror());
            goto error;
        } else {
            csd->init();
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
        count = MAX_CVD_VERSION_STRING_SIZE;

    ret = mixer_ctl_get_array(ctl, cvd_version, count);
    if (ret != 0) {
        ALOGE("%s: ERROR! mixer_ctl_get_array() failed to get CVD Version", __func__);
        goto done;
    }

done:
    return;
}

static int hw_util_open(int card_no)
{
    int fd = -1;
    char dev_name[256];

    snprintf(dev_name, sizeof(dev_name), "/dev/snd/hwC%uD%u",
                               card_no, WCD9XXX_CODEC_HWDEP_NODE);
    ALOGD("%s Opening device %s\n", __func__, dev_name);
    fd = open(dev_name, O_WRONLY);
    if (fd < 0) {
        ALOGE("%s: cannot open device '%s'\n", __func__, dev_name);
        return fd;
    }
    ALOGD("%s success", __func__);
    return fd;
}

struct param_data {
    int    use_case;
    int    acdb_id;
    int    get_size;
    int    buff_size;
    int    data_size;
    void   *buff;
};

static int send_codec_cal(acdb_loader_get_calibration_t acdb_loader_get_calibration, int fd)
{
    int ret = 0, type;

    for (type = WCD9XXX_ANC_CAL; type < WCD9XXX_MAX_CAL; type++) {
        struct wcdcal_ioctl_buffer codec_buffer;
        struct param_data calib;

        if (!strcmp(cal_name_info[type], "mad_cal"))
            calib.acdb_id = SOUND_TRIGGER_DEVICE_HANDSET_MONO_LOW_POWER_ACDB_ID;
        calib.get_size = 1;
        ret = acdb_loader_get_calibration(cal_name_info[type], sizeof(struct param_data),
                                                                 &calib);
        if (ret < 0) {
            ALOGE("%s get_calibration failed\n", __func__);
            return ret;
        }
        calib.get_size = 0;
        calib.buff = malloc(calib.buff_size);
        if(calib.buff == NULL) {
            ALOGE("%s mem allocation for %d bytes for %s failed\n"
                , __func__, calib.buff_size, cal_name_info[type]);
            return -1;
        }
        ret = acdb_loader_get_calibration(cal_name_info[type],
                              sizeof(struct param_data), &calib);
        if (ret < 0) {
            ALOGE("%s get_calibration failed type=%s calib.size=%d\n"
                , __func__, cal_name_info[type], codec_buffer.size);
            free(calib.buff);
            return ret;
        }
        codec_buffer.buffer = calib.buff;
        codec_buffer.size = calib.data_size;
        codec_buffer.cal_type = type;
        if (ioctl(fd, SNDRV_CTL_IOCTL_HWDEP_CAL_TYPE, &codec_buffer) < 0)
            ALOGE("Failed to call ioctl  for %s err=%d calib.size=%d",
                cal_name_info[type], errno, codec_buffer.size);
        ALOGD("%s cal sent for %s calib.size=%d"
            , __func__, cal_name_info[type], codec_buffer.size);
        free(calib.buff);
    }
    return ret;
}

static void audio_hwdep_send_cal(struct platform_data *plat_data)
{
    int fd;

    fd = hw_util_open(plat_data->adev->snd_card);
    if (fd == -1) {
        ALOGE("%s error open\n", __func__);
        return;
    }

    acdb_loader_get_calibration = (acdb_loader_get_calibration_t)
          dlsym(plat_data->acdb_handle, "acdb_loader_get_calibration");

    if (acdb_loader_get_calibration == NULL) {
        ALOGE("%s: ERROR. dlsym Error:%s acdb_loader_get_calibration", __func__,
           dlerror());
        close(fd);
        return;
    }
    if (send_codec_cal(acdb_loader_get_calibration, fd) < 0)
        ALOGE("%s: Could not send anc cal", __FUNCTION__);
    close(fd);
}

int platform_acdb_init(void *platform)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    char *cvd_version = NULL;
    int key = 0;
    const char *snd_card_name;
    int result;
    char value[PROPERTY_VALUE_MAX];
    cvd_version = calloc(1, MAX_CVD_VERSION_STRING_SIZE);
    if (!cvd_version)
        ALOGE("Failed to allocate cvd version");
    else
        get_cvd_version(cvd_version, my_data->adev);

    property_get("audio.ds1.metainfo.key",value,"0");
    key = atoi(value);
    snd_card_name = mixer_get_name(my_data->adev->mixer);
    result = my_data->acdb_init(snd_card_name, cvd_version, key);
    if (cvd_version)
        free(cvd_version);
    if (!result) {
        my_data->is_acdb_initialized = true;
        ALOGD("ACDB initialized");
        audio_hwdep_send_cal(my_data);
    } else {
        my_data->is_acdb_initialized = false;
        ALOGD("ACDB initialization failed");
    }
    return result;
}

#define MAX_PATH             (256)
#define THERMAL_SYSFS "/sys/class/thermal"
#define TZ_TYPE "/sys/class/thermal/thermal_zone%d/type"
#define TZ_WSA "/sys/class/thermal/thermal_zone%d/temp"

static bool is_wsa_found(int *wsaCount)
{
    DIR *tdir = NULL;
    struct dirent *tdirent = NULL;
    int tzn = 0;
    char name[MAX_PATH] = {0};
    char cwd[MAX_PATH] = {0};
    char file[10] = "wsa";
    bool found = false;
    int wsa_count = 0;

    if (!getcwd(cwd, sizeof(cwd)))
        return false;

    chdir(THERMAL_SYSFS); /* Change dir to read the entries. Doesnt work
                             otherwise */
    tdir = opendir(THERMAL_SYSFS);
    if (!tdir) {
        ALOGE("Unable to open %s\n", THERMAL_SYSFS);
        return false;
    }

    while ((tdirent = readdir(tdir))) {
        char buf[50];
        struct dirent *tzdirent;
        DIR *tzdir = NULL;

        tzdir = opendir(tdirent->d_name);
        if (!tzdir)
            continue;
        while ((tzdirent = readdir(tzdir))) {
            if (strcmp(tzdirent->d_name, "type"))
                continue;
            snprintf(name, MAX_PATH, TZ_TYPE, tzn);
            ALOGD("Opening %s\n", name);
            read_line_from_file(name, buf, sizeof(buf));
            if (strstr(buf, file)) {
                wsa_count++;
                /*We support max only two WSA speakers*/
                if (wsa_count == 2)
                    break;
            }
            tzn++;
        }
        closedir(tzdir);
    }
    if (wsa_count > 0){
         ALOGD("Found %d WSA present on the platform", wsa_count);
         found = true;
         *wsaCount = wsa_count;
    }
    closedir(tdir);
    chdir(cwd); /* Restore current working dir */
    return found;
}

void *platform_init(struct audio_device *adev)
{
    char platform[PROPERTY_VALUE_MAX];
    char baseband[PROPERTY_VALUE_MAX];
    char value[PROPERTY_VALUE_MAX];
    struct platform_data *my_data = NULL;
    int retry_num = 0, snd_card_num = 0, key = 0;
    const char *snd_card_name;
    char mixer_xml_path[100],ffspEnable[PROPERTY_VALUE_MAX];
    char *cvd_version = NULL;
    int wsaCount =0;

    my_data = calloc(1, sizeof(struct platform_data));
    if (!my_data) {
        ALOGE("failed to allocate platform data");
        return NULL;
    }

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
        ALOGV("%s: snd_card_name: %s", __func__, snd_card_name);

        my_data->hw_info = hw_info_init(snd_card_name);
        if (!my_data->hw_info) {
            ALOGE("%s: Failed to init hardware info", __func__);
        } else {
            query_platform(snd_card_name, mixer_xml_path);
            ALOGD("%s: mixer path file is %s", __func__,
                                    mixer_xml_path);
            if (audio_extn_read_xml(adev, snd_card_num, mixer_xml_path,
                                    MIXER_XML_PATH_AUXPCM) == -ENOSYS) {
                adev->audio_route = audio_route_init(snd_card_num,
                                                 mixer_xml_path);
            }
            if (!adev->audio_route) {
                ALOGE("%s: Failed to init audio route controls, aborting.",
                       __func__);
                free(my_data);
                mixer_close(adev->mixer);
                return NULL;
            }
            adev->snd_card = snd_card_num;
            update_codec_type(snd_card_name);
            ALOGD("%s: Opened sound card:%d", __func__, snd_card_num);
            break;
        }
        retry_num = 0;
        snd_card_num++;
        mixer_close(adev->mixer);
    }

    if (snd_card_num >= MAX_SND_CARD) {
        ALOGE("%s: Unable to find correct sound card, aborting.", __func__);
        free(my_data);
        return NULL;
    }

    my_data->adev = adev;
    my_data->fluence_in_spkr_mode = false;
    my_data->fluence_in_voice_call = false;
    my_data->fluence_in_voice_rec = false;
    my_data->fluence_in_audio_rec = false;
    my_data->fluence_type = FLUENCE_NONE;
    my_data->fluence_mode = FLUENCE_ENDFIRE;
    my_data->slowtalk = false;
    my_data->hd_voice = false;
    my_data->is_wsa_speaker = false;

    property_get("ro.qc.sdk.audio.fluencetype", my_data->fluence_cap, "");
    if (!strncmp("fluencepro", my_data->fluence_cap, sizeof("fluencepro"))) {
        my_data->fluence_type = FLUENCE_QUAD_MIC | FLUENCE_DUAL_MIC;
    } else if (!strncmp("fluence", my_data->fluence_cap, sizeof("fluence"))) {
        my_data->fluence_type = FLUENCE_DUAL_MIC;
    } else {
        my_data->fluence_type = FLUENCE_NONE;
    }

    if (my_data->fluence_type != FLUENCE_NONE) {
        property_get("persist.audio.fluence.voicecall",value,"");
        if (!strncmp("true", value, sizeof("true"))) {
            my_data->fluence_in_voice_call = true;
        }

        property_get("persist.audio.fluence.voicerec",value,"");
        if (!strncmp("true", value, sizeof("true"))) {
            my_data->fluence_in_voice_rec = true;
        }

        property_get("persist.audio.fluence.audiorec",value,"");
        if (!strncmp("true", value, sizeof("true"))) {
            my_data->fluence_in_audio_rec = true;
        }

        property_get("persist.audio.fluence.speaker",value,"");
        if (!strncmp("true", value, sizeof("true"))) {
            my_data->fluence_in_spkr_mode = true;
        }

        property_get("persist.audio.fluence.mode",value,"");
        if (!strncmp("broadside", value, sizeof("broadside"))) {
            my_data->fluence_mode = FLUENCE_BROADSIDE;
        }
    }

    if (is_wsa_found(&wsaCount)) {
        /*Set ACDB ID of Stereo speaker if two WSAs are present*/
        /*Default ACDB ID for wsa speaker is that for mono*/
        if (wsaCount == 2) {
            platform_set_snd_device_acdb_id(SND_DEVICE_OUT_SPEAKER_WSA, 15);
        }
        my_data->is_wsa_speaker = true;
    }

    property_get("persist.audio.FFSP.enable", ffspEnable, "");
    if (!strncmp("true", ffspEnable, sizeof("true"))) {
        acdb_device_table[SND_DEVICE_OUT_SPEAKER] = 131;
        acdb_device_table[SND_DEVICE_OUT_SPEAKER_WSA] = 131;
        acdb_device_table[SND_DEVICE_OUT_SPEAKER_REVERSE] = 131;
        acdb_device_table[SND_DEVICE_OUT_SPEAKER_AND_HDMI] = 131;
        acdb_device_table[SND_DEVICE_OUT_SPEAKER_AND_USB_HEADSET] = 131;
    }

    my_data->voice_feature_set = VOICE_FEATURE_SET_DEFAULT;
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
                                                    "acdb_loader_send_audio_cal_v2");
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

        my_data->acdb_get_default_app_type = (acdb_get_default_app_type_t)dlsym(
                                                    my_data->acdb_handle,
                                                    "acdb_loader_get_default_app_type");
        if (!my_data->acdb_get_default_app_type)
            ALOGE("%s: Could not find the symbol acdb_get_default_app_type from %s",
                  __func__, LIB_ACDB_LOADER);

        my_data->acdb_init = (acdb_init_t)dlsym(my_data->acdb_handle,
                                                    "acdb_loader_init_v2");
        if (my_data->acdb_init == NULL) {
            ALOGE("%s: dlsym error %s for acdb_loader_init_v2", __func__, dlerror());
            goto acdb_init_fail;
        }
        platform_acdb_init(my_data);
    }
    audio_extn_pm_vote();

acdb_init_fail:
    /* Initialize ACDB ID's */
    platform_info_init(PLATFORM_INFO_XML_PATH);

    /* init usb */
    audio_extn_usb_init(adev);

    /*init a2dp*/
    audio_extn_a2dp_init();

    /* update sound cards appropriately */
    audio_extn_usb_set_proxy_sound_card(adev->snd_card);

    /* Read one time ssr property */
    audio_extn_ssr_update_enabled();
    audio_extn_spkr_prot_init(adev);

    /* init dap hal */
    audio_extn_dap_hal_init(adev->snd_card);

    audio_extn_dolby_set_license(adev);
    audio_hwdep_send_cal(my_data);

    return my_data;
}

void platform_deinit(void *platform)
{
    struct platform_data *my_data = (struct platform_data *)platform;

    hw_info_deinit(my_data->hw_info);
    close_csd_client(my_data->csd);

    free(platform);
    /* deinit usb */
    audio_extn_usb_deinit();
    audio_extn_dap_hal_deinit();
}

int platform_is_acdb_initialized(void *platform)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    ALOGD("%s: acdb initialized %d\n", __func__, my_data->is_acdb_initialized);
    return my_data->is_acdb_initialized;
}

const char *platform_get_snd_device_name(snd_device_t snd_device)
{
    if (snd_device >= SND_DEVICE_MIN && snd_device < SND_DEVICE_MAX)
        return device_table[snd_device];
    else
        return "";
}

int platform_get_snd_device_name_extn(void *platform, snd_device_t snd_device,
                                      char *device_name)
{
    struct platform_data *my_data = (struct platform_data *)platform;

    if (snd_device >= SND_DEVICE_MIN && snd_device < SND_DEVICE_MAX) {
        strlcpy(device_name, device_table[snd_device], DEVICE_NAME_MAX_SIZE);
        hw_info_append_hw_type(my_data->hw_info, snd_device, device_name);
    } else {
        strlcpy(device_name, "", DEVICE_NAME_MAX_SIZE);
        return -EINVAL;
    }

    return 0;
}

void platform_add_backend_name(char *mixer_path, snd_device_t snd_device)
{
    if ((snd_device == SND_DEVICE_IN_BT_SCO_MIC) ||
         (snd_device == SND_DEVICE_IN_BT_SCO_MIC_NREC))
        strlcat(mixer_path, " bt-sco", MIXER_PATH_MAX_LENGTH);
    else if ((snd_device == SND_DEVICE_IN_BT_SCO_MIC_WB) ||
              (snd_device == SND_DEVICE_IN_BT_SCO_MIC_WB_NREC))
        strlcat(mixer_path, " bt-sco-wb", MIXER_PATH_MAX_LENGTH);
    else if(snd_device == SND_DEVICE_OUT_BT_SCO)
        strlcat(mixer_path, " bt-sco", MIXER_PATH_MAX_LENGTH);
    else if(snd_device == SND_DEVICE_OUT_BT_A2DP)
        strlcat(mixer_path, " bt-a2dp", MIXER_PATH_MAX_LENGTH);
    else if(snd_device == SND_DEVICE_OUT_SPEAKER_AND_BT_A2DP)
        strlcat(mixer_path, " speaker-and-bt-a2dp", MIXER_PATH_MAX_LENGTH);
    else if(snd_device == SND_DEVICE_OUT_BT_SCO_WB)
        strlcat(mixer_path, " bt-sco-wb", MIXER_PATH_MAX_LENGTH);
    else if (snd_device == SND_DEVICE_OUT_HDMI)
        strlcat(mixer_path, " hdmi", MIXER_PATH_MAX_LENGTH);
    else if (snd_device == SND_DEVICE_OUT_SPEAKER_AND_HDMI)
        strlcat(mixer_path, " speaker-and-hdmi", MIXER_PATH_MAX_LENGTH);
    else if (snd_device == SND_DEVICE_OUT_AFE_PROXY)
        strlcat(mixer_path, " afe-proxy", MIXER_PATH_MAX_LENGTH);
    else if (snd_device == SND_DEVICE_OUT_USB_HEADSET)
        strlcat(mixer_path, " usb-headphones", MIXER_PATH_MAX_LENGTH);
    else if (snd_device == SND_DEVICE_OUT_SPEAKER_AND_USB_HEADSET)
        strlcat(mixer_path, " speaker-and-usb-headphones",
                MIXER_PATH_MAX_LENGTH);
    else if (snd_device == SND_DEVICE_IN_USB_HEADSET_MIC)
        strlcat(mixer_path, " usb-headset-mic", MIXER_PATH_MAX_LENGTH);
    else if (snd_device == SND_DEVICE_IN_CAPTURE_FM)
        strlcat(mixer_path, " capture-fm", MIXER_PATH_MAX_LENGTH);
    else if (snd_device == SND_DEVICE_OUT_TRANSMISSION_FM)
        strlcat(mixer_path, " transmission-fm", MIXER_PATH_MAX_LENGTH);
}

int platform_get_pcm_device_id(audio_usecase_t usecase, int device_type)
{
    int device_id = -1;

    if (is_external_codec && is_misc_usecase(usecase)) {
        if (device_type == PCM_PLAYBACK)
            device_id = pcm_device_table_of_ext_codec[usecase][0];
        else
            device_id = pcm_device_table_of_ext_codec[usecase][1];
    } else {
        if (device_type == PCM_PLAYBACK)
            device_id = pcm_device_table[usecase][0];
        else
            device_id = pcm_device_table[usecase][1];
    }
    return device_id;
}

int platform_get_snd_device_index(char *snd_device_index_name)
{
    int ret = 0;
    int i;

    if (snd_device_index_name == NULL) {
        ALOGE("%s: snd_device_index_name is NULL", __func__);
        ret = -ENODEV;
        goto done;
    }

    for (i=0; i < SND_DEVICE_MAX; i++) {
        if(strcmp(snd_device_name_index[i].name, snd_device_index_name) == 0) {
            ret = snd_device_name_index[i].index;
            goto done;
        }
    }
    ALOGE("%s: Could not find index for snd_device_index_name = %s",
            __func__, snd_device_index_name);
    ret = -ENODEV;
done:
    return ret;
}

int platform_set_fluence_type(void *platform, char *value)
{
    int ret = 0;
    int fluence_type = FLUENCE_NONE;
    int fluence_flag = NONE_FLAG;
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;

    ALOGV("%s: fluence type:%d", __func__, my_data->fluence_type);

    /* only dual mic turn on and off is supported as of now through setparameters */
    if (!strncmp(AUDIO_PARAMETER_VALUE_DUALMIC,value, sizeof(AUDIO_PARAMETER_VALUE_DUALMIC))) {
        if (!strncmp("fluencepro", my_data->fluence_cap, sizeof("fluencepro")) ||
            !strncmp("fluence", my_data->fluence_cap, sizeof("fluence"))) {
            ALOGV("fluence dualmic feature enabled \n");
            fluence_type = FLUENCE_DUAL_MIC;
            fluence_flag = DMIC_FLAG;
        } else {
            ALOGE("%s: Failed to set DUALMIC", __func__);
            ret = -1;
            goto done;
        }
    } else if (!strncmp(AUDIO_PARAMETER_KEY_NO_FLUENCE, value, sizeof(AUDIO_PARAMETER_KEY_NO_FLUENCE))) {
        ALOGV("fluence disabled");
        fluence_type = FLUENCE_NONE;
    } else {
        ALOGE("Invalid fluence value : %s",value);
        ret = -1;
        goto done;
    }

    if (fluence_type != my_data->fluence_type) {
        ALOGV("%s: Updating fluence_type to :%d", __func__, fluence_type);
        my_data->fluence_type = fluence_type;
        adev->acdb_settings = (adev->acdb_settings & FLUENCE_MODE_CLEAR) | fluence_flag;
    }
done:
    return ret;
}

int platform_get_fluence_type(void *platform, char *value, uint32_t len)
{
    int ret = 0;
    struct platform_data *my_data = (struct platform_data *)platform;

    if (my_data->fluence_type == FLUENCE_QUAD_MIC) {
        strlcpy(value, "quadmic", len);
    } else if (my_data->fluence_type == FLUENCE_DUAL_MIC) {
        strlcpy(value, "dualmic", len);
    } else if (my_data->fluence_type == FLUENCE_NONE) {
        strlcpy(value, "none", len);
    } else
        ret = -1;

    return ret;
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

    acdb_device_table[snd_device] = acdb_id;
done:
    return ret;
}

int platform_get_default_app_type(void *platform)
{
    struct platform_data *my_data = (struct platform_data *)platform;

    if (my_data->acdb_get_default_app_type)
        return my_data->acdb_get_default_app_type();
    else
        return DEFAULT_APP_TYPE;
}

int platform_get_snd_device_acdb_id(snd_device_t snd_device)
{
    if ((snd_device < SND_DEVICE_MIN) || (snd_device >= SND_DEVICE_MAX)) {
        ALOGE("%s: Invalid snd_device = %d", __func__, snd_device);
        return -EINVAL;
    }
    return acdb_device_table[snd_device];
}

int platform_set_snd_device_bit_width(snd_device_t snd_device, unsigned int bit_width)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_get_snd_device_bit_width(snd_device_t snd_device)
{
    ALOGE("%s: Not implemented", __func__);
    return -ENOSYS;
}

int platform_send_audio_calibration(void *platform, struct audio_usecase *usecase,
                                    int app_type, int sample_rate)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int acdb_dev_id, acdb_dev_type;
    struct audio_device *adev = my_data->adev;
    int snd_device = SND_DEVICE_OUT_SPEAKER;

    if (usecase->type == PCM_PLAYBACK) {
        snd_device = usecase->out_snd_device;
        if(usecase->id != USECASE_AUDIO_PLAYBACK_OFFLOAD)
            app_type = APP_TYPE_SYSTEM_SOUNDS;
    } else if ((usecase->type == PCM_HFP_CALL) || (usecase->type == PCM_CAPTURE)) {
        snd_device = usecase->in_snd_device;
        app_type = APP_TYPE_GENERAL_RECORDING;
    }

    acdb_dev_id = acdb_device_table[snd_device];
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
        my_data->acdb_send_audio_cal(acdb_dev_id, acdb_dev_type, app_type,
                                     sample_rate);
    }
    return 0;
}

int platform_switch_voice_call_device_pre(void *platform)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    int ret = 0;

    if (my_data->csd != NULL &&
        my_data->adev->mode == AUDIO_MODE_IN_CALL) {
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

    acdb_rx_id = acdb_device_table[out_snd_device];
    acdb_tx_id = acdb_device_table[in_snd_device];

    if (my_data->csd != NULL) {
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
        acdb_rx_id = acdb_device_table[out_snd_device];
        acdb_tx_id = acdb_device_table[in_snd_device];

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

    acdb_rx_id = acdb_device_table[out_snd_device];
    acdb_tx_id = acdb_device_table[in_snd_device];

    if (my_data->csd != NULL) {
        if (acdb_rx_id > 0 && acdb_tx_id > 0) {
            ret = my_data->csd->enable_device(acdb_rx_id, acdb_tx_id,
                                              my_data->adev->acdb_settings);
            if (ret < 0) {
                ALOGE("%s: csd_enable_device, failed, error %d",
                      __func__, ret);
            }
        } else {
            ALOGE("%s: Incorrect ACDB IDs (rx: %d tx: %d)", __func__,
                  acdb_rx_id, acdb_tx_id);
        }
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

int platform_get_sample_rate(void *platform __unused, uint32_t *rate __unused)
{
    return 0;
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
    vol_index = (int)percent_to_index(volume, MIN_VOL_INDEX, MAX_VOL_INDEX);
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
        ret = my_data->csd->volume(ALL_SESSION_VSID, volume);
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
                              DEFAULT_VOLUME_RAMP_DURATION_MS};

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
        ret = my_data->csd->mic_mute(ALL_SESSION_VSID, state);
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

snd_device_t platform_get_output_snd_device(void *platform, audio_devices_t devices)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    struct audio_device *adev = my_data->adev;
    audio_mode_t mode = adev->mode;
    snd_device_t snd_device = SND_DEVICE_NONE;
#ifdef RECORD_PLAY_CONCURRENCY
    bool use_voip_out_devices = false;
    bool prop_rec_play_enabled = false;
    char recConcPropValue[PROPERTY_VALUE_MAX];

    if (property_get("rec.playback.conc.disabled", recConcPropValue, NULL)) {
        prop_rec_play_enabled = atoi(recConcPropValue) || !strncmp("true", recConcPropValue, 4);
    }
    use_voip_out_devices =  prop_rec_play_enabled &&
                        (my_data->rec_play_conc_set || adev->mode == AUDIO_MODE_IN_COMMUNICATION);
    ALOGV("platform_get_output_snd_device use_voip_out_devices : %d",use_voip_out_devices);
#endif

    audio_channel_mask_t channel_mask = (adev->active_input == NULL) ?
                                AUDIO_CHANNEL_IN_MONO : adev->active_input->channel_mask;
    int channel_count = popcount(channel_mask);

    ALOGV("%s: enter: output devices(%#x)", __func__, devices);
    if (devices == AUDIO_DEVICE_NONE ||
        devices & AUDIO_DEVICE_BIT_IN) {
        ALOGV("%s: Invalid output devices (%#x)", __func__, devices);
        goto exit;
    }

    if (popcount(devices) == 2) {
        if (devices == (AUDIO_DEVICE_OUT_WIRED_HEADPHONE |
                        AUDIO_DEVICE_OUT_SPEAKER)) {
            snd_device = SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES;
        } else if (devices == (AUDIO_DEVICE_OUT_WIRED_HEADSET |
                               AUDIO_DEVICE_OUT_SPEAKER)) {
            if (audio_extn_get_anc_enabled())
                snd_device = SND_DEVICE_OUT_SPEAKER_AND_ANC_HEADSET;
            else
                snd_device = SND_DEVICE_OUT_SPEAKER_AND_HEADPHONES;
        } else if (devices == (AUDIO_DEVICE_OUT_AUX_DIGITAL |
                               AUDIO_DEVICE_OUT_SPEAKER)) {
            snd_device = SND_DEVICE_OUT_SPEAKER_AND_HDMI;
        } else if (devices == (AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET |
                               AUDIO_DEVICE_OUT_SPEAKER)) {
            snd_device = SND_DEVICE_OUT_SPEAKER_AND_USB_HEADSET;
        } else if ((devices & AUDIO_DEVICE_OUT_SPEAKER) &&
                   (devices & AUDIO_DEVICE_OUT_ALL_A2DP)) {
            snd_device = SND_DEVICE_OUT_SPEAKER_AND_BT_A2DP;
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

    if ((mode == AUDIO_MODE_IN_CALL) ||
        voice_extn_compress_voip_is_active(adev)) {
        if (devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
            devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
            if ((adev->voice.tty_mode != TTY_MODE_OFF) &&
                !voice_extn_compress_voip_is_active(adev)) {
                switch (adev->voice.tty_mode) {
                case TTY_MODE_FULL:
                    snd_device = SND_DEVICE_OUT_VOICE_TTY_FULL_HEADPHONES;
                    break;
                case TTY_MODE_VCO:
                    snd_device = SND_DEVICE_OUT_VOICE_TTY_VCO_HEADPHONES;
                    break;
                case TTY_MODE_HCO:
                    snd_device = SND_DEVICE_OUT_VOICE_TTY_HCO_HANDSET;
                    break;
                default:
                    ALOGE("%s: Invalid TTY mode (%#x)",
                          __func__, adev->voice.tty_mode);
                }
            } else if (audio_extn_get_anc_enabled()) {
                if (audio_extn_should_use_fb_anc())
                    snd_device = SND_DEVICE_OUT_VOICE_ANC_FB_HEADSET;
                else
                    snd_device = SND_DEVICE_OUT_VOICE_ANC_HEADSET;
            } else {
                snd_device = SND_DEVICE_OUT_VOICE_HEADPHONES;
            }
        } else if (devices & AUDIO_DEVICE_OUT_ALL_SCO) {
            if (adev->bt_wb_speech_enabled)
                snd_device = SND_DEVICE_OUT_BT_SCO_WB;
            else
                snd_device = SND_DEVICE_OUT_BT_SCO;
        } else if (devices & AUDIO_DEVICE_OUT_SPEAKER) {
                if (my_data->is_wsa_speaker)
                    snd_device = SND_DEVICE_OUT_VOICE_SPEAKER_WSA;
                else
                    snd_device = SND_DEVICE_OUT_VOICE_SPEAKER;
        } else if (devices & AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET ||
                   devices & AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET) {
            snd_device = SND_DEVICE_OUT_USB_HEADSET;
        } else if (devices & AUDIO_DEVICE_OUT_FM_TX) {
            snd_device = SND_DEVICE_OUT_TRANSMISSION_FM;
        } else if (devices & AUDIO_DEVICE_OUT_EARPIECE) {
            if (audio_extn_should_use_handset_anc(channel_count))
                snd_device = SND_DEVICE_OUT_ANC_HANDSET;
            else
                snd_device = SND_DEVICE_OUT_VOICE_HANDSET;
        }
        if (snd_device != SND_DEVICE_NONE) {
            goto exit;
        }
    }

    if (devices & AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
        devices & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
        if (devices & AUDIO_DEVICE_OUT_WIRED_HEADSET
            && audio_extn_get_anc_enabled()) {
#ifdef RECORD_PLAY_CONCURRENCY
            if (use_voip_out_devices) {
                // ANC should be disabled for voip concurrency
                snd_device = SND_DEVICE_OUT_VOIP_HEADPHONES;
            } else
#endif
            {
                if (audio_extn_should_use_fb_anc())
                    snd_device = SND_DEVICE_OUT_ANC_FB_HEADSET;
                else
                    snd_device = SND_DEVICE_OUT_ANC_HEADSET;
            }
        } else {
#ifdef RECORD_PLAY_CONCURRENCY
            if (use_voip_out_devices)
                snd_device = SND_DEVICE_OUT_VOIP_HEADPHONES;
            else
#endif
                snd_device = SND_DEVICE_OUT_HEADPHONES;
        }
    } else if (devices & AUDIO_DEVICE_OUT_SPEAKER) {
#ifdef RECORD_PLAY_CONCURRENCY
        if (use_voip_out_devices) {
            snd_device = SND_DEVICE_OUT_VOIP_SPEAKER;
        } else
#endif
        {
            if (adev->speaker_lr_swap)
                snd_device = SND_DEVICE_OUT_SPEAKER_REVERSE;
            else
            {
                if (my_data->is_wsa_speaker)
                    snd_device = SND_DEVICE_OUT_SPEAKER_WSA;
                else
                    snd_device = SND_DEVICE_OUT_SPEAKER;
            }
        }
    } else if (devices & AUDIO_DEVICE_OUT_ALL_SCO) {
        if (adev->bt_wb_speech_enabled)
            snd_device = SND_DEVICE_OUT_BT_SCO_WB;
        else
            snd_device = SND_DEVICE_OUT_BT_SCO;
    } else if (devices & AUDIO_DEVICE_OUT_AUX_DIGITAL) {
        snd_device = SND_DEVICE_OUT_HDMI ;
    } else if (devices & AUDIO_DEVICE_OUT_ALL_A2DP) {
        snd_device = SND_DEVICE_OUT_BT_A2DP;
    } else if (devices & AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET ||
               devices & AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET) {
        ALOGD("%s: setting USB hadset channel capability(2) for Proxy", __func__);
        audio_extn_set_afe_proxy_channel_mixer(adev, 2);
        snd_device = SND_DEVICE_OUT_USB_HEADSET;
    } else if (devices & AUDIO_DEVICE_OUT_FM_TX) {
        snd_device = SND_DEVICE_OUT_TRANSMISSION_FM;
    } else if (devices & AUDIO_DEVICE_OUT_EARPIECE) {
#ifdef RECORD_PLAY_CONCURRENCY
        if (use_voip_out_devices)
            snd_device = SND_DEVICE_OUT_VOIP_HANDSET;
        else
#endif
            snd_device = SND_DEVICE_OUT_HANDSET;
    } else if (devices & AUDIO_DEVICE_OUT_PROXY) {
        channel_count = audio_extn_get_afe_proxy_channel_count();
        ALOGD("%s: setting sink capability(%d) for Proxy", __func__, channel_count);
        audio_extn_set_afe_proxy_channel_mixer(adev, channel_count);
        snd_device = SND_DEVICE_OUT_AFE_PROXY;
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

    ALOGV("%s: enter: out_device(%#x) in_device(%#x)",
          __func__, out_device, in_device);
    if ((out_device != AUDIO_DEVICE_NONE) && ((mode == AUDIO_MODE_IN_CALL) ||
        voice_extn_compress_voip_is_active(adev) || audio_extn_hfp_is_active(adev))) {
        if ((adev->voice.tty_mode != TTY_MODE_OFF) &&
            !voice_extn_compress_voip_is_active(adev)) {
            if (out_device & AUDIO_DEVICE_OUT_WIRED_HEADPHONE ||
                out_device & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
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
                    ALOGE("%s: Invalid TTY mode (%#x)",
                          __func__, adev->voice.tty_mode);
                }
                goto exit;
            }
        }
        if (out_device & AUDIO_DEVICE_OUT_EARPIECE ||
            out_device & AUDIO_DEVICE_OUT_WIRED_HEADPHONE) {
            if (out_device & AUDIO_DEVICE_OUT_EARPIECE &&
                audio_extn_should_use_handset_anc(channel_count) &&
                my_data->fluence_type != FLUENCE_NONE) {
                snd_device = SND_DEVICE_IN_VOICE_FLUENCE_DMIC_AANC;
                adev->acdb_settings |= DMIC_FLAG;
                ALOGD("Selecting AANC, Fluence combo device");
            } else if (out_device & AUDIO_DEVICE_OUT_EARPIECE &&
                audio_extn_should_use_handset_anc(channel_count)) {
                snd_device = SND_DEVICE_IN_AANC_HANDSET_MIC;
            } else if (my_data->fluence_type == FLUENCE_NONE ||
                my_data->fluence_in_voice_call == false) {
                snd_device = SND_DEVICE_IN_HANDSET_MIC;
                if (audio_extn_hfp_is_active(adev))
                    platform_set_echo_reference(adev->platform, true);
            } else {
                snd_device = SND_DEVICE_IN_VOICE_DMIC;
                adev->acdb_settings |= DMIC_FLAG;
            }
        } else if (out_device & AUDIO_DEVICE_OUT_WIRED_HEADSET) {
            snd_device = SND_DEVICE_IN_VOICE_HEADSET_MIC;
               if (audio_extn_hfp_is_active(adev))
                   platform_set_echo_reference(adev->platform, true);
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
        } else if (out_device & AUDIO_DEVICE_OUT_SPEAKER) {
            if (my_data->fluence_type != FLUENCE_NONE &&
                my_data->fluence_in_voice_call &&
                my_data->fluence_in_spkr_mode) {
                if(my_data->fluence_type & FLUENCE_QUAD_MIC) {
                    adev->acdb_settings |= QMIC_FLAG;
                    snd_device = SND_DEVICE_IN_VOICE_SPEAKER_QMIC;
                } else {
                    adev->acdb_settings |= DMIC_FLAG;
                    if (my_data->fluence_mode == FLUENCE_BROADSIDE)
                       snd_device = SND_DEVICE_IN_VOICE_SPEAKER_DMIC_BROADSIDE;
                    else
                       snd_device = SND_DEVICE_IN_VOICE_SPEAKER_DMIC;
                }
            } else {
                snd_device = SND_DEVICE_IN_VOICE_SPEAKER_MIC;
                if (audio_extn_hfp_is_active(adev))
                    platform_set_echo_reference(adev->platform, true);
            }
        }
    } else if (source == AUDIO_SOURCE_CAMCORDER) {
        if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC ||
            in_device & AUDIO_DEVICE_IN_BACK_MIC) {
            if (my_data->fluence_type & FLUENCE_DUAL_MIC &&
                channel_count == 2)
                snd_device = SND_DEVICE_IN_HANDSET_STEREO_DMIC;
            else
                snd_device = SND_DEVICE_IN_CAMCORDER_MIC;
        }
    } else if (source == AUDIO_SOURCE_VOICE_RECOGNITION) {
        if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC) {
           if (channel_count == 2) {
                snd_device = SND_DEVICE_IN_VOICE_REC_DMIC_STEREO;
                adev->acdb_settings |= DMIC_FLAG;
            } else if (adev->active_input->enable_ns)
                snd_device = SND_DEVICE_IN_VOICE_REC_MIC_NS;
            else if (my_data->fluence_type != FLUENCE_NONE &&
                     my_data->fluence_in_voice_rec) {
                snd_device = SND_DEVICE_IN_VOICE_REC_DMIC_FLUENCE;
                adev->acdb_settings |= DMIC_FLAG;
            } else {
                snd_device = SND_DEVICE_IN_VOICE_REC_MIC;
            }
        }
    } else if ((source == AUDIO_SOURCE_VOICE_COMMUNICATION) ||
              (mode == AUDIO_MODE_IN_COMMUNICATION)) {
        if (out_device & AUDIO_DEVICE_OUT_SPEAKER)
            in_device = AUDIO_DEVICE_IN_BACK_MIC;
        if (adev->active_input) {
            if (adev->active_input->enable_aec &&
                    adev->active_input->enable_ns) {
                if (in_device & AUDIO_DEVICE_IN_BACK_MIC) {
                    if (my_data->fluence_in_spkr_mode) {
                        if (my_data->fluence_type & FLUENCE_QUAD_MIC) {
                            snd_device = SND_DEVICE_IN_SPEAKER_QMIC_AEC_NS;
                        } else if (my_data->fluence_type & FLUENCE_DUAL_MIC) {
                            if (my_data->fluence_mode == FLUENCE_BROADSIDE)
                                snd_device = SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS_BROADSIDE;
                            else
                                snd_device = SND_DEVICE_IN_SPEAKER_DMIC_AEC_NS;
                        }
                        adev->acdb_settings |= DMIC_FLAG;
                    } else
                        snd_device = SND_DEVICE_IN_SPEAKER_MIC_AEC_NS;
                } else if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC) {
                    if (my_data->fluence_type & FLUENCE_DUAL_MIC) {
                        snd_device = SND_DEVICE_IN_HANDSET_DMIC_AEC_NS;
                        adev->acdb_settings |= DMIC_FLAG;
                    } else
                        snd_device = SND_DEVICE_IN_HANDSET_MIC_AEC_NS;
                } else if (in_device & AUDIO_DEVICE_IN_WIRED_HEADSET) {
                    snd_device = SND_DEVICE_IN_HEADSET_MIC_FLUENCE;
                }
                platform_set_echo_reference(adev->platform, true);
            } else if (adev->active_input->enable_aec) {
                if (in_device & AUDIO_DEVICE_IN_BACK_MIC) {
                    if (my_data->fluence_in_spkr_mode) {
                        if (my_data->fluence_type & FLUENCE_QUAD_MIC) {
                            snd_device = SND_DEVICE_IN_SPEAKER_QMIC_AEC;
                        } else if (my_data->fluence_type & FLUENCE_DUAL_MIC) {
                            if (my_data->fluence_mode == FLUENCE_BROADSIDE)
                                snd_device = SND_DEVICE_IN_SPEAKER_DMIC_AEC_BROADSIDE;
                            else
                                snd_device = SND_DEVICE_IN_SPEAKER_DMIC_AEC;
                        }
                        adev->acdb_settings |= DMIC_FLAG;
                    } else
                        snd_device = SND_DEVICE_IN_SPEAKER_MIC_AEC;
                } else if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC) {
                    if (my_data->fluence_type & FLUENCE_DUAL_MIC) {
                        snd_device = SND_DEVICE_IN_HANDSET_DMIC_AEC;
                        adev->acdb_settings |= DMIC_FLAG;
                    } else
                        snd_device = SND_DEVICE_IN_HANDSET_MIC_AEC;
                } else if (in_device & AUDIO_DEVICE_IN_WIRED_HEADSET) {
                    snd_device = SND_DEVICE_IN_HEADSET_MIC_FLUENCE;
                }
                platform_set_echo_reference(adev->platform, true);
            } else if (adev->active_input->enable_ns) {
                if (in_device & AUDIO_DEVICE_IN_BACK_MIC) {
                    if (my_data->fluence_in_spkr_mode) {
                        if (my_data->fluence_type & FLUENCE_QUAD_MIC) {
                            snd_device = SND_DEVICE_IN_SPEAKER_QMIC_NS;
                        } else if (my_data->fluence_type & FLUENCE_DUAL_MIC) {
                            if (my_data->fluence_mode == FLUENCE_BROADSIDE)
                                snd_device = SND_DEVICE_IN_SPEAKER_DMIC_NS_BROADSIDE;
                            else
                                snd_device = SND_DEVICE_IN_SPEAKER_DMIC_NS;
                        }
                        adev->acdb_settings |= DMIC_FLAG;
                    } else
                        snd_device = SND_DEVICE_IN_SPEAKER_MIC_NS;
                } else if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC) {
                    if (my_data->fluence_type & FLUENCE_DUAL_MIC) {
                        snd_device = SND_DEVICE_IN_HANDSET_DMIC_NS;
                        adev->acdb_settings |= DMIC_FLAG;
                    } else
                        snd_device = SND_DEVICE_IN_HANDSET_MIC_NS;
                } else if (in_device & AUDIO_DEVICE_IN_WIRED_HEADSET) {
                    snd_device = SND_DEVICE_IN_HEADSET_MIC_FLUENCE;
                }
                platform_set_echo_reference(adev->platform,false);
            } else
                platform_set_echo_reference(adev->platform, false);
        }
    } else if (source == AUDIO_SOURCE_MIC) {
        if (in_device & AUDIO_DEVICE_IN_BUILTIN_MIC &&
                channel_count == 1 ) {
            ALOGD("Record path active");
            if(my_data->fluence_in_audio_rec) {
                if(my_data->fluence_type & FLUENCE_QUAD_MIC) {
                    ALOGD(" snd_device = SND_DEVICE_IN_HANDSET_QMIC");
                    snd_device = SND_DEVICE_IN_HANDSET_QMIC;
                    platform_set_echo_reference(adev->platform, true);
                } else if (my_data->fluence_type & FLUENCE_DUAL_MIC) {
                    snd_device = SND_DEVICE_IN_HANDSET_DMIC;
                    platform_set_echo_reference(adev->platform, true);
                }
            }
        }
    } else if (source == AUDIO_SOURCE_FM_TUNER) {
        snd_device = SND_DEVICE_IN_CAPTURE_FM;
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
            if (audio_extn_ssr_get_enabled() && channel_count == 6)
                snd_device = SND_DEVICE_IN_QUAD_MIC;
            else if (channel_count == 2)
                snd_device = SND_DEVICE_IN_HANDSET_STEREO_DMIC;
            else
                snd_device = SND_DEVICE_IN_HANDSET_MIC;
        } else if (in_device & AUDIO_DEVICE_IN_BACK_MIC) {
            snd_device = SND_DEVICE_IN_SPEAKER_MIC;
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
        } else if (in_device & AUDIO_DEVICE_IN_ANLG_DOCK_HEADSET ||
                   in_device & AUDIO_DEVICE_IN_DGTL_DOCK_HEADSET) {
            snd_device = SND_DEVICE_IN_USB_HEADSET_MIC;
        } else if (in_device & AUDIO_DEVICE_IN_FM_TUNER) {
            snd_device = SND_DEVICE_IN_CAPTURE_FM;
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
        } else if (out_device & AUDIO_DEVICE_OUT_SPEAKER) {
            if (channel_count > 1)
                snd_device = SND_DEVICE_IN_SPEAKER_STEREO_DMIC;
            else
                snd_device = SND_DEVICE_IN_SPEAKER_MIC;
        } else if (out_device & AUDIO_DEVICE_OUT_WIRED_HEADPHONE) {
            snd_device = SND_DEVICE_IN_HANDSET_MIC;
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
        } else if (out_device & AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET ||
                   out_device & AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET) {
            snd_device = SND_DEVICE_IN_USB_HEADSET_MIC;
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

static int platform_set_slowtalk(struct platform_data *my_data, bool state)
{
    int ret = 0;
    struct audio_device *adev = my_data->adev;
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = "Slowtalk Enable";
    uint32_t set_values[ ] = {0,
                              ALL_SESSION_VSID};

    set_values[0] = state;
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        ret = -EINVAL;
    } else {
        ALOGV("Setting slowtalk state: %d", state);
        ret = mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));
        my_data->slowtalk = state;
    }

    if (my_data->csd != NULL) {
        ret = my_data->csd->slow_talk(ALL_SESSION_VSID, state);
        if (ret < 0) {
            ALOGE("%s: csd_client_disable_device, failed, error %d",
                  __func__, ret);
        }
    }
    return ret;
}

static int set_hd_voice(struct platform_data *my_data, bool state)
{
    struct audio_device *adev = my_data->adev;
    struct mixer_ctl *ctl;
    const char *mixer_ctl_name = "HD Voice Enable";
    int ret = 0;
    uint32_t set_values[ ] = {0,
                              ALL_SESSION_VSID};

    set_values[0] = state;
    ctl = mixer_get_ctl_by_name(adev->mixer, mixer_ctl_name);
    if (!ctl) {
        ALOGE("%s: Could not get ctl for mixer cmd - %s",
              __func__, mixer_ctl_name);
        ret = -EINVAL;
    } else {
        ALOGV("Setting HD Voice state: %d", state);
        ret = mixer_ctl_set_array(ctl, set_values, ARRAY_SIZE(set_values));
        my_data->hd_voice = state;
    }

    return ret;
}

int platform_set_parameters(void *platform, struct str_parms *parms)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    char *str;
    char value[256] = {0};
    int val;
    int ret = 0, err;
    char *kv_pairs = NULL;

    kv_pairs = str_parms_to_str(parms);
    ALOGV("%s: enter: - %s", __func__, kv_pairs);
    free(kv_pairs);

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SLOWTALK, value, sizeof(value));
    if (err >= 0) {
        bool state = false;
        if (!strncmp("true", value, sizeof("true"))) {
            state = true;
        }

        str_parms_del(parms, AUDIO_PARAMETER_KEY_SLOWTALK);
        ret = platform_set_slowtalk(my_data, state);
        if (ret)
            ALOGE("%s: Failed to set slow talk err: %d", __func__, ret);
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_HD_VOICE, value, sizeof(value));
    if (err >= 0) {
        bool state = false;
        if (!strncmp("true", value, sizeof("true"))) {
            state = true;
        }

        str_parms_del(parms, AUDIO_PARAMETER_KEY_HD_VOICE);
        if (my_data->hd_voice != state) {
            ret = set_hd_voice(my_data, state);
            if (ret)
                ALOGE("%s: Failed to set HD voice err: %d", __func__, ret);
        } else {
            ALOGV("%s: HD Voice already set to %d", __func__, state);
        }
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_VOLUME_BOOST,
                            value, sizeof(value));
    if (err >= 0) {
        str_parms_del(parms, AUDIO_PARAMETER_KEY_VOLUME_BOOST);

        if (my_data->acdb_reload_vocvoltable == NULL) {
            ALOGE("%s: acdb_reload_vocvoltable is NULL", __func__);
        } else if (!strcmp(value, "on")) {
            if (!my_data->acdb_reload_vocvoltable(VOICE_FEATURE_SET_VOLUME_BOOST)) {
                my_data->voice_feature_set = 1;
            }
        } else {
            if (!my_data->acdb_reload_vocvoltable(VOICE_FEATURE_SET_DEFAULT)) {
                my_data->voice_feature_set = 0;
            }
        }
    }

#ifdef RECORD_PLAY_CONCURRENCY
    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_REC_PLAY_CONC, value, sizeof(value));
    if (err >= 0) {
        if (!strncmp("true", value, sizeof("true"))) {
            ALOGD("setting record playback concurrency to true");
            my_data->rec_play_conc_set = true;
        } else {
            ALOGD("setting record playback concurrency to false");
            my_data->rec_play_conc_set = false;
        }
    }
#endif
    ALOGV("%s: exit with code(%d)", __func__, ret);
    return ret;
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

int platform_update_lch(void *platform, struct voice_session *session,
                        enum voice_lch_mode lch_mode)
{
    int ret = 0;
    struct platform_data *my_data = (struct platform_data *)platform;

    if ((my_data->csd != NULL) && (my_data->csd->set_lch != NULL))
        ret = my_data->csd->set_lch(session->vsid, lch_mode);
    else
        ret = pcm_ioctl(session->pcm_tx, SNDRV_VOICE_IOCTL_LCH, &lch_mode);

    return ret;
}

void platform_get_parameters(void *platform,
                            struct str_parms *query,
                            struct str_parms *reply)
{
    struct platform_data *my_data = (struct platform_data *)platform;
    char *str = NULL;
    char value[256] = {0};
    int ret;
    char *kv_pairs = NULL;
    char propValue[PROPERTY_VALUE_MAX]={0};
    bool prop_playback_enabled = false;

    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_SLOWTALK,
                            value, sizeof(value));
    if (ret >= 0) {
        str_parms_add_str(reply, AUDIO_PARAMETER_KEY_SLOWTALK,
                          my_data->slowtalk?"true":"false");
    }

    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_HD_VOICE,
                            value, sizeof(value));
    if (ret >= 0) {
        str_parms_add_str(reply, AUDIO_PARAMETER_KEY_HD_VOICE,
                          my_data->hd_voice?"true":"false");
    }

    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_VOLUME_BOOST,
                            value, sizeof(value));
    if (ret >= 0) {
        if (my_data->voice_feature_set == VOICE_FEATURE_SET_VOLUME_BOOST) {
            strlcpy(value, "on", sizeof(value));
        } else {
            strlcpy(value, "off", sizeof(value));
        }

        str_parms_add_str(reply, AUDIO_PARAMETER_KEY_VOLUME_BOOST, value);
    }

    ret = str_parms_get_str(query, AUDIO_PARAMETER_IS_HW_DECODER_SESSION_ALLOWED,
                                    value, sizeof(value));
    if (ret >= 0) {
        int isallowed = 1; /*true*/

        if (property_get("voice.playback.conc.disabled", propValue, NULL)) {
            prop_playback_enabled = atoi(propValue) ||
                !strncmp("true", propValue, 4);
        }

        if (prop_playback_enabled && (voice_is_in_call(my_data->adev) ||
             (SND_CARD_STATE_OFFLINE == get_snd_card_state(my_data->adev)))) {
            char *decoder_mime_type = value;

            //check if unsupported mime type or not
            if(decoder_mime_type) {
                int i = 0;
                for (i = 0; i < sizeof(dsp_only_decoders_mime)/sizeof(dsp_only_decoders_mime[0]); i++) {
                    if (!strncmp(decoder_mime_type, dsp_only_decoders_mime[i],
                    strlen(dsp_only_decoders_mime[i]))) {
                       ALOGD("Rejecting request for DSP only session from HAL during voice call/SSR state");
                       isallowed = 0;
                       break;
                    }
                }
            }
        }
        str_parms_add_int(reply, AUDIO_PARAMETER_IS_HW_DECODER_SESSION_ALLOWED, isallowed);
    }


    /* Handle audio calibration keys */
    kv_pairs = str_parms_to_str(reply);
    ALOGV("%s: exit: returns - %s", __func__, kv_pairs);
    free(kv_pairs);
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

int platform_update_usecase_from_source(int source, int usecase)
{
    ALOGV("%s: input source :%d", __func__, source);
    if(source == AUDIO_SOURCE_FM_TUNER)
        usecase = USECASE_AUDIO_RECORD_FM_VIRTUAL;
    return usecase;
}

bool platform_listen_device_needs_event(snd_device_t snd_device)
{
    bool needs_event = false;

    if ((snd_device >= SND_DEVICE_IN_BEGIN) &&
        (snd_device < SND_DEVICE_IN_END) &&
        (snd_device != SND_DEVICE_IN_CAPTURE_FM) &&
        (snd_device != SND_DEVICE_IN_CAPTURE_VI_FEEDBACK))
        needs_event = true;

    return needs_event;
}

bool platform_listen_usecase_needs_event(audio_usecase_t uc_id)
{
    bool needs_event = false;

    switch(uc_id){
    /* concurrent playback usecases needs event */
    case USECASE_AUDIO_PLAYBACK_DEEP_BUFFER:
    case USECASE_AUDIO_PLAYBACK_MULTI_CH:
    case USECASE_AUDIO_PLAYBACK_OFFLOAD:
        needs_event = true;
        break;
    /* concurrent playback in low latency allowed */
    case USECASE_AUDIO_PLAYBACK_LOW_LATENCY:
        break;
    /* concurrent playback FM needs event */
    case USECASE_AUDIO_PLAYBACK_FM:
        needs_event = true;
        break;

    /* concurrent capture usecases, no event, capture handled by device
    *  USECASE_AUDIO_RECORD:
    *  USECASE_AUDIO_RECORD_COMPRESS:
    *  USECASE_AUDIO_RECORD_LOW_LATENCY:

    *  USECASE_VOICE_CALL:
    *  USECASE_VOICE2_CALL:
    *  USECASE_VOLTE_CALL:
    *  USECASE_QCHAT_CALL:
    *  USECASE_VOWLAN_CALL:
    *  USECASE_COMPRESS_VOIP_CALL:
    *  USECASE_AUDIO_RECORD_FM_VIRTUAL:
    *  USECASE_INCALL_REC_UPLINK:
    *  USECASE_INCALL_REC_DOWNLINK:
    *  USECASE_INCALL_REC_UPLINK_AND_DOWNLINK:
    *  USECASE_INCALL_REC_UPLINK_COMPRESS:
    *  USECASE_INCALL_REC_DOWNLINK_COMPRESS:
    *  USECASE_INCALL_REC_UPLINK_AND_DOWNLINK_COMPRESS:
    *  USECASE_INCALL_MUSIC_UPLINK:
    *  USECASE_INCALL_MUSIC_UPLINK2:
    *  USECASE_AUDIO_SPKR_CALIB_RX:
    *  USECASE_AUDIO_SPKR_CALIB_TX:
    */
    default:
        ALOGV("%s:usecase_id[%d} no need to raise event.", __func__, uc_id);
    }
    return needs_event;
}

bool platform_sound_trigger_device_needs_event(snd_device_t snd_device)
{
    bool needs_event = false;

    if ((snd_device >= SND_DEVICE_IN_BEGIN) &&
        (snd_device < SND_DEVICE_IN_END) &&
        (snd_device != SND_DEVICE_IN_CAPTURE_FM) &&
        (snd_device != SND_DEVICE_IN_CAPTURE_VI_FEEDBACK))
        needs_event = true;

    return needs_event;
}

bool platform_sound_trigger_usecase_needs_event(audio_usecase_t uc_id)
{
    bool needs_event = false;

    switch(uc_id){
    /* concurrent playback usecases needs event */
    case USECASE_AUDIO_PLAYBACK_DEEP_BUFFER:
    case USECASE_AUDIO_PLAYBACK_MULTI_CH:
    case USECASE_AUDIO_PLAYBACK_OFFLOAD:
        needs_event = true;
        break;
    /* concurrent playback in low latency allowed */
    case USECASE_AUDIO_PLAYBACK_LOW_LATENCY:
        break;
    /* concurrent playback FM needs event */
    case USECASE_AUDIO_PLAYBACK_FM:
        needs_event = true;
        break;

    /* concurrent capture usecases, no event, capture handled by device
    *  USECASE_AUDIO_RECORD:
    *  USECASE_AUDIO_RECORD_COMPRESS:
    *  USECASE_AUDIO_RECORD_LOW_LATENCY:

    *  USECASE_VOICE_CALL:
    *  USECASE_VOICE2_CALL:
    *  USECASE_VOLTE_CALL:
    *  USECASE_QCHAT_CALL:
    *  USECASE_VOWLAN_CALL:
    *  USECASE_COMPRESS_VOIP_CALL:
    *  USECASE_AUDIO_RECORD_FM_VIRTUAL:
    *  USECASE_INCALL_REC_UPLINK:
    *  USECASE_INCALL_REC_DOWNLINK:
    *  USECASE_INCALL_REC_UPLINK_AND_DOWNLINK:
    *  USECASE_INCALL_REC_UPLINK_COMPRESS:
    *  USECASE_INCALL_REC_DOWNLINK_COMPRESS:
    *  USECASE_INCALL_REC_UPLINK_AND_DOWNLINK_COMPRESS:
    *  USECASE_INCALL_MUSIC_UPLINK:
    *  USECASE_INCALL_MUSIC_UPLINK2:
    *  USECASE_AUDIO_SPKR_CALIB_RX:
    *  USECASE_AUDIO_SPKR_CALIB_TX:
    */
    default:
        ALOGV("%s:usecase_id[%d] no need to raise event.", __func__, uc_id);
    }
    return needs_event;
}

/* Read  offload buffer size from a property.
 * If value is not power of 2  round it to
 * power of 2.
 */
uint32_t platform_get_compress_offload_buffer_size(audio_offload_info_t* info)
{
    char value[PROPERTY_VALUE_MAX] = {0};
    uint32_t fragment_size = COMPRESS_OFFLOAD_FRAGMENT_SIZE;
    if((property_get("audio.offload.buffer.size.kb", value, "")) &&
            atoi(value)) {
        fragment_size =  atoi(value) * 1024;
    }

    if (info != NULL && info->has_video && info->is_streaming) {
        fragment_size = COMPRESS_OFFLOAD_FRAGMENT_SIZE_FOR_AV_STREAMING;
        ALOGV("%s: offload fragment size reduced for AV streaming to %d",
               __func__, fragment_size);
    }

    fragment_size = ALIGN( fragment_size, 1024);

    if(fragment_size < MIN_COMPRESS_OFFLOAD_FRAGMENT_SIZE)
        fragment_size = MIN_COMPRESS_OFFLOAD_FRAGMENT_SIZE;
    else if(fragment_size > MAX_COMPRESS_OFFLOAD_FRAGMENT_SIZE)
        fragment_size = MAX_COMPRESS_OFFLOAD_FRAGMENT_SIZE;
    ALOGV("%s: fragment_size %d", __func__, fragment_size);
    return fragment_size;
}

uint32_t platform_get_pcm_offload_buffer_size(audio_offload_info_t* info)
{
    uint32_t fragment_size = MIN_PCM_OFFLOAD_FRAGMENT_SIZE;
    uint32_t bits_per_sample = 16;

    if (info->format == AUDIO_FORMAT_PCM_24_BIT_OFFLOAD) {
        bits_per_sample = 32;
    }

    if (!info->has_video) {
        fragment_size = MAX_PCM_OFFLOAD_FRAGMENT_SIZE;

    } else if (info->has_video && info->is_streaming) {
        fragment_size = (PCM_OFFLOAD_BUFFER_DURATION_FOR_AV_STREAMING
                                     * info->sample_rate
                                     * bits_per_sample
                                     * popcount(info->channel_mask))/1000;

    } else if (info->has_video) {
        fragment_size = (PCM_OFFLOAD_BUFFER_DURATION_FOR_AV
                                     * info->sample_rate
                                     * bits_per_sample
                                     * popcount(info->channel_mask))/1000;
    }

    fragment_size = ALIGN( fragment_size, 1024);

    if(fragment_size < MIN_PCM_OFFLOAD_FRAGMENT_SIZE)
        fragment_size = MIN_PCM_OFFLOAD_FRAGMENT_SIZE;
    else if(fragment_size > MAX_PCM_OFFLOAD_FRAGMENT_SIZE)
        fragment_size = MAX_PCM_OFFLOAD_FRAGMENT_SIZE;

    ALOGV("%s: fragment_size %d", __func__, fragment_size);
    return fragment_size;
}

void platform_get_device_to_be_id_map(int **device_to_be_id, int *length)
{
     *device_to_be_id = msm_device_to_be_id;
     *length = msm_be_id_array_len;
}

bool platform_check_24_bit_support() {
    return false;
}

bool platform_check_and_set_codec_backend_cfg(struct audio_device* adev __unused,
                                              struct audio_usecase *usecase __unused)
{
    return false;
}

int platform_get_usecase_index(const char * usecase __unused)
{
    return -ENOSYS;
}

int platform_set_usecase_pcm_id(audio_usecase_t usecase __unused, int32_t type __unused,
                                int32_t pcm_id __unused)
{
    return -ENOSYS;
}

int platform_set_snd_device_backend(snd_device_t snd_device __unused,
                                    const char * backend __unused)
{
    return -ENOSYS;
}

int platform_get_subsys_image_name(char *buf)
{
    strlcpy(buf, PLATFORM_IMAGE_NAME, sizeof(PLATFORM_IMAGE_NAME));
    return 0;
}

/*
 * This is a lookup table to map android audio input device to audio h/w interface (backend).
 * The table can be extended for other input devices by adding appropriate entries.
 * The audio interface for a particular input device need to be added in
 * audio_platform_info.xml file.
 */
struct audio_device_to_audio_interface audio_device_to_interface_table[] = {
    {AUDIO_DEVICE_IN_BUILTIN_MIC, ENUM_TO_STRING(AUDIO_DEVICE_IN_BUILTIN_MIC), ""},
    {AUDIO_DEVICE_IN_BACK_MIC, ENUM_TO_STRING(AUDIO_DEVICE_IN_BACK_MIC), ""},
};

int audio_device_to_interface_table_len  =
    sizeof(audio_device_to_interface_table) / sizeof(audio_device_to_interface_table[0]);

int platform_set_audio_device_interface(const char * device_name,
                                        const char *intf_name,
                                        const char *codec_type)
{
    int ret = 0;
    int i;

    if (device_name == NULL || intf_name == NULL || codec_type == NULL) {
        ALOGE("%s: Invalid input", __func__);

        ret = -EINVAL;
        goto done;
    }

    ALOGD("%s: Enter, device name:%s, intf name:%s, codec_type:%s", __func__,
                            device_name, intf_name, codec_type);

    size_t device_name_len = strlen(device_name);
    for (i = 0; i < audio_device_to_interface_table_len; i++) {
        char* name = audio_device_to_interface_table[i].device_name;
        size_t name_len = strlen(name);
        if ((name_len == device_name_len) &&
            (strncmp(device_name, name, name_len) == 0)) {
            if (is_external_codec &&
               (strncmp(codec_type, "external", strlen(codec_type)) == 0)) {
                ALOGD("%s: Matched device name:%s, overwrite intf name with %s",
                  __func__, device_name, intf_name);

                strlcpy(audio_device_to_interface_table[i].interface_name, intf_name,
                    sizeof(audio_device_to_interface_table[i].interface_name));
            } else if (!is_external_codec &&
                       (strncmp(codec_type, "internal", strlen(codec_type)) == 0)) {
                ALOGD("%s: Matched device name:%s, overwrite intf name with %s",
                  __func__, device_name, intf_name);

                strlcpy(audio_device_to_interface_table[i].interface_name, intf_name,
                    sizeof(audio_device_to_interface_table[i].interface_name));
            } else
                ALOGE("Invalid codec_type specified. Ignoring this interface entry.");
            goto done;
        }
    }
    ALOGE("%s: Could not find matching device name %s",
            __func__, device_name);

    ret = -EINVAL;

done:
    return ret;
}
