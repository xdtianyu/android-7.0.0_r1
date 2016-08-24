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

#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#define LOG_TAG "hwc-gl-worker"

#include <algorithm>
#include <string>
#include <sstream>
#include <unordered_set>

#include <sys/resource.h>

#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>

#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>

#include <utils/Trace.h>

#include "drmdisplaycomposition.h"

#include "glworker.h"

// TODO(zachr): use hwc_drm_bo to turn buffer handles into textures
#ifndef EGL_NATIVE_HANDLE_ANDROID_NVX
#define EGL_NATIVE_HANDLE_ANDROID_NVX 0x322A
#endif

#define MAX_OVERLAPPING_LAYERS 64

namespace android {

// clang-format off
// Column-major order:
// float mat[4] = { 1, 2, 3, 4 } ===
// [ 1 3 ]
// [ 2 4 ]
float kTextureTransformMatrices[] = {
   1.0f,  0.0f,  0.0f,  1.0f, // identity matrix
   0.0f,  1.0f,  1.0f,  0.0f, // swap x and y
};
// clang-format on

static const char *GetGLError(void) {
  switch (glGetError()) {
    case GL_NO_ERROR:
      return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
      return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
      return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
      return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
      return "GL_OUT_OF_MEMORY";
    default:
      return "Unknown error";
  }
}

static const char *GetGLFramebufferError(void) {
  switch (glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
    case GL_FRAMEBUFFER_COMPLETE:
      return "GL_FRAMEBUFFER_COMPLETE";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
    case GL_FRAMEBUFFER_UNSUPPORTED:
      return "GL_FRAMEBUFFER_UNSUPPORTED";
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
      return "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
    default:
      return "Unknown error";
  }
}

static const char *GetEGLError(void) {
  switch (eglGetError()) {
    case EGL_SUCCESS:
      return "EGL_SUCCESS";
    case EGL_NOT_INITIALIZED:
      return "EGL_NOT_INITIALIZED";
    case EGL_BAD_ACCESS:
      return "EGL_BAD_ACCESS";
    case EGL_BAD_ALLOC:
      return "EGL_BAD_ALLOC";
    case EGL_BAD_ATTRIBUTE:
      return "EGL_BAD_ATTRIBUTE";
    case EGL_BAD_CONTEXT:
      return "EGL_BAD_CONTEXT";
    case EGL_BAD_CONFIG:
      return "EGL_BAD_CONFIG";
    case EGL_BAD_CURRENT_SURFACE:
      return "EGL_BAD_CURRENT_SURFACE";
    case EGL_BAD_DISPLAY:
      return "EGL_BAD_DISPLAY";
    case EGL_BAD_SURFACE:
      return "EGL_BAD_SURFACE";
    case EGL_BAD_MATCH:
      return "EGL_BAD_MATCH";
    case EGL_BAD_PARAMETER:
      return "EGL_BAD_PARAMETER";
    case EGL_BAD_NATIVE_PIXMAP:
      return "EGL_BAD_NATIVE_PIXMAP";
    case EGL_BAD_NATIVE_WINDOW:
      return "EGL_BAD_NATIVE_WINDOW";
    case EGL_CONTEXT_LOST:
      return "EGL_CONTEXT_LOST";
    default:
      return "Unknown error";
  }
}

static bool HasExtension(const char *extension, const char *extensions) {
  const char *start, *where, *terminator;
  start = extensions;
  for (;;) {
    where = (char *)strstr((const char *)start, extension);
    if (!where)
      break;
    terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ')
      if (*terminator == ' ' || *terminator == '\0')
        return true;
    start = terminator;
  }
  return false;
}

static AutoGLShader CompileAndCheckShader(GLenum type, unsigned source_count,
                                          const GLchar **sources,
                                          std::ostringstream *shader_log) {
  GLint status;
  AutoGLShader shader(glCreateShader(type));
  if (shader.get() == 0) {
    if (shader_log)
      *shader_log << "Failed glCreateShader call";
    return 0;
  }

  glShaderSource(shader.get(), source_count, sources, NULL);
  glCompileShader(shader.get());
  glGetShaderiv(shader.get(), GL_COMPILE_STATUS, &status);
  if (!status) {
    if (shader_log) {
      GLint log_length;
      glGetShaderiv(shader.get(), GL_INFO_LOG_LENGTH, &log_length);
      std::string info_log(log_length, ' ');
      glGetShaderInfoLog(shader.get(), log_length, NULL, &info_log.front());
      *shader_log << "Failed to compile shader:\n" << info_log.c_str()
                  << "\nShader Source:\n";
      for (unsigned i = 0; i < source_count; i++) {
        *shader_log << sources[i];
      }
      *shader_log << "\n";
    }
    return 0;
  }

  return shader;
}

static std::string GenerateVertexShader(int layer_count) {
  std::ostringstream vertex_shader_stream;
  vertex_shader_stream
      << "#version 300 es\n"
      << "#define LAYER_COUNT " << layer_count << "\n"
      << "precision mediump int;\n"
      << "uniform vec4 uViewport;\n"
      << "uniform vec4 uLayerCrop[LAYER_COUNT];\n"
      << "uniform mat2 uTexMatrix[LAYER_COUNT];\n"
      << "in vec2 vPosition;\n"
      << "in vec2 vTexCoords;\n"
      << "out vec2 fTexCoords[LAYER_COUNT];\n"
      << "void main() {\n"
      << "  for (int i = 0; i < LAYER_COUNT; i++) {\n"
      << "    vec2 tempCoords = vTexCoords * uTexMatrix[i];\n"
      << "    fTexCoords[i] =\n"
      << "        uLayerCrop[i].xy + tempCoords * uLayerCrop[i].zw;\n"
      << "  }\n"
      << "  vec2 scaledPosition = uViewport.xy + vPosition * uViewport.zw;\n"
      << "  gl_Position =\n"
      << "      vec4(scaledPosition * vec2(2.0) - vec2(1.0), 0.0, 1.0);\n"
      << "}\n";
  return vertex_shader_stream.str();
}

static std::string GenerateFragmentShader(int layer_count) {
  std::ostringstream fragment_shader_stream;
  fragment_shader_stream << "#version 300 es\n"
                         << "#define LAYER_COUNT " << layer_count << "\n"
                         << "#extension GL_OES_EGL_image_external : require\n"
                         << "precision mediump float;\n";
  for (int i = 0; i < layer_count; ++i) {
    fragment_shader_stream << "uniform samplerExternalOES uLayerTexture" << i
                           << ";\n";
  }
  fragment_shader_stream << "uniform float uLayerAlpha[LAYER_COUNT];\n"
                         << "uniform float uLayerPremult[LAYER_COUNT];\n"
                         << "in vec2 fTexCoords[LAYER_COUNT];\n"
                         << "out vec4 oFragColor;\n"
                         << "void main() {\n"
                         << "  vec3 color = vec3(0.0, 0.0, 0.0);\n"
                         << "  float alphaCover = 1.0;\n"
                         << "  vec4 texSample;\n"
                         << "  vec3 multRgb;\n";
  for (int i = 0; i < layer_count; ++i) {
    if (i > 0)
      fragment_shader_stream << "  if (alphaCover > 0.5/255.0) {\n";
    // clang-format off
    fragment_shader_stream
        << "  texSample = texture2D(uLayerTexture" << i << ",\n"
        << "                        fTexCoords[" << i << "]);\n"
        << "  multRgb = texSample.rgb *\n"
        << "            max(texSample.a, uLayerPremult[" << i << "]);\n"
        << "  color += multRgb * uLayerAlpha[" << i << "] * alphaCover;\n"
        << "  alphaCover *= 1.0 - texSample.a * uLayerAlpha[" << i << "];\n";
    // clang-format on
  }
  for (int i = 0; i < layer_count - 1; ++i)
    fragment_shader_stream << "  }\n";
  fragment_shader_stream << "  oFragColor = vec4(color, 1.0 - alphaCover);\n"
                         << "}\n";
  return fragment_shader_stream.str();
}

static AutoGLProgram GenerateProgram(unsigned num_textures,
                                     std::ostringstream *shader_log) {
  std::string vertex_shader_string = GenerateVertexShader(num_textures);
  const GLchar *vertex_shader_source = vertex_shader_string.c_str();
  AutoGLShader vertex_shader = CompileAndCheckShader(
      GL_VERTEX_SHADER, 1, &vertex_shader_source, shader_log);
  if (!vertex_shader.get())
    return 0;

  std::string fragment_shader_string = GenerateFragmentShader(num_textures);
  const GLchar *fragment_shader_source = fragment_shader_string.c_str();
  AutoGLShader fragment_shader = CompileAndCheckShader(
      GL_FRAGMENT_SHADER, 1, &fragment_shader_source, shader_log);
  if (!fragment_shader.get())
    return 0;

  AutoGLProgram program(glCreateProgram());
  if (!program.get()) {
    if (shader_log)
      *shader_log << "Failed to create program: " << GetGLError() << "\n";
    return 0;
  }

  glAttachShader(program.get(), vertex_shader.get());
  glAttachShader(program.get(), fragment_shader.get());
  glBindAttribLocation(program.get(), 0, "vPosition");
  glBindAttribLocation(program.get(), 1, "vTexCoords");
  glLinkProgram(program.get());
  glDetachShader(program.get(), vertex_shader.get());
  glDetachShader(program.get(), fragment_shader.get());

  GLint status;
  glGetProgramiv(program.get(), GL_LINK_STATUS, &status);
  if (!status) {
    if (shader_log) {
      GLint log_length;
      glGetProgramiv(program.get(), GL_INFO_LOG_LENGTH, &log_length);
      std::string program_log(log_length, ' ');
      glGetProgramInfoLog(program.get(), log_length, NULL,
                          &program_log.front());
      *shader_log << "Failed to link program:\n" << program_log.c_str() << "\n";
    }
    return 0;
  }

  return program;
}

struct RenderingCommand {
  struct TextureSource {
    unsigned texture_index;
    float crop_bounds[4];
    float alpha;
    float premult;
    float texture_matrix[4];
  };

  float bounds[4];
  unsigned texture_count = 0;
  TextureSource textures[MAX_OVERLAPPING_LAYERS];
};

static void ConstructCommand(const DrmHwcLayer *layers,
                             const DrmCompositionRegion &region,
                             RenderingCommand &cmd) {
  std::copy_n(region.frame.bounds, 4, cmd.bounds);

  for (size_t texture_index : region.source_layers) {
    const DrmHwcLayer &layer = layers[texture_index];

    DrmHwcRect<float> display_rect(layer.display_frame);
    float display_size[2] = {display_rect.bounds[2] - display_rect.bounds[0],
                             display_rect.bounds[3] - display_rect.bounds[1]};

    float tex_width = layer.buffer->width;
    float tex_height = layer.buffer->height;
    DrmHwcRect<float> crop_rect(layer.source_crop.left / tex_width,
                                layer.source_crop.top / tex_height,
                                layer.source_crop.right / tex_width,
                                layer.source_crop.bottom / tex_height);

    float crop_size[2] = {crop_rect.bounds[2] - crop_rect.bounds[0],
                          crop_rect.bounds[3] - crop_rect.bounds[1]};

    RenderingCommand::TextureSource &src = cmd.textures[cmd.texture_count];
    cmd.texture_count++;
    src.texture_index = texture_index;

    bool swap_xy = false;
    bool flip_xy[2] = { false, false };

    if (layer.transform == DrmHwcTransform::kRotate180) {
      swap_xy = false;
      flip_xy[0] = true;
      flip_xy[1] = true;
    } else if (layer.transform == DrmHwcTransform::kRotate270) {
      swap_xy = true;
      flip_xy[0] = true;
      flip_xy[1] = false;
    } else if (layer.transform & DrmHwcTransform::kRotate90) {
      swap_xy = true;
      if (layer.transform & DrmHwcTransform::kFlipH) {
        flip_xy[0] = true;
        flip_xy[1] = true;
      } else if (layer.transform & DrmHwcTransform::kFlipV) {
        flip_xy[0] = false;
        flip_xy[1] = false;
      } else {
        flip_xy[0] = false;
        flip_xy[1] = true;
      }
    } else {
      if (layer.transform & DrmHwcTransform::kFlipH)
        flip_xy[0] = true;
      if (layer.transform & DrmHwcTransform::kFlipV)
        flip_xy[1] = true;
    }

    if (swap_xy)
      std::copy_n(&kTextureTransformMatrices[4], 4, src.texture_matrix);
    else
      std::copy_n(&kTextureTransformMatrices[0], 4, src.texture_matrix);

    for (int j = 0; j < 4; j++) {
      int b = j ^ (swap_xy ? 1 : 0);
      float bound_percent =
          (cmd.bounds[b] - display_rect.bounds[b % 2]) / display_size[b % 2];
      if (flip_xy[j % 2]) {
        src.crop_bounds[j] =
            crop_rect.bounds[j % 2 + 2] - bound_percent * crop_size[j % 2];
      } else {
        src.crop_bounds[j] =
            crop_rect.bounds[j % 2] + bound_percent * crop_size[j % 2];
      }
    }

    if (layer.blending == DrmHwcBlending::kNone) {
      src.alpha = src.premult = 1.0f;
      // This layer is opaque. There is no point in using layers below this one.
      break;
    }

    src.alpha = layer.alpha / 255.0f;
    src.premult = (layer.blending == DrmHwcBlending::kPreMult) ? 1.0f : 0.0f;
  }
}

static int EGLFenceWait(EGLDisplay egl_display, int acquireFenceFd) {
  int ret = 0;

  EGLint attribs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, acquireFenceFd,
                      EGL_NONE};
  EGLSyncKHR egl_sync =
      eglCreateSyncKHR(egl_display, EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
  if (egl_sync == EGL_NO_SYNC_KHR) {
    ALOGE("Failed to make EGLSyncKHR from acquireFenceFd: %s", GetEGLError());
    close(acquireFenceFd);
    return 1;
  }

  EGLint success = eglWaitSyncKHR(egl_display, egl_sync, 0);
  if (success == EGL_FALSE) {
    ALOGE("Failed to wait for acquire: %s", GetEGLError());
    ret = 1;
  }
  eglDestroySyncKHR(egl_display, egl_sync);

  return ret;
}

static int CreateTextureFromHandle(EGLDisplay egl_display,
                                   buffer_handle_t handle,
                                   AutoEGLImageAndGLTexture *out) {
  EGLImageKHR image = eglCreateImageKHR(
      egl_display, EGL_NO_CONTEXT, EGL_NATIVE_HANDLE_ANDROID_NVX,
      (EGLClientBuffer)handle, NULL /* no attribs */);

  if (image == EGL_NO_IMAGE_KHR) {
    ALOGE("Failed to make image %s %p", GetEGLError(), handle);
    return -EINVAL;
  }

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
  glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)image);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

  out->image.reset(egl_display, image);
  out->texture.reset(texture);

  return 0;
}

GLWorkerCompositor::GLWorkerCompositor()
    : egl_display_(EGL_NO_DISPLAY), egl_ctx_(EGL_NO_CONTEXT) {
}

int GLWorkerCompositor::Init() {
  int ret = 0;
  const char *egl_extensions;
  const char *gl_extensions;
  EGLint num_configs;
  EGLint attribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE, EGL_NONE};
  EGLConfig egl_config;

  // clang-format off
  const GLfloat verts[] = {
    0.0f,  0.0f,    0.0f, 0.0f,
    0.0f,  2.0f,    0.0f, 2.0f,
    2.0f,  0.0f,    2.0f, 0.0f
  };
  // clang-format on

  const EGLint config_attribs[] = {EGL_RENDERABLE_TYPE,
                                   EGL_OPENGL_ES2_BIT,
                                   EGL_RED_SIZE,
                                   8,
                                   EGL_GREEN_SIZE,
                                   8,
                                   EGL_BLUE_SIZE,
                                   8,
                                   EGL_NONE};

  const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};

  egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (egl_display_ == EGL_NO_DISPLAY) {
    ALOGE("Failed to get egl display");
    return 1;
  }

  if (!eglInitialize(egl_display_, NULL, NULL)) {
    ALOGE("Failed to initialize egl: %s", GetEGLError());
    return 1;
  }

  egl_extensions = eglQueryString(egl_display_, EGL_EXTENSIONS);

  // These extensions are all technically required but not always reported due
  // to meta EGL filtering them out.
  if (!HasExtension("EGL_KHR_image_base", egl_extensions))
    ALOGW("EGL_KHR_image_base extension not supported");

  if (!HasExtension("EGL_ANDROID_image_native_buffer", egl_extensions))
    ALOGW("EGL_ANDROID_image_native_buffer extension not supported");

  if (!HasExtension("EGL_ANDROID_native_fence_sync", egl_extensions))
    ALOGW("EGL_ANDROID_native_fence_sync extension not supported");

  if (!eglChooseConfig(egl_display_, config_attribs, &egl_config, 1,
                       &num_configs)) {
    ALOGE("eglChooseConfig() failed with error: %s", GetEGLError());
    return 1;
  }

  egl_ctx_ =
      eglCreateContext(egl_display_, egl_config,
                       EGL_NO_CONTEXT /* No shared context */, context_attribs);

  if (egl_ctx_ == EGL_NO_CONTEXT) {
    ALOGE("Failed to create OpenGL ES Context: %s", GetEGLError());
    return 1;
  }

  if (!eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_ctx_)) {
    ALOGE("Failed to make the OpenGL ES Context current: %s", GetEGLError());
    return 1;
  }

  gl_extensions = (const char *)glGetString(GL_EXTENSIONS);

  if (!HasExtension("GL_OES_EGL_image", gl_extensions))
    ALOGW("GL_OES_EGL_image extension not supported");

  if (!HasExtension("GL_OES_EGL_image_external", gl_extensions))
    ALOGW("GL_OES_EGL_image_external extension not supported");

  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  vertex_buffer_.reset(vertex_buffer);

  std::ostringstream shader_log;
  blend_programs_.emplace_back(GenerateProgram(1, &shader_log));
  if (blend_programs_.back().get() == 0) {
    ALOGE("%s", shader_log.str().c_str());
    return 1;
  }

  return 0;
}

GLWorkerCompositor::~GLWorkerCompositor() {
  if (egl_display_ != EGL_NO_DISPLAY && egl_ctx_ != EGL_NO_CONTEXT)
    if (eglDestroyContext(egl_display_, egl_ctx_) == EGL_FALSE)
      ALOGE("Failed to destroy OpenGL ES Context: %s", GetEGLError());
}

int GLWorkerCompositor::Composite(DrmHwcLayer *layers,
                                  DrmCompositionRegion *regions,
                                  size_t num_regions,
                                  const sp<GraphicBuffer> &framebuffer) {
  ATRACE_CALL();
  int ret = 0;
  std::vector<AutoEGLImageAndGLTexture> layer_textures;
  std::vector<RenderingCommand> commands;

  if (num_regions == 0) {
    return -EALREADY;
  }

  GLint frame_width = framebuffer->getWidth();
  GLint frame_height = framebuffer->getHeight();
  CachedFramebuffer *cached_framebuffer =
      PrepareAndCacheFramebuffer(framebuffer);
  if (cached_framebuffer == NULL) {
    ALOGE("Composite failed because of failed framebuffer");
    return -EINVAL;
  }

  std::unordered_set<size_t> layers_used_indices;
  for (size_t region_index = 0; region_index < num_regions; region_index++) {
    DrmCompositionRegion &region = regions[region_index];
    layers_used_indices.insert(region.source_layers.begin(),
                               region.source_layers.end());
    commands.emplace_back();
    ConstructCommand(layers, region, commands.back());
  }

  for (size_t layer_index = 0; layer_index < MAX_OVERLAPPING_LAYERS;
       layer_index++) {
    DrmHwcLayer *layer = &layers[layer_index];

    layer_textures.emplace_back();

    if (layers_used_indices.count(layer_index) == 0)
      continue;

    ret = CreateTextureFromHandle(egl_display_, layer->get_usable_handle(),
                                  &layer_textures.back());

    if (!ret) {
      ret = EGLFenceWait(egl_display_, layer->acquire_fence.Release());
    }
    if (ret) {
      layer_textures.pop_back();
      ret = -EINVAL;
    }
  }

  if (ret)
    return ret;

  glViewport(0, 0, frame_width, frame_height);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_.get());
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, NULL);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4,
                        (void *)(sizeof(float) * 2));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnable(GL_SCISSOR_TEST);

  for (const RenderingCommand &cmd : commands) {
    if (cmd.texture_count == 0)
      continue;

    // TODO(zachr): handle the case of too many overlapping textures for one
    // area by falling back to rendering as many layers as possible using
    // multiple blending passes.
    GLint program = PrepareAndCacheProgram(cmd.texture_count);
    if (program == 0) {
      ALOGE("Too many layers to render in one area");
      continue;
    }

    glUseProgram(program);
    GLint gl_viewport_loc = glGetUniformLocation(program, "uViewport");
    GLint gl_crop_loc = glGetUniformLocation(program, "uLayerCrop");
    GLint gl_alpha_loc = glGetUniformLocation(program, "uLayerAlpha");
    GLint gl_premult_loc = glGetUniformLocation(program, "uLayerPremult");
    GLint gl_tex_matrix_loc = glGetUniformLocation(program, "uTexMatrix");
    glUniform4f(gl_viewport_loc, cmd.bounds[0] / (float)frame_width,
                cmd.bounds[1] / (float)frame_height,
                (cmd.bounds[2] - cmd.bounds[0]) / (float)frame_width,
                (cmd.bounds[3] - cmd.bounds[1]) / (float)frame_height);

    for (unsigned src_index = 0; src_index < cmd.texture_count; src_index++) {
      std::ostringstream texture_name_formatter;
      texture_name_formatter << "uLayerTexture" << src_index;
      GLint gl_tex_loc =
          glGetUniformLocation(program, texture_name_formatter.str().c_str());

      const RenderingCommand::TextureSource &src = cmd.textures[src_index];
      glUniform1f(gl_alpha_loc + src_index, src.alpha);
      glUniform1f(gl_premult_loc + src_index, src.premult);
      glUniform4f(gl_crop_loc + src_index, src.crop_bounds[0],
                  src.crop_bounds[1], src.crop_bounds[2] - src.crop_bounds[0],
                  src.crop_bounds[3] - src.crop_bounds[1]);
      glUniform1i(gl_tex_loc, src_index);
      glUniformMatrix2fv(gl_tex_matrix_loc + src_index, 1, GL_FALSE,
                         src.texture_matrix);
      glActiveTexture(GL_TEXTURE0 + src_index);
      glBindTexture(GL_TEXTURE_EXTERNAL_OES,
                    layer_textures[src.texture_index].texture.get());
    }

    glScissor(cmd.bounds[0], cmd.bounds[1], cmd.bounds[2] - cmd.bounds[0],
              cmd.bounds[3] - cmd.bounds[1]);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    for (unsigned src_index = 0; src_index < cmd.texture_count; src_index++) {
      glActiveTexture(GL_TEXTURE0 + src_index);
      glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    }
  }

  glDisable(GL_SCISSOR_TEST);
  glActiveTexture(GL_TEXTURE0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return ret;
}

void GLWorkerCompositor::Finish() {
  ATRACE_CALL();
  glFinish();

  char use_framebuffer_cache_opt[PROPERTY_VALUE_MAX];
  property_get("hwc.drm.use_framebuffer_cache", use_framebuffer_cache_opt, "1");
  bool use_framebuffer_cache = atoi(use_framebuffer_cache_opt);

  if (use_framebuffer_cache) {
    for (auto &fb : cached_framebuffers_)
      fb.strong_framebuffer.clear();
  } else {
    cached_framebuffers_.clear();
  }
}

GLWorkerCompositor::CachedFramebuffer::CachedFramebuffer(
    const sp<GraphicBuffer> &gb, AutoEGLDisplayImage &&image,
    AutoGLTexture &&tex, AutoGLFramebuffer &&fb)
    : strong_framebuffer(gb),
      weak_framebuffer(gb),
      egl_fb_image(std::move(image)),
      gl_fb_tex(std::move(tex)),
      gl_fb(std::move(fb)) {
}

bool GLWorkerCompositor::CachedFramebuffer::Promote() {
  if (strong_framebuffer.get() != NULL)
    return true;
  strong_framebuffer = weak_framebuffer.promote();
  return strong_framebuffer.get() != NULL;
}

GLWorkerCompositor::CachedFramebuffer *
GLWorkerCompositor::FindCachedFramebuffer(
    const sp<GraphicBuffer> &framebuffer) {
  for (auto &fb : cached_framebuffers_)
    if (fb.weak_framebuffer == framebuffer)
      return &fb;
  return NULL;
}

GLWorkerCompositor::CachedFramebuffer *
GLWorkerCompositor::PrepareAndCacheFramebuffer(
    const sp<GraphicBuffer> &framebuffer) {
  CachedFramebuffer *cached_framebuffer = FindCachedFramebuffer(framebuffer);
  if (cached_framebuffer != NULL) {
    if (cached_framebuffer->Promote()) {
      glBindFramebuffer(GL_FRAMEBUFFER, cached_framebuffer->gl_fb.get());
      return cached_framebuffer;
    }

    for (auto it = cached_framebuffers_.begin();
         it != cached_framebuffers_.end(); ++it) {
      if (it->weak_framebuffer == framebuffer) {
        cached_framebuffers_.erase(it);
        break;
      }
    }
  }

  AutoEGLDisplayImage egl_fb_image(
      egl_display_,
      eglCreateImageKHR(egl_display_, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                        (EGLClientBuffer)framebuffer->getNativeBuffer(),
                        NULL /* no attribs */));

  if (egl_fb_image.image() == EGL_NO_IMAGE_KHR) {
    ALOGE("Failed to make image from target buffer: %s", GetEGLError());
    return NULL;
  }

  GLuint gl_fb_tex;
  glGenTextures(1, &gl_fb_tex);
  AutoGLTexture gl_fb_tex_auto(gl_fb_tex);
  glBindTexture(GL_TEXTURE_2D, gl_fb_tex);
  glEGLImageTargetTexture2DOES(GL_TEXTURE_2D,
                               (GLeglImageOES)egl_fb_image.image());
  glBindTexture(GL_TEXTURE_2D, 0);

  GLuint gl_fb;
  glGenFramebuffers(1, &gl_fb);
  AutoGLFramebuffer gl_fb_auto(gl_fb);
  glBindFramebuffer(GL_FRAMEBUFFER, gl_fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         gl_fb_tex, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    ALOGE("Failed framebuffer check for created target buffer: %s",
          GetGLFramebufferError());
    return NULL;
  }

  cached_framebuffers_.emplace_back(framebuffer, std::move(egl_fb_image),
                                    std::move(gl_fb_tex_auto),
                                    std::move(gl_fb_auto));
  return &cached_framebuffers_.back();
}

GLint GLWorkerCompositor::PrepareAndCacheProgram(unsigned texture_count) {
  if (blend_programs_.size() >= texture_count) {
    GLint program = blend_programs_[texture_count - 1].get();
    if (program != 0)
      return program;
  }

  AutoGLProgram program = GenerateProgram(texture_count, NULL);
  if (program.get() != 0) {
    if (blend_programs_.size() < texture_count)
      blend_programs_.resize(texture_count);
    blend_programs_[texture_count - 1] = std::move(program);
    return blend_programs_[texture_count - 1].get();
  }

  return 0;
}

}  // namespace android
