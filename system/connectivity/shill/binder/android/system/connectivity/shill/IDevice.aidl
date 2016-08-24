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

interface IDevice {
  /**
   * Get the device interface (e.g. "eth0"). This string
   * is guaranteed to uniquely identify this Device instance.
   *
   * @return The device interface
   */
  String GetInterface();

  /**
   * Binder reference to the currently selected service.
   * The selected service of a device is the service for
   * which it is currently receiving link events.  WiFi
   * is slightly different in that it sets the link event
   * immediately after requesting a connection so that
   * failures to connect are correctly attributed.
   *
   * The device guarantees that if it is connected, the
   * connected service will be returned by GetSelectedService().
   * However, GetSelectedService() could also return a null
   * pointer to an service Binder, indicating no selected service.
   * The SelectedService is also not guaranteed to be online
   * (e.g. it could be in the process of being connected, or an
   * error state).
   *
   * @return Binder reference to the selected service
   */
  IBinder GetSelectedService();

  /**
   * Register a callback interface whose OnPropertyChanged()
   * method will be called when the value of a shill property changes.
   *
   * @param callback Binder reference to call back
   */
  void RegisterPropertyChangedSignalHandler(IPropertyChangedCallback callback);
}
