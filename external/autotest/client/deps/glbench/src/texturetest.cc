// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "texturetest.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"

namespace glbench {

namespace {

// Vertex and fragment shader code.
const char* kVertexShader =
    "attribute vec4 c1;"
    "attribute vec4 c2;"
    "varying vec4 v1;"
    "void main() {"
    "  gl_Position = c1;"
    "  v1 = c2;"
    "}";

const char* kFragmentShader =
    "varying vec4 v1;"
    "uniform sampler2D texture;"
    "void main() {"
    "  gl_FragColor = texture2D(texture, v1.xy);"
    "}";

}  // namespace

bool TextureTest::Run() {
  // Two triangles that form one pixel at 0, 0.
  const GLfloat kVertices[8] = {
    0.f, 0.f,
    2.f / g_width, 0.f,
    0.f, 2.f / g_height,
    2.f / g_width, 2.f / g_height,
  };
  const GLfloat kTexCoords[8] = {
    0.f, 0.f, 0.f, 0.f,
    0.f, 0.f, 0.f, 0.f,
  };

  program_ = InitShaderProgram(kVertexShader, kFragmentShader);

  int attr1 = glGetAttribLocation(program_, "c1");
  glVertexAttribPointer(attr1, 2, GL_FLOAT, GL_FALSE, 0, kVertices);
  glEnableVertexAttribArray(attr1);

  int attr2 = glGetAttribLocation(program_, "c2");
  glVertexAttribPointer(attr2, 2, GL_FLOAT, GL_FALSE, 0, kTexCoords);
  glEnableVertexAttribArray(attr2);

  int texture_sampler = glGetUniformLocation(program_, "texture");
  glUniform1i(texture_sampler, 0);
  glActiveTexture(GL_TEXTURE0);

  glGenTextures(kNumberOfTextures, textures_);
  for (int i = 0; i < kNumberOfTextures; ++i) {
    glBindTexture(GL_TEXTURE_2D, textures_[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  // Textures formats
  // TODO(djkurtz): Other formats such as GL_BGRA, GL_RGB, GL_BGR, ... ?
  const GLenum kTexelFormats[] =
     { GL_LUMINANCE, GL_RGBA }; // , GL_BGRA, GL_RGB, GL_BGR };
  const unsigned int kTexelFormatSizes[] = { 1, 4 }; // , 4, 3, 3 };
  const std::string kTexelFormatNames[] =
      { "luminance", "rgba" }; // , "bgra", "rgb", "bgr" };

  // Texture upload commands
  UpdateFlavor kFlavors[] = { TEX_IMAGE, TEX_SUBIMAGE };
  const std::string kFlavorNames[] = { "teximage2d", "texsubimage2d" };

  for (unsigned int fmt = 0; fmt < arraysize(kTexelFormats); fmt++) {
    texel_gl_format_ = kTexelFormats[fmt];
    unsigned int texel_size = kTexelFormatSizes[fmt];
    for (unsigned int flavor = 0; flavor < arraysize(kFlavors); flavor++) {
      flavor_ = kFlavors[flavor];
      const int sizes[] = { 32, 128, 256, 512, 768, 1024, 1536, 2048 };
      for (unsigned int j = 0; j < arraysize(sizes); j++) {
        // In hasty mode only do at most 512x512 sized problems.
        if (g_hasty && sizes[j] > 512)
          continue;

        std::string name = std::string(Name()) + "_" +
            kTexelFormatNames[fmt] + "_" + kFlavorNames[flavor] + "_" +
            base::IntToString(sizes[j]);

        width_ = height_ = sizes[j];
        const unsigned int buffer_size = width_ * height_ * texel_size;
        for (int i = 0; i < kNumberOfTextures; ++i) {
          pixels_[i].reset(new char[buffer_size]);
          memset(pixels_[i].get(), 255, buffer_size);

          // For NPOT texture we must set GL_TEXTURE_WRAP as GL_CLAMP_TO_EDGE
          glBindTexture(GL_TEXTURE_2D, textures_[i]);
          glTexImage2D(GL_TEXTURE_2D, 0, texel_gl_format_, width_, height_, 0,
                       texel_gl_format_, GL_UNSIGNED_BYTE, NULL);
          if (glGetError() != 0) {
            printf("# Error: Failed to allocate %dx%d %u-byte texel texture.\n",
                   width_, height_, texel_size);
          }
          if (IS_NOT_POWER_OF_2(width_) || IS_NOT_POWER_OF_2(height_)) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
          }
        }
        RunTest(this, name.c_str(), buffer_size, g_width, g_height, true);
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
          printf("# GL error code %d after RunTest() with %dx%d %d-byte texture.\n",
                 error, width_, height_, texel_size);
       }
      }
    }
  }
  for (int i = 0; i < kNumberOfTextures; ++i)
    pixels_[i].reset();

  glDeleteTextures(kNumberOfTextures, textures_);
  glDeleteProgram(program_);
  return true;
}

} // namespace glbench
