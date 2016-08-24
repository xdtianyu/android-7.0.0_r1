//
// Copyright (C) 2015 The Android Open Source Project
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

#ifndef APMANAGER_HOSTAPD_MONITOR_H_
#define APMANAGER_HOSTAPD_MONITOR_H_

#include <string>

#include <base/cancelable_callback.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>

#include "apmanager/event_dispatcher.h"

namespace shill {

struct InputData;
class IOHandler;
class IOHandlerFactory;
class Sockets;

}  // namespace shill

namespace apmanager {

// Class for monitoring events from hostapd control interface.
class HostapdMonitor {
 public:
  enum Event {
    kHostapdFailed,
    kHostapdStarted,
    kStationConnected,
    kStationDisconnected,
  };

  typedef base::Callback<void(Event event, const std::string& data)>
      EventCallback;

  HostapdMonitor(const EventCallback& callback_,
                 const std::string& control_interface_path,
                 const std::string& network_interface_name);
  virtual ~HostapdMonitor();

  virtual void Start();

 private:
  friend class HostapdMonitorTest;

  static const char kLocalPathFormat[];
  static const char kHostapdCmdAttach[];
  static const char kHostapdRespOk[];
  static const char kHostapdEventStationConnected[];
  static const char kHostapdEventStationDisconnected[];
  static const int kHostapdCtrlIfaceCheckIntervalMs;
  static const int kHostapdCtrlIfaceCheckMaxAttempts;
  static const int kHostapdAttachTimeoutMs;
  static const int kInvalidSocket;

  // Task for checking if hostapd control interface is up or not.
  void HostapdCtrlIfaceCheckTask();

  // Attach to hostapd control interface to receive unsolicited event
  // notifications.
  void AttachToHostapd();
  void AttachTimeoutHandler();

  bool SendMessage(const char* message, size_t length);
  void ParseMessage(shill::InputData* data);
  void OnReadError(const std::string& error_msg);

  std::unique_ptr<shill::Sockets> sockets_;
  EventCallback event_callback_;

  // File path for interprocess communication with hostapd.
  std::string dest_path_;
  std::string local_path_;

  // Socket descriptor for communication with hostapd.
  int hostapd_socket_;

  base::Callback<void(shill::InputData *)> hostapd_callback_;
  std::unique_ptr<shill::IOHandler> hostapd_input_handler_;
  shill::IOHandlerFactory *io_handler_factory_;
  EventDispatcher* event_dispatcher_;
  base::WeakPtrFactory<HostapdMonitor> weak_ptr_factory_;

  int hostapd_ctrl_iface_check_count_;
  base::CancelableClosure attach_timeout_callback_;

  bool started_;

  DISALLOW_COPY_AND_ASSIGN(HostapdMonitor);
};

}  // namespace apmanager

#endif  // APMANAGER_HOSTAPD_MONITOR_H_
