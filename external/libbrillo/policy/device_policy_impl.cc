// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "policy/device_policy_impl.h"

#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/macros.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

#include <set>
#include <string>

#include "bindings/chrome_device_policy.pb.h"
#include "bindings/device_management_backend.pb.h"

namespace policy {

namespace {
const char kPolicyPath[] = "/var/lib/whitelist/policy";
const char kPublicKeyPath[] = "/var/lib/whitelist/owner.key";

// Reads the public key used to sign the policy from |key_file| and stores it
// in |public_key|. Returns true on success.
bool ReadPublicKeyFromFile(const base::FilePath& key_file,
                           std::string* public_key) {
  if (!base::PathExists(key_file))
    return false;
  public_key->clear();
  if (!base::ReadFileToString(key_file, public_key) || public_key->empty()) {
    LOG(ERROR) << "Could not read public key off disk";
    return false;
  }
  return true;
}

// Verifies that the |signed_data| has correct |signature| with |public_key|.
bool VerifySignature(const std::string& signed_data,
                     const std::string& signature,
                     const std::string& public_key) {
  EVP_MD_CTX ctx;
  EVP_MD_CTX_init(&ctx);

  const EVP_MD* digest = EVP_sha1();

  char* key = const_cast<char*>(public_key.data());
  BIO* bio = BIO_new_mem_buf(key, public_key.length());
  if (!bio) {
    EVP_MD_CTX_cleanup(&ctx);
    return false;
  }

  EVP_PKEY* public_key_ssl = d2i_PUBKEY_bio(bio, nullptr);
  if (!public_key_ssl) {
    BIO_free_all(bio);
    EVP_MD_CTX_cleanup(&ctx);
    return false;
  }

  const unsigned char* sig =
      reinterpret_cast<const unsigned char*>(signature.data());
  int rv = EVP_VerifyInit_ex(&ctx, digest, nullptr);
  if (rv == 1) {
    EVP_VerifyUpdate(&ctx, signed_data.data(), signed_data.length());
    rv = EVP_VerifyFinal(&ctx,
                         sig, signature.length(),
                         public_key_ssl);
  }

  EVP_PKEY_free(public_key_ssl);
  BIO_free_all(bio);
  EVP_MD_CTX_cleanup(&ctx);

  return rv == 1;
}

// Decodes the connection type enum from the device settings protobuf to string
// representations. The strings must match the connection manager definitions.
std::string DecodeConnectionType(int type) {
  static const char* const kConnectionTypes[] = {
    "ethernet",
    "wifi",
    "wimax",
    "bluetooth",
    "cellular",
  };

  if (type < 0 || type >= static_cast<int>(arraysize(kConnectionTypes)))
    return std::string();

  return kConnectionTypes[type];
}

}  // namespace

DevicePolicyImpl::DevicePolicyImpl()
    : policy_path_(kPolicyPath),
      keyfile_path_(kPublicKeyPath) {
}

DevicePolicyImpl::~DevicePolicyImpl() {
}

bool DevicePolicyImpl::LoadPolicy() {
  if (!VerifyPolicyFiles()) {
    return false;
  }

  std::string polstr;
  if (!base::ReadFileToString(policy_path_, &polstr) || polstr.empty()) {
    LOG(ERROR) << "Could not read policy off disk";
    return false;
  }
  if (!policy_.ParseFromString(polstr) || !policy_.has_policy_data()) {
    LOG(ERROR) << "Policy on disk could not be parsed!";
    return false;
  }
  policy_data_.ParseFromString(policy_.policy_data());
  if (!policy_data_.has_policy_value()) {
    LOG(ERROR) << "Policy on disk could not be parsed!";
    return false;
  }

  // Make sure the signature is still valid.
  if (!VerifyPolicySignature()) {
    LOG(ERROR) << "Policy signature verification failed!";
    return false;
  }

  device_policy_.ParseFromString(policy_data_.policy_value());
  return true;
}

bool DevicePolicyImpl::GetPolicyRefreshRate(int* rate) const {
  if (!device_policy_.has_device_policy_refresh_rate())
    return false;
  *rate = static_cast<int>(
      device_policy_.device_policy_refresh_rate().device_policy_refresh_rate());
  return true;
}

bool DevicePolicyImpl::GetUserWhitelist(
    std::vector<std::string>* user_whitelist) const {
  if (!device_policy_.has_user_whitelist())
    return false;
  const enterprise_management::UserWhitelistProto& proto =
      device_policy_.user_whitelist();
  user_whitelist->clear();
  for (int i = 0; i < proto.user_whitelist_size(); i++)
    user_whitelist->push_back(proto.user_whitelist(i));
  return true;
}

bool DevicePolicyImpl::GetGuestModeEnabled(bool* guest_mode_enabled) const {
  if (!device_policy_.has_guest_mode_enabled())
    return false;
  *guest_mode_enabled =
      device_policy_.guest_mode_enabled().guest_mode_enabled();
  return true;
}

bool DevicePolicyImpl::GetCameraEnabled(bool* camera_enabled) const {
  if (!device_policy_.has_camera_enabled())
    return false;
  *camera_enabled = device_policy_.camera_enabled().camera_enabled();
  return true;
}

bool DevicePolicyImpl::GetShowUserNames(bool* show_user_names) const {
  if (!device_policy_.has_show_user_names())
    return false;
  *show_user_names = device_policy_.show_user_names().show_user_names();
  return true;
}

bool DevicePolicyImpl::GetDataRoamingEnabled(bool* data_roaming_enabled) const {
  if (!device_policy_.has_data_roaming_enabled())
    return false;
  *data_roaming_enabled =
      device_policy_.data_roaming_enabled().data_roaming_enabled();
  return true;
}

bool DevicePolicyImpl::GetAllowNewUsers(bool* allow_new_users) const {
  if (!device_policy_.has_allow_new_users())
    return false;
  *allow_new_users = device_policy_.allow_new_users().allow_new_users();
  return true;
}

bool DevicePolicyImpl::GetMetricsEnabled(bool* metrics_enabled) const {
  if (!device_policy_.has_metrics_enabled())
    return false;
  *metrics_enabled = device_policy_.metrics_enabled().metrics_enabled();
  return true;
}

bool DevicePolicyImpl::GetReportVersionInfo(bool* report_version_info) const {
  if (!device_policy_.has_device_reporting())
    return false;

  const enterprise_management::DeviceReportingProto& proto =
      device_policy_.device_reporting();
  if (!proto.has_report_version_info())
    return false;

  *report_version_info = proto.report_version_info();
  return true;
}

bool DevicePolicyImpl::GetReportActivityTimes(
    bool* report_activity_times) const {
  if (!device_policy_.has_device_reporting())
    return false;

  const enterprise_management::DeviceReportingProto& proto =
      device_policy_.device_reporting();
  if (!proto.has_report_activity_times())
    return false;

  *report_activity_times = proto.report_activity_times();
  return true;
}

bool DevicePolicyImpl::GetReportBootMode(bool* report_boot_mode) const {
  if (!device_policy_.has_device_reporting())
    return false;

  const enterprise_management::DeviceReportingProto& proto =
      device_policy_.device_reporting();
  if (!proto.has_report_boot_mode())
    return false;

  *report_boot_mode = proto.report_boot_mode();
  return true;
}

bool DevicePolicyImpl::GetEphemeralUsersEnabled(
    bool* ephemeral_users_enabled) const {
  if (!device_policy_.has_ephemeral_users_enabled())
    return false;
  *ephemeral_users_enabled =
      device_policy_.ephemeral_users_enabled().ephemeral_users_enabled();
  return true;
}

bool DevicePolicyImpl::GetReleaseChannel(
    std::string* release_channel) const {
  if (!device_policy_.has_release_channel())
    return false;

  const enterprise_management::ReleaseChannelProto& proto =
      device_policy_.release_channel();
  if (!proto.has_release_channel())
    return false;

  *release_channel = proto.release_channel();
  return true;
}

bool DevicePolicyImpl::GetReleaseChannelDelegated(
    bool* release_channel_delegated) const {
  if (!device_policy_.has_release_channel())
    return false;

  const enterprise_management::ReleaseChannelProto& proto =
      device_policy_.release_channel();
  if (!proto.has_release_channel_delegated())
    return false;

  *release_channel_delegated = proto.release_channel_delegated();
  return true;
}

bool DevicePolicyImpl::GetUpdateDisabled(
    bool* update_disabled) const {
  if (!device_policy_.has_auto_update_settings())
    return false;

  const enterprise_management::AutoUpdateSettingsProto& proto =
      device_policy_.auto_update_settings();
  if (!proto.has_update_disabled())
    return false;

  *update_disabled = proto.update_disabled();
  return true;
}

bool DevicePolicyImpl::GetTargetVersionPrefix(
    std::string* target_version_prefix) const {
  if (!device_policy_.has_auto_update_settings())
    return false;

  const enterprise_management::AutoUpdateSettingsProto& proto =
      device_policy_.auto_update_settings();
  if (!proto.has_target_version_prefix())
    return false;

  *target_version_prefix = proto.target_version_prefix();
  return true;
}

bool DevicePolicyImpl::GetScatterFactorInSeconds(
    int64_t* scatter_factor_in_seconds) const {
  if (!device_policy_.has_auto_update_settings())
    return false;

  const enterprise_management::AutoUpdateSettingsProto& proto =
      device_policy_.auto_update_settings();
  if (!proto.has_scatter_factor_in_seconds())
    return false;

  *scatter_factor_in_seconds = proto.scatter_factor_in_seconds();
  return true;
}

bool DevicePolicyImpl::GetAllowedConnectionTypesForUpdate(
      std::set<std::string>* connection_types) const {
  if (!device_policy_.has_auto_update_settings())
    return false;

  const enterprise_management::AutoUpdateSettingsProto& proto =
      device_policy_.auto_update_settings();
  if (proto.allowed_connection_types_size() <= 0)
    return false;

  for (int i = 0; i < proto.allowed_connection_types_size(); i++) {
    std::string type = DecodeConnectionType(proto.allowed_connection_types(i));
    if (!type.empty())
      connection_types->insert(type);
  }
  return true;
}

bool DevicePolicyImpl::GetOpenNetworkConfiguration(
    std::string* open_network_configuration) const {
  if (!device_policy_.has_open_network_configuration())
    return false;

  const enterprise_management::DeviceOpenNetworkConfigurationProto& proto =
      device_policy_.open_network_configuration();
  if (!proto.has_open_network_configuration())
    return false;

  *open_network_configuration = proto.open_network_configuration();
  return true;
}

bool DevicePolicyImpl::GetOwner(std::string* owner) const {
  // The device is enterprise enrolled iff a request token exists.
  if (policy_data_.has_request_token()) {
    *owner = "";
    return true;
  }
  if (!policy_data_.has_username())
    return false;
  *owner = policy_data_.username();
  return true;
}

bool DevicePolicyImpl::GetHttpDownloadsEnabled(
    bool* http_downloads_enabled) const {
  if (!device_policy_.has_auto_update_settings())
    return false;

  const enterprise_management::AutoUpdateSettingsProto& proto =
      device_policy_.auto_update_settings();

  if (!proto.has_http_downloads_enabled())
    return false;

  *http_downloads_enabled = proto.http_downloads_enabled();
  return true;
}

bool DevicePolicyImpl::GetAuP2PEnabled(bool* au_p2p_enabled) const {
  if (!device_policy_.has_auto_update_settings())
    return false;

  const enterprise_management::AutoUpdateSettingsProto& proto =
      device_policy_.auto_update_settings();

  if (!proto.has_p2p_enabled())
    return false;

  *au_p2p_enabled = proto.p2p_enabled();
  return true;
}

bool DevicePolicyImpl::VerifyPolicyFiles() {
  // Both the policy and its signature have to exist.
  if (!base::PathExists(policy_path_) || !base::PathExists(keyfile_path_)) {
    return false;
  }

  // Check if the policy and signature file is owned by root.
  struct stat file_stat;
  stat(policy_path_.value().c_str(), &file_stat);
  if (file_stat.st_uid != 0) {
    LOG(ERROR) << "Policy file is not owned by root!";
    return false;
  }
  stat(keyfile_path_.value().c_str(), &file_stat);
  if (file_stat.st_uid != 0) {
    LOG(ERROR) << "Policy signature file is not owned by root!";
    return false;
  }
  return true;
}

bool DevicePolicyImpl::VerifyPolicySignature() {
  if (policy_.has_policy_data_signature()) {
    std::string policy_data = policy_.policy_data();
    std::string policy_data_signature = policy_.policy_data_signature();
    std::string public_key;
    if (!ReadPublicKeyFromFile(base::FilePath(keyfile_path_), &public_key)) {
      LOG(ERROR) << "Could not read owner key off disk";
      return false;
    }
    if (!VerifySignature(policy_data, policy_data_signature, public_key)) {
      LOG(ERROR) << "Signature does not match the data or can not be verified!";
      return false;
    }
    return true;
  }
  LOG(ERROR) << "The policy blob is not signed!";
  return false;
}

}  // namespace policy
