// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This test evaluates the speed of updating a single texture and using it to
// draw after each upload.

#include "base/logging.h"

#include "texturetest.h"
#include "main.h"

namespace glbench {

class TextureUpdateTest : public TextureTest {
 public:
  TextureUpdateTest() {}
  virtual ~TextureUpdateTest() {}
  virtual bool TestFunc(uint64_t iterations);
  virtual const char* Name() const { return "texture_update"; }
  virtual bool IsDrawTest() const { return true; }
};

bool TextureUpdateTest::TestFunc(uint64_t iterations) {
  glGetError();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glFlush();
  for (uint64_t i = 0; i < iterations; ++i) {
    switch (flavor_) {
      case TEX_IMAGE:
        glTexImage2D(GL_TEXTURE_2D, 0, texel_gl_format_, width_, height_,
                     0, texel_gl_format_, GL_UNSIGNED_BYTE,
                     pixels_[i % kNumberOfTextures].get());
        break;
      case TEX_SUBIMAGE:
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_,
                        texel_gl_format_, GL_UNSIGNED_BYTE,
                        pixels_[i % kNumberOfTextures].get());
        break;
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
  return true;
}

TestBase* GetTextureUpdateTest() {
  return new TextureUpdateTest;
}

} // namespace glbench
