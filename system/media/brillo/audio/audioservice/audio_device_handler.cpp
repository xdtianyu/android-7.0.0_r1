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

// Implementation of audio_device_handler.h

#include "audio_device_handler.h"

#include <base/files/file.h>
#include <base/logging.h>
#include <media/AudioSystem.h>

namespace brillo {

static const char kH2WStateFile[] = "/sys/class/switch/h2w/state";

AudioDeviceHandler::AudioDeviceHandler() {
  headphone_ = false;
  microphone_ = false;
}

AudioDeviceHandler::~AudioDeviceHandler() {}

void AudioDeviceHandler::APSDisconnect() {
  aps_.clear();
}

void AudioDeviceHandler::APSConnect(
    android::sp<android::IAudioPolicyService> aps) {
  aps_ = aps;
  // Reset the state
  connected_input_devices_.clear();
  connected_output_devices_.clear();
  // Inform audio policy service about the currently connected devices.
  VLOG(1) << "Calling GetInitialAudioDeviceState on APSConnect.";
  GetInitialAudioDeviceState(base::FilePath(kH2WStateFile));
}

void AudioDeviceHandler::Init(android::sp<android::IAudioPolicyService> aps) {
  aps_ = aps;
  // Reset audio policy service state in case this service crashed and there is
  // a mismatch between the current system state and what audio policy service
  // was previously told.
  VLOG(1) << "Calling DisconnectAllSupportedDevices.";
  DisconnectAllSupportedDevices();

  // Get headphone jack state and update audio policy service with new state.
  VLOG(1) << "Calling ReadInitialAudioDeviceState.";
  GetInitialAudioDeviceState(base::FilePath(kH2WStateFile));
}

void AudioDeviceHandler::GetInitialAudioDeviceState(
    const base::FilePath& path) {
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    LOG(WARNING) << "Kernel does not have wired headset support. Could not "
                 << "open " << path.value() << "( "
                 << base::File::ErrorToString(file.error_details()) << " ).";
    return;
  }
  int state = 0;
  int bytes_read = file.ReadAtCurrentPos(reinterpret_cast<char*>(&state), 1);
  state -= '0';
  if (bytes_read == 0) {
    LOG(WARNING) << "Could not read from " << path.value();
    return;
  }
  VLOG(1) << "Initial audio jack state is " << state;
  static const int kHeadPhoneMask = 0x1;
  bool headphone = state & kHeadPhoneMask;
  static const int kMicrophoneMask = 0x2;
  bool microphone = (state & kMicrophoneMask) >> 1;

  UpdateAudioSystem(headphone, microphone);
}

void AudioDeviceHandler::NotifyAudioPolicyService(
    audio_devices_t device, audio_policy_dev_state_t state) {
  if (aps_ == nullptr) {
    LOG(INFO) << "Audio device handler cannot call audio policy service. Will "
              << "try again later.";
    return;
  }
  VLOG(1) << "Calling Audio Policy Service to change " << device << " to state "
          << state;
  aps_->setDeviceConnectionState(device, state, "", "");
}

void AudioDeviceHandler::ConnectAudioDevice(audio_devices_t device) {
  audio_policy_dev_state_t state = AUDIO_POLICY_DEVICE_STATE_AVAILABLE;
  NotifyAudioPolicyService(device, state);
  if (audio_is_input_device(device))
    connected_input_devices_.insert(device);
  else
    connected_output_devices_.insert(device);
}

void AudioDeviceHandler::DisconnectAudioDevice(audio_devices_t device) {
  audio_policy_dev_state_t state = AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE;
  NotifyAudioPolicyService(device, state);
  if (audio_is_input_device(device))
    connected_input_devices_.erase(device);
  else
    connected_output_devices_.erase(device);
}

void AudioDeviceHandler::DisconnectAllSupportedDevices() {
  for (auto device : kSupportedInputDevices_) {
    DisconnectAudioDevice(device);
  }
  for (auto device : kSupportedOutputDevices_) {
    DisconnectAudioDevice(device);
  }
}

void AudioDeviceHandler::DisconnectAllConnectedDevices() {
  while (!connected_input_devices_.empty()) {
    audio_devices_t device = *(connected_input_devices_.begin());
    DisconnectAudioDevice(device);
  }
  while (!connected_output_devices_.empty()) {
    audio_devices_t device = *(connected_output_devices_.begin());
    DisconnectAudioDevice(device);
  }
}

void AudioDeviceHandler::UpdateAudioSystem(bool headphone, bool microphone) {
  if (microphone) {
    ConnectAudioDevice(AUDIO_DEVICE_IN_WIRED_HEADSET);
  }
  if (headphone && microphone) {
    ConnectAudioDevice(AUDIO_DEVICE_OUT_WIRED_HEADSET);
  } else if (headphone) {
    ConnectAudioDevice(AUDIO_DEVICE_OUT_WIRED_HEADPHONE);
  } else if (!microphone) {
    // No devices are connected. Inform the audio policy service that all
    // connected devices have been disconnected.
    DisconnectAllConnectedDevices();
  }
}

void AudioDeviceHandler::ProcessEvent(const struct input_event& event) {
  VLOG(1) << event.type << " " << event.code << " " << event.value;
  if (event.type == EV_SW) {
    switch (event.code) {
      case SW_HEADPHONE_INSERT:
        headphone_ = event.value;
        break;
      case SW_MICROPHONE_INSERT:
        microphone_ = event.value;
        break;
      default:
        // This event code is not supported by this handler.
        break;
    }
  } else if (event.type == EV_SYN) {
    // We have received all input events. Update the audio system.
    UpdateAudioSystem(headphone_, microphone_);
    // Reset the headphone and microphone flags that are used to track
    // information across multiple calls to ProcessEvent.
    headphone_ = false;
    microphone_ = false;
  }
}

}  // namespace brillo
