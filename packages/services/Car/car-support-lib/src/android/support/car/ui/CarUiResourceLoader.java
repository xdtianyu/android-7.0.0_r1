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
package android.support.car.ui;

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.util.DisplayMetrics;
import android.util.Log;


public class CarUiResourceLoader {
    private static final String TAG = "CarUiResourceLoader";
    private static final String CAR_UI_PACKAGE = "android.car.ui.provider";
    private static final String DRAWABLE = "drawable";
    private static final String BOOL = "bool";
    private static final String DIMEN = "dimen";

    public static synchronized Drawable getDrawable(
            Context context, String drawableName) {
        return getDrawable(context, drawableName, null);
    }

    public static synchronized Drawable getDrawable(
            Context context, String drawableName, DisplayMetrics metrics) {
        Resources res;
        try {
            res = context.getPackageManager().getResourcesForApplication(CAR_UI_PACKAGE);
        } catch (NameNotFoundException e) {
            Log.w(TAG, "CarUiProvider not installed, this class will return blank drawables.");
            return new ColorDrawable(Color.TRANSPARENT);
        }

        int id = res.getIdentifier(drawableName, DRAWABLE, CAR_UI_PACKAGE);
        if (id == 0) {
            Log.w(TAG, "Resource not found in CarUiProvider.apk: " + drawableName);
            return new ColorDrawable(Color.TRANSPARENT);
        }
        if (metrics == null) {
            return res.getDrawable(id, null);
        } else {
            return res.getDrawableForDensity(id, metrics.densityDpi, null);
        }
    }

    public static synchronized boolean getBoolean(
            Context context, String boolName, boolean def) {
        Resources res;
        try {
            res = context.getPackageManager().getResourcesForApplication(CAR_UI_PACKAGE);
        } catch (NameNotFoundException e) {
            Log.w(TAG, "CarUiProvider not installed, returning default");
            return def;
        }

        int id = res.getIdentifier(boolName, BOOL, CAR_UI_PACKAGE);
        if (id == 0) {
            Log.w(TAG, "Resource not found in CarUiProvider.apk: " + boolName);
            return def;
        }
        return res.getBoolean(id);
    }

    public static synchronized float getDimen(
            Context context, String dimenName, float def) {
        Resources res;
        try {
            res = context.getPackageManager().getResourcesForApplication(CAR_UI_PACKAGE);
        } catch (NameNotFoundException e) {
            Log.w(TAG, "CarUiProvider not installed, returning default");
            return def;
        }

        int id = res.getIdentifier(dimenName, DIMEN, CAR_UI_PACKAGE);
        if (id == 0) {
            Log.w(TAG, "Resource not found in CarUiProvider.apk: " + dimenName);
            return def;
        }
        return res.getDimension(id);
    }
}
