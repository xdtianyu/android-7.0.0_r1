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
import android.animation.AnimatorInflater;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ValueAnimator;
import android.content.Context;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.graphics.Bitmap;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.net.Uri;
import android.os.Handler;
import android.support.annotation.Nullable;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.text.style.TextAppearanceSpan;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AnimationUtils;
import android.view.animation.Interpolator;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.common.recording.RecordedProgram;
import com.android.tv.data.Channel;
import com.android.tv.data.Program;
import com.android.tv.data.StreamInfo;
import com.android.tv.util.ImageCache;
import com.android.tv.util.ImageLoader;
import com.android.tv.util.ImageLoader.ImageLoaderCallback;
import com.android.tv.util.ImageLoader.LoadTvInputLogoTask;
import com.android.tv.util.Utils;

import junit.framework.Assert;

import java.util.Objects;

/**
 * A view to render channel banner.
 */
public class ChannelBannerView extends FrameLayout implements TvTransitionManager.TransitionLayout {
    private static final String TAG = "ChannelBannerView";
    private static final boolean DEBUG = false;

    /**
     * Show all information at the channel banner.
     */
    public static final int LOCK_NONE = 0;

    /**
     * Lock program details at the channel banner.
     * This is used when a content is locked so we don't want to show program details
     * including program description text and poster art.
     */
    public static final int LOCK_PROGRAM_DETAIL = 1;

    /**
     * Lock channel information at the channel banner.
     * This is used when a channel is locked so we only want to show input information.
     */
    public static final int LOCK_CHANNEL_INFO = 2;

    private static final String EMPTY_STRING = "";

    private static Program sNoProgram;
    private static Program sLockedChannelProgram;
    private static String sClosedCaptionMark;

    private final MainActivity mMainActivity;
    private final Resources mResources;
    private View mChannelView;

    private TextView mChannelNumberTextView;
    private ImageView mChannelLogoImageView;
    private TextView mProgramTextView;
    private ImageView mTvInputLogoImageView;
    private TextView mChannelNameTextView;
    private TextView mProgramTimeTextView;
    private ProgressBar mRemainingTimeView;
    private TextView mClosedCaptionTextView;
    private TextView mAspectRatioTextView;
    private TextView mResolutionTextView;
    private TextView mAudioChannelTextView;
    private TextView mProgramDescriptionTextView;
    private String mProgramDescriptionText;
    private View mAnchorView;
    private Channel mCurrentChannel;
    private Program mLastUpdatedProgram;
    private RecordedProgram mLastUpdatedRecordedProgram;
    private final Handler mHandler = new Handler();

    private int mLockType;

    private Animator mResizeAnimator;
    private int mCurrentHeight;
    private boolean mProgramInfoUpdatePendingByResizing;

    private final Animator mProgramDescriptionFadeInAnimator;
    private final Animator mProgramDescriptionFadeOutAnimator;

    private final Runnable mHideRunnable = new Runnable() {
        @Override
        public void run() {
            mCurrentHeight = 0;
            mMainActivity.getOverlayManager().hideOverlays(
                    TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_DIALOG
                    | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_SIDE_PANELS
                    | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_PROGRAM_GUIDE
                    | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_MENU
                    | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_FRAGMENT);
        }
    };
    private final long mShowDurationMillis;
    private final int mChannelLogoImageViewWidth;
    private final int mChannelLogoImageViewHeight;
    private final int mChannelLogoImageViewMarginStart;
    private final int mProgramDescriptionTextViewWidth;
    private final int mChannelBannerTextColor;
    private final int mChannelBannerDimTextColor;
    private final int mResizeAnimDuration;
    private final Interpolator mResizeInterpolator;

    private final AnimatorListenerAdapter mResizeAnimatorListener = new AnimatorListenerAdapter() {
        @Override
        public void onAnimationStart(Animator animator) {
            mProgramInfoUpdatePendingByResizing = false;
        }

        @Override
        public void onAnimationEnd(Animator animator) {
            mProgramDescriptionTextView.setAlpha(1f);
            mResizeAnimator = null;
            if (mProgramInfoUpdatePendingByResizing) {
                mProgramInfoUpdatePendingByResizing = false;
                updateProgramInfo(mLastUpdatedProgram);
            }
        }
    };

    private final ContentObserver mProgramUpdateObserver = new ContentObserver(mHandler) {
        @Override
        public void onChange(boolean selfChange, Uri uri) {
            // TODO: This {@code uri} argument may be a program which is not related to this
            // channel. Consider adding channel id as a parameter of program URI to avoid
            // unnecessary update.
            mHandler.post(mProgramUpdateRunnable);
        }
    };

    private final Runnable mProgramUpdateRunnable = new Runnable() {
        @Override
        public void run() {
            removeCallbacks(this);
            updateViews(null);
        }
    };

    public ChannelBannerView(Context context) {
        this(context, null);
    }

    public ChannelBannerView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ChannelBannerView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mResources = getResources();

        mMainActivity = (MainActivity) context;

        mShowDurationMillis = mResources.getInteger(
                R.integer.channel_banner_show_duration);
        mChannelLogoImageViewWidth = mResources.getDimensionPixelSize(
                R.dimen.channel_banner_channel_logo_width);
        mChannelLogoImageViewHeight = mResources.getDimensionPixelSize(
                R.dimen.channel_banner_channel_logo_height);
        mChannelLogoImageViewMarginStart = mResources.getDimensionPixelSize(
                R.dimen.channel_banner_channel_logo_margin_start);
        mProgramDescriptionTextViewWidth = mResources.getDimensionPixelSize(
                R.dimen.channel_banner_program_description_width);
        mChannelBannerTextColor = Utils.getColor(mResources, R.color.channel_banner_text_color);
        mChannelBannerDimTextColor = Utils.getColor(mResources,
                R.color.channel_banner_dim_text_color);
        mResizeAnimDuration = mResources.getInteger(R.integer.channel_banner_fast_anim_duration);

        mResizeInterpolator = AnimationUtils.loadInterpolator(context,
                android.R.interpolator.linear_out_slow_in);

        mProgramDescriptionFadeInAnimator = AnimatorInflater.loadAnimator(mMainActivity,
                R.animator.channel_banner_program_description_fade_in);
        mProgramDescriptionFadeOutAnimator = AnimatorInflater.loadAnimator(mMainActivity,
                R.animator.channel_banner_program_description_fade_out);

        if (sNoProgram == null) {
            sNoProgram = new Program.Builder()
                    .setTitle(context.getString(R.string.channel_banner_no_title))
                    .setDescription(EMPTY_STRING)
                    .build();
        }
        if (sLockedChannelProgram == null){
            sLockedChannelProgram = new Program.Builder()
                    .setTitle(context.getString(R.string.channel_banner_locked_channel_title))
                    .setDescription(EMPTY_STRING)
                    .build();
        }
        if (sClosedCaptionMark == null) {
            sClosedCaptionMark = context.getString(R.string.closed_caption);
        }
    }

    @Override
    protected void onAttachedToWindow() {
        if (DEBUG) Log.d(TAG, "onAttachedToWindow");
        super.onAttachedToWindow();
        getContext().getContentResolver().registerContentObserver(TvContract.Programs.CONTENT_URI,
                true, mProgramUpdateObserver);
    }

    @Override
    protected void onDetachedFromWindow() {
        if (DEBUG) Log.d(TAG, "onDetachedToWindow");
        getContext().getContentResolver().unregisterContentObserver(mProgramUpdateObserver);
        super.onDetachedFromWindow();
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mChannelView = findViewById(R.id.channel_banner_view);

        mChannelNumberTextView = (TextView) findViewById(R.id.channel_number);
        mChannelLogoImageView = (ImageView) findViewById(R.id.channel_logo);
        mProgramTextView = (TextView) findViewById(R.id.program_text);
        mTvInputLogoImageView = (ImageView) findViewById(R.id.tvinput_logo);
        mChannelNameTextView = (TextView) findViewById(R.id.channel_name);
        mProgramTimeTextView = (TextView) findViewById(R.id.program_time_text);
        mRemainingTimeView = (ProgressBar) findViewById(R.id.remaining_time);
        mClosedCaptionTextView = (TextView) findViewById(R.id.closed_caption);
        mAspectRatioTextView = (TextView) findViewById(R.id.aspect_ratio);
        mResolutionTextView = (TextView) findViewById(R.id.resolution);
        mAudioChannelTextView = (TextView) findViewById(R.id.audio_channel);
        mProgramDescriptionTextView = (TextView) findViewById(R.id.program_description);
        mAnchorView = findViewById(R.id.anchor);

        mProgramDescriptionFadeInAnimator.setTarget(mProgramDescriptionTextView);
        mProgramDescriptionFadeOutAnimator.setTarget(mProgramDescriptionTextView);
        mProgramDescriptionFadeOutAnimator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animator) {
                mProgramDescriptionTextView.setText(mProgramDescriptionText);
            }
        });
    }

    @Override
    public void onEnterAction(boolean fromEmptyScene) {
        resetAnimationEffects();
        if (fromEmptyScene) {
            ViewUtils.setTransitionAlpha(mChannelView, 1f);
        }
        scheduleHide();
    }

    @Override
    public void onExitAction() {
        mCurrentHeight = 0;
        cancelHide();
    }

    private void scheduleHide() {
        cancelHide();
        mHandler.postDelayed(mHideRunnable, mShowDurationMillis);
    }

    private void cancelHide() {
        mHandler.removeCallbacks(mHideRunnable);
    }

    private void resetAnimationEffects() {
        setAlpha(1f);
        setScaleX(1f);
        setScaleY(1f);
        setTranslationX(0);
        setTranslationY(0);
    }

    /**
     * Set new lock type.
     *
     * @param lockType Any of LOCK_NONE, LOCK_PROGRAM_DETAIL, or LOCK_CHANNEL_INFO.
     * @return {@code true} only if lock type is changed
     * @throws IllegalArgumentException if lockType is invalid.
     */
    public boolean setLockType(int lockType) {
        if (lockType != LOCK_NONE && lockType != LOCK_CHANNEL_INFO
                && lockType != LOCK_PROGRAM_DETAIL) {
            throw new IllegalArgumentException("No such lock type " + lockType);
        }
        if (mLockType != lockType) {
            mLockType = lockType;
            return true;
        }
        return false;
    }

    /**
     * Update channel banner view.
     *
     * @param info A StreamInfo that includes stream information.
     * If it's {@code null}, only program information will be updated.
     */
    public void updateViews(StreamInfo info) {
        resetAnimationEffects();
        Channel channel = mMainActivity.getCurrentChannel();
        if (!Objects.equals(mCurrentChannel, channel) && isShown()) {
            scheduleHide();
        }
        mCurrentChannel = channel;
        mChannelView.setVisibility(VISIBLE);
        if (info != null) {
            // If the current channels between ChannelTuner and TvView are different,
            // the stream information should not be seen.
            updateStreamInfo(channel != null && channel.equals(info.getCurrentChannel()) ? info
                    : null);
            updateChannelInfo();
        }
        if (mMainActivity.isRecordingPlayback()) {
            updateProgramInfo(mMainActivity.getPlayingRecordedProgram());
        } else {
            updateProgramInfo(mMainActivity.getCurrentProgram());
        }
    }

    private void updateStreamInfo(StreamInfo info) {
        // Update stream information in a channel.
        if (mLockType != LOCK_CHANNEL_INFO && info != null) {
            updateText(mClosedCaptionTextView, info.hasClosedCaption() ? sClosedCaptionMark
                    : EMPTY_STRING);
            updateText(mAspectRatioTextView,
                    Utils.getAspectRatioString(info.getVideoDisplayAspectRatio()));
            updateText(mResolutionTextView,
                    Utils.getVideoDefinitionLevelString(
                            mMainActivity, info.getVideoDefinitionLevel()));
            updateText(mAudioChannelTextView,
                    Utils.getAudioChannelString(mMainActivity, info.getAudioChannelCount()));
        } else {
            // Channel change has been requested. But, StreamInfo hasn't been updated yet.
            mClosedCaptionTextView.setVisibility(View.GONE);
            mAspectRatioTextView.setVisibility(View.GONE);
            mResolutionTextView.setVisibility(View.GONE);
            mAudioChannelTextView.setVisibility(View.GONE);
        }
    }

    private void updateChannelInfo() {
        // Update static information for a channel.
        String displayNumber = EMPTY_STRING;
        String displayName = EMPTY_STRING;
        if (mCurrentChannel != null) {
            displayNumber = mCurrentChannel.getDisplayNumber();
            if (displayNumber == null) {
                displayNumber = EMPTY_STRING;
            }
            displayName = mCurrentChannel.getDisplayName();
            if (displayName == null) {
                displayName = EMPTY_STRING;
            }
        }

        if (displayNumber.isEmpty()) {
            mChannelNumberTextView.setVisibility(GONE);
        } else {
            mChannelNumberTextView.setVisibility(VISIBLE);
        }
        if (displayNumber.length() <= 3) {
            updateTextView(
                    mChannelNumberTextView,
                    R.dimen.channel_banner_channel_number_large_text_size,
                    R.dimen.channel_banner_channel_number_large_margin_top);
        } else if (displayNumber.length() <= 4) {
            updateTextView(
                    mChannelNumberTextView,
                    R.dimen.channel_banner_channel_number_medium_text_size,
                    R.dimen.channel_banner_channel_number_medium_margin_top);
        } else {
            updateTextView(
                    mChannelNumberTextView,
                    R.dimen.channel_banner_channel_number_small_text_size,
                    R.dimen.channel_banner_channel_number_small_margin_top);
        }
        mChannelNumberTextView.setText(displayNumber);
        mChannelNameTextView.setText(displayName);
        TvInputInfo info = mMainActivity.getTvInputManagerHelper().getTvInputInfo(
                getCurrentInputId());
        if (info == null || !ImageLoader.loadBitmap(createTvInputLogoLoaderCallback(info, this),
                        new LoadTvInputLogoTask(getContext(), ImageCache.getInstance(), info))) {
            mTvInputLogoImageView.setVisibility(View.GONE);
            mTvInputLogoImageView.setImageDrawable(null);
        }
        mChannelLogoImageView.setImageBitmap(null);
        mChannelLogoImageView.setVisibility(View.GONE);
        if (mCurrentChannel != null) {
            mCurrentChannel.loadBitmap(getContext(), Channel.LOAD_IMAGE_TYPE_CHANNEL_LOGO,
                    mChannelLogoImageViewWidth, mChannelLogoImageViewHeight,
                    createChannelLogoCallback(this, mCurrentChannel));
        }
    }

    private String getCurrentInputId() {
        Channel channel = mMainActivity.getCurrentChannel();
        if (channel != null) {
            return channel.getInputId();
        } else if (mMainActivity.isRecordingPlayback()) {
            RecordedProgram recordedProgram = mMainActivity.getPlayingRecordedProgram();
            if (recordedProgram != null) {
                return recordedProgram.getInputId();
            }
        }
        return null;
    }

    private void updateTvInputLogo(Bitmap bitmap) {
        mTvInputLogoImageView.setVisibility(View.VISIBLE);
        mTvInputLogoImageView.setImageBitmap(bitmap);
    }

    private static ImageLoaderCallback<ChannelBannerView> createTvInputLogoLoaderCallback(
            final TvInputInfo info, ChannelBannerView channelBannerView) {
        return new ImageLoaderCallback<ChannelBannerView>(channelBannerView) {
            @Override
            public void onBitmapLoaded(ChannelBannerView channelBannerView, Bitmap bitmap) {
                if (bitmap != null && channelBannerView.mCurrentChannel != null
                        && info.getId().equals(channelBannerView.mCurrentChannel.getInputId())) {
                    channelBannerView.updateTvInputLogo(bitmap);
                }
            }
        };
    }

    private void updateText(TextView view, String text) {
        if (TextUtils.isEmpty(text)) {
            view.setVisibility(View.GONE);
        } else {
            view.setVisibility(View.VISIBLE);
            view.setText(text);
        }
    }

    private void updateTextView(TextView textView, int sizeRes, int marginTopRes) {
        float textSize = mResources.getDimension(sizeRes);
        if (textView.getTextSize() != textSize) {
            textView.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSize);
        }
        updateTopMargin(textView, marginTopRes);
    }

    private void updateTopMargin(View view, int marginTopRes) {
        RelativeLayout.LayoutParams lp = (RelativeLayout.LayoutParams) view.getLayoutParams();
        int topMargin = (int) mResources.getDimension(marginTopRes);
        if (lp.topMargin != topMargin) {
            lp.topMargin = topMargin;
            view.setLayoutParams(lp);
        }
    }

    private static ImageLoaderCallback<ChannelBannerView> createChannelLogoCallback(
            ChannelBannerView channelBannerView, final Channel channel) {
        return new ImageLoaderCallback<ChannelBannerView>(channelBannerView) {
            @Override
            public void onBitmapLoaded(ChannelBannerView view, @Nullable Bitmap logo) {
                if (channel != view.mCurrentChannel) {
                    // The logo is obsolete.
                    return;
                }
                view.updateLogo(logo);
            }
        };
    }

    private void updateLogo(@Nullable Bitmap logo) {
        if (logo == null) {
            // Need to update the text size of the program text view depending on the channel logo.
            updateProgramTextView(mLastUpdatedProgram);
            return;
        }

        mChannelLogoImageView.setImageBitmap(logo);
        mChannelLogoImageView.setVisibility(View.VISIBLE);
        updateProgramTextView(mLastUpdatedProgram);

        if (mResizeAnimator == null) {
            String description = mProgramDescriptionTextView.getText().toString();
            boolean needFadeAnimation = !description.equals(mProgramDescriptionText);
            updateBannerHeight(needFadeAnimation);
        } else {
            mProgramInfoUpdatePendingByResizing = true;
        }
    }

    private void updateProgramInfo(Program program) {
        if (mLockType == LOCK_CHANNEL_INFO) {
            program = sLockedChannelProgram;
        } else if (!Program.isValid(program) || TextUtils.isEmpty(program.getTitle())) {
            program = sNoProgram;
        }

        if (mLastUpdatedProgram == null
                || !TextUtils.equals(program.getTitle(), mLastUpdatedProgram.getTitle())
                || !TextUtils.equals(program.getEpisodeDisplayTitle(getContext()),
                mLastUpdatedProgram.getEpisodeDisplayTitle(getContext()))) {
            updateProgramTextView(program);
        }
        updateProgramTimeInfo(program);

        // When the program is changed, but the previous resize animation has not ended yet,
        // cancel the animation.
        boolean isProgramChanged = !Objects.equals(mLastUpdatedProgram, program);
        if (mResizeAnimator != null && isProgramChanged) {
            setLastUpdatedProgram(program);
            mProgramInfoUpdatePendingByResizing = true;
            mResizeAnimator.cancel();
        } else if (mResizeAnimator == null) {
            if (mLockType != LOCK_NONE || TextUtils.isEmpty(program.getDescription())) {
                mProgramDescriptionTextView.setVisibility(GONE);
                mProgramDescriptionText = "";
            } else {
                mProgramDescriptionTextView.setVisibility(VISIBLE);
                mProgramDescriptionText = program.getDescription();
            }
            String description = mProgramDescriptionTextView.getText().toString();
            boolean needFadeAnimation = isProgramChanged
                    || !description.equals(mProgramDescriptionText);
            updateBannerHeight(needFadeAnimation);
        } else {
            mProgramInfoUpdatePendingByResizing = true;
        }
        setLastUpdatedProgram(program);
    }

    private void updateProgramInfo(RecordedProgram recordedProgram) {
        if (mLockType == LOCK_CHANNEL_INFO) {
            updateProgramInfo(sLockedChannelProgram);
            return;
        } else if (recordedProgram == null) {
            updateProgramInfo(sNoProgram);
            return;
        }

        if (mLastUpdatedRecordedProgram == null
                || !TextUtils.equals(recordedProgram.getTitle(),
                mLastUpdatedRecordedProgram.getTitle())
                || !TextUtils.equals(recordedProgram.getEpisodeDisplayTitle(getContext()),
                mLastUpdatedRecordedProgram.getEpisodeDisplayTitle(getContext()))) {
            updateProgramTextView(recordedProgram);
        }
        updateProgramTimeInfo(recordedProgram);

        // When the program is changed, but the previous resize animation has not ended yet,
        // cancel the animation.
        boolean isProgramChanged = !Objects.equals(mLastUpdatedRecordedProgram, recordedProgram);
        if (mResizeAnimator != null && isProgramChanged) {
            setLastUpdatedRecordedProgram(recordedProgram);
            mProgramInfoUpdatePendingByResizing = true;
            mResizeAnimator.cancel();
        } else if (mResizeAnimator == null) {
            if (mLockType != LOCK_NONE
                    || TextUtils.isEmpty(recordedProgram.getShortDescription())) {
                mProgramDescriptionTextView.setVisibility(GONE);
                mProgramDescriptionText = "";
            } else {
                mProgramDescriptionTextView.setVisibility(VISIBLE);
                mProgramDescriptionText = recordedProgram.getShortDescription();
            }
            String description = mProgramDescriptionTextView.getText().toString();
            boolean needFadeAnimation = isProgramChanged
                    || !description.equals(mProgramDescriptionText);
            updateBannerHeight(needFadeAnimation);
        } else {
            mProgramInfoUpdatePendingByResizing = true;
        }
        setLastUpdatedRecordedProgram(recordedProgram);
    }

    private void updateProgramTextView(Program program) {
        if (program == null) {
            return;
        }
        updateProgramTextView(program == sLockedChannelProgram, program.getTitle(),
                program.getEpisodeTitle(), program.getEpisodeDisplayTitle(getContext()));
    }

    private void updateProgramTextView(RecordedProgram recordedProgram) {
        if (recordedProgram == null) {
            return;
        }
        updateProgramTextView(false, recordedProgram.getTitle(), recordedProgram.getEpisodeTitle(),
                recordedProgram.getEpisodeDisplayTitle(getContext()));
    }

    private void updateProgramTextView(boolean dimText, String title, String episodeTitle,
            String episodeDisplayTitle) {
        mProgramTextView.setVisibility(View.VISIBLE);
        if (dimText) {
            mProgramTextView.setTextColor(mChannelBannerDimTextColor);
        } else {
            mProgramTextView.setTextColor(mChannelBannerTextColor);
        }
        updateTextView(mProgramTextView,
                R.dimen.channel_banner_program_large_text_size,
                R.dimen.channel_banner_program_large_margin_top);
        if (TextUtils.isEmpty(episodeTitle)) {
            mProgramTextView.setText(title);
        } else {
            String fullTitle = title + "  " + episodeDisplayTitle;

            SpannableString text = new SpannableString(fullTitle);
            text.setSpan(new TextAppearanceSpan(getContext(),
                            R.style.text_appearance_channel_banner_episode_title),
                    fullTitle.length() - episodeDisplayTitle.length(), fullTitle.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            mProgramTextView.setText(text);
        }
        int width = mProgramDescriptionTextViewWidth
                - ((mChannelLogoImageView.getVisibility() != View.VISIBLE)
                ? 0 : mChannelLogoImageViewWidth + mChannelLogoImageViewMarginStart);
        ViewGroup.LayoutParams lp = mProgramTextView.getLayoutParams();
        lp.width = width;
        mProgramTextView.setLayoutParams(lp);
        mProgramTextView.measure(MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));

        boolean oneline = (mProgramTextView.getLineCount() == 1);
        if (!oneline) {
            updateTextView(
                    mProgramTextView,
                    R.dimen.channel_banner_program_medium_text_size,
                    R.dimen.channel_banner_program_medium_margin_top);
            mProgramTextView.measure(MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY),
                    MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
            oneline = (mProgramTextView.getLineCount() == 1);
        }
        updateTopMargin(mAnchorView, oneline
                ? R.dimen.channel_banner_anchor_one_line_y
                : R.dimen.channel_banner_anchor_two_line_y);
    }

    private void updateProgramTimeInfo(Program program) {
        long startTime = program.getStartTimeUtcMillis();
        long endTime = program.getEndTimeUtcMillis();
        if (mLockType != LOCK_CHANNEL_INFO && startTime > 0 && endTime > startTime) {
            mProgramTimeTextView.setVisibility(View.VISIBLE);
            mRemainingTimeView.setVisibility(View.VISIBLE);

            mProgramTimeTextView.setText(Utils.getDurationString(
                    getContext(), startTime, endTime, true));

            long currTime = mMainActivity.getCurrentPlayingPosition();
            if (currTime <= startTime) {
                mRemainingTimeView.setProgress(0);
            } else if (currTime >= endTime) {
                mRemainingTimeView.setProgress(100);
            } else {
                mRemainingTimeView.setProgress(
                        (int) (100 * (currTime - startTime) / (endTime - startTime)));
            }
        } else {
            mProgramTimeTextView.setVisibility(View.GONE);
            mRemainingTimeView.setVisibility(View.GONE);
        }
    }

    private void updateProgramTimeInfo(RecordedProgram recordedProgram) {
        long durationMs = recordedProgram.getDurationMillis();
        if (mLockType != LOCK_CHANNEL_INFO && durationMs > 0) {
            mProgramTimeTextView.setVisibility(View.VISIBLE);
            mRemainingTimeView.setVisibility(View.VISIBLE);

            mProgramTimeTextView.setText(DateUtils.formatElapsedTime(durationMs / 1000));

            long currTimeMs = mMainActivity.getCurrentPlayingPosition();
            if (currTimeMs <= 0) {
                mRemainingTimeView.setProgress(0);
            } else if (currTimeMs >= durationMs) {
                mRemainingTimeView.setProgress(100);
            } else {
                mRemainingTimeView.setProgress((int) (100 * currTimeMs / durationMs));
            }
        } else {
            mProgramTimeTextView.setVisibility(View.GONE);
            mRemainingTimeView.setVisibility(View.GONE);
        }
    }

    private void setLastUpdatedProgram(Program program) {
        mLastUpdatedProgram = program;
        mLastUpdatedRecordedProgram = null;
    }

    private void setLastUpdatedRecordedProgram(RecordedProgram recordedProgram) {
        mLastUpdatedProgram = null;
        mLastUpdatedRecordedProgram = recordedProgram;
    }

    private void updateBannerHeight(boolean needFadeAnimation) {
        Assert.assertNull(mResizeAnimator);
        // Need to measure the layout height with the new description text.
        CharSequence oldDescription = mProgramDescriptionTextView.getText();
        mProgramDescriptionTextView.setText(mProgramDescriptionText);
        measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
        int targetHeight = getMeasuredHeight();

        if (mCurrentHeight == 0 || !isShown()) {
            // Do not add the resize animation when the banner has not been shown before.
            mCurrentHeight = targetHeight;
            LayoutParams layoutParams = (LayoutParams) getLayoutParams();
            if (targetHeight != layoutParams.height) {
                layoutParams.height = targetHeight;
                setLayoutParams(layoutParams);
            }
        } else if (mCurrentHeight != targetHeight || needFadeAnimation) {
            // Restore description text for fade in/out animation.
            if (needFadeAnimation) {
                mProgramDescriptionTextView.setText(oldDescription);
            }
            mResizeAnimator = createResizeAnimator(targetHeight, needFadeAnimation);
            mResizeAnimator.start();
        }
    }

    private Animator createResizeAnimator(int targetHeight, boolean addFadeAnimation) {
        final ValueAnimator heightAnimator = ValueAnimator.ofInt(mCurrentHeight, targetHeight);
        heightAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                int value = (Integer) animation.getAnimatedValue();
                LayoutParams layoutParams = (LayoutParams) ChannelBannerView.this.getLayoutParams();
                if (value != layoutParams.height) {
                    layoutParams.height = value;
                    ChannelBannerView.this.setLayoutParams(layoutParams);
                }
                mCurrentHeight = value;
            }
        });

        heightAnimator.setDuration(mResizeAnimDuration);
        heightAnimator.setInterpolator(mResizeInterpolator);

        if (!addFadeAnimation) {
            heightAnimator.addListener(mResizeAnimatorListener);
            return heightAnimator;
        }

        AnimatorSet fadeOutAndHeightAnimator = new AnimatorSet();
        fadeOutAndHeightAnimator.playTogether(mProgramDescriptionFadeOutAnimator, heightAnimator);
        AnimatorSet animator = new AnimatorSet();
        animator.playSequentially(fadeOutAndHeightAnimator, mProgramDescriptionFadeInAnimator);
        animator.addListener(mResizeAnimatorListener);
        return animator;
    }
}
