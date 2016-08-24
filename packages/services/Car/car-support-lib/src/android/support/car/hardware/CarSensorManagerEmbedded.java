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

package android.support.car.hardware;

import android.support.car.CarNotConnectedException;

import java.util.LinkedList;

/**
 *  @hide
 */
public class CarSensorManagerEmbedded extends CarSensorManager {

    private final android.car.hardware.CarSensorManager mManager;
    private final LinkedList<CarSensorEventListenerProxy> mListeners = new LinkedList<>();

    public CarSensorManagerEmbedded(Object manager) {
        mManager = (android.car.hardware.CarSensorManager) manager;
    }

    @Override
    public int[] getSupportedSensors() throws CarNotConnectedException {
        try {
            return mManager.getSupportedSensors();
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    @Override
    public boolean isSensorSupported(int sensorType) throws CarNotConnectedException {
        try {
            return mManager.isSensorSupported(sensorType);
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    @Override
    public boolean registerListener(CarSensorEventListener listener, int sensorType,
            int rate) throws CarNotConnectedException, IllegalArgumentException {
        CarSensorEventListenerProxy proxy = null;
        synchronized (this) {
            proxy = findListenerLocked(listener);
            if (proxy == null) {
                proxy = new CarSensorEventListenerProxy(listener, sensorType);
            } else {
                proxy.sensors |= sensorType;
            }
        }
        try {
            return mManager.registerListener(proxy, sensorType, rate);
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    @Override
    public void unregisterListener(CarSensorEventListener listener)
            throws CarNotConnectedException {
        CarSensorEventListenerProxy proxy = null;
        synchronized (this) {
            proxy = findListenerLocked(listener);
            if (proxy == null) {
                return;
            }
            mListeners.remove(proxy);
        }
        try {
            mManager.unregisterListener(proxy);
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    @Override
    public void unregisterListener(CarSensorEventListener listener, int sensorType)
            throws CarNotConnectedException {
        CarSensorEventListenerProxy proxy = null;
        synchronized (this) {
            proxy = findListenerLocked(listener);
            if (proxy == null) {
                return;
            }
            proxy.sensors &= ~sensorType;
            if (proxy.sensors == 0) {
                mListeners.remove(proxy);
            }
        }
        try {
            mManager.unregisterListener(proxy, sensorType);
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    @Override
    public CarSensorEvent getLatestSensorEvent(int type) throws CarNotConnectedException {
        try {
            return convert(mManager.getLatestSensorEvent(type));
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    @Override
    public void onCarDisconnected() {
        //nothing to do
    }

    private CarSensorEventListenerProxy findListenerLocked(CarSensorEventListener listener) {
        for (CarSensorEventListenerProxy proxy : mListeners) {
            if (proxy.listener == listener) {
                return proxy;
            }
        }
        return null;
    }

    private static CarSensorEvent convert(android.car.hardware.CarSensorEvent event) {
        if (event == null) {
            return null;
        }
        return new CarSensorEvent(event.sensorType, event.timeStampNs, event.floatValues,
                event.intValues);
    }

    private static class CarSensorEventListenerProxy implements
            android.car.hardware.CarSensorManager.CarSensorEventListener {

        public final CarSensorEventListener listener;
        public int sensors;

        public CarSensorEventListenerProxy(CarSensorEventListener listener, int sensors) {
            this.listener = listener;
            this.sensors = sensors;
        }

        @Override
        public void onSensorChanged(android.car.hardware.CarSensorEvent event) {
            CarSensorEvent newEvent = convert(event);
            listener.onSensorChanged(newEvent);
        }
    }
}
