// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media_v4l2_device.h"


// Checks whether /dev/videoX is a video capture device. Return value 0 means
// it is a capture device. 1 otherwise.
int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: media_v4l2_is_capture_device /dev/videoX\n");
    return 1;
  }

  V4L2Device v4l2_dev(argv[1], V4L2Device::IO_METHOD_MMAP, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", argv[1]);
    return 1;
  }

  bool is_capture_device = false;
  v4l2_capability caps;
  if (!v4l2_dev.ProbeCaps(&caps, false)) {
    printf("[Error] Can not probe caps on device '%s'\n", argv[1]);
  } else {
    // mem2mem devices have V4L2_CAP_VIDEO_OUTPUT but real cameras do not.
    is_capture_device = ((caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) &&
        !(caps.capabilities & V4L2_CAP_VIDEO_OUTPUT));
  }
  v4l2_dev.CloseDevice();

  return is_capture_device ? 0 : 1;
}
