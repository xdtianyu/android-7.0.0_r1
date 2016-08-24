// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "main.h"
#include "testbase.h"
#include "utils.h"

#include <algorithm>


namespace glbench {


class FillRateTest : public DrawArraysTestFunc {
 public:
  FillRateTest() {}
  virtual ~FillRateTest() {}
  virtual bool Run();
  virtual const char* Name() const { return "fill_rate"; }

 private:
  DISALLOW_COPY_AND_ASSIGN(FillRateTest);
};

class FboFillRateTest : public DrawArraysTestFunc {
 public:
  FboFillRateTest() {}
  virtual ~FboFillRateTest() {}
  virtual bool Run();
  virtual const char* Name() const { return "fbo_fill_rate"; }

 private:
  DISALLOW_COPY_AND_ASSIGN(FboFillRateTest);
};

const char* kVertexShader1 =
    "attribute vec4 position;"
    "void main() {"
    "  gl_Position = position;"
    "}";

const char* kFragmentShader1 =
    "uniform vec4 color;"
    "void main() {"
    "  gl_FragColor = color;"
    "}";


const char* kVertexShader2 =
    "attribute vec4 position;"
    "attribute vec4 texcoord;"
    "uniform float scale;"
    "varying vec4 v1;"
    "void main() {"
    "  gl_Position = position * vec4(scale, scale, 1., 1.);"
    "  v1 = texcoord;"
    "}";

const char* kFragmentShader2 =
    "uniform sampler2D texture;"
    "varying vec4 v1;"
    "void main() {"
    "  gl_FragColor = texture2D(texture, v1.xy);"
    "}";

const GLfloat buffer_vertex[8] = {
  -1.f, -1.f,
  1.f,  -1.f,
  -1.f, 1.f,
  1.f,  1.f,
};

const GLfloat buffer_texture[8] = {
  0.f, 0.f,
  1.f, 0.f,
  0.f, 1.f,
  1.f, 1.f,
};

const GLfloat red[4] = {1.f, 0.f, 0.f, 1.f};


bool FillRateTest::Run() {
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  GLuint vbo_vertex = SetupVBO(GL_ARRAY_BUFFER,
                               sizeof(buffer_vertex), buffer_vertex);
  GLuint program = InitShaderProgram(kVertexShader1, kFragmentShader1);
  GLint position_attribute = glGetAttribLocation(program, "position");
  glVertexAttribPointer(position_attribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(position_attribute);

  GLint color_uniform = glGetUniformLocation(program, "color");
  glUniform4fv(color_uniform, 1, red);

  FillRateTestNormal("fill_solid");
  FillRateTestBlendDepth("fill_solid");

  glDeleteProgram(program);

  program = InitShaderProgram(kVertexShader2, kFragmentShader2);
  position_attribute = glGetAttribLocation(program, "position");
  // Reusing vbo_vertex buffer from the previous test.
  glVertexAttribPointer(position_attribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(position_attribute);

  GLuint vbo_texture = SetupVBO(GL_ARRAY_BUFFER,
                                sizeof(buffer_texture), buffer_texture);
  GLuint texcoord_attribute = glGetAttribLocation(program, "texcoord");
  glVertexAttribPointer(texcoord_attribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(texcoord_attribute);

  // Get a fractal looking source texture of size 512x512 and full levels
  // of detail.
  GLuint texture = SetupTexture(9);

  GLuint texture_uniform = glGetUniformLocation(program, "texture");
  glUniform1i(texture_uniform, 0);

  GLuint scale_uniform = glGetUniformLocation(program, "scale");
  glUniform1f(scale_uniform, 1.f);

  FillRateTestNormal("fill_tex_nearest");

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  FillRateTestNormal("fill_tex_bilinear");

  // lod = 0.5
  float scale = 0.7071f;
  glUniform1f(scale_uniform, scale);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  FillRateTestNormalSubWindow("fill_tex_trilinear_linear_05",
                              g_width, g_height);

  // lod = 0.4
  scale = 0.758f;
  glUniform1f(scale_uniform, scale);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  FillRateTestNormalSubWindow("fill_tex_trilinear_linear_04",
                              g_width, g_height);

  // lod = 0.1
  scale = 0.933f;
  glUniform1f(scale_uniform, scale);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  FillRateTestNormalSubWindow("fill_tex_trilinear_linear_01",
                              g_width, g_height);

  glDeleteProgram(program);
  glDeleteBuffers(1, &vbo_vertex);
  glDeleteBuffers(1, &vbo_texture);
  glDeleteTextures(1, &texture);

  return true;
}

bool FboFillRateTest::Run() {
  char name[256];
  CHECK(!glGetError());
  GLuint vbo_vertex = SetupVBO(GL_ARRAY_BUFFER,
                               sizeof(buffer_vertex), buffer_vertex);
  GLuint program = InitShaderProgram(kVertexShader2, kFragmentShader2);
  GLint position_attribute = glGetAttribLocation(program, "position");
  glVertexAttribPointer(position_attribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(position_attribute);
  GLuint vbo_texture = SetupVBO(GL_ARRAY_BUFFER,
                                sizeof(buffer_texture), buffer_texture);
  GLuint texcoord_attribute = glGetAttribLocation(program, "texcoord");
  glVertexAttribPointer(texcoord_attribute, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(texcoord_attribute);
  glDisable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  CHECK(!glGetError());

  // We don't care for tiny texture sizes. And why the 8K*8K reference is
  // only 700kB in size in the failure case it could be huge to upload to GS.
  // In hasty mode we ignore huge textures all together.
  const int max_size = std::min(g_hasty ? 512 : 4096, g_max_texture_size);
  // Start with 32x32 textures and go up from there.
  int size_log2 = 5;
  for (int size = 1 << size_log2; size <= max_size; size *= 2) {
    sprintf(name, "fbofill_tex_bilinear_%d", size);

    // Setup texture for FBO.
    GLuint destination_texture = 0;
    glGenTextures(1, &destination_texture);
    glBindTexture(GL_TEXTURE_2D, destination_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECK(!glGetError());

    // Setup Framebuffer.
    // TODO(fjhenigman): In WAFFLE_PLATFORM_NULL the default framebuffer
    // is NOT zero, so we have to save the current binding and restore
    // that value later.  Fix this.
    GLint save_fb;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &save_fb);
    GLuint framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D, destination_texture, 0);
    CHECK(!glGetError());

    // Attach texture and check for completeness.
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    CHECK(status == GL_FRAMEBUFFER_COMPLETE);
    glViewport(0, 0, size, size);

    // Get a fractal looking source texture of size size*size.
    GLuint source_texture = SetupTexture(size_log2);
    GLuint texture_uniform = glGetUniformLocation(program, "texture");
    glUniform1i(texture_uniform, 0);
    GLuint scale_uniform = glGetUniformLocation(program, "scale");
    glUniform1f(scale_uniform, 1.f);

    // Run the benchmark, save the images if desired.
    FillRateTestNormalSubWindow(name, size, size);

    // Clean up for this loop.
    glBindFramebuffer(GL_FRAMEBUFFER, save_fb);
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &source_texture);
    glDeleteTextures(1, &destination_texture);
    CHECK(!glGetError());

    size_log2++;
  }
  // Clean up invariants.
  glDeleteProgram(program);
  glDeleteBuffers(1, &vbo_vertex);
  glDeleteBuffers(1, &vbo_texture);
  // Just in case restore the viewport for all other tests.
  glViewport(0, 0, g_width, g_height);

  return true;
}

TestBase* GetFillRateTest() {
  return new FillRateTest;
}

TestBase* GetFboFillRateTest() {
  return new FboFillRateTest;
}

} // namespace glbench
