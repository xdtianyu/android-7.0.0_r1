/*
 * Copyright 2014, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server.telecom;

import android.content.Context;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.provider.Settings;

import com.android.internal.util.Preconditions;

// TODO: Needed for move to system service: import com.android.internal.R;

/**
 * Plays DTMF tones locally for the caller to hear. In order to reduce (1) the amount of times we
 * check the "play local tones" setting and (2) the length of time we keep the tone generator, this
 * class employs a concept of a call "session" that starts and stops when the foreground call
 * changes.
 */
public class DtmfLocalTonePlayer {
    /** Generator used to actually play the tone. */
    private ToneGenerator mToneGenerator;

    /** The current call associated with an existing dtmf session. */
    private Call mCall;

    /**
     * Message codes to be used for creating and deleting ToneGenerator object in the tonegenerator
     * thread.
     */
    private static final int EVENT_CREATE_OBJECT = 1;
    private static final int EVENT_DELETE_OBJECT = 2;

    /** Handler running on the tonegenerator thread. */
    private Handler mHandler;

    public DtmfLocalTonePlayer() { }

    public void onForegroundCallChanged(Call oldForegroundCall, Call newForegroundCall) {
        endDtmfSession(oldForegroundCall);
        startDtmfSession(newForegroundCall);
    }

    /**
     * Starts playing the dtmf tone specified by c.
     *
     * @param call The associated call.
     * @param c The digit to play.
     */
    void playTone(Call call, char c) {
        // Do nothing if it is not the right call.
        if (mCall != call) {
            return;
        }
        synchronized(this) {
            if (mToneGenerator == null) {
                Log.d(this, "playTone: mToneGenerator == null, %c.", c);
            } else {
                Log.d(this, "starting local tone: %c.", c);
                int tone = getMappedTone(c);
                if (tone != ToneGenerator.TONE_UNKNOWN) {
                    mToneGenerator.startTone(tone, -1 /* toneDuration */);
                }
            }
        }
    }

    /**
     * Stops any currently playing dtmf tone.
     *
     * @param call The associated call.
     */
    void stopTone(Call call) {
        // Do nothing if it's not the right call.
        if (mCall != call) {
            return;
        }
        synchronized(this) {
            if (mToneGenerator == null) {
                Log.d(this, "stopTone: mToneGenerator == null.");
            } else {
                Log.d(this, "stopping local tone.");
                mToneGenerator.stopTone();
            }
        }
    }

    /**
     * Runs initialization requires to play local tones during a call.
     *
     * @param call The call associated with this dtmf session.
     */
    private void startDtmfSession(Call call) {
        if (call == null) {
            return;
        }
        final Context context = call.getContext();
        final boolean areLocalTonesEnabled;
        if (context.getResources().getBoolean(R.bool.allow_local_dtmf_tones)) {
            areLocalTonesEnabled = Settings.System.getInt(
                    context.getContentResolver(), Settings.System.DTMF_TONE_WHEN_DIALING, 1) == 1;
        } else {
            areLocalTonesEnabled = false;
        }

        mCall = call;

        if (areLocalTonesEnabled) {
            Log.d(this, "Posting create.");
            postMessage(EVENT_CREATE_OBJECT);
        }
    }

    /**
     * Releases resources needed for playing local dtmf tones.
     *
     * @param call The call associated with the session to end.
     */
    private void endDtmfSession(Call call) {
        if (call != null && mCall == call) {
            // Do a stopTone() in case the sessions ends before we are told to stop the tone.
            stopTone(call);

            mCall = null;
            Log.d(this, "Posting delete.");
            postMessage(EVENT_DELETE_OBJECT);
        }
    }

    /**
     * Posts a message to the tonegenerator-thread handler. Creates the handler if the handler
     * has not been instantiated.
     *
     * @param messageCode The message to post.
     */
    private void postMessage(int messageCode) {
        synchronized(this) {
            if (mHandler == null) {
                mHandler = getNewHandler();
            }

            if (mHandler == null) {
                Log.d(this, "Message %d skipped because there is no handler.", messageCode);
            } else {
                mHandler.obtainMessage(messageCode, null).sendToTarget();
            }
        }
    }

    /**
     * Creates a new tonegenerator Handler running in its own thread.
     */
    private Handler getNewHandler() {
        Preconditions.checkState(mHandler == null);

        HandlerThread thread = new HandlerThread("tonegenerator-dtmf");
        thread.start();

        return new Handler(thread.getLooper()) {
            @Override
            public void handleMessage(Message msg) {
                switch(msg.what) {
                    case EVENT_CREATE_OBJECT:
                        synchronized(DtmfLocalTonePlayer.this) {
                            if (mToneGenerator == null) {
                                try {
                                    mToneGenerator = new ToneGenerator(
                                            AudioManager.STREAM_DTMF, 80);
                                } catch (RuntimeException e) {
                                    Log.e(this, e, "Error creating local tone generator.");
                                    mToneGenerator = null;
                                }
                            }
                        }
                        break;
                    case EVENT_DELETE_OBJECT:
                        synchronized(DtmfLocalTonePlayer.this) {
                            if (mToneGenerator != null) {
                                mToneGenerator.release();
                                mToneGenerator = null;
                            }
                            // Delete the handler after the tone generator object is deleted by
                            // the tonegenerator thread.
                            if (mHandler != null && !mHandler.hasMessages(EVENT_CREATE_OBJECT)) {
                                // Stop the handler only if there are no pending CREATE messages.
                                mHandler.removeMessages(EVENT_DELETE_OBJECT);
                                mHandler.getLooper().quitSafely();
                                mHandler = null;
                            }
                        }
                        break;
                }
            }
        };
    }

    private static int getMappedTone(char digit) {
        if (digit >= '0' && digit <= '9') {
            return ToneGenerator.TONE_DTMF_0 + digit - '0';
        } else if (digit == '#') {
            return ToneGenerator.TONE_DTMF_P;
        } else if (digit == '*') {
            return ToneGenerator.TONE_DTMF_S;
        }
        return ToneGenerator.TONE_UNKNOWN;
    }
}
