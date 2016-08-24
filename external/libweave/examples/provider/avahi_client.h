// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_PROVIDER_AVAHI_CLIENT_H_
#define LIBWEAVE_EXAMPLES_PROVIDER_AVAHI_CLIENT_H_

#include <map>
#include <string>

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/thread-watch.h>

#include <weave/provider/dns_service_discovery.h>

namespace weave {
namespace examples {

// Example of provider::DnsServiceDiscovery implemented with avahi.
class AvahiClient : public provider::DnsServiceDiscovery {
 public:
  AvahiClient();

  ~AvahiClient() override;
  void PublishService(const std::string& service_type,
                      uint16_t port,
                      const std::vector<std::string>& txt) override;
  void StopPublishing(const std::string& service_name) override;

  uint16_t prev_port_{0};
  std::string prev_type_;

  std::unique_ptr<AvahiThreadedPoll, decltype(&avahi_threaded_poll_free)>
      thread_pool_{nullptr, &avahi_threaded_poll_free};

  std::unique_ptr< ::AvahiClient, decltype(&avahi_client_free)> client_{
      nullptr, &avahi_client_free};

  std::unique_ptr<AvahiEntryGroup, decltype(&avahi_entry_group_free)> group_{
      nullptr, &avahi_entry_group_free};
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_PROVIDER_AVAHI_CLIENT_H_
