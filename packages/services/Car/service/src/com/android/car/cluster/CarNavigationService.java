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
package com.android.car.cluster;

import android.car.CarAppContextManager;
import android.car.cluster.renderer.NavigationRenderer;
import android.car.navigation.CarNavigationInstrumentCluster;
import android.car.navigation.CarNavigationManager;
import android.car.navigation.ICarNavigation;
import android.car.navigation.ICarNavigationEventListener;
import android.content.Context;
import android.graphics.Bitmap;
import android.os.Binder;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.util.Log;

import com.android.car.AppContextService;
import com.android.car.CarLog;
import com.android.car.CarServiceBase;
import com.android.car.cluster.InstrumentClusterService.RendererInitializationListener;
import com.android.car.cluster.renderer.ThreadSafeNavigationRenderer;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

/**
 * Service that will push navigation event to navigation renderer in instrument cluster.
 */
public class CarNavigationService extends ICarNavigation.Stub
        implements CarServiceBase, RendererInitializationListener {
    private static final String TAG = CarLog.TAG_NAV;

    private final List<CarNavigationEventListener> mListeners = new ArrayList<>();
    private final AppContextService mAppContextService;
    private final Context mContext;
    private final InstrumentClusterService mInstrumentClusterService;

    private volatile CarNavigationInstrumentCluster mInstrumentClusterInfo = null;
    private volatile NavigationRenderer mNavigationRenderer;

    public CarNavigationService(Context context, AppContextService appContextService,
            InstrumentClusterService instrumentClusterService) {
        mContext = context;
        mAppContextService = appContextService;
        mInstrumentClusterService = instrumentClusterService;
    }

    @Override
    public void init() {
        Log.d(TAG, "init");
        mInstrumentClusterService.registerListener(this);
    }

    @Override
    public void release() {
        synchronized(mListeners) {
            mListeners.clear();
        }
        mInstrumentClusterService.unregisterListener(this);
    }

    @Override
    public void sendNavigationStatus(int status) {
        Log.d(TAG, "sendNavigationStatus, status: " + status);
        if (!isRendererAvailable()) {
            return;
        }
        verifyNavigationContextOwner();

        if (status == CarNavigationManager.STATUS_ACTIVE) {
            mNavigationRenderer.onStartNavigation();
        } else if (status == CarNavigationManager.STATUS_INACTIVE
                || status == CarNavigationManager.STATUS_UNAVAILABLE) {
            mNavigationRenderer.onStopNavigation();
        } else {
            throw new IllegalArgumentException("Unknown navigation status: " + status);
        }
    }

    @Override
    public void sendNavigationTurnEvent(
            int event, String road, int turnAngle, int turnNumber, Bitmap image, int turnSide) {
        Log.d(TAG, "sendNavigationTurnEvent, event:" + event + ", turnAngle: " + turnAngle + ", "
                + "turnNumber: " + turnNumber + ", " + "turnSide: " + turnSide);
        if (!isRendererAvailable()) {
            return;
        }
        verifyNavigationContextOwner();

        mNavigationRenderer.onNextTurnChanged(event, road, turnAngle, turnNumber, image, turnSide);
    }

    @Override
    public void sendNavigationTurnDistanceEvent(int distanceMeters, int timeSeconds) {
        Log.d(TAG, "sendNavigationTurnDistanceEvent, distanceMeters:" + distanceMeters + ", "
                + "timeSeconds: " + timeSeconds);
        if (!isRendererAvailable()) {
            return;
        }
        verifyNavigationContextOwner();

        mNavigationRenderer.onNextTurnDistanceChanged(distanceMeters, timeSeconds);
    }

    @Override
    public boolean registerEventListener(ICarNavigationEventListener listener) {
        CarNavigationEventListener eventListener;
        synchronized(mListeners) {
            if (findClientLocked(listener) != null) {
                return true;
            }

            eventListener = new CarNavigationEventListener(listener);
            try {
                listener.asBinder().linkToDeath(eventListener, 0);
            } catch (RemoteException e) {
                Log.w(TAG, "Adding listener failed.", e);
                return false;
            }
            mListeners.add(eventListener);
        }

        // The new listener needs to be told the instrument cluster parameters.
        if (isRendererAvailable()) {
            return eventListener.onInstrumentClusterStart(mInstrumentClusterInfo);
        }
        return true;
    }

    @Override
    public boolean unregisterEventListener(ICarNavigationEventListener listener) {
        CarNavigationEventListener client;
        synchronized (mListeners) {
            client = findClientLocked(listener);
        }
        return client != null && removeClient(client);
    }

    @Override
    public CarNavigationInstrumentCluster getInstrumentClusterInfo() {
        return mInstrumentClusterInfo;
    }

    @Override
    public boolean isInstrumentClusterSupported() {
        return mInstrumentClusterInfo != null;
    }

    private void verifyNavigationContextOwner() {
        if (!mAppContextService.isContextOwner(
                Binder.getCallingUid(),
                Binder.getCallingPid(),
                CarAppContextManager.APP_CONTEXT_NAVIGATION)) {
            throw new IllegalStateException(
                    "Client is not an owner of APP_CONTEXT_NAVIGATION.");
        }
    }

    private boolean removeClient(CarNavigationEventListener listener) {
        synchronized(mListeners) {
            for (CarNavigationEventListener currentListener : mListeners) {
                // Use asBinder() for comparison.
                if (currentListener == listener) {
                    currentListener.listener.asBinder().unlinkToDeath(currentListener, 0);
                    return mListeners.remove(currentListener);
                }
            }
        }
        return false;
    }

    private CarNavigationEventListener findClientLocked(
            ICarNavigationEventListener listener) {
        for (CarNavigationEventListener existingListener : mListeners) {
            if (existingListener.listener.asBinder() == listener.asBinder()) {
                return existingListener;
            }
        }
        return null;
    }

    @Override
    public void onRendererInitSucceeded() {
        Log.d(TAG, "onRendererInitSucceeded");
        mNavigationRenderer = ThreadSafeNavigationRenderer.createFor(
                Looper.getMainLooper(),
                mInstrumentClusterService.getNavigationRenderer());

        // TODO: we need to obtain this information from InstrumentClusterRenderer.
        mInstrumentClusterInfo = CarNavigationInstrumentCluster.createCluster(1000);

        if (isRendererAvailable()) {
            for (CarNavigationEventListener listener : mListeners) {
                listener.onInstrumentClusterStart(mInstrumentClusterInfo);
            }
        }
    }

    private class CarNavigationEventListener implements IBinder.DeathRecipient {
        final ICarNavigationEventListener listener;

        public CarNavigationEventListener(ICarNavigationEventListener listener) {
            this.listener = listener;
        }

        @Override
        public void binderDied() {
            listener.asBinder().unlinkToDeath(this, 0);
            removeClient(this);
        }

        /** Returns true if event sent successfully */
        public boolean onInstrumentClusterStart(CarNavigationInstrumentCluster clusterInfo) {
            try {
                listener.onInstrumentClusterStart(clusterInfo);
            } catch (RemoteException e) {
                Log.e(TAG, "Unable to call onInstrumentClusterStart for listener: " + listener, e);
                return false;
            }
            return true;
        }
    }

    @Override
    public void dump(PrintWriter writer) {
        // TODO Auto-generated method stub
    }

    private boolean isRendererAvailable() {
        boolean available = mNavigationRenderer != null;
        if (!available) {
            Log.w(TAG, "Instrument cluster renderer is not available.");
        }
        return available;
    }
}
