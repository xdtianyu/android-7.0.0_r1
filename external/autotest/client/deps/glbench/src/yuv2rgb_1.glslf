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
 * This is a conversion of a conversion of a cg shader from Chrome:
 * http://src.chromium.org/viewvc/chrome/trunk/src/o3d/samples/shaders/yuv2rgb.shader
 */

/*
 * This shader takes a Y'UV420p image as a single greyscale plane, and
 * converts it to RGB by sampling the correct parts of the image, and
 * by converting the colorspace to RGB on the fly.
 */

/*
 * These represent the image dimensions of the SOURCE IMAGE (not the
 * Y'UV420p image).  This is the same as the dimensions of the Y'
 * portion of the Y'UV420p image.  They are set from JavaScript.
 */
uniform float imageWidth;
uniform float imageHeight;

/*
 * This is the texture sampler where the greyscale Y'UV420p image is
 * accessed.
 */
uniform sampler2D textureSampler;

#if defined (USE_UNIFORM_MATRIX)
uniform mat4 conversion;
#endif

varying vec4 v1;

/**
 * Given the texture coordinates, our pixel shader grabs the right
 * value from each channel of the source image, converts it from Y'UV
 * to RGB, and returns the result.
 *
 * Each Y texel provides luminance information for one pixel in the image.
 * Each U and V texel provides color information for a 2x2 block of pixels.
 * The U and V texels are just appended to the Y texels.
 *
 * For images that have a height divisible by 4, things work out nicely.
 * For images that are merely divisible by 2, it's not so nice
 * (and YUV420 doesn't work for image sizes not divisible by 2).
 *
 * Here is a 6x6 image, with the layout of the planes of U and V.
 * Notice that the V plane starts halfway through the last scanline
 * that has U on it.
 *
 * 0  +---+---+---+---+---+---+
 *    | Y0| Y0| Y1| Y1| Y2| Y2|
 *    +---+---+---+---+---+---+
 *    | Y0| Y0| Y1| Y1| Y2| Y2|
 *    +---+---+---+---+---+---+
 *    | Y3| Y3| Y4| Y4| Y5| Y5|
 *    +---+---+---+---+---+---+
 *    | Y3| Y3| Y4| Y4| Y5| Y5|
 *    +---+---+---+---+---+---+
 *    | Y6| Y6| Y7| Y7| Y8| Y8|
 *    +---+---+---+---+---+---+
 *    | Y6| Y6| Y7| Y7| Y8| Y8|
 *2/3 +---+---+---+---+---+---+
 *    | U0| U1| U2| U3| U4| U5|
 *    +---+---+---+---+---+---+
 *5/6 | U6| U7| U8| V0| V1| V2|
 *    +---+---+---+---+---+---+
 *    | V3| V4| V5| V6| V7| V8|
 * 1  +---+---+---+---+---+---+
 *    0          0.5          1
 * 
 * Here is a 4x4 image, where the U and V planes are nicely split into
 * separable blocks.
 *
 * 0  +---+---+---+---+
 *    | Y0| Y0| Y1| Y1|
 *    +---+---+---+---+
 *    | Y0| Y0| Y1| Y1|
 *    +---+---+---+---+
 *    | Y2| Y2| Y3| Y3|
 *    +---+---+---+---+
 *    | Y2| Y2| Y3| Y3|
 *2/3 +---+---+---+---+
 *    | U0| U1| U2| U3|
 *5/6 +---+---+---+---+
 *    | V0| V1| V2| V3|
 * 1  +---+---+---+---+
 *    0      0.5      1
 *
 * The number in a cell indicates which U and V values map onto
 * the cell: Un and Vn are used to color the four 'n' cells.  As the
 * image is drawn its texture coordinates range from 0 to 1.  The 'y'
 * coordinate is scaled by 2/3 to map from the Y texels, scaled by 1/6
 * and shifted down 2/3 to map from the U texels, and scaled by 1/6
 * and shifted down 5/6 to map from the V texels.  To map from U or V
 * texels the 'x' coordinate is scaled by 1/2 always and shifted right
 * 1/2 when needed.  For example rows 0 and 1 use left side U texels
 * (U0-U2 in the first example) while rows 2 and 3 right side U texels
 * (U3-U5 in the first example), and so on for the remaining rows.
 * When the image height is a multiple of 4, the 'V side' is the same
 * as the 'U side,' otherwise it is opposite.
*/


void main() {
  float uside, vside;

  // texture origin at top left, vertex origin at bottom left
  vec2 t = vec2(v1.x, (1. - v1.y));

  // y position in pixels
  float ypixel = floor(t.y * imageHeight);

  if (mod(ypixel, 4.) < 2.) {
    // rows 0-1, U on left side
    uside = 0.;
  } else {
    // rows 2-3, U on right side
    uside = .5;
  }

  if (mod(imageHeight, 4.) == 0.) {
    // multiple of 4, V same side as U
    vside = uside;
  } else {
    // assume multiple of 2, V opposite side to U
    vside = .5 - uside;
  }

  // shrink y tex. coord. by 2/3 to cover Y section
  vec2 y = t * vec2(1., 2./3.);

  // for U and V shrink x tex. coord. by 0.5, y by 1/6
  t *= vec2(.5, 1./6.);

  // shift to proper side and translate down...
  vec2 u = t + vec2(uside, 2./3.); // ...to U section
  vec2 v = t + vec2(vside, 5./6.); // ...to V section

  float yChannel = texture2D(textureSampler, y).x;
  float uChannel = texture2D(textureSampler, u).x;
  float vChannel = texture2D(textureSampler, v).x;

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
