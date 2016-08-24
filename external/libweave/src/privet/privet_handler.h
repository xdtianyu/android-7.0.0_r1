// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_PRIVET_HANDLER_H_
#define LIBWEAVE_SRC_PRIVET_PRIVET_HANDLER_H_

#include <map>
#include <string>
#include <utility>

#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <base/scoped_observer.h>
#include <base/time/default_clock.h>

#include "src/privet/cloud_delegate.h"

namespace base {
class Value;
class DictionaryValue;
}  // namespace base

namespace weave {
namespace privet {

class DeviceDelegate;
class IdentityDelegate;
class SecurityDelegate;
class WifiDelegate;

// Privet V3 HTTP/HTTPS requests handler.
// API details at https://developers.google.com/cloud-devices/
class PrivetHandler : public CloudDelegate::Observer {
 public:
  // Callback to handle requests asynchronously.
  // |status| is HTTP status code.
  // |output| is result returned in HTTP response. Contains result of
  // successfully request of information about error.
  using RequestCallback =
      base::Callback<void(int status, const base::DictionaryValue& output)>;

  PrivetHandler(CloudDelegate* cloud,
                DeviceDelegate* device,
                SecurityDelegate* pairing,
                WifiDelegate* wifi,
                base::Clock* clock = nullptr);
  ~PrivetHandler() override;

  void OnTraitDefsChanged() override;
  void OnStateChanged() override;
  void OnComponentTreeChanged() override;

  std::vector<std::string> GetHttpPaths() const;
  std::vector<std::string> GetHttpsPaths() const;

  // Handles HTTP/HTTPS Privet request.
  // |api| is the path from the HTTP request, e.g /privet/info.
  // |auth_header| is the Authentication header from HTTP request.
  // |input| is the POST data from HTTP request. If nullptr, data format is
  // not valid JSON.
  // |callback| will be called exactly once during or after |HandleRequest|
  // call.
  void HandleRequest(const std::string& api,
                     const std::string& auth_header,
                     const base::DictionaryValue* input,
                     const RequestCallback& callback);

 private:
  using ApiHandler = void (PrivetHandler::*)(const base::DictionaryValue&,
                                             const UserInfo&,
                                             const RequestCallback&);

  // Adds a handler for both HTTP and HTTPS interfaces.
  void AddHandler(const std::string& path, ApiHandler handler, AuthScope scope);

  // Adds a handler for both HTTPS interface only.
  void AddSecureHandler(const std::string& path,
                        ApiHandler handler,
                        AuthScope scope);

  void HandleInfo(const base::DictionaryValue&,
                  const UserInfo& user_info,
                  const RequestCallback& callback);
  void HandlePairingStart(const base::DictionaryValue& input,
                          const UserInfo& user_info,
                          const RequestCallback& callback);
  void HandlePairingConfirm(const base::DictionaryValue& input,
                            const UserInfo& user_info,
                            const RequestCallback& callback);
  void HandlePairingCancel(const base::DictionaryValue& input,
                           const UserInfo& user_info,
                           const RequestCallback& callback);
  void HandleAuth(const base::DictionaryValue& input,
                  const UserInfo& user_info,
                  const RequestCallback& callback);
  void HandleAccessControlClaim(const base::DictionaryValue& input,
                                const UserInfo& user_info,
                                const RequestCallback& callback);
  void HandleAccessControlConfirm(const base::DictionaryValue& input,
                                  const UserInfo& user_info,
                                  const RequestCallback& callback);
  void HandleSetupStart(const base::DictionaryValue& input,
                        const UserInfo& user_info,
                        const RequestCallback& callback);
  void HandleSetupStatus(const base::DictionaryValue&,
                         const UserInfo& user_info,
                         const RequestCallback& callback);
  void HandleState(const base::DictionaryValue& input,
                   const UserInfo& user_info,
                   const RequestCallback& callback);
  void HandleCommandDefs(const base::DictionaryValue& input,
                         const UserInfo& user_info,
                         const RequestCallback& callback);
  void HandleCommandsExecute(const base::DictionaryValue& input,
                             const UserInfo& user_info,
                             const RequestCallback& callback);
  void HandleCommandsStatus(const base::DictionaryValue& input,
                            const UserInfo& user_info,
                            const RequestCallback& callback);
  void HandleCommandsList(const base::DictionaryValue& input,
                          const UserInfo& user_info,
                          const RequestCallback& callback);
  void HandleCommandsCancel(const base::DictionaryValue& input,
                            const UserInfo& user_info,
                            const RequestCallback& callback);
  void HandleCheckForUpdates(const base::DictionaryValue& input,
                             const UserInfo& user_info,
                             const RequestCallback& callback);
  void HandleTraits(const base::DictionaryValue& input,
                    const UserInfo& user_info,
                    const RequestCallback& callback);
  void HandleComponents(const base::DictionaryValue& input,
                        const UserInfo& user_info,
                        const RequestCallback& callback);

  void ReplyWithSetupStatus(const RequestCallback& callback) const;
  void ReplyToUpdateRequest(const RequestCallback& callback) const;
  void OnUpdateRequestTimeout(int update_request_id);

  CloudDelegate* cloud_{nullptr};
  DeviceDelegate* device_{nullptr};
  SecurityDelegate* security_{nullptr};
  WifiDelegate* wifi_{nullptr};
  base::DefaultClock default_clock_;
  base::Clock* clock_{nullptr};

  struct HandlerParameters {
    ApiHandler handler;
    AuthScope scope;
    bool https_only = true;
  };
  std::map<std::string, HandlerParameters> handlers_;

  struct UpdateRequestParameters {
    RequestCallback callback;
    int request_id{0};
    uint64_t state_fingerprint{0};
    uint64_t traits_fingerprint{0};
    uint64_t components_fingerprint{0};
  };
  std::vector<UpdateRequestParameters> update_requests_;
  int last_update_request_id_{0};

  uint64_t state_fingerprint_{1};
  uint64_t traits_fingerprint_{1};
  uint64_t components_fingerprint_{1};
  ScopedObserver<CloudDelegate, CloudDelegate::Observer> cloud_observer_{this};

  base::WeakPtrFactory<PrivetHandler> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(PrivetHandler);
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_PRIVET_HANDLER_H_
