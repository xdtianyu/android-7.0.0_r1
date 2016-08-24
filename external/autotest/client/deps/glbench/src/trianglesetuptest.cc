// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdlib.h>

#include "main.h"
#include "testbase.h"
#include "utils.h"


namespace glbench {


class TriangleSetupTest : public DrawElementsTestFunc {
 public:
  TriangleSetupTest() {}
  virtual ~TriangleSetupTest() {}
  virtual bool Run();
  virtual const char* Name() const { return "triangle_setup"; }

 private:
  DISALLOW_COPY_AND_ASSIGN(TriangleSetupTest);
};

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

bool TriangleSetupTest::Run() {
  glViewport(0, 0, g_width, g_height);

  // This specifies a square mesh in the middle of the viewport.
  // Larger meshes make this test too slow for devices that do 1 mtri/sec.
  // Also note that GLES 2.0 uses 16 bit indices.
  GLint width = 128;
  GLint height = 128;

  GLfloat *vertices = NULL;
  GLsizeiptr vertex_buffer_size = 0;
  CreateLattice(&vertices, &vertex_buffer_size, 1.f / g_width, 1.f / g_height,
                width, height);
  GLuint vertex_buffer = SetupVBO(GL_ARRAY_BUFFER,
                                  vertex_buffer_size, vertices);

  GLuint program =
    InitShaderProgram(kVertexShader, kFragmentShader);
  GLint attribute_index = glGetAttribLocation(program, "c");
  glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(attribute_index);

  GLint color_uniform = glGetUniformLocation(program, "color");

  GLushort *indices = NULL;
  GLuint index_buffer = 0;
  GLsizeiptr index_buffer_size = 0;

  {
    // Use orange for drawing solid/all culled quads.
    const GLfloat orange[4] = {1.0f, 0.5f, 0.0f, 1.0f};
    glUniform4fv(color_uniform, 1, orange);
    count_ = CreateMesh(&indices, &index_buffer_size, width, height, 0);

    index_buffer = SetupVBO(GL_ELEMENT_ARRAY_BUFFER,
                            index_buffer_size, indices);
    RunTest(this, "triangle_setup", count_ / 3, g_width, g_height, true);
    glEnable(GL_CULL_FACE);
    RunTest(this, "triangle_setup_all_culled", count_ / 3, g_width, g_height, true);
    glDisable(GL_CULL_FACE);

    glDeleteBuffers(1, &index_buffer);
    delete[] indices;
  }

  {
    // Use blue-ish color for drawing quad with many holes.
    const GLfloat cyan[4] = {0.0f, 0.5f, 0.5f, 1.0f};
    glUniform4fv(color_uniform, 1, cyan);
    count_ = CreateMesh(&indices, &index_buffer_size, width, height,
                        RAND_MAX / 2);

    index_buffer = SetupVBO(GL_ELEMENT_ARRAY_BUFFER,
                            index_buffer_size, indices);
    glEnable(GL_CULL_FACE);
    RunTest(this, "triangle_setup_half_culled", count_ / 3, g_width, g_height, true);

    glDeleteBuffers(1, &index_buffer);
    delete[] indices;
  }

  glDeleteProgram(program);
  glDeleteBuffers(1, &vertex_buffer);
  delete[] vertices;
  return true;
}


TestBase* GetTriangleSetupTest() {
  return new TriangleSetupTest;
}


} // namespace glbench
