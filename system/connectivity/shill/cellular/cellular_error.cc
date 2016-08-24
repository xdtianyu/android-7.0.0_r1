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

#include "shill/cellular/cellular_error.h"

#include <string>

#include <mm/mm-modem.h>

using std::string;

#define MM_MODEM_ERROR(error)  MM_MODEM_INTERFACE "." error
#define MM_MOBILE_ERROR(error) MM_MODEM_GSM_INTERFACE "." error

namespace shill {

static const char* kErrorIncorrectPassword =
    MM_MOBILE_ERROR(MM_ERROR_MODEM_GSM_INCORRECTPASSWORD);
static const char* kErrorSimPinRequired =
    MM_MOBILE_ERROR(MM_ERROR_MODEM_GSM_SIMPINREQUIRED);
static const char* kErrorSimPukRequired =
    MM_MOBILE_ERROR(MM_ERROR_MODEM_GSM_SIMPUKREQUIRED);
static const char* kErrorGprsNotSubscribed =
    MM_MOBILE_ERROR(MM_ERROR_MODEM_GSM_GPRSNOTSUBSCRIBED);

// static
void CellularError::FromChromeosDBusError(brillo::Error* dbus_error,
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
  else if (name == kErrorSimPinRequired)
    type = Error::kPinRequired;
  else if (name == kErrorSimPukRequired)
    type = Error::kPinBlocked;
  else if (name == kErrorGprsNotSubscribed)
    type = Error::kInvalidApn;
  else
    type = Error::kOperationFailed;

  if (!msg.empty())
    return error->Populate(type, msg);
  else
    return error->Populate(type);
}

}  // namespace shill
