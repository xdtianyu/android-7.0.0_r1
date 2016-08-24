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

package android.car.hardware;

import android.car.hardware.CarSensorEvent;
import android.car.hardware.ICarSensorEventListener;

/** @hide */
interface ICarSensor {

    int[] getSupportedSensors() = 0;

    /**
     * register a listener or update registration if already updated.
     * @param sensorType sensor to listen with this listener.
     * @param rate sensor rate.
     * @return false if requested sensors cannot be subscribed / started.
     */
    boolean registerOrUpdateSensorListener(int sensorType, int rate,
            in ICarSensorEventListener listener) = 1;

    /**
     * get latest sensor event for the type. If there was no update after car connection, it will
     * return null immediately.
     */
    CarSensorEvent getLatestSensorEvent(int sensorType) = 2;

    /**
     * Stop listening for the given sensor type. All other sensors registered before will not
     * be affected.
     */
    void unregisterSensorListener(int sensorType, in ICarSensorEventListener listener) = 3;
}
