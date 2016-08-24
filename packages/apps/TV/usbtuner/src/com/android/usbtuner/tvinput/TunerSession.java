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

import android.annotation.TargetApi;
import android.content.Context;
import android.media.PlaybackParams;
import android.media.tv.TvContentRating;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputService;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.text.Html;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.exoplayer.audio.AudioCapabilities;
import com.android.usbtuner.R;
import com.android.usbtuner.cc.CaptionLayout;
import com.android.usbtuner.cc.CaptionTrackRenderer;
import com.android.usbtuner.data.Cea708Data.CaptionEvent;
import com.android.usbtuner.data.Track.AtscCaptionTrack;
import com.android.usbtuner.data.TunerChannel;
import com.android.usbtuner.exoplayer.cache.CacheManager;
import com.android.usbtuner.util.StatusTextUtils;
import com.android.usbtuner.util.SystemPropertiesProxy;

/**
 * Provides a USB tuner TV input session. It handles Overlay UI works. Main tuner input functions
 * are implemented in {@link TunerSessionWorker}.
 */
public class TunerSession extends TvInputService.Session implements Handler.Callback {
    private static final String TAG = "TunerSession";
    private static final boolean DEBUG = false;
    private static final String USBTUNER_SHOW_DEBUG = "persist.usbtuner.show_debug";

    public static final int MSG_UI_SHOW_MESSAGE = 1;
    public static final int MSG_UI_HIDE_MESSAGE = 2;
    public static final int MSG_UI_SHOW_AUDIO_UNPLAYABLE = 3;
    public static final int MSG_UI_HIDE_AUDIO_UNPLAYABLE = 4;
    public static final int MSG_UI_PROCESS_CAPTION_TRACK = 5;
    public static final int MSG_UI_START_CAPTION_TRACK = 6;
    public static final int MSG_UI_STOP_CAPTION_TRACK = 7;
    public static final int MSG_UI_RESET_CAPTION_TRACK = 8;
    public static final int MSG_UI_SET_STATUS_TEXT = 9;
    public static final int MSG_UI_TOAST_RESCAN_NEEDED = 10;

    private final Context mContext;
    private final Handler mUiHandler;
    private final View mOverlayView;
    private final TextView mMessageView;
    private final TextView mStatusView;
    private final TextView mAudioStatusView;
    private final ViewGroup mMessageLayout;
    private final CaptionTrackRenderer mCaptionTrackRenderer;
    private final TunerSessionWorker mSessionWorker;
    private boolean mReleased = false;
    private boolean mVideoAvailable = false;
    private boolean mPlayPaused;
    private long mTuneStartTimestamp;

    public TunerSession(Context context, ChannelDataManager channelDataManager,
            CacheManager cacheManager) {
        super(context);
        mContext = context;
        mUiHandler = new Handler(this);
        LayoutInflater inflater = (LayoutInflater)
                context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mOverlayView = inflater.inflate(R.layout.ut_overlay_view, null);
        mMessageLayout = (ViewGroup) mOverlayView.findViewById(R.id.message_layout);
        mMessageLayout.setVisibility(View.INVISIBLE);
        mMessageView = (TextView) mOverlayView.findViewById(R.id.message);
        mStatusView = (TextView) mOverlayView.findViewById(R.id.tuner_status);
        boolean showDebug = SystemPropertiesProxy.getBoolean(USBTUNER_SHOW_DEBUG, false);
        mStatusView.setVisibility(showDebug ? View.VISIBLE : View.INVISIBLE);
        mAudioStatusView = (TextView) mOverlayView.findViewById(R.id.audio_status);
        mAudioStatusView.setVisibility(View.INVISIBLE);
        mAudioStatusView.setText(Html.fromHtml(StatusTextUtils.getAudioWarningInHTML(
                context.getString(R.string.ut_ac3_passthrough_unavailable))));
        CaptionLayout captionLayout = (CaptionLayout) mOverlayView.findViewById(R.id.caption);
        mCaptionTrackRenderer = new CaptionTrackRenderer(captionLayout);
        mSessionWorker = new TunerSessionWorker(context, channelDataManager,
                cacheManager, this);
    }

    public boolean isReleased() {
        return mReleased;
    }

    @Override
    public View onCreateOverlayView() {
        return mOverlayView;
    }

    @Override
    public boolean onSelectTrack(int type, String trackId) {
        mSessionWorker.sendMessage(
                TunerSessionWorker.MSG_SELECT_TRACK, type, 0, trackId);
        return false;
    }

    @Override
    public void onSetCaptionEnabled(boolean enabled) {
        mSessionWorker.sendMessage(
                TunerSessionWorker.MSG_SET_CAPTION_ENABLED, enabled);
    }

    @Override
    public void onSetStreamVolume(float volume) {
        mSessionWorker.sendMessage(TunerSessionWorker.MSG_SET_STREAM_VOLUME, volume);
    }

    @Override
    public boolean onSetSurface(Surface surface) {
        mSessionWorker.sendMessage(TunerSessionWorker.MSG_SET_SURFACE, surface);
        return true;
    }

    @Override
    public void onTimeShiftPause() {
        mSessionWorker.sendMessage(TunerSessionWorker.MSG_TIMESHIFT_PAUSE);
        mPlayPaused = true;
    }

    @Override
    public void onTimeShiftResume() {
        mSessionWorker.sendMessage(TunerSessionWorker.MSG_TIMESHIFT_RESUME);
        mPlayPaused = false;
    }

    @Override
    public void onTimeShiftSeekTo(long timeMs) {
        if (DEBUG) Log.d(TAG, "Timeshift seekTo requested position: " + timeMs / 1000);
        mSessionWorker.sendMessage(TunerSessionWorker.MSG_TIMESHIFT_SEEK_TO,
                mPlayPaused ? 1 : 0, 0, timeMs);
    }

    @Override
    public void onTimeShiftSetPlaybackParams(PlaybackParams params) {
        mSessionWorker.sendMessage(
                TunerSessionWorker.MSG_TIMESHIFT_SET_PLAYBACKPARAMS, params);
    }

    @Override
    public long onTimeShiftGetStartPosition() {
        return mSessionWorker.getStartPosition();
    }

    @Override
    public long onTimeShiftGetCurrentPosition() {
        return mSessionWorker.getCurrentPosition();
    }

    @Override
    public boolean onTune(Uri channelUri) {
        if (DEBUG) {
            Log.d(TAG, "onTune to " + channelUri != null ? channelUri.toString() : "");
        }
        if (channelUri == null) {
            Log.w(TAG, "onTune() is failed due to null channelUri.");
            mSessionWorker.stopTune();
            return false;
        }
        mTuneStartTimestamp = SystemClock.elapsedRealtime();
        mSessionWorker.tune(channelUri);
        mPlayPaused = false;
        return true;
    }

    @TargetApi(Build.VERSION_CODES.N)
    @Override
    public void onTimeShiftPlay(Uri recordUri) {
        if (recordUri == null) {
            Log.w(TAG, "onTimeShiftPlay() is failed due to null channelUri.");
            mSessionWorker.stopTune();
            return;
        }
        mTuneStartTimestamp = SystemClock.elapsedRealtime();
        mSessionWorker.tune(recordUri);
        mPlayPaused = false;
    }

    @Override
    public void onUnblockContent(TvContentRating unblockedRating) {
        mSessionWorker.sendMessage(TunerSessionWorker.MSG_UNBLOCKED_RATING,
                unblockedRating);
    }

    @Override
    public void onRelease() {
        if (DEBUG) {
            Log.d(TAG, "onRelease");
        }
        mReleased = true;
        mSessionWorker.release();
        mUiHandler.removeCallbacksAndMessages(null);
    }

    /**
     * Sets {@link AudioCapabilities}.
     */
    public void setAudioCapabilities(AudioCapabilities audioCapabilities) {
        mSessionWorker.sendMessage(TunerSessionWorker.MSG_AUDIO_CAPABILITIES_CHANGED,
                audioCapabilities);
    }

    @Override
    public void notifyVideoAvailable() {
        super.notifyVideoAvailable();
        if (mTuneStartTimestamp != 0) {
            Log.i(TAG, "[Profiler] Video available in "
                    + (SystemClock.elapsedRealtime() - mTuneStartTimestamp) + " ms");
            mTuneStartTimestamp = 0;
        }
        mVideoAvailable = true;
    }

    @Override
    public void notifyVideoUnavailable(int reason) {
        switch (reason) {
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_TUNING:
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_BUFFERING:
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_AUDIO_ONLY:
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_WEAK_SIGNAL:
                super.notifyVideoUnavailable(reason);
                break;
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN:
            default:
                super.notifyVideoAvailable();
                TunerChannel channel = mSessionWorker.getCurrentChannel();
                if (channel != null) {
                    sendUiMessage(TunerSession.MSG_UI_SHOW_MESSAGE,
                            mContext.getString(R.string.ut_fail_to_tune, channel.getName()));
                } else {
                    sendUiMessage(TunerSession.MSG_UI_SHOW_MESSAGE,
                            mContext.getString(R.string.ut_fail_to_tune_to_unknown_channel));
                }
                break;
        }
        if (reason != TvInputManager.VIDEO_UNAVAILABLE_REASON_BUFFERING
                && reason != TvInputManager.VIDEO_UNAVAILABLE_REASON_WEAK_SIGNAL) {
            notifyTimeShiftStatusChanged(TvInputManager.TIME_SHIFT_STATUS_UNAVAILABLE);
        }
    }

    public void sendUiMessage(int message) {
        mUiHandler.sendEmptyMessage(message);
    }

    public void sendUiMessage(int message, Object object) {
        mUiHandler.obtainMessage(message, object).sendToTarget();
    }

    public void sendUiMessage(int message, int arg1, int arg2, Object object) {
        mUiHandler.obtainMessage(message, arg1, arg2, object).sendToTarget();
    }

    @Override
    public boolean handleMessage(Message msg) {
        switch (msg.what) {
            case MSG_UI_SHOW_MESSAGE: {
                mMessageView.setText((String) msg.obj);
                mMessageLayout.setVisibility(View.VISIBLE);
                return true;
            }
            case MSG_UI_HIDE_MESSAGE: {
                mMessageLayout.setVisibility(View.INVISIBLE);
                return true;
            }
            case MSG_UI_SHOW_AUDIO_UNPLAYABLE: {
                mAudioStatusView.setVisibility(View.VISIBLE);
                return true;
            }
            case MSG_UI_HIDE_AUDIO_UNPLAYABLE: {
                mAudioStatusView.setVisibility(View.INVISIBLE);
                return true;
            }
            case MSG_UI_PROCESS_CAPTION_TRACK: {
                mCaptionTrackRenderer.processCaptionEvent((CaptionEvent) msg.obj);
                return true;
            }
            case MSG_UI_START_CAPTION_TRACK: {
                mCaptionTrackRenderer.start((AtscCaptionTrack) msg.obj);
                return true;
            }
            case MSG_UI_STOP_CAPTION_TRACK: {
                mCaptionTrackRenderer.stop();
                return true;
            }
            case MSG_UI_RESET_CAPTION_TRACK: {
                mCaptionTrackRenderer.reset();
                return true;
            }
            case MSG_UI_SET_STATUS_TEXT: {
                mStatusView.setText((CharSequence) msg.obj);
                return true;
            }
            case MSG_UI_TOAST_RESCAN_NEEDED: {
                Toast.makeText(mContext, R.string.ut_rescan_needed, Toast.LENGTH_LONG).show();
                return true;
            }
        }
        return false;
    }
}
