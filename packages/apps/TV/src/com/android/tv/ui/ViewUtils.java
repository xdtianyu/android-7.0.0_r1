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

package com.android.tv.ui;

import android.util.Log;
import android.view.View;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * A class that includes convenience methods for view classes.
 */
public class ViewUtils {
    private static final String TAG = "ViewUtils";

    private ViewUtils() {
        // Prevent instantiation.
    }

    public static void setTransitionAlpha(View v, float alpha) {
        Method method;
        try {
            method = View.class.getDeclaredMethod("setTransitionAlpha", Float.TYPE);
            method.invoke(v, alpha);
        } catch (NoSuchMethodException|IllegalAccessException|IllegalArgumentException
                |InvocationTargetException e) {
            Log.e(TAG, "Fail to call View.setTransitionAlpha", e);
        }
    }
}
