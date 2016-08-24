/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_GL_WORKER_H_
#define ANDROID_GL_WORKER_H_

#include <vector>

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <ui/GraphicBuffer.h>

#include "autogl.h"

namespace android {

struct DrmHwcLayer;
struct DrmCompositionRegion;

class GLWorkerCompositor {
 public:
  GLWorkerCompositor();
  ~GLWorkerCompositor();

  int Init();
  int Composite(DrmHwcLayer *layers, DrmCompositionRegion *regions,
                size_t num_regions, const sp<GraphicBuffer> &framebuffer);
  void Finish();

 private:
  struct CachedFramebuffer {
    // If the strong_framebuffer is non-NULL, we are holding a strong reference
    // until we are sure rendering is done. The weak reference will be equal in
    // that case.
    sp<GraphicBuffer> strong_framebuffer;
    wp<GraphicBuffer> weak_framebuffer;
    AutoEGLDisplayImage egl_fb_image;
    AutoGLTexture gl_fb_tex;
    AutoGLFramebuffer gl_fb;

    CachedFramebuffer(const sp<GraphicBuffer> &gb, AutoEGLDisplayImage &&image,
                      AutoGLTexture &&tex, AutoGLFramebuffer &&fb);

    bool Promote();
  };

  CachedFramebuffer *FindCachedFramebuffer(
      const sp<GraphicBuffer> &framebuffer);
  CachedFramebuffer *PrepareAndCacheFramebuffer(
      const sp<GraphicBuffer> &framebuffer);

  GLint PrepareAndCacheProgram(unsigned texture_count);

  EGLDisplay egl_display_;
  EGLContext egl_ctx_;

  std::vector<AutoGLProgram> blend_programs_;
  AutoGLBuffer vertex_buffer_;

  std::vector<CachedFramebuffer> cached_framebuffers_;
};
}

#endif
