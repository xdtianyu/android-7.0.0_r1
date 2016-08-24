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

import android.annotation.SystemApi;
import android.car.CarManagerBase;
import android.os.IBinder;
import android.os.RemoteException;

import com.android.car.vehiclenetwork.IVehicleNetworkHalMock;
import com.android.car.vehiclenetwork.VehicleNetwork.VehicleNetworkHalMock;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfigs;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropValue;
import com.android.car.vehiclenetwork.VehiclePropConfigsParcelable;
import com.android.car.vehiclenetwork.VehiclePropValueParcelable;
import com.android.internal.annotations.GuardedBy;

import java.lang.ref.WeakReference;

/**
 * API for testing only. Allows mocking vehicle hal.
 * @hide
 */
@SystemApi
public class CarTestManager implements CarManagerBase {

    /**
     * Flag for {@link #startMocking(VehicleNetworkHalMock, int)}.
     * This is for passing no flag.
     */
    public static final int FLAG_MOCKING_NONE = 0x0;

    /**
     * Flag for {@link #startMocking(VehicleNetworkHalMock, int)}.
     * When this flag is set, shutdown request from mocked vehicle HAL will shutdown the system
     * instead of avoiding shutdown, which is the default behavior.
     * This can be used to test shutdown flow manually using mocking.
     */
    public static final int FLAG_MOCKING_REAL_SHUTDOWN = 0x1;

    private final ICarTest mService;

    @GuardedBy("this")
    private VehicleNetworkHalMock mHalMock;
    private IVehicleNetworkHalMockImpl mHalMockImpl;

    public CarTestManager(CarTestManagerBinderWrapper wrapper) {
        mService = ICarTest.Stub.asInterface(wrapper.binder);
    }

    @Override
    public void onCarDisconnected() {
        // should not happen for embedded
    }

    /**
     * Inject vehicle prop value event.
     * @param value
     */
    public void injectEvent(VehiclePropValue value) {
        try {
            mService.injectEvent(new VehiclePropValueParcelable(value));
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    /**
     * Check if given property is supported by vehicle hal.
     * @param property
     * @return
     */
    public boolean isPropertySupported(int property) {
        try {
            return mService.isPropertySupported(property);
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
        return false;
    }
    /**
     * Start mocking vehicle HAL. It is somewhat strange to re-use interface in lower level
     * API, but this is only for testing, and interface is exactly the same.
     * @param mock
     * @flags Combination of FLAG_MOCKING_*
     */
    public void startMocking(VehicleNetworkHalMock mock, int flags) {
        IVehicleNetworkHalMockImpl halMockImpl = new IVehicleNetworkHalMockImpl(this);
        synchronized (this) {
            mHalMock = mock;
            mHalMockImpl = halMockImpl;
        }
        try {
            mService.startMocking(halMockImpl, flags);
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    /**
     * Stop previously started mocking.
     */
    public void stopMocking() {
        IVehicleNetworkHalMockImpl halMockImpl;
        synchronized (this) {
            halMockImpl = mHalMockImpl;
            mHalMock = null;
            mHalMockImpl = null;
        }
        try {
            mService.stopMocking(halMockImpl);
        } catch (RemoteException e) {
            handleRemoteException(e);
        }
    }

    private synchronized VehicleNetworkHalMock getHalMock() {
        return mHalMock;
    }

    private void handleRemoteException(RemoteException e) {
        //TODO
    }

    private static class IVehicleNetworkHalMockImpl extends IVehicleNetworkHalMock.Stub {
        private final WeakReference<CarTestManager> mTestManager;

        private IVehicleNetworkHalMockImpl(CarTestManager testManager) {
            mTestManager = new WeakReference<CarTestManager>(testManager);
        }

        @Override
        public VehiclePropConfigsParcelable onListProperties() {
            CarTestManager testManager = mTestManager.get();
            if (testManager == null) {
                return null;
            }
            VehiclePropConfigs configs = testManager.getHalMock().onListProperties();
            return new VehiclePropConfigsParcelable(configs);
        }

        @Override
        public void onPropertySet(VehiclePropValueParcelable value) {
            CarTestManager testManager = mTestManager.get();
            if (testManager == null) {
                return;
            }
            testManager.getHalMock().onPropertySet(value.value);
        }

        @Override
        public VehiclePropValueParcelable onPropertyGet(VehiclePropValueParcelable value) {
            CarTestManager testManager = mTestManager.get();
            if (testManager == null) {
                return null;
            }
            VehiclePropValue retValue = testManager.getHalMock().onPropertyGet(value.value);
            return new VehiclePropValueParcelable(retValue);
        }

        @Override
        public void onPropertySubscribe(int property, float sampleRate, int zones) {
            CarTestManager testManager = mTestManager.get();
            if (testManager == null) {
                return;
            }
            testManager.getHalMock().onPropertySubscribe(property, sampleRate, zones);
        }

        @Override
        public void onPropertyUnsubscribe(int property) {
            CarTestManager testManager = mTestManager.get();
            if (testManager == null) {
                return;
            }
            testManager.getHalMock().onPropertyUnsubscribe(property);
        }
    }
}
