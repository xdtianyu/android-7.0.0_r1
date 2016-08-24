//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef TRUNKS_DBUS_INTERFACE_H_
#define TRUNKS_DBUS_INTERFACE_H_

namespace trunks {

#if defined(__ANDROID__)
constexpr char kTrunksInterface[] = "com.android.Trunks";
constexpr char kTrunksServicePath[] = "/com/android/Trunks";
constexpr char kTrunksServiceName[] = "com.android.Trunks";
#else
constexpr char kTrunksInterface[] = "org.chromium.Trunks";
constexpr char kTrunksServicePath[] = "/org/chromium/Trunks";
constexpr char kTrunksServiceName[] = "org.chromium.Trunks";
#endif

// Methods exported by trunks.
constexpr char kSendCommand[] = "SendCommand";

};  // namespace trunks

#endif  // TRUNKS_DBUS_INTERFACE_H_
