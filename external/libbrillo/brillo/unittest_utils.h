// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Internal implementation of brillo::Any class.

#ifndef LIBBRILLO_BRILLO_UNITTEST_UTILS_H_
#define LIBBRILLO_BRILLO_UNITTEST_UTILS_H_

namespace brillo {

// Helper class to create and close a unidirectional pipe. The file descriptors
// will be closed on destruction, unless set to -1.
class ScopedPipe {
 public:
  // The internal pipe size.
  static const int kPipeSize;

  ScopedPipe();
  ~ScopedPipe();

  // The reader and writer end of the pipe.
  int reader{-1};
  int writer{-1};
};

// Helper class to create and close a bi-directional pair of sockets. The
// sockets will be closed on destruction, unless set to -1.
class ScopedSocketPair {
 public:
  ScopedSocketPair();
  ~ScopedSocketPair();

  // The left and right sockets are bi-directional connected and
  // indistinguishable file descriptor. We named them left/right for easier
  // reading.
  int left{-1};
  int right{-1};
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_UNITTEST_UTILS_H_
