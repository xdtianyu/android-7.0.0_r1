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

#ifndef UPDATE_ENGINE_OMAHA_REQUEST_PARAMS_H_
#define UPDATE_ENGINE_OMAHA_REQUEST_PARAMS_H_

#include <stdint.h>

#include <string>

#include <base/macros.h>
#include <base/time/time.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "update_engine/common/platform_constants.h"
#include "update_engine/image_properties.h"

// This gathers local system information and prepares info used by the
// Omaha request action.

namespace chromeos_update_engine {

class SystemState;

// This class encapsulates the data Omaha gets for the request, along with
// essential state needed for the processing of the request/response.  The
// strings in this struct should not be XML escaped.
//
// TODO(jaysri): chromium-os:39752 tracks the need to rename this class to
// reflect its lifetime more appropriately.
class OmahaRequestParams {
 public:
  explicit OmahaRequestParams(SystemState* system_state)
      : system_state_(system_state),
        os_platform_(constants::kOmahaPlatformName),
        os_version_(kOsVersion),
        delta_okay_(true),
        interactive_(false),
        wall_clock_based_wait_enabled_(false),
        update_check_count_wait_enabled_(false),
        min_update_checks_needed_(kDefaultMinUpdateChecks),
        max_update_checks_allowed_(kDefaultMaxUpdateChecks) {}

  OmahaRequestParams(SystemState* system_state,
                     const std::string& in_os_platform,
                     const std::string& in_os_version,
                     const std::string& in_os_sp,
                     const std::string& in_os_board,
                     const std::string& in_app_id,
                     const std::string& in_app_version,
                     const std::string& in_app_lang,
                     const std::string& in_target_channel,
                     const std::string& in_hwid,
                     const std::string& in_fw_version,
                     const std::string& in_ec_version,
                     bool in_delta_okay,
                     bool in_interactive,
                     const std::string& in_update_url,
                     const std::string& in_target_version_prefix)
      : system_state_(system_state),
        os_platform_(in_os_platform),
        os_version_(in_os_version),
        os_sp_(in_os_sp),
        app_lang_(in_app_lang),
        hwid_(in_hwid),
        fw_version_(in_fw_version),
        ec_version_(in_ec_version),
        delta_okay_(in_delta_okay),
        interactive_(in_interactive),
        update_url_(in_update_url),
        target_version_prefix_(in_target_version_prefix),
        wall_clock_based_wait_enabled_(false),
        update_check_count_wait_enabled_(false),
        min_update_checks_needed_(kDefaultMinUpdateChecks),
        max_update_checks_allowed_(kDefaultMaxUpdateChecks) {
    image_props_.board = in_os_board;
    image_props_.product_id = in_app_id;
    image_props_.canary_product_id = in_app_id;
    image_props_.version = in_app_version;
    image_props_.current_channel = in_target_channel;
    mutable_image_props_.target_channel = in_target_channel;
    mutable_image_props_.is_powerwash_allowed = false;
  }

  virtual ~OmahaRequestParams();

  // Setters and getters for the various properties.
  inline std::string os_platform() const { return os_platform_; }
  inline std::string os_version() const { return os_version_; }
  inline std::string os_sp() const { return os_sp_; }
  inline std::string os_board() const { return image_props_.board; }
  inline std::string board_app_id() const { return image_props_.product_id; }
  inline std::string canary_app_id() const {
    return image_props_.canary_product_id;
  }
  inline std::string app_lang() const { return app_lang_; }
  inline std::string hwid() const { return hwid_; }
  inline std::string fw_version() const { return fw_version_; }
  inline std::string ec_version() const { return ec_version_; }

  inline void set_app_version(const std::string& version) {
    image_props_.version = version;
  }
  inline std::string app_version() const { return image_props_.version; }

  inline std::string current_channel() const {
    return image_props_.current_channel;
  }
  inline std::string target_channel() const {
    return mutable_image_props_.target_channel;
  }
  inline std::string download_channel() const { return download_channel_; }

  // Can client accept a delta ?
  inline void set_delta_okay(bool ok) { delta_okay_ = ok; }
  inline bool delta_okay() const { return delta_okay_; }

  // True if this is a user-initiated update check.
  inline void set_interactive(bool interactive) { interactive_ = interactive; }
  inline bool interactive() const { return interactive_; }

  inline void set_update_url(const std::string& url) { update_url_ = url; }
  inline std::string update_url() const { return update_url_; }

  inline void set_target_version_prefix(const std::string& prefix) {
    target_version_prefix_ = prefix;
  }

  inline std::string target_version_prefix() const {
    return target_version_prefix_;
  }

  inline void set_wall_clock_based_wait_enabled(bool enabled) {
    wall_clock_based_wait_enabled_ = enabled;
  }
  inline bool wall_clock_based_wait_enabled() const {
    return wall_clock_based_wait_enabled_;
  }

  inline void set_waiting_period(base::TimeDelta period) {
    waiting_period_ = period;
  }
  base::TimeDelta waiting_period() const { return waiting_period_; }

  inline void set_update_check_count_wait_enabled(bool enabled) {
    update_check_count_wait_enabled_ = enabled;
  }

  inline bool update_check_count_wait_enabled() const {
    return update_check_count_wait_enabled_;
  }

  inline void set_min_update_checks_needed(int64_t min) {
    min_update_checks_needed_ = min;
  }
  inline int64_t min_update_checks_needed() const {
    return min_update_checks_needed_;
  }

  inline void set_max_update_checks_allowed(int64_t max) {
    max_update_checks_allowed_ = max;
  }
  inline int64_t max_update_checks_allowed() const {
    return max_update_checks_allowed_;
  }

  // True if we're trying to update to a more stable channel.
  // i.e. index(target_channel) > index(current_channel).
  virtual bool to_more_stable_channel() const;

  // Returns the app id corresponding to the current value of the
  // download channel.
  virtual std::string GetAppId() const;

  // Suggested defaults
  static const char kOsVersion[];
  static const char kIsPowerwashAllowedKey[];
  static const int64_t kDefaultMinUpdateChecks = 0;
  static const int64_t kDefaultMaxUpdateChecks = 8;

  // Initializes all the data in the object. Non-empty
  // |in_app_version| or |in_update_url| prevents automatic detection
  // of the parameter. Returns true on success, false otherwise.
  bool Init(const std::string& in_app_version,
            const std::string& in_update_url,
            bool in_interactive);

  // Permanently changes the release channel to |channel|. Performs a
  // powerwash, if required and allowed.
  // Returns true on success, false otherwise. Note: This call will fail if
  // there's a channel change pending already. This is to serialize all the
  // channel changes done by the user in order to avoid having to solve
  // numerous edge cases around ensuring the powerwash happens as intended in
  // all such cases.
  virtual bool SetTargetChannel(const std::string& channel,
                                bool is_powerwash_allowed,
                                std::string* error_message);

  // Updates the download channel for this particular attempt from the current
  // value of target channel.  This method takes a "snapshot" of the current
  // value of target channel and uses it for all subsequent Omaha requests for
  // this attempt (i.e. initial request as well as download progress/error
  // event requests). The snapshot will be updated only when either this method
  // or Init is called again.
  virtual void UpdateDownloadChannel();

  virtual bool is_powerwash_allowed() const {
    return mutable_image_props_.is_powerwash_allowed;
  }

  // Check if the provided update URL is official, meaning either the default
  // autoupdate server or the autoupdate autotest server.
  virtual bool IsUpdateUrlOfficial() const;

  // For unit-tests.
  void set_root(const std::string& root);
  void set_current_channel(const std::string& channel) {
    image_props_.current_channel = channel;
  }
  void set_target_channel(const std::string& channel) {
    mutable_image_props_.target_channel = channel;
  }

 private:
  FRIEND_TEST(OmahaRequestParamsTest, IsValidChannelTest);
  FRIEND_TEST(OmahaRequestParamsTest, ShouldLockDownTest);
  FRIEND_TEST(OmahaRequestParamsTest, ChannelIndexTest);
  FRIEND_TEST(OmahaRequestParamsTest, LsbPreserveTest);
  FRIEND_TEST(OmahaRequestParamsTest, CollectECFWVersionsTest);

  // Returns true if |channel| is a valid channel, false otherwise.
  bool IsValidChannel(const std::string& channel) const;

  // Returns the index of the given channel.
  int GetChannelIndex(const std::string& channel) const;

  // Returns True if we should store the fw/ec versions based on our hwid_.
  // Compares hwid to a set of whitelisted prefixes.
  bool CollectECFWVersions() const;

  // These are individual helper methods to initialize the said properties from
  // the LSB value.
  void SetTargetChannelFromLsbValue();
  void SetCurrentChannelFromLsbValue();
  void SetIsPowerwashAllowedFromLsbValue();

  // Initializes the required properties from the LSB value.
  void InitFromLsbValue();

  // Gets the machine type (e.g. "i686").
  std::string GetMachineType() const;

  // Global system context.
  SystemState* system_state_;

  // The system image properties.
  ImageProperties image_props_;
  MutableImageProperties mutable_image_props_;

  // Basic properties of the OS and Application that go into the Omaha request.
  std::string os_platform_;
  std::string os_version_;
  std::string os_sp_;
  std::string app_lang_;

  // There are three channel values we deal with:
  // * The channel we got the image we are running from or "current channel"
  //   stored in |image_props_.current_channel|.
  //
  // * The release channel we are tracking, where we should get updates from,
  //   stored in |mutable_image_props_.target_channel|. This channel is
  //   normally the same as the current_channel, except when the user changes
  //   the channel. In that case it'll have the release channel the user
  //   switched to, regardless of whether we downloaded an update from that
  //   channel or not, or if we are in the middle of a download from a
  //   previously selected channel  (as opposed to download channel
  //   which gets updated only at the start of next download).
  //
  // * The channel from which we're downloading the payload. This should
  //   normally be the same as target channel. But if the user made another
  //   channel change after we started the download, then they'd be different,
  //   in which case, we'd detect elsewhere that the target channel has been
  //   changed and cancel the current download attempt.
  std::string download_channel_;

  std::string hwid_;  // Hardware Qualification ID of the client
  std::string fw_version_;  // Chrome OS Firmware Version.
  std::string ec_version_;  // Chrome OS EC Version.
  bool delta_okay_;  // If this client can accept a delta
  bool interactive_;   // Whether this is a user-initiated update check

  // The URL to send the Omaha request to.
  std::string update_url_;

  // Prefix of the target OS version that the enterprise wants this device
  // to be pinned to. It's empty otherwise.
  std::string target_version_prefix_;

  // True if scattering is enabled, in which case waiting_period_ specifies the
  // amount of absolute time that we've to wait for before sending a request to
  // Omaha.
  bool wall_clock_based_wait_enabled_;
  base::TimeDelta waiting_period_;

  // True if scattering is enabled to denote the number of update checks
  // we've to skip before we can send a request to Omaha. The min and max
  // values establish the bounds for a random number to be chosen within that
  // range to enable such a wait.
  bool update_check_count_wait_enabled_;
  int64_t min_update_checks_needed_;
  int64_t max_update_checks_allowed_;

  // When reading files, prepend root_ to the paths. Useful for testing.
  std::string root_;

  // TODO(jaysri): Uncomment this after fixing unit tests, as part of
  // chromium-os:39752
  // DISALLOW_COPY_AND_ASSIGN(OmahaRequestParams);
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_OMAHA_REQUEST_PARAMS_H_
