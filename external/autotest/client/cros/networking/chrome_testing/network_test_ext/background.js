// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// chromeTesting.Networking provides wrappers around chrome.networkingPrivate
// functions. The result of each asynchronous call can be accessed through
// chromeTesting.networking.callStatus, which is a dictionary of the form:
//    {
//      <function name>: {
//        "status": <STATUS_PENDING|STATUS_SUCCESS|STATUS_FAILURE>,
//        "result": <Return value or null>,
//        "error": <Error message or null>,
//      },
//      ...
//    }

function Networking() {
  this.callStatus = {};
}

// Returns false if a call |function_name| is pending, otherwise sets up
// the function dictionary and sets the status to STATUS_PENDING.
Networking.prototype._setupFunctionCall = function(function_name) {
  if (this.callStatus[function_name] == null)
    this.callStatus[function_name] = {};
  if (this.callStatus[function_name].status == chromeTesting.STATUS_PENDING)
    return false;
  this.callStatus[function_name].status = chromeTesting.STATUS_PENDING;
  return true;
};

Networking.prototype._setResult = function(function_name, result_value) {
  var error = chrome.runtime.lastError;
  if (error) {
    this.callStatus[function_name].status = chromeTesting.STATUS_FAILURE;
    this.callStatus[function_name].result = null;
    this.callStatus[function_name].error = error.message;
  } else {
    this.callStatus[function_name].status = chromeTesting.STATUS_SUCCESS;
    this.callStatus[function_name].result = result_value;
    this.callStatus[function_name].error = null;
  }
};

Networking.prototype.getEnabledNetworkDevices = function() {
  if (!this._setupFunctionCall("getEnabledNetworkDevices"))
    return;
  var self = this;
  chrome.networkingPrivate.getEnabledNetworkTypes(function(networkTypes) {
    self._setResult("getEnabledNetworkDevices", networkTypes);
  });
};

Networking.prototype.enableNetworkDevice = function(type) {
  if (!this._setupFunctionCall("enableNetworkDevice"))
    return;
  var self = this;
  chrome.networkingPrivate.enableNetworkType(type);
};

Networking.prototype.disableNetworkDevice = function(type) {
  if (!this._setupFunctionCall("disableNetworkDevice"))
    return;
  var self = this;
  chrome.networkingPrivate.disableNetworkType(type);
};

Networking.prototype.requestNetworkScan = function() {
  if (!this._setupFunctionCall("requestNetworkScan"))
    return;
  var self = this;
  chrome.networkingPrivate.requestNetworkScan();
};

Networking.prototype.createNetwork = function(shared, properties) {
  if (!this._setupFunctionCall("createNetwork"))
    return;
  var self = this;
  chrome.networkingPrivate.createNetwork(shared, properties, function(guid) {
    self._setResult("createNetwork", guid);
  });
};

Networking.prototype.setProperties = function(guid, properties) {
  if (!this._setupFunctionCall("setProperties"))
    return;
  var self = this;
  chrome.networkingPrivate.setProperties(guid, properties, function() {
    self._setResult("setProperties", null);
  });
};

Networking.prototype.findNetworks = function(type) {
  if (!this._setupFunctionCall("findNetworks"))
    return;
  var self = this;
  chrome.networkingPrivate.getVisibleNetworks(type, function(networks) {
    self._setResult("findNetworks", networks);
  });
};

Networking.prototype.getNetworks = function(properties) {
  if (!this._setupFunctionCall("getNetworks"))
    return;
  var self = this;
  chrome.networkingPrivate.getNetworks(properties, function(networkList) {
    self._setResult("getNetworks", networkList);
  });
};

Networking.prototype.getNetworkInfo = function(networkId) {
  if (!this._setupFunctionCall("getNetworkInfo"))
    return;
  var self = this;
  chrome.networkingPrivate.getProperties(networkId, function(networkInfo) {
    self._setResult("getNetworkInfo", networkInfo);
  });
};

Networking.prototype.connectToNetwork = function(networkId) {
  if (!this._setupFunctionCall("connectToNetwork"))
    return;
  var self = this;
  chrome.networkingPrivate.startConnect(networkId, function() {
    self._setResult("connectToNetwork", null);
  });
};

Networking.prototype.disconnectFromNetwork = function(networkId) {
  if (!this._setupFunctionCall("disconnectFromNetwork"))
    return;
  var self = this;
  chrome.networkingPrivate.startDisconnect(networkId, function() {
    self._setResult("disconnectFromNetwork", null);
  });
};

Networking.prototype.setWifiTDLSEnabledState = function(ip_or_mac, enable) {
  if (!this._setupFunctionCall("setWifiTDLSEnabledState"))
    return;
  var self = this;
  chrome.networkingPrivate.setWifiTDLSEnabledState(
      ip_or_mac, enable, function(result) {
    self._setResult("setWifiTDLSEnabledState", result);
  });
};

Networking.prototype.getWifiTDLSStatus = function(ip_or_mac) {
  if (!this._setupFunctionCall("getWifiTDLSStatus"))
    return;
  var self = this;
  chrome.networkingPrivate.getWifiTDLSStatus(ip_or_mac,
      function(TDLSStatus) {
    self._setResult("getWifiTDLSStatus", TDLSStatus);
  });
};

Networking.prototype.getCaptivePortalStatus = function(networkPath) {
  if (!this._setupFunctionCall("getCaptivePortalStatus"))
    return;
  var self = this;
  chrome.networkingPrivate.getCaptivePortalStatus(networkPath,
      function(CaptivePortalStatus) {
    self._setResult("getCaptivePortalStatus", CaptivePortalStatus);
  });
};


Networking.prototype.onNetworkListChanged = function() {
  if (!this._setupFunctionCall("onNetworkListChanged"))
    return;
  var self = this;
  chrome.networkingPrivate.onNetworkListChanged.addListener(
      function(changes) {
    self._setResult("onNetworkListChanged", changes);
  });
};

Networking.prototype.onPortalDetectionCompleted = function() {
  if (!this._setupFunctionCall("onPortalDetectionCompleted"))
    return;
  var self = this;
  chrome.networkingPrivate.onPortalDetectionCompleted.addListener(
      function(networkPath, state) {
    self._setResult("onPortalDetectionCompleted", networkPath);
  });
};

var chromeTesting = {
  STATUS_PENDING: "chrome-test-call-status-pending",
  STATUS_SUCCESS: "chrome-test-call-status-success",
  STATUS_FAILURE: "chrome-test-call-status-failure",
  networking: new Networking()
};
