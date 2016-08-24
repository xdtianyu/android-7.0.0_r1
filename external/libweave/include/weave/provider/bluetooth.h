// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_BLUETOOTH_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_BLUETOOTH_H_

namespace weave {
namespace provider {

// Interface with methods to control bluetooth capability of the device.
class Bluetooth {
 public:
  // TODO(rginda): Add bluetooth interface methods here.

 protected:
  virtual ~Bluetooth() {}
};

}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_BLUETOOTH_H_
