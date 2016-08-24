// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Python.h>

#if !defined(USE_DRM)
#include <X11/Xlib.h>
#include <va/va.h>
#include <va/va_x11.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <va/va.h>
#include <va/va_drm.h>
#endif

static PyObject *VaapiError;

namespace {

struct DisplayBundle {
#if !defined(USE_DRM)
  Display *x11_display;
#else
  int drm_fd;
#endif
  VADisplay va_display;
};

static void destroy_display_bundle(PyObject* object) {
  DisplayBundle* bundle = (DisplayBundle*) PyCapsule_GetPointer(object, NULL);
  vaTerminate(bundle->va_display);
#if !defined(USE_DRM)
  XCloseDisplay(bundle->x11_display);
#else
  close(bundle->drm_fd);
#endif
  delete bundle;
}

static PyObject* va_create_display(PyObject* self, PyObject* args) {
#if !defined(USE_DRM)
  const char* display_name;
  if (!PyArg_ParseTuple(args, "s", &display_name))
    return NULL;

  Display *x11_display = XOpenDisplay(display_name);

  if (x11_display == NULL) {
    PyErr_SetString(VaapiError, "Cannot connect X server!");
    return NULL;
  }

  VADisplay va_display = vaGetDisplay(x11_display);
#else
  const char* drm_card_path;
  if (!PyArg_ParseTuple(args, "s", &drm_card_path))
    return NULL;

  int drm_fd = open(drm_card_path, O_RDWR);
  if (drm_fd < 0) {
    PyErr_SetString(VaapiError, "Cannot open drm card path");
    return NULL;
  }

  VADisplay va_display = vaGetDisplayDRM(drm_fd);
#endif
  if (!vaDisplayIsValid(va_display)) {
    PyErr_SetString(VaapiError, "Cannot get a valid display");
    return NULL;
  }

  int major_ver, minor_ver;

  VAStatus va_status = vaInitialize(va_display, &major_ver, &minor_ver);
  if (va_status != VA_STATUS_SUCCESS) {
    PyErr_SetString(VaapiError, "vaInitialize fail");
    return NULL;
  }

  DisplayBundle* bundle = new DisplayBundle();
#if !defined(USE_DRM)
  bundle->x11_display = x11_display;
#else
  bundle->drm_fd = drm_fd;
#endif
  bundle->va_display = va_display;

  return PyCapsule_New(bundle, NULL, destroy_display_bundle);
}

static VADisplay get_va_display(PyObject* object) {
  if (!PyCapsule_CheckExact(object)) {
    PyErr_SetString(VaapiError, "invalid display object");
    return NULL;
  }

  DisplayBundle* bundle = (DisplayBundle*) PyCapsule_GetPointer(object, NULL);

  if (bundle == NULL)
    return NULL;

  return bundle->va_display;
}

static PyObject* va_query_profiles(PyObject* self, PyObject* args) {
  PyObject* bundle;
  if (!PyArg_ParseTuple(args, "O", &bundle))
    return NULL;

  VADisplay va_display = get_va_display(bundle);

  if (va_display == NULL)
    return NULL;

  int num_profiles = vaMaxNumProfiles(va_display);
  VAProfile *profile = new VAProfile[num_profiles];

  VAStatus status = vaQueryConfigProfiles(va_display, profile, &num_profiles);

  if (status != VA_STATUS_SUCCESS) {
    delete [] profile;
    PyErr_SetString(VaapiError, "vaQueryConfigProfiles fail");
    return NULL;
  }

  PyObject *result = PyList_New(0);
  for (int i = 0; i < num_profiles; ++i) {
    size_t value = static_cast<size_t>(profile[i]);
    PyList_Append(result, PyInt_FromSize_t(value));
  }
  delete [] profile;
  return result;
}

static PyObject* va_query_entrypoints(PyObject* self, PyObject* args) {
  PyObject* bundle;
  int profile;
  if (!PyArg_ParseTuple(args, "Oi", &bundle, &profile))
    return NULL;

  VADisplay va_display = get_va_display(bundle);
  if (va_display == NULL) 
    return NULL;

  int num_entrypoints = vaMaxNumEntrypoints(va_display);
  VAEntrypoint* entrypoint = new VAEntrypoint[num_entrypoints];

  VAStatus status = vaQueryConfigEntrypoints(va_display,
                                             static_cast<VAProfile>(profile),
                                             entrypoint,
                                             &num_entrypoints);
  if (status != VA_STATUS_SUCCESS) {
    PyErr_SetString(VaapiError, "vaQueryConfigEntryPoints fail");
    return NULL;
  }

  PyObject *result = PyList_New(0);
  for (int i = 0; i < num_entrypoints; ++i) {
    size_t value = static_cast<size_t>(entrypoint[i]);
    PyList_Append(result, PyInt_FromSize_t(value));
  }
  return result;
}

static PyObject* va_get_rt_format(PyObject* self, PyObject* args) {
  PyObject* bundle;
  int profile;
  int entrypoint;
  if (!PyArg_ParseTuple(args, "Oii", &bundle, &profile, &entrypoint))
    return NULL;

  VADisplay va_display = get_va_display(bundle);
  if (va_display == NULL) 
    return NULL;

  VAConfigAttrib attrib;
  attrib.type = VAConfigAttribRTFormat;
  VAStatus status = vaGetConfigAttributes(va_display,
                                          static_cast<VAProfile>(profile),
                                          static_cast<VAEntrypoint>(entrypoint),
                                          &attrib,
                                          1);
  if (status != VA_STATUS_SUCCESS) {
    PyErr_SetString(VaapiError, "vaGetConfgAttributes fail");
    return NULL;
  }

  return PyInt_FromSize_t(attrib.value);
}

/*
 * Bind Python function names to our C functions
 */
static PyMethodDef vaapi_methods[] = {
    {"create_display", va_create_display, METH_VARARGS},
    {"query_profiles", va_query_profiles, METH_VARARGS},
    {"query_entrypoints", va_query_entrypoints, METH_VARARGS},
    {"get_rt_format", va_get_rt_format, METH_VARARGS},
    {NULL, NULL}
};

} // end of namespace

/*
 * Python calls this to let us initialize our module
 */
PyMODINIT_FUNC initvaapi() {
  PyObject *m = Py_InitModule("vaapi", vaapi_methods);
  if (m == NULL)
    return;

  VaapiError = PyErr_NewException((char*)"vaapi.error", NULL, NULL);
  Py_INCREF(VaapiError);
  PyModule_AddObject(m, "error", VaapiError);
}
