// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/publisher.h"

#include <map>

#include <weave/error.h>
#include <weave/provider/dns_service_discovery.h>

#include "src/privet/cloud_delegate.h"
#include "src/privet/device_delegate.h"
#include "src/privet/device_ui_kind.h"
#include "src/privet/wifi_bootstrap_manager.h"
#include "src/privet/wifi_ssid_generator.h"
#include "src/string_utils.h"

namespace weave {
namespace privet {

namespace {

// The service type we'll expose via DNS-SD.
const char kPrivetServiceType[] = "_privet._tcp";

}  // namespace

Publisher::Publisher(const DeviceDelegate* device,
                     const CloudDelegate* cloud,
                     const WifiDelegate* wifi,
                     provider::DnsServiceDiscovery* dns_sd)
    : dns_sd_{dns_sd}, device_{device}, cloud_{cloud}, wifi_{wifi} {
  CHECK(device_);
  CHECK(cloud_);
  CHECK(dns_sd_);
  Update();
}

Publisher::~Publisher() {
  RemoveService();
}

void Publisher::Update() {
  if (device_->GetHttpEnpoint().first == 0)
    return RemoveService();
  ExposeService();
}

void Publisher::ExposeService() {
  std::string name{cloud_->GetName()};
  std::string model_id{cloud_->GetModelId()};
  DCHECK_EQ(model_id.size(), 5U);

  VLOG(2) << "DNS-SD update requested";
  const uint16_t port = device_->GetHttpEnpoint().first;
  DCHECK_NE(port, 0);

  std::vector<std::string> txt_record{
      {"txtvers=3"},
      {"ty=" + name},
      {"services=" + GetDeviceUiKind(model_id)},
      {"id=" + cloud_->GetDeviceId()},
      {"mmid=" + model_id},
      {"flags=" + WifiSsidGenerator{cloud_, wifi_}.GenerateFlags()},
  };

  if (!cloud_->GetCloudId().empty())
    txt_record.emplace_back("gcd_id=" + cloud_->GetCloudId());

  if (!cloud_->GetDescription().empty())
    txt_record.emplace_back("note=" + cloud_->GetDescription());

  auto new_data = std::make_pair(port, txt_record);
  if (published_ == new_data)
    return;

  VLOG(1) << "Updating service using DNS-SD, port: " << port;
  published_ = new_data;
  dns_sd_->PublishService(kPrivetServiceType, port, txt_record);
}

void Publisher::RemoveService() {
  if (!published_.first)
    return;
  published_ = {};
  VLOG(1) << "Stopping service publishing";
  dns_sd_->StopPublishing(kPrivetServiceType);
}

}  // namespace privet
}  // namespace weave
