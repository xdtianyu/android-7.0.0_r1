//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef SHILL_CELLULAR_MOBILE_OPERATOR_INFO_IMPL_H_
#define SHILL_CELLULAR_MOBILE_OPERATOR_INFO_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/cancelable_callback.h>
#include <base/files/file_util.h>
#include <base/memory/scoped_vector.h>
#include <base/memory/weak_ptr.h>
#include <base/observer_list.h>

#include "shill/cellular/mobile_operator_info.h"
#include "shill/event_dispatcher.h"
#include "shill/mobile_operator_db/mobile_operator_db.pb.h"

namespace shill {

class MobileOperatorInfoImpl {
 public:
  typedef
  std::map<std::string,
           std::vector<const mobile_operator_db::MobileNetworkOperator*>>
      StringToMNOListMap;

  MobileOperatorInfoImpl(EventDispatcher* dispatcher,
                         const std::string& info_owner);
  ~MobileOperatorInfoImpl();

  // API functions of the interface.
  // See mobile_operator_info_impl.h for details.
  void ClearDatabasePaths();
  void AddDatabasePath(const base::FilePath& absolute_path);
  bool Init();
  void AddObserver(MobileOperatorInfo::Observer* observer);
  void RemoveObserver(MobileOperatorInfo::Observer* observer);
  bool IsMobileNetworkOperatorKnown() const;
  bool IsMobileVirtualNetworkOperatorKnown() const;
  const std::string& info_owner() const;
  const std::string& uuid() const;
  const std::string& operator_name() const;
  const std::string& country() const;
  const std::string& mccmnc() const;
  const std::string& sid() const;
  const std::string& nid() const;
  const std::vector<std::string>& mccmnc_list() const;
  const std::vector<std::string>& sid_list() const;
  const std::vector<MobileOperatorInfo::LocalizedName>
      &operator_name_list() const;
  const ScopedVector<MobileOperatorInfo::MobileAPN>& apn_list() const;
  const std::vector<MobileOperatorInfo::OnlinePortal>& olp_list() const;
  const std::string& activation_code() const;
  bool requires_roaming() const;
  void Reset();
  void UpdateIMSI(const std::string& imsi);
  void UpdateICCID(const std::string& iccid);
  void UpdateMCCMNC(const std::string& mccmnc);
  void UpdateSID(const std::string& sid);
  void UpdateNID(const std::string& nid);
  void UpdateOperatorName(const std::string& operator_name);
  void UpdateOnlinePortal(const std::string& url,
                          const std::string& method,
                          const std::string& post_data);

 private:
  friend class MobileOperatorInfoInitTest;

  // ///////////////////////////////////////////////////////////////////////////
  // Static variables.
  // Default databases to load.
  static const char* kDefaultDatabasePath;
  // MCCMNC can be of length 5 or 6. When using this constant, keep in mind that
  // the length of MCCMNC can by |kMCCMNCMinLen| or |kMCCMNCMinLen + 1|.
  static const int kMCCMNCMinLen;

  // ///////////////////////////////////////////////////////////////////////////
  // Functions.
  void PreprocessDatabase();
  // This function assumes that duplicate |values| are never inserted for the
  // same |key|. If you do that, the function is too dumb to deduplicate the
  // |value|s, and two copies will get stored.
  void InsertIntoStringToMNOListMap(
      StringToMNOListMap* table,
      const std::string& key,
      const mobile_operator_db::MobileNetworkOperator* value);

  bool UpdateMNO();
  bool UpdateMVNO();
  bool FilterMatches(const shill::mobile_operator_db::Filter& filter);
  const mobile_operator_db::MobileNetworkOperator* PickOneFromDuplicates(
      const std::vector<const mobile_operator_db::MobileNetworkOperator*>
          &duplicates) const;
  // Reloads the information about M[V]NO from the database.
  void RefreshDBInformation();
  void ClearDBInformation();
  // Reload all data from |data|.
  // Semantics: If a field data.x exists, then it *overwrites* the current
  // information gained from data.x. E.g., if |data.name_size() > 0| is true,
  // then we replace *all* names. Otherwise, we leave names untouched.
  // This allows MVNOs to overwrite information obtained from the corresponding
  // MNO.
  void ReloadData(const mobile_operator_db::Data& data);
  // Append candidates recognized by |mccmnc| to the candidate list.
  bool AppendToCandidatesByMCCMNC(const std::string& mccmnc);
  bool AppendToCandidatesBySID(const std::string& sid);
  std::string OperatorCodeString() const;

  // Notifies all observers that the operator has changed.
  void PostNotifyOperatorChanged();
  // The actual notification is sent out here. This should not be called
  // directly from any function.
  void NotifyOperatorChanged();

  // For a property update that does not result in an M[V]NO update, this
  // function determines whether observers should be notified anyway.
  bool ShouldNotifyPropertyUpdate() const;

  // OperatorName comparisons for determining the MNO are done after normalizing
  // the names to ignore case and spaces.
  std::string NormalizeOperatorName(const std::string& name) const;

  // These functions encapsulate the logic to update different properties
  // properly whenever an update is either received from the user or the
  // database.
  void HandleMCCMNCUpdate();
  void HandleOperatorNameUpdate();
  void HandleSIDUpdate();
  void HandleOnlinePortalUpdate();

  // Accessor functions for testing purpose only.
  mobile_operator_db::MobileOperatorDB* database() {
    return database_.get();
  }

  // ///////////////////////////////////////////////////////////////////////////
  // Data.
  // Not owned by MobileOperatorInfoImpl.
  EventDispatcher* const dispatcher_;

  const std::string info_owner_;

  // Owned by MobileOperatorInfoImpl, may be created externally.
  std::vector<base::FilePath> database_paths_;

  // Owned and modified only by MobileOperatorInfoImpl.
  // The observers added to this list are not owned by this object. Moreover,
  // the observer is likely to outlive this object. We do enforce removal of all
  // observers before this object is destroyed.
  base::ObserverList<MobileOperatorInfo::Observer> observers_;
  base::CancelableClosure notify_operator_changed_task_;

  std::unique_ptr<mobile_operator_db::MobileOperatorDB> database_;
  StringToMNOListMap mccmnc_to_mnos_;
  StringToMNOListMap sid_to_mnos_;
  StringToMNOListMap name_to_mnos_;

  // |candidates_by_operator_code| can be determined either using MCCMNC or
  // using SID.  At any one time, we only expect one of these operator codes to
  // be updated by the user. We use |operator_code_type_| to keep track of which
  // update we have received and warn the user if we receive both.
  enum OperatorCodeType {
    kOperatorCodeTypeUnknown = 0,
    kOperatorCodeTypeMCCMNC,
    kOperatorCodeTypeSID,
  };
  OperatorCodeType operator_code_type_;
  std::vector<const mobile_operator_db::MobileNetworkOperator*>
      candidates_by_operator_code_;

  std::vector<const mobile_operator_db::MobileNetworkOperator*>
      candidates_by_name_;
  const mobile_operator_db::MobileNetworkOperator* current_mno_;
  const mobile_operator_db::MobileVirtualNetworkOperator* current_mvno_;

  // These fields are the information expected to be populated by this object
  // after successfully determining the MVNO.
  std::string uuid_;
  std::string operator_name_;
  std::string country_;
  std::string mccmnc_;
  std::string sid_;
  std::string nid_;
  std::vector<std::string> mccmnc_list_;
  std::vector<std::string> sid_list_;
  std::vector<MobileOperatorInfo::LocalizedName> operator_name_list_;
  ScopedVector<MobileOperatorInfo::MobileAPN> apn_list_;
  std::vector<MobileOperatorInfo::OnlinePortal> olp_list_;
  std::vector<mobile_operator_db::OnlinePortal> raw_olp_list_;
  std::string activation_code_;
  bool requires_roaming_;
  // These fields store the data obtained from the Update* methods.
  // The database information is kept separate from the information gathered
  // through the Update* methods, because one or the other may be given
  // precedence in different situations.
  // Note: For simplicity, we do not allow the user to enforce an empty value
  // for these variables. So, if |user_mccmnc_| == "", the |mccmnc_| obtained
  // from the database will be used, even if |user_mccmnc_| was explicitly set
  // by the user.
  std::string user_imsi_;
  std::string user_iccid_;
  std::string user_mccmnc_;
  std::string user_sid_;
  std::string user_nid_;
  std::string user_operator_name_;
  bool user_olp_empty_;
  MobileOperatorInfo::OnlinePortal user_olp_;

  // This must be the last data member of this class.
  base::WeakPtrFactory<MobileOperatorInfoImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MobileOperatorInfoImpl);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_MOBILE_OPERATOR_INFO_IMPL_H_
