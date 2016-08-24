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

import android.car.test.CarTestManager;
import android.car.test.ICarTest;
import android.content.Context;
import android.util.Log;

import com.android.car.hal.VehicleHal;
import com.android.car.vehiclenetwork.IVehicleNetworkHalMock;
import com.android.car.vehiclenetwork.VehicleNetwork;
import com.android.car.vehiclenetwork.VehicleNetworkProto.VehiclePropConfigs;
import com.android.car.vehiclenetwork.VehiclePropValueParcelable;

import java.io.PrintWriter;

/**
 * Service to allow testing / mocking vehicle HAL.
 * This service uses Vehicle HAL APIs directly (one exception) as vehicle HAL mocking anyway
 * requires accessing that level directly.
 */
public class CarTestService extends ICarTest.Stub implements CarServiceBase {

    private final Context mContext;
    private final VehicleNetwork mVehicleNetwork;
    private final ICarImpl mICarImpl;
    private boolean mInMocking = false;
    private int mMockingFlags = 0;
    private Exception mException = null;

    public CarTestService(Context context, ICarImpl carImpl) {
        mContext = context;
        mICarImpl = carImpl;
        mVehicleNetwork = VehicleHal.getInstance().getVehicleNetwork();
    }

    @Override
    public void init() {
        // nothing to do.
        // This service should not reset anything for init / release to maintain mocking.
    }

    @Override
    public void release() {
        // nothing to do
        // This service should not reset anything for init / release to maintain mocking.
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("*CarTestService*");
        writer.println(" mInMocking" + mInMocking);
    }

    @Override
    public void injectEvent(VehiclePropValueParcelable value) {
        ICarImpl.assertVehicleHalMockPermission(mContext);
        mVehicleNetwork.injectEvent(value.value);
    }

    @Override
    public void startMocking(final IVehicleNetworkHalMock mock, final int flags) {
        ICarImpl.assertVehicleHalMockPermission(mContext);
        CarServiceUtils.runOnMainSync(new Runnable() {
            @Override
            public void run() {
                try {
                    mVehicleNetwork.startMocking(mock);
                    VehicleHal.getInstance().startMocking();
                    mICarImpl.startMocking();
                    synchronized (this) {
                        mInMocking = true;
                        mMockingFlags = flags;
                        mException = null;
                    }
                } catch (Exception e) {
                    Log.w(CarLog.TAG_TEST, "startMocking failed", e);
                    synchronized (this) {
                        mException = e;
                    }
                }
                Log.i(CarLog.TAG_TEST, "start vehicle HAL mocking, flags:0x" +
                        Integer.toHexString(flags));
            }
        });
        synchronized (this) {
            if (mException != null) {
                throw new IllegalStateException(mException);
            }
        }
    }

    @Override
    public void stopMocking(final IVehicleNetworkHalMock mock) {
        ICarImpl.assertVehicleHalMockPermission(mContext);
        CarServiceUtils.runOnMainSync(new Runnable() {
            @Override
            public void run() {
                try {
                    mVehicleNetwork.stopMocking(mock);
                    VehicleHal.getInstance().stopMocking();
                    mICarImpl.stopMocking();
                    synchronized (this) {
                        mInMocking = false;
                        mMockingFlags = 0;
                        mException = null;
                    }
                    Log.i(CarLog.TAG_TEST, "stop vehicle HAL mocking");
                } catch (Exception e) {
                    Log.w(CarLog.TAG_TEST, "stopMocking failed", e);
                }
            }
        });
    }

    @Override
    public boolean isPropertySupported(int property) {
        VehiclePropConfigs configs = VehicleHal.getInstance().getVehicleNetwork().listProperties(
                property);
        return configs != null;
    }

    public synchronized boolean isInMocking() {
        return mInMocking;
    }

    public synchronized boolean shouldDoRealShutdownInMocking() {
        return (mMockingFlags & CarTestManager.FLAG_MOCKING_REAL_SHUTDOWN) != 0;
    }
}
