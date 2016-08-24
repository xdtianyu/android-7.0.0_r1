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
package com.android.car.vehiclenetwork;

import static com.android.car.vehiclenetwork.VehiclePropValueUtil.getVectorLength;
import static com.android.car.vehiclenetwork.VehiclePropValueUtil.isCustomProperty;
import static com.android.car.vehiclenetwork.VehiclePropValueUtil.toFloatArray;
import static com.android.car.vehiclenetwork.VehiclePropValueUtil.toIntArray;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.ServiceSpecificException;
import android.util.Log;

import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleValueType;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfigs;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValues;
import com.android.internal.annotations.GuardedBy;

import java.lang.ref.WeakReference;

/**
 * System API to access Vehicle network. This is only for system services and applications should
 * not use this. All APIs will fail with security error if normal app tries this.
 */
public class VehicleNetwork {
    /**
     * Listener for VNS events.
     */
    public interface VehicleNetworkListener {
        /**
         * Notify HAL events. This requires subscribing the property
         */
        void onVehicleNetworkEvents(VehiclePropValues values);
        void onHalError(int errorCode, int property, int operation);
        void onHalRestart(boolean inMocking);
    }

    public interface VehicleNetworkHalMock {
        VehiclePropConfigs onListProperties();
        void onPropertySet(VehiclePropValue value);
        VehiclePropValue onPropertyGet(VehiclePropValue value);
        void onPropertySubscribe(int property, float sampleRate, int zones);
        void onPropertyUnsubscribe(int property);
    }

    private static final String TAG = VehicleNetwork.class.getSimpleName();

    private final IVehicleNetwork mService;
    private final VehicleNetworkListener mListener;
    private final IVehicleNetworkListenerImpl mVehicleNetworkListener;
    private final EventHandler mEventHandler;

    @GuardedBy("this")
    private VehicleNetworkHalMock mHalMock;
    private IVehicleNetworkHalMock mHalMockImpl;

    private static final int VNS_CONNECT_MAX_RETRY = 10;
    private static final long VNS_RETRY_WAIT_TIME_MS = 1000;
    private static final int NO_ZONE = -1;

    /**
     * Factory method to create VehicleNetwork
     *
     * @param listener listener for listening events
     * @param looper Looper to dispatch listener events
     */
    public static VehicleNetwork createVehicleNetwork(VehicleNetworkListener listener,
            Looper looper) {
        int retryCount = 0;
        IVehicleNetwork service = null;
        while (service == null) {
            service = IVehicleNetwork.Stub.asInterface(ServiceManager.getService(
                    IVehicleNetwork.class.getCanonicalName()));
            retryCount++;
            if (retryCount > VNS_CONNECT_MAX_RETRY) {
                break;
            }
            try {
                Thread.sleep(VNS_RETRY_WAIT_TIME_MS);
            } catch (InterruptedException e) {
                //ignore
            }
        }
        if (service == null) {
            throw new RuntimeException("Vehicle network service not available:" +
                    IVehicleNetwork.class.getCanonicalName());
        }
        return new VehicleNetwork(service, listener, looper);
    }

    private VehicleNetwork(IVehicleNetwork service, VehicleNetworkListener listener,
            Looper looper) {
        mService = service;
        mListener = listener;
        mEventHandler = new EventHandler(looper);
        mVehicleNetworkListener = new IVehicleNetworkListenerImpl(this);
    }

    /**
     * List all properties from vehicle HAL
     *
     * @return all properties
     */
    public VehiclePropConfigs listProperties() {
        return listProperties(0 /* all */);
    }

    /**
     * Return configuration information of single property
     *
     * @param property vehicle property number defined in {@link VehicleNetworkConsts}. 0 has has
     * special meaning of list all properties.
     * @return null if given property does not exist.
     */
    public VehiclePropConfigs listProperties(int property) {
        try {
            VehiclePropConfigsParcelable parcelable = mService.listProperties(property);
            if (parcelable != null) {
                return parcelable.configs;
            }
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
        return null;
    }

    /**
     * Set property which will lead into writing the value to vehicle HAL.
     *
     * @throws IllegalArgumentException If value set has wrong value like wrong valueType, wrong
     * data, and etc.
     */
    public void setProperty(VehiclePropValue value) throws IllegalArgumentException {
        VehiclePropValueParcelable parcelable = new VehiclePropValueParcelable(value);
        try {
            mService.setProperty(parcelable);
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    /**
     * Set integer type property
     *
     * @throws IllegalArgumentException For type mismatch (=the property is not int type)
     */
    public void setIntProperty(int property, int value) throws IllegalArgumentException {
        VehiclePropValue v = VehiclePropValueUtil.createIntValue(property, value, 0);
        setProperty(v);
    }

    /**
     * Set int vector type property. Length of passed values should match with vector length.
     */
    public void setIntVectorProperty(int property, int[] values) throws IllegalArgumentException {
        VehiclePropValue v = VehiclePropValueUtil.createIntVectorValue(property, values, 0);
        setProperty(v);
    }

    /**
     * Set long type property.
     */
    public void setLongProperty(int property, long value) throws IllegalArgumentException {
        VehiclePropValue v = VehiclePropValueUtil.createLongValue(property, value, 0);
        setProperty(v);
    }

    /**
     * Set float type property.
     */
    public void setFloatProperty(int property, float value) throws IllegalArgumentException {
        VehiclePropValue v = VehiclePropValueUtil.createFloatValue(property, value, 0);
        setProperty(v);
    }

    /**
     * Set float vector type property. Length of values should match with vector length.
     */
    public void setFloatVectorProperty(int property, float[] values)
            throws IllegalArgumentException {
        VehiclePropValue v = VehiclePropValueUtil.createFloatVectorValue(property, values, 0);
        setProperty(v);
    }

    /**
     * Set zoned boolean type property
     *
     * @throws IllegalArgumentException For type mismatch (=the property is not boolean type)
     */
    public void setBooleanProperty(int property, boolean value)
            throws IllegalArgumentException {
        VehiclePropValue v = VehiclePropValueUtil.createBooleanValue(property, value, 0);
        setProperty(v);
    }

    /**
     * Set zoned boolean type property
     *
     * @throws IllegalArgumentException For type mismatch (=the property is not boolean type)
     */
    public void setZonedBooleanProperty(int property, int zone, boolean value)
            throws IllegalArgumentException {
        VehiclePropValue v = VehiclePropValueUtil.createZonedBooleanValue(property, zone, value, 0);
        setProperty(v);
    }

    /**
     * Set zoned float type property
     *
     * @throws IllegalArgumentException For type mismatch (=the property is not float type)
     */
    public void setZonedFloatProperty(int property, int zone, float value)
            throws IllegalArgumentException {
        VehiclePropValue v = VehiclePropValueUtil.createZonedFloatValue(property, zone, value, 0);
        setProperty(v);
    }

    /**
     * Set zoned integer type property
     *
     * @throws IllegalArgumentException For type mismatch (=the property is not int type)
     */
    public void setZonedIntProperty(int property, int zone, int value)
            throws IllegalArgumentException {
        VehiclePropValue v = VehiclePropValueUtil.createZonedIntValue(property, zone, value, 0);
        setProperty(v);
    }

    /**
     * Set zoned int vector type property. Length of passed values should match with vector length.
     */
    public void setZonedIntVectorProperty(int property, int zone, int[] values)
            throws IllegalArgumentException {
        VehiclePropValue v = VehiclePropValueUtil
                .createZonedIntVectorValue(property, zone, values, 0);
        setProperty(v);
    }

    /**
     * Set zoned float vector type property. Length of passed values should match with vector
     * length.
     */
    public void setZonedFloatVectorProperty(int property, int zone, float[] values)
            throws IllegalArgumentException {
        VehiclePropValue v = VehiclePropValueUtil
                .createZonedFloatVectorValue(property, zone, values, 0);
        setProperty(v);
    }

    /**
     * Get property. This can be used for a property which does not require any other data.
     */
    public VehiclePropValue getProperty(int property) throws IllegalArgumentException,
            ServiceSpecificException {
        int valueType = VehicleNetworkConsts.getVehicleValueType(property);
        if (valueType == 0) {
            throw new IllegalArgumentException("Data type is unknown for property: " + property);
        }
        VehiclePropValue value = VehiclePropValueUtil.createBuilder(property, valueType, 0).build();
        return getProperty(value);
    }

    /**
     * Generic get method for any type of property. Some property may require setting data portion
     * as get may return different result depending on the data set.
     */
    public VehiclePropValue getProperty(VehiclePropValue value) throws IllegalArgumentException,
            ServiceSpecificException {
        VehiclePropValueParcelable parcelable = new VehiclePropValueParcelable(value);
        try {
            VehiclePropValueParcelable resParcelable = mService.getProperty(parcelable);
            if (resParcelable != null) {
                return resParcelable.value;
            }
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
        return null;
    }

    /**
     * Get int type property.
     */
    public int getIntProperty(int property) throws IllegalArgumentException,
            ServiceSpecificException {
        VehiclePropValue v = getProperty(
                property, NO_ZONE, VehicleValueType.VEHICLE_VALUE_TYPE_INT32);
        if (v.getInt32ValuesCount() != 1) {
            throw new IllegalStateException();
        }
        return v.getInt32Values(0);
    }

    /**
     * Get zoned int type property.
     */
    public int getZonedIntProperty(int property, int zone) throws IllegalArgumentException,
            ServiceSpecificException {
        VehiclePropValue v = getProperty(
                property, zone, VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32);
        if (v.getInt32ValuesCount() != 1) {
            throw new IllegalStateException();
        }
        return v.getInt32Values(0);
    }

    /**
     * Get int vector type property. Length of values should match vector length.
     */
    public int[] getIntVectorProperty(int property) throws IllegalArgumentException,
            ServiceSpecificException {
        VehiclePropValue v = getProperty(
                property, NO_ZONE, VehicleValueType.VEHICLE_VALUE_TYPE_INT32);
        assertVectorLength(v.getInt32ValuesCount(), property, v.getValueType());
        return toIntArray(v.getInt32ValuesList());
    }

    /**
     * Get zoned int vector type property. Length of values should match vector length.
     */
    public int[] getZonedIntVectorProperty(int property, int zone)
            throws IllegalArgumentException, ServiceSpecificException {
        VehiclePropValue v = getProperty(
                property, zone, VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_INT32);
        assertVectorLength(v.getInt32ValuesCount(), property, v.getValueType());
        return toIntArray(v.getInt32ValuesList());
    }

    /**
     * Get float type property.
     */
    public float getFloatProperty(int property) throws IllegalArgumentException,
            ServiceSpecificException {
        VehiclePropValue v = getProperty(
                property, NO_ZONE, VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT);
        if (v.getFloatValuesCount() != 1) {
            throw new IllegalStateException();
        }
        return v.getFloatValues(0);
    }

    /**
     * Get float vector type property. Length of values should match vector's length.
     */
    public float[] getFloatVectorProperty(int property) throws IllegalArgumentException,
            ServiceSpecificException {
        VehiclePropValue v = getProperty(
                property, NO_ZONE, VehicleValueType.VEHICLE_VALUE_TYPE_FLOAT);
        assertVectorLength(v.getFloatValuesCount(), property, v.getValueType());
        return toFloatArray(v.getFloatValuesList());
    }

    /**
     * Get zoned float vector type property. Length of values should match vector's length.
     */
    public float[] getZonedFloatVectorProperty(int property, int zone)
            throws IllegalArgumentException, ServiceSpecificException {
        VehiclePropValue v = getProperty(property, zone,
                VehicleValueType.VEHICLE_VALUE_TYPE_ZONED_FLOAT);
        assertVectorLength(v.getFloatValuesCount(), property, v.getValueType());
        return toFloatArray(v.getFloatValuesList());
    }

    /**
     * Get long type property.
     */
    public long getLongProperty(int property) throws IllegalArgumentException,
            ServiceSpecificException {
        VehiclePropValue v = getProperty(
                property, NO_ZONE, VehicleValueType.VEHICLE_VALUE_TYPE_INT64);
        return v.getInt64Value();
    }

    /**
     * Get string type property.
     */
    //TODO check UTF8 to java string conversion
    public String getStringProperty(int property) throws IllegalArgumentException,
            ServiceSpecificException {
        VehiclePropValue v = getProperty(
                property, NO_ZONE, VehicleValueType.VEHICLE_VALUE_TYPE_STRING);
        return v.getStringValue();
    }

    /**
     * Subscribe given property with given sample rate.
     */
    public void subscribe(int property, float sampleRate) throws IllegalArgumentException {
        subscribe(property, sampleRate, 0);
    }

    /**
     * Subscribe given property with given sample rate.
     */
    public void subscribe(int property, float sampleRate, int zones)
            throws IllegalArgumentException {
        try {
            mService.subscribe(mVehicleNetworkListener, property, sampleRate, zones);
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    /**
     * Stop subscribing the property.
     */
    public void unsubscribe(int property) {
        try {
            mService.unsubscribe(mVehicleNetworkListener, property);
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    /**
     * Inject given value to all clients subscribing the property. This is for testing.
     */
    public synchronized void injectEvent(VehiclePropValue value) {
        try {
            mService.injectEvent(new VehiclePropValueParcelable(value));
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    /**
     * Start mocking of vehicle HAL. For testing only.
     */
    public synchronized void startMocking(VehicleNetworkHalMock mock) {
        mHalMock = mock;
        mHalMockImpl = new IVehicleNetworkHalMockImpl(this);
        try {
            mService.startMocking(mHalMockImpl);
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    /**
     * Stop mocking of vehicle HAL. For testing only.
     */
    public synchronized void stopMocking() {
        if (mHalMockImpl == null) {
            return;
        }
        try {
            mService.stopMocking(mHalMockImpl);
        } catch (RemoteException e) {
            handleRemoteException(e);
        } finally {
            mHalMock = null;
            mHalMockImpl = null;
        }
    }

    /**
     * Start mocking of vehicle HAL. For testing only.
     */
    public synchronized void startMocking(IVehicleNetworkHalMock mock) {
        mHalMock = null;
        mHalMockImpl = mock;
        try {
            mService.startMocking(mHalMockImpl);
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    /**
     * Stop mocking of vehicle HAL. For testing only.
     */
    public synchronized void stopMocking(IVehicleNetworkHalMock mock) {
        if (mock.asBinder() != mHalMockImpl.asBinder()) {
            return;
        }
        try {
            mService.stopMocking(mHalMockImpl);
        } catch (RemoteException e) {
            handleRemoteException(e);
        } finally {
            mHalMock = null;
            mHalMockImpl = null;
        }
    }

    public synchronized void injectHalError(int errorCode, int property, int operation) {
        try {
            mService.injectHalError(errorCode, property, operation);
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    public synchronized void startErrorListening() {
        try {
            mService.startErrorListening(mVehicleNetworkListener);
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    public synchronized void stopErrorListening() {
        try {
            mService.stopErrorListening(mVehicleNetworkListener);
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    public synchronized void startHalRestartMonitoring() {
        try {
            mService.startHalRestartMonitoring(mVehicleNetworkListener);
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    public synchronized void stopHalRestartMonitoring() {
        try {
            mService.stopHalRestartMonitoring(mVehicleNetworkListener);
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    private synchronized VehicleNetworkHalMock getHalMock() {
        return mHalMock;
    }

    private void handleRemoteException(RemoteException e) {
        throw new RuntimeException("Vehicle network service not working ", e);
    }

    private void handleVehicleNetworkEvents(VehiclePropValues values) {
        mListener.onVehicleNetworkEvents(values);
    }

    private void handleHalError(int errorCode, int property, int operation) {
        mListener.onHalError(errorCode, property, operation);
    }

    private void handleHalRestart(boolean inMocking) {
        mListener.onHalRestart(inMocking);
    }

    private class EventHandler extends Handler {

        private static final int MSG_EVENTS = 0;
        private static final int MSG_HAL_ERROR = 1;
        private static final int MSG_HAL_RESTART = 2;

        private EventHandler(Looper looper) {
            super(looper);
        }

        private void notifyEvents(VehiclePropValues values) {
            Message msg = obtainMessage(MSG_EVENTS, values);
            sendMessage(msg);
        }

        private void notifyHalError(int errorCode, int property, int operation) {
            Message msg = obtainMessage(MSG_HAL_ERROR, errorCode, property, operation);
            sendMessage(msg);
        }

        private void notifyHalRestart(boolean inMocking) {
            Message msg = obtainMessage(MSG_HAL_RESTART, inMocking ? 1 : 0, 0);
            sendMessage(msg);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_EVENTS:
                    handleVehicleNetworkEvents((VehiclePropValues) msg.obj);
                    break;
                case MSG_HAL_ERROR:
                    handleHalError(msg.arg1, msg.arg2, (Integer) msg.obj);
                    break;
                case MSG_HAL_RESTART:
                    handleHalRestart(msg.arg1 == 1);
                    break;
                default:
                    Log.w(TAG, "Unknown message:" + msg.what, new RuntimeException());
                    break;
            }
        }
    }

    private static class IVehicleNetworkListenerImpl extends IVehicleNetworkListener.Stub {

        private final WeakReference<VehicleNetwork> mVehicleNetwork;

        private IVehicleNetworkListenerImpl(VehicleNetwork vehicleNewotk) {
            mVehicleNetwork = new WeakReference<>(vehicleNewotk);
        }

        @Override
        public void onVehicleNetworkEvents(VehiclePropValuesParcelable values) {
            VehicleNetwork vehicleNetwork = mVehicleNetwork.get();
            if (vehicleNetwork != null) {
                vehicleNetwork.mEventHandler.notifyEvents(values.values);
            }
        }

        @Override
        public void onHalError(int errorCode, int property, int operation) {
            VehicleNetwork vehicleNetwork = mVehicleNetwork.get();
            if (vehicleNetwork != null) {
                vehicleNetwork.mEventHandler.notifyHalError(errorCode, property, operation);
            }
        }

        @Override
        public void onHalRestart(boolean inMocking) {
            VehicleNetwork vehicleNetwork = mVehicleNetwork.get();
            if (vehicleNetwork != null) {
                vehicleNetwork.mEventHandler.notifyHalRestart(inMocking);
            }
        }
    }

    private static class IVehicleNetworkHalMockImpl extends IVehicleNetworkHalMock.Stub {
        private final WeakReference<VehicleNetwork> mVehicleNetwork;

        private IVehicleNetworkHalMockImpl(VehicleNetwork vehicleNewotk) {
            mVehicleNetwork = new WeakReference<>(vehicleNewotk);
        }

        @Override
        public VehiclePropConfigsParcelable onListProperties() {
            VehicleNetwork vehicleNetwork = mVehicleNetwork.get();
            if (vehicleNetwork == null) {
                return null;
            }
            VehiclePropConfigs configs = vehicleNetwork.getHalMock().onListProperties();
            return new VehiclePropConfigsParcelable(configs);
        }

        @Override
        public void onPropertySet(VehiclePropValueParcelable value) {
            VehicleNetwork vehicleNetwork = mVehicleNetwork.get();
            if (vehicleNetwork == null) {
                return;
            }
            vehicleNetwork.getHalMock().onPropertySet(value.value);
        }

        @Override
        public VehiclePropValueParcelable onPropertyGet(VehiclePropValueParcelable value) {
            VehicleNetwork vehicleNetwork = mVehicleNetwork.get();
            if (vehicleNetwork == null) {
                return null;
            }
            VehiclePropValue resValue = vehicleNetwork.getHalMock().onPropertyGet(value.value);
            return new VehiclePropValueParcelable(resValue);
        }

        @Override
        public void onPropertySubscribe(int property, float sampleRate, int zones) {
            VehicleNetwork vehicleNetwork = mVehicleNetwork.get();
            if (vehicleNetwork == null) {
                return;
            }
            vehicleNetwork.getHalMock().onPropertySubscribe(property, sampleRate, zones);
        }

        @Override
        public void onPropertyUnsubscribe(int property) {
            VehicleNetwork vehicleNetwork = mVehicleNetwork.get();
            if (vehicleNetwork == null) {
                return;
            }
            vehicleNetwork.getHalMock().onPropertyUnsubscribe(property);
        }
    }

    private VehiclePropValue getProperty(int property, int zone, int customPropetyDataType) {
        boolean isCustom = isCustomProperty(property);
        int valueType = isCustom
                ? customPropetyDataType
                : VehicleNetworkConsts.getVehicleValueType(property);

        VehiclePropValue.Builder valuePrototypeBuilder =
                VehiclePropValueUtil.createBuilder(property, valueType, 0);

        if (zone != NO_ZONE) {
            valuePrototypeBuilder.setZone(zone);
        }

        VehiclePropValue v = getProperty(valuePrototypeBuilder.build());
        if (v == null) {
            // if property is invalid, IllegalArgumentException should have been thrown
            // from getProperty.
            throw new IllegalStateException();
        }

        if (!isCustom && v.getValueType() != valueType) {
            throw new IllegalArgumentException(
                    "Unexpected type for property 0x" + Integer.toHexString(property) +
                    " got:0x" + Integer.toHexString(v.getValueType())
                    + ", expecting:0x" + Integer.toHexString(valueType));
        }
        return v;
    }

    private void assertVectorLength(int actual, int property, int valueType) {
        int expectedLen = getVectorLength(valueType);
        if (expectedLen != actual) {
            throw new IllegalStateException("Invalid array size for property: " + property
                    + ". Expected: " + expectedLen
                    + ", actual: " + actual);
        }
    }
}
