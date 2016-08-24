// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media_v4l2_device.h"

#include <assert.h>
#include <time.h>
#include <sys/stat.h>

#define CHECK(a) assert(a)
#define MAJOR(dev) (((uint32_t)(dev)) >> 8)
#define MINOR(dev) (((uint32_t)(dev)) & 0xff)
#define V4L2_VIDEO_CAPTURE_MAJOR      81
#define V4L2_VIDEO_CAPTURE_MINOR_MIN  0
#define V4L2_VIDEO_CAPTURE_MINOR_MAX  64

V4L2Device::V4L2Device(const char* dev_name,
                       IOMethod io,
                       uint32_t buffers)
    : dev_name_(dev_name),
      io_(io),
      fd_(-1),
      v4l2_buffers_(NULL),
      num_buffers_(0),
      min_buffers_(buffers),
      stopped_(false) {
}

bool V4L2Device::OpenDevice() {
  struct stat st;
  if (-1 == stat(dev_name_, &st)) {
    printf("<<< Error: could not find v4l2 device %s: (%d) %s.>>>\n",
           dev_name_, errno, strerror(errno));
    return false;
  }

  if (!S_ISCHR(st.st_mode)) {
    printf("<<< Error: specified v4l2 device %s is not char device.>>>\n",
           dev_name_);
    return false;
  }

  if (MAJOR(st.st_rdev) != V4L2_VIDEO_CAPTURE_MAJOR
      || MINOR(st.st_rdev) >= V4L2_VIDEO_CAPTURE_MINOR_MAX) {
    printf("<<< Error: specified v4l2 device %s is not v4l2 device.>>>\n",
           dev_name_);
    return false;
  }

  fd_ = open(dev_name_, O_RDWR | O_NONBLOCK, 0);
  if (-1 == fd_) {
    printf("<<< Error: specified v4l2 device %s could not be opened.>>>\n",
           dev_name_);
    return false;
  }

  v4l2_capability cap;
  if (!ProbeCaps(&cap))
    return false;

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    printf("<<< Error: %s does not support video capture.>>>\n", dev_name_);
    return false;
  }

  switch (io_) {
    case IO_METHOD_READ:
      if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
        printf("<<< Error: %s does not support read i/o.>>>\n", dev_name_);
        return false;
      }
      break;
    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
      if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        printf("<<< Error: %s does not support streaming.>>>\n", dev_name_);
        return false;
      }
      break;
  }

  return true;
}

void V4L2Device::CloseDevice() {
  if (fd_ != -1)
    close(fd_);
  fd_ = -1;
}

bool V4L2Device::InitDevice(uint32_t width,
                            uint32_t height,
                            uint32_t pixfmt,
                            uint32_t fps) {
  // Crop/Format setting could live across session.
  // We should always initialized them when supported.
  v4l2_cropcap cropcap;
  memset(&cropcap, 0, sizeof(cropcap));
  if (GetCropCap(&cropcap)) {
    v4l2_crop crop;
    memset(&crop, 0, sizeof(crop));
    // Use default capture rectangle.
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect;
    SetCrop(&crop);
  }

  v4l2_format fmt;
  memset(&fmt, 0, sizeof(fmt));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (-1 == DoIoctl(VIDIOC_G_FMT, &fmt)) {
    printf("<<< Error: VIDIOC_G_FMT on %s.>>>\n", dev_name_);
    return false;
  }

  fmt.fmt.pix.width = width;
  fmt.fmt.pix.height = height;
  fmt.fmt.pix.pixelformat = pixfmt;
  fmt.fmt.pix.field = V4L2_FIELD_NONE;

  if (-1 == DoIoctl(VIDIOC_S_FMT, &fmt)) {
    printf("<<< Error: VIDIOC_S_FMT on %s.>>>\n", dev_name_);
    return false;
  }

  v4l2_capability cap;
  if (!ProbeCaps(&cap))
    return false;

  if (cap.capabilities & V4L2_CAP_TIMEPERFRAME) {
    if (fps > 0)
      SetFrameRate(fps);
    fps = GetFrameRate();
  } else {
    // TODO(jiesun): probably we should derive this from VIDIOC_G_STD
    fps = 30;
  }

  printf("actual format for capture %dx%d %c%c%c%c picture at %d fps\n",
         fmt.fmt.pix.width, fmt.fmt.pix.height,
         (pixfmt >> 0) & 0xff, (pixfmt >> 8) & 0xff,
         (pixfmt >> 16) & 0xff, (pixfmt >> 24 ) & 0xff, fps);
  width_ = fmt.fmt.pix.width;
  height_ = fmt.fmt.pix.height;
  pixfmt_ = fmt;

  switch (io_) {
    case IO_METHOD_READ:
      return InitReadIO(fmt.fmt.pix.sizeimage);
    case IO_METHOD_MMAP:
      return InitMmapIO();
    case IO_METHOD_USERPTR:
      return InitUserPtrIO(fmt.fmt.pix.sizeimage);
  }
  return false;
}

bool V4L2Device::UninitDevice() {
  switch (io_) {
    case IO_METHOD_READ:
      // Only one buffer for read() i/o.
      free(v4l2_buffers_[0].start);
      break;
    case IO_METHOD_MMAP:
      for (uint32_t i = 0; i < num_buffers_; ++i)
        if (-1 == munmap(v4l2_buffers_[i].start, v4l2_buffers_[i].length)) {
          printf("<<< Error: munmap() on %s failed.>>>\n", dev_name_);
          return false;
        }
      break;
    case IO_METHOD_USERPTR:
      for (uint32_t i = 0; i < num_buffers_; ++i)
        free(v4l2_buffers_[i].start);
      break;
  }
  FreeBuffer();
  return true;
}

bool V4L2Device::StartCapture() {
  v4l2_buffer buf;
  uint32_t i;
  v4l2_buf_type type;
  switch (io_) {
    case IO_METHOD_READ:
      // Nothing to do.
      break;
    case IO_METHOD_MMAP:
      for (i = 0; i < num_buffers_; ++i) {
        memset(&buf, 0, sizeof(buf));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;
        if (-1 == DoIoctl(VIDIOC_QBUF, &buf)) {
          printf("<<< Error: VIDIOC_QBUF on %s.>>>\n", dev_name_);
          return false;
        }
      }
      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (-1 == DoIoctl(VIDIOC_STREAMON, &type)) {
        printf("<<< Error: VIDIOC_STREAMON on %s.>>>\n", dev_name_);
        return false;
      }
      break;
    case IO_METHOD_USERPTR:
      for (i = 0; i < num_buffers_; ++i) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.index = i;
        buf.m.userptr = (unsigned long) v4l2_buffers_[i].start;
        buf.length = v4l2_buffers_[i].length;
        if (-1 == DoIoctl(VIDIOC_QBUF, &buf)) {
          printf("<<< Error: VIDIOC_QBUF on %s.>>>\n", dev_name_);
          return false;
        }
      }
      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (-1 == DoIoctl(VIDIOC_STREAMON, &type)) {
        printf("<<< Error: VIDIOC_STREAMON on %s.>>>\n", dev_name_);
        return false;
      }
      break;
  }
  return true;
}

bool V4L2Device::StopCapture() {
  v4l2_buf_type type;
  switch (io_) {
    case IO_METHOD_READ:
      // Nothing to do.
      break;
    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
      type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      if (-1 == DoIoctl(VIDIOC_STREAMOFF, &type)) {
        printf("<<< Error: VIDIOC_STREAMOFF on %s.>>>\n", dev_name_);
        return false;
      }
      break;
  }
  return true;
}

void V4L2Device::ProcessImage(const void* p) {
  printf(".");
  fflush(stdout);
}

// Do capture for number of |frames| ( when time_in_sec == 0 )
// or for duration of |time_in_sec|  ( when time_in_sec > 0 ).
bool V4L2Device::Run(uint32_t frames, uint32_t time_in_sec) {
  stopped_ = false;
  if (time_in_sec) // duration setting override the frames setting.
    frames = 30 * time_in_sec; // Assume maximum fps is 30.

  uint64_t start_in_sec = Now();
  int32_t timeout = 5;  // Used 5 seconds for initial delay.
  while (!stopped_ && frames > 0) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd_, &fds);
    timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    timeout = 2;  // Normal timeout will be 2 seconds.
    int32_t r = select(fd_ + 1, &fds, NULL, NULL, &tv);
    if (-1 == r) {
      if (EINTR == errno)  // If interrupted, continue.
        continue;
      printf("<<< Error: select() failed on %s.>>>\n", dev_name_);
      return false;
    }
    if (0 == r) {
      printf("<<< Error: select() timeout on %s.>>>\n", dev_name_);
      return false;
    }
    r = ReadOneFrame();
    if (r < 0)
      return false;
    if (r)
      frames--;
    if (time_in_sec) {
      uint64_t end_in_sec = Now();
      if ( end_in_sec - start_in_sec >= time_in_sec )
        return true;
    }
  }
  return true;
}

bool V4L2Device::Stop() {
  stopped_ = true;
}

int32_t V4L2Device::DoIoctl(int32_t request, void* arg) {
  int32_t r;
  do {
    r = ioctl(fd_, request, arg);
  } while (-1 == r && EINTR == errno);
  return r;
}

// return 1 : successful to retrieve a frame from device
// return 0 : EAGAIN
// negative : error
int32_t V4L2Device::ReadOneFrame() {
  v4l2_buffer buf;
  memset(&buf, 0, sizeof(buf));
  uint32_t i;
  switch (io_) {
    case IO_METHOD_READ:
      if (-1 == read(fd_, v4l2_buffers_[0].start, v4l2_buffers_[0].length)) {
        switch (errno) {
          case EAGAIN:
            return 0;
          case EIO:
            // Could ignore EIO, see spec.
            // Fall through.
          default:
            printf("<<< Error: read() failed on %s.>>>\n", dev_name_);
            return -1;
        }
      }
      ProcessImage(v4l2_buffers_[0].start);
      break;
    case IO_METHOD_MMAP:
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      if (-1 == DoIoctl(VIDIOC_DQBUF, &buf)) {
        switch (errno) {
          case EAGAIN:
            return 0;
          case EIO:
            // Could ignore EIO, see spec.
            // Fall through.
          default:
            printf("<<< Error: VIDIOC_DQBUF failed on %s.>>>\n", dev_name_);
            return -2;
        }
      }
      CHECK(buf.index < num_buffers_);
      // TODO: uvcvideo driver ignores this field. This is negligible,
      // so disabling this for now until we get a fix into the upstream driver.
      // CHECK(buf.field == V4L2_FIELD_NONE);  // progressive only.
      ProcessImage(v4l2_buffers_[buf.index].start);
      if (-1 == DoIoctl(VIDIOC_QBUF, &buf)) {
        printf("<<< Error: VIDIOC_QBUF failed on %s.>>>\n", dev_name_);
        return -3;
      }
      break;
    case IO_METHOD_USERPTR:
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_USERPTR;
      if (-1 == DoIoctl(VIDIOC_DQBUF, &buf)) {
        switch (errno) {
          case EAGAIN:
            return 0;
          case EIO:
            // Could ignore EIO, see spec.
            // Fall through.
          default:
            printf("<<< Error: VIDIOC_DQBUF failed on %s.>>>\n", dev_name_);
            return -2;
        }
      }
      for (i = 0; i < num_buffers_; ++i) {
        if (buf.m.userptr == (unsigned long) v4l2_buffers_[i].start
            && buf.length == v4l2_buffers_[i].length)
          break;
      }
      CHECK(i < num_buffers_);
      ProcessImage(reinterpret_cast<void*>(buf.m.userptr));
      if (-1 == DoIoctl(VIDIOC_QBUF, &buf)) {
        printf("<<< Error: VIDIOC_QBUF failed on %s.>>>\n", dev_name_);
        return -3;
      }
      break;
  }
  return 1;
}

bool V4L2Device::AllocateBuffer(uint32_t buffer_count) {
  v4l2_buffers_ = new Buffer[buffer_count];
  if (!v4l2_buffers_) {
    printf("<<< Error: Out of memory.>>>\n");
    return false;
  }
  return true;
}

bool V4L2Device::FreeBuffer() {
  free(v4l2_buffers_);
  v4l2_buffers_ = NULL;
  return true;
}

bool V4L2Device::InitReadIO(uint32_t buffer_size) {
  if (!AllocateBuffer(1))
    return false;
  v4l2_buffers_[0].length = buffer_size;
  v4l2_buffers_[0].start = new uint8_t[buffer_size];
  if (!v4l2_buffers_[0].start) {
    printf("<<< Error: Out of memory.>>>\n");
    return false;
  }
  return true;
}

bool V4L2Device::InitMmapIO() {
  v4l2_requestbuffers req;
  memset(&req, 0, sizeof(req));
  req.count = min_buffers_;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  if (-1 == DoIoctl(VIDIOC_REQBUFS, &req)) {
    if (EINVAL == errno)
      printf("<<< Error: mmap() io is not supported on %s.>>>\n", dev_name_);
    else
      printf("<<< Error: VIDIOC_REQBUFS failed on %s.>>>\n", dev_name_);
    return false;
  }

  if (req.count < min_buffers_) {
    printf("<<< Error: Insufficient buffer memory on %s >>>\n",
            dev_name_);  // TODO(jiesun) :add flexibilities.
    return false;
  }

  if (!AllocateBuffer(req.count))
    return false;

  for (num_buffers_ = 0; num_buffers_ < req.count; ++num_buffers_) {
    v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = num_buffers_;
    if (-1 == DoIoctl(VIDIOC_QUERYBUF, &buf)) {
      printf("<<< Error: VIDIOC_QUERYBUF failed on %s.>>>\n", dev_name_);
      return false;
    }
    v4l2_buffers_[num_buffers_].length = buf.length;
    v4l2_buffers_[num_buffers_].start =
        mmap(NULL,  // Start anywhere.
             buf.length,
             PROT_READ | PROT_WRITE,
             MAP_SHARED,
             fd_, buf.m.offset);
    if (MAP_FAILED == v4l2_buffers_[num_buffers_].start) {
      printf("<<< Error: mmap() failed on %s.>>>\n", dev_name_);
      return false;
    }
  }
  return true;
}

bool V4L2Device::InitUserPtrIO(uint32_t buffer_size) {
  v4l2_requestbuffers req;
  memset(&req, 0, sizeof(req));
  req.count = min_buffers_;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;

  // Align up buffer_size to page size boundary.
  uint32_t page_size = getpagesize();
  buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);
  if (-1 == DoIoctl(VIDIOC_REQBUFS, &req)) {
    if (EINVAL == errno)
      printf("<<< Error: user pointer is not supported on %s.>>>\n", dev_name_);
    else
      printf("<<< Error: VIDIOC_REQBUFS failed on %s.>>>\n", dev_name_);
    return false;
  }

  if (!AllocateBuffer(4))
    return false;

  for (num_buffers_ = 0; num_buffers_ < min_buffers_; ++num_buffers_) {
    v4l2_buffers_[num_buffers_].length = buffer_size;
    v4l2_buffers_[num_buffers_].start = memalign(page_size, buffer_size);
    if (!v4l2_buffers_[num_buffers_].start) {
      printf("<<< Error: Out of memory.>>>\n");
      return false;
    }
  }
  return true;
}

bool V4L2Device::EnumInput() {
  v4l2_input input;
  int32_t index;
  if (-1 == DoIoctl(VIDIOC_G_INPUT, &index)) {
    printf("<<< Info: VIDIOC_G_INPUT not supported.>>>\n");
    return false;
  }

  for (int32_t i = 0 ; ; ++i) {
    memset(&input, 0, sizeof(input));
    input.index = i;
    if (-1 == DoIoctl(VIDIOC_ENUMINPUT, &input)) {
      if (i == 0) {
        printf("<<< Info: VIDIOC_ENUMINPUT not supported.>>>\n");
        return false;
      } else {
        break;
      }
    }
    printf("Current input: %s %s\n", input.name, i == index ? "*" : "");
  }
  return true;
}

bool V4L2Device::EnumStandard() {
  v4l2_input input;
  v4l2_standard standard;
  memset(&input, 0, sizeof(input));
  if (-1 == DoIoctl(VIDIOC_G_INPUT, &input.index)) {
    printf("<<< Info: VIDIOC_G_INPUT not supported.>>>\n");
    return false;
  }

  if (-1 == DoIoctl(VIDIOC_ENUMINPUT, &input)) {
    printf("<<< Info: VIDIOC_ENUMINPUT not supported.>>>\n");
    return false;
  }

  printf("Current input %s supports:\n", input.name);
  memset(&standard, 0, sizeof(standard));
  standard.index = 0;
  while (0 == DoIoctl(VIDIOC_ENUMSTD, &standard)) {
    if (standard.id & input.std)
      printf("%s\n", standard.name);
    standard.index++;
  }
  // EINVAL indicates the end of the enumeration, which cannot be
  // empty unless this device falls under the USB exception.
  if (errno != EINVAL || standard.index == 0) {
    printf("<<< Info: VIDIOC_ENUMSTD not supported.>>>\n");
    return false;
  }
  return true;
}

bool V4L2Device::EnumControl(bool show_menu) {
  v4l2_queryctrl query_ctrl;
  memset(&query_ctrl, 0, sizeof(query_ctrl));
  for (query_ctrl.id = V4L2_CID_BASE;
       query_ctrl.id < V4L2_CID_LASTP1;
       ++query_ctrl.id) {
    if (0 == DoIoctl(VIDIOC_QUERYCTRL, &query_ctrl)) {
      if (query_ctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
          printf("Control %s is disabled\n", query_ctrl.name);
      } else {
          printf("Control %s is enabled(%d-%d:%d)\n",
                 query_ctrl.name, query_ctrl.minimum,
                 query_ctrl.maximum, query_ctrl.default_value);
      }
      if (query_ctrl.type == V4L2_CTRL_TYPE_MENU && show_menu)
        EnumControlMenu(query_ctrl);
    } else if (errno != EINVAL) {
      printf("<<< Info: VIDIOC_query_ctrl not supported.>>>\n");
      return false;
    }
  }

  for (query_ctrl.id = V4L2_CID_PRIVATE_BASE;; query_ctrl.id++) {
    if (0 == DoIoctl(VIDIOC_QUERYCTRL, &query_ctrl)) {
      if (query_ctrl.flags & V4L2_CTRL_FLAG_DISABLED)
        printf("Private Control %s is disabled\n", query_ctrl.name);
      else
        printf("Private Control %s is enabled\n", query_ctrl.name);
      if (query_ctrl.type == V4L2_CTRL_TYPE_MENU && show_menu)
        EnumControlMenu(query_ctrl);
    } else {
      // Assume private control ids are contiguous.
      if (errno == EINVAL)
        break;
      printf("<<< Info: VIDIOC_query_ctrl not supported.>>>\n");
      return false;
    }
  }
  return true;
}

bool V4L2Device::EnumControlMenu(const v4l2_queryctrl& query_ctrl) {
  v4l2_querymenu query_menu;
  memset(&query_menu, 0, sizeof(query_menu));
  printf("\t\tMenu items:\n");
  query_menu.id = query_ctrl.id;
  for (query_menu.index = query_ctrl.minimum;
       query_menu.index <= query_ctrl.maximum;
       ++query_menu.index) {
    if (0 == DoIoctl(VIDIOC_QUERYMENU, &query_menu)) {
      printf("\t\t\t%s\n", query_menu.name);
    } else {
      printf("<<< Info: VIDIOC_QUERYMENU not supported.>>>\n");
      return false;
    }
  }
  return true;
}

bool V4L2Device::EnumFormat(uint32_t* num_formats, bool show_fmt) {
  uint32_t i;
  for (i = 0; ; ++i) {
    v4l2_fmtdesc format_desc;
    memset(&format_desc, 0, sizeof(format_desc));
    format_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format_desc.index = i;
    if (-1 == DoIoctl(VIDIOC_ENUM_FMT, &format_desc)) {
      if (i == 0) {
          printf("<<< Info: VIDIOC_ENUM_FMT not supported.>>>\n");
          return false;
      } else {
          break;
      }
    }
    if (show_fmt)
      printf("<<< Info supported format #%d: %s (%c%c%c%c) >>>\n",
             i+1, format_desc.description,
             (format_desc.pixelformat >> 0) & 0xff,
             (format_desc.pixelformat >> 8) & 0xff,
             (format_desc.pixelformat >> 16) & 0xff,
             (format_desc.pixelformat >> 24) & 0xff);
  }

  if (num_formats)
    *num_formats = i;
  return true;
}

uint32_t V4L2Device::GetPixelFormat(uint32_t index) {
  v4l2_fmtdesc format_desc;
  memset(&format_desc, 0, sizeof(format_desc));
  format_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  format_desc.index = index;
  if (-1 == DoIoctl(VIDIOC_ENUM_FMT, &format_desc))
    return 0xFFFFFFFF;
  return format_desc.pixelformat;
}

bool V4L2Device::EnumFrameSize(uint32_t pixfmt, bool show_frmsize) {
  for (uint32_t i = 0; ; ++i) {
    v4l2_frmsizeenum frmsize_desc;
    memset(&frmsize_desc, 0, sizeof(frmsize_desc));
    frmsize_desc.pixel_format = pixfmt;
    frmsize_desc.index = i;
    if (-1 == DoIoctl(VIDIOC_ENUM_FRAMESIZES, &frmsize_desc)) {
      if (i == 0) {
        printf("<<< Info: VIDIOC_ENUM_FRAMESIZES not supported.>>>\n");
        return false;
      } else {
        break;
      }
    }
    if (show_frmsize) {
      switch (frmsize_desc.type) {
        case V4L2_FRMSIZE_TYPE_DISCRETE:
          printf("<<< Info supported discrete frame size #%d:"
                 " for pixel foramt(%c%c%c%c): %dx%d >>>\n", i+1,
                 (pixfmt >> 0) & 0xff, (pixfmt >> 8) & 0xff,
                 (pixfmt >> 16) & 0xff, (pixfmt >> 24) & 0xff,
                 frmsize_desc.discrete.width,
                 frmsize_desc.discrete.height);
          break;
        case V4L2_FRMSIZE_TYPE_CONTINUOUS:
          printf("<<< Info supported discrete frame size #%d:"
                 " for pixel foramt(%c%c%c%c): "
                 " from %dx%d to %dx%d >>>\n", i+1,
                 (pixfmt >> 0) & 0xff, (pixfmt >> 8) & 0xff,
                 (pixfmt >> 16) & 0xff, (pixfmt >> 24) & 0xff,
                 frmsize_desc.stepwise.min_width,
                 frmsize_desc.stepwise.min_height,
                 frmsize_desc.stepwise.max_width,
                 frmsize_desc.stepwise.max_height);
          break;
        case V4L2_FRMSIZE_TYPE_STEPWISE:
          printf("<<< Info supported discrete frame size #%d:"
                 " for pixel foramt(%c%c%c%c): "
                 " from %dx%d to %dx%d step(%d,%d) >>>\n", i+1,
                 (pixfmt >> 0) & 0xff, (pixfmt >> 8) & 0xff,
                 (pixfmt >> 16) & 0xff, (pixfmt >> 24) & 0xff,
                 frmsize_desc.stepwise.min_width,
                 frmsize_desc.stepwise.min_height,
                 frmsize_desc.stepwise.max_width,
                 frmsize_desc.stepwise.max_height,
                 frmsize_desc.stepwise.step_width,
                 frmsize_desc.stepwise.step_height);
          break;
      }
    }
  }
  return true;
}

bool V4L2Device::QueryControl(uint32_t id, v4l2_queryctrl* ctrl) {
  memset(ctrl, 0, sizeof(*ctrl));
  ctrl->id = id;
  if (-1 == DoIoctl(VIDIOC_QUERYCTRL, ctrl)) {
    if (errno != EINVAL) return false;
    printf("%d is not supported\n", id);
    return false;
  }
  if (ctrl->flags & V4L2_CTRL_FLAG_DISABLED) {
    printf("%d is not supported\n", id);
    return false;
  }
  return true;
}

bool V4L2Device::SetControl(uint32_t id, int32_t value) {
  v4l2_control control;
  control.id = id;
  control.value = value;
  if (-1 == DoIoctl(VIDIOC_S_CTRL, &control)) {
    printf("<<< Info: VIDIOC_S_CTRL failed. %d>>>\n", errno);
    return false;
  }
  return true;
}

bool V4L2Device::GetCropCap(v4l2_cropcap* cropcap) {
  if (-1 == DoIoctl(VIDIOC_CROPCAP, cropcap)) {
    printf("<<< Warning: VIDIOC_CROPCAP not supported.>>>\n");
    return false;
  }
  return true;
}

bool V4L2Device::GetCrop(v4l2_crop* crop) {
  if (-1 == DoIoctl(VIDIOC_G_CROP, crop)) {
    printf("<<< Warning: VIDIOC_G_CROP not supported.>>>\n");
    return false;
  }
  printf("crop: %d, %d, %d, %d\n",
         crop->c.left, crop->c.top,
         crop->c.width, crop->c.height);
  return true;
}

bool V4L2Device::SetCrop(v4l2_crop* crop) {
  if (-1 == DoIoctl(VIDIOC_S_CROP, crop)) {
    printf("<<< Warning: VIDIOC_S_CROP not supported.>>>\n");
    return false;
  }
  return true;
}

bool V4L2Device::ProbeCaps(v4l2_capability* cap, bool show_caps) {
  if (-1 == DoIoctl(VIDIOC_QUERYCAP, cap)) {
    printf("<<< Error: VIDIOC_QUERYCAP on %s.>>>\n", dev_name_);
    return false;
  }

  if (show_caps) {
    if (cap->capabilities & V4L2_CAP_VIDEO_CAPTURE)
      printf("<<< Info: %s support video capture interface.>>>\n", dev_name_);
    if (cap->capabilities & V4L2_CAP_VIDEO_OUTPUT)
      printf("<<< Info: %s support video output interface.>>>\n", dev_name_);
    if (cap->capabilities & V4L2_CAP_VIDEO_OVERLAY)
      printf("<<< Info: %s support video overlay interface.>>>\n", dev_name_);
    if (cap->capabilities & V4L2_CAP_AUDIO)
      printf("<<< Info: %s support audio i/o interface.>>>\n", dev_name_);

    if (cap->capabilities & V4L2_CAP_READWRITE)
      printf("<<< Info: %s support read/write interface.>>>\n", dev_name_);
    if (cap->capabilities & V4L2_CAP_STREAMING)
      printf("<<< Info: %s support streaming i/o interface.>>>\n", dev_name_);
    if (cap->capabilities & V4L2_CAP_TIMEPERFRAME)
      printf("<<< Info: %s support flexible frame period.>>>\n", dev_name_);
  }

  return true;
}

uint32_t V4L2Device::MapFourCC(const char* fourcc) {
  return v4l2_fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
}

bool V4L2Device::GetParam(v4l2_streamparm* param) {
  param->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == DoIoctl(VIDIOC_G_PARM, param)) {
    printf("<<< Warning: VIDIOC_G_PARM not supported.>>>\n");
    return false;
  }

  return true;
}

bool V4L2Device::SetParam(v4l2_streamparm* param) {
  if (-1 == DoIoctl(VIDIOC_S_PARM, param)) {
    printf("<<< Warning: VIDIOC_S_PARM not supported.>>>\n");
    return false;
  }
  return true;
}

bool V4L2Device::SetFrameRate(uint32_t fps) {
  v4l2_streamparm param;
  if (!GetParam(&param))
    return false;
  param.parm.capture.timeperframe.numerator = 1;
  param.parm.capture.timeperframe.denominator = fps;
  return SetParam(&param);
}

uint32_t V4L2Device::GetFrameRate() {
  v4l2_streamparm param;
  if (!GetParam(&param))
    return -1;
  return (param.parm.capture.timeperframe.denominator /
          param.parm.capture.timeperframe.numerator);
}

uint64_t V4L2Device::Now() {
  struct timespec ts;
  int res = clock_gettime(CLOCK_MONOTONIC, &ts);
  CHECK(res == 0);
  return static_cast<uint64_t>(ts.tv_sec);
}
