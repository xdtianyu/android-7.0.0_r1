// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

request = {action: "should_scroll"}

chrome.runtime.sendMessage(request, function(response) {
  if (response.should_scroll) {
    window.focus();
    lastOffset = window.pageYOffset;
    var start_interval = Math.max(10000, response.scroll_interval);
    function smoothScrollDown() {
      window.scrollBy(0, response.scroll_by);
      if (window.pageYOffset != lastOffset) {
        lastOffset = window.pageYOffset;
        setTimeout(smoothScrollDown, response.scroll_interval);
      } else if (response.should_scroll_up) {
        setTimeout(smoothScrollUp, start_interval);
      }
    }
    function smoothScrollUp() {
      window.scrollBy(0, -1 * response.scroll_by);
      if (window.pageYOffset != lastOffset) {
        lastOffset = window.pageYOffset;
        setTimeout(smoothScrollUp, response.scroll_interval);
      } else if (response.scroll_loop) {
        setTimeout(smoothScrollDown, start_interval);
      }
    }
    setTimeout(smoothScrollDown, start_interval);
  }
});

