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

package com.android.server.wifi;

import android.os.Bundle;
import android.util.Log;

import com.android.server.wifi.WifiNative;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.lang.reflect.Field;
import java.util.Iterator;

public class HalMockUtils {
    private static final String TAG = "HalMockUtils";
    private static final boolean DBG = true;
    private static final boolean VDBG = true;

    private static native int initHalMock();

    public static native void setHalMockObject(Object obj);

    static {
        System.loadLibrary("wifi-hal-mock");
    }

    public static void initHalMockLibrary() throws Exception {
        /*
         * Setting the Wi-Fi HAL handle and interface (array) to dummy
         * values. Required to fake the init checking code to think that
         * the HAL actually started.
         *
         * Note that values are not important since they are only used by
         * the real HAL - which is mocked-out in this use-case.
         */
        Field field = WifiNative.class.getDeclaredField("sWifiHalHandle");
        field.setAccessible(true);
        long currentWifiHalHandle = field.getLong(null);
        if (DBG) Log.d(TAG, "currentWifiHalHandle=" + currentWifiHalHandle);
        if (currentWifiHalHandle == 0) {
            field.setLong(null, 5);

            field = WifiNative.class.getDeclaredField("sWifiIfaceHandles");
            field.setAccessible(true);
            long[] wifiIfaceHandles = {
                    10 };
            field.set(null, wifiIfaceHandles);
        }

        initHalMock();
    }

    /*
     * JSON data-model for passing arguments between mock host (java) and mock
     * HAL (C):
     * {
     *      "name" : { "type" : "int|byte_array", "value" : 123 | [1, 2, 3, 4] }
     * }
     */

    private static final String TYPE_KEY = "type";
    private static final String VALUE_KEY = "value";

    private static final String TYPE_INT = "int";
    private static final String TYPE_BYTE_ARRAY = "byte_array";

    public static Bundle convertJsonToBundle(String jsonArgs) throws JSONException {
        if (VDBG) Log.v(TAG, "convertJsonToBundle: jsonArgs=" + jsonArgs);

        Bundle bundle = new Bundle();

        JSONObject jsonObject = new JSONObject(jsonArgs);
        Iterator<String> iter = jsonObject.keys();
        while (iter.hasNext()) {
            String key = iter.next();
            JSONObject field = jsonObject.optJSONObject(key);

            String type = field.getString(TYPE_KEY);

            if (TYPE_INT.equals(type)) {
                bundle.putInt(key, field.optInt(VALUE_KEY));
            } else if (TYPE_BYTE_ARRAY.equals(type)) {
                JSONArray array = field.optJSONArray(VALUE_KEY);
                byte[] bArray = new byte[array.length()];
                for (int i = 0; i < array.length(); ++i) {
                    bArray[i] = (byte) array.getInt(i);
                }
                bundle.putByteArray(key, bArray);
            } else {
                throw new JSONException("Unexpected TYPE read from mock HAL -- '" + type + "'");
            }
        }

        if (DBG) Log.d(TAG, "convertJsonToBundle: returning bundle=" + bundle);
        return bundle;
    }

    public static JSONObject convertBundleToJson(Bundle bundle) throws JSONException {
        if (VDBG) Log.v(TAG, "convertBundleToJson: bundle=" + bundle.toString());

        JSONObject json = new JSONObject();
        for (String key : bundle.keySet()) {
            Object value = bundle.get(key);
            JSONObject child = new JSONObject();
            if (value instanceof Integer) {
                child.put(TYPE_KEY, TYPE_INT);
                child.put(VALUE_KEY, ((Integer) value).intValue());
            } else if (value instanceof byte[]) {
                byte[] array = (byte[]) value;
                JSONArray jsonArray = new JSONArray();
                for (int i = 0; i < array.length; ++i) {
                    jsonArray.put(array[i]);
                }
                child.put(TYPE_KEY, TYPE_BYTE_ARRAY);
                child.put(VALUE_KEY, jsonArray);
            } else {
                throw new JSONException("Unexpected type of JSON tree node (not an Integer "
                        + "or byte[]): " + value);
            }
            json.put(key, child);
        }

        if (DBG) Log.d(TAG, "convertBundleToJson: returning JSONObject=" + json);
        return json;
    }
}
