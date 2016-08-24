// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var gaia = gaia || {};
gaia.chromeOSLogin = {};

gaia.chromeOSLogin.parent_page_url_ =
'chrome-extension://mfffpogegjflfpflabcdkioaeobkgjik/main.html';

gaia.chromeOSLogin.attemptLogin = function(email, password, attemptToken) {
  var msg = {
    'method': 'attemptLogin',
    'email': email,
    'password': password,
    'attemptToken': attemptToken
  };
  window.parent.postMessage(msg, gaia.chromeOSLogin.parent_page_url_);
};

gaia.chromeOSLogin.clearOldAttempts = function() {
  var msg = {
    'method': 'clearOldAttempts'
  };
  window.parent.postMessage(msg, gaia.chromeOSLogin.parent_page_url_);
};

gaia.chromeOSLogin.onAttemptedLogin = function(emailFormElement,
                                               passwordFormElement,
                                               continueUrlElement) {
    var email = emailFormElement.value;
    var passwd = passwordFormElement.value;
    var attemptToken = new Date().getTime();

    gaia.chromeOSLogin.attemptLogin(email, passwd, attemptToken);

    if (continueUrlElement) {
      var prevAttemptIndex = continueUrlElement.value.indexOf('?attemptToken');
      if (prevAttemptIndex != -1) {
        continueUrlElement.value =
            continueUrlElement.value.substr(0, prevAttemptIndex);
      }
      continueUrlElement.value += '?attemptToken=' + attemptToken;
    }
}
