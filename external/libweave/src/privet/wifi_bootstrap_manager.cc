// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/privet/wifi_bootstrap_manager.h"

#include <base/logging.h>
#include <base/memory/weak_ptr.h>
#include <weave/enum_to_string.h>
#include <weave/provider/network.h>
#include <weave/provider/task_runner.h>
#include <weave/provider/wifi.h>

#include "src/bind_lambda.h"
#include "src/config.h"
#include "src/privet/constants.h"

namespace weave {
namespace privet {

namespace {

const int kMonitoringWithSsidTimeoutSeconds = 15;
const int kMonitoringTimeoutSeconds = 120;
const int kBootstrapTimeoutSeconds = 600;
const int kConnectingTimeoutSeconds = 180;

const EnumToStringMap<WifiBootstrapManager::State>::Map kWifiSetupStateMap[] = {
    {WifiBootstrapManager::State::kDisabled, "disabled"},
    {WifiBootstrapManager::State::kBootstrapping, "waiting"},
    {WifiBootstrapManager::State::kMonitoring, "monitoring"},
    {WifiBootstrapManager::State::kConnecting, "connecting"},
};
}

using provider::Network;

WifiBootstrapManager::WifiBootstrapManager(Config* config,
                                           provider::TaskRunner* task_runner,
                                           provider::Network* network,
                                           provider::Wifi* wifi,
                                           CloudDelegate* gcd)
    : config_{config},
      task_runner_{task_runner},
      network_{network},
      wifi_{wifi},
      ssid_generator_{gcd, this} {
  CHECK(config_);
  CHECK(network_);
  CHECK(task_runner_);
  CHECK(wifi_);
}

void WifiBootstrapManager::Init() {
  UpdateConnectionState();
  network_->AddConnectionChangedCallback(
      base::Bind(&WifiBootstrapManager::OnConnectivityChange,
                 lifetime_weak_factory_.GetWeakPtr()));
  if (config_->GetSettings().last_configured_ssid.empty()) {
    // Give implementation some time to figure out state.
    StartMonitoring(
        base::TimeDelta::FromSeconds(kMonitoringWithSsidTimeoutSeconds));
  } else {
    StartMonitoring(base::TimeDelta::FromSeconds(kMonitoringTimeoutSeconds));
  }
}

void WifiBootstrapManager::StartBootstrapping() {
  if (network_->GetConnectionState() == Network::State::kOnline) {
    // If one of the devices we monitor for connectivity is online, we need not
    // start an AP.  For most devices, this is a situation which happens in
    // testing when we have an ethernet connection.  If you need to always
    // start an AP to bootstrap WiFi credentials, then add your WiFi interface
    // to the device whitelist.
    StartMonitoring(base::TimeDelta::FromSeconds(kMonitoringTimeoutSeconds));
    return;
  }

  UpdateState(State::kBootstrapping);
  if (!config_->GetSettings().last_configured_ssid.empty()) {
    // If we have been configured before, we'd like to periodically take down
    // our AP and find out if we can connect again.  Many kinds of failures are
    // transient, and having an AP up prohibits us from connecting as a client.
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(&WifiBootstrapManager::OnBootstrapTimeout,
                              tasks_weak_factory_.GetWeakPtr()),
        base::TimeDelta::FromSeconds(kBootstrapTimeoutSeconds));
  }
  // TODO(vitalybuka): Add SSID probing.
  privet_ssid_ = GenerateSsid();
  CHECK(!privet_ssid_.empty());

  VLOG(1) << "Starting AP with SSID: " << privet_ssid_;
  wifi_->StartAccessPoint(privet_ssid_);
}

void WifiBootstrapManager::EndBootstrapping() {
  VLOG(1) << "Stopping AP";
  wifi_->StopAccessPoint();
  privet_ssid_.clear();
}

void WifiBootstrapManager::StartConnecting(const std::string& ssid,
                                           const std::string& passphrase) {
  VLOG(1) << "Attempting connect to SSID:" << ssid;
  UpdateState(State::kConnecting);
  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&WifiBootstrapManager::OnConnectTimeout,
                            tasks_weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(kConnectingTimeoutSeconds));
  wifi_->Connect(ssid, passphrase,
                 base::Bind(&WifiBootstrapManager::OnConnectDone,
                            tasks_weak_factory_.GetWeakPtr(), ssid));
}

void WifiBootstrapManager::EndConnecting() {}

void WifiBootstrapManager::StartMonitoring(const base::TimeDelta& timeout) {
  monitor_until_ = {};
  ContinueMonitoring(timeout);
}

void WifiBootstrapManager::ContinueMonitoring(const base::TimeDelta& timeout) {
  VLOG(1) << "Monitoring connectivity.";
  // We already have a callback in place with |network_| to update our
  // connectivity state.  See OnConnectivityChange().
  UpdateState(State::kMonitoring);

  if (network_->GetConnectionState() == Network::State::kOnline) {
    monitor_until_ = {};
  } else {
    if (monitor_until_.is_null()) {
      monitor_until_ = base::Time::Now() + timeout;
      VLOG(2) << "Waiting for connection until: " << monitor_until_;
    }

    // Schedule timeout timer taking into account already offline time.
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(&WifiBootstrapManager::OnMonitorTimeout,
                              tasks_weak_factory_.GetWeakPtr()),
        monitor_until_ - base::Time::Now());
  }
}

void WifiBootstrapManager::EndMonitoring() {}

void WifiBootstrapManager::UpdateState(State new_state) {
  VLOG(3) << "Switching state from " << EnumToString(state_) << " to "
          << EnumToString(new_state);
  // Abort irrelevant tasks.
  tasks_weak_factory_.InvalidateWeakPtrs();

  switch (state_) {
    case State::kDisabled:
      break;
    case State::kBootstrapping:
      EndBootstrapping();
      break;
    case State::kMonitoring:
      EndMonitoring();
      break;
    case State::kConnecting:
      EndConnecting();
      break;
  }

  state_ = new_state;
}

std::string WifiBootstrapManager::GenerateSsid() const {
  const std::string& ssid = config_->GetSettings().test_privet_ssid;
  return ssid.empty() ? ssid_generator_.GenerateSsid() : ssid;
}

const ConnectionState& WifiBootstrapManager::GetConnectionState() const {
  return connection_state_;
}

const SetupState& WifiBootstrapManager::GetSetupState() const {
  return setup_state_;
}

bool WifiBootstrapManager::ConfigureCredentials(const std::string& ssid,
                                                const std::string& passphrase,
                                                ErrorPtr* error) {
  setup_state_ = SetupState{SetupState::kInProgress};
  // Since we are changing network, we need to let the web server send out the
  // response to the HTTP request leading to this action. So, we are waiting
  // a bit before mocking with network set up.
  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&WifiBootstrapManager::StartConnecting,
                            tasks_weak_factory_.GetWeakPtr(), ssid, passphrase),
      base::TimeDelta::FromSeconds(1));
  return true;
}

std::string WifiBootstrapManager::GetCurrentlyConnectedSsid() const {
  // TODO(vitalybuka): Get from shill, if possible.
  return config_->GetSettings().last_configured_ssid;
}

std::string WifiBootstrapManager::GetHostedSsid() const {
  return privet_ssid_;
}

std::set<WifiType> WifiBootstrapManager::GetTypes() const {
  std::set<WifiType> result;
  if (wifi_->IsWifi24Supported())
    result.insert(WifiType::kWifi24);
  if (wifi_->IsWifi50Supported())
    result.insert(WifiType::kWifi50);
  return result;
}

void WifiBootstrapManager::OnConnectDone(const std::string& ssid,
                                         ErrorPtr error) {
  if (error) {
    Error::AddTo(&error, FROM_HERE, errors::kInvalidState,
                 "Failed to connect to provided network");
    setup_state_ = SetupState{std::move(error)};
    return StartBootstrapping();
  }
  VLOG(1) << "Wifi was connected successfully";
  Config::Transaction change{config_};
  change.set_last_configured_ssid(ssid);
  change.Commit();
  setup_state_ = SetupState{SetupState::kSuccess};
  StartMonitoring(base::TimeDelta::FromSeconds(kMonitoringTimeoutSeconds));
}

void WifiBootstrapManager::OnConnectTimeout() {
  ErrorPtr error;
  Error::AddTo(&error, FROM_HERE, errors::kInvalidState,
               "Timeout connecting to provided network");
  setup_state_ = SetupState{std::move(error)};
  return StartBootstrapping();
}

void WifiBootstrapManager::OnBootstrapTimeout() {
  VLOG(1) << "Bootstrapping has timed out.";
  StartMonitoring(base::TimeDelta::FromSeconds(kMonitoringTimeoutSeconds));
}

void WifiBootstrapManager::OnConnectivityChange() {
  UpdateConnectionState();

  if (state_ == State::kMonitoring ||
      (state_ != State::kDisabled &&
       network_->GetConnectionState() == Network::State::kOnline)) {
    ContinueMonitoring(base::TimeDelta::FromSeconds(kMonitoringTimeoutSeconds));
  }
}

void WifiBootstrapManager::OnMonitorTimeout() {
  VLOG(1) << "Spent too long offline. Entering bootstrap mode.";
  // TODO(wiley) Retrieve relevant errors from shill.
  StartBootstrapping();
}

void WifiBootstrapManager::UpdateConnectionState() {
  connection_state_ = ConnectionState{ConnectionState::kUnconfigured};
  Network::State service_state{network_->GetConnectionState()};
  VLOG(3) << "New network state: " << EnumToString(service_state);

  // TODO: Make it true wifi state, currently it's rather online state.
  if (service_state != Network::State::kOnline &&
      config_->GetSettings().last_configured_ssid.empty()) {
    return;
  }

  switch (service_state) {
    case Network::State::kOffline:
      connection_state_ = ConnectionState{ConnectionState::kOffline};
      return;
    case Network::State::kError: {
      // TODO(wiley) Pull error information from somewhere.
      ErrorPtr error;
      Error::AddTo(&error, FROM_HERE, errors::kInvalidState,
                   "Unknown WiFi error");
      connection_state_ = ConnectionState{std::move(error)};
      return;
    }
    case Network::State::kConnecting:
      connection_state_ = ConnectionState{ConnectionState::kConnecting};
      return;
    case Network::State::kOnline:
      connection_state_ = ConnectionState{ConnectionState::kOnline};
      return;
  }
  ErrorPtr error;
  Error::AddToPrintf(&error, FROM_HERE, errors::kInvalidState,
                     "Unknown network state: %s",
                     EnumToString(service_state).c_str());
  connection_state_ = ConnectionState{std::move(error)};
}

}  // namespace privet

template <>
LIBWEAVE_EXPORT
EnumToStringMap<privet::WifiBootstrapManager::State>::EnumToStringMap()
    : EnumToStringMap(privet::kWifiSetupStateMap) {}

}  // namespace weave
