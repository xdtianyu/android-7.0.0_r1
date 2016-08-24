//
// Copyright (C) 2016 The Android Open Source Project
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

#ifndef SHILL_BINDER_BINDER_ADAPTOR_H_
#define SHILL_BINDER_BINDER_ADAPTOR_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <utils/StrongPointer.h>

namespace android {
namespace system {
namespace connectivity {
namespace shill {
class IPropertyChangedCallback;
}  // shill
}  // connectivity
}  // system
}  // android

namespace shill {

// Superclass for all Binder-backed Adaptor objects.
class BinderAdaptor {
 public:
  explicit BinderAdaptor(const std::string& id);
  ~BinderAdaptor() = default;

 protected:
  // Add a IPropertyChangedCallback binder to |property_changed_callbacks_|.
  // This binder's OnPropertyChanged() method will be invoked when shill
  // properties change.
  void AddPropertyChangedSignalHandler(
      const android::sp<
          android::system::connectivity::shill::IPropertyChangedCallback>&
          property_changed_callback);

  // Signals all registered listeners the shill property |name| has changed by
  // calling the OnPropertyChanged() method of all IPropertyChangedCallback
  // binders in |property_changed_callbacks_|.
  void SendPropertyChangedSignal(const std::string& name);

  const std::string& id() { return id_; }

 private:
  // Used to uniquely identify this Binder adaptor.
  std::string id_;

  std::vector<android::sp<
      android::system::connectivity::shill::IPropertyChangedCallback>>
      property_changed_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(BinderAdaptor);
};

}  // namespace shill

#endif  // SHILL_BINDER_BINDER_ADAPTOR_H_
