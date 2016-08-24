//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/cellular/cellular_error.h"

#include <string>

#include <ModemManager/ModemManager.h>

// TODO(armansito): Once we refactor the code to handle the ModemManager D-Bus
// bindings in a dedicated class, this code should move there.
// (See crbug.com/246425)

using std::string;

namespace shill {

namespace {

const char* kErrorGprsMissingOrUnknownApn =
    MM_MOBILE_EQUIPMENT_ERROR_DBUS_PREFIX ".GprsMissingOrUnknownApn";

const char* kErrorGprsServiceOptionNotSubscribed =
    MM_MOBILE_EQUIPMENT_ERROR_DBUS_PREFIX ".GprsServiceOptionNotSubscribed";

const char* kErrorIncorrectPassword =
    MM_MOBILE_EQUIPMENT_ERROR_DBUS_PREFIX ".IncorrectPassword";

const char* kErrorSimPin =
    MM_MOBILE_EQUIPMENT_ERROR_DBUS_PREFIX ".SimPin";

const char* kErrorSimPuk =
    MM_MOBILE_EQUIPMENT_ERROR_DBUS_PREFIX ".SimPuk";

const char* kErrorWrongState = MM_CORE_ERROR_DBUS_PREFIX ".WrongState";

}  // namespace

// static
void CellularError::FromMM1ChromeosDBusError(brillo::Error* dbus_error,
                                             Error* error) {
  if (!error)
    return;

  if (!dbus_error) {
    error->Reset();
    return;
  }

  const string name = dbus_error->GetCode();
  const string msg = dbus_error->GetMessage();
  Error::Type type;

  if (name == kErrorIncorrectPassword)
    type = Error::kIncorrectPin;
  else if (name == kErrorSimPin)
    type = Error::kPinRequired;
  else if (name == kErrorSimPuk)
    type = Error::kPinBlocked;
  else if (name == kErrorGprsMissingOrUnknownApn)
    type = Error::kInvalidApn;
  else if (name == kErrorGprsServiceOptionNotSubscribed)
    type = Error::kInvalidApn;
  else if (name == kErrorWrongState)
    type = Error::kWrongState;
  else
    type = Error::kOperationFailed;

  if (!msg.empty())
    return error->Populate(type, msg);
  else
    return error->Populate(type);
}

}  // namespace shill
