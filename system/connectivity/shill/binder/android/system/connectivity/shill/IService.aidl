/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.system.connectivity.shill;

import android.system.connectivity.shill.IPropertyChangedCallback;

interface IService {
  /**
   * State types that can returned by GetState().
   *
   * Note: keep in sync with Flimflam state options in
   * system_api/dbus/shill/dbus-constants.h.
   */
  // The service is not enabled or otherwise operational.
  const int STATE_IDLE = 0;
  // Intermediate states associated with connection-based devices such as WiFi
  // and Cellular. These are exposed for UI applications to provide more fine-
  // grained status.
  const int STATE_ASSOC = 1;
  // Layer 2 is setup but Layer 3 setup has yet to completed.
  const int STATE_CONFIG = 2;
  // Layer 3 setup is complete; ready to transit and receive data.
  const int STATE_READY = 3;
  // Layer 3 setup is complete but connectivity to the Internet may be limited
  // or unavailable.
  const int STATE_PORTAL = 4;
  // Layer 3 setup is complete and an Internet connection has been checked to
  // support HTTP access to the Manager's captive-portal-checking URL.
  const int STATE_ONLINE = 5;
  // An error occurred while trying to reach the "ready" state. Call GetError()
  // for details.
  const int STATE_FAILURE = 6;

  /**
   * Error types that can returned by GetError().
   *
   * Note: keep in sync with Service::ConnectFailureToString() and Flimflam
   * error options in system_api/dbus/shill/dbus-constants.h.
   */
  const int ERROR_IDLE = 0;
  const int ERROR_AAA_FAILED = 1;
  const int ERROR_ACTIVATION_FAILED = 2;
  const int ERROR_BAD_PASSPHRASE = 3;
  const int ERROR_BAD_WEP_KEY = 4;
  const int ERROR_CONNECT_FAILED = 5;
  const int ERROR_DNS_LOOKUP_FAILED = 6;
  const int ERROR_DHCP_FAILED = 7;
  const int ERROR_HTTP_GET_FAILED = 8;
  const int ERROR_INTERNAL = 9;
  const int ERROR_IPSEC_CERT_AUTH_FAILED = 10;
  const int ERROR_IPSEC_PSK_AUTH_FAILED = 11;
  const int ERROR_NEED_EVDO = 12;
  const int ERROR_NEED_HOME_NETWORK = 13;
  const int ERROR_OTASP_FAILED = 14;
  const int ERROR_OUT_OF_RANGE = 15;
  const int ERROR_PIN_MISSING = 16;
  const int ERROR_PPP_AUTH_FAILED = 17;

  /**
   * Initiate a connection for the specified service.
   *
   * For Ethernet devices, this method can only be used
   * if it has previously been disconnected. Otherwise,
   * the plugging of a cable automatically triggers
   * a connection.  If no cable is plugged in, this
   * method will fail.
   *
   * If the requested service is already connected,
   * this request is ignored and an error is logged.
   *
   * If the requested service is in the process of
   * connecting, this request is ignored and an error
   * is logged.
   *
   * If another service of the same type is connected or
   * connecting, the service is terminated before this
   * request is handled.
   *
   * If the requested service cannot, for reasons not
   * described above, be connected, an error is logged.
   */
  void Connect();

  /**
   * Get the state of the service.
   *
   * See the STATE_* constants defined in this AIDL file
   * for possible return types.
   *
   * @return The state of the service
   */
  int GetState();

  /**
   * Gets the signal strength of the service. This
   * is a normalized value between 0 and 100.
   *
   * This property will not be present for Ethernet
   * devices.
   *
   * @return The signal strength of the service
   */
  byte GetStrength();

  /**
   * Get the service error status details.
   *
   * When an error occurs during connection or disconnection,
   * detailed information is represented in the Error
   * property to help the user interface to present the
   * user with alternate options.
   *
   * This property is only valid when the service is in a
   * failure state. Otherwise it might be empty or not
   * present at all.
   *
   * See the ERROR_* constants defined in this AIDL file
   * for possible return types.
   *
   * @return The signal strength of the service
   */
  int GetError();

  /**
   * Register a callback interface whose OnPropertyChanged()
   * method will be called when the value of a shill property changes.
   *
   * @param callback Binder reference to call back
   */
  void RegisterPropertyChangedSignalHandler(IPropertyChangedCallback callback);
}
