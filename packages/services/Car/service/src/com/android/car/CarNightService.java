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

package com.android.car;

import android.app.UiModeManager;
import android.car.Car;
import android.car.CarNotConnectedException;
import android.car.hardware.CarSensorEvent;
import android.car.hardware.CarSensorManager;
import android.car.hardware.ICarSensorEventListener;
import android.content.Context;
import android.util.Log;

import java.io.PrintWriter;
import java.util.List;


public class CarNightService implements CarServiceBase {

    public static final boolean DBG = true;
    private int mNightSetting = UiModeManager.MODE_NIGHT_YES;
    private final Context mContext;
    private final UiModeManager mUiModeManager;
    private CarSensorService mCarSensorService;

    private final ICarSensorEventListener mICarSensorEventListener =
            new ICarSensorEventListener.Stub() {
        @Override
        public void onSensorChanged(List<CarSensorEvent> events) {
            if (!events.isEmpty()) {
                CarSensorEvent event = events.get(events.size() - 1);
                handleSensorEvent(event);
            }
        }
    };

    public synchronized void handleSensorEvent(CarSensorEvent event) {
        if (event == null) {
            return;
        }
        if (event.sensorType == CarSensorManager.SENSOR_TYPE_NIGHT) {
            if (event.intValues[0] == 1) {
                mNightSetting = UiModeManager.MODE_NIGHT_YES;
            }
            else {
                mNightSetting = UiModeManager.MODE_NIGHT_NO;
            }
            if (mUiModeManager != null) {
                mUiModeManager.setNightMode(mNightSetting);
            }
        }
    }

    CarNightService(Context context) {
        mContext = context;
        mUiModeManager = (UiModeManager) mContext.getSystemService(Context.UI_MODE_SERVICE);
        if (mUiModeManager == null) {
            Log.w(CarLog.TAG_SENSOR,"Failed to get UI_MODE_SERVICE");
        }
    }

    @Override
    public synchronized void init() {
        if (DBG) {
            Log.d(CarLog.TAG_SENSOR,"CAR dayNight init.");
        }
        mCarSensorService = (CarSensorService) ICarImpl.getInstance(mContext).getCarService(
                Car.SENSOR_SERVICE);
        mCarSensorService.registerOrUpdateSensorListener(CarSensorManager.SENSOR_TYPE_NIGHT,
                CarSensorManager.SENSOR_RATE_NORMAL, mICarSensorEventListener);
        CarSensorEvent currentState = mCarSensorService.getLatestSensorEvent(
                CarSensorManager.SENSOR_TYPE_NIGHT);
        handleSensorEvent(currentState);
    }

    @Override
    public synchronized void release() {
    }

    @Override
    public synchronized void dump(PrintWriter writer) {
        writer.println("*DAY NIGHT POLICY*");
        writer.println("Mode:" + ((mNightSetting == UiModeManager.MODE_NIGHT_YES) ? "night" : "day")
                );
    }
}
