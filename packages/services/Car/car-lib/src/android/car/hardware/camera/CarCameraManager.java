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
import android.car.Car;
import android.car.CarManagerBase;
import android.car.CarNotConnectedException;
import android.content.Context;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

/**
 * API for controlling camera system in cars
 * @hide
 */
@SystemApi
public class CarCameraManager implements CarManagerBase {
    public final static boolean DBG = true;
    public final static String TAG = CarCameraManager.class.getSimpleName();

    // Camera capabilities flags
    public static final int ANDROID_OVERLAY_SUPPORT_FLAG    = 0x1;
    public static final int CAMERA_CROP_SUPPORT_FLAG        = 0x2;
    public static final int CAMERA_POSITIONING_SUPPORT_FLAG = 0x4;

    // Camera types
    public static final int CAR_CAMERA_TYPE_NONE            = 0;
    public static final int CAR_CAMERA_TYPE_RVC             = 1;

    private int[] mCameraList;
    private final ICarCamera mService;

    /**
     * Get an instance of the CarCameraManager.
     *
     * Should not be obtained directly by clients, use {@link Car.getCarManager()} instead.
     * @hide
     */
    public CarCameraManager(IBinder service, Context context) throws CarNotConnectedException{
        mService = ICarCamera.Stub.asInterface(service);
        try {
            mCameraList = mService.getCameraList();
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in getCameraList", e);
            mCameraList = null;
            throw new CarNotConnectedException(e);
        }
    }

    /**
     *
     * @return Array of CAR_CAMERA_TYPE_* telling which cameras are present
     */
    public int[] getCameraList() {
        return mCameraList;
    }

    /**
     *
     * @param cameraType Camera type to query capabilites
     * @return Bitmask of camera capabilities available for this device
     * @throws CarNotConnectedException
     */
    public int getCameraCapabilities(int cameraType) throws CarNotConnectedException {
        int capabilities;
        try {
            capabilities = mService.getCapabilities(cameraType);
        } catch (RemoteException e) {
            Log.e(TAG, "Exception in getCameraCapabilities", e);
            throw new CarNotConnectedException(e);
        }
        return capabilities;
    }

    public CarCamera openCamera(int cameraType) {
        CarCamera camera = null;

        // Find cameraType in the list of available cameras
        for (int i : mCameraList) {
            if(i == cameraType) {
                camera = new CarCamera(mService, cameraType);
                break;
            }
        }
        return camera;
    }

    public void closeCamera(CarCamera camera) {
        // TODO:  What should we do?
    }

    /** @hide */
    @Override
    public void onCarDisconnected() {
    }
}
