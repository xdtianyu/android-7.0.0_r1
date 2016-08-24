// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BENCH_GL_YUV2RGB_H_
#define BENCH_GL_YUV2RGB_H_

#define YUV2RGB_VERTEX_1 "yuv2rgb_1.glslv"
#define YUV2RGB_FRAGMENT_1 "yuv2rgb_1.glslf"

#define YUV2RGB_VERTEX_2 "yuv2rgb_2.glslv"
#define YUV2RGB_FRAGMENT_2 "yuv2rgb_2.glslf"

#define YUV2RGB_VERTEX_34 "yuv2rgb_34.glslv"
#define YUV2RGB_FRAGMENT_3 "yuv2rgb_3.glslf"
#define YUV2RGB_FRAGMENT_4 "yuv2rgb_4.glslf"

#define YUV2RGB_NAME "image.yuv"
#define YUV2RGB_WIDTH 720
// YUV2RGB_HEIGHT is total height, which is 3/2 of height of Y plane.
#define YUV2RGB_HEIGHT 729
#define YUV2RGB_PIXEL_HEIGHT (YUV2RGB_HEIGHT * 2 / 3)
#define YUV2RGB_SIZE (YUV2RGB_WIDTH * YUV2RGB_HEIGHT)

#endif
