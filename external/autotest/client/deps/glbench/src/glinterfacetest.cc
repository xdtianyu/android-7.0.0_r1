// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"

#include "glinterfacetest.h"

namespace glbench {

namespace {

// Basic shader code.
const char* kVertexShader =
    "attribute vec4 c;"
    "void main() {"
    "  gl_Position = c;"
    "}";

const char* kFragmentShader =
    "uniform vec4 color;"
    "void main() {"
    "  gl_FragColor = color;"
    "}";

// Vertex arrays used to draw a diamond.
const GLfloat kVertices[] = { 1.0, 0.0,
                              0.0, -1.0,
                              -1.0, 0.0,
                              0.0, 1.0 };
const GLushort kIndices[] = { 0, 1, 2,
                              0, 2, 3 };

}  // namespace

void GLInterfaceTest::SetupGLRendering() {
  vertex_buffer_object_ =
      SetupVBO(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices);

  shader_program_ = InitShaderProgram(kVertexShader, kFragmentShader);
  attribute_index_ = glGetAttribLocation(shader_program_, "c");
  glVertexAttribPointer(attribute_index_, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(attribute_index_);

  GLint color_uniform = glGetUniformLocation(shader_program_, "color");

  const GLfloat white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  glUniform4fv(color_uniform, 1, white);

  num_indices_ = arraysize(kIndices);
  index_buffer_object_ =
      SetupVBO(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices);
}

void GLInterfaceTest::CleanupGLRendering() {
  glDisableVertexAttribArray(attribute_index_);
  glDeleteProgram(shader_program_);
  glDeleteBuffers(1, &index_buffer_object_);
  glDeleteBuffers(1, &vertex_buffer_object_);
}

bool GLInterfaceTest::Run() {
  const std::string test_name_base = std::string(Name()) + "_";

  // Run test without GL commands.
  render_func_.Reset();
  RunTest(this, (test_name_base + "nogl").c_str(), 1.0, g_width, g_height, false);

  // Run main test with simple GL commands.
  SetupGLRendering();
  render_func_ = base::Bind(&GLInterfaceTest::RenderGLSimple,
                            base::Unretained(this));
  RunTest(this, (test_name_base + "glsimple").c_str(), 1.0, g_width, g_height, false);
  CleanupGLRendering();

  // TODO(sque): Run with complex GL commands. See crosbug.com/36746.
  return true;
}

void GLInterfaceTest::RenderGLSimple() {
  glDrawElements(GL_TRIANGLES, num_indices_, GL_UNSIGNED_SHORT, 0);
}

} // namespace glbench
