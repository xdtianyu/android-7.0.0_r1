/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tv.settings.util;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import java.lang.reflect.Method;

// This is a helper class to allow for light-weight transmission of Bitmap data through
// an intent extra.
public class ActivityTransitionBitmapHelper {

    private static final String TAG = "ActivityTransitionBitmapHelper";
    private static final String EXTRA_BINDER =
            "com.android.tv.settings.util.extra_binder";
    private static Method sPutBinder;
    private static Method sGetBinder;

    static {
        // TODO: switch to not use reflection when everybody is building against the right target
        try {
            sPutBinder = Bundle.class.getDeclaredMethod("putBinder", String.class, IBinder.class);
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
        try {
            sGetBinder = Bundle.class.getDeclaredMethod("getBinder", String.class);
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
    }

    public static Bitmap getBitmapFromBinderBundle(Bundle bundle) {
        if (bundle.containsKey(EXTRA_BINDER)) {

            IActivityTransitionBitmapProvider provider = null;
            IBinder binder = null;
            if (sGetBinder != null) {
                try {
                    binder = (IBinder) sGetBinder.invoke(bundle, EXTRA_BINDER);
                } catch (Exception e) {
                    Log.e(TAG, e.getMessage());
                }
            }

            if (binder != null) {
                provider = IActivityTransitionBitmapProvider.
                        Stub.asInterface(binder);
            }
            if (provider != null) {
                try {
                    return provider.getTransitionBitmap();
                } catch (RemoteException e) {
                    Log.d(TAG, "The remote process is not accessible, maybe it was killed.", e);
                }
            }
        }
        return null;
    }

    public static Bundle bitmapAsBinderBundle(Bitmap bitmap) {
        ActivityTransitionBitmapProvider provider = new ActivityTransitionBitmapProvider(bitmap);
        Bundle bundle = new Bundle();

        if (sPutBinder != null) {
            try {
                sPutBinder.invoke(bundle, EXTRA_BINDER, provider);
            } catch (Exception e) {
                Log.e(TAG, "Error invoking binder. ", e);
            }
        }

        return bundle;
    }

    private static class ActivityTransitionBitmapProvider extends
            IActivityTransitionBitmapProvider.Stub {
        private Bitmap mBitmap;

        public ActivityTransitionBitmapProvider(Bitmap bitmap) {
            mBitmap = bitmap;
        }

        @Override
        public Bitmap getTransitionBitmap() {
            Bitmap b = mBitmap;

            // We don't want to hold on to the bitmap object longer than necessary
            mBitmap = null;
            return b;
        }
    }
}
