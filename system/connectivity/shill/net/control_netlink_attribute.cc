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

#include "shill/net/control_netlink_attribute.h"

#include <linux/genetlink.h>

#include <utility>

#include <base/logging.h>

namespace shill {

const int ControlAttributeFamilyId::kName = CTRL_ATTR_FAMILY_ID;
const char ControlAttributeFamilyId::kNameString[] = "CTRL_ATTR_FAMILY_ID";

const int ControlAttributeFamilyName::kName = CTRL_ATTR_FAMILY_NAME;
const char ControlAttributeFamilyName::kNameString[] = "CTRL_ATTR_FAMILY_NAME";

const int ControlAttributeVersion::kName = CTRL_ATTR_VERSION;
const char ControlAttributeVersion::kNameString[] = "CTRL_ATTR_VERSION";

const int ControlAttributeHdrSize::kName = CTRL_ATTR_HDRSIZE;
const char ControlAttributeHdrSize::kNameString[] = "CTRL_ATTR_HDRSIZE";

const int ControlAttributeMaxAttr::kName = CTRL_ATTR_MAXATTR;
const char ControlAttributeMaxAttr::kNameString[] = "CTRL_ATTR_MAXATTR";

const int ControlAttributeAttrOps::kName = CTRL_ATTR_OPS;
const char ControlAttributeAttrOps::kNameString[] = "CTRL_ATTR_OPS";

ControlAttributeAttrOps::ControlAttributeAttrOps()
    : NetlinkNestedAttribute(kName, kNameString) {
  NestedData array(kTypeNested, "FIRST", true);
  array.deeper_nesting.insert(AttrDataPair(
      CTRL_ATTR_OP_UNSPEC, NestedData(kTypeU32, "CTRL_ATTR_OP_UNSPEC", false)));
  array.deeper_nesting.insert(AttrDataPair(
      CTRL_ATTR_OP_ID, NestedData(kTypeU32, "CTRL_ATTR_OP_ID", false)));
  array.deeper_nesting.insert(AttrDataPair(
      CTRL_ATTR_OP_UNSPEC, NestedData(kTypeU32, "CTRL_ATTR_OP_UNSPEC", false)));

  nested_template_.insert(AttrDataPair(kArrayAttrEnumVal, array));
}

const int ControlAttributeMcastGroups::kName = CTRL_ATTR_MCAST_GROUPS;
const char ControlAttributeMcastGroups::kNameString[] =
    "CTRL_ATTR_MCAST_GROUPS";

ControlAttributeMcastGroups::ControlAttributeMcastGroups()
    : NetlinkNestedAttribute(kName, kNameString) {
  NestedData array(kTypeNested, "FIRST", true);
  array.deeper_nesting.insert(
      AttrDataPair(CTRL_ATTR_MCAST_GRP_UNSPEC,
                   NestedData(kTypeU32, "CTRL_ATTR_MCAST_GRP_UNSPEC", false)));
  array.deeper_nesting.insert(
      AttrDataPair(CTRL_ATTR_MCAST_GRP_NAME,
                   NestedData(kTypeString, "CTRL_ATTR_MCAST_GRP_NAME", false)));
  array.deeper_nesting.insert(
      AttrDataPair(CTRL_ATTR_MCAST_GRP_ID,
                   NestedData(kTypeU32, "CTRL_ATTR_MCAST_GRP_ID", false)));

  nested_template_.insert(AttrDataPair(kArrayAttrEnumVal, array));
}

}  // namespace shill
