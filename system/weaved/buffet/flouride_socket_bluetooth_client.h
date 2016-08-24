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

#ifndef BUFFET_FLOURIDE_SOCKET_BLUETOOTH_CLIENT_H
#define BUFFET_FLOURIDE_SOCKET_BLUETOOTH_CLIENT_H

#include <brillo/streams/stream.h>

#include "buffet/bluetooth_client.h"

namespace buffet {

/**
 * A bluetooth client that talks to Brillo's "Flouride" daemon over its
 * soon-to-be-deprecated Unix domain socket interface.
 *
 * The interface that isn't ready yet will be based on Binder, and we'll
 * jump ship to that when possible.
 */
class FlourideSocketBluetoothClient : public BluetoothClient {
 public:
  explicit FlourideSocketBluetoothClient();
  ~FlourideSocketBluetoothClient() override;

 private:
  bool OpenSocket();

  std::unique_ptr<brillo::Stream> stream_;
};

}  // namespace buffet

#endif  // BUFFET_FLOURIDE_SOCKET_BLUETOOTH_CLIENT_H
