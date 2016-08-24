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

#include "apmanager/hostapd_monitor.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <base/bind.h>
#include <base/logging.h>
#include <base/strings/stringprintf.h>
#include <shill/net/io_handler_factory_container.h>
#include <shill/net/sockets.h>

using base::Bind;
using base::Unretained;
using shill::IOHandlerFactoryContainer;
using std::string;

namespace apmanager {

// static.
#if !defined(__ANDROID__)
const char HostapdMonitor::kLocalPathFormat[] =
    "/var/run/apmanager/hostapd/hostapd_ctrl_%s";
#else
const char HostapdMonitor::kLocalPathFormat[] =
    "/data/misc/apmanager/hostapd/hostapd_ctrl_%s";
#endif  // __ANDROID__

const char HostapdMonitor::kHostapdCmdAttach[] = "ATTACH";
const char HostapdMonitor::kHostapdRespOk[] = "OK\n";
const char HostapdMonitor::kHostapdEventStationConnected[] = "AP-STA-CONNECTED";
const char HostapdMonitor::kHostapdEventStationDisconnected[] =
    "AP-STA-DISCONNECTED";
const int HostapdMonitor::kHostapdCtrlIfaceCheckIntervalMs = 500;
const int HostapdMonitor::kHostapdCtrlIfaceCheckMaxAttempts = 5;
const int HostapdMonitor::kHostapdAttachTimeoutMs = 1000;
const int HostapdMonitor::kInvalidSocket = -1;

HostapdMonitor::HostapdMonitor(const EventCallback& callback,
                               const string& control_interface_path,
                               const string& network_interface_name)
    : sockets_(new shill::Sockets()),
      event_callback_(callback),
      dest_path_(base::StringPrintf("%s/%s",
                                    control_interface_path.c_str(),
                                    network_interface_name.c_str())),
      local_path_(base::StringPrintf(kLocalPathFormat,
                                     network_interface_name.c_str())),
      hostapd_socket_(kInvalidSocket),
      io_handler_factory_(
          IOHandlerFactoryContainer::GetInstance()->GetIOHandlerFactory()),
      event_dispatcher_(EventDispatcher::GetInstance()),
      weak_ptr_factory_(this),
      started_(false) {}

HostapdMonitor::~HostapdMonitor() {
  if (hostapd_socket_ != kInvalidSocket) {
    unlink(local_path_.c_str());
    sockets_->Close(hostapd_socket_);
  }
}

void HostapdMonitor::Start() {
  if (started_) {
    LOG(ERROR) << "HostapdMonitor already started";
    return;
  }

  hostapd_ctrl_iface_check_count_ = 0;
  // Start off by checking the control interface file for the hostapd process.
  event_dispatcher_->PostTask(
      Bind(&HostapdMonitor::HostapdCtrlIfaceCheckTask,
           weak_ptr_factory_.GetWeakPtr()));
  started_ = true;
}

void HostapdMonitor::HostapdCtrlIfaceCheckTask() {
  struct stat buf;
  if (stat(dest_path_.c_str(), &buf) != 0) {
    if (hostapd_ctrl_iface_check_count_ >= kHostapdCtrlIfaceCheckMaxAttempts) {
      // This indicates the hostapd failed to start. Invoke callback indicating
      // hostapd start failed.
      LOG(ERROR) << "Timeout waiting for hostapd control interface";
      event_callback_.Run(kHostapdFailed, "");
    } else {
      hostapd_ctrl_iface_check_count_++;
      event_dispatcher_->PostDelayedTask(
          base::Bind(&HostapdMonitor::HostapdCtrlIfaceCheckTask,
                     weak_ptr_factory_.GetWeakPtr()),
          kHostapdCtrlIfaceCheckIntervalMs);
    }
    return;
  }

  // Control interface is up, meaning hostapd started successfully.
  event_callback_.Run(kHostapdStarted, "");

  // Attach to the control interface to receive unsolicited event notifications.
  AttachToHostapd();
}

void HostapdMonitor::AttachToHostapd() {
  if (hostapd_socket_ != kInvalidSocket) {
    LOG(ERROR) << "Socket already initialized";
    return;
  }

  // Setup socket address for local file and remote file.
  struct sockaddr_un local;
  local.sun_family = AF_UNIX;
  snprintf(local.sun_path, sizeof(local.sun_path), "%s", local_path_.c_str());
  struct sockaddr_un dest;
  dest.sun_family = AF_UNIX;
  snprintf(dest.sun_path, sizeof(dest.sun_path), "%s", dest_path_.c_str());

  // Setup socket for interprocess communication.
  hostapd_socket_ = sockets_->Socket(PF_UNIX, SOCK_DGRAM, 0);
  if (hostapd_socket_ < 0) {
    LOG(ERROR) << "Failed to open hostapd socket";
    return;
  }
  if (sockets_->Bind(hostapd_socket_,
                     reinterpret_cast<struct sockaddr*>(&local),
                     sizeof(local)) < 0) {
    PLOG(ERROR) << "Failed to bind to local socket";
    return;
  }
  if (sockets_->Connect(hostapd_socket_,
                        reinterpret_cast<struct sockaddr*>(&dest),
                        sizeof(dest)) < 0) {
    PLOG(ERROR) << "Failed to connect";
    return;
  }

  // Setup IO Input handler.
  hostapd_input_handler_.reset(io_handler_factory_->CreateIOInputHandler(
      hostapd_socket_,
      Bind(&HostapdMonitor::ParseMessage, Unretained(this)),
      Bind(&HostapdMonitor::OnReadError, Unretained(this))));

  if (!SendMessage(kHostapdCmdAttach, strlen(kHostapdCmdAttach))) {
    LOG(ERROR) << "Failed to attach to hostapd";
    return;
  }

  // Start a timer for ATTACH response.
  attach_timeout_callback_.Reset(
      Bind(&HostapdMonitor::AttachTimeoutHandler,
           weak_ptr_factory_.GetWeakPtr()));
  event_dispatcher_->PostDelayedTask(attach_timeout_callback_.callback(),
                                     kHostapdAttachTimeoutMs);
  return;
}

void HostapdMonitor::AttachTimeoutHandler() {
  LOG(ERROR) << "Timeout waiting for attach response";
}

// Method for sending message to hostapd control interface.
bool HostapdMonitor::SendMessage(const char* message, size_t length) {
  if (sockets_->Send(hostapd_socket_, message, length, 0) < 0) {
    PLOG(ERROR) << "Send to hostapd failed";
    return false;
  }

  return true;
}

void HostapdMonitor::ParseMessage(shill::InputData* data) {
  string str(reinterpret_cast<const char*>(data->buf), data->len);
  // "OK" response for the "ATTACH" command.
  if (str == kHostapdRespOk) {
    attach_timeout_callback_.Cancel();
    return;
  }

  // Event messages are in format of <[Level]>[Event] [Detail message].
  // For example: <2>AP-STA-CONNECTED 00:11:22:33:44:55
  // Refer to wpa_ctrl.h for complete list of possible events.
  if (str.find_first_of('<', 0) == 0 && str.find_first_of('>', 0) == 2) {
    // Remove the log level.
    string msg = str.substr(3);
    string event;
    string data;
    size_t pos = msg.find_first_of(' ', 0);
    if (pos == string::npos) {
      event = msg;
    } else {
      event = msg.substr(0, pos);
      data = msg.substr(pos + 1);
    }

    Event event_code;
    if (event == kHostapdEventStationConnected) {
      event_code = kStationConnected;
    } else if (event == kHostapdEventStationDisconnected) {
      event_code = kStationDisconnected;
    } else {
      LOG(INFO) << "Received unknown event: " << event;
      return;
    }
    event_callback_.Run(event_code, data);
    return;
  }

  LOG(INFO) << "Received unknown message: " << str;
}

void HostapdMonitor::OnReadError(const string& error_msg) {
  LOG(FATAL) << "Hostapd Socket read returns error: "
             << error_msg;
}

}  // namespace apmanager
