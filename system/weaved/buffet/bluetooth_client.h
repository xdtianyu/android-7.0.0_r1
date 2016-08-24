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

#ifndef BUFFET_BLUETOOTH_CLIENT_H
#define BUFFET_BLUETOOTH_CLIENT_H

#include <memory>

#include <base/macros.h>
#include <weave/provider/bluetooth.h>

namespace buffet {

class BluetoothClient : public weave::provider::Bluetooth {
 public:
  BluetoothClient() {}
  ~BluetoothClient() override = default;

  static std::unique_ptr<BluetoothClient> CreateInstance();

 private:
  DISALLOW_COPY_AND_ASSIGN(BluetoothClient);
};

}  // namespace buffet

#endif  // BUFFET_BLUETOOTH_CLIENT_H
