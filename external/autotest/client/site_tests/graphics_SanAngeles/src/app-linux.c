/* San Angeles Observation OpenGL ES version example
 * Copyright 2004-2005 Jetro Lauha
 * All rights reserved.
 * Web: http://iki.fi/jetro/
 *
 * This source is free software; you can redistribute it and/or
 * modify it under the terms of EITHER:
 *   (1) The GNU Lesser General Public License as published by the Free
 *       Software Foundation; either version 2.1 of the License, or (at
 *       your option) any later version. The text of the GNU Lesser
 *       General Public License is included with this source in the
 *       file LICENSE-LGPL.txt.
 *   (2) The BSD-style license that is included with this source in
 *       the file LICENSE-BSD.txt.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the files
 * LICENSE-LGPL.txt and LICENSE-BSD.txt for more details.
 *
 * $Id: app-linux.c,v 1.4 2005/02/08 18:42:48 tonic Exp $
 * $Revision: 1.4 $
 *
 * Parts of this source file is based on test/example code from
 * GLESonGL implementation by David Blythe. Here is copy of the
 * license notice from that source:
 *
 * Copyright (C) 2003  David Blythe   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * DAVID BLYTHE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "waffle.h"

#ifdef SAN_ANGELES_OBSERVATION_GLES
#define GL_API WAFFLE_CONTEXT_OPENGL_ES2
#undef IMPORTGL_API
#undef IMPORTGL_FNPTRINIT
#include "importgl.h"
#else  // SAN_ANGELES_OBSERVATION_GLES
#define GL_API WAFFLE_CONTEXT_OPENGL
#undef IMPORTVBO_API
#undef IMPORTVBO_FNPTRINIT
#include "importvbo.h"
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES

#include "app.h"


int gAppAlive = 1;
static struct waffle_display *sDisplay;
static struct waffle_window *sWindow;
static struct waffle_config *sConfig;
static struct waffle_context *sContext;
static int sWindowWidth = WINDOW_DEFAULT_WIDTH;
static int sWindowHeight = WINDOW_DEFAULT_HEIGHT;
#ifdef SAN_ANGELES_OBSERVATION_GLES
static const char sAppName[] =
    "San Angeles Observation OpenGL ES version example (Linux)";
#else  // !SAN_ANGELES_OBSERVATION_GLES
static const char sAppName[] =
    "San Angeles Observation OpenGL version example (Linux)";
#endif  // SAN_ANGELES_OBSERVATION_GLES | !SAN_ANGELES_OBSERVATION_GLES

static void checkGLErrors()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
        fprintf(stderr, "Error: GL error code 0x%04x\n", (int)error);
}

static int waffleError(void)
{
    const struct waffle_error_info *info = waffle_error_get_info();
    const char *code = waffle_error_to_string(info->code);

    fprintf(stderr, "Error: %s", code);
    if (info->message_length > 0)
        fprintf(stderr, ": %s", info->message);
    fprintf(stderr, "\n");
    return 0;
}

// Initializes and opens both display and OpenGL/GLES.
static int initGraphics(int32_t platform)
{
    int32_t configAttribs[] =
    {
        WAFFLE_CONTEXT_API,     GL_API,
        WAFFLE_RED_SIZE,        5,
        WAFFLE_GREEN_SIZE,      5,
        WAFFLE_BLUE_SIZE,       5,
        WAFFLE_ALPHA_SIZE,      0,
        WAFFLE_DEPTH_SIZE,      16,
        WAFFLE_DOUBLE_BUFFERED, true,
        0
    };

    int32_t initAttribs[] =
    {
        WAFFLE_PLATFORM, platform,
        0
    };

    bool ok = waffle_init(initAttribs);
    if (!ok)
        return waffleError();

    sDisplay = waffle_display_connect(NULL);
    if (!sDisplay)
        return waffleError();

    sConfig = waffle_config_choose(sDisplay, configAttribs);
    if (!sConfig)
        return waffleError();

    sContext = waffle_context_create(sConfig, NULL);
    if (!sContext)
        return waffleError();

    sWindow = waffle_window_create(sConfig, sWindowWidth, sWindowHeight);
    if (!sWindow)
        return waffleError();

    if (!waffle_window_show(sWindow))
        return waffleError();

    ok = waffle_make_current(sDisplay, sWindow, sContext);
    if (!ok)
        return waffleError();

#ifdef SAN_ANGELES_OBSERVATION_GLES
#ifndef DISABLE_IMPORTGL
    int importGLResult;
    importGLResult = importGLInit();
    if (!importGLResult)
        return 0;
#endif  // !DISABLE_IMPORTGL
#endif  // SAN_ANGELES_OBSERVATION_GLES

    glEnable(GL_DEPTH_TEST);

    int rt = 1;
#ifndef SAN_ANGELES_OBSERVATION_GLES
    rt = loadVBOProcs();
#endif  // !SAN_ANGELES_OBSERVATION_GLES
    return rt;
}

static void deinitGraphics()
{
    if (!waffle_make_current(sDisplay, NULL, NULL))
        waffleError();
    if (!waffle_window_destroy(sWindow))
        waffleError();
    if (!waffle_context_destroy(sContext))
        waffleError();
    if (!waffle_config_destroy(sConfig))
        waffleError();
    if (!waffle_display_disconnect(sDisplay))
        waffleError();
}

#define PLATFORM(x) { #x, WAFFLE_PLATFORM_##x }

static struct platform_item {
    const char *name;
    int32_t value;
} platform_list[] = {
    PLATFORM(GLX),
    PLATFORM(X11_EGL),
    PLATFORM(GBM),
    PLATFORM(NULL),
    { NULL, 0 }
};

int main(int argc, char *argv[])
{
    // TODO(fjhenigman): add waffle_to_string_to_enum to waffle then use it
    // to parse the platform arg.
    int32_t platform_value = WAFFLE_NONE;
    struct platform_item *p = platform_list;
    while (argc == 2 && p->name && platform_value == WAFFLE_NONE) {
        if (!strcasecmp(argv[1], p->name))
            platform_value = p->value;
        ++p;
    }

    if (platform_value == WAFFLE_NONE)
    {
        fprintf(stderr, "Usage: SanOGLES <platform>\n");
        return EXIT_FAILURE;
    }

    if (!initGraphics(platform_value))
    {
        fprintf(stderr, "Error: Graphics initialization failed.\n");
        return EXIT_FAILURE;
    }

    if (!appInit())
    {
        fprintf(stderr, "Error: Application initialization failed.\n");
        return EXIT_FAILURE;
    }

    double total_time = 0.0;
    int num_frames = 0;

    while (1)
    {
        struct timeval timeNow, timeAfter;

        gettimeofday(&timeNow, NULL);
        appRender(TIME_SPEEDUP * (timeNow.tv_sec * 1000 +
                                  timeNow.tv_usec / 1000),
                  sWindowWidth, sWindowHeight);
        gettimeofday(&timeAfter, NULL);

#ifdef SAN_ANGELES_OBSERVATION_GLES
        checkGLErrors();
#endif

        if (!gAppAlive)
            break;

        if (!waffle_window_swap_buffers(sWindow))
            waffleError();

#ifndef SAN_ANGELES_OBSERVATION_GLES
        checkGLErrors();
#endif

        total_time += (timeAfter.tv_sec - timeNow.tv_sec) +
                      (timeAfter.tv_usec - timeNow.tv_usec) / 1000000.0;
        num_frames++;
    }

    appDeinit();
    deinitGraphics();

    fprintf(stdout, "frame_rate = %.1f\n", num_frames / total_time);

    return EXIT_SUCCESS;
}

