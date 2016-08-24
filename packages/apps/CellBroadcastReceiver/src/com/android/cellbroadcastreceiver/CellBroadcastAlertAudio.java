/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.cellbroadcastreceiver;

import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnErrorListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Vibrator;
import android.speech.tts.TextToSpeech;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.util.Log;

import java.util.HashMap;
import java.util.Locale;
import java.util.MissingResourceException;

import static com.android.cellbroadcastreceiver.CellBroadcastReceiver.DBG;

/**
 * Manages alert audio and vibration and text-to-speech. Runs as a service so that
 * it can continue to play if another activity overrides the CellBroadcastListActivity.
 */
public class CellBroadcastAlertAudio extends Service implements TextToSpeech.OnInitListener,
        TextToSpeech.OnUtteranceCompletedListener {
    private static final String TAG = "CellBroadcastAlertAudio";

    /** Action to start playing alert audio/vibration/speech. */
    static final String ACTION_START_ALERT_AUDIO = "ACTION_START_ALERT_AUDIO";

    /** Extra for alert audio duration (from settings). */
    public static final String ALERT_AUDIO_DURATION_EXTRA =
            "com.android.cellbroadcastreceiver.ALERT_AUDIO_DURATION";

    /** Extra for message body to speak (if speech enabled in settings). */
    public static final String ALERT_AUDIO_MESSAGE_BODY =
            "com.android.cellbroadcastreceiver.ALERT_AUDIO_MESSAGE_BODY";

    /** Extra for text-to-speech preferred language (if speech enabled in settings). */
    public static final String ALERT_AUDIO_MESSAGE_PREFERRED_LANGUAGE =
            "com.android.cellbroadcastreceiver.ALERT_AUDIO_MESSAGE_PREFERRED_LANGUAGE";

    /** Extra for text-to-speech default language when preferred language is
        not available (if speech enabled in settings). */
    public static final String ALERT_AUDIO_MESSAGE_DEFAULT_LANGUAGE =
            "com.android.cellbroadcastreceiver.ALERT_AUDIO_MESSAGE_DEFAULT_LANGUAGE";

    /** Extra for alert audio vibration enabled (from settings). */
    public static final String ALERT_AUDIO_VIBRATE_EXTRA =
            "com.android.cellbroadcastreceiver.ALERT_AUDIO_VIBRATE";

    /** Extra for alert audio ETWS behavior (always vibrate, even in silent mode). */
    public static final String ALERT_AUDIO_ETWS_VIBRATE_EXTRA =
            "com.android.cellbroadcastreceiver.ALERT_AUDIO_ETWS_VIBRATE";

    private static final String TTS_UTTERANCE_ID = "com.android.cellbroadcastreceiver.UTTERANCE_ID";

    /** Pause duration between alert sound and alert speech. */
    private static final int PAUSE_DURATION_BEFORE_SPEAKING_MSEC = 1000;

    /** Duration of a CMAS alert. */
    private static final int CMAS_DURATION_MSEC = 10500;

    /** Vibration uses the same on/off pattern as the CMAS alert tone */
    private static final long[] sVibratePattern = { 0, 2000, 500, 1000, 500, 1000, 500,
            2000, 500, 1000, 500, 1000};

    private static final int STATE_IDLE = 0;
    private static final int STATE_ALERTING = 1;
    private static final int STATE_PAUSING = 2;
    private static final int STATE_SPEAKING = 3;

    private int mState;

    private TextToSpeech mTts;
    private boolean mTtsEngineReady;

    private String mMessageBody;
    private String mMessagePreferredLanguage;
    private String mMessageDefaultLanguage;
    private boolean mTtsLanguageSupported;
    private boolean mEnableVibrate;
    private boolean mEnableAudio;

    private Vibrator mVibrator;
    private MediaPlayer mMediaPlayer;
    private AudioManager mAudioManager;
    private TelephonyManager mTelephonyManager;
    private int mInitialCallState;

    private PendingIntent mPlayReminderIntent;

    // Internal messages
    private static final int ALERT_SOUND_FINISHED = 1000;
    private static final int ALERT_PAUSE_FINISHED = 1001;
    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case ALERT_SOUND_FINISHED:
                    if (DBG) log("ALERT_SOUND_FINISHED");
                    stop();     // stop alert sound
                    // if we can speak the message text
                    if (mMessageBody != null && mTtsEngineReady && mTtsLanguageSupported) {
                        mHandler.sendMessageDelayed(mHandler.obtainMessage(ALERT_PAUSE_FINISHED),
                                PAUSE_DURATION_BEFORE_SPEAKING_MSEC);
                        mState = STATE_PAUSING;
                    } else {
                        if (DBG) log("MessageEmpty = " + (mMessageBody == null) +
                                ", mTtsEngineReady = " + mTtsEngineReady +
                                ", mTtsLanguageSupported = " + mTtsLanguageSupported);
                        stopSelf();
                        mState = STATE_IDLE;
                    }
                    break;

                case ALERT_PAUSE_FINISHED:
                    if (DBG) log("ALERT_PAUSE_FINISHED");
                    int res = TextToSpeech.ERROR;
                    if (mMessageBody != null && mTtsEngineReady && mTtsLanguageSupported) {
                        if (DBG) log("Speaking broadcast text: " + mMessageBody);

                        Bundle params = new Bundle();
                        // Play TTS in notification stream.
                        params.putInt(TextToSpeech.Engine.KEY_PARAM_STREAM,
                                AudioManager.STREAM_NOTIFICATION);
                        // Use the non-public parameter 2 --> TextToSpeech.QUEUE_DESTROY for TTS.
                        // The entire playback queue is purged. This is different from QUEUE_FLUSH
                        // in that all entries are purged, not just entries from a given caller.
                        // This is for emergency so we want to kill all other TTS sessions.
                        res = mTts.speak(mMessageBody, 2, params, TTS_UTTERANCE_ID);
                        mState = STATE_SPEAKING;
                    }
                    if (res != TextToSpeech.SUCCESS) {
                        loge("TTS engine not ready or language not supported or speak() failed");
                        stopSelf();
                        mState = STATE_IDLE;
                    }
                    break;

                default:
                    loge("Handler received unknown message, what=" + msg.what);
            }
        }
    };

    private final PhoneStateListener mPhoneStateListener = new PhoneStateListener() {
        @Override
        public void onCallStateChanged(int state, String ignored) {
            // Stop the alert sound and speech if the call state changes.
            if (state != TelephonyManager.CALL_STATE_IDLE
                    && state != mInitialCallState) {
                stopSelf();
            }
        }
    };

    /**
     * Callback from TTS engine after initialization.
     * @param status {@link TextToSpeech#SUCCESS} or {@link TextToSpeech#ERROR}.
     */
    @Override
    public void onInit(int status) {
        if (DBG) log("onInit() TTS engine status: " + status);
        if (status == TextToSpeech.SUCCESS) {
            mTtsEngineReady = true;
            mTts.setOnUtteranceCompletedListener(this);
            // try to set the TTS language to match the broadcast
            setTtsLanguage();
        } else {
            mTtsEngineReady = false;
            mTts = null;
            loge("onInit() TTS engine error: " + status);
        }
    }

    /**
     * Try to set the TTS engine language to the preferred language. If failed, set
     * it to the default language. mTtsLanguageSupported will be updated based on the response.
     */
    private void setTtsLanguage() {

        String language = mMessagePreferredLanguage;
        if (language == null || language.isEmpty() ||
                TextToSpeech.LANG_AVAILABLE != mTts.isLanguageAvailable(new Locale(language))) {
            language = mMessageDefaultLanguage;
            if (language == null || language.isEmpty() ||
                    TextToSpeech.LANG_AVAILABLE != mTts.isLanguageAvailable(new Locale(language))) {
                mTtsLanguageSupported = false;
                return;
            }
            if (DBG) log("Language '" + mMessagePreferredLanguage + "' is not available, using" +
                    "the default language '" + mMessageDefaultLanguage + "'");
        }

        if (DBG) log("Setting TTS language to '" + language + '\'');

        try {
            int result = mTts.setLanguage(new Locale(language));
            if (DBG) log("TTS setLanguage() returned: " + result);
            mTtsLanguageSupported = (result == TextToSpeech.LANG_AVAILABLE);
        }
        catch (MissingResourceException e) {
            mTtsLanguageSupported = false;
            loge("Language '" + language + "' is not available.");
        }
    }

    /**
     * Callback from TTS engine.
     * @param utteranceId the identifier of the utterance.
     */
    @Override
    public void onUtteranceCompleted(String utteranceId) {
        if (utteranceId.equals(TTS_UTTERANCE_ID)) {
            // When we reach here, it could be TTS completed or TTS was cut due to another
            // new alert started playing. We don't want to stop the service in the later case.
            if (mState == STATE_SPEAKING) {
                stopSelf();
            }
        }
    }

    @Override
    public void onCreate() {
        mVibrator = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        // Listen for incoming calls to kill the alarm.
        mTelephonyManager =
                (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
        mTelephonyManager.listen(
                mPhoneStateListener, PhoneStateListener.LISTEN_CALL_STATE);
    }

    @Override
    public void onDestroy() {
        // stop audio, vibration and TTS
        stop();
        // Stop listening for incoming calls.
        mTelephonyManager.listen(mPhoneStateListener, 0);
        // shutdown TTS engine
        if (mTts != null) {
            try {
                mTts.shutdown();
            } catch (IllegalStateException e) {
                // catch "Unable to retrieve AudioTrack pointer for stop()" exception
                loge("exception trying to shutdown text-to-speech");
            }
        }
        if (mEnableAudio) {
            // Release the audio focus so other audio (e.g. music) can resume.
            // Do not do this in stop() because stop() is also called when we stop the tone (before
            // TTS is playing). We only want to release the focus when tone and TTS are played.
            mAudioManager.abandonAudioFocus(null);
        }
        // release CPU wake lock acquired by CellBroadcastAlertService
        CellBroadcastAlertWakeLock.releaseCpuLock();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // No intent, tell the system not to restart us.
        if (intent == null) {
            stopSelf();
            return START_NOT_STICKY;
        }

        // This extra should always be provided by CellBroadcastAlertService,
        // but default to 10.5 seconds just to be safe (CMAS requirement).
        int duration = intent.getIntExtra(ALERT_AUDIO_DURATION_EXTRA, CMAS_DURATION_MSEC);
        if (DBG) log("Duration: " + duration);

        // Get text to speak (if enabled by user)
        mMessageBody = intent.getStringExtra(ALERT_AUDIO_MESSAGE_BODY);
        mMessagePreferredLanguage = intent.getStringExtra(ALERT_AUDIO_MESSAGE_PREFERRED_LANGUAGE);
        mMessageDefaultLanguage = intent.getStringExtra(ALERT_AUDIO_MESSAGE_DEFAULT_LANGUAGE);

        mEnableVibrate = intent.getBooleanExtra(ALERT_AUDIO_VIBRATE_EXTRA, true);
        if (intent.getBooleanExtra(ALERT_AUDIO_ETWS_VIBRATE_EXTRA, false)) {
            mEnableVibrate = true;  // force enable vibration for ETWS alerts
        }

        switch (mAudioManager.getRingerMode()) {
            case AudioManager.RINGER_MODE_SILENT:
                if (DBG) log("Ringer mode: silent");
                mEnableAudio = false;
                mEnableVibrate = false;
                break;

            case AudioManager.RINGER_MODE_VIBRATE:
                if (DBG) log("Ringer mode: vibrate");
                mEnableAudio = false;
                break;

            case AudioManager.RINGER_MODE_NORMAL:
            default:
                if (DBG) log("Ringer mode: normal");
                mEnableAudio = true;
                break;
        }

        if (mMessageBody != null && mEnableAudio) {
            if (mTts == null) {
                mTts = new TextToSpeech(this, this);
            } else if (mTtsEngineReady) {
                setTtsLanguage();
            }
        }

        if (mEnableAudio || mEnableVibrate) {
            play(duration);     // in milliseconds
        } else {
            stopSelf();
            return START_NOT_STICKY;
        }

        // Record the initial call state here so that the new alarm has the
        // newest state.
        mInitialCallState = mTelephonyManager.getCallState();

        return START_STICKY;
    }

    // Volume suggested by media team for in-call alarms.
    private static final float IN_CALL_VOLUME = 0.125f;

    /**
     * Start playing the alert sound, and send delayed message when it's time to stop.
     * @param duration the alert sound duration in milliseconds
     */
    private void play(int duration) {
        // stop() checks to see if we are already playing.
        stop();

        if (DBG) log("play()");

        // Start the vibration first.
        if (mEnableVibrate) {
            mVibrator.vibrate(sVibratePattern, -1);
        }

        if (mEnableAudio) {
            // future optimization: reuse media player object
            mMediaPlayer = new MediaPlayer();
            mMediaPlayer.setOnErrorListener(new OnErrorListener() {
                public boolean onError(MediaPlayer mp, int what, int extra) {
                    loge("Error occurred while playing audio.");
                    mp.stop();
                    mp.release();
                    mMediaPlayer = null;
                    return true;
                }
            });

            mMediaPlayer.setOnCompletionListener(new OnCompletionListener() {
                public void onCompletion(MediaPlayer mp) {
                    if (DBG) log("Audio playback complete.");
                    mHandler.sendMessage(mHandler.obtainMessage(ALERT_SOUND_FINISHED));
                    return;
                }
            });

            try {
                // Check if we are in a call. If we are, play the alert
                // sound at a low volume to not disrupt the call.
                if (mTelephonyManager.getCallState()
                        != TelephonyManager.CALL_STATE_IDLE) {
                    log("in call: reducing volume");
                    mMediaPlayer.setVolume(IN_CALL_VOLUME, IN_CALL_VOLUME);
                }

                // start playing alert audio (unless master volume is vibrate only or silent).
                setDataSourceFromResource(getResources(), mMediaPlayer,
                        R.raw.attention_signal);
                mAudioManager.requestAudioFocus(null, AudioManager.STREAM_NOTIFICATION,
                        AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
                // if the duration isn't equal to one play of the full 10.5s file then play
                // with looping enabled.
                startAlarm(mMediaPlayer, duration != CMAS_DURATION_MSEC);
            } catch (Exception ex) {
                loge("Failed to play alert sound: " + ex);
            }
        }

        // stop alert after the specified duration, unless we are playing the full 10.5s file once
        // in which case we'll use the end of playback callback rather than a delayed message.
        // This is to avoid the CMAS alert potentially being truncated due to audio playback lag.
        if (duration != CMAS_DURATION_MSEC) {
            mHandler.sendMessageDelayed(mHandler.obtainMessage(ALERT_SOUND_FINISHED), duration);
        }
        mState = STATE_ALERTING;
    }

    // Do the common stuff when starting the alarm.
    private static void startAlarm(MediaPlayer player, boolean looping)
            throws java.io.IOException, IllegalArgumentException, IllegalStateException {
        player.setAudioStreamType(AudioManager.STREAM_NOTIFICATION);
        player.setLooping(looping);
        player.prepare();
        player.start();
    }

    private static void setDataSourceFromResource(Resources resources,
            MediaPlayer player, int res) throws java.io.IOException {
        AssetFileDescriptor afd = resources.openRawResourceFd(res);
        if (afd != null) {
            player.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(),
                    afd.getLength());
            afd.close();
        }
    }

    private void playAlertReminderSound() {
        Uri notificationUri = RingtoneManager.getDefaultUri(
                RingtoneManager.TYPE_NOTIFICATION | RingtoneManager.TYPE_ALARM);
        if (notificationUri == null) {
            loge("Can't get URI for alert reminder sound");
            return;
        }
        Ringtone r = RingtoneManager.getRingtone(this, notificationUri);
        if (r != null) {
            log("playing alert reminder sound");
            r.play();
        } else {
            loge("can't get Ringtone for alert reminder sound");
        }
    }

    /**
     * Stops alert audio and speech.
     */
    public void stop() {
        if (DBG) log("stop()");

        if (mPlayReminderIntent != null) {
            mPlayReminderIntent.cancel();
            mPlayReminderIntent = null;
        }

        mHandler.removeMessages(ALERT_SOUND_FINISHED);
        mHandler.removeMessages(ALERT_PAUSE_FINISHED);

        if (mState == STATE_ALERTING) {
            // Stop audio playing
            if (mMediaPlayer != null) {
                try {
                    mMediaPlayer.stop();
                    mMediaPlayer.release();
                } catch (IllegalStateException e) {
                    // catch "Unable to retrieve AudioTrack pointer for stop()" exception
                    loge("exception trying to stop media player");
                }
                mMediaPlayer = null;
            }

            // Stop vibrator
            mVibrator.cancel();
        } else if (mState == STATE_SPEAKING && mTts != null) {
            try {
                mTts.stop();
            } catch (IllegalStateException e) {
                // catch "Unable to retrieve AudioTrack pointer for stop()" exception
                loge("exception trying to stop text-to-speech");
            }
        }
        mState = STATE_IDLE;
    }

    private static void log(String msg) {
        Log.d(TAG, msg);
    }

    private static void loge(String msg) {
        Log.e(TAG, msg);
    }
}
