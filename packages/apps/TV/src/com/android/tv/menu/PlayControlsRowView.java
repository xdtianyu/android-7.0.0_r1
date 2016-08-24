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

package com.android.tv.menu;

import android.content.Context;
import android.content.res.Resources;
import android.text.format.DateFormat;
import android.text.format.DateUtils;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.android.tv.R;
import com.android.tv.TimeShiftManager;
import com.android.tv.TimeShiftManager.TimeShiftActionId;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.data.Program;
import com.android.tv.menu.Menu.MenuShowReason;

public class PlayControlsRowView extends MenuRowView {
    // Dimensions
    private final int mTimeIndicatorLeftMargin;
    private final int mTimeTextLeftMargin;
    private final int mTimelineWidth;
    // Views
    private View mBackgroundView;
    private View mTimeIndicator;
    private TextView mTimeText;
    private View mProgressEmptyBefore;
    private View mProgressWatched;
    private View mProgressBuffered;
    private View mProgressEmptyAfter;
    private View mControlBar;
    private PlayControlsButton mJumpPreviousButton;
    private PlayControlsButton mRewindButton;
    private PlayControlsButton mPlayPauseButton;
    private PlayControlsButton mFastForwardButton;
    private PlayControlsButton mJumpNextButton;
    private TextView mProgramStartTimeText;
    private TextView mProgramEndTimeText;
    private View mUnavailableMessageText;
    private TimeShiftManager mTimeShiftManager;

    private final java.text.DateFormat mTimeFormat;
    private long mProgramStartTimeMs;
    private long mProgramEndTimeMs;

    public PlayControlsRowView(Context context) {
        this(context, null);
    }

    public PlayControlsRowView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public PlayControlsRowView(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public PlayControlsRowView(Context context, AttributeSet attrs, int defStyleAttr,
            int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        Resources res = context.getResources();
        mTimeIndicatorLeftMargin =
                - res.getDimensionPixelSize(R.dimen.play_controls_time_indicator_width) / 2;
        mTimeTextLeftMargin =
                - res.getDimensionPixelOffset(R.dimen.play_controls_time_width) / 2;
        mTimelineWidth = res.getDimensionPixelSize(R.dimen.play_controls_width);
        mTimeFormat = DateFormat.getTimeFormat(context);
    }

    @Override
    protected int getContentsViewId() {
        return R.id.play_controls;
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        // Clip the ViewGroup(body) to the rounded rectangle of outline.
        findViewById(R.id.body).setClipToOutline(true);
        mBackgroundView = findViewById(R.id.background);
        mTimeIndicator = findViewById(R.id.time_indicator);
        mTimeText = (TextView) findViewById(R.id.time_text);
        mProgressEmptyBefore = findViewById(R.id.timeline_bg_start);
        mProgressWatched = findViewById(R.id.watched);
        mProgressBuffered = findViewById(R.id.buffered);
        mProgressEmptyAfter = findViewById(R.id.timeline_bg_end);
        mControlBar = findViewById(R.id.play_control_bar);
        mJumpPreviousButton = (PlayControlsButton) findViewById(R.id.jump_previous);
        mRewindButton = (PlayControlsButton) findViewById(R.id.rewind);
        mPlayPauseButton = (PlayControlsButton) findViewById(R.id.play_pause);
        mFastForwardButton = (PlayControlsButton) findViewById(R.id.fast_forward);
        mJumpNextButton = (PlayControlsButton) findViewById(R.id.jump_next);
        mProgramStartTimeText = (TextView) findViewById(R.id.program_start_time);
        mProgramEndTimeText = (TextView) findViewById(R.id.program_end_time);
        mUnavailableMessageText = findViewById(R.id.unavailable_text);

        initializeButton(mJumpPreviousButton, R.drawable.lb_ic_skip_previous,
                R.string.play_controls_description_skip_previous, new Runnable() {
            @Override
            public void run() {
                if (mTimeShiftManager.isAvailable()) {
                    mTimeShiftManager.jumpToPrevious();
                    updateAll();
                }
            }
        });
        initializeButton(mRewindButton, R.drawable.lb_ic_fast_rewind,
                R.string.play_controls_description_fast_rewind, new Runnable() {
            @Override
            public void run() {
                if (mTimeShiftManager.isAvailable()) {
                    mTimeShiftManager.rewind();
                    updateButtons();
                }
            }
        });
        initializeButton(mPlayPauseButton, R.drawable.lb_ic_play,
                R.string.play_controls_description_play_pause, new Runnable() {
            @Override
            public void run() {
                if (mTimeShiftManager.isAvailable()) {
                    mTimeShiftManager.togglePlayPause();
                    updateButtons();
                }
            }
        });
        initializeButton(mFastForwardButton, R.drawable.lb_ic_fast_forward,
                R.string.play_controls_description_fast_forward, new Runnable() {
            @Override
            public void run() {
                if (mTimeShiftManager.isAvailable()) {
                    mTimeShiftManager.fastForward();
                    updateButtons();
                }
            }
        });
        initializeButton(mJumpNextButton, R.drawable.lb_ic_skip_next,
                R.string.play_controls_description_skip_next, new Runnable() {
            @Override
            public void run() {
                if (mTimeShiftManager.isAvailable()) {
                    mTimeShiftManager.jumpToNext();
                    updateAll();
                }
            }
        });
    }

    private void initializeButton(PlayControlsButton button, int imageResId,
            int descriptionId, Runnable clickAction) {
        button.setImageResId(imageResId);
        button.setAction(clickAction);
        button.findViewById(R.id.button)
                .setContentDescription(getResources().getString(descriptionId));
    }

    @Override
    public void onBind(MenuRow row) {
        super.onBind(row);
        PlayControlsRow playControlsRow = (PlayControlsRow) row;
        mTimeShiftManager = playControlsRow.getTimeShiftManager();
        mTimeShiftManager.setListener(new TimeShiftManager.Listener() {
            @Override
            public void onAvailabilityChanged() {
                updateMenuVisibility();
                PlayControlsRowView.this.onAvailabilityChanged();
            }

            @Override
            public void onPlayStatusChanged(int status) {
                updateMenuVisibility();
                if (mTimeShiftManager.isAvailable()) {
                    updateAll();
                }
            }

            @Override
            public void onRecordTimeRangeChanged() {
                if (!mTimeShiftManager.isAvailable()) {
                    return;
                }
                updateAll();
            }

            @Override
            public void onCurrentPositionChanged() {
                if (!mTimeShiftManager.isAvailable()) {
                    return;
                }
                initializeTimeline();
                updateAll();
            }

            @Override
            public void onProgramInfoChanged() {
                if (!mTimeShiftManager.isAvailable()) {
                    return;
                }
                initializeTimeline();
                updateAll();
            }

            @Override
            public void onActionEnabledChanged(@TimeShiftActionId int actionId, boolean enabled) {
                // Move focus to the play/pause button when the PREVIOUS, NEXT, REWIND or
                // FAST_FORWARD button is clicked and the button becomes disabled.
                // No need to update the UI here because the UI will be updated by other callbacks.
                if (!enabled &&
                        ((actionId == TimeShiftManager.TIME_SHIFT_ACTION_ID_JUMP_TO_PREVIOUS
                                && mJumpPreviousButton.hasFocus())
                        || (actionId == TimeShiftManager.TIME_SHIFT_ACTION_ID_REWIND
                                && mRewindButton.hasFocus())
                        || (actionId == TimeShiftManager.TIME_SHIFT_ACTION_ID_FAST_FORWARD
                                && mFastForwardButton.hasFocus())
                        || (actionId == TimeShiftManager.TIME_SHIFT_ACTION_ID_JUMP_TO_NEXT
                                && mJumpNextButton.hasFocus()))) {
                    mPlayPauseButton.requestFocus();
                }
            }
        });
        onAvailabilityChanged();
    }

    private void onAvailabilityChanged() {
        if (mTimeShiftManager.isAvailable()) {
            setEnabled(true);
            initializeTimeline();
            mBackgroundView.setEnabled(true);
        } else {
            setEnabled(false);
            mBackgroundView.setEnabled(false);
        }
        updateAll();
    }

    private void initializeTimeline() {
        if (mTimeShiftManager.isRecordingPlayback()) {
            mProgramStartTimeMs = mTimeShiftManager.getRecordStartTimeMs();
            mProgramEndTimeMs = mTimeShiftManager.getRecordEndTimeMs();
        } else {
            Program program = mTimeShiftManager.getProgramAt(
                    mTimeShiftManager.getCurrentPositionMs());
            mProgramStartTimeMs = program.getStartTimeUtcMillis();
            mProgramEndTimeMs = program.getEndTimeUtcMillis();
        }
        SoftPreconditions.checkArgument(mProgramStartTimeMs <= mProgramEndTimeMs);
    }

    private void updateMenuVisibility() {
        boolean keepMenuVisible =
                mTimeShiftManager.isAvailable() && !mTimeShiftManager.isNormalPlaying();
        getMenu().setKeepVisible(keepMenuVisible);
    }

    @Override
    public void onSelected(boolean showTitle) {
        super.onSelected(showTitle);
        updateAll();
        postHideRippleAnimation();
    }

    @Override
    public void initialize(@MenuShowReason int reason) {
        super.initialize(reason);
        switch (reason) {
            case Menu.REASON_PLAY_CONTROLS_JUMP_TO_PREVIOUS:
                if (mTimeShiftManager.isActionEnabled(
                        TimeShiftManager.TIME_SHIFT_ACTION_ID_JUMP_TO_PREVIOUS)) {
                    setInitialFocusView(mJumpPreviousButton);
                } else {
                    setInitialFocusView(mPlayPauseButton);
                }
                break;
            case Menu.REASON_PLAY_CONTROLS_REWIND:
                if (mTimeShiftManager.isActionEnabled(
                        TimeShiftManager.TIME_SHIFT_ACTION_ID_REWIND)) {
                    setInitialFocusView(mRewindButton);
                } else {
                    setInitialFocusView(mPlayPauseButton);
                }
                break;
            case Menu.REASON_PLAY_CONTROLS_FAST_FORWARD:
                if (mTimeShiftManager.isActionEnabled(
                        TimeShiftManager.TIME_SHIFT_ACTION_ID_FAST_FORWARD)) {
                    setInitialFocusView(mFastForwardButton);
                } else {
                    setInitialFocusView(mPlayPauseButton);
                }
                break;
            case Menu.REASON_PLAY_CONTROLS_JUMP_TO_NEXT:
                if (mTimeShiftManager.isActionEnabled(
                        TimeShiftManager.TIME_SHIFT_ACTION_ID_JUMP_TO_NEXT)) {
                    setInitialFocusView(mJumpNextButton);
                } else {
                    setInitialFocusView(mPlayPauseButton);
                }
                break;
            case Menu.REASON_PLAY_CONTROLS_PLAY_PAUSE:
            case Menu.REASON_PLAY_CONTROLS_PLAY:
            case Menu.REASON_PLAY_CONTROLS_PAUSE:
            default:
                setInitialFocusView(mPlayPauseButton);
                break;
        }
        postHideRippleAnimation();
    }

    private void postHideRippleAnimation() {
        // Focus may be changed in another message if requestFocus is called in this message.
        // After the focus is actually changed, hideRippleAnimation should run
        // to reflect the result of the focus change. To be sure, hideRippleAnimation is posted.
        post(new Runnable() {
            @Override
            public void run() {
                mJumpPreviousButton.hideRippleAnimation();
                mRewindButton.hideRippleAnimation();
                mPlayPauseButton.hideRippleAnimation();
                mFastForwardButton.hideRippleAnimation();
                mJumpNextButton.hideRippleAnimation();
            }
        });
    }

    @Override
    protected void onChildFocusChange(View v, boolean hasFocus) {
        super.onChildFocusChange(v, hasFocus);
        if ((v.getParent().equals(mRewindButton) || v.getParent().equals(mFastForwardButton))
                && !hasFocus) {
            if (mTimeShiftManager.getPlayStatus() == TimeShiftManager.PLAY_STATUS_PLAYING) {
                mTimeShiftManager.play();
                updateButtons();
            }
        }
    }

    private void updateAll() {
        updateTime();
        updateProgress();
        updateRecTimeText();
        updateButtons();
    }

    private void updateTime() {
        if (isEnabled()) {
            mTimeText.setVisibility(View.VISIBLE);
            mTimeIndicator.setVisibility(View.VISIBLE);
        } else {
            mTimeText.setVisibility(View.INVISIBLE);
            mTimeIndicator.setVisibility(View.INVISIBLE);
            return;
        }
        long currentPositionMs = mTimeShiftManager.getCurrentPositionMs();
        ViewGroup.MarginLayoutParams params =
                (ViewGroup.MarginLayoutParams) mTimeText.getLayoutParams();
        int currentTimePositionPixel =
                convertDurationToPixel(currentPositionMs - mProgramStartTimeMs);
        params.leftMargin = currentTimePositionPixel + mTimeTextLeftMargin;
        mTimeText.setLayoutParams(params);
        mTimeText.setText(getTimeString(currentPositionMs));
        params = (ViewGroup.MarginLayoutParams) mTimeIndicator.getLayoutParams();
        params.leftMargin = currentTimePositionPixel + mTimeIndicatorLeftMargin;
        mTimeIndicator.setLayoutParams(params);
    }

    private void updateProgress() {
        if (isEnabled()) {
            mProgressWatched.setVisibility(View.VISIBLE);
            mProgressBuffered.setVisibility(View.VISIBLE);
            mProgressEmptyAfter.setVisibility(View.VISIBLE);
        } else {
            mProgressWatched.setVisibility(View.INVISIBLE);
            mProgressBuffered.setVisibility(View.INVISIBLE);
            mProgressEmptyAfter.setVisibility(View.INVISIBLE);
            if (mProgramStartTimeMs < mProgramEndTimeMs) {
                layoutProgress(mProgressEmptyBefore, mProgramStartTimeMs, mProgramEndTimeMs);
            } else {
                // Not initialized yet.
                layoutProgress(mProgressEmptyBefore, mTimelineWidth);
            }
            return;
        }

        long progressStartTimeMs = Math.min(mProgramEndTimeMs,
                    Math.max(mProgramStartTimeMs, mTimeShiftManager.getRecordStartTimeMs()));
        long currentPlayingTimeMs = Math.min(mProgramEndTimeMs,
                    Math.max(mProgramStartTimeMs, mTimeShiftManager.getCurrentPositionMs()));
        long progressEndTimeMs = Math.min(mProgramEndTimeMs,
                    Math.max(mProgramStartTimeMs, mTimeShiftManager.getRecordEndTimeMs()));

        layoutProgress(mProgressEmptyBefore, mProgramStartTimeMs, progressStartTimeMs);
        layoutProgress(mProgressWatched, progressStartTimeMs, currentPlayingTimeMs);
        layoutProgress(mProgressBuffered, currentPlayingTimeMs, progressEndTimeMs);
    }

    private void layoutProgress(View progress, long progressStartTimeMs, long progressEndTimeMs) {
        layoutProgress(progress, Math.max(0,
                convertDurationToPixel(progressEndTimeMs - progressStartTimeMs)) + 1);
    }

    private void layoutProgress(View progress, int width) {
        ViewGroup.MarginLayoutParams params =
                (ViewGroup.MarginLayoutParams) progress.getLayoutParams();
        params.width = width;
        progress.setLayoutParams(params);
    }

    private void updateRecTimeText() {
        if (isEnabled()) {
            if (mTimeShiftManager.isRecordingPlayback()) {
                mProgramStartTimeText.setVisibility(View.GONE);
            } else {
                mProgramStartTimeText.setVisibility(View.VISIBLE);
                mProgramStartTimeText.setText(getTimeString(mProgramStartTimeMs));
            }
            mProgramEndTimeText.setVisibility(View.VISIBLE);
            mProgramEndTimeText.setText(getTimeString(mProgramEndTimeMs));
        } else {
            mProgramStartTimeText.setVisibility(View.GONE);
            mProgramEndTimeText.setVisibility(View.GONE);
        }
    }

    private void updateButtons() {
        if (isEnabled()) {
            mControlBar.setVisibility(View.VISIBLE);
            mUnavailableMessageText.setVisibility(View.GONE);
        } else {
            mControlBar.setVisibility(View.INVISIBLE);
            mUnavailableMessageText.setVisibility(View.VISIBLE);
            return;
        }

        if (mTimeShiftManager.getPlayStatus() == TimeShiftManager.PLAY_STATUS_PAUSED) {
            mPlayPauseButton.setImageResId(R.drawable.lb_ic_play);
            mPlayPauseButton.setEnabled(mTimeShiftManager.isActionEnabled(
                    TimeShiftManager.TIME_SHIFT_ACTION_ID_PLAY));
        } else {
            mPlayPauseButton.setImageResId(R.drawable.lb_ic_pause);
            mPlayPauseButton.setEnabled(mTimeShiftManager.isActionEnabled(
                    TimeShiftManager.TIME_SHIFT_ACTION_ID_PAUSE));
        }
        mJumpPreviousButton.setEnabled(mTimeShiftManager.isActionEnabled(
                TimeShiftManager.TIME_SHIFT_ACTION_ID_JUMP_TO_PREVIOUS));
        mRewindButton.setEnabled(mTimeShiftManager.isActionEnabled(
                TimeShiftManager.TIME_SHIFT_ACTION_ID_REWIND));
        mFastForwardButton.setEnabled(mTimeShiftManager.isActionEnabled(
                TimeShiftManager.TIME_SHIFT_ACTION_ID_FAST_FORWARD));
        mJumpNextButton.setEnabled(mTimeShiftManager.isActionEnabled(
                TimeShiftManager.TIME_SHIFT_ACTION_ID_JUMP_TO_NEXT));

        PlayControlsButton button;
        if (mTimeShiftManager.getPlayDirection() == TimeShiftManager.PLAY_DIRECTION_FORWARD) {
            mRewindButton.setLabel(null);
            button = mFastForwardButton;
        } else {
            mFastForwardButton.setLabel(null);
            button = mRewindButton;
        }
        if (mTimeShiftManager.getDisplayedPlaySpeed() == TimeShiftManager.PLAY_SPEED_1X) {
            button.setLabel(null);
        } else {
            button.setLabel(getResources().getString(R.string.play_controls_speed,
                    mTimeShiftManager.getDisplayedPlaySpeed()));
        }
    }

    private String getTimeString(long timeMs) {
        return mTimeShiftManager.isRecordingPlayback()
                ? DateUtils.formatElapsedTime(timeMs / 1000)
                : mTimeFormat.format(timeMs);
    }

    private int convertDurationToPixel(long duration) {
        if (mProgramEndTimeMs <= mProgramStartTimeMs) {
            return 0;
        }
        return (int) (duration * mTimelineWidth / (mProgramEndTimeMs - mProgramStartTimeMs));
    }
}
