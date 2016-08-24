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
 * limitations under the License
 */

package com.android.tv.dvr;

import android.annotation.TargetApi;
import android.content.Context;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvRecordingClient;
import android.os.Build;
import android.os.Handler;
import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;
import android.support.v4.util.ArrayMap;
import android.util.Log;

import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.data.Channel;

/**
 * Manages Dvr Sessions.
 * Responsible for:
 * <ul>
 *     <li>Manage DvrSession</li>
 *     <li>Manage capabilities (conflict)</li>
 * </ul>
 */
@TargetApi(Build.VERSION_CODES.N)
public class DvrSessionManager extends TvInputManager.TvInputCallback {
    //consider moving all of this to TvInputManagerHelper
    private final static String TAG = "DvrSessionManager";
    private static final boolean DEBUG = false;

    private final Context mContext;
    private final TvInputManager mTvInputManager;
    private final ArrayMap<String, TvInputInfo> mRecordingTvInputs = new ArrayMap<>();

    public DvrSessionManager(Context context) {
        this(context, (TvInputManager) context.getSystemService(Context.TV_INPUT_SERVICE),
                new Handler());
    }

    @VisibleForTesting
    DvrSessionManager(Context context, TvInputManager tvInputManager, Handler handler) {
        SoftPreconditions.checkFeatureEnabled(context, CommonFeatures.DVR, TAG);
        mTvInputManager = tvInputManager;
        mContext = context.getApplicationContext();
        for (TvInputInfo info : tvInputManager.getTvInputList()) {
            if (DEBUG) {
                Log.d(TAG, info + " canRecord=" + info.canRecord() + " tunerCount=" + info
                        .getTunerCount());
            }
            if (info.canRecord()) {
                mRecordingTvInputs.put(info.getId(), info);
            }
        }
        tvInputManager.registerCallback(this, handler);

    }

    public TvRecordingClient createTvRecordingClient(String tag,
            TvRecordingClient.RecordingCallback callback, Handler handler) {
        return new TvRecordingClient(mContext, tag, callback, handler);
    }

    public boolean canAcquireDvrSession(String inputId, Channel channel) {
        // TODO(DVR): implement checking tuner count etc.
        TvInputInfo info = mRecordingTvInputs.get(inputId);
        return info != null;
    }

    public void releaseTvRecordingClient(TvRecordingClient recordingClient) {
        recordingClient.release();
    }

    @Override
    public void onInputAdded(String inputId) {
        super.onInputAdded(inputId);
        TvInputInfo info = mTvInputManager.getTvInputInfo(inputId);
        if (DEBUG) {
            Log.d(TAG, "onInputAdded " + info.toString() + " canRecord=" + info.canRecord()
                    + " tunerCount=" + info.getTunerCount());
        }
        if (info.canRecord()) {
            mRecordingTvInputs.put(inputId, info);
        }
    }

    @Override
    public void onInputRemoved(String inputId) {
        super.onInputRemoved(inputId);
        if (DEBUG) Log.d(TAG, "onInputRemoved " + inputId);
        mRecordingTvInputs.remove(inputId);
    }

    @Override
    public void onInputUpdated(String inputId) {
        super.onInputUpdated(inputId);
        TvInputInfo info = mTvInputManager.getTvInputInfo(inputId);
        if (DEBUG) {
            Log.d(TAG, "onInputUpdated " + info.toString() + " canRecord=" + info.canRecord()
                    + " tunerCount=" + info.getTunerCount());
        }
        if (info.canRecord()) {
            mRecordingTvInputs.put(inputId, info);
        } else {
            mRecordingTvInputs.remove(inputId);
        }
    }

    @Nullable
    public TvInputInfo getTvInputInfo(String inputId) {
        return mRecordingTvInputs.get(inputId);
    }
}
