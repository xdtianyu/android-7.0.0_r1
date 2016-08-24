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

package com.android.services.telephony;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.os.SystemVibrator;
import android.os.Vibrator;
import android.provider.Settings;

/**
 * Plays an emergency tone when placing emergency calls on CDMA devices.
 */
class EmergencyTonePlayer {

    private static final int EMERGENCY_TONE_OFF = 0;
    private static final int EMERGENCY_TONE_ALERT = 1;
    private static final int EMERGENCY_TONE_VIBRATE = 2;

    private static final int ALERT_RELATIVE_VOLUME_PERCENT = 100;

    private static final int VIBRATE_LENGTH_MILLIS = 1000;
    private static final int VIBRATE_PAUSE_MILLIS = 1000;
    private static final long[] VIBRATE_PATTERN =
            new long[] { VIBRATE_LENGTH_MILLIS, VIBRATE_PAUSE_MILLIS};

    private static final AudioAttributes VIBRATION_ATTRIBUTES =
            new AudioAttributes.Builder()
                .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
                .setUsage(AudioAttributes.USAGE_VOICE_COMMUNICATION)
                .build();

    // We don't rely on getSystemService(Context.VIBRATOR_SERVICE) to make sure that this vibrator
    // object will be isolated from others.
    private final Vibrator mVibrator = new SystemVibrator();
    private final Context mContext;
    private final AudioManager mAudioManager;

    private ToneGenerator mToneGenerator;
    private int mSavedInCallVolume;
    private boolean mIsVibrating = false;

    EmergencyTonePlayer(Context context) {
        mContext = context;
        mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
    }

    public void start() {
        switch (getToneSetting()) {
            case EMERGENCY_TONE_VIBRATE:
                startVibrate();
                break;
            case EMERGENCY_TONE_ALERT:
                // Only start if we are not in silent mode.
                int ringerMode = mAudioManager.getRingerMode();
                if (ringerMode == AudioManager.RINGER_MODE_NORMAL) {
                    startAlert();
                }
                break;
            case EMERGENCY_TONE_OFF:
                // nothing;
                break;
        }
    }

    public void stop() {
        stopVibrate();
        stopAlert();
    }

    private void startVibrate() {
        if (!mIsVibrating) {
            mVibrator.vibrate(VIBRATE_PATTERN, 0, VIBRATION_ATTRIBUTES);
            mIsVibrating = true;
        }
    }

    private void stopVibrate() {
        if (mIsVibrating) {
            mVibrator.cancel();
            mIsVibrating = false;
        }
    }

    private void startAlert() {
        if (mToneGenerator == null) {
            mToneGenerator = new ToneGenerator(
                    AudioManager.STREAM_VOICE_CALL, ALERT_RELATIVE_VOLUME_PERCENT);

            // Set the volume to max and save the old volume setting.
            mSavedInCallVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_VOICE_CALL);
            mAudioManager.setStreamVolume(
                    AudioManager.STREAM_VOICE_CALL,
                    mAudioManager.getStreamMaxVolume(AudioManager.STREAM_VOICE_CALL),
                    0);
            mToneGenerator.startTone(ToneGenerator.TONE_CDMA_EMERGENCY_RINGBACK);
        } else {
            Log.d(this, "An alert is already running.");
        }
    }

    private void stopAlert() {
        if (mToneGenerator != null) {
            mToneGenerator.stopTone();
            mToneGenerator.release();
            mToneGenerator = null;

            mAudioManager.setStreamVolume(AudioManager.STREAM_VOICE_CALL, mSavedInCallVolume, 0);
            mSavedInCallVolume = 0;
        }
    }

    private int getToneSetting() {
       return Settings.Global.getInt(
               mContext.getContentResolver(), Settings.Global.EMERGENCY_TONE, EMERGENCY_TONE_OFF);
    }
}
