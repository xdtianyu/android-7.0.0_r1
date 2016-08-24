/*
 * Copyright (C) 2016 The Android Open Source Project
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
package android.support.car.navigation;

import android.graphics.Bitmap;
import android.support.car.CarNotConnectedException;

/**
 * @hide
 */
public class CarNavigationManagerEmbedded extends CarNavigationManager {

    private final android.car.navigation.CarNavigationManager mManager;
    private CarNavigationListenerProxy mListener;

    public CarNavigationManagerEmbedded(Object manager) {
        mManager = (android.car.navigation.CarNavigationManager) manager;
    }

    /**
     * @param status new instrument cluster navigation status.
     * @return true if successful.
     * @throws CarNotConnectedException
     */
    @Override
    public boolean sendNavigationStatus(int status) throws CarNotConnectedException {
        try {
            return mManager.sendNavigationStatus(status);
        } catch (android.car.CarNotConnectedException e) {
           throw new CarNotConnectedException(e);
        }
    }

    @Override
    public boolean sendNavigationTurnEvent(int event, String road, int turnAngle, int turnNumber,
            Bitmap image, int turnSide) throws CarNotConnectedException {
        try {
            return mManager.sendNavigationTurnEvent(event, road, turnAngle, turnNumber, image,
                    turnSide);
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    @Override
    public boolean sendNavigationTurnDistanceEvent(int distanceMeters, int timeSeconds)
            throws CarNotConnectedException {
        try {
            return mManager.sendNavigationTurnDistanceEvent(distanceMeters, timeSeconds);
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    @Override
    public boolean isInstrumentClusterSupported() throws CarNotConnectedException {
        try {
            return mManager.isInstrumentClusterSupported();
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    @Override
    public void onCarDisconnected() {
        //nothing to do
    }

    @Override
    public void registerListener(CarNavigationListener listener)
            throws CarNotConnectedException {
        CarNavigationListenerProxy proxy = null;
        synchronized (this) {
            proxy = new CarNavigationListenerProxy(listener);
            mListener = proxy;
        }
        try {
            mManager.registerListener(proxy);
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    @Override
    public void unregisterListener() {
        synchronized (this) {
            mListener = null;
        }
        mManager.unregisterListener();
    }

    private static CarNavigationInstrumentCluster convert(
            android.car.navigation.CarNavigationInstrumentCluster ic) {
        if (ic == null) {
            return null;
        }
        return new CarNavigationInstrumentCluster(ic.getMinIntervalMs(), ic.getType(),
                ic.getImageWidth(), ic.getImageHeight(), ic.getImageColorDepthBits());
    }

    private static class CarNavigationListenerProxy implements
            android.car.navigation.CarNavigationManager.CarNavigationListener {

        private final CarNavigationListener mListener;

        private CarNavigationListenerProxy(CarNavigationListener listener) {
            mListener = listener;
        }

        @Override
        public void onInstrumentClusterStart(
                android.car.navigation.CarNavigationInstrumentCluster instrumentCluster) {
            mListener.onInstrumentClusterStart(convert(instrumentCluster));
        }

        @Override
        public void onInstrumentClusterStop() {
            mListener.onInstrumentClusterStop();
        }
    }
}
