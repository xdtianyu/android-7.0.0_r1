/*
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BUFFET_AVAHI_MDNS_CLIENT_H_
#define BUFFET_AVAHI_MDNS_CLIENT_H_

#include <map>
#include <string>

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/thread-watch.h>

#include "buffet/mdns_client.h"

namespace buffet {

// Publishes privet service on mDns using Avahi.
class AvahiMdnsClient : public MdnsClient {
 public:
  explicit AvahiMdnsClient();
  ~AvahiMdnsClient() override;

  // weave::provider::DnsServiceDiscovery implementation.
  void PublishService(const std::string& service_type, uint16_t port,
                      const std::vector<std::string>& txt) override;
  void StopPublishing(const std::string& service_type) override;

 private:
  static void OnAvahiClientStateUpdate(AvahiClient* s,
                                       AvahiClientState state,
                                       void* userdata);
  void RepublishService();

  uint16_t prev_port_{0};
  std::string prev_service_type_;
  std::string service_name_;
  std::vector<std::string> txt_records_;
  std::unique_ptr<AvahiThreadedPoll, decltype(&avahi_threaded_poll_free)>
      thread_pool_{nullptr, &avahi_threaded_poll_free};
  std::unique_ptr< ::AvahiClient, decltype(&avahi_client_free)> client_{
      nullptr, &avahi_client_free};
  std::unique_ptr<AvahiEntryGroup, decltype(&avahi_entry_group_free)> group_{
      nullptr, &avahi_entry_group_free};

  DISALLOW_COPY_AND_ASSIGN(AvahiMdnsClient);
};

}  // namespace buffet

#endif  // BUFFET_AVAHI_MDNS_CLIENT_H_
