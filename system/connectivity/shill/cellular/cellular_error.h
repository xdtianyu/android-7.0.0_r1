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

#ifndef SHILL_CELLULAR_CELLULAR_ERROR_H_
#define SHILL_CELLULAR_CELLULAR_ERROR_H_

#include <brillo/errors/error.h>

#include "shill/error.h"

namespace shill {

class CellularError {
 public:
  static void FromChromeosDBusError(brillo::Error* dbus_error,
                                    Error* error);

  static void FromMM1ChromeosDBusError(brillo::Error* dbus_error,
                                       Error* error);

 private:
  DISALLOW_COPY_AND_ASSIGN(CellularError);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_CELLULAR_ERROR_H_
