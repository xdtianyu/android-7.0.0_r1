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

package android.support.car;

import android.os.Bundle;
import android.os.RemoteException;
import android.support.car.annotation.ValueTypeDef;

import java.lang.reflect.Field;
import java.util.HashMap;

/**
 * Utility to retrieve various static information from car. For given string keys, there can be
 * different types of values and right query API like {@link #getFloat(String)} for float
 * type, and {@link #getInt(String)} for int type, should be used. Passing a key string to wrong
 * API will lead into {@link IllegalArgumentException}. All get* apis return null if requested
 * property is not supported by the car. So caller should always check for null result.
 */
public abstract class CarInfoManager implements CarManagerBase {

    /**
     * Manufacturer of the car.
     */
    @ValueTypeDef(type = String.class)
    public static final String KEY_MANUFACTURER = "manufacturer";
    /**
     * Model name of the car. This information may not necessarily allow distinguishing different
     * car models as the same name may be used for different cars depending on manufacturers.
     */
    @ValueTypeDef(type = String.class)
    public static final String KEY_MODEL = "model";
    /**
     * Model year of the car in AC.
     */
    @ValueTypeDef(type = Integer.class)
    public static final String KEY_MODEL_YEAR = "model-year";
    /**
     * Unique identifier for the car. This is not VIN, and id is persistent until user resets it.
     */
    @ValueTypeDef(type = String.class)
    public static final String KEY_VEHICLE_ID = "vehicle-id";

    /**
     * Retrieve floating point information for car.
     * @param key
     * @return null if the key is not supported.
     * @throws CarNotConnectedException
     * @throws IllegalArgumentException
     */
    public abstract Float getFloat(String key)
            throws CarNotConnectedException, IllegalArgumentException;

    public abstract Integer getInt(String key)
            throws CarNotConnectedException, IllegalArgumentException;

    public abstract Long getLong(String key)
            throws CarNotConnectedException, IllegalArgumentException;

    public abstract String getString(String key)
            throws CarNotConnectedException, IllegalArgumentException;

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
    public abstract Bundle getBundle(String key)
            throws CarNotConnectedException, IllegalArgumentException;
}
