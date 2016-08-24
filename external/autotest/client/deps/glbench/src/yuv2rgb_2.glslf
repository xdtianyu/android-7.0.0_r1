/*
 * Copyright 2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This is an optimized version of the YUV to RGB conversion shader.  This
 * shader assumes the height of U and V planes is odd, which means the height
 * of the original image is 2 modulo 4.
 */

uniform sampler2D textureSampler;
uniform sampler2D paritySampler;

varying vec2 lineCounter;
varying vec2 yPlane;
varying vec2 uPlane;
varying vec2 vPlane;

#if defined (USE_UNIFORM_MATRIX)
uniform mat4 conversion;
#endif

void main() {
  /*
   * If the height of the original image is even, offset_odd is not needed.
   */
  vec2 offset_even = vec2(texture2D(paritySampler, lineCounter).x * 0.5, 0.0);
  vec2 offset_odd = vec2(0.5 - offset_even.x, 0.0);

  float yChannel = texture2D(textureSampler, yPlane).x;
  float uChannel = texture2D(textureSampler, uPlane + offset_even).x;
  float vChannel = texture2D(textureSampler, vPlane + offset_odd).x;

  /*
   * This does the colorspace conversion from Y'UV to RGB as a matrix
   * multiply.  It also does the offset of the U and V channels from
   * [0,1] to [-.5,.5] as part of the transform.
   */
  vec4 channels = vec4(yChannel, uChannel, vChannel, 1.0);
#if !defined(USE_UNIFORM_MATRIX)
  mat4 conversion = mat4( 1.0,    1.0,    1.0,   0.0,
                          0.0,   -0.344,  1.772, 0.0,
                          1.402, -0.714,  0.0,   0.0,
                         -0.701,  0.529, -0.886, 1.0);
#endif

  gl_FragColor = conversion * channels;
}
