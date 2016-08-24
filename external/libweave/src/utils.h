// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_UTILS_H_
#define LIBWEAVE_SRC_UTILS_H_

#include <memory>
#include <string>

#include <base/values.h>
#include <weave/error.h>

namespace weave {

namespace errors {
extern const char kSchemaError[];
extern const char kInvalidCategoryError[];
extern const char kInvalidPackageError[];
}  // namespace errors

// kDefaultCategory represents a default state property category for standard
// properties from "base" package which are provided by buffet and not any of
// the daemons running on the device.
const char kDefaultCategory[] = "";

// Helper function to load a JSON dictionary from a string.
std::unique_ptr<base::DictionaryValue> LoadJsonDict(
    const std::string& json_string,
    ErrorPtr* error);

std::unique_ptr<base::DictionaryValue> ErrorInfoToJson(const Error& error);

}  // namespace weave

#endif  // LIBWEAVE_SRC_UTILS_H_
