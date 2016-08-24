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

package com.android.car.hal;

import android.car.hardware.CarSensorEvent;

import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;

import java.io.PrintWriter;
import java.util.LinkedList;
import java.util.List;

/**
 * Common base for all SensorHal implementation.
 * It is wholly based on subscription and there is no explicit API for polling, but each sensor
 * should report its initial state immediately after {@link #requestSensorStart(int, int)} call.
 * It is ok to report sensor data {@link SensorListener#onSensorData(CarSensorEvent)} inside
 * the {@link #requestSensorStart(int, int)} call.
 */
public abstract class SensorHalServiceBase  extends HalServiceBase {
    /**
     * Listener for monitoring sensor event. Only sensor service will implement this.
     */
    public interface SensorListener {
        /**
         * Sensor Hal is ready and is fully accessible.
         * This will be called after {@link SensorHalServiceBase#init()}.
         */
        void onSensorHalReady(SensorHalServiceBase hal);
        /**
         * Sensor events are available.
         * @param events
         */
        void onSensorEvents(List<CarSensorEvent> events);
    }

    private final LinkedList<CarSensorEvent> mDispatchQ = new LinkedList<CarSensorEvent>();

    public abstract void registerSensorListener(SensorListener listener);

    /**
     * Sensor HAL should be ready after init call.
     * @return
     */
    public abstract boolean isReady();

    /**
     * This should work after {@link #init()}.
     * @return
     */
    public abstract int[] getSupportedSensors();

    public abstract boolean requestSensorStart(int sensorType, int rate);

    public abstract void requestSensorStop(int sensorType);

    /**
     * Utility to help service to send one event as listener only takes list form.
     * @param listener
     * @param event
     */
    protected void dispatchCarSensorEvent(SensorListener listener, CarSensorEvent event) {
        synchronized (mDispatchQ) {
            mDispatchQ.add(event);
            listener.onSensorEvents(mDispatchQ);
            mDispatchQ.clear();
        }
    }

    @Override
    public void handleHalEvents(List<VehiclePropValue> values) {
        // default no-op impl. Necessary to not propagate this HAL specific event to logical
        // sensor provider.
        throw new RuntimeException("should not be called");
    }

    @Override
    public List<VehiclePropConfig> takeSupportedProperties(List<VehiclePropConfig> allProperties) {
        return null;
    }
}
