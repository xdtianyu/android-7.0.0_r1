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

#include "update_engine/chrome_browser_proxy_resolver.h"

#include <deque>
#include <map>
#include <string>
#include <utility>

#include <base/bind.h>
#include <base/strings/string_tokenizer.h>
#include <base/strings/string_util.h>

#include "update_engine/common/utils.h"

namespace chromeos_update_engine {

using base::StringTokenizer;
using base::TimeDelta;
using brillo::MessageLoop;
using std::deque;
using std::make_pair;
using std::pair;
using std::string;

const char kLibCrosServiceName[] = "org.chromium.LibCrosService";
const char kLibCrosProxyResolveName[] = "ProxyResolved";
const char kLibCrosProxyResolveSignalInterface[] =
    "org.chromium.UpdateEngineLibcrosProxyResolvedInterface";

namespace {

const int kTimeout = 5;  // seconds

}  // namespace

ChromeBrowserProxyResolver::ChromeBrowserProxyResolver(
    LibCrosProxy* libcros_proxy)
    : libcros_proxy_(libcros_proxy), timeout_(kTimeout) {}

bool ChromeBrowserProxyResolver::Init() {
  libcros_proxy_->ue_proxy_resolved_interface()
      ->RegisterProxyResolvedSignalHandler(
          base::Bind(&ChromeBrowserProxyResolver::OnProxyResolvedSignal,
                     base::Unretained(this)),
          base::Bind(&ChromeBrowserProxyResolver::OnSignalConnected,
                     base::Unretained(this)));
  return true;
}

ChromeBrowserProxyResolver::~ChromeBrowserProxyResolver() {
  // Kill outstanding timers.
  for (auto& timer : timers_) {
    MessageLoop::current()->CancelTask(timer.second);
    timer.second = MessageLoop::kTaskIdNull;
  }
}

bool ChromeBrowserProxyResolver::GetProxiesForUrl(const string& url,
                                                  ProxiesResolvedFn callback,
                                                  void* data) {
  int timeout = timeout_;
  brillo::ErrorPtr error;
  if (!libcros_proxy_->service_interface_proxy()->ResolveNetworkProxy(
          url.c_str(),
          kLibCrosProxyResolveSignalInterface,
          kLibCrosProxyResolveName,
          &error)) {
    LOG(WARNING) << "Can't resolve the proxy. Continuing with no proxy.";
    timeout = 0;
  }

  callbacks_.insert(make_pair(url, make_pair(callback, data)));
  MessageLoop::TaskId timer = MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&ChromeBrowserProxyResolver::HandleTimeout,
                 base::Unretained(this),
                 url),
      TimeDelta::FromSeconds(timeout));
  timers_.insert(make_pair(url, timer));
  return true;
}

bool ChromeBrowserProxyResolver::DeleteUrlState(
    const string& source_url,
    bool delete_timer,
    pair<ProxiesResolvedFn, void*>* callback) {
  {
    CallbacksMap::iterator it = callbacks_.lower_bound(source_url);
    TEST_AND_RETURN_FALSE(it != callbacks_.end());
    TEST_AND_RETURN_FALSE(it->first == source_url);
    if (callback)
      *callback = it->second;
    callbacks_.erase(it);
  }
  {
    TimeoutsMap::iterator it = timers_.lower_bound(source_url);
    TEST_AND_RETURN_FALSE(it != timers_.end());
    TEST_AND_RETURN_FALSE(it->first == source_url);
    if (delete_timer)
      MessageLoop::current()->CancelTask(it->second);
    timers_.erase(it);
  }
  return true;
}

void ChromeBrowserProxyResolver::OnSignalConnected(const string& interface_name,
                                                   const string& signal_name,
                                                   bool successful) {
  if (!successful) {
    LOG(ERROR) << "Couldn't connect to the signal " << interface_name << "."
               << signal_name;
  }
}

void ChromeBrowserProxyResolver::OnProxyResolvedSignal(
    const string& source_url,
    const string& proxy_info,
    const string& error_message) {
  pair<ProxiesResolvedFn, void*> callback;
  TEST_AND_RETURN(DeleteUrlState(source_url, true, &callback));
  if (!error_message.empty()) {
    LOG(WARNING) << "ProxyResolved error: " << error_message;
  }
  (*callback.first)(ParseProxyString(proxy_info), callback.second);
}

void ChromeBrowserProxyResolver::HandleTimeout(string source_url) {
  LOG(INFO) << "Timeout handler called. Seems Chrome isn't responding.";
  pair<ProxiesResolvedFn, void*> callback;
  TEST_AND_RETURN(DeleteUrlState(source_url, false, &callback));
  deque<string> proxies;
  proxies.push_back(kNoProxy);
  (*callback.first)(proxies, callback.second);
}

deque<string> ChromeBrowserProxyResolver::ParseProxyString(
    const string& input) {
  deque<string> ret;
  // Some of this code taken from
  // http://src.chromium.org/svn/trunk/src/net/proxy/proxy_server.cc and
  // http://src.chromium.org/svn/trunk/src/net/proxy/proxy_list.cc
  StringTokenizer entry_tok(input, ";");
  while (entry_tok.GetNext()) {
    string token = entry_tok.token();
    base::TrimWhitespaceASCII(token, base::TRIM_ALL, &token);

    // Start by finding the first space (if any).
    string::iterator space;
    for (space = token.begin(); space != token.end(); ++space) {
      if (base::IsAsciiWhitespace(*space)) {
        break;
      }
    }

    string scheme = base::ToLowerASCII(string(token.begin(), space));
    // Chrome uses "socks" to mean socks4 and "proxy" to mean http.
    if (scheme == "socks")
      scheme += "4";
    else if (scheme == "proxy")
      scheme = "http";
    else if (scheme != "https" &&
             scheme != "socks4" &&
             scheme != "socks5" &&
             scheme != "direct")
      continue;  // Invalid proxy scheme

    string host_and_port = string(space, token.end());
    base::TrimWhitespaceASCII(host_and_port, base::TRIM_ALL, &host_and_port);
    if (scheme != "direct" && host_and_port.empty())
      continue;  // Must supply host/port when non-direct proxy used.
    ret.push_back(scheme + "://" + host_and_port);
  }
  if (ret.empty() || *ret.rbegin() != kNoProxy)
    ret.push_back(kNoProxy);
  return ret;
}

}  // namespace chromeos_update_engine
