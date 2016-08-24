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

package com.android.car;

import android.car.hardware.CarSensorEvent;

//TODO add memory pool and recycling
public class CarSensorEventFactory {

    public static CarSensorEvent createBooleanEvent(int sensorType, long timeStampNs,
            boolean value) {
        CarSensorEvent event = new CarSensorEvent(sensorType, timeStampNs, 0, 1);
        event.intValues[0] = value ? 1 : 0;
        return event;
    }

    public static CarSensorEvent createIntEvent(int sensorType, long timeStampNs, int value) {
        CarSensorEvent event = new CarSensorEvent(sensorType, timeStampNs, 0, 1);
        event.intValues[0] = value;
        return event;
    }

    public static CarSensorEvent createFloatEvent(int sensorType, long timeStampNs, float value) {
        CarSensorEvent event = new CarSensorEvent(sensorType, timeStampNs, 1, 0);
        event.floatValues[0] = value;
        return event;
    }

    public static void returnToPool(CarSensorEvent event) {
        //TODO
    }
}
