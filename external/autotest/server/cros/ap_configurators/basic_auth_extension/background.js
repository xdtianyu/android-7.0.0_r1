// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Script that automatically listend for an onAuthRequired request and sends
// hardcoded credentials back.

var gPendingCallbacks = [];
var bkg = chrome.extension.getBackgroundPage();

bkg.console.log("Listening")
chrome.webRequest.onAuthRequired.addListener(handleAuthRequest,
                                             {urls: ["<all_urls>"]},
                                             ["asyncBlocking"]);

function processPendingCallbacks() {
  bkg.console.log("Calling back with credentials");
  var callback = gPendingCallbacks.pop();
  callback({authCredentials: {username: 'admin', password: 'password'}});
}

function handleAuthRequest(details, callback) {
  gPendingCallbacks.push(callback);
  processPendingCallbacks();
}


