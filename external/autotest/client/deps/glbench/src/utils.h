// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BENCH_GL_UTILS_H_
#define BENCH_GL_UTILS_H_

#if defined(USE_OPENGLES)
#include "GLES2/gl2.h"
#elif defined(USE_OPENGL)
#include "GL/gl.h"
#endif

#include "base/files/file_path.h"

extern double g_initial_temperature;

void SetBasePathFromArgv0(const char* argv0, const char* relative);
void *MmapFile(const char *name, size_t *length);
const base::FilePath& GetBasePath();

// Returns temperature of system before testing started. It is used as a
// reference for keeping the machine cool.
const double GetInitialMachineTemperature();
// For thermal monitoring of system.
double GetMachineTemperature();
// Wait for machine to cool with temperature in Celsius and timeout in seconds.
// Returns the time spent waiting and sets the last observed temperature.
double WaitForCoolMachine(double cold_temperature, double timeout,
                          double *temperature);

namespace glbench {

GLuint SetupTexture(GLsizei size_log2);
GLuint SetupVBO(GLenum target, GLsizeiptr size, const GLvoid *data);
void CreateLattice(GLfloat **vertices, GLsizeiptr *size,
                   GLfloat size_x, GLfloat size_y, int width, int height);
int CreateMesh(GLushort **indices, GLsizeiptr *size,
                      int width, int height, int culled_ratio);
GLuint InitShaderProgram(const char *vertex_src, const char *fragment_src);
GLuint InitShaderProgramWithHeader(const char* header,
                                   const char* vertex_src,
                                   const char* fragment_src);
GLuint InitShaderProgramWithHeaders(const char** headers,
                                    int count,
                                    const char* vertex_src,
                                    const char* fragment_src);
void ClearBuffers();

} // namespace glbench

#endif // BENCH_GL_UTILS_H_
