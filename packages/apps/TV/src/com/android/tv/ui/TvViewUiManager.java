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
import android.animation.ArgbEvaluator;
import android.animation.ObjectAnimator;
import android.animation.TimeInterpolator;
import android.animation.TypeEvaluator;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Point;
import android.hardware.display.DisplayManager;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.Log;
import android.util.Property;
import android.view.Display;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.ViewGroup.MarginLayoutParams;
import android.view.animation.AnimationUtils;
import android.widget.FrameLayout;

import com.android.tv.R;
import com.android.tv.TvOptionsManager;
import com.android.tv.data.DisplayMode;
import com.android.tv.util.TvSettings;
import com.android.tv.util.Utils;

/**
 * The TvViewUiManager is responsible for handling UI layouting and animation of main and PIP
 * TvViews. It also control the settings regarding TvView UI such as display mode, PIP layout,
 * and PIP size.
 */
public class TvViewUiManager {
    private static final String TAG = "TvViewManager";
    private static final boolean DEBUG = false;

    private static final float DISPLAY_MODE_EPSILON = 0.001f;
    private static final float DISPLAY_ASPECT_RATIO_EPSILON = 0.01f;

    private final Context mContext;
    private final Resources mResources;
    private final FrameLayout mContentView;
    private final TunableTvView mTvView;
    private final TunableTvView mPipView;
    private final TvOptionsManager mTvOptionsManager;
    private final int mTvViewPapWidth;
    private final int mTvViewShrunkenStartMargin;
    private final int mTvViewShrunkenEndMargin;
    private final int mTvViewPapStartMargin;
    private final int mTvViewPapEndMargin;
    private int mWindowWidth;
    private int mWindowHeight;
    private final int mPipViewHorizontalMargin;
    private final int mPipViewTopMargin;
    private final int mPipViewBottomMargin;
    private final SharedPreferences mSharedPreferences;
    private final TimeInterpolator mLinearOutSlowIn;
    private final TimeInterpolator mFastOutLinearIn;
    private final Handler mHandler = new Handler();
    private int mDisplayMode;
    // Used to restore the previous state from ShrunkenTvView state.
    private int mTvViewStartMarginBeforeShrunken;
    private int mTvViewEndMarginBeforeShrunken;
    private int mDisplayModeBeforeShrunken;
    private boolean mIsUnderShrunkenTvView;
    private int mTvViewStartMargin;
    private int mTvViewEndMargin;
    private int mPipLayout;
    private int mPipSize;
    private boolean mPipStarted;
    private ObjectAnimator mTvViewAnimator;
    private FrameLayout.LayoutParams mTvViewLayoutParams;
    // TV view's position when the display mode is FULL. It is used to compute PIP location relative
    // to TV view's position.
    private MarginLayoutParams mTvViewFrame;
    private MarginLayoutParams mLastAnimatedTvViewFrame;
    private MarginLayoutParams mOldTvViewFrame;
    private ObjectAnimator mBackgroundAnimator;
    private int mBackgroundColor;
    private int mAppliedDisplayedMode = DisplayMode.MODE_NOT_DEFINED;
    private int mAppliedTvViewStartMargin;
    private int mAppliedTvViewEndMargin;
    private float mAppliedVideoDisplayAspectRatio;

    public TvViewUiManager(Context context, TunableTvView tvView, TunableTvView pipView,
            FrameLayout contentView, TvOptionsManager tvOptionManager) {
        mContext = context;
        mResources = mContext.getResources();
        mTvView = tvView;
        mPipView = pipView;
        mContentView = contentView;
        mTvOptionsManager = tvOptionManager;

        DisplayManager displayManager = (DisplayManager) mContext
                .getSystemService(Context.DISPLAY_SERVICE);
        Display display = displayManager.getDisplay(Display.DEFAULT_DISPLAY);
        Point size = new Point();
        display.getSize(size);
        mWindowWidth = size.x;
        mWindowHeight = size.y;

        // Have an assumption that PIP and TvView Shrinking happens only in full screen.
        mTvViewShrunkenStartMargin = mResources
                .getDimensionPixelOffset(R.dimen.shrunken_tvview_margin_start);
        mTvViewShrunkenEndMargin =
                mResources.getDimensionPixelOffset(R.dimen.shrunken_tvview_margin_end)
                        + mResources.getDimensionPixelSize(R.dimen.side_panel_width);
        int papMarginHorizontal = mResources
                .getDimensionPixelOffset(R.dimen.papview_margin_horizontal);
        int papSpacing = mResources.getDimensionPixelOffset(R.dimen.papview_spacing);
        mTvViewPapWidth = (mWindowWidth - papSpacing) / 2 - papMarginHorizontal;
        mTvViewPapStartMargin = papMarginHorizontal + mTvViewPapWidth + papSpacing;
        mTvViewPapEndMargin = papMarginHorizontal;
        mTvViewFrame = createMarginLayoutParams(0, 0, 0, 0);

        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mContext);

        mLinearOutSlowIn = AnimationUtils
                .loadInterpolator(mContext, android.R.interpolator.linear_out_slow_in);
        mFastOutLinearIn = AnimationUtils
                .loadInterpolator(mContext, android.R.interpolator.fast_out_linear_in);

        mPipViewHorizontalMargin = mResources
                .getDimensionPixelOffset(R.dimen.pipview_margin_horizontal);
        mPipViewTopMargin = mResources.getDimensionPixelOffset(R.dimen.pipview_margin_top);
        mPipViewBottomMargin = mResources.getDimensionPixelOffset(R.dimen.pipview_margin_bottom);
        mContentView.addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
            @Override
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                int windowWidth = right - left;
                int windowHeight = bottom - top;
                if (windowWidth > 0 && windowHeight > 0) {
                    if (mWindowWidth != windowWidth || mWindowHeight != windowHeight) {
                        mWindowWidth = windowWidth;
                        mWindowHeight = windowHeight;
                        applyDisplayMode(mTvView.getVideoDisplayAspectRatio(), false, true);
                    }
                }
            }
        });
    }

    /**
     * Initializes animator in advance of using the animator to improve animation performance.
     * For fast first tune, it is not expected to be called in Activity.onCreate, but called
     * a few seconds later after onCreate.
     */
    public void initAnimatorIfNeeded() {
        initTvAnimatorIfNeeded();
        initBackgroundAnimatorIfNeeded();
    }

    /**
     * It is called when shrunken TvView is desired, such as EditChannelFragment and
     * ChannelsLockedFragment.
     */
    public void startShrunkenTvView() {
        mIsUnderShrunkenTvView = true;

        mTvViewStartMarginBeforeShrunken = mTvViewStartMargin;
        mTvViewEndMarginBeforeShrunken = mTvViewEndMargin;
        if (mPipStarted && getPipLayout() == TvSettings.PIP_LAYOUT_SIDE_BY_SIDE) {
            float sidePanelWidth = mResources.getDimensionPixelOffset(R.dimen.side_panel_width);
            float factor = 1.0f - sidePanelWidth / mWindowWidth;
            int startMargin = (int) (mTvViewPapStartMargin * factor);
            int endMargin = (int) (mTvViewPapEndMargin * factor + sidePanelWidth);
            setTvViewMargin(startMargin, endMargin);
        } else {
            setTvViewMargin(mTvViewShrunkenStartMargin, mTvViewShrunkenEndMargin);
        }
        mDisplayModeBeforeShrunken = setDisplayMode(DisplayMode.MODE_NORMAL, false, true);
    }

    /**
     * It is called when shrunken TvView is no longer desired, such as EditChannelFragment and
     * ChannelsLockedFragment.
     */
    public void endShrunkenTvView() {
        mIsUnderShrunkenTvView = false;
        setTvViewMargin(mTvViewStartMarginBeforeShrunken, mTvViewEndMarginBeforeShrunken);
        setDisplayMode(mDisplayModeBeforeShrunken, false, true);
    }

    /**
     * Returns true, if TvView is shrunken.
     */
    public boolean isUnderShrunkenTvView() {
        return mIsUnderShrunkenTvView;
    }

    /**
     * Returns true, if {@code displayMode} is available now. If screen ratio is matched to
     * video ratio, other display modes than {@link DisplayMode#MODE_NORMAL} are not available.
     */
    public boolean isDisplayModeAvailable(int displayMode) {
        if (displayMode == DisplayMode.MODE_NORMAL) {
            return true;
        }

        int viewWidth = mContentView.getWidth();
        int viewHeight = mContentView.getHeight();

        float videoDisplayAspectRatio = mTvView.getVideoDisplayAspectRatio();
        if (viewWidth <= 0 || viewHeight <= 0 || videoDisplayAspectRatio <= 0f) {
            Log.w(TAG, "Video size is currently unavailable");
            if (DEBUG) {
                Log.d(TAG, "isDisplayModeAvailable: "
                        + "viewWidth=" + viewWidth
                        + ", viewHeight=" + viewHeight
                        + ", videoDisplayAspectRatio=" + videoDisplayAspectRatio
                );
            }
            return false;
        }

        float viewRatio = viewWidth / (float) viewHeight;
        return Math.abs(viewRatio - videoDisplayAspectRatio) >= DISPLAY_MODE_EPSILON;
    }

    /**
     * Returns a constant defined in DisplayMode.
     */
    public int getDisplayMode() {
        if (isDisplayModeAvailable(mDisplayMode)) {
            return mDisplayMode;
        }
        return DisplayMode.MODE_NORMAL;
    }

    /**
     * Sets the display mode to the given value.
     *
     * @return the previous display mode.
     */
    public int setDisplayMode(int displayMode, boolean storeInPreference, boolean animate) {
        int prev = mDisplayMode;
        mDisplayMode = displayMode;
        if (storeInPreference) {
            mSharedPreferences.edit().putInt(TvSettings.PREF_DISPLAY_MODE, displayMode).apply();
        }
        applyDisplayMode(mTvView.getVideoDisplayAspectRatio(), animate, false);
        return prev;
    }

    /**
     * Restores the display mode to the display mode stored in preference.
     */
    public void restoreDisplayMode(boolean animate) {
        int displayMode = mSharedPreferences
                .getInt(TvSettings.PREF_DISPLAY_MODE, DisplayMode.MODE_NORMAL);
        setDisplayMode(displayMode, false, animate);
    }

    /**
     * Updates TvView. It is called when video resolution is updated.
     */
    public void updateTvView() {
        applyDisplayMode(mTvView.getVideoDisplayAspectRatio(), false, false);
        if (mTvView.isVideoAvailable() && mTvView.isFadedOut()) {
            mTvView.fadeIn(mResources.getInteger(R.integer.tvview_fade_in_duration),
                    mFastOutLinearIn, null);
        }
    }

    /**
     * Fades in TvView.
     */
    public void fadeInTvView() {
        if (mTvView.isFadedOut()) {
            mTvView.fadeIn(mResources.getInteger(R.integer.tvview_fade_in_duration),
                    mFastOutLinearIn, null);
        }
    }

    /**
     * Fades out TvView.
     */
    public void fadeOutTvView(Runnable postAction) {
        if (!mTvView.isFadedOut()) {
            mTvView.fadeOut(mResources.getInteger(R.integer.tvview_fade_out_duration),
                    mLinearOutSlowIn, postAction);
        }
    }

    /**
     * Returns the current PIP layout. The layout should be one of
     * {@link TvSettings#PIP_LAYOUT_BOTTOM_RIGHT}, {@link TvSettings#PIP_LAYOUT_TOP_RIGHT},
     * {@link TvSettings#PIP_LAYOUT_TOP_LEFT}, {@link TvSettings#PIP_LAYOUT_BOTTOM_LEFT} and
     * {@link TvSettings#PIP_LAYOUT_SIDE_BY_SIDE}.
     */
    public int getPipLayout() {
        return mPipLayout;
    }

    /**
     * Sets the PIP layout. The layout should be one of
     * {@link TvSettings#PIP_LAYOUT_BOTTOM_RIGHT}, {@link TvSettings#PIP_LAYOUT_TOP_RIGHT},
     * {@link TvSettings#PIP_LAYOUT_TOP_LEFT}, {@link TvSettings#PIP_LAYOUT_BOTTOM_LEFT} and
     * {@link TvSettings#PIP_LAYOUT_SIDE_BY_SIDE}.
     *
     * @param storeInPreference if true, the stored value will be restored by
     *                          {@link #restorePipLayout()}.
     */
    public void setPipLayout(int pipLayout, boolean storeInPreference) {
        mPipLayout = pipLayout;
        if (storeInPreference) {
            TvSettings.setPipLayout(mContext, pipLayout);
        }
        updatePipView(mTvViewFrame);
        if (mPipLayout == TvSettings.PIP_LAYOUT_SIDE_BY_SIDE) {
            setTvViewMargin(mTvViewPapStartMargin, mTvViewPapEndMargin);
            setDisplayMode(DisplayMode.MODE_NORMAL, false, false);
        } else {
            setTvViewMargin(0, 0);
            restoreDisplayMode(false);
        }
        mTvOptionsManager.onPipLayoutChanged(pipLayout);
    }

    /**
     * Restores the PIP layout which {@link #setPipLayout} lastly stores.
     */
    public void restorePipLayout() {
        setPipLayout(TvSettings.getPipLayout(mContext), false);
    }

    /**
     * Called when PIP is started.
     */
    public void onPipStart() {
        mPipStarted = true;
        updatePipView();
        mPipView.setVisibility(View.VISIBLE);
    }

    /**
     * Called when PIP is stopped.
     */
    public void onPipStop() {
        setTvViewMargin(0, 0);
        mPipView.setVisibility(View.GONE);
        mPipStarted = false;
    }

    /**
     * Called when PIP is resumed.
     */
    public void showPipForResume() {
        mPipView.setVisibility(View.VISIBLE);
    }

    /**
     * Called when PIP is paused.
     */
    public void hidePipForPause() {
        if (mPipLayout != TvSettings.PIP_LAYOUT_SIDE_BY_SIDE) {
            mPipView.setVisibility(View.GONE);
        }
    }

    /**
     * Updates PIP view. It is usually called, when video resolution in PIP is updated.
     */
    public void updatePipView() {
        updatePipView(mTvViewFrame);
    }

    /**
     * Returns the size of the PIP view.
     */
    public int getPipSize() {
        return mPipSize;
    }

    /**
     * Sets PIP size and applies it immediately.
     *
     * @param pipSize           PIP size. The value should be one of {@link TvSettings#PIP_SIZE_BIG}
     *                          and {@link TvSettings#PIP_SIZE_SMALL}.
     * @param storeInPreference if true, the stored value will be restored by
     *                          {@link #restorePipSize()}.
     */
    public void setPipSize(int pipSize, boolean storeInPreference) {
        mPipSize = pipSize;
        if (storeInPreference) {
            TvSettings.setPipSize(mContext, pipSize);
        }
        updatePipView(mTvViewFrame);
        mTvOptionsManager.onPipSizeChanged(pipSize);
    }

    /**
     * Restores the PIP size which {@link #setPipSize} lastly stores.
     */
    public void restorePipSize() {
        setPipSize(TvSettings.getPipSize(mContext), false);
    }

    /**
     * This margins will be applied when applyDisplayMode is called.
     */
    private void setTvViewMargin(int tvViewStartMargin, int tvViewEndMargin) {
        mTvViewStartMargin = tvViewStartMargin;
        mTvViewEndMargin = tvViewEndMargin;
    }

    private boolean isTvViewFullScreen() {
        return mTvViewStartMargin == 0 && mTvViewEndMargin == 0;
    }

    private void setBackgroundColor(int color, FrameLayout.LayoutParams targetLayoutParams,
            boolean animate) {
        if (animate) {
            initBackgroundAnimatorIfNeeded();
            if (mBackgroundAnimator.isStarted()) {
                // Cancel the current animation and start new one.
                mBackgroundAnimator.cancel();
            }

            int decorViewWidth = mContentView.getWidth();
            int decorViewHeight = mContentView.getHeight();
            boolean hasPillarBox = mTvView.getWidth() != decorViewWidth
                    || mTvView.getHeight() != decorViewHeight;
            boolean willHavePillarBox = ((targetLayoutParams.width != LayoutParams.MATCH_PARENT)
                    && targetLayoutParams.width != decorViewWidth) || (
                    (targetLayoutParams.height != LayoutParams.MATCH_PARENT)
                            && targetLayoutParams.height != decorViewHeight);

            if (!isTvViewFullScreen() && !hasPillarBox) {
                // If there is no pillar box, no animation is needed.
                mContentView.setBackgroundColor(color);
            } else if (!isTvViewFullScreen() || willHavePillarBox) {
                mBackgroundAnimator.setIntValues(mBackgroundColor, color);
                mBackgroundAnimator.setEvaluator(new ArgbEvaluator());
                mBackgroundAnimator.setInterpolator(mFastOutLinearIn);
                mBackgroundAnimator.start();
            }
            // In the 'else' case (TV activity is getting out of the shrunken tv view mode and will
            // have a pillar box), we keep the background color and don't show the animation.
        } else {
            mContentView.setBackgroundColor(color);
        }
        mBackgroundColor = color;
    }

    private void setTvViewPosition(final FrameLayout.LayoutParams layoutParams,
            MarginLayoutParams tvViewFrame, boolean animate) {
        if (DEBUG) {
            Log.d(TAG, "setTvViewPosition: w=" + layoutParams.width + " h=" + layoutParams.height
                    + " s=" + layoutParams.getMarginStart() + " t=" + layoutParams.topMargin
                    + " e=" + layoutParams.getMarginEnd() + " b=" + layoutParams.bottomMargin
                    + " animate=" + animate);
        }
        MarginLayoutParams oldTvViewFrame = mTvViewFrame;
        mTvViewLayoutParams = layoutParams;
        mTvViewFrame = tvViewFrame;
        if (animate) {
            initTvAnimatorIfNeeded();
            if (mTvViewAnimator.isStarted()) {
                // Cancel the current animation and start new one.
                mTvViewAnimator.cancel();
                mOldTvViewFrame = mLastAnimatedTvViewFrame;
            } else {
                mOldTvViewFrame = oldTvViewFrame;
            }
            mTvViewAnimator.setObjectValues(mTvView.getLayoutParams(), layoutParams);
            mTvViewAnimator.setEvaluator(new TypeEvaluator<FrameLayout.LayoutParams>() {
                FrameLayout.LayoutParams lp;
                @Override
                public FrameLayout.LayoutParams evaluate(float fraction,
                        FrameLayout.LayoutParams startValue, FrameLayout.LayoutParams endValue) {
                    if (lp == null) {
                        lp = new FrameLayout.LayoutParams(0, 0);
                        lp.gravity = startValue.gravity;
                    }
                    interpolateMarginsRelative(lp, startValue, endValue, fraction);
                    return lp;
                }
            });
            mTvViewAnimator
                    .setInterpolator(isTvViewFullScreen() ? mFastOutLinearIn : mLinearOutSlowIn);
            mTvViewAnimator.start();
        } else {
            if (mTvViewAnimator != null && mTvViewAnimator.isStarted()) {
                // Continue the current animation.
                // layoutParams will be applied when animation ends.
                return;
            }
            // This block is also called when animation ends.
            if (isTvViewFullScreen()) {
                // When this layout is for full screen, fix the surface size after layout to make
                // resize animation smooth.
                mTvView.post(new Runnable() {
                    @Override
                    public void run() {
                        if (DEBUG) {
                            Log.d(TAG, "setFixedSize: w=" + layoutParams.width + " h="
                                    + layoutParams.height);
                        }
                        mTvView.setLayoutParams(layoutParams);
                        mTvView.setFixedSurfaceSize(layoutParams.width, layoutParams.height);
                    }
                });
            } else {
                mTvView.setLayoutParams(layoutParams);
            }
            updatePipView(mTvViewFrame);
        }
    }

    /**
     * The redlines assume that the ratio of the TV screen is 16:9. If the radio is not 16:9, the
     * layout of PAP can be broken.
     */
    @SuppressLint("RtlHardcoded")
    private void updatePipView(MarginLayoutParams tvViewFrame) {
        if (!mPipStarted) {
            return;
        }
        int width;
        int height;
        int startMargin;
        int endMargin;
        int topMargin;
        int bottomMargin;
        int gravity;

        if (mPipLayout == TvSettings.PIP_LAYOUT_SIDE_BY_SIDE) {
            gravity = Gravity.CENTER_VERTICAL | Gravity.START;
            height = tvViewFrame.height;
            float videoDisplayAspectRatio = mPipView.getVideoDisplayAspectRatio();
            if (videoDisplayAspectRatio <= 0f) {
                width = tvViewFrame.width;
            } else {
                width = (int) (height * videoDisplayAspectRatio);
                if (width > tvViewFrame.width) {
                    width = tvViewFrame.width;
                }
            }
            startMargin = mResources.getDimensionPixelOffset(R.dimen.papview_margin_horizontal)
                    * tvViewFrame.width / mTvViewPapWidth + (tvViewFrame.width - width) / 2;
            endMargin = 0;
            topMargin = 0;
            bottomMargin = 0;
        } else {
            int tvViewWidth = tvViewFrame.width;
            int tvViewHeight = tvViewFrame.height;
            int tvStartMargin = tvViewFrame.getMarginStart();
            int tvEndMargin = tvViewFrame.getMarginEnd();
            int tvTopMargin = tvViewFrame.topMargin;
            int tvBottomMargin = tvViewFrame.bottomMargin;
            float horizontalScaleFactor = (float) tvViewWidth / mWindowWidth;
            float verticalScaleFactor = (float) tvViewHeight / mWindowHeight;

            int maxWidth;
            if (mPipSize == TvSettings.PIP_SIZE_SMALL) {
                maxWidth = (int) (mResources.getDimensionPixelSize(R.dimen.pipview_small_size_width)
                        * horizontalScaleFactor);
                height = (int) (mResources.getDimensionPixelSize(R.dimen.pipview_small_size_height)
                        * verticalScaleFactor);
            } else if (mPipSize == TvSettings.PIP_SIZE_BIG) {
                maxWidth = (int) (mResources.getDimensionPixelSize(R.dimen.pipview_large_size_width)
                        * horizontalScaleFactor);
                height = (int) (mResources.getDimensionPixelSize(R.dimen.pipview_large_size_height)
                        * verticalScaleFactor);
            } else {
                throw new IllegalArgumentException("Invalid PIP size: " + mPipSize);
            }
            float videoDisplayAspectRatio = mPipView.getVideoDisplayAspectRatio();
            if (videoDisplayAspectRatio <= 0f) {
                width = maxWidth;
            } else {
                width = (int) (height * videoDisplayAspectRatio);
                if (width > maxWidth) {
                    width = maxWidth;
                }
            }

            startMargin = tvStartMargin + (int) (mPipViewHorizontalMargin * horizontalScaleFactor);
            endMargin = tvEndMargin + (int) (mPipViewHorizontalMargin * horizontalScaleFactor);
            topMargin = tvTopMargin + (int) (mPipViewTopMargin * verticalScaleFactor);
            bottomMargin = tvBottomMargin + (int) (mPipViewBottomMargin * verticalScaleFactor);

            switch (mPipLayout) {
                case TvSettings.PIP_LAYOUT_TOP_LEFT:
                    gravity = Gravity.TOP | Gravity.LEFT;
                    break;
                case TvSettings.PIP_LAYOUT_TOP_RIGHT:
                    gravity = Gravity.TOP | Gravity.RIGHT;
                    break;
                case TvSettings.PIP_LAYOUT_BOTTOM_LEFT:
                    gravity = Gravity.BOTTOM | Gravity.LEFT;
                    break;
                case TvSettings.PIP_LAYOUT_BOTTOM_RIGHT:
                    gravity = Gravity.BOTTOM | Gravity.RIGHT;
                    break;
                default:
                    throw new IllegalArgumentException("Invalid PIP location: " + mPipLayout);
            }
        }

        FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams) mPipView.getLayoutParams();
        if (lp.width != width || lp.height != height || lp.getMarginStart() != startMargin
                || lp.getMarginEnd() != endMargin || lp.topMargin != topMargin
                || lp.bottomMargin != bottomMargin || lp.gravity != gravity) {
            lp.width = width;
            lp.height = height;
            lp.setMarginStart(startMargin);
            lp.setMarginEnd(endMargin);
            lp.topMargin = topMargin;
            lp.bottomMargin = bottomMargin;
            lp.gravity = gravity;
            mPipView.setLayoutParams(lp);
        }
    }

    private void initTvAnimatorIfNeeded() {
        if (mTvViewAnimator != null) {
            return;
        }

        // TvViewAnimator animates TvView by repeatedly re-layouting TvView.
        // TvView includes a SurfaceView on which scale/translation effects do not work. Normally,
        // SurfaceView can be animated by changing left/top/right/bottom directly using
        // ObjectAnimator, although it would require calling getChildAt(0) against TvView (which is
        // supposed to be opaque). More importantly, this method does not work in case of TvView,
        // because TvView may request layout itself during animation and layout SurfaceView with
        // its own parameters when TvInputService requests to do so.
        mTvViewAnimator = new ObjectAnimator();
        mTvViewAnimator.setTarget(mTvView);
        mTvViewAnimator.setProperty(
                Property.of(FrameLayout.class, ViewGroup.LayoutParams.class, "layoutParams"));
        mTvViewAnimator.setDuration(mResources.getInteger(R.integer.tvview_anim_duration));
        mTvViewAnimator.addListener(new AnimatorListenerAdapter() {
            private boolean mCanceled = false;

            @Override
            public void onAnimationCancel(Animator animation) {
                mCanceled = true;
            }

            @Override
            public void onAnimationEnd(Animator animation) {
                if (mCanceled) {
                    mCanceled = false;
                    return;
                }
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        setTvViewPosition(mTvViewLayoutParams, mTvViewFrame, false);
                    }
                });
            }
        });
        mTvViewAnimator.addUpdateListener(new AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animator) {
                float fraction = animator.getAnimatedFraction();
                mLastAnimatedTvViewFrame = new MarginLayoutParams(0, 0);
                interpolateMarginsRelative(mLastAnimatedTvViewFrame,
                        mOldTvViewFrame, mTvViewFrame, fraction);
                updatePipView(mLastAnimatedTvViewFrame);
            }
        });
    }

    private void initBackgroundAnimatorIfNeeded() {
        if (mBackgroundAnimator != null) {
            return;
        }

        mBackgroundAnimator = new ObjectAnimator();
        mBackgroundAnimator.setTarget(mContentView);
        mBackgroundAnimator.setPropertyName("backgroundColor");
        mBackgroundAnimator
                .setDuration(mResources.getInteger(R.integer.tvactivity_background_anim_duration));
        mBackgroundAnimator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        mContentView.setBackgroundColor(mBackgroundColor);
                    }
                });
            }
        });
    }

    private void applyDisplayMode(float videoDisplayAspectRatio, boolean animate,
            boolean forceUpdate) {
        if (mAppliedDisplayedMode == mDisplayMode
                && mAppliedTvViewStartMargin == mTvViewStartMargin
                && mAppliedTvViewEndMargin == mTvViewEndMargin
                && Math.abs(mAppliedVideoDisplayAspectRatio - videoDisplayAspectRatio) <
                        DISPLAY_ASPECT_RATIO_EPSILON) {
            if (!forceUpdate) {
                return;
            }
        } else {
            mAppliedDisplayedMode = mDisplayMode;
            mAppliedTvViewStartMargin = mTvViewStartMargin;
            mAppliedTvViewEndMargin = mTvViewEndMargin;
            mAppliedVideoDisplayAspectRatio = videoDisplayAspectRatio;
        }
        int availableAreaWidth = mWindowWidth - mTvViewStartMargin - mTvViewEndMargin;
        int availableAreaHeight = availableAreaWidth * mWindowHeight / mWindowWidth;
        FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(0, 0,
                ((FrameLayout.LayoutParams) mTvView.getLayoutParams()).gravity);
        int displayMode = mDisplayMode;
        double availableAreaRatio = 0;
        double videoRatio = 0;
        if (availableAreaWidth <= 0 || availableAreaHeight <= 0) {
            displayMode = DisplayMode.MODE_FULL;
            Log.w(TAG, "Some resolution info is missing during applyDisplayMode. ("
                    + "availableAreaWidth=" + availableAreaWidth + ", availableAreaHeight="
                    + availableAreaHeight + ")");
        } else {
            availableAreaRatio = (double) availableAreaWidth / availableAreaHeight;
            if (videoDisplayAspectRatio <= 0f) {
                videoRatio = (double) mWindowWidth / mWindowHeight;
            } else {
                videoRatio = videoDisplayAspectRatio;
            }
        }

        int tvViewFrameTop = (mWindowHeight - availableAreaHeight) / 2;
        MarginLayoutParams tvViewFrame = createMarginLayoutParams(
                mTvViewStartMargin, mTvViewEndMargin, tvViewFrameTop, tvViewFrameTop);
        layoutParams.width = availableAreaWidth;
        layoutParams.height = availableAreaHeight;
        switch (displayMode) {
            case DisplayMode.MODE_FULL:
                layoutParams.width = availableAreaWidth;
                layoutParams.height = availableAreaHeight;
                break;
            case DisplayMode.MODE_ZOOM:
                if (videoRatio < availableAreaRatio) {
                    // Y axis will be clipped.
                    layoutParams.width = availableAreaWidth;
                    layoutParams.height = (int) (availableAreaWidth / videoRatio);
                } else {
                    // X axis will be clipped.
                    layoutParams.width = (int) (availableAreaHeight * videoRatio);
                    layoutParams.height = availableAreaHeight;
                }
                break;
            case DisplayMode.MODE_NORMAL:
                if (videoRatio < availableAreaRatio) {
                    // X axis has black area.
                    layoutParams.width = (int) (availableAreaHeight * videoRatio);
                    layoutParams.height = availableAreaHeight;
                } else {
                    // Y axis has black area.
                    layoutParams.width = availableAreaWidth;
                    layoutParams.height = (int) (availableAreaWidth / videoRatio);
                }
                break;
        }

        // FrameLayout has an issue with centering when left and right margins differ.
        // So stick to Gravity.START | Gravity.CENTER_VERTICAL.
        int marginStart = mTvViewStartMargin + (availableAreaWidth - layoutParams.width) / 2;
        layoutParams.setMarginStart(marginStart);
        // Set marginEnd as well because setTvViewPosition uses both start/end margin.
        layoutParams.setMarginEnd(mWindowWidth - layoutParams.width - marginStart);

        setBackgroundColor(Utils.getColor(mResources, isTvViewFullScreen()
                ? R.color.tvactivity_background : R.color.tvactivity_background_on_shrunken_tvview),
                layoutParams, animate);
        setTvViewPosition(layoutParams, tvViewFrame, animate);

        // Update the current display mode.
        mTvOptionsManager.onDisplayModeChanged(displayMode);
    }

    private static int interpolate(int start, int end, float fraction) {
        return (int) (start + (end - start) * fraction);
    }

    private static void interpolateMarginsRelative(MarginLayoutParams out,
            MarginLayoutParams startValue, MarginLayoutParams endValue, float fraction) {
        out.topMargin = interpolate(startValue.topMargin, endValue.topMargin, fraction);
        out.bottomMargin = interpolate(startValue.bottomMargin, endValue.bottomMargin, fraction);
        out.setMarginStart(interpolate(startValue.getMarginStart(), endValue.getMarginStart(),
                fraction));
        out.setMarginEnd(interpolate(startValue.getMarginEnd(), endValue.getMarginEnd(), fraction));
        out.width = interpolate(startValue.width, endValue.width, fraction);
        out.height = interpolate(startValue.height, endValue.height, fraction);
    }

    private MarginLayoutParams createMarginLayoutParams(
            int startMargin, int endMargin, int topMargin, int bottomMargin) {
        MarginLayoutParams lp = new MarginLayoutParams(0, 0);
        lp.setMarginStart(startMargin);
        lp.setMarginEnd(endMargin);
        lp.topMargin = topMargin;
        lp.bottomMargin = bottomMargin;
        lp.width = mWindowWidth - startMargin - endMargin;
        lp.height = mWindowHeight - topMargin - bottomMargin;
        return lp;
    }
}
