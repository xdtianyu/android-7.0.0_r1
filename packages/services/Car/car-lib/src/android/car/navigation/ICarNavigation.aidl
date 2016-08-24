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
package android.car.navigation;

import android.graphics.Bitmap;
import android.car.navigation.CarNavigationInstrumentCluster;
import android.car.navigation.ICarNavigationEventListener;

/**
 * Binder API for CarNavigationManager.
 * @hide
 */
interface ICarNavigation {
    void sendNavigationStatus(int status) = 0;
    void sendNavigationTurnEvent(
        int event, String road, int turnAngle, int turnNumber, in Bitmap image, int turnSide) = 1;
    void sendNavigationTurnDistanceEvent(int distanceMeters, int timeSeconds) = 2;
    boolean isInstrumentClusterSupported() = 3;
    CarNavigationInstrumentCluster getInstrumentClusterInfo() = 4;
    boolean registerEventListener(ICarNavigationEventListener listener) = 5;
    boolean unregisterEventListener(ICarNavigationEventListener listener) = 6;
}
