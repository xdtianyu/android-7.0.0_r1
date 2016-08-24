// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/any.h>

#include <algorithm>

namespace brillo {

Any::Any() {
}

Any::Any(const Any& rhs) : data_buffer_(rhs.data_buffer_) {
}

// NOLINTNEXTLINE(build/c++11)
Any::Any(Any&& rhs) : data_buffer_(std::move(rhs.data_buffer_)) {
}

Any::~Any() {
}

Any& Any::operator=(const Any& rhs) {
  data_buffer_ = rhs.data_buffer_;
  return *this;
}

// NOLINTNEXTLINE(build/c++11)
Any& Any::operator=(Any&& rhs) {
  data_buffer_ = std::move(rhs.data_buffer_);
  return *this;
}

bool Any::operator==(const Any& rhs) const {
  // Make sure both objects contain data of the same type.
  if (strcmp(GetTypeTagInternal(), rhs.GetTypeTagInternal()) != 0)
    return false;

  if (IsEmpty())
    return true;

  return data_buffer_.GetDataPtr()->CompareEqual(rhs.data_buffer_.GetDataPtr());
}

const char* Any::GetTypeTagInternal() const {
  if (!IsEmpty())
    return data_buffer_.GetDataPtr()->GetTypeTag();

  return "";
}

void Any::Swap(Any& other) {
  std::swap(data_buffer_, other.data_buffer_);
}

bool Any::IsEmpty() const {
  return data_buffer_.IsEmpty();
}

void Any::Clear() {
  data_buffer_.Clear();
}

bool Any::IsConvertibleToInteger() const {
  return !IsEmpty() && data_buffer_.GetDataPtr()->IsConvertibleToInteger();
}

intmax_t Any::GetAsInteger() const {
  CHECK(!IsEmpty()) << "Must not be called on an empty Any";
  return data_buffer_.GetDataPtr()->GetAsInteger();
}

void Any::AppendToDBusMessageWriter(dbus::MessageWriter* writer) const {
  CHECK(!IsEmpty()) << "Must not be called on an empty Any";
  data_buffer_.GetDataPtr()->AppendToDBusMessage(writer);
}

}  // namespace brillo
