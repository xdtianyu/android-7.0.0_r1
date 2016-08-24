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

#ifndef SHILL_NET_ATTRIBUTE_LIST_H_
#define SHILL_NET_ATTRIBUTE_LIST_H_

#include <linux/nl80211.h>

#include <map>
#include <memory>
#include <string>

#include <base/bind.h>

#include "shill/net/netlink_message.h"
#include "shill/net/shill_export.h"

struct nlattr;
namespace shill {

class AttributeList;
typedef scoped_refptr<const AttributeList> AttributeListConstRefPtr;
typedef scoped_refptr<AttributeList> AttributeListRefPtr;

class ByteString;
class NetlinkAttribute;
class NetlinkRawAttribute;

class SHILL_EXPORT AttributeList : public base::RefCounted<AttributeList> {
 public:
  using AttributePointer = std::shared_ptr<NetlinkAttribute>;
  using NewFromIdMethod = base::Callback<NetlinkAttribute*(int id)>;
  using AttributeMethod = base::Callback<bool(int id, const ByteString& value)>;

  AttributeList() {}

  // Instantiates an NetlinkAttribute of the appropriate type from |id|,
  // and adds it to |attributes_|.
  bool CreateAttribute(int id, NewFromIdMethod factory);

  // Helper function for creating control attribute.
  bool CreateControlAttribute(int id);

  // Helper function for creating nl80211 attribute.
  bool CreateNl80211Attribute(int id, NetlinkMessage::MessageContext context);

  // Instantiates an NetlinkAttribute of the appropriate type from |id|
  // using |factory|, initializes it from |value|, and adds it to |attributes_|.
  bool CreateAndInitAttribute(const NewFromIdMethod& factory,
                              int id, const ByteString& value);

  // Initializes the attribute |id| from the data in |value|.
  bool InitAttributeFromValue(int id, const ByteString& value);

  // Prints the attribute list with each attribute using no less than 1 line.
  // |indent| indicates the amout of leading spaces to be printed (useful for
  // nested attributes).
  void Print(int log_level, int indent) const;

  // Visit each attribute in |payload| starting at |offset|.  Call |method|
  // for each attribute.  If |method| returns false, the travesal is terminated
  // and false is returned.  If a malformed attribute entry is encountered,
  // this method also returns false.
  static bool IterateAttributes(const ByteString& payload, size_t offset,
                                const AttributeMethod& method);

  // Decode an attribute list starting from |offset| within |payload|.  Use
  // |factory| to create each attribute object.
  bool Decode(const ByteString& payload,
              size_t offset, const NewFromIdMethod& factory);

  // Returns the attributes as the payload portion of a netlink message
  // suitable for Sockets::Send.  Return value is empty on failure (or if no
  // attributes exist).
  ByteString Encode() const;

  // Create, get, and set attributes of the given types.  Attributes are
  // accessed via an integer |id|.  |id_string| is a string used to describe
  // the attribute in debug output.
  bool CreateU8Attribute(int id, const char* id_string);
  bool SetU8AttributeValue(int id, uint8_t value);
  bool GetU8AttributeValue(int id, uint8_t* value) const;

  bool CreateU16Attribute(int id, const char* id_string);
  bool SetU16AttributeValue(int id, uint16_t value);
  bool GetU16AttributeValue(int id, uint16_t* value) const;

  bool CreateU32Attribute(int id, const char* id_string);
  bool SetU32AttributeValue(int id, uint32_t value);
  bool GetU32AttributeValue(int id, uint32_t* value) const;

  bool CreateU64Attribute(int id, const char* id_string);
  bool SetU64AttributeValue(int id, uint64_t value);
  bool GetU64AttributeValue(int id, uint64_t* value) const;

  bool CreateFlagAttribute(int id, const char* id_string);
  bool SetFlagAttributeValue(int id, bool value);
  bool GetFlagAttributeValue(int id, bool* value) const;
  // |IsFlagAttributeTrue| returns true if the flag attribute |id| is true.  It
  // retruns false if the attribute does not exist, is not of type kTypeFlag,
  // or is not true.
  bool IsFlagAttributeTrue(int id) const;

  bool CreateStringAttribute(int id, const char* id_string);
  // SSID attributes are derived from string attributes.
  bool CreateSsidAttribute(int id, const char* id_string);
  bool SetStringAttributeValue(int id, std::string value);
  bool GetStringAttributeValue(int id, std::string* value) const;

  bool CreateNestedAttribute(int id, const char* id_string);
  bool SetNestedAttributeHasAValue(int id);
  bool GetNestedAttributeList(int id, AttributeListRefPtr* value);
  bool ConstGetNestedAttributeList(int id,
                                   AttributeListConstRefPtr* value) const;

  bool CreateRawAttribute(int id, const char* id_string);
  // |value| should point to the data (after the |nlattr| header, if there is
  // one).
  bool SetRawAttributeValue(int id, ByteString value);
  bool GetRawAttributeValue(int id, ByteString* output) const;

  // This retrieves a string from any kind of attribute.
  bool GetAttributeAsString(int id, std::string* value) const;

 protected:
  friend class base::RefCounted<AttributeList>;
  virtual ~AttributeList() {}

 private:
  typedef std::map<int, AttributePointer> AttributeMap;
  friend class AttributeIdIterator;
  friend class NetlinkNestedAttribute;

  // Using this to get around issues with const and operator[].
  SHILL_PRIVATE NetlinkAttribute* GetAttribute(int id) const;

  AttributeMap attributes_;

  DISALLOW_COPY_AND_ASSIGN(AttributeList);
};

// Provides a mechanism to iterate through the ids of all of the attributes
// in an |AttributeList|.  This class is really only useful if the caller
// knows the type of each attribute in advance (such as with a nested array).
class AttributeIdIterator {
 public:
  explicit AttributeIdIterator(const AttributeList& list)
      : iter_(list.attributes_.begin()),
        end_(list.attributes_.end()) {
  }
  void Advance() { ++iter_; }
  bool AtEnd() const { return iter_ == end_; }
  int GetId() const { return iter_->first; }

 private:
  AttributeList::AttributeMap::const_iterator iter_;
  const AttributeList::AttributeMap::const_iterator end_;

  DISALLOW_COPY_AND_ASSIGN(AttributeIdIterator);
};

}  // namespace shill

#endif  // SHILL_NET_ATTRIBUTE_LIST_H_
