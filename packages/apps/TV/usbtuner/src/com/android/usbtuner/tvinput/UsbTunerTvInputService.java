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

package com.android.usbtuner.tvinput;

import android.content.ComponentName;
import android.content.Context;
import android.media.tv.TvContract;
import android.os.Environment;
import android.util.Log;

import com.android.usbtuner.exoplayer.cache.CacheManager;
import com.android.usbtuner.exoplayer.cache.TrickplayStorageManager;
import com.android.usbtuner.util.SystemPropertiesProxy;

import java.io.File;

/**
 * {@link UsbTunerTvInputService} serves TV channels coming from a usb tuner device.
 */
public class UsbTunerTvInputService extends BaseTunerTvInputService {
    private static final String TAG = "UsbTunerTvInputService";
    private static final boolean DEBUG = false;


    private static final String MAX_CACHE_SIZE_KEY = "usbtuner.cachesize_mbytes";
    private static final int MAX_CACHE_SIZE_DEF = 2 * 1024;  // 2GB
    private static final int MIN_CACHE_SIZE_DEF = 256;  // 256MB

    @Override
    protected CacheManager createCacheManager() {
        int maxCacheSizeMb = SystemPropertiesProxy.getInt(MAX_CACHE_SIZE_KEY, MAX_CACHE_SIZE_DEF);
        if (maxCacheSizeMb >= MIN_CACHE_SIZE_DEF) {
            boolean useExternalStorage = Environment.MEDIA_MOUNTED.equals(
                    Environment.getExternalStorageState()) &&
                    Environment.isExternalStorageRemovable();
            if (DEBUG) Log.d(TAG, "useExternalStorage for trickplay: " + useExternalStorage);
            boolean allowToUseInternalStorage = true;
            if (useExternalStorage || allowToUseInternalStorage) {
                File baseDir = useExternalStorage ? getExternalCacheDir() : getCacheDir();
                return new CacheManager(
                        new TrickplayStorageManager(getApplicationContext(), baseDir,
                                1024L * 1024 * maxCacheSizeMb));
            }
        }
        return null;
    }

    public static String getInputId(Context context) {
        return TvContract.buildInputId(new ComponentName(context, UsbTunerTvInputService.class));
    }
}
