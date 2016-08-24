// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>

#include <string>

#include "media_v4l2_device.h"

void ExerciseControl(V4L2Device* v4l2_dev, uint32_t id, const char* control) {
  v4l2_queryctrl query_ctrl;
  if (v4l2_dev->QueryControl(id, &query_ctrl)) {
    if (!v4l2_dev->SetControl(id, query_ctrl.maximum))
      printf("[Warning] Can not set %s to maximum value\n", control);
    if (!v4l2_dev->SetControl(id, query_ctrl.minimum))
      printf("[Warning] Can not set %s to minimum value\n", control);
    if (!v4l2_dev->SetControl(id, query_ctrl.default_value))
      printf("[Warning] Can not set %s to default value\n", control);
  } else {
    printf("[Warning] Can not query control name :%s\n", control);
  }
}

void TestMultipleOpen(const char* dev_name, V4L2Device::IOMethod io) {
  V4L2Device v4l2_dev1(dev_name, io, 4);
  V4L2Device v4l2_dev2(dev_name, io, 4);
  if (!v4l2_dev1.OpenDevice()) {
    printf("[Error] Can not open device '%s' for the first time\n", dev_name);
  }
  if (!v4l2_dev2.OpenDevice()) {
    printf("[Error] Can not open device '%s' for the second time\n", dev_name);
    exit(EXIT_FAILURE);
  }
  v4l2_dev1.CloseDevice();
  v4l2_dev2.CloseDevice();
  printf("[OK ] V4L2DeviceTest.MultipleOpen\n");
}

void TestMultipleInit(const char* dev_name, V4L2Device::IOMethod io) {
  V4L2Device v4l2_dev1(dev_name, io, 4);
  V4L2Device v4l2_dev2(dev_name, io, 4);
  if (!v4l2_dev1.OpenDevice()) {
    printf("[Error] Can not open device '%s' for the first time\n", dev_name);
  }
  if (!v4l2_dev2.OpenDevice()) {
    printf("[Error] Can not open device '%s' for the second time\n", dev_name);
  }

  if (!v4l2_dev1.InitDevice(640, 480, V4L2_PIX_FMT_YUYV, 0)) {
    printf("[Error] Can not init device '%s' for the first time\n", dev_name);
  }

  // multiple streaming request should fail.
  if (v4l2_dev2.InitDevice(640, 480, V4L2_PIX_FMT_YUYV, 0)) {
    printf("[Error] Multiple init device '%s' should fail\n", dev_name);
    exit(EXIT_FAILURE);
  }

  v4l2_dev1.UninitDevice();
  v4l2_dev2.UninitDevice();
  v4l2_dev1.CloseDevice();
  v4l2_dev2.CloseDevice();
  printf("[OK ] V4L2DeviceTest.MultipleInit\n");
}

void TestEnumInputAndStandard(const char* dev_name, V4L2Device::IOMethod io) {
  V4L2Device v4l2_dev1(dev_name, io, 4);
  if (!v4l2_dev1.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
  }
  v4l2_dev1.EnumInput();
  v4l2_dev1.EnumStandard();
  v4l2_dev1.CloseDevice();
  printf("[OK ] V4L2DeviceTest.EnumInputAndStandard\n");
}

void TestEnumControl(const char* dev_name, V4L2Device::IOMethod io) {
  V4L2Device v4l2_dev(dev_name, io, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
  }
  v4l2_dev.EnumControl();
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.EnumControl\n");
}

void TestSetControl(const char* dev_name, V4L2Device::IOMethod io) {
  V4L2Device v4l2_dev(dev_name, io, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
  }
  ExerciseControl(&v4l2_dev, V4L2_CID_BRIGHTNESS, "brightness");
  ExerciseControl(&v4l2_dev, V4L2_CID_CONTRAST, "contrast");
  ExerciseControl(&v4l2_dev, V4L2_CID_SATURATION, "saturation");
  ExerciseControl(&v4l2_dev, V4L2_CID_GAMMA, "gamma");
  ExerciseControl(&v4l2_dev, V4L2_CID_HUE, "hue");
  ExerciseControl(&v4l2_dev, V4L2_CID_GAIN, "gain");
  ExerciseControl(&v4l2_dev, V4L2_CID_SHARPNESS, "sharpness");
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.SetControl\n");
}

void TestSetCrop(const char* dev_name, V4L2Device::IOMethod io) {
  V4L2Device v4l2_dev(dev_name, io, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
  }
  v4l2_cropcap cropcap;
  memset(&cropcap, 0, sizeof(cropcap));
  if (v4l2_dev.GetCropCap(&cropcap)) {
    v4l2_crop crop;
    memset(&crop, 0, sizeof(crop));
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect;
    v4l2_dev.SetCrop(&crop);
  }
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.SetCrop\n");
}

void TestGetCrop(const char* dev_name, V4L2Device::IOMethod io) {
  V4L2Device v4l2_dev(dev_name, io, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
  }
  v4l2_crop crop;
  memset(&crop, 0, sizeof(crop));
  crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  v4l2_dev.GetCrop(&crop);
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.GetCrop\n");
}

void TestProbeCaps(const char* dev_name, V4L2Device::IOMethod io) {
  V4L2Device v4l2_dev(dev_name, io, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
  }
  v4l2_capability caps;
  if (!v4l2_dev.ProbeCaps(&caps, true)) {
    printf("[Error] Can not probe caps on device '%s'\n", dev_name);
  }
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.ProbeCaps\n");
}

void TestEnumFormats(const char* dev_name, V4L2Device::IOMethod io) {
  V4L2Device v4l2_dev(dev_name, io, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
  }
  v4l2_dev.EnumFormat(NULL);
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.EnumFormats\n");
}

void TestEnumFrameSize(const char* dev_name, V4L2Device::IOMethod io) {
  V4L2Device v4l2_dev(dev_name, io, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
  }
  uint32_t format_count = 0;
  v4l2_dev.EnumFormat(&format_count);
  for (uint32_t i = 0; i < format_count; ++i) {
    uint32_t pixfmt = v4l2_dev.GetPixelFormat(i);
    if (pixfmt == 0xFFFFFFFF) {
      printf("[Error] Enumerate format error on device '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    }
    if (!v4l2_dev.EnumFrameSize(pixfmt)) {
      printf("[Warning] Enumerate frame size error on device '%s'\n", dev_name);
    };
  }
  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.EnumFrameSize\n");
}

void TestFrameRate(const char* dev_name, V4L2Device::IOMethod io) {
  V4L2Device v4l2_dev(dev_name, io, 4);
  if (!v4l2_dev.OpenDevice()) {
    printf("[Error] Can not open device '%s'\n", dev_name);
  }
  v4l2_capability caps;
  if (!v4l2_dev.ProbeCaps(&caps, true)) {
    printf("[Error] Can not probe caps on device '%s'\n", dev_name);
    exit(EXIT_FAILURE);
  }
  // we only try to adjust frame rate when it claims can.
  if (caps.capabilities & V4L2_CAP_TIMEPERFRAME) {
    v4l2_streamparm param;
    if (!v4l2_dev.GetParam(&param)) {
      printf("[Error] Can not get stream param on device '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    }
    if (!v4l2_dev.SetParam(&param)) {
      printf("[Error] Can not set stream param on device '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    }

    if (!v4l2_dev.SetFrameRate(15)) {
      printf("[Error] SetFrameRate failed on '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    }
    if (!v4l2_dev.GetParam(&param)) {
      printf("[Error] Can not get stream param on device '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    }
    if (param.parm.capture.timeperframe.denominator !=
              param.parm.capture.timeperframe.numerator * 15) {
      printf("[Error] Can not set frame rate to 15 on '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    }

    if (!v4l2_dev.SetFrameRate(10)) {
      printf("[Error] SetFrameRate failed on '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    }
    if (!v4l2_dev.GetParam(&param)) {
      printf("[Error] Can not get stream param on device '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    }
    if (param.parm.capture.timeperframe.denominator !=
              param.parm.capture.timeperframe.numerator * 10) {
      printf("[Error] Can not set frame rate to 10 on '%s'\n", dev_name);
      exit(EXIT_FAILURE);
    }
  }

  v4l2_dev.CloseDevice();
  printf("[OK ] V4L2DeviceTest.FrameRate\n");
}

static void PrintUsage() {
  printf("Usage: media_v4l2_unittest [options]\n\n"
         "Options:\n"
         "--device=DEVICE_NAME   Video device name [/dev/video]\n"
         "--help                 Print usage\n"
         "--buffer-io=mmap       Use memory mapped buffers\n"
         "--buffer-io=read       Use read() calls\n"
         "--buffer-io=userp      Use application allocated buffers\n");
}

static const char short_options[] = "d:?b";
static const struct option long_options[] = {
        { "device",       required_argument, NULL, 'd' },
        { "help",         no_argument,       NULL, '?' },
        { "buffer-io",    required_argument, NULL, 'b' },
};

int main(int argc, char** argv) {
  std::string dev_name = "/dev/video", io_name;
  V4L2Device::IOMethod io = V4L2Device::IO_METHOD_MMAP;

  // Parse the command line.
  for (;;) {
    int32_t index;
    int32_t c = getopt_long(argc, argv, short_options, long_options, &index);
    if (-1 == c)
      break;
    switch (c) {
      case 0:  // getopt_long() flag.
        break;
      case 'd':
        // Initialize default v4l2 device name.
        dev_name = strdup(optarg);
        break;
      case '?':
        PrintUsage();
        exit (EXIT_SUCCESS);
      case 'b':
        io_name = strdup(optarg);
        if (io_name == "mmap") {
          io = V4L2Device::IO_METHOD_MMAP;
        } else if (io_name == "read") {
          io = V4L2Device::IO_METHOD_READ;
        } else if (io_name == "userp") {
          io = V4L2Device::IO_METHOD_USERPTR;
        } else {
          PrintUsage();
          exit(EXIT_FAILURE);
        }
        break;
      default:
        PrintUsage();
        exit(EXIT_FAILURE);
    }
  }

  TestMultipleOpen(dev_name.c_str(), io);
  TestMultipleInit(dev_name.c_str(), io);
  TestEnumInputAndStandard(dev_name.c_str(), io);
  TestEnumControl(dev_name.c_str(), io);
  TestSetControl(dev_name.c_str(), io);
  TestSetCrop(dev_name.c_str(), io);
  TestGetCrop(dev_name.c_str(), io);
  TestProbeCaps(dev_name.c_str(), io);
  TestEnumFormats(dev_name.c_str(), io);
  TestEnumFrameSize(dev_name.c_str(), io);
  TestFrameRate(dev_name.c_str(), io);
}

