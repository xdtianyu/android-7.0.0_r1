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

// Implementation of audio_daemon.h.

#include "audio_daemon.h"

#include <sysexits.h>

#include <base/bind.h>
#include <base/files/file_enumerator.h>
#include <base/files/file_path.h>
#include <base/time/time.h>
#include <binderwrapper/binder_wrapper.h>
#include <linux/input.h>

namespace brillo {

static const char kAPSServiceName[] = "media.audio_policy";
static const char kInputDeviceDir[] = "/dev/input";

void AudioDaemon::InitializeHandler() {
  // Start and initialize the audio device handler.
  audio_device_handler_ =
      std::unique_ptr<AudioDeviceHandler>(new AudioDeviceHandler());
  audio_device_handler_->Init(aps_);

  // Poll on all files in kInputDeviceDir.
  base::FileEnumerator fenum(base::FilePath(kInputDeviceDir),
                             false /*recursive*/, base::FileEnumerator::FILES);
  for (base::FilePath name = fenum.Next(); !name.empty(); name = fenum.Next()) {
    base::File file(name, base::File::FLAG_OPEN | base::File::FLAG_READ);
    if (file.IsValid()) {
      MessageLoop* message_loop = MessageLoop::current();
      int fd = file.GetPlatformFile();
      // Move file to files_ and ensure that when binding we get a pointer from
      // the object in files_.
      files_.emplace(std::move(file));
      base::Closure callback =
          base::Bind(&AudioDaemon::Callback, weak_ptr_factory_.GetWeakPtr(),
                     &files_.top());
      message_loop->WatchFileDescriptor(fd, MessageLoop::kWatchRead,
                                        true /*persistent*/, callback);
    } else {
      LOG(WARNING) << "Could not open " << name.value() << " for reading. ("
                   << base::File::ErrorToString(file.error_details()) << ")";
    }
  }
  handler_initialized_ = true;
}

void AudioDaemon::ConnectToAPS() {
  android::BinderWrapper* binder_wrapper = android::BinderWrapper::Get();
  auto binder = binder_wrapper->GetService(kAPSServiceName);
  // If we didn't get the audio policy service, try again in 500 ms.
  if (!binder.get()) {
    LOG(INFO) << "Could not connect to audio policy service. Trying again...";
    brillo::MessageLoop::current()->PostDelayedTask(
        base::Bind(&AudioDaemon::ConnectToAPS, weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(500));
    return;
  }
  LOG(INFO) << "Connected to audio policy service.";
  binder_wrapper->RegisterForDeathNotifications(
      binder,
      base::Bind(&AudioDaemon::OnAPSDisconnected,
                 weak_ptr_factory_.GetWeakPtr()));
  VLOG(1) << "Registered death notification.";
  aps_ = android::interface_cast<android::IAudioPolicyService>(binder);
  if (!handler_initialized_)
    InitializeHandler();
  else
    audio_device_handler_->APSConnect(aps_);
}

void AudioDaemon::OnAPSDisconnected() {
  LOG(INFO) << "Audio policy service died. Will try to reconnect.";
  audio_device_handler_->APSDisconnect();
  aps_ = nullptr;
  ConnectToAPS();
}

// OnInit, we want to do the following:
//   - Get a binder to the audio policy service.
//   - Initialize the audio device handler.
//   - Set up polling on files in /dev/input.
int AudioDaemon::OnInit() {
  int exit_code = Daemon::OnInit();
  if (exit_code != EX_OK) return exit_code;
  // Initialize a binder wrapper.
  android::BinderWrapper::Create();
  // Initialize a binder watcher.
  binder_watcher_.Init();
  ConnectToAPS();
  return EX_OK;
}

void AudioDaemon::Callback(base::File* file) {
  input_event event;
  int bytes_read =
      file->ReadAtCurrentPos(reinterpret_cast<char*>(&event), sizeof(event));
  if (bytes_read != sizeof(event)) {
    LOG(WARNING) << "Couldn't read an input event.";
    return;
  }
  audio_device_handler_->ProcessEvent(event);
}

}  // namespace brillo
