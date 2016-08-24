// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_V4L2_DEVICE_H_
#define MEDIA_V4L2_DEVICE_H_

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

class V4L2Device {
 public:
  enum IOMethod {
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
  };

  struct Buffer {
    void* start;
    size_t length;
  };

  V4L2Device(const char* dev_name,
             IOMethod io,
             uint32_t buffers);
  virtual ~V4L2Device() {}

  virtual bool OpenDevice();
  virtual void CloseDevice();
  virtual bool InitDevice(uint32_t width,
                          uint32_t height,
                          uint32_t pixfmt,
                          uint32_t fps);
  virtual bool UninitDevice();
  virtual bool StartCapture();
  virtual bool StopCapture();
  virtual bool Run(uint32_t frames, uint32_t time_in_sec = 0);

  // Helper methods.
  bool EnumInput();
  bool EnumStandard();
  bool EnumControl(bool show_menu = true);
  bool EnumControlMenu(const v4l2_queryctrl& query_ctrl);
  bool EnumFormat(uint32_t* num_formats, bool show_fmt = true);
  bool EnumFrameSize(uint32_t pixfmt, bool show_frmsize = true);

  bool QueryControl(uint32_t id, v4l2_queryctrl* ctrl);
  bool SetControl(uint32_t id, int32_t value);
  bool ProbeCaps(v4l2_capability* cap, bool show_caps = false);
  bool GetCropCap(v4l2_cropcap* cropcap);
  bool GetCrop(v4l2_crop* crop);
  bool SetCrop(v4l2_crop* crop);
  bool GetParam(v4l2_streamparm* param);
  bool SetParam(v4l2_streamparm* param);
  bool SetFrameRate(uint32_t fps);
  uint32_t GetPixelFormat(uint32_t index);
  uint32_t GetFrameRate();
  bool Stop();

  // Getter.
  int32_t GetActualWidth() {
    return width_;
  }

  int32_t GetActualHeight() {
    return height_;
  }

  v4l2_format& GetActualPixelFormat() {
    return pixfmt_;
  }

  static uint32_t MapFourCC(const char* fourcc);

  virtual void ProcessImage(const void* p);

 private:
  int32_t DoIoctl(int32_t request, void* arg);
  int32_t ReadOneFrame();
  bool InitReadIO(uint32_t buffer_size);
  bool InitMmapIO();
  bool InitUserPtrIO(uint32_t buffer_size);
  bool AllocateBuffer(uint32_t buffer_count);
  bool FreeBuffer();
  uint64_t Now();

  const char* dev_name_;
  IOMethod io_;
  int32_t fd_;
  Buffer* v4l2_buffers_;
  uint32_t num_buffers_;  // Actual buffers allocation.
  uint32_t min_buffers_;  // Minimum buffers requirement.
  bool stopped_;

  // Valid only after |InitDevice()|.
  uint32_t width_, height_;
  v4l2_format pixfmt_;
};

#endif  // MEDIA_V4L2_DEVICE_H_

