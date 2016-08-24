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

#ifndef SHILL_PROPERTY_OBSERVER_INTERFACE_H_
#define SHILL_PROPERTY_OBSERVER_INTERFACE_H_

#include <base/macros.h>

namespace shill {

// An abstract interface for objects that retain a saved copy of
// a property accessor and can report whether that value has changed.
class PropertyObserverInterface {
 public:
  PropertyObserverInterface() {}
  virtual ~PropertyObserverInterface() {}

  // Update the saved value used for comparison.
  virtual void Update() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(PropertyObserverInterface);
};

}  // namespace shill

#endif  // SHILL_PROPERTY_OBSERVER_INTERFACE_H_
