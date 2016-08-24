/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.smokefast;

import android.media.MediaPlayer;
import android.os.Looper;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.LargeTest;
import android.util.Log;

import com.android.smokefast.app.MediaPlaybackTestApp;

/**
 * Basic tests for video playback
 */
public class MediaPlaybackTest extends ActivityInstrumentationTestCase2<MediaPlaybackTestApp> {

    private static final String TAG = "MediaPlaybackTest";
    private static final int LOOP_START_BUFFER_MS = 10000;
    private static final int PLAY_BUFFER_MS = 2000;
    private final Object mCompletionLock = new Object();
    private final Object mLooperLock = new Object();
    private boolean mPlaybackSucceeded = false;
    private boolean mPlaybackError = false;
    private Looper mLooper;
    private MediaPlayer mPlayer;

    public MediaPlaybackTest() {
        super(MediaPlaybackTestApp.class);
    }

    @Override
    public void setUp() throws Exception {
        // start activity
        getActivity();
    }

    @LargeTest
    public void testVideoPlayback() {
        // start the MediaPlayer on a Looper thread, so it does not deadlock itself
        new Thread() {
            @Override
            public void run() {
                Looper.prepare();
                mLooper = Looper.myLooper();
                mPlayer = MediaPlayer.create(getInstrumentation().getContext(), R.raw.bbb);
                mPlayer.setDisplay(getActivity().getSurfaceHolder());
                synchronized (mLooperLock) {
                    mLooperLock.notify();
                }
                Looper.loop();
            }
        }.start();
        // make sure the looper is really started before we proceed
        synchronized (mLooperLock) {
            try {
                mLooperLock.wait(LOOP_START_BUFFER_MS);
            } catch (InterruptedException e) {
                fail("Loop thread start was interrupted");
            }
        }
        mPlayer.setOnErrorListener(new MediaPlayer.OnErrorListener() {
            @Override
            public boolean onError(MediaPlayer mp, int what, int extra) {
                mPlaybackError = true;
                mp.reset();
                return true;
            }
        });
        mPlayer.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
            @Override
            public void onCompletion(MediaPlayer mp) {
                synchronized (mCompletionLock) {
                    Log.w(TAG, "Hit onCompletion!");
                    mPlaybackSucceeded = true;
                    mCompletionLock.notifyAll();
                }
            }
        });
        mPlayer.start();
        int duration = mPlayer.getDuration();
        int currentPosition = mPlayer.getCurrentPosition();
        synchronized (mCompletionLock) {
            try {
                mCompletionLock.wait(duration - currentPosition + PLAY_BUFFER_MS);
            } catch (InterruptedException e) {
                fail("Wait for playback was interrupted");
            }
        }
        mLooper.quit();
        mPlayer.release();
        assertFalse(mPlaybackError);
        assertTrue(mPlaybackSucceeded);
    }
}
