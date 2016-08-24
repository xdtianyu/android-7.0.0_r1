//
// Copyright (C) 2012 The Android Open Source Project
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

#ifndef SHILL_NET_IO_HANDLER_H_
#define SHILL_NET_IO_HANDLER_H_

#include <string>

#include <base/callback.h>

#include "shill/net/shill_export.h"

namespace shill {

struct SHILL_EXPORT InputData {
  InputData() : buf(nullptr), len(0) {}
  InputData(unsigned char* in_buf, size_t in_len) : buf(in_buf), len(in_len) {}

  unsigned char* buf;
  size_t len;
};

class SHILL_EXPORT IOHandler {
 public:
  enum ReadyMode {
    kModeInput,
    kModeOutput
  };

  typedef base::Callback<void(const std::string&)> ErrorCallback;
  typedef base::Callback<void(InputData*)> InputCallback;
  typedef base::Callback<void(int)> ReadyCallback;

  // Data buffer size in bytes.
  static const int kDataBufferSize = 4096;

  IOHandler() {}
  virtual ~IOHandler() {}

  virtual void Start() {}
  virtual void Stop() {}
};

}  // namespace shill

#endif  // SHILL_NET_IO_HANDLER_H_
