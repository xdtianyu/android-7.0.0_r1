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
import android.car.hardware.CarSensorManager;
import android.os.ServiceSpecificException;
import android.util.Log;
import android.util.SparseArray;

import com.android.car.CarLog;
import com.android.car.CarSensorEventFactory;
import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropAccess;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropChangeMode;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;

import java.io.PrintWriter;
import java.util.LinkedList;
import java.util.List;

/**
 * Sensor HAL implementation for physical sensors in car.
 */
public class SensorHalService extends SensorHalServiceBase {

    private static final boolean DBG_EVENTS = false;

    private static final int SENSOR_TYPE_INVALD = -1;

    private final VehicleHal mHal;
    private boolean mIsReady = false;
    private SensorHalServiceBase.SensorListener mSensorListener;
    private final SparseArray<VehiclePropConfig> mSensorToHalProperty =
            new SparseArray<VehiclePropConfig>();

    public SensorHalService(VehicleHal hal) {
        mHal = hal;
    }

    @Override
    public synchronized void init() {
        //TODO
        mIsReady = true;
    }

    @Override
    public synchronized List<VehiclePropConfig> takeSupportedProperties(
            List<VehiclePropConfig> allProperties) {
        LinkedList<VehiclePropConfig> supportedProperties = new LinkedList<VehiclePropConfig>();
        for (VehiclePropConfig halProperty : allProperties) {
            int sensor = getSensorTypeFromHalProperty(halProperty.getProp());
            if (sensor != SENSOR_TYPE_INVALD &&
                halProperty.getChangeMode() !=
                    VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_STATIC &&
                (halProperty.getAccess() == VehiclePropAccess.VEHICLE_PROP_ACCESS_READ
                    || halProperty.getAccess() ==
                    VehiclePropAccess.VEHICLE_PROP_ACCESS_WRITE)) {
                supportedProperties.add(halProperty);
                mSensorToHalProperty.append(sensor, halProperty);
            }
        }
        return supportedProperties;
    }

    @Override
    public synchronized void release() {
        mSensorToHalProperty.clear();
        mIsReady = false;
    }

    // should be used only insidehandleHalEvents.
    private final LinkedList<CarSensorEvent> mEventsToDispatch = new LinkedList<CarSensorEvent>();
    @Override
    public void handleHalEvents(List<VehiclePropValue> values) {
        for (VehiclePropValue v : values) {
            CarSensorEvent event = createCarSensorEvent(v);
            if (event != null) {
                mEventsToDispatch.add(event);
            }
        }
        SensorHalServiceBase.SensorListener sensorListener = null;
        synchronized (this) {
            sensorListener = mSensorListener;
        }
        if (sensorListener != null) {
            sensorListener.onSensorEvents(mEventsToDispatch);
        }
        mEventsToDispatch.clear();
    }

    private CarSensorEvent createCarSensorEvent(VehiclePropValue v) {
        int property = v.getProp();
        int sensorType = getSensorTypeFromHalProperty(property);
        if (sensorType == SENSOR_TYPE_INVALD) {
            throw new RuntimeException("handleBooleanHalEvent no sensor defined for property " +
                    property);
        }
        switch (property) {
            // boolean
            case VehicleNetworkConsts.VEHICLE_PROPERTY_NIGHT_MODE:
            case VehicleNetworkConsts.VEHICLE_PROPERTY_PARKING_BRAKE_ON:
            case VehicleNetworkConsts.VEHICLE_PROPERTY_FUEL_LEVEL_LOW: {
                if (DBG_EVENTS) {
                    Log.i(CarLog.TAG_SENSOR, "boolean event, property:" +
                            Integer.toHexString(property) + " value:" + v.getInt32Values(0));
                }
                return CarSensorEventFactory.createBooleanEvent(sensorType, v.getTimestamp(),
                        v.getInt32Values(0) == 1);
            }
            // int
            case VehicleNetworkConsts.VEHICLE_PROPERTY_GEAR_SELECTION:
            case VehicleNetworkConsts.VEHICLE_PROPERTY_DRIVING_STATUS: {
                if (DBG_EVENTS) {
                    Log.i(CarLog.TAG_SENSOR, "int event, property:" +
                            Integer.toHexString(property) + " value:" + v.getInt32Values(0));
                }
                return CarSensorEventFactory.createIntEvent(sensorType, v.getTimestamp(),
                        v.getInt32Values(0));
            }
            // float
            case VehicleNetworkConsts.VEHICLE_PROPERTY_PERF_VEHICLE_SPEED: {
                if (DBG_EVENTS) {
                    Log.i(CarLog.TAG_SENSOR, "float event, property:" +
                            Integer.toHexString(property) + " value:" + v.getFloatValues(0));
                }
                return CarSensorEventFactory.createFloatEvent(sensorType, v.getTimestamp(),
                        v.getFloatValues(0));
            }
        }
        return null;
    }

    @Override
    public synchronized void registerSensorListener(SensorHalServiceBase.SensorListener listener) {
        mSensorListener = listener;
        if (mIsReady) {
            listener.onSensorHalReady(this);
        }
    }

    @Override
    public synchronized boolean isReady() {
        return mIsReady;
    }

    @Override
    public synchronized int[] getSupportedSensors() {
        int[] supportedSensors = new int[mSensorToHalProperty.size()];
        for (int i = 0; i < supportedSensors.length; i++) {
            supportedSensors[i] = mSensorToHalProperty.keyAt(i);
        }
        return supportedSensors;
    }

    @Override
    public synchronized boolean requestSensorStart(int sensorType, int rate) {
        VehiclePropConfig config = mSensorToHalProperty.get(sensorType);
        if (config == null) {
            return false;
        }
        //TODO calculate sampling rate properly
        mHal.subscribeProperty(this, config.getProp(), fixSamplingRateForProperty(config, rate));
        return true;
    }

    public CarSensorEvent getCurrentSensorValue(int sensorType) {
        VehiclePropConfig config;
        synchronized (this) {
            config = mSensorToHalProperty.get(sensorType);
        }
        if (config == null) {
            return null;
        }
        try {
            VehiclePropValue value = mHal.getVehicleNetwork().getProperty(config.getProp());
            return createCarSensorEvent(value);
        } catch (ServiceSpecificException e) {
            Log.e(CarLog.TAG_SENSOR, "property not ready 0x" +
                    Integer.toHexString(config.getProp()), e);
            return null;
        }
    }

    private float fixSamplingRateForProperty(VehiclePropConfig prop, int carSensorManagerRate) {
        if (prop.getChangeMode() ==  VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_ON_CHANGE) {
            return 0;
        }
        float rate = 1.0f;
        switch (carSensorManagerRate) {
            case CarSensorManager.SENSOR_RATE_FASTEST:
            case CarSensorManager.SENSOR_RATE_FAST:
                rate = 10f;
                break;
            case CarSensorManager.SENSOR_RATE_UI:
                rate = 5f;
                break;
            default: // fall back to default.
                break;
        }
        if (rate > prop.getSampleRateMax()) {
            rate = prop.getSampleRateMax();
        }
        if (rate < prop.getSampleRateMin()) {
            rate = prop.getSampleRateMin();
        }
        return rate;
    }

    @Override
    public synchronized void requestSensorStop(int sensorType) {
        VehiclePropConfig config = mSensorToHalProperty.get(sensorType);
        if (config == null) {
            return;
        }
        mHal.unsubscribeProperty(this, config.getProp());
    }

    /**
     * Covert hal property to sensor type. This is also used to check if specific property
     * is supported by sensor hal or not.
     * @param halPropertyType
     * @return
     */
    static int getSensorTypeFromHalProperty(int halPropertyType) {
        switch (halPropertyType) {
            case VehicleNetworkConsts.VEHICLE_PROPERTY_PERF_VEHICLE_SPEED:
                return CarSensorManager.SENSOR_TYPE_CAR_SPEED;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_GEAR_SELECTION:
                return CarSensorManager.SENSOR_TYPE_GEAR;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_NIGHT_MODE:
                return CarSensorManager.SENSOR_TYPE_NIGHT;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_PARKING_BRAKE_ON:
                return CarSensorManager.SENSOR_TYPE_PARKING_BRAKE;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_DRIVING_STATUS:
                return CarSensorManager.SENSOR_TYPE_DRIVING_STATUS;
            case VehicleNetworkConsts.VEHICLE_PROPERTY_FUEL_LEVEL_LOW:
                return CarSensorManager.SENSOR_TYPE_FUEL_LEVEL;
            default:
                return SENSOR_TYPE_INVALD;
        }
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("*Sensor HAL*");
        writer.println("**Supported properties**");
        for (int i = 0; i < mSensorToHalProperty.size(); i++) {
            writer.println(mSensorToHalProperty.valueAt(i).toString());
        }
    }
}
