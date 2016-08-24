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

import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.media.tv.TvInputHardwareInfo;
import android.media.tv.TvInputInfo;
import android.os.Environment;
import android.util.Log;

import com.android.usbtuner.exoplayer.cache.CacheManager;
import com.android.usbtuner.exoplayer.cache.TrickplayStorageManager;
import com.android.usbtuner.util.SystemPropertiesProxy;
import com.android.usbtuner.util.TisConfiguration;
import org.xmlpull.v1.XmlPullParserException;

import java.io.File;
import java.io.IOException;

/**
 * {@link InternalTunerTvInputService} serves TV channels coming from a internal tuner device.
 */
public class InternalTunerTvInputService extends BaseTunerTvInputService {
    private static final String TAG = "InternalTunerTvInputService";
    private static final boolean DEBUG = false;


    private static final String MAX_CACHE_SIZE_KEY = "usbtuner.cachesize_mbytes";
    private static final int MAX_CACHE_SIZE_DEF = 2 * 1024;  // 2GB
    private static final int MIN_CACHE_SIZE_DEF = 256;  // 256MB

    private ResolveInfo mResolveInfo;
    private String mTvInputId;

    @Override
    public void onCreate() {
        super.onCreate();
        mResolveInfo = getPackageManager().resolveService(
                new Intent(SERVICE_INTERFACE).setClass(this, getClass()),
                PackageManager.GET_SERVICES | PackageManager.GET_META_DATA);
    }

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

    @Override
    public TvInputInfo onHardwareAdded(TvInputHardwareInfo hardwareInfo) {
        if (DEBUG) Log.d(TAG, "onHardwareAdded: " + hardwareInfo.toString());
        if (mTvInputId != null) {
            return null;
        }
        TvInputInfo info = null;
        if (hardwareInfo.getType() == TvInputHardwareInfo.TV_INPUT_TYPE_TUNER &&
                TisConfiguration.getTunerHwDeviceId(this) == hardwareInfo.getDeviceId()) {
            try {
                info = TvInputInfo.createTvInputInfo(this, mResolveInfo, hardwareInfo,
                        "Google Tuner", null);
                mTvInputId = info.getId();
            } catch (XmlPullParserException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return info;
    }

    @Override
    public String onHardwareRemoved(TvInputHardwareInfo hardwareInfo) {
        if (DEBUG) Log.d(TAG, "onHardwareRemoved: " + hardwareInfo.toString());
        return null;
    }
}
