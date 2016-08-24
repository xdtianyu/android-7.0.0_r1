/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tv.settings.util;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.graphics.RectF;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Interpolator;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/**
 * Class used by an activity to perform animation of multiple TransitionImageViews
 * Usage:
 * - on activity create:
 *   TransitionImageAnimation animation = new TransitionImageAnimation(rootView);
 *   for_each TransitionImage of source
 *       animation.addTransitionSource(source);
 *   animation.startCancelTimer();
 * - When the activity loads all target images
 *   for_each TransitionImage of target
 *       animation.addTransitionTarget(target);
 *   animation.startTransition();
 */
public class TransitionImageAnimation {

    public static class Listener {

        public void onRemovedView(TransitionImage src, TransitionImage dst) {
        }

        public void onCancelled(TransitionImageAnimation animation) {
        }

        public void onFinished(TransitionImageAnimation animation) {
        }
    }

    private static final long DEFAULT_TRANSITION_TIMEOUT_MS = 2000;
    private static final long DEFAULT_CANCEL_TRANSITION_MS = 250;
    private static final long DEFAULT_TRANSITION_DURATION_MS = 250;
    private static final long DEFAULT_TRANSITION_START_DELAY_MS = 160;

    private Interpolator mInterpolator = new DecelerateInterpolator();
    private final ViewGroup mRoot;
    private long mTransitionTimeoutMs = DEFAULT_TRANSITION_TIMEOUT_MS;
    private long mCancelTransitionMs = DEFAULT_CANCEL_TRANSITION_MS;
    private long mTransitionDurationMs = DEFAULT_TRANSITION_DURATION_MS;
    private long mTransitionStartDelayMs = DEFAULT_TRANSITION_START_DELAY_MS;
    private final List<TransitionImageView> mTransitions = new ArrayList<>();
    private Listener mListener;
    private Comparator<TransitionImage> mComparator = new TransitionImageMatcher();

    private static final int STATE_INITIAL = 0;
    private static final int STATE_WAIT_DST = 1;
    private static final int STATE_TRANSITION = 2;
    private static final int STATE_CANCELLED = 3;
    private static final int STATE_FINISHED = 4;
    private int mState;

    private boolean mListeningLayout;
    private static final RectF sTmpRect1 = new RectF();
    private static final RectF sTmpRect2 = new RectF();

    public TransitionImageAnimation(ViewGroup root) {
        mRoot = root;
        mState = STATE_INITIAL;
    }

    /**
     * Set listener for animation events
     */
    public TransitionImageAnimation listener(Listener listener) {
        mListener = listener;
        return this;
    }

    public Listener getListener() {
        return mListener;
    }

    /**
     * set comparator for matching src and dst ImageTransition
     */
    public TransitionImageAnimation comparator(Comparator<TransitionImage> comparator) {
        mComparator = comparator;
        return this;
    }

    public Comparator<TransitionImage> getComparator() {
        return mComparator;
    }

    /**
     * set interpolator used for animating the Transition
     */
    public TransitionImageAnimation interpolator(Interpolator interpolator) {
        mInterpolator = interpolator;
        return this;
    }

    public Interpolator getInterpolator() {
        return mInterpolator;
    }

    /**
     * set timeout in ms for {@link #startCancelTimer}
     */
    public TransitionImageAnimation timeoutMs(long timeoutMs) {
        mTransitionTimeoutMs = timeoutMs;
        return this;
    }

    public long getTimeoutMs() {
        return mTransitionTimeoutMs;
    }

    /**
     * set duration of fade out animation when cancel the transition
     */
    public TransitionImageAnimation cancelDurationMs(long ms) {
        mCancelTransitionMs = ms;
        return this;
    }

    public long getCancelDurationMs() {
        return mCancelTransitionMs;
    }

    /**
     * set start delay of transition animation
     */
    public TransitionImageAnimation transitionStartDelayMs(long delay) {
        mTransitionStartDelayMs = delay;
        return this;
    }

    public long getTransitionStartDelayMs() {
        return mTransitionStartDelayMs;
    }

    /**
     * set duration of transition animation
     */
    public TransitionImageAnimation transitionDurationMs(long duration) {
        mTransitionDurationMs = duration;
        return this;
    }

    public long getTransitionDurationMs() {
        return mTransitionDurationMs;
    }

    /**
     * add source transition and create initial view in root
     */
    public void addTransitionSource(TransitionImage src) {
        if (mState != STATE_INITIAL) {
            return;
        }
        TransitionImageView view = new TransitionImageView(mRoot.getContext());
        mRoot.addView(view);
        view.setSourceTransition(src);
        mTransitions.add(view);
        if (!mListeningLayout) {
            mListeningLayout = true;
            mRoot.addOnLayoutChangeListener(mInitializeClip);
        }
    }

    /**
     * kick off the timer for cancel transition
     */
    public void startCancelTimer() {
        if (mState != STATE_INITIAL) {
            return;
        }
        mRoot.postDelayed(mCancelTransitionRunnable, mTransitionTimeoutMs);
        mState = STATE_WAIT_DST;
    }

    private final Runnable mCancelTransitionRunnable = new Runnable() {

        @Override
        public void run() {
            cancelTransition();
        }

    };

    private void setProgress(float progress) {
        // draw from last child (top most in z-order)
        int lastIndex = mTransitions.size() - 1;
        for (int i = lastIndex; i >= 0; i--) {
            TransitionImageView view = mTransitions.get(i);
            view.setProgress(progress);
            sTmpRect2.left = 0;
            sTmpRect2.top = 0;
            sTmpRect2.right = view.getWidth();
            sTmpRect2.bottom = view.getHeight();
            WindowLocationUtil.getLocationsInWindow(view, sTmpRect2);
            if (i == lastIndex) {
                view.clearExcludeClipRect();
                sTmpRect1.set(sTmpRect2);
            } else {
                view.setExcludeClipRect(sTmpRect1);
                // FIXME: this assumes 3rd image will be clipped by "1st union 2nd",
                // applies to certain situation such as images are stacked in one row
                sTmpRect1.union(sTmpRect2);
            }
            view.invalidate();
        }
    }

    private final View.OnLayoutChangeListener mInitializeClip = new View.OnLayoutChangeListener() {
        @Override
        public void onLayoutChange(View v, int left, int top, int right, int bottom,
                int oldLeft, int oldTop, int oldRight, int oldBottom) {
            v.removeOnLayoutChangeListener(this);
            mListeningLayout = false;
            // set initial clipping for all views
            setProgress(0f);
        }
    };

    /**
     * start transition
     */
    public void startTransition() {
        if (mState != STATE_WAIT_DST && mState != STATE_INITIAL) {
            return;
        }
        for (int i = mTransitions.size() - 1; i >= 0; i--) {
            TransitionImageView view = mTransitions.get(i);
            if (view.getDestTransition() == null) {
                cancelTransition(view);
                mTransitions.remove(i);
            }
        }
        if (mTransitions.size() == 0) {
            mState = STATE_CANCELLED;
            if (mListener != null) {
                mListener.onCancelled(this);
            }
            return;
        }
        ValueAnimator v = ValueAnimator.ofFloat(0f, 1f);
        v.setInterpolator(mInterpolator);
        v.setDuration(mTransitionDurationMs);
        v.setStartDelay(mTransitionStartDelayMs);
        v.addUpdateListener(new AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                float progress = animation.getAnimatedFraction();
                setProgress(progress);
            }
        });
        v.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                for (int i = 0, count = mTransitions.size(); i < count; i++) {
                    final TransitionImageView view = mTransitions.get(i);
                    if (mListener != null) {
                        mListener.onRemovedView(view.getSourceTransition(),
                                view.getDestTransition());
                    }
                    mRoot.removeView(view);
                }
                mTransitions.clear();
                mState = STATE_FINISHED;
                if (mListener != null) {
                    mListener.onFinished(TransitionImageAnimation.this);
                }
            }
        });
        v.start();
        mState = STATE_TRANSITION;
    }

    private void cancelTransition(final View iv) {
        iv.animate().alpha(0f).setDuration(mCancelTransitionMs).
            setListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator arg0) {
                mRoot.removeView(iv);
            }
        }).start();
    }

    /**
     * Cancel the transition before it starts, no effect if it already starts
     */
    public void cancelTransition() {
        if (mState != STATE_WAIT_DST && mState != STATE_INITIAL) {
            return;
        }
        int count = mTransitions.size();
        if (count > 0) {
            for (int i = 0; i < count; i++) {
                cancelTransition(mTransitions.get(i));
            }
            mTransitions.clear();
        }
        mState = STATE_CANCELLED;
        if (mListener != null) {
            mListener.onCancelled(this);
        }
    }

    /**
     * find a matching source and relate it with destination
     */
    public boolean addTransitionTarget(TransitionImage dst) {
        if (mState != STATE_WAIT_DST && mState != STATE_INITIAL) {
            return false;
        }
        for (int i = 0, count = mTransitions.size(); i < count; i++) {
            TransitionImageView view = mTransitions.get(i);
            if (mComparator.compare(view.getSourceTransition(), dst) == 0) {
                view.setDestTransition(dst);
                return true;
            }
        }
        return false;
    }
}
