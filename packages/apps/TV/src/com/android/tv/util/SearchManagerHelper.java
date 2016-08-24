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

package com.android.tv.util;

import android.app.SearchManager;
import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.os.UserHandle;
import android.util.Log;

import java.lang.reflect.InvocationTargetException;

/**
 * A convenience class for calling methods in android.app.SearchManager.
 */
public final class SearchManagerHelper {
    private static final String TAG = "SearchManagerHelper";

    private static final Object sLock = new Object();
    private static SearchManagerHelper sInstance;

    private final SearchManager mSearchManager;

    private SearchManagerHelper(Context context) {
        mSearchManager = ((android.app.SearchManager) context.getSystemService(
                Context.SEARCH_SERVICE));
    }

    public static SearchManagerHelper getInstance(Context context) {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new SearchManagerHelper(context.getApplicationContext());
            }
            return sInstance;
        }
    }

    public void launchAssistAction() {
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                SearchManager.class.getDeclaredMethod(
                        "launchLegacyAssist", String.class, Integer.TYPE, Bundle.class).invoke(
                                mSearchManager, null, UserHandle.myUserId(), null);
            } else {
                SearchManager.class.getDeclaredMethod(
                        "launchAssistAction", Integer.TYPE, String.class, Integer.TYPE).invoke(
                                mSearchManager, 0, null, UserHandle.myUserId());
            }
        }  catch (NoSuchMethodException | IllegalArgumentException | IllegalAccessException
                | InvocationTargetException e) {
            Log.e(TAG, "Fail to call SearchManager.launchAssistAction", e);
        }
    }
}
