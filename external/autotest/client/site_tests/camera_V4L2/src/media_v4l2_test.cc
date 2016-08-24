// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <getopt.h>

#include <string>

#include "media_v4l2_device.h"

static void PrintUsage(int argc, char** argv) {
  printf("Usage: %s [options]\n\n"
         "Options:\n"
         "--device=DEVICE_NAME    Video device name [/dev/video]\n"
         "--help                  Print usage\n"
         "--mmap                  Use memory mapped buffers\n"
         "--read                  Use read() calls\n"
         "--userp                 Use application allocated buffers\n"
         "--buffers=[NUM]         Minimum buffers required\n"
         "--frames=[NUM]          Maximum frame to capture\n"
         "--width=[NUM]           Picture width to capture\n"
         "--height=[NUM]          Picture height to capture\n"
         "--pixel-format=[fourcc] Picture format fourcc code\n"
         "--fps=[NUM]             Frame rate for capture\n"
         "--time=[NUM]            Time to capture in seconds\n",
         argv[0]);
}

static const char short_options[] = "d:?mrun:f:w:h:t:x:z:";
static const struct option
long_options[] = {
        { "device",       required_argument, NULL, 'd' },
        { "help",         no_argument,       NULL, '?' },
        { "mmap",         no_argument,       NULL, 'm' },
        { "read",         no_argument,       NULL, 'r' },
        { "userp",        no_argument,       NULL, 'u' },
        { "buffers",      required_argument, NULL, 'n' },
        { "frames",       required_argument, NULL, 'f' },
        { "width",        required_argument, NULL, 'w' },
        { "height",       required_argument, NULL, 'h' },
        { "pixel-format", required_argument, NULL, 't' },
        { "fps",          required_argument, NULL, 'x' },
        { "time",         required_argument, NULL, 'z' },
        { 0, 0, 0, 0 }
};

int main(int argc, char** argv) {
  std::string dev_name = "/dev/video";
  V4L2Device::IOMethod io = V4L2Device::IO_METHOD_MMAP;
  uint32_t buffers = 4;
  uint32_t frames = 100;
  uint32_t width = 640;
  uint32_t height = 480;
  uint32_t pixfmt = V4L2_PIX_FMT_YUYV;
  uint32_t fps = 0;
  uint32_t time_to_capture = 0;

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
        PrintUsage(argc, argv);
        exit (EXIT_SUCCESS);
      case 'm':
        io = V4L2Device::IO_METHOD_MMAP;
        break;
      case 'r':
        io = V4L2Device::IO_METHOD_READ;
        break;
      case 'u':
        io = V4L2Device::IO_METHOD_USERPTR;
        break;
      case 'n':
        buffers = atoi(optarg);
        break;
      case 'f':
        frames = atoi(optarg);
        break;
      case 'w':
        width = atoi(optarg);
        break;
      case 'h':
        height = atoi(optarg);
        break;
      case 't': {
        std::string fourcc = optarg;
        if (fourcc.length() != 4) {
          PrintUsage(argc, argv);
          exit (EXIT_FAILURE);
        }
        pixfmt = V4L2Device::MapFourCC(fourcc.c_str());
        break;
      }
      case 'x':
        fps = atoi(optarg);
        break;
      case 'z':
        time_to_capture = atoi(optarg);
        break;
      default:
        PrintUsage(argc, argv);
        exit(EXIT_FAILURE);
    }
  }

  if (time_to_capture) {
    printf("capture %dx%d %c%c%c%c picture for %d seconds at %d fps\n",
           width, height, (pixfmt >> 0) & 0xff, (pixfmt >> 8) & 0xff,
           (pixfmt >> 16) & 0xff, (pixfmt >> 24) & 0xff, time_to_capture, fps);
  } else {
    printf("capture %dx%d %c%c%c%c picture for %d frames at %d fps\n",
           width, height, (pixfmt >> 0) & 0xff, (pixfmt >> 8) & 0xff,
           (pixfmt >> 16) & 0xff, (pixfmt >> 24) & 0xff, frames, fps);
  }

  V4L2Device* device = new V4L2Device(dev_name.c_str(), io, buffers);

  int32_t retcode = 0;

  if (!device->OpenDevice())
    retcode = 1;

  if (!retcode && !device->InitDevice(width, height, pixfmt, fps))
    retcode = 2;

  if (!retcode && !device->StartCapture())
    retcode = 3;

  if (!retcode && !device->Run(frames, time_to_capture))
    retcode = 4;

  if (!retcode && !device->StopCapture())
    retcode = 5;

  if (!retcode && !device->UninitDevice())
    retcode = 6;

  device->CloseDevice();

  delete device;

  return retcode;
}

