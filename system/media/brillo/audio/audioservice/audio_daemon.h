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

// Main loop of the brillo audio service.

#ifndef BRILLO_AUDIO_AUDIOSERVICE_AUDIO_DAEMON_H_
#define BRILLO_AUDIO_AUDIOSERVICE_AUDIO_DAEMON_H_

#include <memory>
#include <stack>

#include <base/files/file.h>
#include <base/memory/weak_ptr.h>
#include <brillo/binder_watcher.h>
#include <brillo/daemons/daemon.h>
#include <media/IAudioPolicyService.h>

#include "audio_device_handler.h"

namespace brillo {

class AudioDaemon : public Daemon {
 public:
  AudioDaemon() {}

 protected:
  // Initialize the audio device handler and start pollig the files in
  // /dev/input.
  int OnInit() override;

 private:
  // Callback function for input events. Events are handled by the audio device
  // handler.
  void Callback(base::File* file);

  // Callback function for audio policy service death notification.
  void OnAPSDisconnected();

  // Connect to the audio policy service and register a callback to be invoked
  // if the audio policy service dies.
  void ConnectToAPS();

  // Initialize the audio_device_handler_.
  //
  // Note: This can only occur after we have connected to the audio policy
  // service.
  void InitializeHandler();

  // Store the file objects that are created during initialization for the files
  // being polled. This is done so these objects can be freed when the
  // AudioDaemon object is destroyed.
  std::stack<base::File> files_;
  // Handler for audio device input events.
  std::unique_ptr<AudioDeviceHandler> audio_device_handler_;
  // Used to generate weak_ptr to AudioDaemon for use in base::Bind.
  base::WeakPtrFactory<AudioDaemon> weak_ptr_factory_{this};
  // Pointer to the audio policy service.
  android::sp<android::IAudioPolicyService> aps_;
  // Flag to indicate whether the handler has been initialized.
  bool handler_initialized_ = false;
  // Binder watcher to watch for binder messages.
  brillo::BinderWatcher binder_watcher_;
};

}  // namespace brillo

#endif  // BRILLO_AUDIO_AUDIOSERVICE_AUDIO_DAEMON_H_
