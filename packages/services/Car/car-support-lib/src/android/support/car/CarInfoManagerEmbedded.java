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

package android.support.car;

import android.os.Bundle;
import android.os.RemoteException;
import android.support.car.annotation.ValueTypeDef;

import java.lang.reflect.Field;
import java.util.HashMap;

/** @hide */
public class CarInfoManagerEmbedded extends CarInfoManager {

    private final android.car.CarInfoManager mManager;


    /** @hide */
    CarInfoManagerEmbedded(Object manager) {
        mManager = (android.car.CarInfoManager) manager;
    }

    @Override
    public Float getFloat(String key) throws CarNotConnectedException, IllegalArgumentException {
        try {
            return mManager.getFloat(key);
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    @Override
    public Integer getInt(String key) throws CarNotConnectedException, IllegalArgumentException {
        try {
            return mManager.getInt(key);
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    @Override
    public Long getLong(String key) throws CarNotConnectedException, IllegalArgumentException {
        try {
            return mManager.getLong(key);
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    @Override
    public String getString(String key) throws CarNotConnectedException, IllegalArgumentException {
        try {
            return mManager.getString(key);
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    /**
     * get Bundle for the given key. This is intended for passing vendor specific data for key
     * defined only for the car vendor. Vendor extension can be used for other APIs like
     * getInt / getString, but this is for passing more complex data.
     * @param key
     * @return
     * @throws CarNotConnectedException
     * @throws IllegalArgumentException
     * @hide
     */
    @Override
    public Bundle getBundle(String key) throws CarNotConnectedException, IllegalArgumentException {
        try {
            return mManager.getBundle(key);
        } catch (android.car.CarNotConnectedException e) {
            throw new CarNotConnectedException(e);
        }
    }

    /** @hide */
    @Override
    public void onCarDisconnected() {
        //nothing to do
    }
}
