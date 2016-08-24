// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Python.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>

static int do_ioctl(int fd, int request, void* arg) {
  int r;
  do {
    r = ioctl(fd, request, arg);
  } while (-1 == r && EINTR == errno);
  return r;
}

static void v4l2_enum_formats(const char *dev, int buf_type,
                              PyObject *formats) {
  int fd = open(dev, O_RDWR | O_NONBLOCK, 0);
  if (fd == -1) return;

  for (int i = 0; ; ++i) {
    char pixel_format[4];
    v4l2_fmtdesc format_desc;

    memset(&format_desc, 0, sizeof(format_desc));
    format_desc.type = (v4l2_buf_type) buf_type;
    format_desc.index = i;
    if (-1 == do_ioctl(fd, VIDIOC_ENUM_FMT, &format_desc)) {
      break;
    }
    pixel_format[0] = format_desc.pixelformat & 0xFF;
    pixel_format[1] = (format_desc.pixelformat >> 8) & 0xFF;
    pixel_format[2] = (format_desc.pixelformat >> 16) & 0xFF;
    pixel_format[3] = (format_desc.pixelformat >> 24) & 0xFF;
    PyObject* item = PyString_FromStringAndSize(pixel_format, 4);
    PyList_Append(formats, item);
    Py_DECREF(item);
  }
  close(fd);
}

static PyObject *v4l2_enum_capture_formats(PyObject *self, PyObject *args) {
  const char *dev;
  if (!PyArg_ParseTuple(args, "s", &dev))
    return NULL;
  PyObject *formats = PyList_New(0);
  v4l2_enum_formats(dev, V4L2_BUF_TYPE_VIDEO_CAPTURE, formats);
  v4l2_enum_formats(dev, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, formats);
  return formats;
}

static PyObject *v4l2_enum_output_formats(PyObject *self, PyObject *args) {
  const char *dev;
  if (!PyArg_ParseTuple(args, "s", &dev))
    return NULL;
  PyObject *formats = PyList_New(0);
  v4l2_enum_formats(dev, V4L2_BUF_TYPE_VIDEO_OUTPUT, formats);
  v4l2_enum_formats(dev, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, formats);
  return formats;
}

/*
 * Bind Python function names to our C functions
 */
static PyMethodDef v4l2_methods[] = {
    {"enum_capture_formats", v4l2_enum_capture_formats, METH_VARARGS},
    {"enum_output_formats", v4l2_enum_output_formats, METH_VARARGS},
    {NULL, NULL}
};

/*
 * Python calls this to let us initialize our module
 */
PyMODINIT_FUNC initv4l2() {
  (void) Py_InitModule("v4l2", v4l2_methods);
}
