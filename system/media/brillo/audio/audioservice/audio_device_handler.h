// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// Handler for input events in /dev/input. AudioDeviceHandler handles events
// only for audio devices being plugged in/removed from the system. Implements
// some of the functionality present in WiredAccessoryManager.java.

#ifndef BRILLO_AUDIO_AUDIOSERVICE_AUDIO_DEVICE_HANDLER_H_
#define BRILLO_AUDIO_AUDIOSERVICE_AUDIO_DEVICE_HANDLER_H_

#include <set>
#include <vector>

#include <base/files/file_path.h>
#include <gtest/gtest_prod.h>
#include <linux/input.h>
#include <media/IAudioPolicyService.h>
#include <system/audio.h>
#include <system/audio_policy.h>

namespace brillo {

class AudioDeviceHandler {
 public:
  AudioDeviceHandler();
  virtual ~AudioDeviceHandler();

  // Get the current state of the headset jack and update AudioSystem based on
  // the initial state.
  //
  // |aps| is a pointer to the binder object.
  void Init(android::sp<android::IAudioPolicyService> aps);

  // Process input events from the kernel. Connecting/disconnecting an audio
  // device will result in multiple calls to this method.
  //
  // |event| is a pointer to an input_event. This function should be able to
  // gracefully handle input events that are not relevant to the functionality
  // provided by this class.
  void ProcessEvent(const struct input_event& event);

  // Inform the handler that the audio policy service has been disconnected.
  void APSDisconnect();

  // Inform the handler that the audio policy service is reconnected.
  //
  // |aps| is a pointer to the binder object.
  void APSConnect(android::sp<android::IAudioPolicyService> aps);

 private:
  friend class AudioDeviceHandlerTest;
  FRIEND_TEST(AudioDeviceHandlerTest,
              DisconnectAllSupportedDevicesCallsDisconnect);
  FRIEND_TEST(AudioDeviceHandlerTest, InitCallsDisconnectAllSupportedDevices);
  FRIEND_TEST(AudioDeviceHandlerTest, InitialAudioStateMic);
  FRIEND_TEST(AudioDeviceHandlerTest, InitialAudioStateHeadphone);
  FRIEND_TEST(AudioDeviceHandlerTest, InitialAudioStateHeadset);
  FRIEND_TEST(AudioDeviceHandlerTest, InitialAudioStateNone);
  FRIEND_TEST(AudioDeviceHandlerTest, InitialAudioStateInvalid);
  FRIEND_TEST(AudioDeviceHandlerTest, ProcessEventEmpty);
  FRIEND_TEST(AudioDeviceHandlerTest, ProcessEventMicrophonePresent);
  FRIEND_TEST(AudioDeviceHandlerTest, ProcessEventHeadphonePresent);
  FRIEND_TEST(AudioDeviceHandlerTest, ProcessEventMicrophoneNotPresent);
  FRIEND_TEST(AudioDeviceHandlerTest, ProcessEventHeadphoneNotPresent);
  FRIEND_TEST(AudioDeviceHandlerTest, ProcessEventInvalid);
  FRIEND_TEST(AudioDeviceHandlerTest, UpdateAudioSystemNone);
  FRIEND_TEST(AudioDeviceHandlerTest, UpdateAudioSystemConnectMic);
  FRIEND_TEST(AudioDeviceHandlerTest, UpdateAudioSystemConnectHeadphone);
  FRIEND_TEST(AudioDeviceHandlerTest, UpdateAudioSystemConnectHeadset);
  FRIEND_TEST(AudioDeviceHandlerTest, UpdateAudioSystemDisconnectMic);
  FRIEND_TEST(AudioDeviceHandlerTest, UpdateAudioSystemDisconnectHeadphone);
  FRIEND_TEST(AudioDeviceHandlerTest, UpdateAudioSystemDisconnectHeadset);
  FRIEND_TEST(AudioDeviceHandlerTest, ConnectAudioDeviceInput);
  FRIEND_TEST(AudioDeviceHandlerTest, ConnectAudioDeviceOutput);
  FRIEND_TEST(AudioDeviceHandlerTest, DisconnectAudioDeviceInput);
  FRIEND_TEST(AudioDeviceHandlerTest, DisconnectAudioDeviceOutput);

  // Read the initial state of audio devices in /sys/class/* and update
  // the audio policy service.
  //
  // |path| is the file that contains the initial audio jack state.
  void GetInitialAudioDeviceState(const base::FilePath& path);

  // Update the audio policy service once an input_event has completed.
  //
  // |headphone| is true is headphones are connected.
  // |microphone| is true is microphones are connected.
  void UpdateAudioSystem(bool headphone, bool microphone);

  // Notify the audio policy service that this device has been removed.
  //
  // |device| is the audio device whose state is to be changed.
  // |state| is the current state of |device|.
  virtual void NotifyAudioPolicyService(audio_devices_t device,
                                        audio_policy_dev_state_t state);

  // Connect an audio device by calling aps and add it to the appropriate set
  // (either connected_input_devices_ or connected_output_devices_).
  //
  // |device| is the audio device that has been added.
  void ConnectAudioDevice(audio_devices_t device);

  // Disconnect an audio device by calling aps and remove it from the
  // appropriate set (either connected_input_devices_ or
  // connected_output_devices_).
  //
  // |device| is the audio device that has been disconnected.
  void DisconnectAudioDevice(audio_devices_t device);

  // Disconnected all connected audio devices.
  void DisconnectAllConnectedDevices();

  // Disconnect all supported audio devices.
  void DisconnectAllSupportedDevices();

  // All input devices currently supported by AudioDeviceHandler.
  std::vector<audio_devices_t> kSupportedInputDevices_{
      AUDIO_DEVICE_IN_WIRED_HEADSET};
  // All output devices currently supported by AudioDeviceHandler.
  std::vector<audio_devices_t> kSupportedOutputDevices_{
      AUDIO_DEVICE_OUT_WIRED_HEADSET, AUDIO_DEVICE_OUT_WIRED_HEADPHONE};
  // Pointer to the audio policy service.
  android::sp<android::IAudioPolicyService> aps_;

 protected:
  // Set of connected input devices.
  std::set<audio_devices_t> connected_input_devices_;
  // Set of connected output devices.
  std::set<audio_devices_t> connected_output_devices_;
  // Keeps track of whether a headphone has been connected. Used by ProcessEvent
  // and UpdateAudioSystem.
  bool headphone_;
  // Keeps track of whether a microphone has been connected. Used by
  // ProcessEvent and UpdateAudioSystem.
  bool microphone_;
};

}  // namespace brillo

#endif  // BRILLO_AUDIO_AUDIOSERVICE_AUDIO_DEVICE_HANDLER_H_
