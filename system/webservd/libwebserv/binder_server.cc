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

#include "libwebserv/binder_server.h"

#include <utils/String16.h>

#include "libwebserv/binder_constants.h"

#include <string>
#include <vector>

using android::String16;
using android::sp;
using android::IBinder;
using std::string;
using std::vector;

namespace libwebserv {

BinderServer::BinderServer(brillo::MessageLoop* message_loop,
                           const base::Closure& on_server_online,
                           const base::Closure& on_server_offline,
                           android::BinderWrapper* binder_wrapper)
    : message_loop_{message_loop},
      on_server_online_{on_server_online},
      on_server_offline_{on_server_offline},
      binder_wrapper_{binder_wrapper} {
  message_loop_->PostTask(FROM_HERE,
                          base::Bind(&BinderServer::TryConnecting,
                                     weak_ptr_factory_.GetWeakPtr()));
}

void BinderServer::TryConnecting() {
  ClearLocalState();
  android::sp<android::IBinder> binder = binder_wrapper_->GetService(
      libwebserv::kWebserverBinderServiceName);
  if (!binder.get()) {
    LOG(INFO) << "Webservd has not registered with service manager.";
  } else if (!BuildLocalState(binder)) {
    ClearLocalState();
  } else {
    // Got a binder, built up appropriate local state, our job is done.
    return;
  }
  message_loop_->PostDelayedTask(FROM_HERE,
                                 base::Bind(&BinderServer::TryConnecting,
                                            weak_ptr_factory_.GetWeakPtr()),
                                 base::TimeDelta::FromSeconds(1));
}


void BinderServer::ClearLocalState() {
  // Remove all references to remote protocol handlers from our local wrappers.

  // TODO(wiley) Define this method
  //for (auto& local_handler: local_protocol_handlers_) {
  //  local_handler.second->ResetRemoteProtocolHandlers();
  //}

  remote_server_.clear();
}

bool BinderServer::BuildLocalState(android::sp<android::IBinder> server) {
  remote_server_ = android::interface_cast<RemoteServer>(server);
  vector<sp<IBinder>> remote_raw_binders;
  if (!remote_server_->GetProtocolHandlers(&remote_raw_binders).isOk()) {
    // Possibly the server died, this is not necessarily an error.
    LOG(INFO) << "Webservd failed to tell us about protocol handlers.";
    return false;
  }

  // Tell the local wrappers about the remote handlers that exist now.
  for (auto& raw_binder: remote_raw_binders) {
    sp<RemoteProtocolHandler> remote_handler =
        android::interface_cast<RemoteProtocolHandler>(raw_binder);
    String16 name;
    if (!remote_handler->GetName(&name).isOk()) {
      LOG(INFO) << "Remote handler could not report its name.";
      return false;
    }
    // TODO(wiley) Look for a BinderPHGroup by that name in local_handlers_
    //             Create a new BinderPHGroup if necessary
    //             Add it to the map under |name|
    //             Update |it| appropriately
    //             Tell |it| about |remote_handler|
  }
  return true;
}

}  // namespace libwebserv
