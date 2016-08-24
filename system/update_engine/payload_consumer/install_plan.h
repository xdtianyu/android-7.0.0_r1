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

#ifndef UPDATE_ENGINE_PAYLOAD_CONSUMER_INSTALL_PLAN_H_
#define UPDATE_ENGINE_PAYLOAD_CONSUMER_INSTALL_PLAN_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <brillo/secure_blob.h>

#include "update_engine/common/action.h"
#include "update_engine/common/boot_control_interface.h"

// InstallPlan is a simple struct that contains relevant info for many
// parts of the update system about the install that should happen.
namespace chromeos_update_engine {

enum class InstallPayloadType {
  kUnknown,
  kFull,
  kDelta,
};

std::string InstallPayloadTypeToString(InstallPayloadType type);

struct InstallPlan {
  InstallPlan() = default;

  bool operator==(const InstallPlan& that) const;
  bool operator!=(const InstallPlan& that) const;

  void Dump() const;

  // Load the |source_path| and |target_path| of all |partitions| based on the
  // |source_slot| and |target_slot| if available. Returns whether it succeeded
  // to load all the partitions for the valid slots.
  bool LoadPartitionsFromSlots(BootControlInterface* boot_control);

  bool is_resume{false};
  InstallPayloadType payload_type{InstallPayloadType::kUnknown};
  std::string download_url;  // url to download from
  std::string version;       // version we are installing.

  uint64_t payload_size{0};              // size of the payload
  std::string payload_hash;              // SHA256 hash of the payload
  uint64_t metadata_size{0};             // size of the metadata
  std::string metadata_signature;        // signature of the  metadata

  // The partition slots used for the update.
  BootControlInterface::Slot source_slot{BootControlInterface::kInvalidSlot};
  BootControlInterface::Slot target_slot{BootControlInterface::kInvalidSlot};

  // The vector below is used for partition verification. The flow is:
  //
  // 1. FilesystemVerifierAction computes and fills in the source partition
  // hash based on the guessed source size for delta major version 1 updates.
  //
  // 2. DownloadAction verifies the source partition sizes and hashes against
  // the expected values transmitted in the update manifest. It fills in the
  // expected target partition sizes and hashes based on the manifest.
  //
  // 3. FilesystemVerifierAction computes and verifies the applied partition
  // sizes and hashes against the expected values in target_partition_hashes.
  struct Partition {
    bool operator==(const Partition& that) const;

    // The name of the partition.
    std::string name;

    std::string source_path;
    uint64_t source_size{0};
    brillo::Blob source_hash;

    std::string target_path;
    uint64_t target_size{0};
    brillo::Blob target_hash;

    // Whether we should run the postinstall script from this partition and the
    // postinstall parameters.
    bool run_postinstall{false};
    std::string postinstall_path;
    std::string filesystem_type;
  };
  std::vector<Partition> partitions;

  // True if payload hash checks are mandatory based on the system state and
  // the Omaha response.
  bool hash_checks_mandatory{false};

  // True if Powerwash is required on reboot after applying the payload.
  // False otherwise.
  bool powerwash_required{false};

  // If not blank, a base-64 encoded representation of the PEM-encoded
  // public key in the response.
  std::string public_key_rsa;
};

class InstallPlanAction;

template<>
class ActionTraits<InstallPlanAction> {
 public:
  // Takes the install plan as input
  typedef InstallPlan InputObjectType;
  // Passes the install plan as output
  typedef InstallPlan OutputObjectType;
};

// Basic action that only receives and sends Install Plans.
// Can be used to construct an Install Plan to send to any other Action that
// accept an InstallPlan.
class InstallPlanAction : public Action<InstallPlanAction> {
 public:
  InstallPlanAction() {}
  explicit InstallPlanAction(const InstallPlan& install_plan):
    install_plan_(install_plan) {}

  void PerformAction() override {
    if (HasOutputPipe()) {
      SetOutputObject(install_plan_);
    }
    processor_->ActionComplete(this, ErrorCode::kSuccess);
  }

  InstallPlan* install_plan() { return &install_plan_; }

  static std::string StaticType() { return "InstallPlanAction"; }
  std::string Type() const override { return StaticType(); }

  typedef ActionTraits<InstallPlanAction>::InputObjectType InputObjectType;
  typedef ActionTraits<InstallPlanAction>::OutputObjectType OutputObjectType;

 private:
  InstallPlan install_plan_;

  DISALLOW_COPY_AND_ASSIGN(InstallPlanAction);
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_PAYLOAD_CONSUMER_INSTALL_PLAN_H_
