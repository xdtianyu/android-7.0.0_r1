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
package android.support.car.ui;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.Rect;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.List;

/**
 * Handles the quantum animation to show or hide elements using a circular clip animation. Views
 * that should be clipped can be added to the animation and are notified through the
 * {@link PathClippingView} interface.
 *
 * The effect is implemented using a circular clip. The state runs the animation and at each
 * step computes the effect's parameters (circle center and radius), which it then passes to
 * registered {@link PathClippingView}s.
 *
 * This a modified version of GoogleSearch/com.google.android.shared.ui/CircularClipAnimation
 */
public class CircularClipAnimation implements ValueAnimator.AnimatorUpdateListener, Animator.AnimatorListener {
    private static final boolean DBG = true;
    private static final String TAG = "CircularClipAnimation";

    public static final int DURATION_MS = 300;

    private static float getFillRadius(int width, int height, int x, int y) {
        return (float) Math.sqrt(
                Math.pow(Math.max(width - x, x), 2) + Math.pow(Math.max(height - y, y), 2));
    }

    /**
     * Container view, in whose coordinate space the clip parameters are computed. The offset for
     * each listener view is computed relative to this view.
     */
    private final ViewGroup mContainer;

    /**
     * Listeners that will be called when the clip animation updates.
     */
    private final List<PathClipingViewInfo> mViewInfos = new ArrayList<>();

    private final Rect mRect;
    private final ValueAnimator mAnimator;

    /** A view, to which the circle's center can be anchored to during animation. */
    private View mAnchorView;

    /*
     * Circular crop's parameters. These are updated by the animator but can also be set manually
     * prior to starting the animation. Do not modify when animation is in progress.
     */
    private int mCircleX;
    private int mCircleY;
    private float mCircleRadius;
    private int mCircleStartRadius;

    public CircularClipAnimation(ViewGroup container) {
        mRect = new Rect();

        mContainer = container;

        mAnimator = new ValueAnimator();
        mAnimator.setInterpolator(
                new QuantumInterpolator(QuantumInterpolator.FAST_OUT_SLOW_IN, 0, 1, 0));

        mAnimator.addUpdateListener(this);
        mAnimator.addListener(this);
        mAnimator.setDuration(DURATION_MS);
        mCircleStartRadius = container.getContext().getResources().getDimensionPixelSize(
                R.dimen.car_touch_feedback_radius);
    }

    private int findViewInfo(PathClippingView clipView) {
        for (int c = 0; c < mViewInfos.size(); c++) {
            PathClipingViewInfo info = mViewInfos.get(c);
            if (info.mClippingView == clipView) {
                return c;
            }
        }
        return -1;
    }

    /**
     * @param clipView Listener that will receive updates to the animation.
     * @param view The view in whose coordinate space we will call onClipUpdate.
     */
    public void addView(PathClippingView clipView, View view) {
        if (findViewInfo(clipView) == -1) {
            mViewInfos.add(new PathClipingViewInfo(clipView, view));
        }
    }

    public void removeView(PathClippingView clipView) {
        if (!mAnimator.isRunning()) {
            Log.e(TAG, "Animator is not running.");
            return;
        }
        int index = findViewInfo(clipView);
        if (index > -1) {
            mViewInfos.remove(index);
        }
    }

    /**
     * Updates the circle's center to given point in container coordinate space.
     * Overrides any previous calls to {@link #setupCenter}.
     */
    public void setupCenter(int x, int y) {
        mCircleX = x;
        mCircleY = y;
    }

    /**
     * Triggers the effect. Any previous animation will be cancelled.
     *
     * @param showing Whether the animation is supposed to show or hide components.
     * @param x X of the content to be revealed or hidden.
     * @param y Y of the content to be revealed or hidden.
     * @param width Width of the content to be revealed or hidden.
     * @param height Height of the content to be revealed or hidden.
     * @param anchorView See {@link #mAnchorView}.
     */
    public void start(boolean showing, int x, int y, int width, int height, View anchorView) {
        if (DBG) Log.v(TAG,
                "start(" + showing + ", " + width + ", " + height + ", " + anchorView + ")");

        // Compute the radius.
        final float radius = getFillRadius(width, height,
                mCircleX - x, mCircleY - y);

        // Set anchor view.
        mAnchorView = anchorView;

        // Set up animation values. Note that the minimum radius should be larger than 0,
        // otherwise the addCircle operation becomes a No-op, leading to inverted clipping.
        if (showing) {
            mAnimator.setFloatValues(mCircleStartRadius, radius);
        } else {
            mAnimator.setFloatValues(radius, 1);
        }
        mAnimator.start();
    }

    /**
     * Updates the circle's center to that of the anchor view.
     */
    private void updateCircleCenterTo(View view) {
        mRect.set(0, 0, view.getWidth(), view.getHeight());
        mContainer.offsetDescendantRectToMyCoords(view, mRect);

        mCircleX = mRect.centerX();
        mCircleY = mRect.centerY();
    }

    // Animation listeners.
    @Override
    public void onAnimationStart(Animator animation) {
        for (int c = 0; c < mViewInfos.size(); c++) {
            // We find out the offset of each Listeners View from the mContainer and store it
            // so we can offset the paths we pass to each Listener into its coords space.
            mRect.left = 0;
            mRect.top = 0;
            mContainer.offsetRectIntoDescendantCoords(mViewInfos.get(c).mView, mRect);
            mViewInfos.get(c).mOffset.set(mRect.left, mRect.top);
        }
    }

    @Override
    public void onAnimationUpdate(ValueAnimator animation) {
        if (mAnchorView != null) {
            updateCircleCenterTo(mAnchorView);
        }

        mCircleRadius = (Float) animation.getAnimatedValue();

        // Notify listeners.
        for (int c = 0; c < mViewInfos.size(); c++) {
            PathClipingViewInfo info = mViewInfos.get(c);
            info.mPath.reset();
            info.mPath.addCircle(mCircleX + info.mOffset.x, mCircleY + info.mOffset.y,
                    mCircleRadius, Path.Direction.CW);
            info.mClippingView.setClipPath(info.mPath);
        }
    }

    @Override
    public void onAnimationEnd(Animator animation) {
        for (int c = 0; c < mViewInfos.size(); c++) {
            PathClipingViewInfo info = mViewInfos.get(c);
            info.mClippingView.setClipPath(null);
        }
    }

    @Override
    public void onAnimationCancel(Animator animation) {
    }

    @Override
    public void onAnimationRepeat(Animator animation) {
    }

    public void addListener(Animator.AnimatorListener listener) {
        mAnimator.addListener(listener);
    }

    /**
     * Internal structure to keep track of the clipping components.
     */
    private static class PathClipingViewInfo {
        final PathClippingView mClippingView;
        final View mView;
        final Point mOffset;
        final Path mPath;

        PathClipingViewInfo(PathClippingView clippingView, View view) {
            mClippingView = clippingView;
            mView = view;
            mOffset = new Point();
            mPath = new Path();
        }
    }
}
