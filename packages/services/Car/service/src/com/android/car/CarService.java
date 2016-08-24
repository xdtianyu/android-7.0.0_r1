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

import java.io.FileDescriptor;
import java.io.PrintWriter;

import com.android.car.hal.VehicleHal;

import android.app.Service;
import android.car.Car;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class CarService extends Service {

    // main thread only
    private ICarImpl mICarImpl;

    @Override
    public void onCreate() {
        Log.i(CarLog.TAG_SERVICE, "Service onCreate");
        mICarImpl = ICarImpl.getInstance(this);
        super.onCreate();
    }

    @Override
    public void onDestroy() {
        Log.i(CarLog.TAG_SERVICE, "Service onDestroy");
        ICarImpl.releaseInstance();
        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // keep it alive.
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mICarImpl;
    }

    @Override
    protected void dump(FileDescriptor fd, PrintWriter writer, String[] args) {
        writer.println("*dump car service*");
        writer.println("*dump HAL*");
        VehicleHal.getInstance().dump(writer);
        writer.println("*dump services*");
        ICarImpl.getInstance(this).dump(writer);
    }
}
