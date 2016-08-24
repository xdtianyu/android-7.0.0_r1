// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_MINIJAIL_MOCK_MINIJAIL_H_
#define LIBBRILLO_BRILLO_MINIJAIL_MOCK_MINIJAIL_H_

#include <vector>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "brillo/minijail/minijail.h"

namespace brillo {

class MockMinijail : public brillo::Minijail {
 public:
  MockMinijail() {}
  virtual ~MockMinijail() {}

  MOCK_METHOD0(New, struct minijail*());
  MOCK_METHOD1(Destroy, void(struct minijail*));

  MOCK_METHOD3(DropRoot,
               bool(struct minijail* jail,
                    const char* user,
                    const char* group));
  MOCK_METHOD2(UseSeccompFilter, void(struct minijail* jail, const char* path));
  MOCK_METHOD2(UseCapabilities, void(struct minijail* jail, uint64_t capmask));
  MOCK_METHOD1(ResetSignalMask, void(struct minijail* jail));
  MOCK_METHOD1(Enter, void(struct minijail* jail));
  MOCK_METHOD3(Run,
               bool(struct minijail* jail,
                    std::vector<char*> args,
                    pid_t* pid));
  MOCK_METHOD3(RunSync,
               bool(struct minijail* jail,
                    std::vector<char*> args,
                    int* status));
  MOCK_METHOD3(RunAndDestroy,
               bool(struct minijail* jail,
                    std::vector<char*> args,
                    pid_t* pid));
  MOCK_METHOD3(RunSyncAndDestroy,
               bool(struct minijail* jail,
                    std::vector<char*> args,
                    int* status));
  MOCK_METHOD4(RunPipeAndDestroy,
               bool(struct minijail* jail,
                    std::vector<char*> args,
                    pid_t* pid,
                    int* stdin));
  MOCK_METHOD6(RunPipesAndDestroy,
               bool(struct minijail* jail,
                    std::vector<char*> args,
                    pid_t* pid,
                    int* stdin,
                    int* stdout,
                    int* stderr));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockMinijail);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_MINIJAIL_MOCK_MINIJAIL_H_
