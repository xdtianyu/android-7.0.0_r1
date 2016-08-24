// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_PRIVET_WIFI_BOOTSTRAP_MANAGER_H_
#define LIBWEAVE_SRC_PRIVET_WIFI_BOOTSTRAP_MANAGER_H_

#include <set>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <base/scoped_observer.h>
#include <base/time/time.h>

#include "src/privet/privet_types.h"
#include "src/privet/wifi_delegate.h"
#include "src/privet/wifi_ssid_generator.h"

namespace weave {

class Config;

namespace provider {
class Network;
class TaskRunner;
class Wifi;
}

namespace privet {

class CloudDelegate;
class DeviceDelegate;

class WifiBootstrapManager : public WifiDelegate {
 public:
  enum class State {
    kDisabled,
    kBootstrapping,
    kMonitoring,
    kConnecting,
  };

  WifiBootstrapManager(Config* config,
                       provider::TaskRunner* task_runner,
                       provider::Network* shill_client,
                       provider::Wifi* wifi,
                       CloudDelegate* gcd);
  ~WifiBootstrapManager() override = default;
  virtual void Init();

  // Overrides from WifiDelegate.
  const ConnectionState& GetConnectionState() const override;
  const SetupState& GetSetupState() const override;
  bool ConfigureCredentials(const std::string& ssid,
                            const std::string& passphrase,
                            ErrorPtr* error) override;
  std::string GetCurrentlyConnectedSsid() const override;
  std::string GetHostedSsid() const override;
  std::set<WifiType> GetTypes() const override;

 private:
  // These Start* tasks:
  //   1) Do state appropriate work for entering the indicated state.
  //   2) Update the state variable to reflect that we're in a new state
  //   3) Call StateListeners to notify that we've transitioned.
  // These End* tasks perform cleanup on leaving indicated state.
  void StartBootstrapping();
  void EndBootstrapping();

  void StartConnecting(const std::string& ssid, const std::string& passphrase);
  void EndConnecting();

  void StartMonitoring(const base::TimeDelta& timeout);
  void ContinueMonitoring(const base::TimeDelta& timeout);
  void EndMonitoring();

  // Update the current state, post tasks to notify listeners accordingly to
  // the MessageLoop.
  void UpdateState(State new_state);

  std::string GenerateSsid() const;

  // If we've been bootstrapped successfully before, and we're bootstrapping
  // again because we slipped offline for a sufficiently longtime, we want
  // to return to monitoring mode periodically in case our connectivity issues
  // were temporary.
  void OnBootstrapTimeout();
  void OnConnectDone(const std::string& ssid, ErrorPtr error);
  void OnConnectTimeout();
  void OnConnectivityChange();
  void OnMonitorTimeout();
  void UpdateConnectionState();

  State state_{State::kDisabled};
  // Setup state is the temporal state of the most recent bootstrapping attempt.
  // It is not persisted to disk.
  SetupState setup_state_{SetupState::kNone};
  ConnectionState connection_state_{ConnectionState::kDisabled};
  Config* config_{nullptr};
  provider::TaskRunner* task_runner_{nullptr};
  provider::Network* network_{nullptr};
  provider::Wifi* wifi_{nullptr};
  WifiSsidGenerator ssid_generator_;
  base::Time monitor_until_;

  bool currently_online_{false};
  std::string privet_ssid_;

  // Helps to reset irrelevant tasks switching state.
  base::WeakPtrFactory<WifiBootstrapManager> tasks_weak_factory_{this};

  base::WeakPtrFactory<WifiBootstrapManager> lifetime_weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(WifiBootstrapManager);
};

}  // namespace privet
}  // namespace weave

#endif  // LIBWEAVE_SRC_PRIVET_WIFI_BOOTSTRAP_MANAGER_H_
