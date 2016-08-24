/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.bluetooth.avrcp;

import java.util.Timer;
import java.util.TimerTask;

import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothAvrcp;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.media.AudioManager;
import android.media.MediaDescription;
import android.media.MediaMetadata;
import android.media.session.MediaController;
import android.media.session.MediaSessionManager;
import android.media.session.PlaybackState;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.ParcelUuid;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.util.Log;
import android.view.KeyEvent;

import com.android.bluetooth.R;
import com.android.bluetooth.btservice.AdapterService;
import com.android.bluetooth.btservice.ProfileService;
import com.android.bluetooth.Utils;
import com.android.internal.util.IState;
import com.android.internal.util.State;
import com.android.internal.util.StateMachine;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
/**
 * support Bluetooth AVRCP profile.
 * support metadata, play status and event notification
 */
public final class Avrcp {
    private static final boolean DEBUG = false;
    private static final String TAG = "Avrcp";
    private static final String ABSOLUTE_VOLUME_BLACKLIST = "absolute_volume_blacklist";

    private Context mContext;
    private final AudioManager mAudioManager;
    private AvrcpMessageHandler mHandler;
    private MediaSessionManager mMediaSessionManager;
    private MediaSessionChangeListener mSessionChangeListener;
    private MediaController mMediaController;
    private MediaControllerListener mMediaControllerCb;
    private MediaAttributes mMediaAttributes;
    private int mTransportControlFlags;
    private PlaybackState mCurrentPlayState;
    private long mLastStateUpdate;
    private int mPlayStatusChangedNT;
    private int mTrackChangedNT;
    private int mPlayPosChangedNT;
    private long mTrackNumber;
    private long mSongLengthMs;
    private long mPlaybackIntervalMs;
    private long mNextPosMs;
    private long mPrevPosMs;
    private long mSkipStartTime;
    private int mFeatures;
    private int mRemoteVolume;
    private int mLastRemoteVolume;
    private int mInitialRemoteVolume;

    /* Local volume in audio index 0-15 */
    private int mLocalVolume;
    private int mLastLocalVolume;
    private int mAbsVolThreshold;

    private String mAddress;
    private HashMap<Integer, Integer> mVolumeMapping;

    private int mLastDirection;
    private final int mVolumeStep;
    private final int mAudioStreamMax;
    private boolean mVolCmdAdjustInProgress;
    private boolean mVolCmdSetInProgress;
    private int mAbsVolRetryTimes;
    private int mSkipAmount;

    /* BTRC features */
    public static final int BTRC_FEAT_METADATA = 0x01;
    public static final int BTRC_FEAT_ABSOLUTE_VOLUME = 0x02;
    public static final int BTRC_FEAT_BROWSE = 0x04;

    /* AVRC response codes, from avrc_defs */
    private static final int AVRC_RSP_NOT_IMPL = 8;
    private static final int AVRC_RSP_ACCEPT = 9;
    private static final int AVRC_RSP_REJ = 10;
    private static final int AVRC_RSP_IN_TRANS = 11;
    private static final int AVRC_RSP_IMPL_STBL = 12;
    private static final int AVRC_RSP_CHANGED = 13;
    private static final int AVRC_RSP_INTERIM = 15;

    private static final int MESSAGE_GET_RC_FEATURES = 1;
    private static final int MESSAGE_GET_PLAY_STATUS = 2;
    private static final int MESSAGE_GET_ELEM_ATTRS = 3;
    private static final int MESSAGE_REGISTER_NOTIFICATION = 4;
    private static final int MESSAGE_PLAY_INTERVAL_TIMEOUT = 5;
    private static final int MESSAGE_VOLUME_CHANGED = 6;
    private static final int MESSAGE_ADJUST_VOLUME = 7;
    private static final int MESSAGE_SET_ABSOLUTE_VOLUME = 8;
    private static final int MESSAGE_ABS_VOL_TIMEOUT = 9;
    private static final int MESSAGE_FAST_FORWARD = 10;
    private static final int MESSAGE_REWIND = 11;
    private static final int MESSAGE_CHANGE_PLAY_POS = 12;
    private static final int MESSAGE_SET_A2DP_AUDIO_STATE = 13;

    private static final int BUTTON_TIMEOUT_TIME = 2000;
    private static final int BASE_SKIP_AMOUNT = 2000;
    private static final int KEY_STATE_PRESS = 1;
    private static final int KEY_STATE_RELEASE = 0;
    private static final int SKIP_PERIOD = 400;
    private static final int SKIP_DOUBLE_INTERVAL = 3000;
    private static final long MAX_MULTIPLIER_VALUE = 128L;
    private static final int CMD_TIMEOUT_DELAY = 2000;
    private static final int MAX_ERROR_RETRY_TIMES = 6;
    private static final int AVRCP_MAX_VOL = 127;
    private static final int AVRCP_BASE_VOLUME_STEP = 1;

    static {
        classInitNative();
    }

    private Avrcp(Context context) {
        mMediaAttributes = new MediaAttributes(null);
        mCurrentPlayState = new PlaybackState.Builder().setState(PlaybackState.STATE_NONE, -1L, 0.0f).build();
        mPlayStatusChangedNT = NOTIFICATION_TYPE_CHANGED;
        mTrackChangedNT = NOTIFICATION_TYPE_CHANGED;
        mTrackNumber = -1L;
        mLastStateUpdate = -1L;
        mSongLengthMs = 0L;
        mPlaybackIntervalMs = 0L;
        mPlayPosChangedNT = NOTIFICATION_TYPE_CHANGED;
        mNextPosMs = -1;
        mPrevPosMs = -1;
        mFeatures = 0;
        mRemoteVolume = -1;
        mInitialRemoteVolume = -1;
        mLastRemoteVolume = -1;
        mLastDirection = 0;
        mVolCmdAdjustInProgress = false;
        mVolCmdSetInProgress = false;
        mAbsVolRetryTimes = 0;
        mLocalVolume = -1;
        mLastLocalVolume = -1;
        mAbsVolThreshold = 0;
        mVolumeMapping = new HashMap<Integer, Integer>();

        mContext = context;

        initNative();

        mMediaSessionManager = (MediaSessionManager) context.getSystemService(Context.MEDIA_SESSION_SERVICE);
        mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        mAudioStreamMax = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        mVolumeStep = Math.max(AVRCP_BASE_VOLUME_STEP, AVRCP_MAX_VOL/mAudioStreamMax);
        Resources resources = context.getResources();
        if (resources != null) {
            mAbsVolThreshold = resources.getInteger(R.integer.a2dp_absolute_volume_initial_threshold);
        }
    }

    private void start() {
        HandlerThread thread = new HandlerThread("BluetoothAvrcpHandler");
        thread.start();
        Looper looper = thread.getLooper();
        mHandler = new AvrcpMessageHandler(looper);

        mSessionChangeListener = new MediaSessionChangeListener();
        mMediaSessionManager.addOnActiveSessionsChangedListener(mSessionChangeListener, null, mHandler);
        List<MediaController> sessions = mMediaSessionManager.getActiveSessions(null);
        mMediaControllerCb = new MediaControllerListener();
        if (sessions.size() > 0) {
            updateCurrentMediaController(sessions.get(0));
        }
    }

    public static Avrcp make(Context context) {
        if (DEBUG) Log.v(TAG, "make");
        Avrcp ar = new Avrcp(context);
        ar.start();
        return ar;
    }

    public void doQuit() {
        mHandler.removeCallbacksAndMessages(null);
        Looper looper = mHandler.getLooper();
        if (looper != null) {
            looper.quit();
        }
        mMediaSessionManager.removeOnActiveSessionsChangedListener(mSessionChangeListener);
    }

    public void cleanup() {
        cleanupNative();
        if (mVolumeMapping != null)
            mVolumeMapping.clear();
    }

    private class MediaControllerListener extends MediaController.Callback {
        @Override
        public void onMetadataChanged(MediaMetadata metadata) {
            Log.v(TAG, "MediaController metadata changed");
            updateMetadata(metadata);
        }

        @Override
        public void onPlaybackStateChanged(PlaybackState state) {
            Log.v(TAG, "MediaController playback changed: " + state.toString());
            updatePlaybackState(state);
        }

        @Override
        public void onSessionDestroyed() {
            Log.v(TAG, "MediaController session destroyed");
        }
    }

    private class MediaSessionChangeListener implements MediaSessionManager.OnActiveSessionsChangedListener {
        public MediaSessionChangeListener() {
        }

        @Override
        public void onActiveSessionsChanged(List<MediaController> controllers) {
            Log.v(TAG, "Active sessions changed, " + controllers.size() + " sessions");
            if (controllers.size() > 0) {
                updateCurrentMediaController(controllers.get(0));
            }
        }
    }

    private void updateCurrentMediaController(MediaController controller) {
        Log.v(TAG, "Updating media controller to " + controller);
        if (mMediaController != null) {
            mMediaController.unregisterCallback(mMediaControllerCb);
        }
        mMediaController = controller;
        if (mMediaController == null) {
            updateMetadata(null);
            return;
        }
        mMediaController.registerCallback(mMediaControllerCb, mHandler);
        updateMetadata(mMediaController.getMetadata());
    }

    /** Handles Avrcp messages. */
    private final class AvrcpMessageHandler extends Handler {
        private AvrcpMessageHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_GET_RC_FEATURES:
                String address = (String) msg.obj;
                if (DEBUG) Log.v(TAG, "MESSAGE_GET_RC_FEATURES: address="+address+
                                                             ", features="+msg.arg1);
                mFeatures = msg.arg1;
                mFeatures = modifyRcFeatureFromBlacklist(mFeatures, address);
                mAudioManager.avrcpSupportsAbsoluteVolume(address, isAbsoluteVolumeSupported());
                mLastLocalVolume = -1;
                mRemoteVolume = -1;
                mLocalVolume = -1;
                mInitialRemoteVolume = -1;
                mAddress = address;
                if (mVolumeMapping != null)
                    mVolumeMapping.clear();
                break;

            case MESSAGE_GET_PLAY_STATUS:
                if (DEBUG) Log.v(TAG, "MESSAGE_GET_PLAY_STATUS");
                getPlayStatusRspNative(convertPlayStateToPlayStatus(mCurrentPlayState),
                                       (int)mSongLengthMs, (int)getPlayPosition());
                break;

            case MESSAGE_GET_ELEM_ATTRS:
                String[] textArray;
                int[] attrIds;
                byte numAttr = (byte) msg.arg1;
                ArrayList<Integer> attrList = (ArrayList<Integer>) msg.obj;
                Log.v(TAG, "MESSAGE_GET_ELEM_ATTRS:numAttr=" + numAttr);
                attrIds = new int[numAttr];
                textArray = new String[numAttr];
                for (int i = 0; i < numAttr; ++i) {
                    attrIds[i] = attrList.get(i).intValue();
                    textArray[i] = mMediaAttributes.getString(attrIds[i]);
                    Log.v(TAG, "getAttributeString:attrId=" + attrIds[i] +
                               " str=" + textArray[i]);
                }
                getElementAttrRspNative(numAttr, attrIds, textArray);
                break;

            case MESSAGE_REGISTER_NOTIFICATION:
                if (DEBUG) Log.v(TAG, "MESSAGE_REGISTER_NOTIFICATION:event=" + msg.arg1 +
                                      " param=" + msg.arg2);
                processRegisterNotification(msg.arg1, msg.arg2);
                break;

            case MESSAGE_PLAY_INTERVAL_TIMEOUT:
                if (DEBUG) Log.v(TAG, "MESSAGE_PLAY_INTERVAL_TIMEOUT");
                sendPlayPosNotificationRsp(false);
                break;

            case MESSAGE_VOLUME_CHANGED:
                if (!isAbsoluteVolumeSupported()) {
                    if (DEBUG) Log.v(TAG, "ignore MESSAGE_VOLUME_CHANGED");
                    break;
                }

                if (DEBUG) Log.v(TAG, "MESSAGE_VOLUME_CHANGED: volume=" + ((byte)msg.arg1 & 0x7f)
                                                        + " ctype=" + msg.arg2);


                boolean volAdj = false;
                if (msg.arg2 == AVRC_RSP_ACCEPT || msg.arg2 == AVRC_RSP_REJ) {
                    if (mVolCmdAdjustInProgress == false && mVolCmdSetInProgress == false) {
                        Log.e(TAG, "Unsolicited response, ignored");
                        break;
                    }
                    removeMessages(MESSAGE_ABS_VOL_TIMEOUT);

                    volAdj = mVolCmdAdjustInProgress;
                    mVolCmdAdjustInProgress = false;
                    mVolCmdSetInProgress = false;
                    mAbsVolRetryTimes = 0;
                }

                byte absVol = (byte)((byte)msg.arg1 & 0x7f); // discard MSB as it is RFD
                // convert remote volume to local volume
                int volIndex = convertToAudioStreamVolume(absVol);
                if (mInitialRemoteVolume == -1) {
                    mInitialRemoteVolume = absVol;
                    if (mAbsVolThreshold > 0 && mAbsVolThreshold < mAudioStreamMax && volIndex > mAbsVolThreshold) {
                        if (DEBUG) Log.v(TAG, "remote inital volume too high " + volIndex + ">" + mAbsVolThreshold);
                        Message msg1 = mHandler.obtainMessage(MESSAGE_SET_ABSOLUTE_VOLUME, mAbsVolThreshold , 0);
                        mHandler.sendMessage(msg1);
                        mRemoteVolume = absVol;
                        mLocalVolume = volIndex;
                        break;
                    }
                }

                if (mLocalVolume != volIndex && (msg.arg2 == AVRC_RSP_ACCEPT ||
                                                 msg.arg2 == AVRC_RSP_CHANGED ||
                                                 msg.arg2 == AVRC_RSP_INTERIM)) {
                    /* If the volume has successfully changed */
                    mLocalVolume = volIndex;
                    if (mLastLocalVolume != -1 && msg.arg2 == AVRC_RSP_ACCEPT) {
                        if (mLastLocalVolume != volIndex) {
                            /* remote volume changed more than requested due to
                             * local and remote has different volume steps */
                            if (DEBUG) Log.d(TAG, "Remote returned volume does not match desired volume "
                                + mLastLocalVolume + " vs "
                                + volIndex);
                            mLastLocalVolume = mLocalVolume;
                        }
                    }
                    // remember the remote volume value, as it's the one supported by remote
                    if (volAdj) {
                        synchronized (mVolumeMapping) {
                            mVolumeMapping.put(volIndex, (int)absVol);
                            if (DEBUG) Log.v(TAG, "remember volume mapping " +volIndex+ "-"+absVol);
                        }
                    }

                    notifyVolumeChanged(mLocalVolume);
                    mRemoteVolume = absVol;
                    long pecentVolChanged = ((long)absVol * 100) / 0x7f;
                    Log.e(TAG, "percent volume changed: " + pecentVolChanged + "%");
                } else if (msg.arg2 == AVRC_RSP_REJ) {
                    Log.e(TAG, "setAbsoluteVolume call rejected");
                } else if (volAdj && mLastRemoteVolume > 0 && mLastRemoteVolume < AVRCP_MAX_VOL &&
                        mLocalVolume == volIndex &&
                        (msg.arg2 == AVRC_RSP_ACCEPT )) {
                    /* oops, the volume is still same, remote does not like the value
                     * retry a volume one step up/down */
                    if (DEBUG) Log.d(TAG, "Remote device didn't tune volume, let's try one more step.");
                    int retry_volume = Math.min(AVRCP_MAX_VOL,
                            Math.max(0, mLastRemoteVolume + mLastDirection));
                    if (setVolumeNative(retry_volume)) {
                        mLastRemoteVolume = retry_volume;
                        sendMessageDelayed(obtainMessage(MESSAGE_ABS_VOL_TIMEOUT),
                                           CMD_TIMEOUT_DELAY);
                        mVolCmdAdjustInProgress = true;
                    }
                }
                break;

            case MESSAGE_ADJUST_VOLUME:
                if (!isAbsoluteVolumeSupported()) {
                    if (DEBUG) Log.v(TAG, "ignore MESSAGE_ADJUST_VOLUME");
                    break;
                }

                if (DEBUG) Log.d(TAG, "MESSAGE_ADJUST_VOLUME: direction=" + msg.arg1);

                if (mVolCmdAdjustInProgress || mVolCmdSetInProgress) {
                    if (DEBUG) Log.w(TAG, "There is already a volume command in progress.");
                    break;
                }

                // Remote device didn't set initial volume. Let's black list it
                if (mInitialRemoteVolume == -1) {
                    Log.d(TAG, "remote " + mAddress + " never tell us initial volume, black list it.");
                    blackListCurrentDevice();
                    break;
                }

                // Wait on verification on volume from device, before changing the volume.
                if (mRemoteVolume != -1 && (msg.arg1 == -1 || msg.arg1 == 1)) {
                    int setVol = -1;
                    int targetVolIndex = -1;
                    if (mLocalVolume == 0 && msg.arg1 == -1) {
                        if (DEBUG) Log.w(TAG, "No need to Vol down from 0.");
                        break;
                    }
                    if (mLocalVolume == mAudioStreamMax && msg.arg1 == 1) {
                        if (DEBUG) Log.w(TAG, "No need to Vol up from max.");
                        break;
                    }

                    targetVolIndex = mLocalVolume + msg.arg1;
                    if (DEBUG) Log.d(TAG, "Adjusting volume to  " + targetVolIndex);

                    Integer i;
                    synchronized (mVolumeMapping) {
                        i = mVolumeMapping.get(targetVolIndex);
                    }

                    if (i != null) {
                        /* if we already know this volume mapping, use it */
                        setVol = i.byteValue();
                        if (setVol == mRemoteVolume) {
                            if (DEBUG) Log.d(TAG, "got same volume from mapping for " + targetVolIndex + ", ignore.");
                            setVol = -1;
                        }
                        if (DEBUG) Log.d(TAG, "set volume from mapping " + targetVolIndex + "-" + setVol);
                    }

                    if (setVol == -1) {
                        /* otherwise use phone steps */
                        setVol = Math.min(AVRCP_MAX_VOL,
                                 convertToAvrcpVolume(Math.max(0, targetVolIndex)));
                        if (DEBUG) Log.d(TAG, "set volume from local volume "+ targetVolIndex+"-"+ setVol);
                    }

                    if (setVolumeNative(setVol)) {
                        sendMessageDelayed(obtainMessage(MESSAGE_ABS_VOL_TIMEOUT),
                                           CMD_TIMEOUT_DELAY);
                        mVolCmdAdjustInProgress = true;
                        mLastDirection = msg.arg1;
                        mLastRemoteVolume = setVol;
                        mLastLocalVolume = targetVolIndex;
                    } else {
                         if (DEBUG) Log.d(TAG, "setVolumeNative failed");
                    }
                } else {
                    Log.e(TAG, "Unknown direction in MESSAGE_ADJUST_VOLUME");
                }
                break;

            case MESSAGE_SET_ABSOLUTE_VOLUME:
                if (!isAbsoluteVolumeSupported()) {
                    if (DEBUG) Log.v(TAG, "ignore MESSAGE_SET_ABSOLUTE_VOLUME");
                    break;
                }

                if (DEBUG) Log.v(TAG, "MESSAGE_SET_ABSOLUTE_VOLUME");

                if (mVolCmdSetInProgress || mVolCmdAdjustInProgress) {
                    if (DEBUG) Log.w(TAG, "There is already a volume command in progress.");
                    break;
                }

                // Remote device didn't set initial volume. Let's black list it
                if (mInitialRemoteVolume == -1) {
                    if (DEBUG) Log.d(TAG, "remote " + mAddress + " never tell us initial volume, black list it.");
                    blackListCurrentDevice();
                    break;
                }

                int avrcpVolume = convertToAvrcpVolume(msg.arg1);
                avrcpVolume = Math.min(AVRCP_MAX_VOL, Math.max(0, avrcpVolume));
                if (DEBUG) Log.d(TAG, "Setting volume to " + msg.arg1 + "-" + avrcpVolume);
                if (setVolumeNative(avrcpVolume)) {
                    sendMessageDelayed(obtainMessage(MESSAGE_ABS_VOL_TIMEOUT), CMD_TIMEOUT_DELAY);
                    mVolCmdSetInProgress = true;
                    mLastRemoteVolume = avrcpVolume;
                    mLastLocalVolume = msg.arg1;
                } else {
                     if (DEBUG) Log.d(TAG, "setVolumeNative failed");
                }
                break;

            case MESSAGE_ABS_VOL_TIMEOUT:
                if (DEBUG) Log.v(TAG, "MESSAGE_ABS_VOL_TIMEOUT: Volume change cmd timed out.");
                mVolCmdAdjustInProgress = false;
                mVolCmdSetInProgress = false;
                if (mAbsVolRetryTimes >= MAX_ERROR_RETRY_TIMES) {
                    mAbsVolRetryTimes = 0;
                    /* too many volume change failures, black list the device */
                    blackListCurrentDevice();
                } else {
                    mAbsVolRetryTimes += 1;
                    if (setVolumeNative(mLastRemoteVolume)) {
                        sendMessageDelayed(obtainMessage(MESSAGE_ABS_VOL_TIMEOUT),
                                           CMD_TIMEOUT_DELAY);
                        mVolCmdSetInProgress = true;
                    }
                }
                break;

            case MESSAGE_FAST_FORWARD:
            case MESSAGE_REWIND:
                if (msg.what == MESSAGE_FAST_FORWARD) {
                    if ((mCurrentPlayState.getActions() &
                                PlaybackState.ACTION_FAST_FORWARD) != 0) {
                        int keyState = msg.arg1 == KEY_STATE_PRESS ?
                                KeyEvent.ACTION_DOWN : KeyEvent.ACTION_UP;
                        KeyEvent keyEvent =
                                new KeyEvent(keyState, KeyEvent.KEYCODE_MEDIA_FAST_FORWARD);
                        mMediaController.dispatchMediaButtonEvent(keyEvent);
                        break;
                    }
                } else if ((mCurrentPlayState.getActions() &
                            PlaybackState.ACTION_REWIND) != 0) {
                    int keyState = msg.arg1 == KEY_STATE_PRESS ?
                            KeyEvent.ACTION_DOWN : KeyEvent.ACTION_UP;
                    KeyEvent keyEvent =
                            new KeyEvent(keyState, KeyEvent.KEYCODE_MEDIA_REWIND);
                    mMediaController.dispatchMediaButtonEvent(keyEvent);
                    break;
                }

                int skipAmount;
                if (msg.what == MESSAGE_FAST_FORWARD) {
                    if (DEBUG) Log.v(TAG, "MESSAGE_FAST_FORWARD");
                    removeMessages(MESSAGE_FAST_FORWARD);
                    skipAmount = BASE_SKIP_AMOUNT;
                } else {
                    if (DEBUG) Log.v(TAG, "MESSAGE_REWIND");
                    removeMessages(MESSAGE_REWIND);
                    skipAmount = -BASE_SKIP_AMOUNT;
                }

                if (hasMessages(MESSAGE_CHANGE_PLAY_POS) &&
                        (skipAmount != mSkipAmount)) {
                    Log.w(TAG, "missing release button event:" + mSkipAmount);
                }

                if ((!hasMessages(MESSAGE_CHANGE_PLAY_POS)) ||
                        (skipAmount != mSkipAmount)) {
                    mSkipStartTime = SystemClock.elapsedRealtime();
                }

                removeMessages(MESSAGE_CHANGE_PLAY_POS);
                if (msg.arg1 == KEY_STATE_PRESS) {
                    mSkipAmount = skipAmount;
                    changePositionBy(mSkipAmount * getSkipMultiplier());
                    Message posMsg = obtainMessage(MESSAGE_CHANGE_PLAY_POS);
                    posMsg.arg1 = 1;
                    sendMessageDelayed(posMsg, SKIP_PERIOD);
                }

                break;

            case MESSAGE_CHANGE_PLAY_POS:
                if (DEBUG) Log.v(TAG, "MESSAGE_CHANGE_PLAY_POS:" + msg.arg1);
                changePositionBy(mSkipAmount * getSkipMultiplier());
                if (msg.arg1 * SKIP_PERIOD < BUTTON_TIMEOUT_TIME) {
                    Message posMsg = obtainMessage(MESSAGE_CHANGE_PLAY_POS);
                    posMsg.arg1 = msg.arg1 + 1;
                    sendMessageDelayed(posMsg, SKIP_PERIOD);
                }
                break;

            case MESSAGE_SET_A2DP_AUDIO_STATE:
                if (DEBUG) Log.v(TAG, "MESSAGE_SET_A2DP_AUDIO_STATE:" + msg.arg1);
                updateA2dpAudioState(msg.arg1);
                break;
            }
        }
    }

    private void updateA2dpAudioState(int state) {
        boolean isPlaying = (state == BluetoothA2dp.STATE_PLAYING);
        if (isPlaying != isPlayingState(mCurrentPlayState)) {
            /* if a2dp is streaming, check to make sure music is active */
            if (isPlaying && !mAudioManager.isMusicActive())
                return;
            PlaybackState.Builder builder = new PlaybackState.Builder();
            if (isPlaying) {
                builder.setState(PlaybackState.STATE_PLAYING,
                                 PlaybackState.PLAYBACK_POSITION_UNKNOWN, 1.0f);
            } else {
                builder.setState(PlaybackState.STATE_PAUSED,
                                 PlaybackState.PLAYBACK_POSITION_UNKNOWN, 0.0f);
            }
            updatePlaybackState(builder.build());
        }
    }

    private void updatePlaybackState(PlaybackState state) {
        if (DEBUG) Log.v(TAG,
                "updatePlaybackState: old=" + mCurrentPlayState + ", new=" + state);
        if (state == null) {
          state = new PlaybackState.Builder().setState(PlaybackState.STATE_NONE,
                         PlaybackState.PLAYBACK_POSITION_UNKNOWN, 0.0f).build();
        }

        int newPlayStatus = convertPlayStateToPlayStatus(state);

        mCurrentPlayState = state;
        mLastStateUpdate = SystemClock.elapsedRealtime();

        sendPlayPosNotificationRsp(false);

        if (mPlayStatusChangedNT == NOTIFICATION_TYPE_INTERIM) {
            mPlayStatusChangedNT = NOTIFICATION_TYPE_CHANGED;
            registerNotificationRspPlayStatusNative(mPlayStatusChangedNT, newPlayStatus);
        }
    }

    private void updateTransportControls(int transportControlFlags) {
        mTransportControlFlags = transportControlFlags;
    }

    class MediaAttributes {
        private boolean exists;
        private String title;
        private String artistName;
        private String albumName;
        private String mediaNumber;
        private String mediaTotalNumber;
        private String genre;
        private String playingTimeMs;

        private static final int ATTR_TITLE = 1;
        private static final int ATTR_ARTIST_NAME = 2;
        private static final int ATTR_ALBUM_NAME = 3;
        private static final int ATTR_MEDIA_NUMBER = 4;
        private static final int ATTR_MEDIA_TOTAL_NUMBER = 5;
        private static final int ATTR_GENRE = 6;
        private static final int ATTR_PLAYING_TIME_MS = 7;


        public MediaAttributes(MediaMetadata data) {
            exists = data != null;
            if (!exists)
                return;

            artistName = stringOrBlank(data.getString(MediaMetadata.METADATA_KEY_ARTIST));
            albumName = stringOrBlank(data.getString(MediaMetadata.METADATA_KEY_ALBUM));
            mediaNumber = longStringOrBlank(data.getLong(MediaMetadata.METADATA_KEY_TRACK_NUMBER));
            mediaTotalNumber = longStringOrBlank(data.getLong(MediaMetadata.METADATA_KEY_NUM_TRACKS));
            genre = stringOrBlank(data.getString(MediaMetadata.METADATA_KEY_GENRE));
            playingTimeMs = longStringOrBlank(data.getLong(MediaMetadata.METADATA_KEY_DURATION));

            // Try harder for the title.
            title = data.getString(MediaMetadata.METADATA_KEY_TITLE);

            if (title == null) {
                MediaDescription desc = data.getDescription();
                if (desc != null) {
                    CharSequence val = desc.getDescription();
                    if (val != null)
                        title = val.toString();
                }
            }

            if (title == null)
                title = new String();
        }

        public boolean equals(MediaAttributes other) {
            if (other == null)
                return false;

            if (exists != other.exists)
                return false;

            if (exists == false)
                return true;

            return (title.equals(other.title)) &&
                (artistName.equals(other.artistName)) &&
                (albumName.equals(other.albumName)) &&
                (mediaNumber.equals(other.mediaNumber)) &&
                (mediaTotalNumber.equals(other.mediaTotalNumber)) &&
                (genre.equals(other.genre)) &&
                (playingTimeMs.equals(other.playingTimeMs));
        }

        public String getString(int attrId) {
            if (!exists)
                return new String();

            switch (attrId) {
                case ATTR_TITLE:
                    return title;
                case ATTR_ARTIST_NAME:
                    return artistName;
                case ATTR_ALBUM_NAME:
                    return albumName;
                case ATTR_MEDIA_NUMBER:
                    return mediaNumber;
                case ATTR_MEDIA_TOTAL_NUMBER:
                    return mediaTotalNumber;
                case ATTR_GENRE:
                    return genre;
                case ATTR_PLAYING_TIME_MS:
                    return playingTimeMs;
                default:
                    return new String();
            }
        }

        private String stringOrBlank(String s) {
            return s == null ? new String() : s;
        }

        private String longStringOrBlank(Long s) {
            return s == null ? new String() : s.toString();
        }

        public String toString() {
            if (!exists)
                return "[MediaAttributes: none]";

            return "[MediaAttributes: " + title + " - " + albumName + " by "
                + artistName + " (" + mediaNumber + "/" + mediaTotalNumber + ") "
                + genre + "]";
        }
    }

    private void updateMetadata(MediaMetadata data) {
        MediaAttributes oldAttributes = mMediaAttributes;
        mMediaAttributes = new MediaAttributes(data);
        if (data == null) {
            mSongLengthMs = 0L;
        } else {
            mSongLengthMs = data.getLong(MediaMetadata.METADATA_KEY_DURATION);
        }
        if (!oldAttributes.equals(mMediaAttributes)) {
            Log.v(TAG, "MediaAttributes Changed to " + mMediaAttributes.toString());
            mTrackNumber++;

            // Update the play state, which sends play state and play position
            // notifications if needed.
            if (mMediaController != null) {
              updatePlaybackState(mMediaController.getPlaybackState());
            } else {
              updatePlaybackState(null);
            }

            if (mTrackChangedNT == NOTIFICATION_TYPE_INTERIM) {
                mTrackChangedNT = NOTIFICATION_TYPE_CHANGED;
                sendTrackChangedRsp();
            }
        } else {
            Log.v(TAG, "Updated " + mMediaAttributes.toString() + " but no change!");
        }

    }

    private void getRcFeatures(byte[] address, int features) {
        Message msg = mHandler.obtainMessage(MESSAGE_GET_RC_FEATURES, features, 0,
                                             Utils.getAddressStringFromByte(address));
        mHandler.sendMessage(msg);
    }

    private void getPlayStatus() {
        Message msg = mHandler.obtainMessage(MESSAGE_GET_PLAY_STATUS);
        mHandler.sendMessage(msg);
    }

    private void getElementAttr(byte numAttr, int[] attrs) {
        int i;
        ArrayList<Integer> attrList = new ArrayList<Integer>();
        for (i = 0; i < numAttr; ++i) {
            attrList.add(attrs[i]);
        }
        Message msg = mHandler.obtainMessage(MESSAGE_GET_ELEM_ATTRS, numAttr, 0, attrList);
        mHandler.sendMessage(msg);
    }

    private void registerNotification(int eventId, int param) {
        Message msg = mHandler.obtainMessage(MESSAGE_REGISTER_NOTIFICATION, eventId, param);
        mHandler.sendMessage(msg);
    }

    private void processRegisterNotification(int eventId, int param) {
        switch (eventId) {
            case EVT_PLAY_STATUS_CHANGED:
                mPlayStatusChangedNT = NOTIFICATION_TYPE_INTERIM;
                registerNotificationRspPlayStatusNative(mPlayStatusChangedNT,
                        convertPlayStateToPlayStatus(mCurrentPlayState));
                break;

            case EVT_TRACK_CHANGED:
                mTrackChangedNT = NOTIFICATION_TYPE_INTERIM;
                sendTrackChangedRsp();
                break;

            case EVT_PLAY_POS_CHANGED:
                mPlayPosChangedNT = NOTIFICATION_TYPE_INTERIM;
                sendPlayPosNotificationRsp(true);
                mPlaybackIntervalMs = (long)param * 1000L;
                break;

        }
    }

    private void handlePassthroughCmd(int id, int keyState) {
        switch (id) {
            case BluetoothAvrcp.PASSTHROUGH_ID_REWIND:
                rewind(keyState);
                break;
            case BluetoothAvrcp.PASSTHROUGH_ID_FAST_FOR:
                fastForward(keyState);
                break;
        }
    }

    private void fastForward(int keyState) {
        Message msg = mHandler.obtainMessage(MESSAGE_FAST_FORWARD, keyState, 0);
        mHandler.sendMessage(msg);
    }

    private void rewind(int keyState) {
        Message msg = mHandler.obtainMessage(MESSAGE_REWIND, keyState, 0);
        mHandler.sendMessage(msg);
    }

    private void changePositionBy(long amount) {
        long currentPosMs = getPlayPosition();
        if (currentPosMs == -1L) return;
        long newPosMs = Math.max(0L, currentPosMs + amount);
        mMediaController.getTransportControls().seekTo(newPosMs);
    }

    private int getSkipMultiplier() {
        long currentTime = SystemClock.elapsedRealtime();
        long multi = (long) Math.pow(2, (currentTime - mSkipStartTime)/SKIP_DOUBLE_INTERVAL);
        return (int) Math.min(MAX_MULTIPLIER_VALUE, multi);
    }

    private void sendTrackChangedRsp() {
        byte[] track = new byte[TRACK_ID_SIZE];

        /* If no track is currently selected, then return
           0xFFFFFFFFFFFFFFFF in the interim response */
        long trackNumberRsp = -1L;

        if (isPlayingState(mCurrentPlayState)) {
            trackNumberRsp = mTrackNumber;
        }

        /* track is stored in big endian format */
        for (int i = 0; i < TRACK_ID_SIZE; ++i) {
            track[i] = (byte) (trackNumberRsp >> (56 - 8 * i));
        }
        registerNotificationRspTrackChangeNative(mTrackChangedNT, track);
    }

    private long getPlayPosition() {
        if (mCurrentPlayState == null)
            return -1L;

        if (mCurrentPlayState.getPosition() == PlaybackState.PLAYBACK_POSITION_UNKNOWN)
            return -1L;

        if (isPlayingState(mCurrentPlayState)) {
            return SystemClock.elapsedRealtime() - mLastStateUpdate + mCurrentPlayState.getPosition();
        }

        return mCurrentPlayState.getPosition();
    }

    private int convertPlayStateToPlayStatus(PlaybackState state) {
        int playStatus = PLAYSTATUS_ERROR;
        switch (state.getState()) {
            case PlaybackState.STATE_PLAYING:
            case PlaybackState.STATE_BUFFERING:
                playStatus = PLAYSTATUS_PLAYING;
                break;

            case PlaybackState.STATE_STOPPED:
            case PlaybackState.STATE_NONE:
                playStatus = PLAYSTATUS_STOPPED;
                break;

            case PlaybackState.STATE_PAUSED:
                playStatus = PLAYSTATUS_PAUSED;
                break;

            case PlaybackState.STATE_FAST_FORWARDING:
            case PlaybackState.STATE_SKIPPING_TO_NEXT:
            case PlaybackState.STATE_SKIPPING_TO_QUEUE_ITEM:
                playStatus = PLAYSTATUS_FWD_SEEK;
                break;

            case PlaybackState.STATE_REWINDING:
            case PlaybackState.STATE_SKIPPING_TO_PREVIOUS:
                playStatus = PLAYSTATUS_REV_SEEK;
                break;

            case PlaybackState.STATE_ERROR:
                playStatus = PLAYSTATUS_ERROR;
                break;

        }
        return playStatus;
    }

    private boolean isPlayingState(PlaybackState state) {
        return (state.getState() == PlaybackState.STATE_PLAYING) ||
                (state.getState() == PlaybackState.STATE_BUFFERING);
    }

    /**
     * Sends a play position notification, or schedules one to be
     * sent later at an appropriate time. If |requested| is true,
     * does both because this was called in reponse to a request from the
     * TG.
     */
    private void sendPlayPosNotificationRsp(boolean requested) {
        long playPositionMs = getPlayPosition();

        // mNextPosMs is set to -1 when the previous position was invalid
        // so this will be true if the new position is valid & old was invalid.
        // mPlayPositionMs is set to -1 when the new position is invalid,
        // and the old mPrevPosMs is >= 0 so this is true when the new is invalid
        // and the old was valid.
        if (requested || ((mPlayPosChangedNT == NOTIFICATION_TYPE_INTERIM) &&
             ((playPositionMs >= mNextPosMs) || (playPositionMs <= mPrevPosMs)))) {
            if (!requested) mPlayPosChangedNT = NOTIFICATION_TYPE_CHANGED;
            registerNotificationRspPlayPosNative(mPlayStatusChangedNT, (int)playPositionMs);
            if (playPositionMs != PlaybackState.PLAYBACK_POSITION_UNKNOWN) {
                mNextPosMs = playPositionMs + mPlaybackIntervalMs;
                mPrevPosMs = playPositionMs - mPlaybackIntervalMs;
            } else {
                mNextPosMs = -1;
                mPrevPosMs = -1;
            }
        }

        mHandler.removeMessages(MESSAGE_PLAY_INTERVAL_TIMEOUT);
        if (mPlayStatusChangedNT == NOTIFICATION_TYPE_INTERIM) {
            Message msg = mHandler.obtainMessage(MESSAGE_PLAY_INTERVAL_TIMEOUT);
            long delay = mPlaybackIntervalMs;
            if (mNextPosMs != -1) {
                delay = mNextPosMs - (playPositionMs > 0 ? playPositionMs : 0);
            }
            mHandler.sendMessageDelayed(msg, delay);
        }
    }

    /**
     * This is called from AudioService. It will return whether this device supports abs volume.
     * NOT USED AT THE MOMENT.
     */
    public boolean isAbsoluteVolumeSupported() {
        return ((mFeatures & BTRC_FEAT_ABSOLUTE_VOLUME) != 0);
    }

    /**
     * We get this call from AudioService. This will send a message to our handler object,
     * requesting our handler to call setVolumeNative()
     */
    public void adjustVolume(int direction) {
        Message msg = mHandler.obtainMessage(MESSAGE_ADJUST_VOLUME, direction, 0);
        mHandler.sendMessage(msg);
    }

    public void setAbsoluteVolume(int volume) {
        if (volume == mLocalVolume) {
            if (DEBUG) Log.v(TAG, "setAbsoluteVolume is setting same index, ignore "+volume);
            return;
        }

        mHandler.removeMessages(MESSAGE_ADJUST_VOLUME);
        Message msg = mHandler.obtainMessage(MESSAGE_SET_ABSOLUTE_VOLUME, volume, 0);
        mHandler.sendMessage(msg);
    }

    /* Called in the native layer as a btrc_callback to return the volume set on the carkit in the
     * case when the volume is change locally on the carkit. This notification is not called when
     * the volume is changed from the phone.
     *
     * This method will send a message to our handler to change the local stored volume and notify
     * AudioService to update the UI
     */
    private void volumeChangeCallback(int volume, int ctype) {
        Message msg = mHandler.obtainMessage(MESSAGE_VOLUME_CHANGED, volume, ctype);
        mHandler.sendMessage(msg);
    }

    private void notifyVolumeChanged(int volume) {
        mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, volume,
                      AudioManager.FLAG_SHOW_UI | AudioManager.FLAG_BLUETOOTH_ABS_VOLUME);
    }

    private int convertToAudioStreamVolume(int volume) {
        // Rescale volume to match AudioSystem's volume
        return (int) Math.floor((double) volume*mAudioStreamMax/AVRCP_MAX_VOL);
    }

    private int convertToAvrcpVolume(int volume) {
        return (int) Math.ceil((double) volume*AVRCP_MAX_VOL/mAudioStreamMax);
    }

    private void blackListCurrentDevice() {
        mFeatures &= ~BTRC_FEAT_ABSOLUTE_VOLUME;
        mAudioManager.avrcpSupportsAbsoluteVolume(mAddress, isAbsoluteVolumeSupported());

        SharedPreferences pref = mContext.getSharedPreferences(ABSOLUTE_VOLUME_BLACKLIST,
                Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = pref.edit();
        editor.putBoolean(mAddress, true);
        editor.commit();
    }

    private int modifyRcFeatureFromBlacklist(int feature, String address) {
        SharedPreferences pref = mContext.getSharedPreferences(ABSOLUTE_VOLUME_BLACKLIST,
                Context.MODE_PRIVATE);
        if (!pref.contains(address)) {
            return feature;
        }
        if (pref.getBoolean(address, false)) {
            feature &= ~BTRC_FEAT_ABSOLUTE_VOLUME;
        }
        return feature;
    }

    public void resetBlackList(String address) {
        SharedPreferences pref = mContext.getSharedPreferences(ABSOLUTE_VOLUME_BLACKLIST,
                Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = pref.edit();
        editor.remove(address);
        editor.commit();
    }

    /**
     * This is called from A2dpStateMachine to set A2dp audio state.
     */
    public void setA2dpAudioState(int state) {
        Message msg = mHandler.obtainMessage(MESSAGE_SET_A2DP_AUDIO_STATE, state, 0);
        mHandler.sendMessage(msg);
    }

    public void dump(StringBuilder sb) {
        sb.append("AVRCP:\n");
        ProfileService.println(sb, "mMediaAttributes: " + mMediaAttributes);
        ProfileService.println(sb, "mTransportControlFlags: " + mTransportControlFlags);
        ProfileService.println(sb, "mCurrentPlayState: " + mCurrentPlayState);
        ProfileService.println(sb, "mLastStateUpdate: " + mLastStateUpdate);
        ProfileService.println(sb, "mPlayStatusChangedNT: " + mPlayStatusChangedNT);
        ProfileService.println(sb, "mTrackChangedNT: " + mTrackChangedNT);
        ProfileService.println(sb, "mTrackNumber: " + mTrackNumber);
        ProfileService.println(sb, "mSongLengthMs: " + mSongLengthMs);
        ProfileService.println(sb, "mPlaybackIntervalMs: " + mPlaybackIntervalMs);
        ProfileService.println(sb, "mPlayPosChangedNT: " + mPlayPosChangedNT);
        ProfileService.println(sb, "mNextPosMs: " + mNextPosMs);
        ProfileService.println(sb, "mPrevPosMs: " + mPrevPosMs);
        ProfileService.println(sb, "mSkipStartTime: " + mSkipStartTime);
        ProfileService.println(sb, "mFeatures: " + mFeatures);
        ProfileService.println(sb, "mRemoteVolume: " + mRemoteVolume);
        ProfileService.println(sb, "mLastRemoteVolume: " + mLastRemoteVolume);
        ProfileService.println(sb, "mLastDirection: " + mLastDirection);
        ProfileService.println(sb, "mVolumeStep: " + mVolumeStep);
        ProfileService.println(sb, "mAudioStreamMax: " + mAudioStreamMax);
        ProfileService.println(sb, "mVolCmdAdjustInProgress: " + mVolCmdAdjustInProgress);
        ProfileService.println(sb, "mVolCmdSetInProgress: " + mVolCmdSetInProgress);
        ProfileService.println(sb, "mAbsVolRetryTimes: " + mAbsVolRetryTimes);
        ProfileService.println(sb, "mSkipAmount: " + mSkipAmount);
        ProfileService.println(sb, "mVolumeMapping: " + mVolumeMapping.toString());
        if (mMediaController != null)
            ProfileService.println(sb, "mMediaSession pkg: " + mMediaController.getPackageName());
    }

    // Do not modify without updating the HAL bt_rc.h files.

    // match up with btrc_play_status_t enum of bt_rc.h
    final static int PLAYSTATUS_STOPPED = 0;
    final static int PLAYSTATUS_PLAYING = 1;
    final static int PLAYSTATUS_PAUSED = 2;
    final static int PLAYSTATUS_FWD_SEEK = 3;
    final static int PLAYSTATUS_REV_SEEK = 4;
    final static int PLAYSTATUS_ERROR = 255;

    // match up with btrc_media_attr_t enum of bt_rc.h
    final static int MEDIA_ATTR_TITLE = 1;
    final static int MEDIA_ATTR_ARTIST = 2;
    final static int MEDIA_ATTR_ALBUM = 3;
    final static int MEDIA_ATTR_TRACK_NUM = 4;
    final static int MEDIA_ATTR_NUM_TRACKS = 5;
    final static int MEDIA_ATTR_GENRE = 6;
    final static int MEDIA_ATTR_PLAYING_TIME = 7;

    // match up with btrc_event_id_t enum of bt_rc.h
    final static int EVT_PLAY_STATUS_CHANGED = 1;
    final static int EVT_TRACK_CHANGED = 2;
    final static int EVT_TRACK_REACHED_END = 3;
    final static int EVT_TRACK_REACHED_START = 4;
    final static int EVT_PLAY_POS_CHANGED = 5;
    final static int EVT_BATT_STATUS_CHANGED = 6;
    final static int EVT_SYSTEM_STATUS_CHANGED = 7;
    final static int EVT_APP_SETTINGS_CHANGED = 8;

    // match up with btrc_notification_type_t enum of bt_rc.h
    final static int NOTIFICATION_TYPE_INTERIM = 0;
    final static int NOTIFICATION_TYPE_CHANGED = 1;

    // match up with BTRC_UID_SIZE of bt_rc.h
    final static int TRACK_ID_SIZE = 8;

    private native static void classInitNative();
    private native void initNative();
    private native void cleanupNative();
    private native boolean getPlayStatusRspNative(int playStatus, int songLen, int songPos);
    private native boolean getElementAttrRspNative(byte numAttr, int[] attrIds, String[] textArray);
    private native boolean registerNotificationRspPlayStatusNative(int type, int playStatus);
    private native boolean registerNotificationRspTrackChangeNative(int type, byte[] track);
    private native boolean registerNotificationRspPlayPosNative(int type, int playPos);
    private native boolean setVolumeNative(int volume);
    private native boolean sendPassThroughCommandNative(int keyCode, int keyState);

}
