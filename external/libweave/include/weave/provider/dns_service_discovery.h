// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_DNS_SERVICE_DISCOVERY_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_DNS_SERVICE_DISCOVERY_H_

#include <string>
#include <vector>

#include <base/callback.h>

namespace weave {
namespace provider {

// This interface should be implemented by the user of libweave and
// provided during device creation in Device::Create(...)
// libweave will use this interface to start/stop mDNS service discovery.
//
// Implementation of the PublishService(...) method should publish mDNS
// service. Publishing service should be done according to RFC 6762 (mDNS)
// and RFC 6763 (DNS Service discovery).
//
// service_type will contain name of the service before .local.
//   For example, "_privet._tcp".
// port will have a port number where Weave HTTP server is running.
//   For example, 80.
// txt will contain a list of strings for mDNS TXT records.
//   For example, "txtver=3", "name=MyDevice"
//
// The following mDNS records should be published in the above examples:
// _privet._tcp.local PTR <service_name>._privet._tcp.local
// <service_name>._privet._tcp.local SRV <local_domain> 80
// <service_name>._privet._tcp.local TXT "txtver=3" "name=MyDevice"
// <local_domain> A <IPv4 address>
// <local_domain> AAAA <IPv6 address>
//
// In the list above, it is implementer's responsibility to choose
// <service_name> and <local_domain>.
// If device only supports IPv4 or IPv6, then only the corresponding mDNS
// records should be published. <IPv4 address> and <IPv6 address> should
// be the addresses of the network interface this record is advertised on.
//
// Implementation of PublishService(...) may use existing libraries or
// services to implement mDNS service discovery. For example, Avahi or
// Bonjour. Such implementation may require IPC or similar async
// communication mechanisms. PublishService(...) implementation may
// just start the process and return quickly (non-blocking) while the
// full mDNS implementation is started in the background. In such case
// PublishService(...) implementation should remember all input parameters
// so it can restart service publishing in case of failures.
// From libweave perspective, discovery is started after
// PublishService(...) returns and libweave may not call this method again.
//
// Implementation of the StopPublishing(...) method should stop advertising
// specified service type on the mDNS. This should be done according to mDNS
// (RFC 6762) and DNS-SD (RFC 6763) specifications, which require announcing
// DNS records that will be going away with TTL=1.
//
// Since this interface allows multiple service types to be published, proper
// implementation should maintain list of service types and stop advertising
// only the type specified in this request. Other service types, as well as
// records necessary for other services, like A, AAAA may still be available
// over mDNS.
//
// In case a device has multiple networking interfaces, the device developer
// needs to make a decision where mDNS advertising is necessary and where it is
// not. For example, there should be no mDNS advertising on cellular (LTE) or
// WAN (for routers) network interfaces. In some cases, there might be more
// then one network interface where advertising makes sense. For example,
// a device may have both WiFi and Ethernet connections. In such case,
// PublishService(...) should make service available on both interface.
//
// From libweave perspective, it always looks like there is only one network
// interface (for both service discovery and web server). It is
// the job of this interface implementation to hide network complexity from
// the libweave and to bring webserver up on the same port on both interfaces,
// as well as publish an mDNS service (uses webserver port).
//
// See libweave/examples/provider/avahi_client.cc for complete example
// using Avahi for DNS service discovery.

class DnsServiceDiscovery {
 public:
  // Publishes new service using DNS-SD or updates existing one.
  virtual void PublishService(const std::string& service_type,
                              uint16_t port,
                              const std::vector<std::string>& txt) = 0;

  // Stops publishing service.
  virtual void StopPublishing(const std::string& service_type) = 0;

 protected:
  virtual ~DnsServiceDiscovery() {}
};

}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_DNS_SERVICE_DISCOVERY_H_
