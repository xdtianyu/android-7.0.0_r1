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
import android.car.hardware.CarSensorEvent;
import android.car.hardware.CarSensorManager;
import android.content.Context;
import android.os.SystemClock;
import android.util.Log;

import com.android.car.hal.SensorHalServiceBase.SensorListener;

import java.io.PrintWriter;

//TODO
public class DayNightModePolicy extends CarSensorService.LogicalSensorHalBase {

    private final Context mContext;
    private SensorListener mSensorListener;
    private boolean mIsReady = false;
    private boolean mStarted = false;

    public DayNightModePolicy(Context context) {
        mContext = context;
    }

    @Override
    public synchronized void init() {
        mIsReady = true;
    }

    @Override
    public synchronized void release() {
        // TODO Auto-generated method stub
    }

    public static CarSensorEvent getDefaultValue(int sensorType) {
        return createEvent(true /* isNight */);
    }

    @Override
    public synchronized void registerSensorListener(SensorListener listener) {
        mSensorListener = listener;
        if (mIsReady) {
            mSensorListener.onSensorHalReady(this);
        }
    }

    @Override
    public synchronized void onSensorServiceReady() {
        // TODO Auto-generated method stub
    }

    @Override
    public synchronized boolean isReady() {
        return mIsReady;
    }

    @Override
    public synchronized int[] getSupportedSensors() {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public synchronized boolean requestSensorStart(int sensorType, int rate) {
        mStarted = true;
        // TODO Auto-generated method stub
        Log.w(CarLog.TAG_SENSOR,
                "DayNightModePolicy.requestSensorStart, default policy not implemented");
        return false;
    }

    @Override
    public synchronized void requestSensorStop(int sensorType) {
        // TODO Auto-generated method stub
    }

    private static CarSensorEvent createEvent(boolean isNight) {
        CarSensorEvent event = new CarSensorEvent(CarSensorManager.SENSOR_TYPE_NIGHT,
                SystemClock.elapsedRealtimeNanos(), 0, 1);
        event.intValues[0] = isNight ? 1 : 0;
        return event;
    }

    @Override
    public synchronized void dump(PrintWriter writer) {
        // TODO Auto-generated method stub
    }
}
