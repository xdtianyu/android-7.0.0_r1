/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This module exposes the constants from the linux/if_tun.h header file to
 * allow a Python script to create and manipulate TUN/TAP interfaces.
 * It also includes constants from linux/if.h and sys/ioctl.h not available in
 * other python modules.
 *
 * Some of these constants are architecture specific and can't be implemented
 * in pure Python, like the ioctl() call numbers.
 */

#include <Python.h>

/* Python wrappers */
void _init_linux_if_h(PyObject *m);
void _init_linux_if_tun_h(PyObject *m);
void _init_sys_ioctl_h(PyObject *m);

/* Module initialization */
static PyMethodDef pyiftun_methods[] = {
  {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initpyiftun(void) {
  PyObject *m;
  m = Py_InitModule("pyiftun", pyiftun_methods);
  if (!m) return;

  /* Initialize the wrappers */
  _init_linux_if_h(m);
  _init_linux_if_tun_h(m);
  _init_sys_ioctl_h(m);
}
