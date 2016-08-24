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

package com.android.tv.ui;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.TimeInterpolator;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.ContentUris;
import android.content.Context;
import android.content.pm.PackageManager;
import android.media.PlaybackParams;
import android.media.tv.TvContentRating;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvTrackInfo;
import android.media.tv.TvView;
import android.media.tv.TvView.OnUnhandledInputEventListener;
import android.media.tv.TvView.TvInputCallback;
import android.net.ConnectivityManager;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.support.annotation.UiThread;
import android.support.v4.os.BuildCompat;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;

import com.android.tv.ApplicationSingletons;
import com.android.tv.R;
import com.android.tv.TvApplication;
import com.android.tv.analytics.DurationTimer;
import com.android.tv.analytics.Tracker;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.common.recording.RecordedProgram;
import com.android.tv.data.Channel;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.StreamInfo;
import com.android.tv.data.WatchedHistoryManager;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.parental.ContentRatingsManager;
import com.android.tv.recommendation.NotificationService;
import com.android.tv.util.NetworkUtils;
import com.android.tv.util.PermissionUtils;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.util.Utils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;

public class TunableTvView extends FrameLayout implements StreamInfo {
    private static final boolean DEBUG = false;
    private static final String TAG = "TunableTvView";

    public static final int VIDEO_UNAVAILABLE_REASON_NOT_TUNED = -1;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({BLOCK_SCREEN_TYPE_NO_UI, BLOCK_SCREEN_TYPE_SHRUNKEN_TV_VIEW, BLOCK_SCREEN_TYPE_NORMAL})
    public @interface BlockScreenType {}
    public static final int BLOCK_SCREEN_TYPE_NO_UI = 0;
    public static final int BLOCK_SCREEN_TYPE_SHRUNKEN_TV_VIEW = 1;
    public static final int BLOCK_SCREEN_TYPE_NORMAL = 2;

    private static final String PERMISSION_RECEIVE_INPUT_EVENT =
            "com.android.tv.permission.RECEIVE_INPUT_EVENT";

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ TIME_SHIFT_STATE_NONE, TIME_SHIFT_STATE_PLAY, TIME_SHIFT_STATE_PAUSE,
            TIME_SHIFT_STATE_REWIND, TIME_SHIFT_STATE_FAST_FORWARD })
    private @interface TimeShiftState {}
    private static final int TIME_SHIFT_STATE_NONE = 0;
    private static final int TIME_SHIFT_STATE_PLAY = 1;
    private static final int TIME_SHIFT_STATE_PAUSE = 2;
    private static final int TIME_SHIFT_STATE_REWIND = 3;
    private static final int TIME_SHIFT_STATE_FAST_FORWARD = 4;

    private static final int FADED_IN = 0;
    private static final int FADED_OUT = 1;
    private static final int FADING_IN = 2;
    private static final int FADING_OUT = 3;

    private static final long INVALID_TIME = -1;

    // It is too small to see the description text without PIP_BLOCK_SCREEN_SCALE_FACTOR.
    private static final float PIP_BLOCK_SCREEN_SCALE_FACTOR = 1.2f;

    private AppLayerTvView mTvView;
    private Channel mCurrentChannel;
    private RecordedProgram mRecordedProgram;
    private TvInputManagerHelper mInputManagerHelper;
    private ContentRatingsManager mContentRatingsManager;
    @Nullable
    private WatchedHistoryManager mWatchedHistoryManager;
    private boolean mStarted;
    private TvInputInfo mInputInfo;
    private OnTuneListener mOnTuneListener;
    private int mVideoWidth;
    private int mVideoHeight;
    private int mVideoFormat = StreamInfo.VIDEO_DEFINITION_LEVEL_UNKNOWN;
    private float mVideoFrameRate;
    private float mVideoDisplayAspectRatio;
    private int mAudioChannelCount = StreamInfo.AUDIO_CHANNEL_COUNT_UNKNOWN;
    private boolean mHasClosedCaption = false;
    private boolean mVideoAvailable;
    private boolean mScreenBlocked;
    private OnScreenBlockingChangedListener mOnScreenBlockedListener;
    private TvContentRating mBlockedContentRating;
    private int mVideoUnavailableReason = VIDEO_UNAVAILABLE_REASON_NOT_TUNED;
    private boolean mCanReceiveInputEvent;
    private boolean mIsMuted;
    private float mVolume;
    private boolean mParentControlEnabled;
    private int mFixedSurfaceWidth;
    private int mFixedSurfaceHeight;
    private boolean mIsPip;
    private int mScreenHeight;
    private int mShrunkenTvViewHeight;
    private final boolean mCanModifyParentalControls;

    @TimeShiftState private int mTimeShiftState = TIME_SHIFT_STATE_NONE;
    private TimeShiftListener mTimeShiftListener;
    private boolean mTimeShiftAvailable;
    private long mTimeShiftCurrentPositionMs = INVALID_TIME;

    private final Tracker mTracker;
    private final DurationTimer mChannelViewTimer = new DurationTimer();
    private InternetCheckTask mInternetCheckTask;

    // A block screen view which has lock icon with black background.
    // This indicates that user's action is needed to play video.
    private final BlockScreenView mBlockScreenView;

    // A View to hide screen when there's problem in video playback.
    private final BlockScreenView mHideScreenView;

    // A View to block screen until onContentAllowed is received if parental control is on.
    private final View mBlockScreenForTuneView;

    // A spinner view to show buffering status.
    private final View mBufferingSpinnerView;

    // A View for fade-in/out animation
    private final View mDimScreenView;
    private int mFadeState = FADED_IN;
    private Runnable mActionAfterFade;

    @BlockScreenType private int mBlockScreenType;

    private final DvrDataManager mDvrDataManager;
    private final ChannelDataManager mChannelDataManager;
    private final ConnectivityManager mConnectivityManager;

    private final TvInputCallback mCallback =
            new TvInputCallback() {
                @Override
                public void onConnectionFailed(String inputId) {
                    Log.w(TAG, "Failed to bind an input");
                    mTracker.sendInputConnectionFailure(inputId);
                    Channel channel = mCurrentChannel;
                    mCurrentChannel = null;
                    mInputInfo = null;
                    mCanReceiveInputEvent = false;
                    if (mOnTuneListener != null) {
                        // If tune is called inside onTuneFailed, mOnTuneListener will be set to
                        // a new instance. In order to avoid to clear the new mOnTuneListener,
                        // we copy mOnTuneListener to l and clear mOnTuneListener before
                        // calling onTuneFailed.
                        OnTuneListener listener = mOnTuneListener;
                        mOnTuneListener = null;
                        listener.onTuneFailed(channel);
                    }
                }

                @Override
                public void onDisconnected(String inputId) {
                    Log.w(TAG, "Session is released by crash");
                    mTracker.sendInputDisconnected(inputId);
                    Channel channel = mCurrentChannel;
                    mCurrentChannel = null;
                    mInputInfo = null;
                    mCanReceiveInputEvent = false;
                    if (mOnTuneListener != null) {
                        OnTuneListener listener = mOnTuneListener;
                        mOnTuneListener = null;
                        listener.onUnexpectedStop(channel);
                    }
                }

                @Override
                public void onChannelRetuned(String inputId, Uri channelUri) {
                    if (DEBUG) {
                        Log.d(TAG, "onChannelRetuned(inputId=" + inputId + ", channelUri="
                                + channelUri + ")");
                    }
                    if (mOnTuneListener != null) {
                        mOnTuneListener.onChannelRetuned(channelUri);
                    }
                }

                @Override
                public void onTracksChanged(String inputId, List<TvTrackInfo> tracks) {
                    mHasClosedCaption = false;
                    for (TvTrackInfo track : tracks) {
                        if (track.getType() == TvTrackInfo.TYPE_SUBTITLE) {
                            mHasClosedCaption = true;
                            break;
                        }
                    }
                    if (mOnTuneListener != null) {
                        mOnTuneListener.onStreamInfoChanged(TunableTvView.this);
                    }
                }

                @Override
                public void onTrackSelected(String inputId, int type, String trackId) {
                    if (trackId == null) {
                        // A track is unselected.
                        if (type == TvTrackInfo.TYPE_VIDEO) {
                            mVideoWidth = 0;
                            mVideoHeight = 0;
                            mVideoFormat = StreamInfo.VIDEO_DEFINITION_LEVEL_UNKNOWN;
                            mVideoFrameRate = 0f;
                            mVideoDisplayAspectRatio = 0f;
                        } else if (type == TvTrackInfo.TYPE_AUDIO) {
                            mAudioChannelCount = StreamInfo.AUDIO_CHANNEL_COUNT_UNKNOWN;
                        }
                    } else {
                        List<TvTrackInfo> tracks = getTracks(type);
                        boolean trackFound = false;
                        if (tracks != null) {
                            for (TvTrackInfo track : tracks) {
                                if (track.getId().equals(trackId)) {
                                    if (type == TvTrackInfo.TYPE_VIDEO) {
                                        mVideoWidth = track.getVideoWidth();
                                        mVideoHeight = track.getVideoHeight();
                                        mVideoFormat = Utils.getVideoDefinitionLevelFromSize(
                                                mVideoWidth, mVideoHeight);
                                        mVideoFrameRate = track.getVideoFrameRate();
                                        if (mVideoWidth <= 0 || mVideoHeight <= 0) {
                                            mVideoDisplayAspectRatio = 0.0f;
                                        } else if (android.os.Build.VERSION.SDK_INT >=
                                                android.os.Build.VERSION_CODES.M) {
                                            float VideoPixelAspectRatio =
                                                    track.getVideoPixelAspectRatio();
                                            mVideoDisplayAspectRatio = VideoPixelAspectRatio
                                                    * mVideoWidth / mVideoHeight;
                                        } else {
                                            mVideoDisplayAspectRatio = mVideoWidth
                                                    / (float) mVideoHeight;
                                        }
                                    } else if (type == TvTrackInfo.TYPE_AUDIO) {
                                        mAudioChannelCount = track.getAudioChannelCount();
                                    }
                                    trackFound = true;
                                    break;
                                }
                            }
                        }
                        if (!trackFound) {
                            Log.w(TAG, "Invalid track ID: " + trackId);
                        }
                    }
                    if (mOnTuneListener != null) {
                        mOnTuneListener.onStreamInfoChanged(TunableTvView.this);
                    }
                }

                @Override
                public void onVideoAvailable(String inputId) {
                    unhideScreenByVideoAvailability();
                    if (mOnTuneListener != null) {
                        mOnTuneListener.onStreamInfoChanged(TunableTvView.this);
                    }
                }

                @Override
                public void onVideoUnavailable(String inputId, int reason) {
                    hideScreenByVideoAvailability(reason);
                    if (mOnTuneListener != null) {
                        mOnTuneListener.onStreamInfoChanged(TunableTvView.this);
                    }
                    switch (reason) {
                        case TvInputManager.VIDEO_UNAVAILABLE_REASON_BUFFERING:
                        case TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN:
                        case TvInputManager.VIDEO_UNAVAILABLE_REASON_WEAK_SIGNAL:
                            mTracker.sendChannelVideoUnavailable(mCurrentChannel, reason);
                        default:
                            // do nothing
                    }
                }

                @Override
                public void onContentAllowed(String inputId) {
                    mBlockScreenForTuneView.setVisibility(View.GONE);
                    unblockScreenByContentRating();
                    if (mOnTuneListener != null) {
                        mOnTuneListener.onContentAllowed();
                    }
                }

                @Override
                public void onContentBlocked(String inputId, TvContentRating rating) {
                    blockScreenByContentRating(rating);
                    if (mOnTuneListener != null) {
                        mOnTuneListener.onContentBlocked();
                    }
                }

                @Override
                @TargetApi(Build.VERSION_CODES.M)
                public void onTimeShiftStatusChanged(String inputId, int status) {
                    boolean available = status == TvInputManager.TIME_SHIFT_STATUS_AVAILABLE;
                    setTimeShiftAvailable(available);
                }
            };

    public TunableTvView(Context context) {
        this(context, null);
    }

    public TunableTvView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public TunableTvView(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public TunableTvView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        inflate(getContext(), R.layout.tunable_tv_view, this);

        ApplicationSingletons appSingletons = TvApplication.getSingletons(context);
        mDvrDataManager = CommonFeatures.DVR.isEnabled(context) && BuildCompat.isAtLeastN()
                ? appSingletons.getDvrDataManager()
                : null;
        mChannelDataManager = appSingletons.getChannelDataManager();
        mConnectivityManager = (ConnectivityManager) context
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        mCanModifyParentalControls = PermissionUtils.hasModifyParentalControls(context);
        mTracker = appSingletons.getTracker();
        mBlockScreenType = BLOCK_SCREEN_TYPE_NORMAL;
        mBlockScreenView = (BlockScreenView) findViewById(R.id.block_screen);
        if (!mCanModifyParentalControls) {
            mBlockScreenView.setImage(R.drawable.ic_message_lock_no_permission);
            mBlockScreenView.setScaleType(ImageView.ScaleType.CENTER);
        } else {
            mBlockScreenView.setImage(R.drawable.ic_message_lock);
        }
        mBlockScreenView.setShrunkenImage(R.drawable.ic_message_lock_preview);
        mBlockScreenView.addFadeOutAnimationListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                adjustBlockScreenSpacingAndText();
            }
        });

        mHideScreenView = (BlockScreenView) findViewById(R.id.hide_screen);
        mHideScreenView.setImageVisibility(false);
        mBufferingSpinnerView = findViewById(R.id.buffering_spinner);
        mBlockScreenForTuneView = findViewById(R.id.block_screen_for_tune);
        mDimScreenView = findViewById(R.id.dim);
        mDimScreenView.animate().setListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                if (mActionAfterFade != null) {
                    mActionAfterFade.run();
                }
            }

            @Override
            public void onAnimationCancel(Animator animation) {
                if (mActionAfterFade != null) {
                    mActionAfterFade.run();
                }
            }
        });
    }

    public void initialize(AppLayerTvView tvView, boolean isPip, int screenHeight,
            int shrunkenTvViewHeight) {
        mTvView = tvView;
        mIsPip = isPip;
        mScreenHeight = screenHeight;
        mShrunkenTvViewHeight = shrunkenTvViewHeight;
        mTvView.setZOrderOnTop(isPip);
        copyLayoutParamsToTvView();
    }

    public void start(TvInputManagerHelper tvInputManagerHelper) {
        mInputManagerHelper = tvInputManagerHelper;
        mContentRatingsManager = tvInputManagerHelper.getContentRatingsManager();
        if (mStarted) {
            return;
        }
        mStarted = true;
    }

    public void stop() {
        if (!mStarted) {
            return;
        }
        mStarted = false;
        if (mCurrentChannel != null) {
            long duration = mChannelViewTimer.reset();
            mTracker.sendChannelViewStop(mCurrentChannel, duration);
            if (mWatchedHistoryManager != null && !mCurrentChannel.isPassthrough()) {
                mWatchedHistoryManager.logChannelViewStop(mCurrentChannel,
                        System.currentTimeMillis(), duration);
            }
        }
        reset();
    }

    public void reset() {
        mTvView.reset();
        mCurrentChannel = null;
        mRecordedProgram = null;
        mInputInfo = null;
        mCanReceiveInputEvent = false;
        mOnTuneListener = null;
        setTimeShiftAvailable(false);
        hideScreenByVideoAvailability(VIDEO_UNAVAILABLE_REASON_NOT_TUNED);
    }

    public void setMain() {
        mTvView.setMain();
    }

    public void setWatchedHistoryManager(WatchedHistoryManager watchedHistoryManager) {
        mWatchedHistoryManager = watchedHistoryManager;
    }

    public boolean isPlaying() {
        return mStarted;
    }

    /**
     * Called when parental control is changed.
     */
    public void onParentalControlChanged(boolean enabled) {
        mParentControlEnabled = enabled;
        if (!mParentControlEnabled) {
            mBlockScreenForTuneView.setVisibility(View.GONE);
        }
    }

    /**
     * Returns {@code true}, if this view is the recording playback mode.
     */
    public boolean isRecordingPlayback() {
        return mRecordedProgram != null;
    }

    /**
     * Returns the recording which is being played right now.
     */
    public RecordedProgram getPlayingRecordedProgram() {
        return mRecordedProgram;
    }

    /**
     * Plays a recording.
     */
    public boolean playRecording(Uri recordingUri, OnTuneListener listener) {
        if (!mStarted) {
            throw new IllegalStateException("TvView isn't started");
        }
        if (!CommonFeatures.DVR.isEnabled(getContext()) || !BuildCompat.isAtLeastN()) {
            return false;
        }
        if (DEBUG) Log.d(TAG, "playRecording " + recordingUri);
        long recordingId = ContentUris.parseId(recordingUri);
        mRecordedProgram = mDvrDataManager.getRecordedProgram(recordingId);
        if (mRecordedProgram == null) {
            Log.w(TAG, "No recorded program (Uri=" + recordingUri + ")");
            return false;
        }
        String inputId = mRecordedProgram.getInputId();
        TvInputInfo inputInfo = mInputManagerHelper.getTvInputInfo(inputId);
        if (inputInfo == null) {
            return false;
        }
        mOnTuneListener = listener;
        // mCurrentChannel can be null.
        mCurrentChannel = mChannelDataManager.getChannel(mRecordedProgram.getChannelId());
        // For recording playback, input event should not be sent.
        mCanReceiveInputEvent = false;
        boolean needSurfaceSizeUpdate = false;
        if (!inputInfo.equals(mInputInfo)) {
            mInputInfo = inputInfo;
            if (DEBUG) {
                Log.d(TAG, "Input \'" + mInputInfo.getId() + "\' can receive input event: "
                        + mCanReceiveInputEvent);
            }
            needSurfaceSizeUpdate = true;
        }
        mChannelViewTimer.start();
        mVideoWidth = 0;
        mVideoHeight = 0;
        mVideoFormat = StreamInfo.VIDEO_DEFINITION_LEVEL_UNKNOWN;
        mVideoFrameRate = 0f;
        mVideoDisplayAspectRatio = 0f;
        mAudioChannelCount = StreamInfo.AUDIO_CHANNEL_COUNT_UNKNOWN;
        mHasClosedCaption = false;
        mTvView.setCallback(mCallback);
        mTimeShiftCurrentPositionMs = INVALID_TIME;
        mTvView.setTimeShiftPositionCallback(null);
        setTimeShiftAvailable(false);
        mTvView.timeShiftPlay(inputId, recordingUri);
        if (needSurfaceSizeUpdate && mFixedSurfaceWidth > 0 && mFixedSurfaceHeight > 0) {
            // When the input is changed, TvView recreates its SurfaceView internally.
            // So we need to call SurfaceHolder.setFixedSize for the new SurfaceView.
            getSurfaceView().getHolder().setFixedSize(mFixedSurfaceWidth, mFixedSurfaceHeight);
        }
        hideScreenByVideoAvailability(TvInputManager.VIDEO_UNAVAILABLE_REASON_TUNING);
        unblockScreenByContentRating();
        if (mParentControlEnabled) {
            mBlockScreenForTuneView.setVisibility(View.VISIBLE);
        }
        if (mOnTuneListener != null) {
            mOnTuneListener.onStreamInfoChanged(this);
        }
        return true;
    }

    /**
     * Tunes to a channel with the {@code channelId}.
     *
     * @param params extra data to send it to TIS and store the data in TIMS.
     * @return false, if the TV input is not a proper state to tune to a channel. For example,
     *         if the state is disconnected or channelId doesn't exist, it returns false.
     */
    public boolean tuneTo(Channel channel, Bundle params, OnTuneListener listener) {
        if (!mStarted) {
            throw new IllegalStateException("TvView isn't started");
        }
        if (DEBUG) Log.d(TAG, "tuneTo " + channel);
        TvInputInfo inputInfo = mInputManagerHelper.getTvInputInfo(channel.getInputId());
        if (inputInfo == null) {
            return false;
        }
        if (mCurrentChannel != null) {
            long duration = mChannelViewTimer.reset();
            mTracker.sendChannelViewStop(mCurrentChannel, duration);
            if (mWatchedHistoryManager != null && !mCurrentChannel.isPassthrough()) {
                mWatchedHistoryManager.logChannelViewStop(mCurrentChannel,
                        System.currentTimeMillis(), duration);
            }
        }
        mOnTuneListener = listener;
        mCurrentChannel = channel;
        mRecordedProgram = null;
        boolean tunedByRecommendation = params != null
                && params.getString(NotificationService.TUNE_PARAMS_RECOMMENDATION_TYPE) != null;
        boolean needSurfaceSizeUpdate = false;
        if (!inputInfo.equals(mInputInfo)) {
            mInputInfo = inputInfo;
            mCanReceiveInputEvent = getContext().getPackageManager().checkPermission(
                    PERMISSION_RECEIVE_INPUT_EVENT, mInputInfo.getServiceInfo().packageName)
                            == PackageManager.PERMISSION_GRANTED;
            if (DEBUG) {
                Log.d(TAG, "Input \'" + mInputInfo.getId() + "\' can receive input event: "
                        + mCanReceiveInputEvent);
            }
            needSurfaceSizeUpdate = true;
        }
        mTracker.sendChannelViewStart(mCurrentChannel, tunedByRecommendation);
        mChannelViewTimer.start();
        mVideoWidth = 0;
        mVideoHeight = 0;
        mVideoFormat = StreamInfo.VIDEO_DEFINITION_LEVEL_UNKNOWN;
        mVideoFrameRate = 0f;
        mVideoDisplayAspectRatio = 0f;
        mAudioChannelCount = StreamInfo.AUDIO_CHANNEL_COUNT_UNKNOWN;
        mHasClosedCaption = false;
        mTvView.setCallback(mCallback);
        mTimeShiftCurrentPositionMs = INVALID_TIME;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            // To reduce the IPCs, unregister the callback here and register it when necessary.
            mTvView.setTimeShiftPositionCallback(null);
        }
        setTimeShiftAvailable(false);
        mTvView.tune(mInputInfo.getId(), mCurrentChannel.getUri(), params);
        if (needSurfaceSizeUpdate && mFixedSurfaceWidth > 0 && mFixedSurfaceHeight > 0) {
            // When the input is changed, TvView recreates its SurfaceView internally.
            // So we need to call SurfaceHolder.setFixedSize for the new SurfaceView.
            getSurfaceView().getHolder().setFixedSize(mFixedSurfaceWidth, mFixedSurfaceHeight);
        }
        hideScreenByVideoAvailability(TvInputManager.VIDEO_UNAVAILABLE_REASON_TUNING);
        unblockScreenByContentRating();
        if (channel.isPassthrough()) {
            mBlockScreenForTuneView.setVisibility(View.GONE);
        } else if (mParentControlEnabled) {
            mBlockScreenForTuneView.setVisibility(View.VISIBLE);
        }
        if (mOnTuneListener != null) {
            mOnTuneListener.onStreamInfoChanged(this);
        }
        return true;
    }

    @Override
    public Channel getCurrentChannel() {
        return mCurrentChannel;
    }

    /**
     * Sets the current channel. Call this method only when setting the current channel without
     * actually tuning to it.
     *
     * @param currentChannel The new current channel to set to.
     */
    public void setCurrentChannel(Channel currentChannel) {
        mCurrentChannel = currentChannel;
    }

    public void setStreamVolume(float volume) {
        if (!mStarted) {
            throw new IllegalStateException("TvView isn't started");
        }
        if (DEBUG) Log.d(TAG, "setStreamVolume " + volume);
        mVolume = volume;
        if (!mIsMuted) {
            mTvView.setStreamVolume(volume);
        }
    }

    /**
     * Sets fixed size for the internal {@link android.view.Surface} of
     * {@link android.media.tv.TvView}. If either {@code width} or {@code height} is non positive,
     * the {@link android.view.Surface}'s size will be matched to the layout.
     *
     * Note: Once {@link android.view.SurfaceHolder#setFixedSize} is called,
     * {@link android.view.SurfaceView} and its underlying window can be misaligned, when the size
     * of {@link android.view.SurfaceView} is changed without changing either left position or top
     * position. For detail, please refer the codes of android.view.SurfaceView.updateWindow().
     */
    public void setFixedSurfaceSize(int width, int height) {
        mFixedSurfaceWidth = width;
        mFixedSurfaceHeight = height;
        if (mFixedSurfaceWidth > 0 && mFixedSurfaceHeight > 0) {
            // When the input is changed, TvView recreates its SurfaceView internally.
            // So we need to call SurfaceHolder.setFixedSize for the new SurfaceView.
            SurfaceView surfaceView = (SurfaceView) mTvView.getChildAt(0);
            surfaceView.getHolder().setFixedSize(mFixedSurfaceWidth, mFixedSurfaceHeight);
        } else {
            SurfaceView surfaceView = (SurfaceView) mTvView.getChildAt(0);
            surfaceView.getHolder().setSizeFromLayout();
        }
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        return mCanReceiveInputEvent && mTvView.dispatchKeyEvent(event);
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        return mCanReceiveInputEvent && mTvView.dispatchTouchEvent(event);
    }

    @Override
    public boolean dispatchTrackballEvent(MotionEvent event) {
        return mCanReceiveInputEvent && mTvView.dispatchTrackballEvent(event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {
        return mCanReceiveInputEvent && mTvView.dispatchGenericMotionEvent(event);
    }

    public interface OnTuneListener {
        void onTuneFailed(Channel channel);
        void onUnexpectedStop(Channel channel);
        void onStreamInfoChanged(StreamInfo info);
        void onChannelRetuned(Uri channel);
        void onContentBlocked();
        void onContentAllowed();
    }

    public void unblockContent(TvContentRating rating) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            try {
                Method method = TvView.class.getMethod("requestUnblockContent",
                        TvContentRating.class);
                method.invoke(mTvView, rating);
            } catch (NoSuchMethodException|IllegalAccessException|InvocationTargetException e) {
                e.printStackTrace();
            }
        } else {
            mTvView.unblockContent(rating);
        }
    }

    @Override
    public int getVideoWidth() {
        return mVideoWidth;
    }

    @Override
    public int getVideoHeight() {
        return mVideoHeight;
    }

    @Override
    public int getVideoDefinitionLevel() {
        return mVideoFormat;
    }

    @Override
    public float getVideoFrameRate() {
        return mVideoFrameRate;
    }

    /**
     * Returns displayed aspect ratio (video width / video height * pixel ratio).
     */
    @Override
    public float getVideoDisplayAspectRatio() {
        return mVideoDisplayAspectRatio;
    }

    @Override
    public int getAudioChannelCount() {
        return mAudioChannelCount;
    }

    @Override
    public boolean hasClosedCaption() {
        return mHasClosedCaption;
    }

    @Override
    public boolean isVideoAvailable() {
        return mVideoAvailable;
    }

    @Override
    public int getVideoUnavailableReason() {
        return mVideoUnavailableReason;
    }

    /**
     * Returns the {@link android.view.SurfaceView} of the {@link android.media.tv.TvView}.
     */
    private SurfaceView getSurfaceView() {
        return (SurfaceView) mTvView.getChildAt(0);
    }

    public void setOnUnhandledInputEventListener(OnUnhandledInputEventListener listener) {
        mTvView.setOnUnhandledInputEventListener(listener);
    }

    public void setClosedCaptionEnabled(boolean enabled) {
        mTvView.setCaptionEnabled(enabled);
    }

    public List<TvTrackInfo> getTracks(int type) {
        return mTvView.getTracks(type);
    }

    public String getSelectedTrack(int type) {
        return mTvView.getSelectedTrack(type);
    }

    public void selectTrack(int type, String trackId) {
        mTvView.selectTrack(type, trackId);
    }

    /**
     * Returns if the screen is blocked by {@link #blockScreen()}.
     */
    public boolean isScreenBlocked() {
        return mScreenBlocked;
    }

    public void setOnScreenBlockedListener(OnScreenBlockingChangedListener listener) {
        mOnScreenBlockedListener = listener;
    }

    /**
     * Returns currently blocked content rating. {@code null} if it's not blocked.
     */
    public TvContentRating getBlockedContentRating() {
        return mBlockedContentRating;
    }

    /**
     * Locks current TV screen and mutes.
     * There would be black screen with lock icon in order to show that
     * screen block is intended and not an error.
     * TODO: Accept parameter to show lock icon or not.
     */
    public void blockScreen() {
        mScreenBlocked = true;
        checkBlockScreenAndMuteNeeded();
        if (mOnScreenBlockedListener != null) {
            mOnScreenBlockedListener.onScreenBlockingChanged(true);
        }
    }

    private void blockScreenByContentRating(TvContentRating rating) {
        mBlockedContentRating = rating;
        checkBlockScreenAndMuteNeeded();
    }

    @Override
    @SuppressLint("RtlHardcoded")
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        if (mIsPip) {
            int height = bottom - top;
            float scale;
            if (mBlockScreenType == BLOCK_SCREEN_TYPE_SHRUNKEN_TV_VIEW) {
                scale = height * PIP_BLOCK_SCREEN_SCALE_FACTOR / mShrunkenTvViewHeight;
            } else {
                scale = height * PIP_BLOCK_SCREEN_SCALE_FACTOR / mScreenHeight;
            }
            // TODO: need to get UX confirmation.
            mBlockScreenView.scaleContainerView(scale);
        }
    }

    @Override
    public void setLayoutParams(ViewGroup.LayoutParams params) {
        super.setLayoutParams(params);
        if (mTvView != null) {
            copyLayoutParamsToTvView();
        }
    }

    private void copyLayoutParamsToTvView() {
        FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams) getLayoutParams();
        FrameLayout.LayoutParams tvViewLp = (FrameLayout.LayoutParams) mTvView.getLayoutParams();
        if (tvViewLp.bottomMargin != lp.bottomMargin
                || tvViewLp.topMargin != lp.topMargin
                || tvViewLp.leftMargin != lp.leftMargin
                || tvViewLp.rightMargin != lp.rightMargin
                || tvViewLp.gravity != lp.gravity
                || tvViewLp.height != lp.height
                || tvViewLp.width != lp.width) {
            if (lp.topMargin == tvViewLp.topMargin && lp.leftMargin == tvViewLp.leftMargin) {
                // HACK: If top and left position aren't changed and SurfaceHolder.setFixedSize is
                // used, SurfaceView doesn't catch the width and height change. It causes a bug that
                // PIP size change isn't shown when PIP is located TOP|LEFT. So we adjust 1 px for
                // small size PIP as a workaround.
                tvViewLp.leftMargin = lp.leftMargin + 1;
            } else {
                tvViewLp.leftMargin = lp.leftMargin;
            }
            tvViewLp.topMargin = lp.topMargin;
            tvViewLp.bottomMargin = lp.bottomMargin;
            tvViewLp.rightMargin = lp.rightMargin;
            tvViewLp.gravity = lp.gravity;
            tvViewLp.height = lp.height;
            tvViewLp.width = lp.width;
            mTvView.setLayoutParams(tvViewLp);
        }
    }

    @Override
    protected void onVisibilityChanged(View changedView, int visibility) {
        super.onVisibilityChanged(changedView, visibility);
        if (mTvView != null) {
            mTvView.setVisibility(visibility);
        }
    }

    /**
     * Set the type of block screen. If {@code type} is set to {@code BLOCK_SCREEN_TYPE_NO_UI}, the
     * block screen will not show any description such as a lock icon and a text for the blocked
     * reason, if {@code type} is set to {@code BLOCK_SCREEN_TYPE_SHRUNKEN_TV_VIEW}, the block screen
     * will show the description for shrunken tv view (Small icon and short text), and if
     * {@code type} is set to {@code BLOCK_SCREEN_TYPE_NORMAL}, the block screen will show the
     * description for normal tv view (Big icon and long text).
     *
     * @param type The type of block screen to set.
     */
    public void setBlockScreenType(@BlockScreenType int type) {
        // TODO: need to support the transition from NORMAL to SHRUNKEN and vice verse.
        if (mBlockScreenType != type) {
            mBlockScreenType = type;
            updateBlockScreenUI(true);
        }
    }

    private void updateBlockScreenUI(boolean animation) {
        mBlockScreenView.endAnimations();

        if (!mScreenBlocked && mBlockedContentRating == null) {
            mBlockScreenView.setVisibility(GONE);
            return;
        }

        mBlockScreenView.setVisibility(VISIBLE);
        if (!animation || mBlockScreenType != TunableTvView.BLOCK_SCREEN_TYPE_NO_UI) {
            adjustBlockScreenSpacingAndText();
        }
        mBlockScreenView.onBlockStatusChanged(mBlockScreenType, animation);
    }

    private void adjustBlockScreenSpacingAndText() {
        // TODO: need to add animation for padding change when the block screen type is changed
        // NORMAL to SHRUNKEN and vice verse.
        mBlockScreenView.setSpacing(mBlockScreenType);
        String text = getBlockScreenText();
        if (text != null) {
            mBlockScreenView.setText(text);
        }
    }

    /**
     * Returns the block screen text corresponding to the current status.
     * Note that returning {@code null} value means that the current text should not be changed.
     */
    private String getBlockScreenText() {
        if (mScreenBlocked) {
            switch (mBlockScreenType) {
                case BLOCK_SCREEN_TYPE_NO_UI:
                case BLOCK_SCREEN_TYPE_SHRUNKEN_TV_VIEW:
                    return "";
                case BLOCK_SCREEN_TYPE_NORMAL:
                    if (mCanModifyParentalControls) {
                        return getResources().getString(R.string.tvview_channel_locked);
                    } else {
                        return getResources().getString(
                                R.string.tvview_channel_locked_no_permission);
                    }
            }
        } else if (mBlockedContentRating != null) {
            String name = mContentRatingsManager.getDisplayNameForRating(mBlockedContentRating);
            switch (mBlockScreenType) {
                case BLOCK_SCREEN_TYPE_NO_UI:
                    return "";
                case BLOCK_SCREEN_TYPE_SHRUNKEN_TV_VIEW:
                    if (TextUtils.isEmpty(name)) {
                        return getResources().getString(R.string.shrunken_tvview_content_locked);
                    } else {
                        return getContext().getString(
                                R.string.shrunken_tvview_content_locked_format, name);
                    }
                case BLOCK_SCREEN_TYPE_NORMAL:
                    if (TextUtils.isEmpty(name)) {
                        if (mCanModifyParentalControls) {
                            return getResources().getString(R.string.tvview_content_locked);
                        } else {
                            return getResources().getString(
                                    R.string.tvview_content_locked_no_permission);
                        }
                    } else {
                        if (mCanModifyParentalControls) {
                            return getContext().getString(
                                    R.string.tvview_content_locked_format, name);
                        } else {
                            return getContext().getString(
                                    R.string.tvview_content_locked_format_no_permission, name);
                        }
                    }
            }
        }
        return null;
    }

    private void checkBlockScreenAndMuteNeeded() {
        updateBlockScreenUI(false);
        if (mScreenBlocked || mBlockedContentRating != null) {
            mute();
            if (mIsPip) {
                // If we don't make mTvView invisible, some frames are leaked when a user changes
                // PIP layout in options.
                // Note: When video is unavailable, we keep the mTvView's visibility, because
                // TIS implementation may not send video available with no surface.
                mTvView.setVisibility(View.INVISIBLE);
            }
        } else {
            unmuteIfPossible();
            if (mIsPip) {
                mTvView.setVisibility(View.VISIBLE);
            }
        }
    }

    public void unblockScreen() {
        mScreenBlocked = false;
        checkBlockScreenAndMuteNeeded();
        if (mOnScreenBlockedListener != null) {
            mOnScreenBlockedListener.onScreenBlockingChanged(false);
        }
    }

    private void unblockScreenByContentRating() {
        mBlockedContentRating = null;
        checkBlockScreenAndMuteNeeded();
    }

    @UiThread
    private void hideScreenByVideoAvailability(int reason) {
        mVideoAvailable = false;
        mVideoUnavailableReason = reason;
        if (mInternetCheckTask != null) {
            mInternetCheckTask.cancel(true);
            mInternetCheckTask = null;
        }
        switch (reason) {
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_AUDIO_ONLY:
                mHideScreenView.setVisibility(VISIBLE);
                mHideScreenView.setImageVisibility(false);
                mHideScreenView.setText(R.string.tvview_msg_audio_only);
                mBufferingSpinnerView.setVisibility(GONE);
                unmuteIfPossible();
                break;
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_BUFFERING:
                mBufferingSpinnerView.setVisibility(VISIBLE);
                mute();
                break;
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_WEAK_SIGNAL:
                mHideScreenView.setVisibility(VISIBLE);
                mHideScreenView.setText(R.string.tvview_msg_weak_signal);
                mBufferingSpinnerView.setVisibility(GONE);
                mute();
                break;
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_TUNING:
            case VIDEO_UNAVAILABLE_REASON_NOT_TUNED:
                mHideScreenView.setVisibility(VISIBLE);
                mHideScreenView.setImageVisibility(false);
                mHideScreenView.setText(null);
                mBufferingSpinnerView.setVisibility(GONE);
                mute();
                break;
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN:
            default:
                mHideScreenView.setVisibility(VISIBLE);
                mHideScreenView.setImageVisibility(false);
                mHideScreenView.setText(null);
                mBufferingSpinnerView.setVisibility(GONE);
                mute();
                if (mCurrentChannel != null && !mCurrentChannel.isPhysicalTunerChannel()) {
                    mInternetCheckTask = new InternetCheckTask();
                    mInternetCheckTask.execute();
                }
                break;
        }
    }

    private void unhideScreenByVideoAvailability() {
        mVideoAvailable = true;
        mHideScreenView.setVisibility(GONE);
        mBufferingSpinnerView.setVisibility(GONE);
        unmuteIfPossible();
    }

    private void unmuteIfPossible() {
        if (mVideoAvailable && !mScreenBlocked && mBlockedContentRating == null) {
            unmute();
        }
    }

    private void mute() {
        mIsMuted = true;
        mTvView.setStreamVolume(0);
    }

    private void unmute() {
        mIsMuted = false;
        mTvView.setStreamVolume(mVolume);
    }

    /** Returns true if this view is faded out. */
    public boolean isFadedOut() {
        return mFadeState == FADED_OUT;
    }

    /** Fade out this TunableTvView. Fade out by increasing the dimming. */
    public void fadeOut(int durationMillis, TimeInterpolator interpolator,
            final Runnable actionAfterFade) {
        mDimScreenView.setAlpha(0f);
        mDimScreenView.setVisibility(View.VISIBLE);
        mDimScreenView.animate()
                .alpha(1f)
                .setDuration(durationMillis)
                .setInterpolator(interpolator)
                .withStartAction(new Runnable() {
                    @Override
                    public void run() {
                        mFadeState = FADING_OUT;
                        mActionAfterFade = actionAfterFade;
                    }
                })
                .withEndAction(new Runnable() {
                    @Override
                    public void run() {
                        mFadeState = FADED_OUT;
                    }
                });
    }

    /** Fade in this TunableTvView. Fade in by decreasing the dimming. */
    public void fadeIn(int durationMillis, TimeInterpolator interpolator,
            final Runnable actionAfterFade) {
        mDimScreenView.setAlpha(1f);
        mDimScreenView.setVisibility(View.VISIBLE);
        mDimScreenView.animate()
                .alpha(0f)
                .setDuration(durationMillis)
                .setInterpolator(interpolator)
                .withStartAction(new Runnable() {
                    @Override
                    public void run() {
                        mFadeState = FADING_IN;
                        mActionAfterFade = actionAfterFade;
                    }
                })
                .withEndAction(new Runnable() {
                    @Override
                    public void run() {
                        mFadeState = FADED_IN;
                        mDimScreenView.setVisibility(View.GONE);
                    }
                });
    }

    /** Remove the fade effect. */
    public void removeFadeEffect() {
        mDimScreenView.animate().cancel();
        mDimScreenView.setVisibility(View.GONE);
        mFadeState = FADED_IN;
    }

    /**
     * Sets the TimeShiftListener
     *
     * @param listener The instance of {@link TimeShiftListener}.
     */
    public void setTimeShiftListener(TimeShiftListener listener) {
        mTimeShiftListener = listener;
    }

    private void setTimeShiftAvailable(boolean isTimeShiftAvailable) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M || mTimeShiftAvailable == isTimeShiftAvailable) {
            return;
        }
        mTimeShiftAvailable = isTimeShiftAvailable;
        if (isTimeShiftAvailable) {
            mTvView.setTimeShiftPositionCallback(new TvView.TimeShiftPositionCallback() {
                @Override
                public void onTimeShiftStartPositionChanged(String inputId, long timeMs) {
                    if (mTimeShiftListener != null && mCurrentChannel != null
                            && mCurrentChannel.getInputId().equals(inputId)) {
                        mTimeShiftListener.onRecordStartTimeChanged(timeMs);
                    }
                }

                @Override
                public void onTimeShiftCurrentPositionChanged(String inputId, long timeMs) {
                    mTimeShiftCurrentPositionMs = timeMs;
                }
            });
        } else {
            mTvView.setTimeShiftPositionCallback(null);
        }
        if (mTimeShiftListener != null) {
            mTimeShiftListener.onAvailabilityChanged();
        }
    }

    /**
     * Returns if the time shift is available for the current channel.
     */
    public boolean isTimeShiftAvailable() {
        return mTimeShiftAvailable;
    }

    /**
     * Returns the current time-shift state. It returns one of {@link #TIME_SHIFT_STATE_NONE},
     * {@link #TIME_SHIFT_STATE_PLAY}, {@link #TIME_SHIFT_STATE_PAUSE},
     * {@link #TIME_SHIFT_STATE_REWIND}, {@link #TIME_SHIFT_STATE_FAST_FORWARD}
     * or {@link #TIME_SHIFT_STATE_PAUSE}.
     */
    @TimeShiftState public int getTimeShiftState() {
        return mTimeShiftState;
    }

    /**
     * Plays the media, if the current input supports time-shifting.
     */
    public void timeshiftPlay() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            Log.w(TAG, "Time shifting is not supported in this platform.");
            return;
        }
        if (!isTimeShiftAvailable()) {
            throw new IllegalStateException("Time-shift is not supported for the current channel");
        }
        if (mTimeShiftState == TIME_SHIFT_STATE_PLAY) {
            return;
        }
        mTvView.timeShiftResume();
    }

    /**
     * Pauses the media, if the current input supports time-shifting.
     */
    public void timeshiftPause() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            Log.w(TAG, "Time shifting is not supported in this platform.");
            return;
        }
        if (!isTimeShiftAvailable()) {
            throw new IllegalStateException("Time-shift is not supported for the current channel");
        }
        if (mTimeShiftState == TIME_SHIFT_STATE_PAUSE) {
            return;
        }
        mTvView.timeShiftPause();
    }

    /**
     * Rewinds the media with the given speed, if the current input supports time-shifting.
     *
     * @param speed The speed to rewind the media. e.g. 2 for 2x, 3 for 3x and 4 for 4x.
     */
    public void timeshiftRewind(int speed) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            Log.w(TAG, "Time shifting is not supported in this platform.");
        } else if (!isTimeShiftAvailable()) {
            throw new IllegalStateException("Time-shift is not supported for the current channel");
        } else {
            if (speed <= 0) {
                throw new IllegalArgumentException("The speed should be a positive integer.");
            }
            mTimeShiftState = TIME_SHIFT_STATE_REWIND;
            PlaybackParams params = new PlaybackParams();
            params.setSpeed(speed * -1);
            mTvView.timeShiftSetPlaybackParams(params);
        }
    }

    /**
     * Fast-forwards the media with the given speed, if the current input supports time-shifting.
     *
     * @param speed The speed to forward the media. e.g. 2 for 2x, 3 for 3x and 4 for 4x.
     */
    public void timeshiftFastForward(int speed) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            Log.w(TAG, "Time shifting is not supported in this platform.");
        } else if (!isTimeShiftAvailable()) {
            throw new IllegalStateException("Time-shift is not supported for the current channel");
        } else {
            if (speed <= 0) {
                throw new IllegalArgumentException("The speed should be a positive integer.");
            }
            mTimeShiftState = TIME_SHIFT_STATE_FAST_FORWARD;
            PlaybackParams params = new PlaybackParams();
            params.setSpeed(speed);
            mTvView.timeShiftSetPlaybackParams(params);
        }
    }

    /**
     * Seek to the given time position.
     *
     * @param timeMs The time in milliseconds to seek to.
     */
    public void timeshiftSeekTo(long timeMs) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            Log.w(TAG, "Time shifting is not supported in this platform.");
            return;
        }
        if (!isTimeShiftAvailable()) {
            throw new IllegalStateException("Time-shift is not supported for the current channel");
        }
        mTvView.timeShiftSeekTo(timeMs);
    }

    /**
     * Returns the current playback position in milliseconds.
     */
    public long timeshiftGetCurrentPositionMs() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            Log.w(TAG, "Time shifting is not supported in this platform.");
            return INVALID_TIME;
        }
        if (!isTimeShiftAvailable()) {
            throw new IllegalStateException("Time-shift is not supported for the current channel");
        }
        if (DEBUG) {
            Log.d(TAG, "timeshiftGetCurrentPositionMs: current position ="
                    + Utils.toTimeString(mTimeShiftCurrentPositionMs));
        }
        return mTimeShiftCurrentPositionMs;
    }

    /**
     * Used to receive the time-shift events.
     */
    public static abstract class TimeShiftListener {
        /**
         * Called when the availability of the time-shift for the current channel has been changed.
         * It should be guaranteed that this is called only when the availability is really changed.
         */
        public abstract void onAvailabilityChanged();

        /**
         * Called when the record start time has been changed.
         */
        public abstract void onRecordStartTimeChanged(long recordStartTimeMs);
    }

    /**
     * A listener which receives the notification when the screen is blocked/unblocked.
     */
    public static abstract class OnScreenBlockingChangedListener {
        /**
         * Called when the screen is blocked/unblocked.
         */
        public abstract void onScreenBlockingChanged(boolean blocked);
    }

    public class InternetCheckTask extends AsyncTask<Void, Void, Boolean> {
        @Override
        protected Boolean doInBackground(Void... params) {
            return NetworkUtils.isNetworkAvailable(mConnectivityManager);
        }

        @Override
        protected void onPostExecute(Boolean networkAvailable) {
            mInternetCheckTask = null;
            if (!mVideoAvailable && !networkAvailable && isAttachedToWindow()
                    && mVideoUnavailableReason == TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN) {
                mHideScreenView.setImageVisibility(true);
                mHideScreenView.setImage(R.drawable.ic_sad_cloud);
                mHideScreenView.setText(R.string.tvview_msg_no_internet_connection);
            }
        }
    }
}
