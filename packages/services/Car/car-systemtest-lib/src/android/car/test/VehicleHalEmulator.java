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
package android.car.test;

import android.car.Car;
import android.car.CarNotConnectedException;
import android.util.Log;

import com.android.car.vehiclenetwork.VehicleNetwork.VehicleNetworkHalMock;
import com.android.car.vehiclenetwork.VehicleNetworkConsts;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePermissionModel;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropAccess;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehiclePropChangeMode;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfig;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfigs;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;
import com.android.car.vehiclenetwork.VehiclePropValueUtil;

import java.lang.reflect.Field;
import java.util.HashMap;

/**
 * This is for mocking vehicle HAL and testing system's internal behavior.
 * By default, emulated vehicle HAL will have all properties defined with default values
 * returned for get call. For interested properties, each test can replace default behavior with
 * {@link #addProperty(VehiclePropConfig, VehicleHalPropertyHandler)} or
 * {@link #addStaticProperty(VehiclePropConfig, VehiclePropValue)}.
 * To test a case where specific property should not be present, test can call
 * {@link #removeProperty(int)}.
 *
 * Adding / removing properties should be done before calling {@link #start()} as the call will
 * start emulating with properties added / removed up to now.
 * @hide
 */
public class VehicleHalEmulator {
    private static final String TAG = VehicleHalEmulator.class.getSimpleName();
    /**
     * Interface for handler of each property.
     */
    public interface VehicleHalPropertyHandler {
        void onPropertySet(VehiclePropValue value);
        VehiclePropValue onPropertyGet(VehiclePropValue value);
        void onPropertySubscribe(int property, float sampleRate, int zones);
        void onPropertyUnsubscribe(int property);
    }

    private final HashMap<Integer, VehicleHalProperty> mProperties =
            new HashMap<>();

    private final CarTestManager mCarTestManager;
    private final HalMock mMock = new HalMock();
    private boolean mDefaultPropertiesPopulated = false;
    private boolean mStarted = false;

    /**
     * Constructor. Car instance passed should be already connected to car service.
     * @param car
     */
    public VehicleHalEmulator(Car car) {
        try {
            mCarTestManager = new CarTestManager(
                    (CarTestManagerBinderWrapper) car.getCarManager(Car.TEST_SERVICE));
        } catch (CarNotConnectedException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Add property to mocked vehicle hal.
     * @param config
     * @param handler
     */
    public synchronized void addProperty(VehiclePropConfig config,
            VehicleHalPropertyHandler handler) {
        populateDefaultPropertiesIfNecessary();
        VehicleHalProperty halProp = new VehicleHalProperty(config, handler);
        mProperties.put(config.getProp(), halProp);
    }

    /**
     * Add static property to mocked vehicle hal.
     * @param config
     * @param value
     */
    public synchronized void addStaticProperty(VehiclePropConfig config, VehiclePropValue value) {
        populateDefaultPropertiesIfNecessary();
        DefaultPropertyHandler handler = new DefaultPropertyHandler(config, value);
        VehicleHalProperty halProp = new VehicleHalProperty(config, handler);
        mProperties.put(config.getProp(), halProp);
    }

    /**
     * Remove this property from vehicle HAL properties. Emulated vehicle HAL will not have this
     * property. This is useful to test the case where specific property is not present.
     * @param property
     */
    public synchronized void removeProperty(int property) {
        populateDefaultPropertiesIfNecessary();
        mProperties.remove(property);
    }

    /**
     * Start emulation. All necessary properties should have been added / removed before this.
     */
    public void start() {
        mCarTestManager.startMocking(mMock, CarTestManager.FLAG_MOCKING_NONE);
        synchronized (this) {
            mStarted = true;
        }
    }

    /** Whether emulation is started or not. */
    public synchronized boolean isStarted() {
        return mStarted;
    }

    /**
     * Stop emulation. should be done before finishing test.
     */
    public void stop() {
        mCarTestManager.stopMocking();
        synchronized (this) {
            mStarted = false;
        }
    }

    /**
     * Inject given value to VNS which ultimately delivered as HAL event to clients.
     * This can be used to emulate H/W side change.
     * @param value
     */
    public void injectEvent(VehiclePropValue value) {
        mCarTestManager.injectEvent(value);
    }

    public static void assertPropertyForGet(VehiclePropConfig config, int property) {
        assertProperty(config, property);
        if ((config.getAccess() & VehiclePropAccess.VEHICLE_PROP_ACCESS_READ) == 0) {
            throw new IllegalArgumentException("cannot set write-only property 0x" +
                    Integer.toHexString(config.getProp()));
        }
    }

    public static void assertPropertyForSet(VehiclePropConfig config, VehiclePropValue value) {
        assertProperty(config, value.getProp());
        if ((config.getAccess() & VehiclePropAccess.VEHICLE_PROP_ACCESS_WRITE) == 0) {
            throw new IllegalArgumentException("cannot set read-only property 0x" +
                    Integer.toHexString(config.getProp()));
        }
    }

    public static void assertPropertyForSubscribe(VehiclePropConfig config, int property,
            float sampleRate, int zones) {
        assertPropertyForGet(config, property);
        if (config.getChangeMode() == VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_STATIC) {
            throw new IllegalStateException("cannot subscribe static property 0x" +
                    Integer.toHexString(config.getProp()));
        }
    }

    public static void assertProperty(VehiclePropConfig config, int property) {
        if (config.getProp() != property) {
            throw new IllegalStateException("Wrong prop, expecting 0x" +
                    Integer.toHexString(config.getProp()) + " while got 0x" +
                    Integer.toHexString(property));
        }
    }

    private synchronized void populateDefaultPropertiesIfNecessary() {
        if (mDefaultPropertiesPopulated) {
            return;
        }
        for (Field f : VehicleNetworkConsts.class.getDeclaredFields()) {
            if (f.getType() == int.class) {
                int property = 0;
                try {
                    property = f.getInt(null);
                } catch (IllegalAccessException e) {
                    continue;
                }
                int valueType = VehicleNetworkConsts.getVehicleValueType(property);
                if (valueType == VehicleValueType.VEHICLE_VALUE_TYPE_SHOUD_NOT_USE) {
                    // invalid property or not a property
                    continue;
                }
                int changeMode = VehiclePropChangeMode.VEHICLE_PROP_CHANGE_MODE_STATIC;
                int[] changeModes = VehicleNetworkConsts.getVehicleChangeMode(property);
                if (changeModes != null) {
                    changeMode = changeModes[0];
                }
                int[] accesses = VehicleNetworkConsts.getVehicleAccess(property);
                if (accesses == null) { // invalid
                    continue;
                }
                VehiclePropConfig config = VehiclePropConfig.newBuilder().
                        setProp(property).
                        setAccess(accesses[0]).
                        setChangeMode(changeMode).
                        setValueType(valueType).
                        setPermissionModel(
                                VehiclePermissionModel.VEHICLE_PERMISSION_NO_RESTRICTION).
                        addConfigArray(0).
                        setSampleRateMax(0).
                        setSampleRateMin(0).
                        build();
                VehiclePropValue initialValue = VehiclePropValueUtil.createDummyValue(property,
                        valueType);
                DefaultPropertyHandler handler = new DefaultPropertyHandler(config, initialValue);
                VehicleHalProperty halProp = new VehicleHalProperty(config, handler);
                mProperties.put(property, halProp);
            }
        }
        mDefaultPropertiesPopulated = true;
    }

    private synchronized VehiclePropConfigs handleListProperties() {
        VehiclePropConfigs.Builder builder = VehiclePropConfigs.newBuilder();
        for (VehicleHalProperty halProp : mProperties.values()) {
            builder.addConfigs(halProp.config);
        }
        return builder.build();
    }

    private synchronized void handlePropertySet(VehiclePropValue value) {
        getHalPropertyLocked(value.getProp()).handler.onPropertySet(value);
    }

    private synchronized VehiclePropValue handlePropertyGet(VehiclePropValue value) {
        return getHalPropertyLocked(value.getProp()).handler.onPropertyGet(value);
    }

    private synchronized void handlePropertySubscribe(int property, float sampleRate, int zones) {
        getHalPropertyLocked(property).handler.onPropertySubscribe(property, sampleRate, zones);
    }

    private synchronized void handlePropertyUnsubscribe(int property) {
        getHalPropertyLocked(property).handler.onPropertyUnsubscribe(property);
    }

    private VehicleHalProperty getHalPropertyLocked(int property) {
        VehicleHalProperty halProp = mProperties.get(property);
        if (halProp == null) {
            IllegalArgumentException e = new IllegalArgumentException();
            Log.i(TAG, "property not supported:" + Integer.toHexString(property), e);
            throw e;
        }
        return halProp;
    }

    private static class VehicleHalProperty {
        public final VehiclePropConfig config;
        public final VehicleHalPropertyHandler handler;

        public VehicleHalProperty(VehiclePropConfig config, VehicleHalPropertyHandler handler) {
            this.config = config;
            this.handler = handler;
        }
    }

    private static class DefaultPropertyHandler implements VehicleHalPropertyHandler {
        private final VehiclePropConfig mConfig;
        private VehiclePropValue mValue;
        private boolean mSubscribed = false;

        public DefaultPropertyHandler(VehiclePropConfig config, VehiclePropValue initialValue) {
            mConfig = config;
            mValue = initialValue;
        }

        @Override
        public synchronized void onPropertySet(VehiclePropValue value) {
            assertPropertyForSet(mConfig, value);
            mValue = value;
        }

        @Override
        public synchronized VehiclePropValue onPropertyGet(VehiclePropValue value) {
            assertPropertyForGet(mConfig, value.getProp());
            return mValue;
        }

        @Override
        public synchronized void onPropertySubscribe(int property, float sampleRate, int zones) {
            assertPropertyForSubscribe(mConfig, property, sampleRate, zones);
            mSubscribed = true;
        }

        @Override
        public synchronized void onPropertyUnsubscribe(int property) {
            assertProperty(mConfig, property);
            if (!mSubscribed) {
                throw new IllegalArgumentException("unsubscibe for not subscribed property 0x" +
                        Integer.toHexString(property));
            }
            mSubscribed = false;
        }

    }

    private class HalMock implements VehicleNetworkHalMock {

        @Override
        public VehiclePropConfigs onListProperties() {
            return handleListProperties();
        }

        @Override
        public void onPropertySet(VehiclePropValue value) {
            handlePropertySet(value);
        }

        @Override
        public VehiclePropValue onPropertyGet(VehiclePropValue value) {
            return handlePropertyGet(value);
        }

        @Override
        public void onPropertySubscribe(int property, float sampleRate, int zones) {
            handlePropertySubscribe(property, sampleRate, zones);
        }

        @Override
        public void onPropertyUnsubscribe(int property) {
            handlePropertyUnsubscribe(property);
        }
    }
}
