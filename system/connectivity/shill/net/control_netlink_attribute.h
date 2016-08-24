//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_NET_CONTROL_NETLINK_ATTRIBUTE_H_
#define SHILL_NET_CONTROL_NETLINK_ATTRIBUTE_H_

#include <base/macros.h>

#include "shill/net/netlink_attribute.h"

struct nlattr;

namespace shill {

// Control.

class ControlAttributeFamilyId : public NetlinkU16Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  ControlAttributeFamilyId() : NetlinkU16Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ControlAttributeFamilyId);
};

class ControlAttributeFamilyName : public NetlinkStringAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  ControlAttributeFamilyName() : NetlinkStringAttribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ControlAttributeFamilyName);
};

class ControlAttributeVersion : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  ControlAttributeVersion() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ControlAttributeVersion);
};

class ControlAttributeHdrSize : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  ControlAttributeHdrSize() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ControlAttributeHdrSize);
};

class ControlAttributeMaxAttr : public NetlinkU32Attribute {
 public:
  static const int kName;
  static const char kNameString[];
  ControlAttributeMaxAttr() : NetlinkU32Attribute(kName, kNameString) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ControlAttributeMaxAttr);
};

class ControlAttributeAttrOps : public NetlinkNestedAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  ControlAttributeAttrOps();

 private:
  DISALLOW_COPY_AND_ASSIGN(ControlAttributeAttrOps);
};

class ControlAttributeMcastGroups : public NetlinkNestedAttribute {
 public:
  static const int kName;
  static const char kNameString[];
  ControlAttributeMcastGroups();

 private:
  DISALLOW_COPY_AND_ASSIGN(ControlAttributeMcastGroups);
};

}  // namespace shill

#endif  // SHILL_NET_CONTROL_NETLINK_ATTRIBUTE_H_
