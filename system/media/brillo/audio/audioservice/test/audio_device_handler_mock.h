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

// Mock of AudioDeviceHandler.

#ifndef BRILLO_AUDIO_AUDIOSERVICE_TEST_AUDIO_DEVICE_HANDLER_MOCK_H_
#define BRILLO_AUDIO_AUDIOSERVICE_TEST_AUDIO_DEVICE_HANDLER_MOCK_H_

#include <base/files/file_path.h>
#include <gmock/gmock.h>
#include <gtest/gtest_prod.h>
#include <system/audio.h>
#include <system/audio_policy.h>

#include "audio_device_handler.h"

namespace brillo {

class AudioDeviceHandlerMock : public AudioDeviceHandler {
 public:
  AudioDeviceHandlerMock() = default;
  ~AudioDeviceHandlerMock() {}

  // Reset all local data.
  void Reset() {
    connected_input_devices_.clear();
    connected_output_devices_.clear();
    headphone_ = false;
    microphone_ = false;
  }

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
  FRIEND_TEST(AudioDeviceHandlerTest, InitCallsDisconnect);
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

  MOCK_METHOD2(NotifyAudioPolicyService,
               void(audio_devices_t device, audio_policy_dev_state_t state));
};

}  // namespace brillo

#endif  // BRILLO_AUDIO_AUDIOSERVICE_TEST_AUDIO_DEVICE_HANDLER_MOCK_H_
