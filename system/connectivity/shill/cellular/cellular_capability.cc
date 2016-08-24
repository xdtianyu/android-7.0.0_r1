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

#include "shill/cellular/cellular_capability.h"

#include <base/bind.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/cellular/cellular.h"
#include "shill/error.h"
#include "shill/logging.h"
#include "shill/property_accessor.h"

using base::Closure;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kCellular;
static string ObjectID(CellularCapability* c) {
  return c->cellular()->GetRpcIdentifier();
}
}

const char CellularCapability::kModemPropertyIMSI[] = "imsi";
const char CellularCapability::kModemPropertyState[] = "State";
// All timeout values are in milliseconds
const int CellularCapability::kTimeoutActivate = 300000;
const int CellularCapability::kTimeoutConnect = 45000;
const int CellularCapability::kTimeoutDefault = 5000;
const int CellularCapability::kTimeoutDisconnect = 45000;
const int CellularCapability::kTimeoutEnable = 45000;
const int CellularCapability::kTimeoutRegister = 90000;
const int CellularCapability::kTimeoutReset = 90000;
const int CellularCapability::kTimeoutScan = 120000;

CellularCapability::CellularCapability(Cellular* cellular,
                                       ControlInterface* control_interface,
                                       ModemInfo* modem_info)
    : cellular_(cellular),
      control_interface_(control_interface),
      modem_info_(modem_info) {}

CellularCapability::~CellularCapability() {}

void CellularCapability::OnUnsupportedOperation(const char* operation,
                                                Error* error) {
  string message("The ");
  message.append(operation).append(" operation is not supported.");
  Error::PopulateAndLog(FROM_HERE, error, Error::kNotSupported, message);
}

void CellularCapability::DisconnectCleanup() {}

void CellularCapability::Activate(const string& carrier,
                                  Error* error,
                                  const ResultCallback& callback) {
  OnUnsupportedOperation(__func__, error);
}

void CellularCapability::CompleteActivation(Error* error) {
  OnUnsupportedOperation(__func__, error);
}

bool CellularCapability::IsServiceActivationRequired() const {
  return false;
}

void CellularCapability::RegisterOnNetwork(
    const string& /*network_id*/,
    Error* error,
    const ResultCallback& /*callback*/) {
  OnUnsupportedOperation(__func__, error);
}

void CellularCapability::RequirePIN(const std::string& /*pin*/,
                                    bool /*require*/,
                                    Error* error,
                                    const ResultCallback& /*callback*/) {
  OnUnsupportedOperation(__func__, error);
}

void CellularCapability::EnterPIN(const string& /*pin*/,
                                  Error* error,
                                  const ResultCallback& /*callback*/) {
  OnUnsupportedOperation(__func__, error);
}

void CellularCapability::UnblockPIN(const string& /*unblock_code*/,
                                    const string& /*pin*/,
                                    Error* error,
                                    const ResultCallback& /*callback*/) {
  OnUnsupportedOperation(__func__, error);
}

void CellularCapability::ChangePIN(const string& /*old_pin*/,
                                   const string& /*new_pin*/,
                                   Error* error,
                                   const ResultCallback& /*callback*/) {
  OnUnsupportedOperation(__func__, error);
}

void CellularCapability::Scan(Error* error,
                              const ResultStringmapsCallback& callback) {
  OnUnsupportedOperation(__func__, error);
}

void CellularCapability::Reset(Error* error,
                               const ResultCallback& /*callback*/) {
  OnUnsupportedOperation(__func__, error);
}

void CellularCapability::SetCarrier(const std::string& /*carrier*/,
                                    Error* error,
                                    const ResultCallback& /*callback*/) {
  OnUnsupportedOperation(__func__, error);
}

CellularBearer* CellularCapability::GetActiveBearer() const {
  return nullptr;
}

bool CellularCapability::IsActivating() const {
  return false;
}

bool CellularCapability::ShouldDetectOutOfCredit() const {
  return false;
}

void CellularCapability::OnOperatorChanged() {
  SLOG(this, 3) << __func__;
  if (cellular()->service()) {
    UpdateServiceOLP();
  }
}

void CellularCapability::UpdateServiceOLP() {
  SLOG(this, 3) << __func__;
}

}  // namespace shill
