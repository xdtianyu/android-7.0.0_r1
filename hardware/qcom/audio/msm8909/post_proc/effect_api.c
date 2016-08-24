/*
 * Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#define LOG_TAG "offload_effect_api"
#define LOG_NDEBUG 0
//#define VERY_VERY_VERBOSE_LOGGING
#ifdef VERY_VERY_VERBOSE_LOGGING
#define ALOGVV ALOGV
#else
#define ALOGVV(a...) do { } while(0)
#endif

#include <stdbool.h>
#include <cutils/log.h>
#include <tinyalsa/asoundlib.h>
#include <sound/audio_effects.h>
#include <errno.h>

#include "effect_api.h"

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])

typedef enum eff_mode {
    OFFLOAD,
} eff_mode_t;

#define OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL 19
const int map_eq_opensl_preset_2_offload_preset[] = {
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL,   /* Normal Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+1, /* Classical Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+2, /* Dance Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+3, /* Flat Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+4, /* Folk Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+5, /* Heavy Metal Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+6, /* Hip Hop Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+7, /* Jazz Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+8, /* Pop Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+9, /* Rock Preset */
    OFFLOAD_PRESET_START_OFFSET_FOR_OPENSL+10 /* FX Booster */
};

const int map_reverb_opensl_preset_2_offload_preset
                  [NUM_OSL_REVERB_PRESETS_SUPPORTED][2] = {
    {1, 15},
    {2, 16},
    {3, 17},
    {4, 18},
    {5, 3},
    {6, 20}
};

int offload_update_mixer_and_effects_ctl(int card, int device_id,
                                         struct mixer *mixer,
                                         struct mixer_ctl *ctl)
{
    char mixer_string[128];

    snprintf(mixer_string, sizeof(mixer_string),
             "%s %d", "Audio Effects Config", device_id);
    ALOGV("%s: mixer_string: %s", __func__, mixer_string);
    mixer = mixer_open(card);
    if (!mixer) {
        ALOGE("Failed to open mixer");
        ctl = NULL;
        return -EINVAL;
    } else {
        ctl = mixer_get_ctl_by_name(mixer, mixer_string);
        if (!ctl) {
            ALOGE("mixer_get_ctl_by_name failed");
            mixer_close(mixer);
            mixer = NULL;
            return -EINVAL;
        }
    }
    ALOGV("mixer: %p, ctl: %p", mixer, ctl);
    return 0;
}

void offload_close_mixer(struct mixer *mixer)
{
    mixer_close(mixer);
}

void offload_bassboost_set_device(struct bass_boost_params *bassboost,
                                  uint32_t device)
{
    ALOGVV("%s: device 0x%x", __func__, device);
    bassboost->device = device;
}

void offload_bassboost_set_enable_flag(struct bass_boost_params *bassboost,
                                       bool enable)
{
    ALOGVV("%s: enable=%d", __func__, (int)enable);
    bassboost->enable_flag = enable;
}

int offload_bassboost_get_enable_flag(struct bass_boost_params *bassboost)
{
    ALOGVV("%s: enable=%d", __func__, (int)bassboost->enable_flag);
    return bassboost->enable_flag;
}

void offload_bassboost_set_strength(struct bass_boost_params *bassboost,
                                    int strength)
{
    ALOGVV("%s: strength %d", __func__, strength);
    bassboost->strength = strength;
}

void offload_bassboost_set_mode(struct bass_boost_params *bassboost,
                                int mode)
{
    ALOGVV("%s: mode %d", __func__, mode);
    bassboost->mode = mode;
}

int offload_bassboost_send_params(struct mixer_ctl *ctl,
                                  struct bass_boost_params bassboost,
                                  unsigned param_send_flags)
{
    int param_values[128] = {0};
    int *p_param_values = param_values;

    ALOGV("%s: flags 0x%x", __func__, param_send_flags);
    *p_param_values++ = BASS_BOOST_MODULE;
    *p_param_values++ = bassboost.device;
    *p_param_values++ = 0; /* num of commands*/
    if (param_send_flags & OFFLOAD_SEND_BASSBOOST_ENABLE_FLAG) {
        *p_param_values++ = BASS_BOOST_ENABLE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = BASS_BOOST_ENABLE_PARAM_LEN;
        *p_param_values++ = bassboost.enable_flag;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_BASSBOOST_STRENGTH) {
        *p_param_values++ = BASS_BOOST_STRENGTH;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = BASS_BOOST_STRENGTH_PARAM_LEN;
        *p_param_values++ = bassboost.strength;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_BASSBOOST_MODE) {
        *p_param_values++ = BASS_BOOST_MODE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = BASS_BOOST_MODE_PARAM_LEN;
        *p_param_values++ = bassboost.mode;
        param_values[2] += 1;
    }

    if (param_values[2] && ctl)
        mixer_ctl_set_array(ctl, param_values, ARRAY_SIZE(param_values));

    return 0;
}

void offload_pbe_set_device(struct pbe_params *pbe,
                            uint32_t device)
{
    ALOGV("%s: device=%d", __func__, device);
    pbe->device = device;
}

void offload_pbe_set_enable_flag(struct pbe_params *pbe,
                                 bool enable)
{
    ALOGV("%s: enable=%d", __func__, enable);
    pbe->enable_flag = enable;
}

int offload_pbe_get_enable_flag(struct pbe_params *pbe)
{
    ALOGV("%s: enabled=%d", __func__, pbe->enable_flag);
    return pbe->enable_flag;
}

static int pbe_send_params(eff_mode_t mode, void *ctl,
                            struct pbe_params *pbe,
                            unsigned param_send_flags)
{
    int param_values[128] = {0};
    int i, *p_param_values = param_values, *cfg = NULL;

    ALOGV("%s: enabled=%d", __func__, pbe->enable_flag);
    *p_param_values++ = PBE_MODULE;
    *p_param_values++ = pbe->device;
    *p_param_values++ = 0; /* num of commands*/
    if (param_send_flags & OFFLOAD_SEND_PBE_ENABLE_FLAG) {
        *p_param_values++ = PBE_ENABLE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = PBE_ENABLE_PARAM_LEN;
        *p_param_values++ = pbe->enable_flag;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_PBE_CONFIG) {
        *p_param_values++ = PBE_CONFIG;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = pbe->cfg_len;
        cfg = (int *)&pbe->config;
        for (i = 0; i < (int)pbe->cfg_len ; i+= sizeof(*p_param_values))
            *p_param_values++ = *cfg++;
        param_values[2] += 1;
    }

    if ((mode == OFFLOAD) && param_values[2] && ctl) {
        mixer_ctl_set_array((struct mixer_ctl *)ctl, param_values,
                            ARRAY_SIZE(param_values));
    }

    return 0;
}

int offload_pbe_send_params(struct mixer_ctl *ctl,
                                  struct pbe_params *pbe,
                                  unsigned param_send_flags)
{
    return pbe_send_params(OFFLOAD, (void *)ctl, pbe,
                                 param_send_flags);
}

void offload_virtualizer_set_device(struct virtualizer_params *virtualizer,
                                    uint32_t device)
{
    ALOGVV("%s: device=0x%x", __func__, device);
    virtualizer->device = device;
}

void offload_virtualizer_set_enable_flag(struct virtualizer_params *virtualizer,
                                         bool enable)
{
    ALOGVV("%s: enable=%d", __func__, (int)enable);
    virtualizer->enable_flag = enable;
}

int offload_virtualizer_get_enable_flag(struct virtualizer_params *virtualizer)
{
    ALOGVV("%s: enabled %d", __func__, (int)virtualizer->enable_flag);
    return virtualizer->enable_flag;
}

void offload_virtualizer_set_strength(struct virtualizer_params *virtualizer,
                                      int strength)
{
    ALOGVV("%s: strength %d", __func__, strength);
    virtualizer->strength = strength;
}

void offload_virtualizer_set_out_type(struct virtualizer_params *virtualizer,
                                      int out_type)
{
    ALOGVV("%s: out_type %d", __func__, out_type);
    virtualizer->out_type = out_type;
}

void offload_virtualizer_set_gain_adjust(struct virtualizer_params *virtualizer,
                                         int gain_adjust)
{
    ALOGVV("%s: gain %d", __func__, gain_adjust);
    virtualizer->gain_adjust = gain_adjust;
}

int offload_virtualizer_send_params(struct mixer_ctl *ctl,
                                    struct virtualizer_params virtualizer,
                                    unsigned param_send_flags)
{
    int param_values[128] = {0};
    int *p_param_values = param_values;

    ALOGV("%s: flags 0x%x", __func__, param_send_flags);
    *p_param_values++ = VIRTUALIZER_MODULE;
    *p_param_values++ = virtualizer.device;
    *p_param_values++ = 0; /* num of commands*/
    if (param_send_flags & OFFLOAD_SEND_VIRTUALIZER_ENABLE_FLAG) {
        *p_param_values++ = VIRTUALIZER_ENABLE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = VIRTUALIZER_ENABLE_PARAM_LEN;
        *p_param_values++ = virtualizer.enable_flag;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_VIRTUALIZER_STRENGTH) {
        *p_param_values++ = VIRTUALIZER_STRENGTH;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = VIRTUALIZER_STRENGTH_PARAM_LEN;
        *p_param_values++ = virtualizer.strength;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_VIRTUALIZER_OUT_TYPE) {
        *p_param_values++ = VIRTUALIZER_OUT_TYPE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = VIRTUALIZER_OUT_TYPE_PARAM_LEN;
        *p_param_values++ = virtualizer.out_type;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_VIRTUALIZER_GAIN_ADJUST) {
        *p_param_values++ = VIRTUALIZER_GAIN_ADJUST;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = VIRTUALIZER_GAIN_ADJUST_PARAM_LEN;
        *p_param_values++ = virtualizer.gain_adjust;
        param_values[2] += 1;
    }

    if (param_values[2] && ctl)
        mixer_ctl_set_array(ctl, param_values, ARRAY_SIZE(param_values));

    return 0;
}

void offload_eq_set_device(struct eq_params *eq, uint32_t device)
{
    ALOGVV("%s: device 0x%x", __func__, device);
    eq->device = device;
}

void offload_eq_set_enable_flag(struct eq_params *eq, bool enable)
{
    ALOGVV("%s: enable=%d", __func__, (int)enable);
    eq->enable_flag = enable;
}

int offload_eq_get_enable_flag(struct eq_params *eq)
{
    ALOGVV("%s: enabled=%d", __func__, (int)eq->enable_flag);
    return eq->enable_flag;
}

void offload_eq_set_preset(struct eq_params *eq, int preset)
{
    ALOGVV("%s: preset %d", __func__, preset);
    eq->config.preset_id = preset;
    eq->config.eq_pregain = Q27_UNITY;
}

void offload_eq_set_bands_level(struct eq_params *eq, int num_bands,
                                const uint16_t *band_freq_list,
                                int *band_gain_list)
{
    int i;
    ALOGVV("%s", __func__);
    eq->config.num_bands = num_bands;
    for (i=0; i<num_bands; i++) {
        eq->per_band_cfg[i].band_idx = i;
        eq->per_band_cfg[i].filter_type = EQ_BAND_BOOST;
        eq->per_band_cfg[i].freq_millihertz = band_freq_list[i] * 1000;
        eq->per_band_cfg[i].gain_millibels = band_gain_list[i] * 100;
        eq->per_band_cfg[i].quality_factor = Q8_UNITY;
    }
}

int offload_eq_send_params(struct mixer_ctl *ctl, struct eq_params eq,
                           unsigned param_send_flags)
{
    int param_values[128] = {0};
    int *p_param_values = param_values;
    uint32_t i;

    ALOGV("%s: flags 0x%x", __func__, param_send_flags);
    if ((eq.config.preset_id < -1) ||
            ((param_send_flags & OFFLOAD_SEND_EQ_PRESET) && (eq.config.preset_id == -1))) {
        ALOGV("No Valid preset to set");
        return 0;
    }
    *p_param_values++ = EQ_MODULE;
    *p_param_values++ = eq.device;
    *p_param_values++ = 0; /* num of commands*/
    if (param_send_flags & OFFLOAD_SEND_EQ_ENABLE_FLAG) {
        *p_param_values++ = EQ_ENABLE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = EQ_ENABLE_PARAM_LEN;
        *p_param_values++ = eq.enable_flag;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_EQ_PRESET) {
        *p_param_values++ = EQ_CONFIG;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = EQ_CONFIG_PARAM_LEN;
        *p_param_values++ = eq.config.eq_pregain;
        *p_param_values++ =
                     map_eq_opensl_preset_2_offload_preset[eq.config.preset_id];
        *p_param_values++ = 0;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_EQ_BANDS_LEVEL) {
        *p_param_values++ = EQ_CONFIG;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = EQ_CONFIG_PARAM_LEN +
                            eq.config.num_bands * EQ_CONFIG_PER_BAND_PARAM_LEN;
        *p_param_values++ = eq.config.eq_pregain;
        *p_param_values++ = CUSTOM_OPENSL_PRESET;
        *p_param_values++ = eq.config.num_bands;
        for (i=0; i<eq.config.num_bands; i++) {
            *p_param_values++ = eq.per_band_cfg[i].band_idx;
            *p_param_values++ = eq.per_band_cfg[i].filter_type;
	    *p_param_values++ = eq.per_band_cfg[i].freq_millihertz;
            *p_param_values++ = eq.per_band_cfg[i].gain_millibels;
            *p_param_values++ = eq.per_band_cfg[i].quality_factor;
        }
        param_values[2] += 1;
    }

    if (param_values[2] && ctl)
        mixer_ctl_set_array(ctl, param_values, ARRAY_SIZE(param_values));

    return 0;
}

void offload_reverb_set_device(struct reverb_params *reverb, uint32_t device)
{
    ALOGVV("%s: device 0x%x", __func__, device);
    reverb->device = device;
}

void offload_reverb_set_enable_flag(struct reverb_params *reverb, bool enable)
{
    ALOGVV("%s: enable=%d", __func__, (int)enable);
    reverb->enable_flag = enable;
}

int offload_reverb_get_enable_flag(struct reverb_params *reverb)
{
    ALOGVV("%s: enabled=%d", __func__, reverb->enable_flag);
    return reverb->enable_flag;
}

void offload_reverb_set_mode(struct reverb_params *reverb, int mode)
{
    ALOGVV("%s", __func__);
    reverb->mode = mode;
}

void offload_reverb_set_preset(struct reverb_params *reverb, int preset)
{
    ALOGVV("%s: preset %d", __func__, preset);
    if (preset && (preset <= NUM_OSL_REVERB_PRESETS_SUPPORTED))
        reverb->preset = map_reverb_opensl_preset_2_offload_preset[preset-1][1];
}

void offload_reverb_set_wet_mix(struct reverb_params *reverb, int wet_mix)
{
    ALOGVV("%s: wet_mix %d", __func__, wet_mix);
    reverb->wet_mix = wet_mix;
}

void offload_reverb_set_gain_adjust(struct reverb_params *reverb,
                                    int gain_adjust)
{
    ALOGVV("%s: gain %d", __func__, gain_adjust);
    reverb->gain_adjust = gain_adjust;
}

void offload_reverb_set_room_level(struct reverb_params *reverb, int room_level)
{
    ALOGVV("%s: level %d", __func__, room_level);
    reverb->room_level = room_level;
}

void offload_reverb_set_room_hf_level(struct reverb_params *reverb,
                                      int room_hf_level)
{
    ALOGVV("%s: level %d", __func__, room_hf_level);
    reverb->room_hf_level = room_hf_level;
}

void offload_reverb_set_decay_time(struct reverb_params *reverb, int decay_time)
{
    ALOGVV("%s: decay time %d", __func__, decay_time);
    reverb->decay_time = decay_time;
}

void offload_reverb_set_decay_hf_ratio(struct reverb_params *reverb,
                                       int decay_hf_ratio)
{
    ALOGVV("%s: decay_hf_ratio %d", __func__, decay_hf_ratio);
    reverb->decay_hf_ratio = decay_hf_ratio;
}

void offload_reverb_set_reflections_level(struct reverb_params *reverb,
                                          int reflections_level)
{
    ALOGVV("%s: ref level %d", __func__, reflections_level);
    reverb->reflections_level = reflections_level;
}

void offload_reverb_set_reflections_delay(struct reverb_params *reverb,
                                          int reflections_delay)
{
    ALOGVV("%s: ref delay", __func__, reflections_delay);
    reverb->reflections_delay = reflections_delay;
}

void offload_reverb_set_reverb_level(struct reverb_params *reverb,
                                     int reverb_level)
{
    ALOGD("%s: reverb level %d", __func__, reverb_level);
    reverb->level = reverb_level;
}

void offload_reverb_set_delay(struct reverb_params *reverb, int delay)
{
    ALOGVV("%s: delay %d", __func__, delay);
    reverb->delay = delay;
}

void offload_reverb_set_diffusion(struct reverb_params *reverb, int diffusion)
{
    ALOGVV("%s: diffusion %d", __func__, diffusion);
    reverb->diffusion = diffusion;
}

void offload_reverb_set_density(struct reverb_params *reverb, int density)
{
    ALOGVV("%s: density %d", __func__, density);
    reverb->density = density;
}

int offload_reverb_send_params(struct mixer_ctl *ctl,
                               struct reverb_params reverb,
                               unsigned param_send_flags)
{
    int param_values[128] = {0};
    int *p_param_values = param_values;

    ALOGV("%s: flags 0x%x", __func__, param_send_flags);
    *p_param_values++ = REVERB_MODULE;
    *p_param_values++ = reverb.device;
    *p_param_values++ = 0; /* num of commands*/

    if (param_send_flags & OFFLOAD_SEND_REVERB_ENABLE_FLAG) {
        *p_param_values++ = REVERB_ENABLE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_ENABLE_PARAM_LEN;
        *p_param_values++ = reverb.enable_flag;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_MODE) {
        *p_param_values++ = REVERB_MODE;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_MODE_PARAM_LEN;
        *p_param_values++ = reverb.mode;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_PRESET) {
        *p_param_values++ = REVERB_PRESET;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_PRESET_PARAM_LEN;
        *p_param_values++ = reverb.preset;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_WET_MIX) {
        *p_param_values++ = REVERB_WET_MIX;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_WET_MIX_PARAM_LEN;
        *p_param_values++ = reverb.wet_mix;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_GAIN_ADJUST) {
        *p_param_values++ = REVERB_GAIN_ADJUST;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_GAIN_ADJUST_PARAM_LEN;
        *p_param_values++ = reverb.gain_adjust;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_ROOM_LEVEL) {
        *p_param_values++ = REVERB_ROOM_LEVEL;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_ROOM_LEVEL_PARAM_LEN;
        *p_param_values++ = reverb.room_level;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_ROOM_HF_LEVEL) {
        *p_param_values++ = REVERB_ROOM_HF_LEVEL;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_ROOM_HF_LEVEL_PARAM_LEN;
        *p_param_values++ = reverb.room_hf_level;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_DECAY_TIME) {
        *p_param_values++ = REVERB_DECAY_TIME;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_DECAY_TIME_PARAM_LEN;
        *p_param_values++ = reverb.decay_time;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_DECAY_HF_RATIO) {
        *p_param_values++ = REVERB_DECAY_HF_RATIO;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_DECAY_HF_RATIO_PARAM_LEN;
        *p_param_values++ = reverb.decay_hf_ratio;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_REFLECTIONS_LEVEL) {
        *p_param_values++ = REVERB_REFLECTIONS_LEVEL;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_REFLECTIONS_LEVEL_PARAM_LEN;
        *p_param_values++ = reverb.reflections_level;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_REFLECTIONS_DELAY) {
        *p_param_values++ = REVERB_REFLECTIONS_DELAY;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_REFLECTIONS_DELAY_PARAM_LEN;
        *p_param_values++ = reverb.reflections_delay;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_LEVEL) {
        *p_param_values++ = REVERB_LEVEL;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_LEVEL_PARAM_LEN;
        *p_param_values++ = reverb.level;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_DELAY) {
        *p_param_values++ = REVERB_DELAY;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_DELAY_PARAM_LEN;
        *p_param_values++ = reverb.delay;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_DIFFUSION) {
        *p_param_values++ = REVERB_DIFFUSION;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_DIFFUSION_PARAM_LEN;
        *p_param_values++ = reverb.diffusion;
        param_values[2] += 1;
    }
    if (param_send_flags & OFFLOAD_SEND_REVERB_DENSITY) {
        *p_param_values++ = REVERB_DENSITY;
        *p_param_values++ = CONFIG_SET;
        *p_param_values++ = 0; /* start offset if param size if greater than 128  */
        *p_param_values++ = REVERB_DENSITY_PARAM_LEN;
        *p_param_values++ = reverb.density;
        param_values[2] += 1;
    }

    if (param_values[2] && ctl)
        mixer_ctl_set_array(ctl, param_values, ARRAY_SIZE(param_values));

    return 0;
}
