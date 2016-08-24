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

import android.os.PersistableBundle;

import android.system.connectivity.shill.IPropertyChangedCallback;

interface IManager {
  /**
   * Technology types that can be passed to RequestScan().
   */
  const int TECHNOLOGY_ANY = 0;
  const int TECHNOLOGY_WIFI = 1;

  /**
   * (Brillo only) Ask WiFi driver to setup an AP mode interface.
   * The driver might teardown the station mode interface as a result
   * of this call. Shill will revert to station mode if the remote
   * service that called this method vanishes.
   *
   * @return Interface name on success, empty string on error
   */
  String SetupApModeInterface();

  /**
   * (Brillo only) Ask WiFi driver to setup a station mode interface.
   * The driver might teardown the AP mode interface as a result
   * of this call.
   *
   * @return Interface name on success, empty string on error
   */
  String SetupStationModeInterface();

  /**
   * Assign the ownership of a device |interface_name| to the
   * claimer |claimer_name|. The specified device will be
   * added to the blacklist. Any current connection on that device
   * will be terminated, and shill will stop managing that device.
   *
   * @param claimer_name Name of the claimer
   * @param interface_name Name of the interface to be claimed
   */
  void ClaimInterface(String claimer_name, String interface_name);

  /**
   * Take ownership of a device |interface_name| from
   * claimer |claimer_name| back to shill. The specified device
   * will be removed from the blacklist and managed by shill.
   *
   * @param claimer_name Name of the claimer
   * @param interface_name Name of the interface to be released
   */
  void ReleaseInterface(String claimer_name, String interface_name);

  /**
   * Update the configuration of a service in memory
   * and in the profile. If no matching service exists
   * in memory, it is temporarily created to carry out
   * this work, and may be removed later.
   *
   * If a GUID property is specified in properties,
   * it is used to find the service; otherwise Type,
   * Security, and type-specific properties such as
   * WiFi.SSID are used to find any existing service.
   * If no service is located in memory, a new one is
   * created with the supplied properties.
   *
   * All provided parameters are applied to the in
   * memory service regardless of errors.  But if an
   * error occurs while setting a property and the
   * service object was created as a result of this
   * call, it is removed.
   *
   * See the Service Properties section of doc/service-api.txt
   * for a list of properties and constraints on property values.
   *
   * @param properties Properties to configure the service with
   * @return Binder reference to the service updated or created
   */
  IBinder ConfigureService(in PersistableBundle properties);

  /**
   * Request a scan for the specified technology |type|. If
   * |type| is TECHNOLOGY_ANY, then a scan request is
   * made for each technology.
   *
   * See the TECHNOLOGY_* constants defined in this AIDL file
   * for valid technology types.
   *
   * @param type Technology type that the scan is requested for
   */
  void RequestScan(int type);

  /**
   * Get a list of device Binder references of all
   * devices managed by Manager.
   *
   * @return List of device Binder references
   */
  List<IBinder> GetDevices();

  /**
   * Register a callback interface whose OnPropertyChanged()
   * method will be called when the value of a shill property changes.
   *
   * @param callback Binder reference to call back
   */
  void RegisterPropertyChangedSignalHandler(IPropertyChangedCallback callback);
}
