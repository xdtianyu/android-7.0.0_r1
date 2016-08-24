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

#ifndef SHILL_SCOPED_UMASK_H_
#define SHILL_SCOPED_UMASK_H_

#include <sys/types.h>

#include <base/macros.h>

namespace shill {

class ScopedUmask {
 public:
  explicit ScopedUmask(mode_t new_umask);
  ~ScopedUmask();

 private:
  mode_t saved_umask_;

  DISALLOW_COPY_AND_ASSIGN(ScopedUmask);
};

}  // namespace shill

#endif  // SHILL_SCOPED_UMASK_H_
