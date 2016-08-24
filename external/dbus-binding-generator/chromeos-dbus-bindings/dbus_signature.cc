// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/dbus_signature.h"

#include <base/logging.h>
#include <base/strings/stringprintf.h>
#include <brillo/strings/string_utils.h>
#include <dbus/dbus-protocol.h>

using base::StringPrintf;
using std::string;
using std::vector;

namespace chromeos_dbus_bindings {

// static
const char DbusSignature::kArrayTypename[] = "std::vector";
const char DbusSignature::kBooleanTypename[] = "bool";
const char DbusSignature::kByteTypename[] = "uint8_t";
const char DbusSignature::kDefaultObjectPathTypename[] = "dbus::ObjectPath";
const char DbusSignature::kDictTypename[] = "std::map";
const char DbusSignature::kDoubleTypename[] = "double";
const char DbusSignature::kSigned16Typename[] = "int16_t";
const char DbusSignature::kSigned32Typename[] = "int32_t";
const char DbusSignature::kSigned64Typename[] = "int64_t";
const char DbusSignature::kStringTypename[] = "std::string";
const char DbusSignature::kUnixFdTypename[] = "dbus::FileDescriptor";
const char DbusSignature::kUnsigned16Typename[] = "uint16_t";
const char DbusSignature::kUnsigned32Typename[] = "uint32_t";
const char DbusSignature::kUnsigned64Typename[] = "uint64_t";
const char DbusSignature::kVariantTypename[] = "brillo::Any";
const char DbusSignature::kVariantDictTypename[] = "brillo::VariantDictionary";
const char DbusSignature::kTupleTypename[] = "std::tuple";

DbusSignature::DbusSignature()
    : object_path_typename_(kDefaultObjectPathTypename) {}

bool DbusSignature::Parse(const string& signature, string* output) {
  string::const_iterator end;
  if (!GetTypenameForSignature(
          signature.begin(), signature.end(), &end, output)) {
    LOG(ERROR) << "Parse failed for signature " << signature;
    return false;
  }
  if (end != signature.end()) {
    LOG(WARNING) << "A portion of signature " << signature
                 << " is left unparsed: " << string(end, signature.end());
  }
  return true;
}

bool DbusSignature::GetTypenameForSignature(
    string::const_iterator signature,
    string::const_iterator end,
    string::const_iterator* next,
    string* output) {
  if (signature == end) {
    LOG(ERROR) << "Signature is empty";
    return false;
  }

  string::const_iterator cur = signature;
  int signature_value = *cur++;
  switch (signature_value) {
    case DBUS_STRUCT_BEGIN_CHAR:
      if (!GetStructTypenameForSignature(cur, end, &cur, output)) {
        return false;
      }
      break;

    case DBUS_TYPE_ARRAY:
      if (!GetArrayTypenameForSignature(cur, end, &cur, output)) {
        return false;
      }
      break;

    case DBUS_TYPE_BOOLEAN:
      *output = kBooleanTypename;
      break;

    case DBUS_TYPE_BYTE:
      *output = kByteTypename;
      break;

    case DBUS_TYPE_DOUBLE:
      *output = kDoubleTypename;
      break;

    case DBUS_TYPE_OBJECT_PATH:
      *output = object_path_typename_;
      break;

    case DBUS_TYPE_INT16:
      *output = kSigned16Typename;
      break;

    case DBUS_TYPE_INT32:
      *output = kSigned32Typename;
      break;

    case DBUS_TYPE_INT64:
      *output = kSigned64Typename;
      break;

    case DBUS_TYPE_STRING:
      *output = kStringTypename;
      break;

    case DBUS_TYPE_UNIX_FD:
      *output = kUnixFdTypename;
      break;

    case DBUS_TYPE_UINT16:
      *output = kUnsigned16Typename;
      break;

    case DBUS_TYPE_UINT32:
      *output = kUnsigned32Typename;
      break;

    case DBUS_TYPE_UINT64:
      *output = kUnsigned64Typename;
      break;

    case DBUS_TYPE_VARIANT:
      *output = kVariantTypename;
      break;

    default:
      LOG(ERROR) << "Unexpected token " << *signature;
      return false;
  }

  if (next) {
    *next = cur;
  }

  return true;
}

bool DbusSignature::GetArrayTypenameForSignature(
    string::const_iterator signature,
    string::const_iterator end,
    string::const_iterator* next,
    string* output) {
  string::const_iterator cur = signature;
  if (cur == end) {
    LOG(ERROR) << "At end of string while reading array parameter";
    return false;
  }

  if (*cur == DBUS_DICT_ENTRY_BEGIN_CHAR) {
    vector<string> children;
    ++cur;
    while (cur != end && *cur != DBUS_DICT_ENTRY_END_CHAR) {
      children.emplace_back();
      if (!GetTypenameForSignature(cur, end, &cur, &children.back())) {
        LOG(ERROR) << "Unable to decode child elements starting at "
                   << string(cur, end);
        return false;
      }
    }
    if (cur == end) {
      LOG(ERROR) << "At end of string while processing dict "
                 << "starting at " << string(signature, end);
      return false;
    }

    DCHECK_EQ(DBUS_DICT_ENTRY_END_CHAR, *cur);
    ++cur;

    if (children.size() != 2) {
      LOG(ERROR) << "Dict entry contains " << children.size()
                 << " members starting at " << string(signature, end)
                 << " where only 2 children is valid.";
      return false;
    }
    string dict_signature{signature, cur};
    if (dict_signature == "{sv}") {
      *output = kVariantDictTypename;
    } else {
      *output = StringPrintf("%s<%s, %s>", kDictTypename,
                             children[0].c_str(), children[1].c_str());
    }
  } else {
    string child;
    if (!GetTypenameForSignature(cur, end, &cur, &child)) {
      LOG(ERROR) << "Unable to decode child element starting at "
                 << string(cur, end);
      return false;
    }
    *output = StringPrintf("%s<%s>", kArrayTypename, child.c_str());
  }

  if (next) {
    *next = cur;
  }

  return true;
}

bool DbusSignature::GetStructTypenameForSignature(
    string::const_iterator signature,
    string::const_iterator end,
    string::const_iterator* next,
    string* output) {
  string::const_iterator cur = signature;
  if (cur == end) {
    LOG(ERROR) << "At end of string while reading struct parameter";
    return false;
  }

  vector<string> children;
  while (cur != end && *cur != DBUS_STRUCT_END_CHAR) {
    children.emplace_back();
    if (!GetTypenameForSignature(cur, end, &cur, &children.back())) {
      LOG(ERROR) << "Unable to decode child elements starting at "
                 << string(cur, end);
      return false;
    }
  }
  if (cur == end) {
    LOG(ERROR) << "At end of string while processing struct "
               << "starting at " << string(signature, end);
    return false;
  }

  DCHECK_EQ(DBUS_STRUCT_END_CHAR, *cur);
  ++cur;

  *output = StringPrintf("%s<%s>", kTupleTypename,
                         brillo::string_utils::Join(", ", children).c_str());

  if (next) {
    *next = cur;
  }

  return true;
}

}  // namespace chromeos_dbus_bindings
