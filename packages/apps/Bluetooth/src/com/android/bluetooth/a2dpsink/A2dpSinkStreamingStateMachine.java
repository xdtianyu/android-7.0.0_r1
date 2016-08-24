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

package com.android.bluetooth.a2dpsink;

import android.bluetooth.BluetoothA2dpSink;
import android.bluetooth.BluetoothAvrcpController;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.media.AudioManager;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.os.Message;
import android.util.Log;

import com.android.bluetooth.avrcp.AvrcpControllerService;
import com.android.bluetooth.R;
import com.android.internal.util.IState;
import com.android.internal.util.State;
import com.android.internal.util.StateMachine;

/**
 * Bluetooth A2DP SINK Streaming StateMachine.
 *
 * This state machine defines how the stack behaves once the A2DP connection is established and both
 * devices are ready for streaming. For simplification we assume that the connection can either
 * stream music immediately (i.e. data packets coming in or have potential to come in) or it cannot
 * stream (i.e. Idle and Open states are treated alike). See Fig 4-1 of GAVDP Spec 1.0.
 *
 * Legend:
 * SRC: Source (Remote)
 *    (SRC, F) - Remote is not streaming. F stands for false.
 *    (SRC, T) - Remote is intent to streaming. T stands for true.
 * ACT: Action
 *    (ACT, F) - Local/Remote user intent is to pause/stop (AVRCP pause/stop).
 *    (ACT, T) - Local/Remote user intent is to play (AVRCP play).
 * The way to detect action is two fold:
 *  -- We can detect action on the SNK side by directly monitoring the AVRCP controller service.
 *  -- On the SRC side, any AVRCP action will be accompanied by an update via AVRCP and hence we can
 *  update our action state.
 *
 * A state will be a combination of SRC and ACT state. Hence a state such as:
 * (F, T) will mean that user has shown intent to play on local or remote device (second T) but the
 * connection is not in streaming state yet.
 *
 *  -----------------------------------------------------------------------------------------------
 *  Start State   |    End State | Transition(s)
 *  -----------------------------------------------------------------------------------------------
 *  (F, F)            (F, T)       ACT Play (No streaming in either states)
 *  (F, F)            (T, F)       Remote streams (play depends on policy)
 *  (T, F)            (F, F)       Remote stops streaming.
 *  (T, F)            (T, T)       ACT Play (streaming already existed).
 *  (F, T)            (F, F)       ACT Pause.
 *  (F, T)            (T, T)       Remote starts streaming (ACT Play already existed)
 *  (T, T)            (F, T)       Remote stops streaming.
 *  (T, T)            (F, F)       ACT stop.
 *  (T, T)            (T, F)       ACT pause.
 *
 *  -----------------------------------------------------------------------------------------------
 *  State   | Action(s)
 *  -----------------------------------------------------------------------------------------------
 *  (F, F)    1. Lose audio focus (if it exists) and notify fluoride of audio focus loss.
 *            2. Stop AVRCP from pushing updates to UI.
 *  (T, F)    1. If policy is opt-in then get focus and stream (get audio focus etc).
 *            2. Else throw away the data (lose audio focus etc).
 *  (F, T)    In this state the source does not stream although we have play intent.
 *            1. Show a spinny that data will come through.
 *  (T, T)    1. Request Audio Focus and on success update AVRCP to show UI updates.
 *            2. On Audio focus enable streaming in Fluoride.
 */
final class A2dpSinkStreamingStateMachine extends StateMachine {
    private static final boolean DBG = true;
    private static final String TAG = "A2dpSinkStreamingStateMachine";
    private static final int ACT_PLAY_NUM_RETRIES = 5;
    private static final int ACT_PLAY_RETRY_DELAY = 2000; // millis.
    private static final int DEFAULT_DUCK_PERCENT = 25;

    // Streaming states (see the description above).
    private SRC_F_ACT_F mSrcFActF;
    private SRC_F_ACT_T mSrcFActT;
    private SRC_T_ACT_F mSrcTActF;
    private SRC_T_ACT_T mSrcTActT;

    // Transitions.
    public static final int SRC_STR_START = 0;
    public static final int SRC_STR_STOP = 1;
    public static final int SRC_STR_STOP_JITTER_WAIT_OVER = 2;
    public static final int ACT_PLAY = 3;
    public static final int ACT_PLAY_RETRY = 4;
    public static final int ACT_PAUSE = 5;
    public static final int AUDIO_FOCUS_CHANGE = 6;
    public static final int DISCONNECT = 7;

    // Private variables.
    private A2dpSinkStateMachine mA2dpSinkSm;
    private Context mContext;
    private AudioManager mAudioManager;
    // Set default focus to loss since we have never requested it before.
    private int mCurrentAudioFocus = AudioManager.AUDIOFOCUS_LOSS;

     /* Used to indicate focus lost */
    private static final int STATE_FOCUS_LOST = 0;
    /* Used to inform bluedroid that focus is granted */
    private static final int STATE_FOCUS_GRANTED = 1;

    /* Wait in millis before the ACT loses focus on SRC jitter when streaming */
    private static final int SRC_STR_JITTER_WAIT = 5 * 1000; // 5sec

    /* Focus changes when we are currently holding focus (i.e. we're in SRC_T_ACT_T state). */
    private OnAudioFocusChangeListener mAudioFocusListener = new OnAudioFocusChangeListener() {
        public void onAudioFocusChange(int focusChange){
            if (DBG) {
                Log.d(TAG, "onAudioFocusChangeListener focuschange " + focusChange);
            }
            A2dpSinkStreamingStateMachine.this.sendMessage(AUDIO_FOCUS_CHANGE, focusChange);
        }
    };

    private A2dpSinkStreamingStateMachine(A2dpSinkStateMachine a2dpSinkSm, Context context) {
        super("A2dpSinkStreamingStateMachine");
        mA2dpSinkSm = a2dpSinkSm;
        mContext = context;
        mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);

        mSrcFActF = new SRC_F_ACT_F();
        mSrcFActT = new SRC_F_ACT_T();
        mSrcTActF = new SRC_T_ACT_F();
        mSrcTActT = new SRC_T_ACT_T();

        // States are independent of each other. We simply use transitionTo.
        addState(mSrcFActF);
        addState(mSrcFActT);
        addState(mSrcTActF);
        addState(mSrcTActT);
        setInitialState(mSrcFActF);

    }

    public static A2dpSinkStreamingStateMachine make(
            A2dpSinkStateMachine a2dpSinkSm, Context context) {
        if (DBG) {
            Log.d(TAG, "make");
        }
        A2dpSinkStreamingStateMachine a2dpStrStateMachine =
            new A2dpSinkStreamingStateMachine(a2dpSinkSm, context);
        a2dpStrStateMachine.start();
        return a2dpStrStateMachine;
    }

    /**
     * Utility functions that can be used by all states.
     */
    private boolean requestAudioFocus() {
        return (mAudioManager.requestAudioFocus(
            mAudioFocusListener, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN) ==
            AudioManager.AUDIOFOCUS_REQUEST_GRANTED);
    }

    private void startAvrcpUpdates() {
        // Since AVRCP gets started after A2DP we may need to request it later in cycle.
        AvrcpControllerService avrcpService = AvrcpControllerService.getAvrcpControllerService();

        if (DBG) {
            Log.d(TAG, "startAvrcpUpdates");
        }
        if (avrcpService != null && avrcpService.getConnectedDevices().size() == 1) {
            avrcpService.startAvrcpUpdates();
        } else {
            Log.e(TAG, "startAvrcpUpdates failed because of connection.");
        }
    }

    private void stopAvrcpUpdates() {
        // Since AVRCP gets started after A2DP we may need to request it later in cycle.
        AvrcpControllerService avrcpService = AvrcpControllerService.getAvrcpControllerService();

        if (DBG) {
            Log.d(TAG, "stopAvrcpUpdates");
        }
        if (avrcpService != null && avrcpService.getConnectedDevices().size() == 1) {
            avrcpService.stopAvrcpUpdates();
        } else {
            Log.e(TAG, "stopAvrcpUpdates failed because of connection.");
        }
    }

    private void sendAvrcpPause() {
        // Since AVRCP gets started after A2DP we may need to request it later in cycle.
        AvrcpControllerService avrcpService = AvrcpControllerService.getAvrcpControllerService();

        if (DBG) {
            Log.d(TAG, "sendAvrcpPause");
        }
        if (avrcpService != null && avrcpService.getConnectedDevices().size() == 1) {
            if (DBG) {
                Log.d(TAG, "Pausing AVRCP.");
            }
            avrcpService.sendPassThroughCmd(
                avrcpService.getConnectedDevices().get(0),
                BluetoothAvrcpController.PASS_THRU_CMD_ID_PAUSE,
                BluetoothAvrcpController.KEY_STATE_PRESSED);
            avrcpService.sendPassThroughCmd(
                avrcpService.getConnectedDevices().get(0),
                BluetoothAvrcpController.PASS_THRU_CMD_ID_PAUSE,
                BluetoothAvrcpController.KEY_STATE_RELEASED);
        } else {
            Log.e(TAG, "Passthrough not sent, connection un-available.");
        }
    }

    private void sendAvrcpPlay() {
        // Since AVRCP gets started after A2DP we may need to request it later in cycle.
        AvrcpControllerService avrcpService = AvrcpControllerService.getAvrcpControllerService();

        if (DBG) {
            Log.d(TAG, "sendAvrcpPlay");
        }
        if (avrcpService != null && avrcpService.getConnectedDevices().size() == 1) {
            if (DBG) {
                Log.d(TAG, "Playing AVRCP.");
            }
            avrcpService.sendPassThroughCmd(
                avrcpService.getConnectedDevices().get(0),
                BluetoothAvrcpController.PASS_THRU_CMD_ID_PLAY,
                BluetoothAvrcpController.KEY_STATE_PRESSED);
            avrcpService.sendPassThroughCmd(
                avrcpService.getConnectedDevices().get(0),
                BluetoothAvrcpController.PASS_THRU_CMD_ID_PLAY,
                BluetoothAvrcpController.KEY_STATE_RELEASED);
        } else {
            Log.e(TAG, "Passthrough not sent, connection un-available.");
        }
    }

    private void startFluorideStreaming() {
        mA2dpSinkSm.informAudioFocusStateNative(STATE_FOCUS_GRANTED);
        mA2dpSinkSm.informAudioTrackGainNative(1.0f);
    }

    private void stopFluorideStreaming() {
        mA2dpSinkSm.informAudioFocusStateNative(STATE_FOCUS_LOST);
    }

    private void setFluorideAudioTrackGain(float gain) {
        mA2dpSinkSm.informAudioTrackGainNative(gain);
    }

    private class SRC_F_ACT_F extends State {
        private static final String STATE_TAG = A2dpSinkStreamingStateMachine.TAG + ".SRC_F_ACT_F";
        @Override
        public void enter() {
            if (DBG) {
                Log.d(STATE_TAG, "Enter: " + getCurrentMessage().what);
            }
        }

        @Override
        public boolean processMessage(Message message) {
            if (DBG) {
                Log.d(STATE_TAG, " process message: " + message.what);
            }
            switch (message.what) {
                case SRC_STR_START:
                    // Opt out of all sounds without AVRCP play. We simply throw away.
                    transitionTo(mSrcTActF);
                    break;

                case ACT_PLAY:
                    // Wait in next state for actual playback. We defer the message so that the next
                    // state (SRC_F_ACT_T) can execute the retry logic.
                    deferMessage(message);
                    transitionTo(mSrcFActT);
                    break;

                case DISCONNECT:
                    mAudioManager.abandonAudioFocus(mAudioFocusListener);
                    mCurrentAudioFocus = AudioManager.AUDIOFOCUS_LOSS;
                    break;

                case AUDIO_FOCUS_CHANGE:
                    // If we are regaining focus after transient loss this indicates that we should
                    // press play again.
                    int newAudioFocus = message.arg1;
                    if (DBG) {
                        Log.d(STATE_TAG,
                            "prev focus " + mCurrentAudioFocus + " new focus " + newAudioFocus);
                    }
                    if (mCurrentAudioFocus == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT &&
                        newAudioFocus == AudioManager.AUDIOFOCUS_GAIN) {
                        sendAvrcpPlay();
                        // We should transition to SRC_F_ACT_T after this message. We also send some
                        // retries here because after phone calls we may have race conditions.
                        sendMessageDelayed(
                            ACT_PLAY_RETRY, ACT_PLAY_NUM_RETRIES, ACT_PLAY_RETRY_DELAY);
                    }
                    mCurrentAudioFocus = newAudioFocus;
                    break;

                default:
                    Log.e(TAG, "Don't know how to handle " + message.what);
            }
            return HANDLED;
        }
    }

    private class SRC_F_ACT_T extends State {
        private static final String STATE_TAG = A2dpSinkStreamingStateMachine.TAG + ".SRC_F_ACT_T";
        private boolean mPlay = false;

        @Override
        public void enter() {
            if (DBG) {
                Log.d(STATE_TAG, "Enter: " + getCurrentMessage().what);
            }
        }

        @Override
        public boolean processMessage(Message message) {
            if (DBG) {
                Log.d(STATE_TAG, " process message: " + message.what);
            }
            switch (message.what) {
                case SRC_STR_START:
                    deferMessage(message);
                    transitionTo(mSrcTActT);
                    break;

                case ACT_PAUSE:
                    transitionTo(mSrcFActF);
                    break;

                case ACT_PLAY:
                    // Retry if the remote has not yet started playing music. This is seen in some
                    // devices where after the phone call it requires multiple play commands to
                    // start music.
                    break;

                case ACT_PLAY_RETRY:
                    if (message.arg1 > 0) {
                        Log.d(STATE_TAG, "Retry " + message.arg1);
                        sendAvrcpPlay();
                        sendMessageDelayed(ACT_PLAY_RETRY, message.arg1 - 1, ACT_PLAY_RETRY_DELAY);
                    }
                    break;


                case DISCONNECT:
                    deferMessage(message);
                    transitionTo(mSrcFActF);
                    mPlay = false;
                    break;

                case AUDIO_FOCUS_CHANGE:
                    int newAudioFocus = message.arg1;
                    if (DBG) {
                        Log.d(STATE_TAG,
                              "prev focus " + mCurrentAudioFocus + " new focus " + newAudioFocus);
                    }
                    if (newAudioFocus == AudioManager.AUDIOFOCUS_GAIN) {
                        sendAvrcpPlay();
                        mCurrentAudioFocus = newAudioFocus;
                    } else if (newAudioFocus == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT) {
                        sendAvrcpPause();
                        mCurrentAudioFocus = newAudioFocus;
                    } else if (newAudioFocus == AudioManager.AUDIOFOCUS_LOSS) {
                        mAudioManager.abandonAudioFocus(mAudioFocusListener);
                        mCurrentAudioFocus = AudioManager.AUDIOFOCUS_LOSS;
                    }
                    break;

                default:
                    Log.e(TAG, "Don't know how to handle " + message.what);
            }
            return HANDLED;
        }
    }

    private class SRC_T_ACT_F extends State {
        private static final String STATE_TAG = A2dpSinkStreamingStateMachine.TAG + ".SRC_T_ACT_F";
        @Override
        public void enter() {
            if (DBG) {
                Log.d(STATE_TAG, "Enter: " + getCurrentMessage().what);
            }
        }

        @Override
        public boolean processMessage(Message message) {
            if (DBG) {
                Log.d(STATE_TAG, " process message: " + message.what);
            }
            switch (message.what) {
                case SRC_STR_STOP:
                    transitionTo(mSrcFActF);
                    break;

                case ACT_PLAY:
                    deferMessage(message);
                    transitionTo(mSrcTActT);
                    break;

                case DISCONNECT:
                    deferMessage(message);
                    transitionTo(mSrcFActF);
                    break;

                case AUDIO_FOCUS_CHANGE:
                    // If we regain focus from TRANSIENT that means that the remote was playing all
                    // this while although we must have sent a PAUSE (see focus loss in SRC_T_ACT_T
                    // state). In any case, we should resume music here if that is the case.
                    int newAudioFocus = message.arg1;
                    if (DBG) {
                        Log.d(STATE_TAG,
                              "prev focus " + mCurrentAudioFocus + " new focus " + newAudioFocus);
                    }
                    if (newAudioFocus == AudioManager.AUDIOFOCUS_LOSS ||
                        newAudioFocus == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT) {
                        sendAvrcpPause();
                    } else if (newAudioFocus == AudioManager.AUDIOFOCUS_GAIN) {
                        sendAvrcpPlay();
                    }
                    mCurrentAudioFocus = newAudioFocus;
                    break;

                default:
                    Log.e(TAG, "Don't know how to handle " + message.what);
            }
            return HANDLED;
        }
    }

    private class SRC_T_ACT_T extends State {
        private static final String STATE_TAG = A2dpSinkStreamingStateMachine.TAG + ".SRC_T_ACT_T";
        private boolean mWaitForJitter = false;
        @Override
        public void enter() {
            if (DBG) {
                Log.d(STATE_TAG, "Enter: " + getCurrentMessage().what);
            }
        }

        @Override
        public boolean processMessage(Message message) {
            if (DBG) {
                Log.d(STATE_TAG, " process message: " + message.what);
            }
            switch (message.what) {
                case ACT_PAUSE:
                    // Stop avrcp updates.
                    stopAvrcpUpdates();
                    stopFluorideStreaming();
                    transitionTo(mSrcTActF);
                    if (mCurrentAudioFocus == AudioManager.AUDIOFOCUS_GAIN) {
                        // If we have focus gain and we still get pause that means that we must have
                        // gotten a PAUSE by user explicitly pressing PAUSE on Car or Phone. Hence
                        // we release focus.
                        mAudioManager.abandonAudioFocus(mAudioFocusListener);
                        mCurrentAudioFocus = AudioManager.AUDIOFOCUS_LOSS;
                    }
                    break;

                case SRC_STR_STOP:
                    stopAvrcpUpdates();
                    stopFluorideStreaming();
                    transitionTo(mSrcFActT);
                    // This could be variety of reasons including that the remote is going to
                    // (eventually send us pause) or the device is going to go into a call state
                    // etc. Also it may simply be stutter of music. Instead of sending pause
                    // prematurely we wait for either a Pause from remote or AudioFocus change owing
                    // an ongoing call.
                    break;

                case SRC_STR_START:
                case ACT_PLAY:
                    Log.d(STATE_TAG, "Current Audio Focus " + mCurrentAudioFocus);
                    boolean startStream = true;
                    if (mCurrentAudioFocus == AudioManager.AUDIOFOCUS_LOSS) {
                        if (!requestAudioFocus()) {
                            Log.e(STATE_TAG, "Cannot get focus, hence not starting streaming.");
                            startStream = false;
                        } else {
                            mCurrentAudioFocus = AudioManager.AUDIOFOCUS_GAIN;
                        }
                    }
                    if (startStream) {
                        startAvrcpUpdates();
                        startFluorideStreaming();
                    }
                    // If we did not get focus, it may mean that the device in a call state and
                    // hence we should wait for an audio focus event.
                    break;

                // On Audio Focus events we stay in the same state but this can potentially change
                // if we playback.
                case AUDIO_FOCUS_CHANGE:
                    int newAudioFocus = (int) message.arg1;
                    if (DBG) {
                        Log.d(STATE_TAG,
                            "prev focus " + mCurrentAudioFocus + " new focus " + newAudioFocus);
                    }

                    if (newAudioFocus == AudioManager.AUDIOFOCUS_GAIN) {
                        // We have gained focus so play with 1.0 gain.
                        sendAvrcpPlay();
                        startAvrcpUpdates();
                        startFluorideStreaming();
                        setFluorideAudioTrackGain(1.0f);
                    } else if (newAudioFocus == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK) {
                        // Make the volume duck.
                        int duckPercent =
                            mContext.getResources().getInteger(R.integer.a2dp_sink_duck_percent);
                        if (duckPercent < 0 || duckPercent > 100) {
                            Log.e(STATE_TAG, "Invalid duck percent using default.");
                            duckPercent = DEFAULT_DUCK_PERCENT;
                        }
                        float duckRatio = (float) ((duckPercent * 1.0f) / 100);
                        Log.d(STATE_TAG,
                            "Setting reduce gain on transient loss gain=" + duckRatio);
                        setFluorideAudioTrackGain(duckRatio);
                    } else {
                        // We either are in transient loss or we are in permanent loss,
                        // either ways we should stop streaming.
                        sendAvrcpPause();
                        stopAvrcpUpdates();
                        stopFluorideStreaming();

                        // If it is permanent focus loss then we should abandon focus here and wait
                        // for user to explicitly play again.
                        if (newAudioFocus == AudioManager.AUDIOFOCUS_LOSS) {
                            mAudioManager.abandonAudioFocus(mAudioFocusListener);
                            mCurrentAudioFocus = AudioManager.AUDIOFOCUS_LOSS;
                        }
                    }
                    mCurrentAudioFocus = newAudioFocus;
                    break;

                case DISCONNECT:
                    deferMessage(message);
                    transitionTo(mSrcFActF);
                    break;

                default:
                    Log.e(TAG, "Don't know how to handle " + message.what);
            }
            return HANDLED;
        }
    }
}
