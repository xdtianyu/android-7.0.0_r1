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

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputManager.TvInputCallback;
import android.os.Handler;
import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import android.util.Log;

import com.android.tv.common.SoftPreconditions;
import com.android.tv.parental.ContentRatingsManager;
import com.android.tv.parental.ParentalControlSettings;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class TvInputManagerHelper {
    private static final String TAG = "TvInputManagerHelper";
    private static final boolean DEBUG = false;

    // Hardcoded list for known bundled inputs not written by OEM/SOCs.
    // Bundled (system) inputs not in the list will get the high priority
    // so they and their channels come first in the UI.
    private static final Set<String> BUNDLED_PACKAGE_SET = new HashSet<>();

    static {
        BUNDLED_PACKAGE_SET.add("com.android.tv");
        BUNDLED_PACKAGE_SET.add("com.android.usbtuner");
    }

    private final Context mContext;
    private final TvInputManager mTvInputManager;
    private final Map<String, Integer> mInputStateMap = new HashMap<>();
    private final Map<String, TvInputInfo> mInputMap = new HashMap<>();
    private final Map<String, Boolean> mInputIdToPartnerInputMap = new HashMap<>();
    private final TvInputCallback mInternalCallback = new TvInputCallback() {
        @Override
        public void onInputStateChanged(String inputId, int state) {
            if (DEBUG) Log.d(TAG, "onInputStateChanged " + inputId + " state=" + state);
            mInputStateMap.put(inputId, state);
            for (TvInputCallback callback : mCallbacks) {
                callback.onInputStateChanged(inputId, state);
            }
        }

        @Override
        public void onInputAdded(String inputId) {
            if (DEBUG) Log.d(TAG, "onInputAdded " + inputId);
            TvInputInfo info = mTvInputManager.getTvInputInfo(inputId);
            if (info != null) {
                mInputMap.put(inputId, info);
                mInputStateMap.put(inputId, mTvInputManager.getInputState(inputId));
                mInputIdToPartnerInputMap.put(inputId, isPartnerInput(info));
            }
            mContentRatingsManager.update();
            for (TvInputCallback callback : mCallbacks) {
                callback.onInputAdded(inputId);
            }
        }

        @Override
        public void onInputRemoved(String inputId) {
            if (DEBUG) Log.d(TAG, "onInputRemoved " + inputId);
            mInputMap.remove(inputId);
            mInputStateMap.remove(inputId);
            mInputIdToPartnerInputMap.remove(inputId);
            mContentRatingsManager.update();
            for (TvInputCallback callback : mCallbacks) {
                callback.onInputRemoved(inputId);
            }
        }

        @Override
        public void onInputUpdated(String inputId) {
            if (DEBUG) Log.d(TAG, "onInputUpdated " + inputId);
            TvInputInfo info = mTvInputManager.getTvInputInfo(inputId);
            mInputMap.put(inputId, info);
            for (TvInputCallback callback : mCallbacks) {
                callback.onInputUpdated(inputId);
            }
        }
    };

    private final Handler mHandler = new Handler();
    private boolean mStarted;
    private final HashSet<TvInputCallback> mCallbacks = new HashSet<>();
    private final ContentRatingsManager mContentRatingsManager;
    private final ParentalControlSettings mParentalControlSettings;
    private final Comparator<TvInputInfo> mTvInputInfoComparator;

    public TvInputManagerHelper(Context context) {
        mContext = context.getApplicationContext();
        mTvInputManager = (TvInputManager) context.getSystemService(Context.TV_INPUT_SERVICE);
        mContentRatingsManager = new ContentRatingsManager(context);
        mParentalControlSettings = new ParentalControlSettings(context);
        mTvInputInfoComparator = new TvInputInfoComparator(this);
    }

    public void start() {
        if (mStarted) {
            return;
        }
        if (DEBUG) Log.d(TAG, "start");
        mStarted = true;
        mTvInputManager.registerCallback(mInternalCallback, mHandler);
        mInputMap.clear();
        mInputStateMap.clear();
        mInputIdToPartnerInputMap.clear();
        for (TvInputInfo input : mTvInputManager.getTvInputList()) {
            if (DEBUG) Log.d(TAG, "Input detected " + input);
            String inputId = input.getId();
            mInputMap.put(inputId, input);
            int state = mTvInputManager.getInputState(inputId);
            mInputStateMap.put(inputId, state);
            mInputIdToPartnerInputMap.put(inputId, isPartnerInput(input));
        }
        SoftPreconditions.checkState(mInputStateMap.size() == mInputMap.size(), TAG,
                "mInputStateMap not the same size as mInputMap");
        mContentRatingsManager.update();
    }

    public void stop() {
        if (!mStarted) {
            return;
        }
        mTvInputManager.unregisterCallback(mInternalCallback);
        mStarted = false;
        mInputStateMap.clear();
        mInputMap.clear();
        mInputIdToPartnerInputMap.clear();
    }

    public List<TvInputInfo> getTvInputInfos(boolean availableOnly, boolean tunerOnly) {
        ArrayList<TvInputInfo> list = new ArrayList<>();
        for (Map.Entry<String, Integer> pair : mInputStateMap.entrySet()) {
            if (availableOnly && pair.getValue() == TvInputManager.INPUT_STATE_DISCONNECTED) {
                continue;
            }
            TvInputInfo input = getTvInputInfo(pair.getKey());
            if (tunerOnly && input.getType() != TvInputInfo.TYPE_TUNER) {
                continue;
            }
            list.add(input);
        }
        Collections.sort(list, mTvInputInfoComparator);
        return list;
    }

    /**
     * Returns the default comparator for {@link TvInputInfo}.
     * See {@link TvInputInfoComparator} for detail.
     */
    public Comparator<TvInputInfo> getDefaultTvInputInfoComparator() {
        return mTvInputInfoComparator;
    }

    /**
     * Checks if the input is from a partner.
     *
     * It's visible for comparator test.
     * Package private is enough for this method, but public is necessary to workaround mockito
     * bug.
     */
    @VisibleForTesting
    public boolean isPartnerInput(TvInputInfo inputInfo) {
        return isSystemInput(inputInfo) && !isBundledInput(inputInfo);
    }

    /**
     * Does the input have {@link ApplicationInfo#FLAG_SYSTEM} set.
     */
    public boolean isSystemInput(TvInputInfo inputInfo) {
        return inputInfo != null
                && (inputInfo.getServiceInfo().applicationInfo.flags
                    & ApplicationInfo.FLAG_SYSTEM) != 0;
    }

    /**
     * Is the input one known bundled inputs not written by OEM/SOCs.
     */
    public boolean isBundledInput(TvInputInfo inputInfo) {
        return inputInfo != null 
               && BUNDLED_PACKAGE_SET.contains(
                   inputInfo.getServiceInfo().applicationInfo.packageName);
    }

    /**
     * Returns if the given input is bundled and written by OEM/SOCs.
     * This returns the cached result.
     */
    public boolean isPartnerInput(String inputId) {
        Boolean isPartnerInput = mInputIdToPartnerInputMap.get(inputId);
        return (isPartnerInput != null) ? isPartnerInput : false;
    }

    /**
     * Loads label of {@code info}.
     *
     * It's visible for comparator test to mock TvInputInfo.
     * Package private is enough for this method, but public is necessary to workaround mockito
     * bug.
     */
    @VisibleForTesting
    public String loadLabel(TvInputInfo info) {
        return info.loadLabel(mContext).toString();
    }

    /**
     * Returns if TV input exists with the input id.
     */
    public boolean hasTvInputInfo(String inputId) {
        SoftPreconditions.checkState(mStarted, TAG,
                "hasTvInputInfo() called before TvInputManagerHelper was started.");
        if (!mStarted) {
            return false;
        }
        return !TextUtils.isEmpty(inputId) && mInputMap.get(inputId) != null;
    }

    public TvInputInfo getTvInputInfo(String inputId) {
        SoftPreconditions.checkState(mStarted, TAG,
                "getTvInputInfo() called before TvInputManagerHelper was started.");
        if (!mStarted) {
            return null;
        }
        if (inputId == null) {
            return null;
        }
        return mInputMap.get(inputId);
    }

    public ApplicationInfo getTvInputAppInfo(String inputId) {
        TvInputInfo info = getTvInputInfo(inputId);
        return info == null ? null : info.getServiceInfo().applicationInfo;
    }

    public int getTunerTvInputSize() {
        int size = 0;
        for (TvInputInfo input : mInputMap.values()) {
            if (input.getType() == TvInputInfo.TYPE_TUNER) {
                ++size;
            }
        }
        return size;
    }

    public int getInputState(TvInputInfo inputInfo) {
        return getInputState(inputInfo.getId());
    }

    public int getInputState(String inputId) {
        SoftPreconditions.checkState(mStarted, TAG, "AvailabilityManager not started");
        if (!mStarted) {
            return TvInputManager.INPUT_STATE_DISCONNECTED;

        }
        Integer state = mInputStateMap.get(inputId);
        if (state == null) {
            Log.w(TAG, "getInputState: no such input (id=" + inputId + ")");
            return TvInputManager.INPUT_STATE_DISCONNECTED;
        }
        return state;
    }

    public void addCallback(TvInputCallback callback) {
        mCallbacks.add(callback);
    }

    public void removeCallback(TvInputCallback callback) {
        mCallbacks.remove(callback);
    }

    public ParentalControlSettings getParentalControlSettings() {
        return mParentalControlSettings;
    }

    /**
     * Returns a ContentRatingsManager instance for a given application context.
     */
    public ContentRatingsManager getContentRatingsManager() {
        return mContentRatingsManager;
    }

    /**
     * Default comparator for TvInputInfo.
     *
     * It's static class that accepts {@link TvInputManagerHelper} as parameter to test.
     * To test comparator, we need to mock API in parent class such as {@link #isPartnerInput},
     * but it's impossible for an inner class to use mocked methods.
     * (i.e. Mockito's spy doesn't work)
     */
    @VisibleForTesting
    static class TvInputInfoComparator implements Comparator<TvInputInfo> {
        private final TvInputManagerHelper mInputManager;

        public TvInputInfoComparator(TvInputManagerHelper inputManager) {
            mInputManager = inputManager;
        }

        @Override
        public int compare(TvInputInfo lhs, TvInputInfo rhs) {
            if (mInputManager.isPartnerInput(lhs) != mInputManager.isPartnerInput(rhs)) {
                return mInputManager.isPartnerInput(lhs) ? -1 : 1;
            }
            return mInputManager.loadLabel(lhs).compareTo(mInputManager.loadLabel(rhs));
        }
    }
}
