// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function createWindow(url, l, t, w, h, fullscreen) {
  chrome.windows.create(
      {left: l, top: t, width: w, height: h, focused: true, url: url},
      function(win) {
        if (fullscreen) {
          chrome.windows.update(win.id, {state: "fullscreen"});
        }
      });
}

function onMessageHandler(message, sender, sendResponse) {
  console.log("Background got message: " + message.method);
  if (!message.method)
    return;
  if (message.method == "createWindow") {
    console.log("Create window.");
    createWindow(message.url, message.left, message.top,
        message.width, message.height, message.fullscreen);
  } else if (message.method == "setFullscreen") {
    console.log("Set window " + sender.tab.windowId + " to fullscreen.");
    chrome.windows.update(sender.tab.windowId, {state: "fullscreen"});
  } else if (message.method == "updateWindow") {
    console.log("Update window " + sender.tab.windowId + ": " +
                message.updateInfo);
    chrome.windows.update(sender.tab.windowId, message.updateInfo);
  } else if (message.method == "moveAndSetFullscreen") {
    console.log("Move window " + sender.tab.windowId +
                " to external display and set it to fullscreen.");
    chrome.system.display.getInfo(function(info) {
        var internal_width = null;
        var i = 0;
        for (i = 0; i < info.length; i++) {
          if (info[i].isInternal) {
            internal_width = info[i].bounds.width;
          }
        }

        if (internal_width == null) {
          console.log('Cannot get internal display width.');
          return;
        }
        chrome.windows.update(sender.tab.windowId, {
            left: internal_width + 1,
            top: 0,
            width: 300,
            height: 300});
        chrome.windows.update(sender.tab.windowId, {state: "fullscreen"});
    });
  }
}

chrome.runtime.onMessage.addListener(onMessageHandler);
