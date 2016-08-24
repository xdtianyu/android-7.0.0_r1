// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "main.h"
#include "testbase.h"
#include "utils.h"


namespace glbench {

const float kScreenScaleFactor = 1e6f * (WINDOW_WIDTH * WINDOW_HEIGHT) /
    (1280.f * 768);

class WindowManagerCompositingTest : public TestBase {
 public:
  WindowManagerCompositingTest(bool scissor)
      : scissor_(scissor),
      compositing_background_program_(0),
      compositing_foreground_program_(0) {}
  virtual ~WindowManagerCompositingTest() {}
  virtual bool TestFunc(uint64_t iterations);
  virtual bool Run();
  virtual const char* Name() const { return "compositing"; }
  virtual bool IsDrawTest() const { return true; }
  virtual const char* Unit() const { return "1280x768_fps"; }

  void InitializeCompositing();
  void TeardownCompositing();
  void InitBaseTexture();
  void UpdateTexture();
  void LoadTexture();

 private:
  bool scissor_;
  uint32_t texture_base_[WINDOW_HEIGHT*WINDOW_WIDTH];
  uint32_t texture_update_[WINDOW_HEIGHT*WINDOW_WIDTH];
  GLuint compositing_textures_[5];
  GLuint compositing_background_program_;
  GLuint compositing_foreground_program_;
  DISALLOW_COPY_AND_ASSIGN(WindowManagerCompositingTest);
};

TestBase* GetWindowManagerCompositingTest(bool enable_scissor) {
  return new WindowManagerCompositingTest(enable_scissor);
}

bool WindowManagerCompositingTest::Run() {
  const char* testname = "compositing";
  if (scissor_) {
    glScissor(0, 0, 1, 1);
    glEnable(GL_SCISSOR_TEST);
    testname = "compositing_no_fill";
  }
  InitializeCompositing();
  RunTest(this, testname, kScreenScaleFactor, WINDOW_WIDTH, WINDOW_HEIGHT, true);
  TeardownCompositing();
  return true;
}

bool WindowManagerCompositingTest::TestFunc(uint64_t iterations) {
  for (uint64_t i = 0 ; i < iterations; ++i) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the background
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    // We have to blend three textures, but we use multi-texture for this
    // blending, not fb blend, to avoid the external memory traffic
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, compositing_textures_[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, compositing_textures_[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, compositing_textures_[2]);
    // Use the right shader
    glUseProgram(compositing_background_program_);
    // Draw the quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Use the right shader
    glUseProgram(compositing_foreground_program_);

    // Compositing is blending, so we shall blend.
    glEnable(GL_BLEND);
    // Depth test is on for window occlusion
    glEnable(GL_DEPTH_TEST);

    // Draw window number one
    // This update acts like a chrome webkit sw rendering update.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, compositing_textures_[3]);
    UpdateTexture();
    // TODO(papakipos): this LoadTexture is likely doing more CPU memory copies
    // than we would like.
    LoadTexture();
    // TODO(papakipos): add color interpolation here, and modulate
    // texture against it.
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Draw window number two
    // This is a static window, so we don't update it.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, compositing_textures_[4]);
    // TODO(papakipos): add color interpolation here, and modulate
    // texture against it.
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
  return true;
}

const char *kBasicTextureVertexShader =
    "attribute vec4 c1;"
    "attribute vec4 c2;"
    "varying vec4 v1;"
    "void main() {"
    "    gl_Position = c1;"
    "    v1 = c2;"
    "}";

const char *kBasicTextureFragmentShader =
    "uniform sampler2D texture_sampler;"
    "varying vec4 v1;"
    "void main() {"
    "    gl_FragColor = texture2D(texture_sampler, v1.st);"
    "}";

GLuint BasicTextureShaderProgram(GLuint vertex_buffer, GLuint texture_buffer) {
  GLuint program = InitShaderProgram(kBasicTextureVertexShader,
                                     kBasicTextureFragmentShader);

  // Set up the texture sampler
  int textureSampler = glGetUniformLocation(program, "texture_sampler");
  glUniform1i(textureSampler, 0);

  // Set up vertex attribute
  int attribute_index = glGetAttribLocation(program, "c1");
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(attribute_index);

  // Set up texture attribute
  attribute_index = glGetAttribLocation(program, "c2");
  glBindBuffer(GL_ARRAY_BUFFER, texture_buffer);
  glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(attribute_index);

  return program;
}

const char *kDoubleTextureBlendVertexShader =
    "attribute vec4 c1;"
    "attribute vec4 c2;"
    "attribute vec4 c3;"
    "varying vec4 v1;"
    "varying vec4 v2;"
    "void main() {"
    "    gl_Position = c1;"
    "    v1 = c2;"
    "    v2 = c3;"
    "}";

const char *kDoubleTextureBlendFragmentShader =
    "uniform sampler2D texture_sampler_0;"
    "uniform sampler2D texture_sampler_1;"
    "varying vec4 v1;"
    "varying vec4 v2;"
    "void main() {"
    "    vec4 one = texture2D(texture_sampler_0, v1.st);"
    "    vec4 two = texture2D(texture_sampler_1, v2.st);"
    "    gl_FragColor = mix(one, two, 0.5);"
    "}";

// This shader blends the three textures
GLuint DoubleTextureBlendShaderProgram(GLuint vertex_buffer,
                                       GLuint texture_buffer_0,
                                       GLuint texture_buffer_1) {
  GLuint program = InitShaderProgram(kDoubleTextureBlendVertexShader,
                                     kDoubleTextureBlendFragmentShader);
  // Set up the texture sampler
  int textureSampler0 = glGetUniformLocation(program, "texture_sampler_0");
  glUniform1i(textureSampler0, 0);
  int textureSampler1 = glGetUniformLocation(program, "texture_sampler_1");
  glUniform1i(textureSampler1, 1);

  // Set up vertex attribute
  int attribute_index = glGetAttribLocation(program, "c1");
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(attribute_index);

  // Set up texture attributes
  attribute_index = glGetAttribLocation(program, "c2");
  glBindBuffer(GL_ARRAY_BUFFER, texture_buffer_0);
  glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(attribute_index);

  attribute_index = glGetAttribLocation(program, "c3");
  glBindBuffer(GL_ARRAY_BUFFER, texture_buffer_1);
  glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(attribute_index);

  return program;
}

const char *triple_texture_blend_vertex_shader =
"attribute vec4 c1;"
"attribute vec4 c2;"
"attribute vec4 c3;"
"attribute vec4 c4;"
"varying vec4 v1;"
"varying vec4 v2;"
"varying vec4 v3;"
"void main() {"
"    gl_Position = c1;"
"    v1 = c2;"
"    v2 = c3;"
"    v3 = c4;"
"}";

const char *triple_texture_blend_fragment_shader =
"uniform sampler2D texture_sampler_0;"
"uniform sampler2D texture_sampler_1;"
"uniform sampler2D texture_sampler_2;"
"varying vec4 v1;"
"varying vec4 v2;"
"varying vec4 v3;"
"void main() {"
"    vec4 one = texture2D(texture_sampler_0, v1.st);"
"    vec4 two = texture2D(texture_sampler_1, v2.st);"
"    vec4 three = texture2D(texture_sampler_2, v3.st);"
"    gl_FragColor = mix(mix(one, two, 0.5), three, 0.5);"
"}";

// This shader blends the three textures
GLuint TripleTextureBlendShaderProgram(GLuint vertex_buffer,
                                              GLuint texture_buffer_0,
                                              GLuint texture_buffer_1,
                                              GLuint texture_buffer_2) {
  GLuint program =
    InitShaderProgram(triple_texture_blend_vertex_shader,
                      triple_texture_blend_fragment_shader);

  // Set up the texture sampler
  int textureSampler0 = glGetUniformLocation(program, "texture_sampler_0");
  glUniform1i(textureSampler0, 0);
  int textureSampler1 = glGetUniformLocation(program, "texture_sampler_1");
  glUniform1i(textureSampler1, 1);
  int textureSampler2 = glGetUniformLocation(program, "texture_sampler_2");
  glUniform1i(textureSampler2, 2);

  // Set up vertex attribute
  int attribute_index = glGetAttribLocation(program, "c1");
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(attribute_index);

  // Set up texture attributes
  attribute_index = glGetAttribLocation(program, "c2");
  glBindBuffer(GL_ARRAY_BUFFER, texture_buffer_0);
  glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(attribute_index);

  attribute_index = glGetAttribLocation(program, "c3");
  glBindBuffer(GL_ARRAY_BUFFER, texture_buffer_1);
  glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(attribute_index);

  attribute_index = glGetAttribLocation(program, "c4");
  glBindBuffer(GL_ARRAY_BUFFER, texture_buffer_2);
  glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(attribute_index);

  return program;
}

void WindowManagerCompositingTest::InitializeCompositing() {
  InitBaseTexture();

  glClearColor(0.f, 0.f, 0.f, 0.f);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthFunc(GL_LEQUAL);

  glGenTextures(5, compositing_textures_);
  glActiveTexture(GL_TEXTURE0);
  for (int i = 0; i < 5; i++) {
    glBindTexture(GL_TEXTURE_2D, compositing_textures_[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR);
  }

  // Set up the vertex arrays for drawing textured quads later on.
  GLfloat buffer_vertex[8] = {
    -1.f, -1.f,
    1.f,  -1.f,
    -1.f, 1.f,
    1.f,  1.f,
  };
  GLuint vbo_vertex = SetupVBO(GL_ARRAY_BUFFER,
                               sizeof(buffer_vertex), buffer_vertex);

  GLfloat buffer_texture[8] = {
    0.f, 0.f,
    1.f, 0.f,
    0.f, 1.f,
    1.f, 1.f,
  };
  GLuint vbo_texture = SetupVBO(GL_ARRAY_BUFFER,
                                sizeof(buffer_texture), buffer_texture);

  // Set up the static background textures.
  UpdateTexture();
  UpdateTexture();
  UpdateTexture();
  // Load these textures into bound texture ids and keep using them
  // from there to avoid having to reload this texture every frame
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, compositing_textures_[0]);
  LoadTexture();
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, compositing_textures_[1]);
  LoadTexture();
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, compositing_textures_[2]);
  LoadTexture();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, compositing_textures_[3]);
  UpdateTexture();
  LoadTexture();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, compositing_textures_[4]);
  UpdateTexture();
  LoadTexture();

  // Set up vertex & fragment shaders.
  compositing_background_program_ =
      TripleTextureBlendShaderProgram(vbo_vertex,
                                      vbo_texture, vbo_texture, vbo_texture);
  compositing_foreground_program_ =
      BasicTextureShaderProgram(vbo_vertex, vbo_texture);
  if (!compositing_background_program_ || !compositing_foreground_program_) {
    printf("# Warning: Could not set up compositing shader.\n");
  }
}

void WindowManagerCompositingTest::TeardownCompositing() {
  glDeleteProgram(compositing_background_program_);
  glDeleteProgram(compositing_foreground_program_);
}

void WindowManagerCompositingTest::InitBaseTexture() {
  for (int y = 0; y < WINDOW_HEIGHT; y++) {
    for (int x = 0; x < WINDOW_WIDTH; x++) {
      // This color is gray, half alpha.
      texture_base_[y*WINDOW_WIDTH+x] = 0x80808080;
    }
  }
}

// UpdateTexture simulates Chrome updating tab contents.
// We cause a bunch of read and write cpu memory bandwidth.
// It's a very rough approximation.
void WindowManagerCompositingTest::UpdateTexture() {
  memcpy(texture_update_, texture_base_, sizeof(texture_base_));
}

void WindowManagerCompositingTest::LoadTexture() {
  // Use GL_RGBA for compatibility with GLES2.0.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
               WINDOW_WIDTH, WINDOW_HEIGHT, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, texture_update_);
}

} // namespace glbench
