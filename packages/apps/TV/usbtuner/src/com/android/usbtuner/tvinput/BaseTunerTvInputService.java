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

import android.media.tv.TvInputService;
import android.util.Log;
import com.google.android.exoplayer.audio.AudioCapabilities;
import com.google.android.exoplayer.audio.AudioCapabilitiesReceiver;
import com.android.usbtuner.exoplayer.cache.CacheManager;

import java.util.Collections;
import java.util.Set;
import java.util.WeakHashMap;

/**
 * {@link BaseTunerTvInputService} serves TV channels coming from a tuner device.
 */
public abstract class BaseTunerTvInputService extends TvInputService
        implements AudioCapabilitiesReceiver.Listener {
    private static final String TAG = "BaseTunerTvInputService";
    private static final boolean DEBUG = false;

    // WeakContainer for {@link TvInputSessionImpl}
    private final Set<TunerSession> mTunerSessions = Collections.newSetFromMap(
            new WeakHashMap<TunerSession, Boolean>());
    private ChannelDataManager mChannelDataManager;
    private AudioCapabilitiesReceiver mAudioCapabilitiesReceiver;
    private AudioCapabilities mAudioCapabilities;
    private CacheManager mCacheManager;

    @Override
    public void onCreate() {
        super.onCreate();
        if (DEBUG) Log.d(TAG, "onCreate");
        mChannelDataManager = new ChannelDataManager(getApplicationContext());
        mAudioCapabilitiesReceiver = new AudioCapabilitiesReceiver(getApplicationContext(), this);
        mAudioCapabilitiesReceiver.register();
        mCacheManager = createCacheManager();
        if (mCacheManager == null) {
            Log.i(TAG, "Trickplay is disabled");
        } else {
            Log.i(TAG, "Trickplay is enabled");
        }
    }

    /**
     * Creates {@CacheManager}. It returns null, if storage in not enough.
     */
    protected abstract CacheManager createCacheManager();

    @Override
    public void onDestroy() {
        if (DEBUG) Log.d(TAG, "onDestroy");
        super.onDestroy();
        mChannelDataManager.release();
        mAudioCapabilitiesReceiver.unregister();
        if (mCacheManager != null) {
            mCacheManager.close();
        }
    }

    @Override
    public RecordingSession onCreateRecordingSession(String inputId) {
        return new TunerRecordingSession(this, inputId, mChannelDataManager);
    }

    @Override
    public Session onCreateSession(String inputId) {
        if (DEBUG) Log.d(TAG, "onCreateSession");
        final TunerSession session = new TunerSession(
                this, mChannelDataManager, mCacheManager);
        mTunerSessions.add(session);
        session.setAudioCapabilities(mAudioCapabilities);
        session.setOverlayViewEnabled(true);
        return session;
    }

    @Override
    public void onAudioCapabilitiesChanged(AudioCapabilities audioCapabilities) {
        mAudioCapabilities = audioCapabilities;
        for (TunerSession session : mTunerSessions) {
            if (!session.isReleased()) {
                session.setAudioCapabilities(audioCapabilities);
            }
        }
    }
}
