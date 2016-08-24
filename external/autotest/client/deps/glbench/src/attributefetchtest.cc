// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "main.h"
#include "utils.h"
#include "testbase.h"


namespace glbench {

class AttributeFetchShaderTest : public DrawElementsTestFunc {
 public:
  AttributeFetchShaderTest() {}
  virtual ~AttributeFetchShaderTest() {}
  virtual bool Run();
  virtual const char* Name() const { return "attribute_fetch_shader"; }
  virtual bool IsDrawTest() const { return false; }
  virtual const char* Unit() const { return "mvtx_sec"; }

 private:
  DISALLOW_COPY_AND_ASSIGN(AttributeFetchShaderTest);
};


const char *simple_vertex_shader =
"attribute vec4 c1;"
"void main() {"
"    gl_Position = c1;"
"}";

const char *simple_vertex_shader_2_attr =
"attribute vec4 c1;"
"attribute vec4 c2;"
"void main() {"
"    gl_Position = c1+c2;"
"}";

const char *simple_vertex_shader_4_attr =
"attribute vec4 c1;"
"attribute vec4 c2;"
"attribute vec4 c3;"
"attribute vec4 c4;"
"void main() {"
"    gl_Position = c1+c2+c3+c4;"
"}";

const char *simple_vertex_shader_8_attr =
"attribute vec4 c1;"
"attribute vec4 c2;"
"attribute vec4 c3;"
"attribute vec4 c4;"
"attribute vec4 c5;"
"attribute vec4 c6;"
"attribute vec4 c7;"
"attribute vec4 c8;"
"void main() {"
"    gl_Position = c1+c2+c3+c4+c5+c6+c7+c8;"
"}";

const char *simple_fragment_shader =
"void main() {"
"    gl_FragColor = vec4(0.5);"
"}";

GLuint AttributeFetchShaderProgram(int attribute_count,
                                   GLuint vertex_buffers[]) {
  const char *vertex_shader = NULL;
  switch (attribute_count) {
    case 1: vertex_shader = simple_vertex_shader; break;
    case 2: vertex_shader = simple_vertex_shader_2_attr; break;
    case 4: vertex_shader = simple_vertex_shader_4_attr; break;
    case 8: vertex_shader = simple_vertex_shader_8_attr; break;
    default: return 0;
  }
  GLuint program =
    InitShaderProgram(vertex_shader, simple_fragment_shader);

  for (int i = 0; i < attribute_count; i++) {
    char attribute[] = "c_";
    attribute[1] = '1' + i;
    int attribute_index = glGetAttribLocation(program, attribute);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[i]);
    glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(attribute_index);
  }

  return program;
}

bool AttributeFetchShaderTest::Run() {
  GLint width = 64;
  GLint height = 64;

  glViewport(0, 0, g_width, g_height);

  GLfloat *vertices = NULL;
  GLsizeiptr vertex_buffer_size = 0;
  CreateLattice(&vertices, &vertex_buffer_size, 1.f / g_width, 1.f / g_height,
                width, height);
  GLuint vertex_buffer = SetupVBO(GL_ARRAY_BUFFER,
                                  vertex_buffer_size, vertices);

  GLushort *indices = NULL;
  GLuint index_buffer = 0;
  GLsizeiptr index_buffer_size = 0;

  // Everything will be back-face culled.
  count_ = CreateMesh(&indices, &index_buffer_size, width, height, 0);
  index_buffer = SetupVBO(GL_ELEMENT_ARRAY_BUFFER,
                          index_buffer_size, indices);

  glEnable(GL_CULL_FACE);

  GLuint vertex_buffers[8];
  for (GLuint i = 0; i < sizeof(vertex_buffers)/sizeof(vertex_buffers[0]); i++)
    vertex_buffers[i] = vertex_buffer;

  GLuint program = AttributeFetchShaderProgram(1, vertex_buffers);
  RunTest(this, "attribute_fetch_shader", count_, g_width, g_height, true);
  glDeleteProgram(program);

  program = AttributeFetchShaderProgram(2, vertex_buffers);
  RunTest(this,
          "attribute_fetch_shader_2_attr", count_, g_width, g_height, true);
  glDeleteProgram(program);

  program = AttributeFetchShaderProgram(4, vertex_buffers);
  RunTest(this,
          "attribute_fetch_shader_4_attr", count_, g_width, g_height, true);
  glDeleteProgram(program);

  program = AttributeFetchShaderProgram(8, vertex_buffers);
  RunTest(this,
          "attribute_fetch_shader_8_attr", count_, g_width, g_height, true);
  glDeleteProgram(program);

  glDeleteBuffers(1, &index_buffer);
  delete[] indices;

  glDeleteBuffers(1, &vertex_buffer);
  delete[] vertices;
  return true;
}

TestBase* GetAttributeFetchShaderTest() {
  return new AttributeFetchShaderTest();
}

} // namespace glbench
