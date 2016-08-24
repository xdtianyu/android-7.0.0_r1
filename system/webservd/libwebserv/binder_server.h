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

#ifndef WEBSERVER_LIBWEBSERV_BINDER_SERVER_H_
#define WEBSERVER_LIBWEBSERV_BINDER_SERVER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <brillo/message_loops/message_loop.h>
#include <base/bind.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>

#include <binderwrapper/binder_wrapper.h>
#include <utils/StrongPointer.h>
#include <binder/IBinder.h>

#include "android/webservd/IProtocolHandler.h"
#include "android/webservd/IServer.h"
#include "libwebserv/server.h"

namespace libwebserv {

class ProtocolHandler;
class BinderPHGroup;

class LIBWEBSERV_PRIVATE BinderServer : public Server {
 public:
  BinderServer(brillo::MessageLoop* message_loop,
               const base::Closure& on_server_online,
               const base::Closure& on_server_offline,
               android::BinderWrapper* binder_wrapper);
  ~BinderServer() override = default;

  ProtocolHandler* GetDefaultHttpHandler() override;
  ProtocolHandler* GetDefaultHttpsHandler() override;
  ProtocolHandler* GetProtocolHandler(const std::string& name) override;

  bool IsConnected() const override;

  void OnProtocolHandlerConnected(
      const base::Callback<void(ProtocolHandler*)>& callback) override;
  void OnProtocolHandlerDisconnected(
      const base::Callback<void(ProtocolHandler*)>& callback) override;

  base::TimeDelta GetDefaultRequestTimeout() const override;

 private:
  using RemoteServer = android::webservd::IServer;
  using RemoteProtocolHandler = android::webservd::IProtocolHandler;

  void TryConnecting();
  void ClearLocalState();
  bool BuildLocalState(android::sp<android::IBinder> server);

  // Used to poll for webservd availability and notify the user of changes
  brillo::MessageLoop* message_loop_;
  base::Closure on_server_online_;
  base::Closure on_server_offline_;
  android::BinderWrapper* binder_wrapper_;

  android::sp<RemoteServer> remote_server_;
  //std::map<std::string, std::unique_ptr<BinderPHGroup>>
  //    local_protocol_handlers_;

  base::WeakPtrFactory<BinderServer> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(BinderServer);
};

}  // namespace libwebserv

#endif  // WEBSERVER_LIBWEBSERV_BINDER_SERVER_H_
