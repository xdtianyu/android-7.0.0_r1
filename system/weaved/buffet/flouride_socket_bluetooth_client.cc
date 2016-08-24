/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "buffet/flouride_socket_bluetooth_client.h"

#include <sys/socket.h>
#include <sys/un.h>

#include <brillo/streams/file_stream.h>

namespace buffet {

const char kFlourideSocketPath[] = "/dev/socket/bluetooth";

std::unique_ptr<BluetoothClient> BluetoothClient::CreateInstance() {
  return std::unique_ptr<BluetoothClient>{new FlourideSocketBluetoothClient};
}

FlourideSocketBluetoothClient::FlourideSocketBluetoothClient() {}

FlourideSocketBluetoothClient::~FlourideSocketBluetoothClient() {}

// TODO(rginda): Call this from somewhere.
bool FlourideSocketBluetoothClient::OpenSocket() {
  LOG(INFO) << "Opening: " << kFlourideSocketPath;

  int socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    PLOG(ERROR) << "Failed to create domain socket: " << kFlourideSocketPath;
    return false;
  }

  sockaddr_un addr{AF_UNIX};
  static_assert(sizeof(kFlourideSocketPath) <= sizeof(addr.sun_path),
                "kFlourideSocketPath too long");
  strncpy(addr.sun_path, kFlourideSocketPath, sizeof(addr.sun_path));
  if (connect(socket_fd, reinterpret_cast<sockaddr *>(&addr),
              sizeof(sockaddr_un))) {
    PLOG(ERROR) << "Failed to connect to domain socket: "
                << kFlourideSocketPath;
    close(socket_fd);
    return false;
  }

  stream_ = brillo::FileStream::FromFileDescriptor(socket_fd, true, nullptr);
  return true;
}

}  // namespace buffet
