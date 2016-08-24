// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

extern int DefeatTailOptimizationForBomb();

__attribute__ ((noinline)) int recbomb(int n) {
  if (n < 2) {
    *(char*)0x16 = 0;
    return 1;
  }
  return recbomb(n - 1) + DefeatTailOptimizationForBomb();
}

int DefeatTailOptimizationForCrasher() {
  return 0;
}
