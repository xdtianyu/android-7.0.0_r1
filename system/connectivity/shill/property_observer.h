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

#ifndef SHILL_PROPERTY_OBSERVER_H_
#define SHILL_PROPERTY_OBSERVER_H_

#include <memory>

#include <base/callback.h>
#include <base/macros.h>

#include "shill/accessor_interface.h"
#include "shill/error.h"
#include "shill/property_observer_interface.h"

namespace shill {

// A templated object that retains a reference to a typed accessor,
// and a saved value retrieved from the accessor.  When the update
// method is called, it compares its saved value to the current
// value returned by the accessor.  If the value has changed, it
// calls the supplied callback and updates the saved value.
template <class T>
class PropertyObserver : public PropertyObserverInterface {
 public:
  typedef base::Callback<void(const T& new_value)> Callback;

  PropertyObserver(std::shared_ptr<AccessorInterface<T>> accessor,
                   Callback callback)
      : accessor_(accessor), callback_(callback) {
    Error unused_error;
    saved_value_ = accessor_->Get(&unused_error);
  }
  ~PropertyObserver() override {}

  // Implements PropertyObserverInterface.  Compares the saved value with
  // what the Get() method of |accessor_| returns.  If the value has changed
  // |callback_| is invoked and |saved_value_| is updated.
  void Update() override {
    Error error;
    T new_value_ = accessor_->Get(&error);
    if (!error.IsSuccess() || saved_value_ == new_value_) {
      return;
    }
    callback_.Run(new_value_);
    saved_value_ = new_value_;
  }

 private:
  friend class PropertyObserverTest;

  std::shared_ptr<AccessorInterface<T>> accessor_;
  Callback callback_;
  T saved_value_;

  DISALLOW_COPY_AND_ASSIGN(PropertyObserver);
};

}  // namespace shill

#endif  // SHILL_PROPERTY_OBSERVER_H_
