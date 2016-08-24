// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Application that displays graphics using OpenGL [ES] with the intent
// of being used in functional tests.

#include <gflags/gflags.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cmath>

#include "glinterface.h"
#include "main.h"
#include "utils.h"


GLuint GenerateAndBindTexture() {
  GLuint name = ~0;
  glGenTextures(1, &name);
  glBindTexture(GL_TEXTURE_2D, name);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  return name;
}

unsigned char* CreateBitmap(int w, int h) {
  unsigned char* bitmap = new unsigned char[4 * w * h];
  unsigned char* pixel = bitmap;
  float w2 = 0.5f * w;
  float h2 = 0.5f * h;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      // Fill with soft ellipse
      float dx = fabs((x - w2) / w2);
      float dy = fabs((y - h2) / h2);
      float dist2 = dx*dx + dy*dy;
      if (dist2 > 1.f)
        dist2 = 1.f;
      *pixel = (1.f - dist2) * 255.f;
      pixel++;
      *pixel = (1.f - dist2) * 255.f;
      pixel++;
      *pixel = (1.f - dist2) * 255.f;
      pixel++;
      *pixel = 0;
      pixel++;
    }
  }
  return bitmap;
}


const char kVertexShader[] =
    "attribute vec4 vertices;"
    "varying vec2 v1;"
    "void main() {"
    "    gl_Position = vec4(vertices.x, vertices.y, 0.0, 1.0);"
    "    v1 = vec2(0.5 * vertices.x + 0.5, 0.5 * vertices.y + 0.5);"
    "}";

const char kFragmentShader[] =
    "uniform sampler2D tex;"
    "uniform vec4 color;"
    "varying vec2 v1;"
    "void main() {"
    "    gl_FragColor = color * texture2D(tex, v1);"
    "}";

// Command line flags
DEFINE_double(screenshot1_sec, 2.f, "seconds delay before screenshot1_cmd");
DEFINE_double(screenshot2_sec, 1.f, "seconds delay before screenshot2_cmd");
DEFINE_string(screenshot1_cmd, "", "system command to take a screen shot 1");
DEFINE_string(screenshot2_cmd, "", "system command to take a screen shot 2");
DEFINE_double(cooldown_sec, 1.f, "seconds delay after all screenshots");

int main(int argc, char* argv[]) {
  // Configure full screen
  g_width = -1;
  g_height = -1;

  google::ParseCommandLineFlags(&argc, &argv, true);

  g_main_gl_interface.reset(GLInterface::Create());
  if (!g_main_gl_interface->Init()) {
    printf("# Error: Failed to initialize %s.\n", argv[0]);
    return 1;
  }

  GLint viewport[2];
  glGetIntegerv(GL_MAX_VIEWPORT_DIMS, viewport);
  printf("# MAX_VIEWPORT_DIMS=(%d, %d)\n", viewport[0], viewport[1]);
  if (viewport[0] < g_width || viewport[1] < g_height) {
    printf("# Error: MAX_VIEWPORT_DIMS too small\n");
    return 1;
  }
  glViewport(0, 0, g_width, g_height);

  unsigned char* bitmap = CreateBitmap(g_height, g_width);
  GLuint texture = GenerateAndBindTexture();
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_height, g_width, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, bitmap);

  GLfloat vertices[8] = {
    -1.f, -1.f,
    1.f, -1.f,
    -1.f, 1.f,
    1.f, 1.f,
  };

  GLuint program = glbench::InitShaderProgram(kVertexShader, kFragmentShader);
  int attribute_index = glGetAttribLocation(program, "vertices");
  glVertexAttribPointer(attribute_index, 2, GL_FLOAT, GL_FALSE, 0, vertices);
  glEnableVertexAttribArray(attribute_index);

  int texture_sampler = glGetUniformLocation(program, "tex");
  glUniform1i(texture_sampler, 0);

  int display_color = glGetUniformLocation(program, "color");
  float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float blue[4] = {0.5f, 0.5f, 1.0f, 1.0f};

  uint64_t last_event_time = GetUTime();
  enum State {
    kStateScreenShot1,
    kStateScreenShot2,
    kStateCooldown,
    kStateExit
  } state = kStateScreenShot1;
  float seconds_delay_for_next_state[] = {
      static_cast<float>(FLAGS_screenshot1_sec),
      static_cast<float>(FLAGS_screenshot2_sec),
      static_cast<float>(FLAGS_cooldown_sec),
      0
  };

  do {
    // Draw
    glClear(GL_COLOR_BUFFER_BIT);
    if (state == kStateScreenShot1)
      glUniform4fv(display_color, 1, white);
    else
      glUniform4fv(display_color, 1, blue);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    g_main_gl_interface->SwapBuffers();
    // Loop until next event
    float seconds_since_last_event =
        static_cast<float>(GetUTime() - last_event_time) / 1000000ULL;
    if (seconds_since_last_event < seconds_delay_for_next_state[state])
      continue;

    // State change. Perform action.
    switch(state) {
      case kStateScreenShot1:
        system(FLAGS_screenshot1_cmd.c_str());
        break;
      case kStateScreenShot2:
        system(FLAGS_screenshot2_cmd.c_str());
        break;
      default:
        break;
    }

    // Advance to next state
    last_event_time = GetUTime();
    state = static_cast<State>(state + 1);

  } while (state != kStateExit);

  glDeleteTextures(1, &texture);
  g_main_gl_interface->Cleanup();
  return 0;
}
