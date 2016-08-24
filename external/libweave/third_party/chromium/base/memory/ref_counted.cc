// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ref_counted.h"

namespace base {

namespace subtle {

bool RefCountedThreadSafeBase::HasOneRef() const {
  return ref_count_ == 1;
}

RefCountedThreadSafeBase::RefCountedThreadSafeBase() : ref_count_(0) {
#ifndef NDEBUG
  in_dtor_ = false;
#endif
}

RefCountedThreadSafeBase::~RefCountedThreadSafeBase() {
#ifndef NDEBUG
  DCHECK(in_dtor_) << "RefCountedThreadSafe object deleted without "
                      "calling Release()";
#endif
}

void RefCountedThreadSafeBase::AddRef() const {
#ifndef NDEBUG
  DCHECK(!in_dtor_);
#endif
  ++ref_count_;
}

bool RefCountedThreadSafeBase::Release() const {
#ifndef NDEBUG
  DCHECK(!in_dtor_);
  DCHECK(ref_count_ != 0);
#endif
  if (--ref_count_ == 0) {
#ifndef NDEBUG
    in_dtor_ = true;
#endif
    return true;
  }
  return false;
}

}  // namespace subtle

}  // namespace base
