// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Convert seconds to milliseconds
function seconds(s) {
    return s * 1000;
}

// Convert minutes to milliseconds
function minutes(m) {
    return seconds(m * 60);
}
