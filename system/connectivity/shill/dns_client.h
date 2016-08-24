//
// Copyright (C) 2011 The Android Open Source Project
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

#ifndef SHILL_DNS_CLIENT_H_
#define SHILL_DNS_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/cancelable_callback.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/error.h"
#include "shill/event_dispatcher.h"
#include "shill/net/ip_address.h"
#include "shill/refptr_types.h"

struct hostent;

namespace shill {

class Ares;
class Time;
struct DNSClientState;

// Implements a DNS resolution client that can run asynchronously.
class DNSClient {
 public:
  typedef base::Callback<void(const Error&, const IPAddress&)> ClientCallback;

  static const char kErrorNoData[];
  static const char kErrorFormErr[];
  static const char kErrorServerFail[];
  static const char kErrorNotFound[];
  static const char kErrorNotImp[];
  static const char kErrorRefused[];
  static const char kErrorBadQuery[];
  static const char kErrorNetRefused[];
  static const char kErrorTimedOut[];
  static const char kErrorUnknown[];

  DNSClient(IPAddress::Family family,
            const std::string& interface_name,
            const std::vector<std::string>& dns_servers,
            int timeout_ms,
            EventDispatcher* dispatcher,
            const ClientCallback& callback);
  virtual ~DNSClient();

  // Returns true if the DNS client started successfully, false otherwise.
  // If successful, the callback will be called with the result of the
  // request.  If Start() fails and returns false, the callback will not
  // be called, but the error that caused the failure will be returned in
  // |error|.
  virtual bool Start(const std::string& hostname, Error* error);

  // Aborts any running DNS client transaction.  This will cancel any callback
  // invocation.
  virtual void Stop();

  virtual bool IsActive() const;

  std::string interface_name() { return interface_name_; }

 private:
  friend class DNSClientTest;

  void HandleCompletion();
  void HandleDNSRead(int fd);
  void HandleDNSWrite(int fd);
  void HandleTimeout();
  void ReceiveDNSReply(int status, struct hostent* hostent);
  static void ReceiveDNSReplyCB(void* arg, int status, int timeouts,
                                struct hostent* hostent);
  bool RefreshHandles();

  static const int kDefaultDNSPort;

  Error error_;
  IPAddress address_;
  std::string interface_name_;
  std::vector<std::string> dns_servers_;
  EventDispatcher* dispatcher_;
  ClientCallback callback_;
  int timeout_ms_;
  bool running_;
  std::unique_ptr<DNSClientState> resolver_state_;
  base::CancelableClosure timeout_closure_;
  base::WeakPtrFactory<DNSClient> weak_ptr_factory_;
  Ares* ares_;
  Time* time_;

  DISALLOW_COPY_AND_ASSIGN(DNSClient);
};

}  // namespace shill

#endif  // SHILL_DNS_CLIENT_H_
