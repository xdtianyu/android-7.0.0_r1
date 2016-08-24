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

import android.car.Car;
import android.car.hardware.hvac.CarHvacEvent;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.hvac.ICarHvac;
import android.car.hardware.hvac.ICarHvacEventListener;
import android.content.Context;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import com.android.car.hal.HvacHalService;
import com.android.car.hal.VehicleHal;

import java.io.PrintWriter;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class CarHvacService extends ICarHvac.Stub
        implements CarServiceBase, HvacHalService.HvacHalListener {
    public static final boolean DBG = true;
    public static final String  TAG = CarLog.TAG_HVAC + ".CarHvacService";

    private HvacHalService mHvacHal;
    private final Map<IBinder, ICarHvacEventListener> mListenersMap = new HashMap<>();
    private final Map<IBinder, HvacDeathRecipient> mDeathRecipientMap = new HashMap<>();
    private final Context mContext;

    public CarHvacService(Context context) {
        mHvacHal = VehicleHal.getInstance().getHvacHal();
        mContext = context;
    }

    class HvacDeathRecipient implements IBinder.DeathRecipient {
        private static final String TAG = CarHvacService.TAG + ".HvacDeathRecipient";
        private IBinder mListenerBinder;

        HvacDeathRecipient(IBinder listenerBinder) {
            mListenerBinder = listenerBinder;
        }

        /**
         * Client died. Remove the listener from HAL service and unregister if this is the last
         * client.
         */
        @Override
        public void binderDied() {
            if (DBG) {
                Log.d(TAG, "binderDied " + mListenerBinder);
            }
            CarHvacService.this.unregisterListenerLocked(mListenerBinder);
        }

        void release() {
            mListenerBinder.unlinkToDeath(this, 0);
        }
    }

    @Override
    public synchronized void init() {
    }

    @Override
    public synchronized void release() {
        for (HvacDeathRecipient recipient : mDeathRecipientMap.values()) {
            recipient.release();
        }
        mDeathRecipientMap.clear();
        mListenersMap.clear();
    }

    @Override
    public void dump(PrintWriter writer) {
        // TODO
    }

    @Override
    public synchronized void registerListener(ICarHvacEventListener listener) {
        if (DBG) {
            Log.d(TAG, "registerListener");
        }
        ICarImpl.assertHvacPermission(mContext);
        if (listener == null) {
            Log.e(TAG, "registerListener: Listener is null.");
            throw new IllegalArgumentException("listener cannot be null.");
        }

        IBinder listenerBinder = listener.asBinder();
        if (mListenersMap.containsKey(listenerBinder)) {
            // Already registered, nothing to do.
            return;
        }

        HvacDeathRecipient deathRecipient = new HvacDeathRecipient(listenerBinder);
        try {
            listenerBinder.linkToDeath(deathRecipient, 0);
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to link death for recipient. " + e);
            throw new IllegalStateException(Car.CAR_NOT_CONNECTED_EXCEPTION_MSG);
        }
        mDeathRecipientMap.put(listenerBinder, deathRecipient);

        if (mListenersMap.isEmpty()) {
            mHvacHal.setListener(this);
        }

        mListenersMap.put(listenerBinder, listener);
    }

    @Override
    public synchronized void unregisterListener(ICarHvacEventListener listener) {
        if (DBG) {
            Log.d(TAG, "unregisterListener");
        }
        ICarImpl.assertHvacPermission(mContext);
        if (listener == null) {
            Log.e(TAG, "unregisterListener: Listener is null.");
            throw new IllegalArgumentException("Listener is null");
        }

        IBinder listenerBinder = listener.asBinder();
        if (!mListenersMap.containsKey(listenerBinder)) {
            Log.e(TAG, "unregisterListener: Listener was not previously registered.");
        }
        unregisterListenerLocked(listenerBinder);
    }

    // Removes the listenerBinder from the current state.
    // The function assumes that the binder will exist both in listeners and death recipients list.
    private void unregisterListenerLocked(IBinder listenerBinder) {
        Object status = mListenersMap.remove(listenerBinder);

        if (status != null) {
            mDeathRecipientMap.get(listenerBinder).release();
            mDeathRecipientMap.remove(listenerBinder);
        }

        if (mListenersMap.isEmpty()) {
            mHvacHal.setListener(null);
        }
    }

    @Override
    public synchronized List<CarPropertyConfig> getHvacProperties() {
        ICarImpl.assertHvacPermission(mContext);
        return mHvacHal.getHvacProperties();
    }

    @Override
    public synchronized CarPropertyValue getProperty(int prop, int zone) {
        ICarImpl.assertHvacPermission(mContext);
        return mHvacHal.getHvacProperty(prop, zone);
    }

    @Override
    public synchronized void setProperty(CarPropertyValue prop) {
        ICarImpl.assertHvacPermission(mContext);
        mHvacHal.setHvacProperty(prop);
    }

    // Implement HvacHalListener interface
    @Override
    public synchronized void onPropertyChange(CarHvacEvent event) {
        for (ICarHvacEventListener l : mListenersMap.values()) {
            try {
                l.onEvent(event);
            } catch (RemoteException ex) {
                // If we could not send a record, its likely the connection snapped. Let the binder
                // death handle the situation.
                Log.e(TAG, "onEvent calling failed: " + ex);
            }
        }
    }

    @Override
    public synchronized void onError(int zone, int property) {
        // TODO:
    }
}
