// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This test evaluates the speed of using the same textures to draw repeatedly.
// It uploads a series of textures initially.  On subsequent iterations, it uses
// those uploaded textures to draw.

#include "base/logging.h"

#include "texturetest.h"
#include "main.h"

namespace glbench {

class TextureReuseTest : public TextureTest {
 public:
  TextureReuseTest() {}
  virtual ~TextureReuseTest() {}
  virtual bool TestFunc(uint64_t iterations);
  virtual const char* Name() const { return "texture_reuse"; }
  virtual bool IsDrawTest() const { return true; }
};

bool TextureReuseTest::TestFunc(uint64_t iterations) {
  glGetError();

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glFlush();

  for (uint64_t i = 0; i < iterations; ++i) {
    glBindTexture(GL_TEXTURE_2D, textures_[i % kNumberOfTextures]);
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

    // After having uploaded |kNumberOfTextures| textures, use each of them to
    // draw once before uploading new textures.
    if ((i % kNumberOfTextures) == (kNumberOfTextures - 1)) {
      for (int j = 0; j < kNumberOfTextures; ++j) {
        glBindTexture(GL_TEXTURE_2D, textures_[j]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      }
    }
  }

  return true;
}

TestBase* GetTextureReuseTest() {
  return new TextureReuseTest;
}

} // namespace glbench
