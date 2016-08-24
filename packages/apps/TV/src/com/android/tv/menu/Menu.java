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

import android.animation.Animator;
import android.animation.AnimatorInflater;
import android.animation.AnimatorListenerAdapter;
import android.content.Context;
import android.content.res.Resources;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import com.android.tv.ChannelTuner;
import com.android.tv.R;
import com.android.tv.TvApplication;
import com.android.tv.analytics.DurationTimer;
import com.android.tv.analytics.Tracker;
import com.android.tv.common.TvCommonUtils;
import com.android.tv.common.WeakHandler;
import com.android.tv.data.Channel;
import com.android.tv.menu.MenuRowFactory.PartnerRow;
import com.android.tv.menu.MenuRowFactory.PipOptionsRow;
import com.android.tv.menu.MenuRowFactory.TvOptionsRow;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

/**
 * A class which controls the menu.
 */
public class Menu {
    private static final String TAG = "Menu";
    private static final boolean DEBUG = false;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({REASON_NONE, REASON_GUIDE, REASON_PLAY_CONTROLS_PLAY, REASON_PLAY_CONTROLS_PAUSE,
        REASON_PLAY_CONTROLS_PLAY_PAUSE, REASON_PLAY_CONTROLS_REWIND,
        REASON_PLAY_CONTROLS_FAST_FORWARD, REASON_PLAY_CONTROLS_JUMP_TO_PREVIOUS,
        REASON_PLAY_CONTROLS_JUMP_TO_NEXT, REASON_RECORDING_PLAYBACK})
    public @interface MenuShowReason {}
    public static final int REASON_NONE = 0;
    public static final int REASON_GUIDE = 1;
    public static final int REASON_PLAY_CONTROLS_PLAY = 2;
    public static final int REASON_PLAY_CONTROLS_PAUSE = 3;
    public static final int REASON_PLAY_CONTROLS_PLAY_PAUSE = 4;
    public static final int REASON_PLAY_CONTROLS_REWIND = 5;
    public static final int REASON_PLAY_CONTROLS_FAST_FORWARD = 6;
    public static final int REASON_PLAY_CONTROLS_JUMP_TO_PREVIOUS = 7;
    public static final int REASON_PLAY_CONTROLS_JUMP_TO_NEXT = 8;
    public static final int REASON_RECORDING_PLAYBACK = 9;

    private static final List<String> sRowIdListForReason = new ArrayList<>();
    static {
        sRowIdListForReason.add(null);  // REASON_NONE
        sRowIdListForReason.add(ChannelsRow.ID);  // REASON_GUIDE
        sRowIdListForReason.add(PlayControlsRow.ID);  // REASON_PLAY_CONTROLS_PLAY
        sRowIdListForReason.add(PlayControlsRow.ID);  // REASON_PLAY_CONTROLS_PAUSE
        sRowIdListForReason.add(PlayControlsRow.ID);  // REASON_PLAY_CONTROLS_PLAY_PAUSE
        sRowIdListForReason.add(PlayControlsRow.ID);  // REASON_PLAY_CONTROLS_REWIND
        sRowIdListForReason.add(PlayControlsRow.ID);  // REASON_PLAY_CONTROLS_FAST_FORWARD
        sRowIdListForReason.add(PlayControlsRow.ID);  // REASON_PLAY_CONTROLS_JUMP_TO_PREVIOUS
        sRowIdListForReason.add(PlayControlsRow.ID);  // REASON_PLAY_CONTROLS_JUMP_TO_NEXT
        sRowIdListForReason.add(PlayControlsRow.ID);  // REASON_RECORDING_PLAYBACK
    }

    private static final String SCREEN_NAME = "Menu";

    private static final int MSG_HIDE_MENU = 1000;

    private final IMenuView mMenuView;
    private final Tracker mTracker;
    private final DurationTimer mVisibleTimer = new DurationTimer();
    private final long mShowDurationMillis;
    private final OnMenuVisibilityChangeListener mOnMenuVisibilityChangeListener;
    private final WeakHandler<Menu> mHandler = new MenuWeakHandler(this, Looper.getMainLooper());

    private final ChannelTuner.Listener mChannelTunerListener = new ChannelTuner.Listener() {
        @Override
        public void onLoadFinished() {}

        @Override
        public void onBrowsableChannelListChanged() {
            mMenuView.update(isActive());
        }

        @Override
        public void onCurrentChannelUnavailable(Channel channel) {}

        @Override
        public void onChannelChanged(Channel previousChannel, Channel currentChannel) {}
    };

    private final List<MenuRow> mMenuRows = new ArrayList<>();
    private final Animator mShowAnimator;
    private final Animator mHideAnimator;

    private ChannelTuner mChannelTuner;
    private boolean mKeepVisible;
    private boolean mAnimationDisabledForTest;

    /**
     * A constructor.
     */
    public Menu(Context context, IMenuView menuView, MenuRowFactory menuRowFactory,
                OnMenuVisibilityChangeListener onMenuVisibilityChangeListener) {
        mMenuView = menuView;
        mTracker = TvApplication.getSingletons(context).getTracker();
        Resources res = context.getResources();
        mShowDurationMillis = res.getInteger(R.integer.menu_show_duration);
        mOnMenuVisibilityChangeListener = onMenuVisibilityChangeListener;
        mShowAnimator = AnimatorInflater.loadAnimator(context, R.animator.menu_enter);
        mShowAnimator.setTarget(mMenuView);
        mHideAnimator = AnimatorInflater.loadAnimator(context, R.animator.menu_exit);
        mHideAnimator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                hideInternal();
            }
        });
        mHideAnimator.setTarget(mMenuView);
        // Build menu rows
        addMenuRow(menuRowFactory.createMenuRow(this, PlayControlsRow.class));
        addMenuRow(menuRowFactory.createMenuRow(this, ChannelsRow.class));
        addMenuRow(menuRowFactory.createMenuRow(this, PartnerRow.class));
        addMenuRow(menuRowFactory.createMenuRow(this, TvOptionsRow.class));
        addMenuRow(menuRowFactory.createMenuRow(this, PipOptionsRow.class));
        mMenuView.setMenuRows(mMenuRows);
    }

    /**
     * Sets the instance of {@link ChannelTuner}. Call this method when the channel tuner is ready
     * or not available any more.
     */
    public void setChannelTuner(ChannelTuner channelTuner) {
        if (mChannelTuner != null) {
            mChannelTuner.removeListener(mChannelTunerListener);
        }
        mChannelTuner = channelTuner;
        if (mChannelTuner != null) {
            mChannelTuner.addListener(mChannelTunerListener);
        }
        mMenuView.update(isActive());
    }

    private void addMenuRow(MenuRow row) {
        if (row != null) {
            mMenuRows.add(row);
        }
    }

    /**
     * Call this method to end the lifetime of the menu.
     */
    public void release() {
        setChannelTuner(null);
        for (MenuRow row : mMenuRows) {
            row.release();
        }
        mHandler.removeCallbacksAndMessages(null);
    }

    /**
     * Shows the main menu.
     *
     * @param reason A reason why this is called. See {@link MenuShowReason}
     */
    public void show(@MenuShowReason int reason) {
        if (DEBUG) Log.d(TAG, "show reason:" + reason);
        mTracker.sendShowMenu();
        mVisibleTimer.start();
        mTracker.sendScreenView(SCREEN_NAME);
        if (mHideAnimator.isStarted()) {
            mHideAnimator.end();
        }
        if (mOnMenuVisibilityChangeListener != null) {
            mOnMenuVisibilityChangeListener.onMenuVisibilityChange(true);
        }
        String rowIdToSelect = sRowIdListForReason.get(reason);
        mMenuView.onShow(reason, rowIdToSelect, mAnimationDisabledForTest ? null : new Runnable() {
            @Override
            public void run() {
                mShowAnimator.start();
            }
        });
        scheduleHide();
    }

    /**
     * Closes the menu.
     */
    public void hide(boolean withAnimation) {
        if (!isActive()) {
            return;
        }
        if (mAnimationDisabledForTest) {
            withAnimation = false;
        }
        mHandler.removeMessages(MSG_HIDE_MENU);
        if (withAnimation) {
            if (!mHideAnimator.isStarted()) {
                mHideAnimator.start();
            }
        } else if (mHideAnimator.isStarted()) {
            // mMenuView.onHide() is called in AnimatorListener.
            mHideAnimator.end();
        } else {
            hideInternal();
        }
    }

    private void hideInternal() {
        mMenuView.onHide();
        mTracker.sendHideMenu(mVisibleTimer.reset());
        if (mOnMenuVisibilityChangeListener != null) {
            mOnMenuVisibilityChangeListener.onMenuVisibilityChange(false);
        }
    }

    /**
     * Schedules to hide the menu in some seconds.
     */
    public void scheduleHide() {
        mHandler.removeMessages(MSG_HIDE_MENU);
        if (!mKeepVisible) {
            mHandler.sendEmptyMessageDelayed(MSG_HIDE_MENU, mShowDurationMillis);
        }
    }

    /**
     * Called when the caller wants the main menu to be kept visible or not.
     * If {@code keepVisible} is set to {@code true}, the hide schedule doesn't close the main menu,
     * but calling {@link #hide} still hides it.
     * If {@code keepVisible} is set to {@code false}, the hide schedule works as usual.
     */
    public void setKeepVisible(boolean keepVisible) {
        mKeepVisible = keepVisible;
        if (mKeepVisible) {
            mHandler.removeMessages(MSG_HIDE_MENU);
        } else if (isActive()) {
            scheduleHide();
        }
    }

    @VisibleForTesting
    boolean isHideScheduled() {
        return mHandler.hasMessages(MSG_HIDE_MENU);
    }

    /**
     * Returns {@code true} if the menu is open and not hiding.
     */
    public boolean isActive() {
        return mMenuView.isVisible() && !mHideAnimator.isStarted();
    }

    /**
     * Updates menu contents.
     *
     * <p>Returns <@code true> if the contents have been changed, otherwise {@code false}.
     */
    public boolean update() {
        if (DEBUG) Log.d(TAG, "update main menu");
        return mMenuView.update(isActive());
    }

    /**
     * This method is called when channels are changed.
     */
    public void onRecentChannelsChanged() {
        if (DEBUG) Log.d(TAG, "onRecentChannelsChanged");
        for (MenuRow row : mMenuRows) {
            row.onRecentChannelsChanged();
        }
    }

    /**
     * This method is called when the stream information is changed.
     */
    public void onStreamInfoChanged() {
        if (DEBUG) Log.d(TAG, "update options row in main menu");
        for (MenuRow row : mMenuRows) {
            row.onStreamInfoChanged();
        }
    }

    @VisibleForTesting
    void disableAnimationForTest() {
        if (!TvCommonUtils.isRunningInTest()) {
            throw new RuntimeException("Animation may only be enabled/disabled during tests.");
        }
        mAnimationDisabledForTest = true;
    }

    /**
     * A listener which receives the notification when the menu is visible/invisible.
     */
    public static abstract class OnMenuVisibilityChangeListener {
        /**
         * Called when the menu becomes visible/invisible.
         */
        public abstract void onMenuVisibilityChange(boolean visible);
    }

    private static class MenuWeakHandler extends WeakHandler<Menu> {
        public MenuWeakHandler(Menu menu, Looper mainLooper) {
            super(mainLooper, menu);
        }

        @Override
        public void handleMessage(Message msg, @NonNull Menu menu) {
            if (msg.what == MSG_HIDE_MENU) {
                menu.hide(true);
            }
        }
    }
}
