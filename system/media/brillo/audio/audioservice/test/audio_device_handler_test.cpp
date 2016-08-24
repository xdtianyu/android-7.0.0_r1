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

// Tests for audio device handler.

#include "audio_device_handler_mock.h"

#include <string>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/strings/string_number_conversions.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using base::FilePath;
using base::IntToString;
using base::ScopedTempDir;
using base::WriteFile;
using brillo::AudioDeviceHandlerMock;
using testing::_;
using testing::AnyNumber;

namespace brillo {

class AudioDeviceHandlerTest : public testing::Test {
 public:
  void SetUp() override {
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
    h2w_file_path_ = temp_dir_.path().Append("h2wstate");
  }

  void TearDown() override { handler_.Reset(); }

  // Method to store the current state of the audio jack to a file.
  //
  // |value| - Value in the h2w file.
  void WriteToH2WFile(int value) {
    std::string value_string = IntToString(value);
    WriteFile(h2w_file_path_, value_string.c_str(), value_string.length());
  }

  AudioDeviceHandlerMock handler_;
  FilePath h2w_file_path_;

 private:
  ScopedTempDir temp_dir_;
};

// Test that DisconnectAllSupportedDevices() calls NotifyAudioPolicyService()
// the right number of times.
TEST_F(AudioDeviceHandlerTest, DisconnectAllSupportedDevicesCallsDisconnect) {
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(
                  _, AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE)).Times(3);
  handler_.DisconnectAllSupportedDevices();
}

// Test that Init() calls DisconnectAllSupportedDevices().
TEST_F(AudioDeviceHandlerTest, InitCallsDisconnectAllSupportedDevices) {
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(
                  _, AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE)).Times(3);
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(
                  _, AUDIO_POLICY_DEVICE_STATE_AVAILABLE)).Times(AnyNumber());
  handler_.Init(nullptr);
}

// Test GetInitialAudioDeviceState() with just a microphone.
TEST_F(AudioDeviceHandlerTest, InitialAudioStateMic) {
  WriteToH2WFile(2);
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(AUDIO_DEVICE_IN_WIRED_HEADSET,
                                       AUDIO_POLICY_DEVICE_STATE_AVAILABLE));
  handler_.GetInitialAudioDeviceState(h2w_file_path_);
  EXPECT_NE(
      handler_.connected_input_devices_.find(AUDIO_DEVICE_IN_WIRED_HEADSET),
      handler_.connected_input_devices_.end());
  EXPECT_EQ(handler_.connected_output_devices_.size(), 0);
}

// Test GetInitialAudioDeviceState() with a headphone.
TEST_F(AudioDeviceHandlerTest, InitialAudioStateHeadphone) {
  WriteToH2WFile(1);
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
                                       AUDIO_POLICY_DEVICE_STATE_AVAILABLE));
  handler_.GetInitialAudioDeviceState(h2w_file_path_);
  EXPECT_EQ(handler_.connected_input_devices_.size(), 0);
  EXPECT_NE(
      handler_.connected_output_devices_.find(AUDIO_DEVICE_OUT_WIRED_HEADPHONE),
      handler_.connected_output_devices_.end());
}

// Test GetInitialAudioDeviceState() with a headset.
TEST_F(AudioDeviceHandlerTest, InitialAudioStateHeadset) {
  WriteToH2WFile(3);
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(AUDIO_DEVICE_IN_WIRED_HEADSET,
                                       AUDIO_POLICY_DEVICE_STATE_AVAILABLE));
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(AUDIO_DEVICE_OUT_WIRED_HEADSET,
                                       AUDIO_POLICY_DEVICE_STATE_AVAILABLE));
  handler_.GetInitialAudioDeviceState(h2w_file_path_);
  EXPECT_NE(
      handler_.connected_input_devices_.find(AUDIO_DEVICE_IN_WIRED_HEADSET),
      handler_.connected_input_devices_.end());
  EXPECT_NE(
      handler_.connected_output_devices_.find(AUDIO_DEVICE_OUT_WIRED_HEADSET),
      handler_.connected_output_devices_.end());
}

// Test GetInitialAudioDeviceState() without any devices connected to the audio
// jack. No need to call NotifyAudioPolicyService() since that's already handled
// by Init().
TEST_F(AudioDeviceHandlerTest, InitialAudioStateNone) {
  WriteToH2WFile(0);
  handler_.GetInitialAudioDeviceState(h2w_file_path_);
  EXPECT_EQ(handler_.connected_input_devices_.size(), 0);
  EXPECT_EQ(handler_.connected_output_devices_.size(), 0);
}

// Test GetInitialAudioDeviceState() with an invalid file. The audio handler
// should not fail in this case because it should work on boards that don't
// support audio jacks.
TEST_F(AudioDeviceHandlerTest, InitialAudioStateInvalid) {
  FilePath path = h2w_file_path_;
  handler_.GetInitialAudioDeviceState(h2w_file_path_);
  EXPECT_EQ(handler_.connected_input_devices_.size(), 0);
  EXPECT_EQ(handler_.connected_output_devices_.size(), 0);
}

// Test ProcessEvent() with an empty input_event arg.
TEST_F(AudioDeviceHandlerTest, ProcessEventEmpty) {
  struct input_event event;
  event.type = 0;
  event.code = 0;
  event.value = 0;
  handler_.ProcessEvent(event);
  EXPECT_FALSE(handler_.headphone_);
  EXPECT_FALSE(handler_.microphone_);
}

// Test ProcessEvent() with a microphone present input_event arg.
TEST_F(AudioDeviceHandlerTest, ProcessEventMicrophonePresent) {
  struct input_event event;
  event.type = EV_SW;
  event.code = SW_MICROPHONE_INSERT;
  event.value = 1;
  handler_.ProcessEvent(event);
  EXPECT_FALSE(handler_.headphone_);
  EXPECT_TRUE(handler_.microphone_);
}

// Test ProcessEvent() with a headphone present input_event arg.
TEST_F(AudioDeviceHandlerTest, ProcessEventHeadphonePresent) {
  struct input_event event;
  event.type = EV_SW;
  event.code = SW_HEADPHONE_INSERT;
  event.value = 1;
  handler_.ProcessEvent(event);
  EXPECT_TRUE(handler_.headphone_);
  EXPECT_FALSE(handler_.microphone_);
}

// Test ProcessEvent() with a microphone not present input_event arg.
TEST_F(AudioDeviceHandlerTest, ProcessEventMicrophoneNotPresent) {
  struct input_event event;
  event.type = EV_SW;
  event.code = SW_MICROPHONE_INSERT;
  event.value = 0;
  handler_.ProcessEvent(event);
  EXPECT_FALSE(handler_.headphone_);
  EXPECT_FALSE(handler_.microphone_);
}

// Test ProcessEvent() with a headphone not preset input_event arg.
TEST_F(AudioDeviceHandlerTest, ProcessEventHeadphoneNotPresent) {
  struct input_event event;
  event.type = EV_SW;
  event.code = SW_HEADPHONE_INSERT;
  event.value = 0;
  handler_.ProcessEvent(event);
  EXPECT_FALSE(handler_.headphone_);
  EXPECT_FALSE(handler_.microphone_);
}

// Test ProcessEvent() with an unsupported input_event arg.
TEST_F(AudioDeviceHandlerTest, ProcessEventInvalid) {
  struct input_event event;
  event.type = EV_SW;
  event.code = SW_MAX;
  event.value = 0;
  handler_.ProcessEvent(event);
  EXPECT_FALSE(handler_.headphone_);
  EXPECT_FALSE(handler_.microphone_);
}

// Test UpdateAudioSystem() without any devices connected.
TEST_F(AudioDeviceHandlerTest, UpdateAudioSystemNone) {
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(
                  _, AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE)).Times(0);
  handler_.UpdateAudioSystem(handler_.headphone_, handler_.microphone_);
}

// Test UpdateAudioSystem() when disconnecting a microphone.
TEST_F(AudioDeviceHandlerTest, UpdateAudioSystemDisconnectMic) {
  audio_devices_t device = AUDIO_DEVICE_IN_WIRED_HEADSET;
  handler_.connected_input_devices_.insert(device);
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(device,
                                       AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE));
  handler_.UpdateAudioSystem(handler_.headphone_, handler_.microphone_);
  EXPECT_EQ(handler_.connected_input_devices_.size(), 0);
  EXPECT_EQ(handler_.connected_output_devices_.size(), 0);
}

// Test UpdateAudioSystem() when disconnecting a headphone.
TEST_F(AudioDeviceHandlerTest, UpdateAudioSystemDisconnectHeadphone) {
  audio_devices_t device = AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
  handler_.connected_output_devices_.insert(device);
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(device,
                                       AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE));
  handler_.UpdateAudioSystem(handler_.headphone_, handler_.microphone_);
  EXPECT_EQ(handler_.connected_input_devices_.size(), 0);
  EXPECT_EQ(handler_.connected_output_devices_.size(), 0);
}

// Test UpdateAudioSystem() when disconnecting a headset & headphones.
TEST_F(AudioDeviceHandlerTest, UpdateAudioSystemDisconnectHeadset) {
  handler_.connected_input_devices_.insert(AUDIO_DEVICE_IN_WIRED_HEADSET);
  handler_.connected_output_devices_.insert(AUDIO_DEVICE_OUT_WIRED_HEADSET);
  handler_.connected_output_devices_.insert(AUDIO_DEVICE_OUT_WIRED_HEADPHONE);
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(AUDIO_DEVICE_IN_WIRED_HEADSET,
                                       AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE));
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(AUDIO_DEVICE_OUT_WIRED_HEADSET,
                                       AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE));
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
                                       AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE));
  handler_.UpdateAudioSystem(handler_.headphone_, handler_.microphone_);
  EXPECT_EQ(handler_.connected_input_devices_.size(), 0);
  EXPECT_EQ(handler_.connected_output_devices_.size(), 0);
}

// Test UpdateAudioSystem() when connecting a microphone.
TEST_F(AudioDeviceHandlerTest, UpdateAudioSystemConnectMic) {
  handler_.microphone_ = true;
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(AUDIO_DEVICE_IN_WIRED_HEADSET,
                                       AUDIO_POLICY_DEVICE_STATE_AVAILABLE));
  handler_.UpdateAudioSystem(handler_.headphone_, handler_.microphone_);
  EXPECT_EQ(handler_.connected_input_devices_.size(), 1);
  EXPECT_EQ(handler_.connected_output_devices_.size(), 0);
}

// Test UpdateAudioSystem() when connecting a headphone.
TEST_F(AudioDeviceHandlerTest, UpdateAudioSystemConnectHeadphone) {
  handler_.headphone_ = true;
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
                                       AUDIO_POLICY_DEVICE_STATE_AVAILABLE));
  handler_.UpdateAudioSystem(handler_.headphone_, handler_.microphone_);
  EXPECT_EQ(handler_.connected_input_devices_.size(), 0);
  EXPECT_EQ(handler_.connected_output_devices_.size(), 1);
}

// Test UpdateAudioSystem() when connecting a headset.
TEST_F(AudioDeviceHandlerTest, UpdateAudioSystemConnectHeadset) {
  handler_.headphone_ = true;
  handler_.microphone_ = true;
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(AUDIO_DEVICE_IN_WIRED_HEADSET,
                                       AUDIO_POLICY_DEVICE_STATE_AVAILABLE));
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(AUDIO_DEVICE_OUT_WIRED_HEADSET,
                                       AUDIO_POLICY_DEVICE_STATE_AVAILABLE));
  handler_.UpdateAudioSystem(handler_.headphone_, handler_.microphone_);
  EXPECT_EQ(handler_.connected_input_devices_.size(), 1);
  EXPECT_EQ(handler_.connected_output_devices_.size(), 1);
}

// Test ConnectAudioDevice() with an input device.
TEST_F(AudioDeviceHandlerTest, ConnectAudioDeviceInput) {
  audio_devices_t device = AUDIO_DEVICE_IN_WIRED_HEADSET;
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(device,
                                       AUDIO_POLICY_DEVICE_STATE_AVAILABLE));
  handler_.ConnectAudioDevice(device);
  EXPECT_EQ(handler_.connected_output_devices_.size(), 0);
  EXPECT_NE(
      handler_.connected_input_devices_.find(device),
      handler_.connected_input_devices_.end());
}

// Test ConnectAudioDevice() with an output device.
TEST_F(AudioDeviceHandlerTest, ConnectAudioDeviceOutput) {
  audio_devices_t device = AUDIO_DEVICE_OUT_WIRED_HEADSET;
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(device,
                                       AUDIO_POLICY_DEVICE_STATE_AVAILABLE));
  handler_.ConnectAudioDevice(device);
  EXPECT_EQ(handler_.connected_input_devices_.size(), 0);
  EXPECT_NE(
      handler_.connected_output_devices_.find(device),
      handler_.connected_output_devices_.end());
}

// Test DisconnectAudioDevice() with an input device.
TEST_F(AudioDeviceHandlerTest, DisconnectAudioDeviceInput) {
  audio_devices_t device = AUDIO_DEVICE_IN_WIRED_HEADSET;
  handler_.connected_input_devices_.insert(device);
  handler_.connected_output_devices_.insert(device);
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(device,
                                       AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE));
  handler_.DisconnectAudioDevice(device);
  EXPECT_EQ(handler_.connected_input_devices_.size(), 0);
  EXPECT_EQ(handler_.connected_output_devices_.size(), 1);
}

// Test DisconnectAudioDevice() with an output device.
TEST_F(AudioDeviceHandlerTest, DisconnectAudioDeviceOutput) {
  audio_devices_t device = AUDIO_DEVICE_OUT_WIRED_HEADSET;
  handler_.connected_input_devices_.insert(device);
  handler_.connected_output_devices_.insert(device);
  EXPECT_CALL(handler_,
              NotifyAudioPolicyService(device,
                                       AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE));
  handler_.DisconnectAudioDevice(device);
  EXPECT_EQ(handler_.connected_input_devices_.size(), 1);
  EXPECT_EQ(handler_.connected_output_devices_.size(), 0);
}

}  // namespace brillo
