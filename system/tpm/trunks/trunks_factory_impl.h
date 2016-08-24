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

#ifndef TRUNKS_TRUNKS_FACTORY_IMPL_H_
#define TRUNKS_TRUNKS_FACTORY_IMPL_H_

#include "trunks/trunks_factory.h"

#include <string>

#include <base/macros.h>
#include <base/memory/scoped_ptr.h>

#include "trunks/command_transceiver.h"
#include "trunks/trunks_export.h"

namespace trunks {

class Tpm;

// TrunksFactoryImpl is the default TrunksFactory implementation.
class TRUNKS_EXPORT TrunksFactoryImpl : public TrunksFactory {
 public:
  // Uses an IPC proxy as the default CommandTransceiver. If |failure_is_fatal|
  // is set then a failure to initialize the proxy will abort.
  explicit TrunksFactoryImpl(bool failure_is_fatal);
  // TrunksFactoryImpl does not take ownership of |transceiver|. This
  // transceiver is forwarded down to the Tpm instance maintained by
  // this factory.
  explicit TrunksFactoryImpl(CommandTransceiver* transceiver);
  ~TrunksFactoryImpl() override;

  // TrunksFactory methods.
  Tpm* GetTpm() const override;
  scoped_ptr<TpmState> GetTpmState() const override;
  scoped_ptr<TpmUtility> GetTpmUtility() const override;
  scoped_ptr<AuthorizationDelegate> GetPasswordAuthorization(
      const std::string& password) const override;
  scoped_ptr<SessionManager> GetSessionManager() const override;
  scoped_ptr<HmacSession> GetHmacSession() const override;
  scoped_ptr<PolicySession> GetPolicySession() const override;
  scoped_ptr<PolicySession> GetTrialSession() const override;
  scoped_ptr<BlobParser> GetBlobParser() const override;

 private:
  scoped_ptr<CommandTransceiver> default_transceiver_;
  CommandTransceiver* transceiver_;
  scoped_ptr<Tpm> tpm_;

  DISALLOW_COPY_AND_ASSIGN(TrunksFactoryImpl);
};

}  // namespace trunks

#endif  // TRUNKS_TRUNKS_FACTORY_IMPL_H_
