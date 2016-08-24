/*
 * Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
 * Not a contribution.
 *
 * Copyright (C) 2009 The Android Open Source Project
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


#include <audiopolicy/managerdefault/AudioPolicyManager.h>
#include <audio_policy_conf.h>
#include <Volume.h>


namespace android {
#ifndef FLAC_OFFLOAD_ENABLED
#define AUDIO_FORMAT_FLAC 0x1D000000UL
#endif

#ifndef WMA_OFFLOAD_ENABLED
#define AUDIO_FORMAT_WMA 0x13000000UL
#define AUDIO_FORMAT_WMA_PRO 0x14000000UL
#endif

#ifndef ALAC_OFFLOAD_ENABLED
#define AUDIO_FORMAT_ALAC 0x1F000000UL
#endif

#ifndef APE_OFFLOAD_ENABLED
#define AUDIO_FORMAT_APE 0x20000000UL
#endif
#ifndef AUDIO_EXTN_AFE_PROXY_ENABLED
#define AUDIO_DEVICE_OUT_PROXY 0x1000000
#endif
// ----------------------------------------------------------------------------

class AudioPolicyManagerCustom: public AudioPolicyManager
{

public:
        AudioPolicyManagerCustom(AudioPolicyClientInterface *clientInterface);

        virtual ~AudioPolicyManagerCustom() {}

        status_t setDeviceConnectionStateInt(audio_devices_t device,
                                          audio_policy_dev_state_t state,
                                          const char *device_address,
                                          const char *device_name);
        virtual void setPhoneState(audio_mode_t state);
        virtual void setForceUse(audio_policy_force_use_t usage,
                                 audio_policy_forced_cfg_t config);

        virtual bool isOffloadSupported(const audio_offload_info_t& offloadInfo);

        virtual status_t getInputForAttr(const audio_attributes_t *attr,
                                         audio_io_handle_t *input,
                                         audio_session_t session,
                                         uid_t uid,
                                         uint32_t samplingRate,
                                         audio_format_t format,
                                         audio_channel_mask_t channelMask,
                                         audio_input_flags_t flags,
                                         audio_port_handle_t selectedDeviceId,
                                         input_type_t *inputType);
        // indicates to the audio policy manager that the input starts being used.
        virtual status_t startInput(audio_io_handle_t input,
                                    audio_session_t session);
        // indicates to the audio policy manager that the input stops being used.
        virtual status_t stopInput(audio_io_handle_t input,
                                   audio_session_t session);

protected:

#ifdef NON_WEARABLE_TARGET
         status_t checkAndSetVolume(audio_stream_type_t stream,
                                                    int index,
                                                    const sp<AudioOutputDescriptor>& outputDesc,
                                                    audio_devices_t device,
                                                    int delayMs = 0, bool force = false);
#else
         status_t checkAndSetVolume(audio_stream_type_t stream,
                                                   int index,
                                                   const sp<SwAudioOutputDescriptor>& outputDesc,
                                                   audio_devices_t device,
                                                   int delayMs = 0, bool force = false);
#endif

        // selects the most appropriate device on output for current state
        // must be called every time a condition that affects the device choice for a given output is
        // changed: connected device, phone state, force use, output start, output stop..
        // see getDeviceForStrategy() for the use of fromCache parameter
        audio_devices_t getNewOutputDevice(const sp<AudioOutputDescriptor>& outputDesc,
                                           bool fromCache);
        // returns true if given output is direct output
        bool isDirectOutput(audio_io_handle_t output);

        // if argument "device" is different from AUDIO_DEVICE_NONE,  startSource() will force
        // the re-evaluation of the output device.
        status_t startSource(sp<AudioOutputDescriptor> outputDesc,
                             audio_stream_type_t stream,
                             audio_devices_t device,
                             uint32_t *delayMs);
        status_t stopSource(sp<AudioOutputDescriptor> outputDesc,
                            audio_stream_type_t stream,
                            bool forceDeviceUpdate);
        // event is one of STARTING_OUTPUT, STARTING_BEACON, STOPPING_OUTPUT, STOPPING_BEACON   313
        // returns 0 if no mute/unmute event happened, the largest latency of the device where   314
        //   the mute/unmute happened 315
        uint32_t handleEventForBeacon(int){return 0;}
        uint32_t setBeaconMute(bool){return 0;}
#ifdef VOICE_CONCURRENCY
        static audio_output_flags_t getFallBackPath();
        int mFallBackflag;
#endif /*VOICE_CONCURRENCY*/

        // handle special cases for sonification strategy while in call: mute streams or replace by
        // a special tone in the device used for communication
        void handleIncallSonification(audio_stream_type_t stream, bool starting, bool stateChange, audio_io_handle_t output);
        //parameter indicates of HDMI speakers disabled
        bool mHdmiAudioDisabled;
        //parameter indicates if HDMI plug in/out detected
        bool mHdmiAudioEvent;
private:
        static float volIndexToAmpl(audio_devices_t device, const StreamDescriptor& streamDesc,
                int indexInUi);
        // updates device caching and output for streams that can influence the
        //    routing of notifications
        void handleNotificationRoutingForStream(audio_stream_type_t stream);
        static bool isVirtualInputDevice(audio_devices_t device);
        static bool deviceDistinguishesOnAddress(audio_devices_t device);
        uint32_t nextUniqueId();
        // internal method to return the output handle for the given device and format
        audio_io_handle_t getOutputForDevice(
                audio_devices_t device,
                audio_session_t session,
                audio_stream_type_t stream,
                uint32_t samplingRate,
                audio_format_t format,
                audio_channel_mask_t channelMask,
                audio_output_flags_t flags,
                const audio_offload_info_t *offloadInfo);
        // Used for voip + voice concurrency usecase
        int mPrevPhoneState;
        int mvoice_call_state;
#ifdef RECORD_PLAY_CONCURRENCY
        // Used for record + playback concurrency
        bool mIsInputRequestOnProgress;
#endif


};

};
