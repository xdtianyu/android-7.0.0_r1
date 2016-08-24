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

import android.car.CarInfoManager;
import android.car.ICarInfo;
import android.car.annotation.ValueTypeDef;
import android.content.Context;
import android.os.Bundle;
import android.provider.Settings;

import com.android.car.hal.InfoHalService;
import com.android.car.hal.VehicleHal;

import java.io.PrintWriter;
import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Map;

public class CarInfoService extends ICarInfo.Stub implements CarServiceBase {

    private static final HashMap<String, Class> sKeyValueTypeMap;
    static {
        sKeyValueTypeMap = new HashMap<String, Class>();
        for (Field f : CarInfoManager.class.getDeclaredFields()) {
            try {
                if (f.isAnnotationPresent(ValueTypeDef.class) && f.getType() == String.class) {
                    ValueTypeDef typeDef = f.getAnnotation(ValueTypeDef.class);
                    Class type = typeDef.type();
                    sKeyValueTypeMap.put((String)f.get(null), type);
                }
            } catch (IllegalAccessException e) {
                //ignore
            }
        }
    }

    private final InfoHalService mInfoHal;
    private final HashMap<String, Object> mInfoCache = new HashMap<String, Object>();
    private final Context mContext;

    public CarInfoService(Context context) {
        mInfoHal = VehicleHal.getInstance().getInfoHal();
        mContext = context;
    }

    @Override
    public int[] getInt(String key) {
        assertType(key, Integer.class);
        Object o = findFromCache(key);
        if (o != null) {
            return (int[])o;
        }
        int[] v = mInfoHal.getInt(key);
        if (v != null) {
            storeToCache(key, v);
        }
        return v;
    }

    @Override
    public float[] getFloat(String key) {
        assertType(key, Float.class);
        Object o = findFromCache(key);
        if (o != null) {
            return (float[])o;
        }
        float[] v = mInfoHal.getFloat(key);
        if (v != null) {
            storeToCache(key, v);
        }
        return v;
    }

    @Override
    public long[] getLong(String key) {
        assertType(key, Long.class);
        Object o = findFromCache(key);
        if (o != null) {
            return (long[])o;
        }
        long[] v = mInfoHal.getLong(key);
        if (v != null) {
            storeToCache(key, v);
        }
        return v;
    }

    @Override
    public String getString(String key) {
        assertType(key, String.class);
        if (CarInfoManager.KEY_VEHICLE_ID.equals(key)) { // do not show real ID.
            // never put this into cache as ANDROID_ID can be changed.
            return Settings.Secure.getString(mContext.getContentResolver(),
                    Settings.Secure.ANDROID_ID);
        }
        Object o = findFromCache(key);
        if (o != null) {
            return (String)o;
        }
        String v = mInfoHal.getString(key);
        if (v != null) {
            storeToCache(key, v);
        }
        return v;
    }

    @Override
    public Bundle getBundle(String key) {
        // OEM may extend this.
        return null;
    }

    @Override
    public void init() {
        //nothing to do
    }

    @Override
    public synchronized void release() {
        mInfoCache.clear();
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println("*CarInfoService*");
        writer.println("***Dump info cache***");
        for (Map.Entry<String, Object> entry : mInfoCache.entrySet()) {
            writer.println(entry.getKey() + ":" + entry.getValue());
        }
    }

    private synchronized Object findFromCache(String key) {
        return mInfoCache.get(key);
    }

    private synchronized void storeToCache(String key, Object value) {
        mInfoCache.put(key, value);
    }

    private void assertType(String key, Class type) throws IllegalArgumentException {
        Class expectedType = sKeyValueTypeMap.get(key);
        if (expectedType == null || !expectedType.equals(type)) {
            throw new IllegalArgumentException("Given key " + key + " expects type " +
                    expectedType + " while used in method for " + type);
        }
    }
}
