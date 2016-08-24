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

package com.android.bluetooth.a2dpsink.mbs;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothAvrcpController;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.MediaMetadata;
import android.media.browse.MediaBrowser.MediaItem;
import android.media.session.MediaController;
import android.media.session.MediaSession;
import android.media.session.PlaybackState;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.ResultReceiver;
import android.service.media.MediaBrowserService;
import android.util.Pair;
import android.util.Log;

import com.android.bluetooth.R;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;

public class A2dpMediaBrowserService extends MediaBrowserService {
    private static final String TAG = "A2dpMediaBrowserService";
    private static final String MEDIA_ID_ROOT = "__ROOT__";
    private static final String UNKNOWN_BT_AUDIO = "__UNKNOWN_BT_AUDIO__";
    private static final float PLAYBACK_SPEED = 1.0f;

    // Message sent when A2DP device is disconnected.
    private static final int MSG_DEVICE_DISCONNECT = 0;
    // Message snet when the AVRCP profile is disconnected = 1;
    private static final int MSG_PROFILE_DISCONNECT = 1;
    // Message sent when A2DP device is connected.
    private static final int MSG_DEVICE_CONNECT = 2;
    // Message sent when AVRCP profile is connected (note AVRCP profile may be connected before or
    // after A2DP device is connected).
    private static final int MSG_PROFILE_CONNECT = 3;
    // Message sent when we recieve a TRACK update from AVRCP profile over a connected A2DP device.
    private static final int MSG_TRACK = 4;
    // Internal message sent to trigger a AVRCP action.
    private static final int MSG_AVRCP_PASSTHRU = 5;

    private MediaSession mSession;
    private MediaMetadata mA2dpMetadata;

    private BluetoothAdapter mAdapter;
    private BluetoothAvrcpController mAvrcpProfile;
    private BluetoothDevice mA2dpDevice = null;
    private Handler mAvrcpCommandQueue;

    private long mTransportControlFlags = PlaybackState.ACTION_PAUSE | PlaybackState.ACTION_PLAY
            | PlaybackState.ACTION_SKIP_TO_NEXT | PlaybackState.ACTION_SKIP_TO_PREVIOUS;

    private static final class AvrcpCommandQueueHandler extends Handler {
        WeakReference<A2dpMediaBrowserService> mInst;

        AvrcpCommandQueueHandler(Looper looper, A2dpMediaBrowserService sink) {
            super(looper);
            mInst = new WeakReference<A2dpMediaBrowserService>(sink);
        }

        @Override
        public void handleMessage(Message msg) {
            A2dpMediaBrowserService inst = mInst.get();
            if (inst == null) {
                Log.e(TAG, "Parent class has died; aborting.");
                return;
            }

            switch (msg.what) {
                case MSG_DEVICE_CONNECT:
                    inst.msgDeviceConnect((BluetoothDevice) msg.obj);
                    break;
                case MSG_PROFILE_CONNECT:
                    inst.msgProfileConnect((BluetoothProfile) msg.obj);
                    break;
                case MSG_DEVICE_DISCONNECT:
                    inst.msgDeviceDisconnect((BluetoothDevice) msg.obj);
                    break;
                case MSG_PROFILE_DISCONNECT:
                    inst.msgProfileDisconnect();
                    break;
                case MSG_TRACK:
                    Pair<PlaybackState, MediaMetadata> pair =
                        (Pair<PlaybackState, MediaMetadata>) (msg.obj);
                    inst.msgTrack(pair.first, pair.second);
                    break;
                case MSG_AVRCP_PASSTHRU:
                    inst.msgPassThru((int) msg.obj);
                    break;
            }
        }
    }

    @Override
    public void onCreate() {
        Log.d(TAG, "onCreate");
        super.onCreate();
        mSession = new MediaSession(this, TAG);
        setSessionToken(mSession.getSessionToken());
        mSession.setCallback(mSessionCallbacks);
        mSession.setFlags(MediaSession.FLAG_HANDLES_MEDIA_BUTTONS |
                MediaSession.FLAG_HANDLES_TRANSPORT_CONTROLS);
        mAvrcpCommandQueue = new AvrcpCommandQueueHandler(Looper.getMainLooper(), this);

        mAdapter = BluetoothAdapter.getDefaultAdapter();
        mAdapter.getProfileProxy(this, mServiceListener, BluetoothProfile.AVRCP_CONTROLLER);

        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothAvrcpController.ACTION_CONNECTION_STATE_CHANGED);
        filter.addAction(BluetoothAvrcpController.ACTION_TRACK_EVENT);
        registerReceiver(mBtReceiver, filter);
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy");
        mSession.release();
        unregisterReceiver(mBtReceiver);
        super.onDestroy();
    }

    @Override
    public BrowserRoot onGetRoot(String clientPackageName, int clientUid, Bundle rootHints) {
        return new BrowserRoot(MEDIA_ID_ROOT, null);
    }

    @Override
    public void onLoadChildren(final String parentMediaId, final Result<List<MediaItem>> result) {
        Log.d(TAG, "onLoadChildren parentMediaId=" + parentMediaId);
        List<MediaItem> items = new ArrayList<MediaItem>();
        result.sendResult(items);
    }

    BluetoothProfile.ServiceListener mServiceListener = new BluetoothProfile.ServiceListener() {
        public void onServiceConnected(int profile, BluetoothProfile proxy) {
            Log.d(TAG, "onServiceConnected");
            if (profile == BluetoothProfile.AVRCP_CONTROLLER) {
                mAvrcpCommandQueue.obtainMessage(MSG_PROFILE_CONNECT, proxy).sendToTarget();
                List<BluetoothDevice> devices = proxy.getConnectedDevices();
                if (devices != null && devices.size() > 0) {
                    BluetoothDevice device = devices.get(0);
                    Log.d(TAG, "got AVRCP device " + device);
                }
            }
        }

        public void onServiceDisconnected(int profile) {
            Log.d(TAG, "onServiceDisconnected " + profile);
            if (profile == BluetoothProfile.AVRCP_CONTROLLER) {
                mAvrcpProfile = null;
                mAvrcpCommandQueue.obtainMessage(MSG_PROFILE_DISCONNECT).sendToTarget();
            }
        }
    };

    // Media Session Stuff.
    private MediaSession.Callback mSessionCallbacks = new MediaSession.Callback() {
        @Override
        public void onPlay() {
            Log.d(TAG, "onPlay");
            mAvrcpCommandQueue.obtainMessage(
                MSG_AVRCP_PASSTHRU, BluetoothAvrcpController.PASS_THRU_CMD_ID_PLAY).sendToTarget();
            // TRACK_EVENT should be fired eventually and the UI should be hence updated.
        }

        @Override
        public void onPause() {
            Log.d(TAG, "onPause");
            mAvrcpCommandQueue.obtainMessage(
                MSG_AVRCP_PASSTHRU, BluetoothAvrcpController.PASS_THRU_CMD_ID_PAUSE).sendToTarget();
            // TRACK_EVENT should be fired eventually and the UI should be hence updated.
        }

        @Override
        public void onSkipToNext() {
            Log.d(TAG, "onSkipToNext");
            mAvrcpCommandQueue.obtainMessage(
                MSG_AVRCP_PASSTHRU, BluetoothAvrcpController.PASS_THRU_CMD_ID_FORWARD)
                .sendToTarget();
            // TRACK_EVENT should be fired eventually and the UI should be hence updated.
        }

        @Override
        public void onSkipToPrevious() {
            Log.d(TAG, "onSkipToPrevious");

            mAvrcpCommandQueue.obtainMessage(
                MSG_AVRCP_PASSTHRU, BluetoothAvrcpController.PASS_THRU_CMD_ID_BACKWARD)
                .sendToTarget();
            // TRACK_EVENT should be fired eventually and the UI should be hence updated.
        }

        // These are not yet supported.
        @Override
        public void onStop() {
            Log.d(TAG, "onStop");
        }

        @Override
        public void onCustomAction(String action, Bundle extras) {
            Log.d(TAG, "onCustomAction action=" + action + " extras=" + extras);
        }

        @Override
        public void onPlayFromSearch(String query, Bundle extras) {
            Log.d(TAG, "playFromSearch not supported in AVRCP");
        }

        @Override
        public void onCommand(String command, Bundle args, ResultReceiver cb) {
            Log.d(TAG, "onCommand command=" + command + " args=" + args);
        }

        @Override
        public void onSkipToQueueItem(long queueId) {
            Log.d(TAG, "onSkipToQueueItem");
        }

        @Override
        public void onPlayFromMediaId(String mediaId, Bundle extras) {
            Log.d(TAG, "onPlayFromMediaId mediaId=" + mediaId + " extras=" + extras);
        }

    };

    private BroadcastReceiver mBtReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "onReceive intent=" + intent);
            String action = intent.getAction();
            BluetoothDevice btDev =
                    (BluetoothDevice) intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
            int state = intent.getIntExtra(BluetoothProfile.EXTRA_STATE, -1);

            if (BluetoothAvrcpController.ACTION_CONNECTION_STATE_CHANGED.equals(action)) {
                Log.d(TAG, "handleConnectionStateChange: newState="
                        + state + " btDev=" + btDev);

                // Connected state will be handled when AVRCP BluetoothProfile gets connected.
                if (state == BluetoothProfile.STATE_CONNECTED) {
                    mAvrcpCommandQueue.obtainMessage(MSG_DEVICE_CONNECT, btDev).sendToTarget();
                } else if (state == BluetoothProfile.STATE_DISCONNECTED) {
                    // Set the playback state to unconnected.
                    mAvrcpCommandQueue.obtainMessage(MSG_DEVICE_DISCONNECT, btDev).sendToTarget();
                }
            } else if (BluetoothAvrcpController.ACTION_TRACK_EVENT.equals(action)) {
                PlaybackState pbb =
                    intent.getParcelableExtra(BluetoothAvrcpController.EXTRA_PLAYBACK);
                MediaMetadata mmd =
                    intent.getParcelableExtra(BluetoothAvrcpController.EXTRA_METADATA);
                mAvrcpCommandQueue.obtainMessage(
                    MSG_TRACK, new Pair<PlaybackState, MediaMetadata>(pbb, mmd)).sendToTarget();
            }
        }
    };

    private void msgDeviceConnect(BluetoothDevice device) {
        Log.d(TAG, "msgDeviceConnect");
        // We are connected to a new device via A2DP now.
        mA2dpDevice = device;
        refreshInitialPlayingState();
    }

    private void msgProfileConnect(BluetoothProfile profile) {
        Log.d(TAG, "msgProfileConnect");
        if (profile != null) {
            mAvrcpProfile = (BluetoothAvrcpController) profile;
        }
        refreshInitialPlayingState();
    }

    // Refresh the UI if we have a connected device and AVRCP is initialized.
    private void refreshInitialPlayingState() {
        if (mAvrcpProfile == null || mA2dpDevice == null) {
            Log.d(TAG, "AVRCP Profile " + mAvrcpProfile + " device " + mA2dpDevice);
            return;
        }

        List<BluetoothDevice> devices = mAvrcpProfile.getConnectedDevices();
        if (devices.size() == 0) {
            Log.w(TAG, "No devices connected yet");
            return;
        }

        if (mA2dpDevice != null && !mA2dpDevice.equals(devices.get(0))) {
            Log.e(TAG, "A2dp device : " + mA2dpDevice + " avrcp device " + devices.get(0));
        }
        mA2dpDevice = devices.get(0);

        PlaybackState playbackState = mAvrcpProfile.getPlaybackState(mA2dpDevice);
        // Add actions required for playback and rebuild the object.
        PlaybackState.Builder pbb = new PlaybackState.Builder(playbackState);
        playbackState = pbb.setActions(mTransportControlFlags).build();

        MediaMetadata mediaMetadata = mAvrcpProfile.getMetadata(mA2dpDevice);
        Log.d(TAG, "Media metadata " + mediaMetadata + " playback state " + playbackState);
        mSession.setMetadata(mAvrcpProfile.getMetadata(mA2dpDevice));
        mSession.setPlaybackState(playbackState);
    }

    private void msgDeviceDisconnect(BluetoothDevice device) {
        Log.d(TAG, "msgDeviceDisconnect");
        if (mA2dpDevice == null) {
            Log.w(TAG, "Already disconnected - nothing to do here.");
            return;
        } else if (!mA2dpDevice.equals(device)) {
            Log.e(TAG, "Not the right device to disconnect current " +
                mA2dpDevice + " dc " + device);
            return;
        }

        // Unset the session.
        PlaybackState.Builder pbb = new PlaybackState.Builder();
        pbb = pbb.setState(PlaybackState.STATE_ERROR, PlaybackState.PLAYBACK_POSITION_UNKNOWN,
                    PLAYBACK_SPEED)
                .setActions(mTransportControlFlags)
                .setErrorMessage(getString(R.string.bluetooth_disconnected));
        mSession.setPlaybackState(pbb.build());
    }

    private void msgProfileDisconnect() {
        Log.d(TAG, "msgProfileDisconnect");
        // The profile is disconnected - even if the device is still connected we cannot really have
        // a functioning UI so reset the session.
        mAvrcpProfile = null;

        // Unset the session.
        PlaybackState.Builder pbb = new PlaybackState.Builder();
        pbb = pbb.setState(PlaybackState.STATE_ERROR, PlaybackState.PLAYBACK_POSITION_UNKNOWN,
                    PLAYBACK_SPEED)
                .setActions(mTransportControlFlags)
                .setErrorMessage(getString(R.string.bluetooth_disconnected));
        mSession.setPlaybackState(pbb.build());
    }

    private void msgTrack(PlaybackState pb, MediaMetadata mmd) {
        Log.d(TAG, "msgTrack: playback: " + pb + " mmd: " + mmd);
        // Log the current track position/content.
        MediaController controller = mSession.getController();
        PlaybackState prevPS = controller.getPlaybackState();
        MediaMetadata prevMM = controller.getMetadata();

        if (prevPS != null) {
            Log.d(TAG, "prevPS " + prevPS);
        }

        if (prevMM != null) {
            String title = prevMM.getString(MediaMetadata.METADATA_KEY_TITLE);
            long trackLen = prevMM.getLong(MediaMetadata.METADATA_KEY_DURATION);
            Log.d(TAG, "prev MM title " + title + " track len " + trackLen);
        }

        if (mmd != null) {
            Log.d(TAG, "msgTrack() mmd " + mmd.getDescription());
            mSession.setMetadata(mmd);
        }

        if (pb != null) {
            Log.d(TAG, "msgTrack() playbackstate " + pb);
            PlaybackState.Builder pbb = new PlaybackState.Builder(pb);
            pb = pbb.setActions(mTransportControlFlags).build();
            mSession.setPlaybackState(pb);
        }
    }

    private void msgPassThru(int cmd) {
        Log.d(TAG, "msgPassThru " + cmd);
        if (mA2dpDevice == null) {
            // We should have already disconnected - ignore this message.
            Log.e(TAG, "Already disconnected ignoring.");
            return;
        }

        if (mAvrcpProfile == null) {
            // We may be disconnected with the profile but there is not much we can do for now but
            // to wait for the profile to come back up.
            Log.e(TAG, "Profile disconnected; ignoring.");
            return;
        }

        // Send the pass through.
        mAvrcpProfile.sendPassThroughCmd(
            mA2dpDevice, cmd, BluetoothAvrcpController.KEY_STATE_PRESSED);
        mAvrcpProfile.sendPassThroughCmd(
            mA2dpDevice, cmd, BluetoothAvrcpController.KEY_STATE_RELEASED);
    }
}
