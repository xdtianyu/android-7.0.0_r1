/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree.
 */

'use strict';

function isBlackFrame(data, length) {
  var accumulatedLuma = 0;
  var nonBlackPixelLumaThreshold = 20;
  for (var i = 4; i < length; i += 4) {
    // Use Luma as in Rec. 709: Y′709 = 0.21R + 0.72G + 0.07B;
    accumulatedLuma += (0.21 * data[i] +  0.72 * data[i + 1]
        + 0.07 * data[i + 2]);
    // Early termination if the average Luma so far is bright enough.
    if (accumulatedLuma > (nonBlackPixelLumaThreshold * i / 4)) {
      return false;
    }
  }
  return true;
}