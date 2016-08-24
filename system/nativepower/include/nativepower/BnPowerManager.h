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

#ifndef SYSTEM_NATIVEPOWER_INCLUDE_NATIVEPOWER_BN_POWER_MANAGER_H_
#define SYSTEM_NATIVEPOWER_INCLUDE_NATIVEPOWER_BN_POWER_MANAGER_H_

#include <binder/IInterface.h>
#include <powermanager/IPowerManager.h>

namespace android {

// Receiver-side binder implementation.
class BnPowerManager : public BnInterface<IPowerManager> {
public:
  // BnInterface:
  status_t onTransact(uint32_t code,
                      const Parcel& data,
                      Parcel* reply,
                      uint32_t flags=0) override;
};

}  // namespace android

#endif  // SYSTEM_NATIVEPOWER_INCLUDE_NATIVEPOWER_BN_POWER_MANAGER_H_
