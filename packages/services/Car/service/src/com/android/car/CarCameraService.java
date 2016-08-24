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

import android.content.Context;
import android.car.hardware.camera.CarCameraState;
import android.car.hardware.camera.ICarCamera;
import android.graphics.Rect;
import android.util.Log;

import java.io.PrintWriter;
import java.util.Collection;
import java.util.HashMap;
import java.util.Set;

public class CarCameraService extends ICarCamera.Stub implements CarServiceBase {
    public static final boolean DBG = false;
    public static final String  TAG = CarLog.TAG_CAMERA + ".CarCameraService";

    private final Context mContext;
    private long mModule;
    private final HashMap<Integer, Long> mDeviceMap;

    public CarCameraService(Context context) {
        mContext = context;
        mDeviceMap = new HashMap<Integer, Long>();
    }

    @Override
    public synchronized void init() {
        if (DBG) {
            Log.d(TAG, "init called");
        }
        mModule = nativeOpen();

        if (mModule != 0) {
            int[] cameraType = nativeGetSupportedCameras(mModule);

            if (cameraType != null) {
                for (int i : cameraType) {
                    long devicePtr = nativeGetDevice(mModule, i);
                    if (devicePtr == 0) {
                        Log.e(TAG, "Null device pointer returned for cameraType = " + i);
                    } else {
                        mDeviceMap.put(i, devicePtr);
                    }
                }
            } else {
                Log.e(TAG, "No car cameras are supported");
            }
        } else {
            Log.w(TAG, "Cannot load camera module");
        }
    }

    @Override
    public synchronized void release() {
        if (DBG) {
            Log.d(TAG, "release called");
        }
        Collection<Long> devices = mDeviceMap.values();
        for (Long device : devices) {
            nativeClose(device);
        }
        mDeviceMap.clear();
        mModule = 0;
    }

    @Override
    public void dump(PrintWriter writer) {
        // TODO
    }

    @Override
    public int[] getCameraList() {
        if (DBG) {
            Log.d(TAG, "getCameraList called");
        }
        Set<Integer> keySet;
        synchronized (this) {
            keySet = mDeviceMap.keySet();
        }

        int keySetSize = keySet.size();

        if (keySetSize > 0) {
            int[] keyArray = new int[keySet.size()];
            int i = 0;
            for (Integer key : keySet) {
                keyArray[i++] = key.intValue();
            }
            return keyArray;
        } else {
            return null;
        }
    }

    @Override
    public int getCapabilities(int cameraType) {
        if (DBG) {
            Log.d(TAG, "getCapabilities called, type = " + String.valueOf(cameraType));
        }
        synchronized (this) {
            long device = getDeviceIdLocked(cameraType);
            return nativeGetCapabilities(device);
        }
    }

    @Override
    public Rect getCameraCrop(int cameraType) {
        Rect rect;
        synchronized (this) {
            long device = getDeviceIdLocked(cameraType);
            rect = nativeGetCameraCrop(device);
        }
        if(DBG && (rect != null)) {
            Log.d(TAG, "getCameraCrop called:  " + rect.toString());
        }
        return rect;
    }

    @Override
    public void setCameraCrop(int cameraType, Rect rect) {
        if (DBG) {
            Log.d(TAG, "setCameraCrop called." + rect.toString());
        }
        synchronized (this) {
            long device = getDeviceIdLocked(cameraType);
            nativeSetCameraCrop(device, rect);
        }
    }

    @Override
    public Rect getCameraPosition(int cameraType) {
        Rect rect;
        synchronized (this) {
            long device = getDeviceIdLocked(cameraType);
            rect = nativeGetCameraPosition(device);
        }
        if(DBG && (rect != null)) {
            Log.d(TAG, "getCameraPosition called:  " + rect.toString());
        }
        return rect;
    }

    @Override
    public void setCameraPosition(int cameraType, Rect rect) {
        if (DBG) {
            Log.d(TAG, "setCameraPosition called." + rect.toString());
        }
        synchronized (this) {
            long device = getDeviceIdLocked(cameraType);
            nativeSetCameraPosition(device, rect);
        }
    }

    @Override
    public CarCameraState getCameraState(int cameraType) {
        CarCameraState state;
        synchronized (this) {
            long device = getDeviceIdLocked(cameraType);
            state = nativeGetCameraState(device);
        }
        if(DBG && (state != null)) {
            Log.d(TAG, "getCameraState called: " + state.toString());
        }
        return state;
    }

    @Override
    public void setCameraState(int cameraType, CarCameraState state) {
        if (DBG) {
            Log.d(TAG, "setCameraState called.  state: " + state.toString());
        }
        synchronized (this) {
            long device = getDeviceIdLocked(cameraType);
            nativeSetCameraState(device, state);
        }
    }

    /**
     * Validates that the cameraType is available and ready to be used.
     * @param cameraType
     * @return
     */
    private long getDeviceIdLocked(int cameraType) {
        Long deviceId = mDeviceMap.get(cameraType);

        if (deviceId == null) {
            throw new IllegalArgumentException("cameraType " + cameraType + " doesn't exist in"
                    + "device map");
        }
        return deviceId;
    }

    /*
     * Native function definitions
     */
    private native long nativeOpen();
    private native void nativeClose(long module);
    private native int[] nativeGetSupportedCameras(long module);
    private native long nativeGetDevice(long module, int cameraType);
    private native int nativeGetCapabilities(long device);
    private native Rect nativeGetCameraCrop(long device);
    private native void nativeSetCameraCrop(long device, Rect rect);
    private native Rect nativeGetCameraPosition(long device);
    private native void nativeSetCameraPosition(long device, Rect rect);
    private native CarCameraState nativeGetCameraState(long device);
    private native void nativeSetCameraState(long device, CarCameraState state);
}
