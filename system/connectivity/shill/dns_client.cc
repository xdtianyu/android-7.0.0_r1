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

#include "shill/dns_client.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/bind_helpers.h>
#include <base/stl_util.h>
#include <base/strings/string_number_conversions.h>

#include "shill/logging.h"
#include "shill/net/shill_time.h"
#include "shill/shill_ares.h"

using base::Bind;
using base::Unretained;
using std::map;
using std::set;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDNS;
static string ObjectID(DNSClient* d) { return d->interface_name(); }
}

const char DNSClient::kErrorNoData[] = "The query response contains no answers";
const char DNSClient::kErrorFormErr[] = "The server says the query is bad";
const char DNSClient::kErrorServerFail[] = "The server says it had a failure";
const char DNSClient::kErrorNotFound[] = "The queried-for domain was not found";
const char DNSClient::kErrorNotImp[] = "The server doesn't implement operation";
const char DNSClient::kErrorRefused[] = "The server replied, refused the query";
const char DNSClient::kErrorBadQuery[] = "Locally we could not format a query";
const char DNSClient::kErrorNetRefused[] = "The network connection was refused";
const char DNSClient::kErrorTimedOut[] = "The network connection was timed out";
const char DNSClient::kErrorUnknown[] = "DNS Resolver unknown internal error";

const int DNSClient::kDefaultDNSPort = 53;

// Private to the implementation of resolver so callers don't include ares.h
struct DNSClientState {
  DNSClientState() : channel(nullptr), start_time{} {}

  ares_channel channel;
  map<ares_socket_t, std::shared_ptr<IOHandler>> read_handlers;
  map<ares_socket_t, std::shared_ptr<IOHandler>> write_handlers;
  struct timeval start_time;
};

DNSClient::DNSClient(IPAddress::Family family,
                     const string& interface_name,
                     const vector<string>& dns_servers,
                     int timeout_ms,
                     EventDispatcher* dispatcher,
                     const ClientCallback& callback)
    : address_(IPAddress(family)),
      interface_name_(interface_name),
      dns_servers_(dns_servers),
      dispatcher_(dispatcher),
      callback_(callback),
      timeout_ms_(timeout_ms),
      running_(false),
      weak_ptr_factory_(this),
      ares_(Ares::GetInstance()),
      time_(Time::GetInstance()) {}

DNSClient::~DNSClient() {
  Stop();
}

bool DNSClient::Start(const string& hostname, Error* error) {
  if (running_) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kInProgress,
                          "Only one DNS request is allowed at a time");
    return false;
  }

  if (!resolver_state_.get()) {
    struct ares_options options;
    memset(&options, 0, sizeof(options));
    options.timeout = timeout_ms_;

    if (dns_servers_.empty()) {
      Error::PopulateAndLog(FROM_HERE, error, Error::kInvalidArguments,
                            "No valid DNS server addresses");
      return false;
    }

    resolver_state_.reset(new DNSClientState);
    int status = ares_->InitOptions(&resolver_state_->channel,
                                   &options,
                                   ARES_OPT_TIMEOUTMS);
    if (status != ARES_SUCCESS) {
      Error::PopulateAndLog(FROM_HERE, error, Error::kOperationFailed,
                            "ARES initialization returns error code: " +
                            base::IntToString(status));
      resolver_state_.reset();
      return false;
    }

    // Format DNS server addresses string as "host:port[,host:port...]" to be
    // used in call to ares_set_servers_csv for setting DNS server addresses.
    // There is a bug in ares library when parsing IPv6 addresses, where it
    // always assumes the port number are specified when address contains ":".
    // So when IPv6 address are given without port number as "xx:xx:xx::yy",the
    // parser would parse the address as "xx:xx:xx:" and port number as "yy".
    // To work around this bug, port number are added to each address.
    //
    // Alternatively, we can use ares_set_servers instead, where we would
    // explicitly construct a link list of ares_addr_node.
    string server_addresses;
    bool first = true;
    for (const auto& ip : dns_servers_) {
      if (!first) {
        server_addresses += ",";
      } else {
        first = false;
      }
      server_addresses += (ip + ":" + base::IntToString(kDefaultDNSPort));
    }
    status = ares_->SetServersCsv(resolver_state_->channel,
                                  server_addresses.c_str());
    if (status != ARES_SUCCESS) {
      Error::PopulateAndLog(FROM_HERE, error, Error::kOperationFailed,
                            "ARES set DNS servers error code: " +
                            base::IntToString(status));
      resolver_state_.reset();
      return false;
    }

    ares_->SetLocalDev(resolver_state_->channel, interface_name_.c_str());
  }

  running_ = true;
  time_->GetTimeMonotonic(&resolver_state_->start_time);
  ares_->GetHostByName(resolver_state_->channel, hostname.c_str(),
                       address_.family(), ReceiveDNSReplyCB, this);

  if (!RefreshHandles()) {
    LOG(ERROR) << "Impossibly short timeout.";
    error->CopyFrom(error_);
    Stop();
    return false;
  }

  return true;
}

void DNSClient::Stop() {
  SLOG(this, 3) << "In " << __func__;
  if (!resolver_state_.get()) {
    return;
  }

  running_ = false;
  weak_ptr_factory_.InvalidateWeakPtrs();
  error_.Reset();
  address_.SetAddressToDefault();
  ares_->Destroy(resolver_state_->channel);
  resolver_state_.reset();
}

bool DNSClient::IsActive() const {
  return running_;
}

// We delay our call to completion so that we exit all IOHandlers, and
// can clean up all of our local state before calling the callback, or
// during the process of the execution of the callee (which is free to
// call our destructor safely).
void DNSClient::HandleCompletion() {
  SLOG(this, 3) << "In " << __func__;
  Error error;
  error.CopyFrom(error_);
  IPAddress address(address_);
  if (!error.IsSuccess()) {
    // If the DNS request did not succeed, do not trust it for future
    // attempts.
    Stop();
  } else {
    // Prepare our state for the next request without destroying the
    // current ARES state.
    error_.Reset();
    address_.SetAddressToDefault();
  }
  callback_.Run(error, address);
}

void DNSClient::HandleDNSRead(int fd) {
  ares_->ProcessFd(resolver_state_->channel, fd, ARES_SOCKET_BAD);
  RefreshHandles();
}

void DNSClient::HandleDNSWrite(int fd) {
  ares_->ProcessFd(resolver_state_->channel, ARES_SOCKET_BAD, fd);
  RefreshHandles();
}

void DNSClient::HandleTimeout() {
  ares_->ProcessFd(resolver_state_->channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
  RefreshHandles();
}

void DNSClient::ReceiveDNSReply(int status, struct hostent* hostent) {
  if (!running_) {
    // We can be called during ARES shutdown -- ignore these events.
    return;
  }
  SLOG(this, 3) << "In " << __func__;
  running_ = false;
  timeout_closure_.Cancel();
  dispatcher_->PostTask(Bind(&DNSClient::HandleCompletion,
                             weak_ptr_factory_.GetWeakPtr()));

  if (status == ARES_SUCCESS &&
      hostent != nullptr &&
      hostent->h_addrtype == address_.family() &&
      static_cast<size_t>(hostent->h_length) ==
      IPAddress::GetAddressLength(address_.family()) &&
      hostent->h_addr_list != nullptr &&
      hostent->h_addr_list[0] != nullptr) {
    address_ = IPAddress(address_.family(),
                         ByteString(reinterpret_cast<unsigned char*>(
                             hostent->h_addr_list[0]), hostent->h_length));
  } else {
    switch (status) {
      case ARES_ENODATA:
        error_.Populate(Error::kOperationFailed, kErrorNoData);
        break;
      case ARES_EFORMERR:
        error_.Populate(Error::kOperationFailed, kErrorFormErr);
        break;
      case ARES_ESERVFAIL:
        error_.Populate(Error::kOperationFailed, kErrorServerFail);
        break;
      case ARES_ENOTFOUND:
        error_.Populate(Error::kOperationFailed, kErrorNotFound);
        break;
      case ARES_ENOTIMP:
        error_.Populate(Error::kOperationFailed, kErrorNotImp);
        break;
      case ARES_EREFUSED:
        error_.Populate(Error::kOperationFailed, kErrorRefused);
        break;
      case ARES_EBADQUERY:
      case ARES_EBADNAME:
      case ARES_EBADFAMILY:
      case ARES_EBADRESP:
        error_.Populate(Error::kOperationFailed, kErrorBadQuery);
        break;
      case ARES_ECONNREFUSED:
        error_.Populate(Error::kOperationFailed, kErrorNetRefused);
        break;
      case ARES_ETIMEOUT:
        error_.Populate(Error::kOperationTimeout, kErrorTimedOut);
        break;
      default:
        error_.Populate(Error::kOperationFailed, kErrorUnknown);
        if (status == ARES_SUCCESS) {
          LOG(ERROR) << "ARES returned success but hostent was invalid!";
        } else {
          LOG(ERROR) << "ARES returned unhandled error status " << status;
        }
        break;
    }
  }
}

void DNSClient::ReceiveDNSReplyCB(void* arg, int status,
                                  int /*timeouts*/,
                                  struct hostent* hostent) {
  DNSClient* res = static_cast<DNSClient*>(arg);
  res->ReceiveDNSReply(status, hostent);
}

bool DNSClient::RefreshHandles() {
  map<ares_socket_t, std::shared_ptr<IOHandler>> old_read =
      resolver_state_->read_handlers;
  map<ares_socket_t, std::shared_ptr<IOHandler>> old_write =
      resolver_state_->write_handlers;

  resolver_state_->read_handlers.clear();
  resolver_state_->write_handlers.clear();

  ares_socket_t sockets[ARES_GETSOCK_MAXNUM];
  int action_bits = ares_->GetSock(resolver_state_->channel, sockets,
                                   ARES_GETSOCK_MAXNUM);

  base::Callback<void(int)> read_callback(
      Bind(&DNSClient::HandleDNSRead, weak_ptr_factory_.GetWeakPtr()));
  base::Callback<void(int)> write_callback(
      Bind(&DNSClient::HandleDNSWrite, weak_ptr_factory_.GetWeakPtr()));
  for (int i = 0; i < ARES_GETSOCK_MAXNUM; i++) {
    if (ARES_GETSOCK_READABLE(action_bits, i)) {
      if (ContainsKey(old_read, sockets[i])) {
        resolver_state_->read_handlers[sockets[i]] = old_read[sockets[i]];
      } else {
        resolver_state_->read_handlers[sockets[i]] =
            std::shared_ptr<IOHandler> (
                dispatcher_->CreateReadyHandler(sockets[i],
                                                IOHandler::kModeInput,
                                                read_callback));
      }
    }
    if (ARES_GETSOCK_WRITABLE(action_bits, i)) {
      if (ContainsKey(old_write, sockets[i])) {
        resolver_state_->write_handlers[sockets[i]] = old_write[sockets[i]];
      } else {
        resolver_state_->write_handlers[sockets[i]] =
            std::shared_ptr<IOHandler> (
                dispatcher_->CreateReadyHandler(sockets[i],
                                                IOHandler::kModeOutput,
                                                write_callback));
      }
    }
  }

  if (!running_) {
    // We are here just to clean up socket handles, and the ARES state was
    // cleaned up during the last call to ares_->ProcessFd().
    return false;
  }

  // Schedule timer event for the earlier of our timeout or one requested by
  // the resolver library.
  struct timeval now, elapsed_time, timeout_tv;
  time_->GetTimeMonotonic(&now);
  timersub(&now, &resolver_state_->start_time, &elapsed_time);
  timeout_tv.tv_sec = timeout_ms_ / 1000;
  timeout_tv.tv_usec = (timeout_ms_ % 1000) * 1000;
  timeout_closure_.Cancel();

  if (timercmp(&elapsed_time, &timeout_tv, >=)) {
    // There are 3 cases of interest:
    //  - If we got here from Start(), when we return, Stop() will be
    //    called, so our cleanup task will not run, so we will not have the
    //    side-effect of both invoking the callback and returning False
    //    in Start().
    //  - If we got here from the tail of an IO event, we can't call
    //    Stop() since that will blow away the IOHandler we are running
    //    in.  We will perform the cleanup in the posted task below.
    //  - If we got here from a timeout handler, we will perform cleanup
    //    in the posted task.
    running_ = false;
    error_.Populate(Error::kOperationTimeout, kErrorTimedOut);
    dispatcher_->PostTask(Bind(&DNSClient::HandleCompletion,
                               weak_ptr_factory_.GetWeakPtr()));
    return false;
  } else {
    struct timeval max, ret_tv;
    timersub(&timeout_tv, &elapsed_time, &max);
    struct timeval* tv = ares_->Timeout(resolver_state_->channel,
                                        &max, &ret_tv);
    timeout_closure_.Reset(
        Bind(&DNSClient::HandleTimeout, weak_ptr_factory_.GetWeakPtr()));
    dispatcher_->PostDelayedTask(timeout_closure_.callback(),
                                 tv->tv_sec * 1000 + tv->tv_usec / 1000);
  }

  return true;
}

}  // namespace shill
