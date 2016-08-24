/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.weave;

oneway interface IWeaveServiceManagerNotificationListener {
  const int CLOUD_ID = 1;
  const int DEVICE_ID = 2;
  const int DEVICE_NAME = 3;
  const int DEVICE_DESCRIPTION = 4;
  const int DEVICE_LOCATION = 5;
  const int OEM_NAME = 6;
  const int MODEL_NAME = 7;
  const int MODEL_ID = 8;
  const int PAIRING_SESSION_ID = 9;
  const int PAIRING_MODE = 10;
  const int PAIRING_CODE = 11;
  const int TRAITS = 12;
  const int COMPONENTS = 13;
  const int STATE = 14;

  void notifyServiceManagerChange(in int[] notificationIds);
}
