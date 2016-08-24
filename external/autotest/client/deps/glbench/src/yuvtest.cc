// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <sys/mman.h>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"

#include "main.h"
#include "testbase.h"
#include "utils.h"
#include "yuv2rgb.h"

namespace glbench {

class YuvToRgbTest : public DrawArraysTestFunc {
 public:
  YuvToRgbTest() {
    memset(textures_, 0, sizeof(textures_));
  }
  virtual ~YuvToRgbTest() {
    glDeleteTextures(arraysize(textures_), textures_);
  }
  virtual bool Run();
  virtual const char* Name() const { return "yuv_to_rgb"; }

  enum YuvTestFlavor {
    YUV_PLANAR_ONE_TEXTURE_SLOW,
    YUV_PLANAR_ONE_TEXTURE_FASTER,
    YUV_PLANAR_THREE_TEXTURES,
    YUV_SEMIPLANAR_TWO_TEXTURES,
  };

 private:
  GLuint textures_[6];
  YuvTestFlavor flavor_;
  GLuint YuvToRgbShaderProgram(GLuint vertex_buffer, int width, int height);
  bool SetupTextures();
  DISALLOW_COPY_AND_ASSIGN(YuvToRgbTest);
};


GLuint YuvToRgbTest::YuvToRgbShaderProgram(GLuint vertex_buffer,
                                           int width, int height) {
  const char *vertex = NULL;
  const char *fragment = NULL;

  switch (flavor_) {
    case YUV_PLANAR_ONE_TEXTURE_SLOW:
      vertex = YUV2RGB_VERTEX_1;
      fragment = YUV2RGB_FRAGMENT_1;
      break;
    case YUV_PLANAR_ONE_TEXTURE_FASTER:
      vertex = YUV2RGB_VERTEX_2;
      fragment = YUV2RGB_FRAGMENT_2;
      break;
    case YUV_PLANAR_THREE_TEXTURES:
      vertex = YUV2RGB_VERTEX_34;
      fragment = YUV2RGB_FRAGMENT_3;
      break;
    case YUV_SEMIPLANAR_TWO_TEXTURES:
      vertex = YUV2RGB_VERTEX_34;
      fragment = YUV2RGB_FRAGMENT_4;
      break;
  }

  size_t size_vertex = 0;
  size_t size_fragment = 0;
  char *yuv_to_rgb_vertex = static_cast<char *>(
      MmapFile(vertex, &size_vertex));
  char *yuv_to_rgb_fragment = static_cast<char *>(
      MmapFile(fragment, &size_fragment));
  GLuint program = 0;

  if (!yuv_to_rgb_fragment || !yuv_to_rgb_vertex)
    goto done;

  {
    program = InitShaderProgramWithHeader(NULL, yuv_to_rgb_vertex,
                                          yuv_to_rgb_fragment);

    int imageWidthUniform = glGetUniformLocation(program, "imageWidth");
    int imageHeightUniform = glGetUniformLocation(program, "imageHeight");

    int textureSampler = glGetUniformLocation(program, "textureSampler");
    int evenLinesSampler = glGetUniformLocation(program, "paritySampler");
    int ySampler = glGetUniformLocation(program, "ySampler");
    int uSampler = glGetUniformLocation(program, "uSampler");
    int vSampler = glGetUniformLocation(program, "vSampler");
    int uvSampler = glGetUniformLocation(program, "uvSampler");

    glUniform1f(imageWidthUniform, width);
    glUniform1f(imageHeightUniform, height);
    glUniform1i(textureSampler, 0);
    glUniform1i(evenLinesSampler, 1);

    glUniform1i(ySampler, 2);
    glUniform1i(uSampler, 3);
    glUniform1i(vSampler, 4);
    glUniform1i(uvSampler, 5);

    {
      // This is used only if USE_UNIFORM_MATRIX is enabled in fragment
      // shaders.
      float c[] = {
        1.0,    1.0,    1.0,   0.0,
        0.0,   -0.344,  1.772, 0.0,
        1.402, -0.714,  0.0,   0.0,
        -0.701,  0.529, -0.886, 1.0
      };
      int conversion = glGetUniformLocation(program, "conversion");
      glUniformMatrix4fv(conversion, 1, GL_FALSE, c);
      assert(glGetError() == 0);
    }

    int attribute_index = glGetAttribLocation(program, "c");
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(attribute_index);
    return program;
  }


done:
  munmap(yuv_to_rgb_fragment, size_fragment);
  munmap(yuv_to_rgb_fragment, size_vertex);
  return program;
}


bool YuvToRgbTest::SetupTextures() {
  bool ret = false;
  size_t size = 0;
  char evenodd[2] = {0, static_cast<char>(-1)};
  char* pixels = static_cast<char *>(MmapFile(YUV2RGB_NAME, &size));
  const int luma_size = YUV2RGB_WIDTH * YUV2RGB_PIXEL_HEIGHT;
  const int chroma_size = YUV2RGB_WIDTH/2 * YUV2RGB_PIXEL_HEIGHT/2;
  const char* u_plane = pixels + luma_size;
  const char* v_plane = pixels + luma_size + chroma_size;
  if (!pixels) {
    printf("# Error: Could not open image file: %s\n", YUV2RGB_NAME);
    goto done;
  }
  if (size != YUV2RGB_SIZE) {
    printf("# Error: Image file of wrong size, got %d, expected %d\n",
           static_cast<int>(size), YUV2RGB_SIZE);
    goto done;
  }

  glGenTextures(arraysize(textures_), textures_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textures_[0]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, YUV2RGB_WIDTH, YUV2RGB_HEIGHT,
               0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, textures_[1]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 2, 1,
               0, GL_LUMINANCE, GL_UNSIGNED_BYTE, evenodd);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, textures_[2]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
               YUV2RGB_WIDTH, YUV2RGB_PIXEL_HEIGHT,
               0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);

  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, textures_[3]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
               YUV2RGB_WIDTH/2, YUV2RGB_PIXEL_HEIGHT/2,
               0, GL_LUMINANCE, GL_UNSIGNED_BYTE, u_plane);

  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, textures_[4]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
               YUV2RGB_WIDTH/2, YUV2RGB_PIXEL_HEIGHT/2,
               0, GL_LUMINANCE, GL_UNSIGNED_BYTE, v_plane);

  {
    scoped_ptr<char[]> buf_uv(new char[chroma_size * 2]);
    char* buf_uv_ptr = buf_uv.get();
    for (int i = 0; i < chroma_size; i++) {
        *buf_uv_ptr++ = u_plane[i];
        *buf_uv_ptr++ = v_plane[i];
    }

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, textures_[5]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                 YUV2RGB_WIDTH/2, YUV2RGB_PIXEL_HEIGHT/2,
                 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, buf_uv.get());
  }

  for (unsigned int i = 0; i < arraysize(textures_); i++) {
    glActiveTexture(GL_TEXTURE0 + i);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  ret = true;

done:
  munmap(pixels, size);
  return ret;
}


bool YuvToRgbTest::Run() {
  glClearColor(0.f, 1.f, 0.f, 1.f);

  GLuint program = 0;
  GLuint vertex_buffer = 0;
  GLfloat vertices[8] = {
    0.f, 0.f,
    1.f, 0.f,
    0.f, 1.f,
    1.f, 1.f,
  };
  vertex_buffer = SetupVBO(GL_ARRAY_BUFFER, sizeof(vertices), vertices);

  if (!SetupTextures())
    return false;

  glViewport(0, 0, YUV2RGB_WIDTH, YUV2RGB_PIXEL_HEIGHT);

  YuvTestFlavor flavors[] = {
    YUV_PLANAR_ONE_TEXTURE_SLOW, YUV_PLANAR_ONE_TEXTURE_FASTER,
    YUV_PLANAR_THREE_TEXTURES, YUV_SEMIPLANAR_TWO_TEXTURES
  };
  const char* flavor_names[] = {
    "yuv_shader_1", "yuv_shader_2", "yuv_shader_3", "yuv_shader_4"
  };
  for (unsigned int f = 0; f < arraysize(flavors); f++) {
    flavor_ = flavors[f];

    program = YuvToRgbShaderProgram(vertex_buffer,
                                    YUV2RGB_WIDTH, YUV2RGB_PIXEL_HEIGHT);
    if (program) {
      FillRateTestNormalSubWindow(flavor_names[f],
                                  std::min(YUV2RGB_WIDTH, g_width),
                                  std::min(YUV2RGB_PIXEL_HEIGHT, g_height));
    } else {
      printf("# Error: Could not set up YUV shader.\n");
    }

    glDeleteProgram(program);
  }

  glDeleteBuffers(1, &vertex_buffer);

  return true;
}


TestBase* GetYuvToRgbTest() {
  return new YuvToRgbTest();
}

} // namespace glbench
