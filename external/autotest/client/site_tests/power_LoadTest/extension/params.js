// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
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

var test_time_ms = minutes(60);
var test_startup_delay = seconds(5);
var should_scroll = true;
var should_scroll_up = true;
var scroll_loop = false;
var scroll_interval_ms = 10000;
var scroll_by_pixels = 600;
