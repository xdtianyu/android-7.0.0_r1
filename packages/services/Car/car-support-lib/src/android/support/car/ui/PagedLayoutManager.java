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

import android.content.Context;
import android.graphics.PointF;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.LinearSmoothScroller;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.View;
import android.view.animation.AnimationUtils;
import android.view.animation.Interpolator;

/**
 * An extension of {@link LinearLayoutManager} that adds some helper methods for paging
 * such as whether or not it is at the top or bottom of a list and layout param checking.
 */
public class PagedLayoutManager extends LinearLayoutManager {
    private static final String TAG = PagedLayoutManager.class.getSimpleName();

    private final LinearSmoothScroller mSmoothScrollerForDrag;
    private final LinearSmoothScroller mSmoothScrollerForNonDrag;

    private int mLastScrollPosition = 0;
    private boolean mScrollingEnabled = true;
    public Runnable mItemsChangedRunnable;

    public PagedLayoutManager(Context context) {
        super(context, VERTICAL, false);
        mSmoothScrollerForDrag = new SnapToStartSmoothScroller(context, true);
        mSmoothScrollerForNonDrag = new SnapToStartSmoothScroller(context, true);
    }

    public void setItemsChangedListener(Runnable runnable) {
        mItemsChangedRunnable = runnable;
    }

    @Override
    public void onItemsChanged(RecyclerView recyclerView) {
        super.onItemsChanged(recyclerView);
        if (mItemsChangedRunnable != null) {
            mItemsChangedRunnable.run();
        }
    }

    @Override
    protected int getExtraLayoutSpace(RecyclerView.State state) {
        return getHeight() - getPaddingTop() - getPaddingBottom();
    }

    @Override
    public void smoothScrollToPosition(RecyclerView recyclerView, RecyclerView.State state,
            int position) {
        boolean forDrag = recyclerView.getScrollState() == RecyclerView.SCROLL_STATE_SETTLING;
        LinearSmoothScroller ss;
        if (forDrag) {
            ss = mSmoothScrollerForDrag;
        } else {
            ss = mSmoothScrollerForNonDrag;
        }
        ss.setTargetPosition(position);
        startSmoothScroll(ss);
    }

    public int getLastScrollPosition() {
        return mLastScrollPosition;
    }

    @Override
    public void scrollToPosition(int position) {
        super.scrollToPosition(position);
        mLastScrollPosition = position;
    }

    @Override
    public int scrollVerticallyBy(int dy, RecyclerView.Recycler r, RecyclerView.State s) {
        // Our isAtBottom will return true if the view is on screen the the margin extends below
        // the bottom. This will make it so that you can't scroll if only the margin is hanging
        // off the bottom.
        if (isAtBottom() && dy > 0) {
            return 0;
        }
        return super.scrollVerticallyBy(dy, r, s);
    }

    @Override
    public boolean canScrollVertically() {
        return mScrollingEnabled;
    }

    public void setScrollingEnabled(boolean enabled) {
        mScrollingEnabled = enabled;
    }

    public boolean isAtTop() {
        if (getChildCount() == 0 || getItemCount() == 0) {
            return true;
        }
        return findFirstCompletelyVisibleItemPosition() < 1;
    }

    public boolean isAtBottom() {
        if (getChildCount() == 0 || getItemCount() == 0) {
            return true;
        }
        return findLastCompletelyVisibleItemPosition() == getItemCount() - 1;
    }

    private class SnapToStartSmoothScroller extends LinearSmoothScroller {
        private static final int DURATION_MS = 500;
        private final Interpolator mInterpolator;

        public SnapToStartSmoothScroller(Context context, boolean forDrag) {
            super(context);
            int interpolator = forDrag ? android.R.interpolator.decelerate_quint :
                    android.R.interpolator.fast_out_slow_in;
            mInterpolator = AnimationUtils.loadInterpolator(context, interpolator);
        }

        @Override
        protected void onTargetFound(View targetView, RecyclerView.State state, Action action) {
            int dy = calculateDyToMakeVisible(targetView, SNAP_TO_START);
            if (dy == 0) {
                Log.w(TAG, "Scroll distance is 0.");
                return;
            }

            action.update(0, -dy, DURATION_MS, mInterpolator);
        }

        @Override
        public PointF computeScrollVectorForPosition(int targetPosition) {
            return PagedLayoutManager.this
                    .computeScrollVectorForPosition(targetPosition);
        }
    }
}
