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

package android.car.hardware.camera;

import android.annotation.SystemApi;
import android.car.CarNotConnectedException;
import android.graphics.Rect;
import android.os.RemoteException;
import android.util.Log;

/**
 * API for controlling camera system in cars
 * @hide
 */
@SystemApi
public class CarCamera {
    public final static String TAG = CarCamera.class.getSimpleName();
    public final int mCameraType;
    private final ICarCamera mService;

    public CarCamera(ICarCamera service, int cameraType) {
        mService = service;
        mCameraType = cameraType;
    }

    public int getCapabilities() throws CarNotConnectedException {
        try {
            return mService.getCapabilities(mCameraType);
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in getCapabilities", e);
            throw new CarNotConnectedException(e);
        }
    }

    public Rect getCameraCrop() throws CarNotConnectedException {
        try {
            return mService.getCameraCrop(mCameraType);
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in getCameraCrop", e);
            throw new CarNotConnectedException(e);
        }
    }

    public void setCameraCrop(Rect rect) throws CarNotConnectedException {
        try {
            mService.setCameraCrop(mCameraType, rect);
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in setCameraCrop", e);
            throw new CarNotConnectedException(e);
        }
    }

    public Rect getCameraPosition() throws CarNotConnectedException {
        try {
            return mService.getCameraPosition(mCameraType);
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in getCameraPosition", e);
            throw new CarNotConnectedException(e);
        }
    }

    public void setCameraPosition(Rect rect) throws CarNotConnectedException {
        try {
            mService.setCameraPosition(mCameraType, rect);
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in setCameraPosition", e);
        }
    }

    public CarCameraState getCameraState() throws CarNotConnectedException {
        try {
            return mService.getCameraState(mCameraType);
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in getCameraState", e);
            throw new CarNotConnectedException(e);
        }
    }

    public void setCameraState(CarCameraState state) throws CarNotConnectedException {
        try {
            mService.setCameraState(mCameraType, state);
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in setCameraState", e);
            throw new CarNotConnectedException(e);
        }
    }
}
