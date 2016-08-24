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

#ifndef SHILL_CELLULAR_MOBILE_OPERATOR_INFO_H_
#define SHILL_CELLULAR_MOBILE_OPERATOR_INFO_H_

#include <memory>
#include <string>
#include <vector>

#include <base/files/file_util.h>
#include <base/memory/scoped_vector.h>

namespace shill {

class EventDispatcher;
class MobileOperatorInfoImpl;

// An MobileOperatorInfo object encapsulates the knowledge pertaining to all
// mobile operators. Typical usage consists of three steps:
//   - Initialize the object, set database file paths for the operator
//   information.
//   - Add observers to be notified whenever an M[V]NO has been determined / any
//   information about the M[V]NO changes.
//   - Send operator information updates to the object.
//
// So a class Foo that wants to use this object typically looks like:
//
// class Foo {
//   class OperatorObserver : public MobileOperatorInfoObserver {
//     // Implement all Observer functions.
//   }
//   ...
//
//   MobileOperatorInfo operator_info;
//   // Optional: Set a non-default database file.
//   operator_info.ClearDatabasePaths();
//   operator_info.AddDatabasePath(some_path);
//
//   operator_info.Init();  // Required.
//
//   OperatorObserver my_observer;
//   operator_info.AddObserver(my_observer);
//   ...
//   operator_info.UpdateIMSI(some_imsi);
//   operator_info.UpdateName(some_name);
//   ...
//   // Whenever enough information is available, |operator_info| notifies us
//   through |my_observer|.
// };
//
class MobileOperatorInfo {
 public:
  class Observer {
   public:
    virtual ~Observer() {}

    // This event fires when
    //   - A mobile [virtual] network operator
    //     - is first determined.
    //     - changes.
    //     - becomes invalid.
    //   - Some information about the known operator changes.
    virtual void OnOperatorChanged() = 0;
  };

  // |Init| must be called on the constructed object before it is used.
  // This object does not take ownership of dispatcher, and |dispatcher| is
  // expected to outlive this object.
  MobileOperatorInfo(EventDispatcher* dispatcher,
                     const std::string& info_owner);
  virtual ~MobileOperatorInfo();

  // These functions can be called before Init to read non default database
  // file(s).
  void ClearDatabasePaths();
  void AddDatabasePath(const base::FilePath& absolute_path);

  std::string GetLogPrefix(const char* func) const;
  bool Init();

  // Add/remove observers to subscribe to notifications.
  void AddObserver(MobileOperatorInfo::Observer* observer);
  void RemoveObserver(MobileOperatorInfo::Observer* observer);

  // ///////////////////////////////////////////////////////////////////////////
  // Objects that encapsulate related information about the mobile operator.

  // Encapsulates a name and the language that name has been localized to.
  // The name can be a carrier name, or the name that a cellular carrier
  // prefers to show for a certain access point.
  struct LocalizedName {
    // The name as it appears in the corresponding language.
    std::string name;
    // The language of this localized name. The format of a language is a two
    // letter language code, e.g. 'en' for English.
    // It is legal for an instance of LocalizedName to have an empty |language|
    // field, as sometimes the underlying database does not contain that
    // information.
    std::string language;
  };

  // Encapsulates information on a mobile access point name. This information
  // is usually necessary for 3GPP networks to be able to connect to a mobile
  // network. So far, CDMA networks don't use this information.
  struct MobileAPN {
    // The access point url, which is fed to the modemmanager while connecting.
    std::string apn;
    // A list of localized names for this access point. Usually there is only
    // one for each country that the associated cellular carrier operates in.
    std::vector<LocalizedName> operator_name_list;
    // The username and password fields that are required by the modemmanager.
    // Either of these values can be empty if none is present. If a MobileAPN
    // instance that is obtained from this parser contains a non-empty value
    // for username/password, this usually means that the carrier requires
    // a certain default pair.
    std::string username;
    std::string password;
  };

  // Encapsulates information about the Online payment portal used by chrome to
  // redirect users for some carriers.
  struct OnlinePortal {
    std::string url;
    std::string method;
    std::string post_data;
  };

  // ///////////////////////////////////////////////////////////////////////////
  // Functions to obtain information about the current mobile operator.
  // Any of these accessors can return an emtpy response if the information is
  // not available. Use |IsMobileNetworkOperatorKnown| and
  // |IsMobileVirtualNetworkOperatorKnown| to determine if a fix on the operator
  // has been made. Note that the information returned by the other accessors is
  // only valid when at least |IsMobileNetworkOperatorKnown| returns true. Their
  // values are undefined otherwise.

  // Query whether a mobile network operator has been successfully determined.
  virtual bool IsMobileNetworkOperatorKnown() const;
  // Query whether a mobile network operator has been successfully
  // determined.
  bool IsMobileVirtualNetworkOperatorKnown() const;


  // The unique identifier of this carrier. This is primarily used to
  // identify the user profile in store for each carrier. This identifier is
  // access technology agnostic and should be the same across 3GPP and CDMA.
  virtual const std::string& uuid() const;

  virtual const std::string& operator_name() const;
  virtual const std::string& country() const;
  virtual const std::string& mccmnc() const;
  const std::string& sid() const;
  const std::string& nid() const;

  // A given MVNO can be associated with multiple mcc/mnc pairs. A list of all
  // associated mcc/mnc pairs concatenated together.
  const std::vector<std::string>& mccmnc_list() const;
  // A given MVNO can be associated with multiple sid(s). A list of all
  // associated sid(s).
  // There are likely many SID values associated with a CDMA carrier as they
  // vary across regions and are more fine grained than countries. An important
  // thing to keep in mind is that, since an SID contains fine grained
  // information on where a modem is physically located, it should be regarded
  // as user-sensitive information.
  const std::vector<std::string>& sid_list() const;
  // All localized names associated with this carrier entry.
  const std::vector<LocalizedName>& operator_name_list() const;
  // All access point names associated with this carrier entry.
  const ScopedVector<MobileAPN>& apn_list() const;
  // All Online Payment Portal URLs associated with this carrier entry. There
  // are usually multiple OLPs based on access technology and it is up to the
  // application to use the appropriate one.
  virtual const std::vector<OnlinePortal>& olp_list() const;

  // The number to dial for automatic activation.
  virtual const std::string& activation_code() const;
  // Some carriers are only available while roaming. This is mainly used by
  // Chrome.
  bool requires_roaming() const;

  // ///////////////////////////////////////////////////////////////////////////
  // Functions used to notify this object of operator data changes.
  // The Update* methods update the corresponding property of the network
  // operator, and this value may be used to determine the M[V]NO.
  // These values are also the values reported through accessors, overriding any
  // information from the database.

  // Throw away all information provided to the object, and start from top.
  void Reset();

  // Both MCCMNC and SID correspond to operator code in the different
  // technologies. They are never to be used together. If you want to use SID
  // after MCCMNC (or vice-versa), ensure a call to |Reset| to clear state.
  virtual void UpdateMCCMNC(const std::string& mccmnc);
  virtual void UpdateSID(const std::string& sid);

  virtual void UpdateIMSI(const std::string& imsi);
  void UpdateICCID(const std::string& iccid);
  virtual void UpdateNID(const std::string& nid);
  virtual void UpdateOperatorName(const std::string& operator_name);
  void UpdateOnlinePortal(const std::string& url,
                          const std::string& method,
                          const std::string& post_data);

  // ///////////////////////////////////////////////////////////////////////////
  // Expose implementation for test purposes only.
  MobileOperatorInfoImpl* impl() { return impl_.get(); }

 private:
  std::unique_ptr<MobileOperatorInfoImpl> impl_;
  DISALLOW_COPY_AND_ASSIGN(MobileOperatorInfo);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_MOBILE_OPERATOR_INFO_H_
