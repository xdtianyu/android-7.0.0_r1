/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.calculator2;

import android.content.Context;
import android.graphics.Color;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

public class CalculatorPadViewPager extends ViewPager {

    private final PagerAdapter mStaticPagerAdapter = new PagerAdapter() {
        @Override
        public int getCount() {
            return getChildCount();
        }

        @Override
        public View instantiateItem(ViewGroup container, int position) {
            return getChildAt(position);
        }

        @Override
        public void destroyItem(ViewGroup container, int position, Object object) {
            removeViewAt(position);
        }

        @Override
        public boolean isViewFromObject(View view, Object object) {
            return view == object;
        }

        @Override
        public float getPageWidth(int position) {
            return position == 1 ? 7.0f / 9.0f : 1.0f;
        }
    };

    private final OnPageChangeListener mOnPageChangeListener = new SimpleOnPageChangeListener() {
        @Override
        public void onPageSelected(int position) {
            for (int i = getChildCount() - 1; i >= 0; --i) {
                final View child = getChildAt(i);
                // Prevent clicks and accessibility focus from going through to descendants of
                // other pages which are covered by the current page.
                child.setClickable(i == position);
                child.setImportantForAccessibility(i == position
                        ? IMPORTANT_FOR_ACCESSIBILITY_AUTO
                        : IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS);
            }
        }
    };

    private final PageTransformer mPageTransformer = new PageTransformer() {
        @Override
        public void transformPage(View view, float position) {
            if (position < 0.0f) {
                // Pin the left page to the left side.
                view.setTranslationX(getWidth() * -position);
                view.setAlpha(Math.max(1.0f + position, 0.0f));
            } else {
                // Use the default slide transition when moving to the next page.
                view.setTranslationX(0.0f);
                view.setAlpha(1.0f);
            }
        }
    };

    public CalculatorPadViewPager(Context context) {
        this(context, null /* attrs */);
    }

    public CalculatorPadViewPager(Context context, AttributeSet attrs) {
        super(context, attrs);

        setAdapter(mStaticPagerAdapter);
        setBackgroundColor(Color.BLACK);
        setPageMargin(getResources().getDimensionPixelSize(R.dimen.pad_page_margin));
        setPageTransformer(false, mPageTransformer);
        addOnPageChangeListener(mOnPageChangeListener);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        // Invalidate the adapter's data set since children may have been added during inflation.
        getAdapter().notifyDataSetChanged();

        // Let page change listener know about our initial position.
        mOnPageChangeListener.onPageSelected(getCurrentItem());
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        boolean shouldIntercept = super.onInterceptTouchEvent(ev);

        // Only allow the current item to receive touch events.
        if (!shouldIntercept && ev.getActionMasked() == MotionEvent.ACTION_DOWN) {
            final int x = (int) ev.getX() + getScrollX();
            final int y = (int) ev.getY() + getScrollY();

            final int childCount = getChildCount();
            for (int i = childCount - 1; i >= 0; --i) {
                final int childIndex = getChildDrawingOrder(childCount, i);
                final View child = getChildAt(childIndex);
                if (child.getVisibility() == View.VISIBLE
                        && x >= child.getLeft() && x < child.getRight()
                        && y >= child.getTop() && y < child.getBottom()) {
                    shouldIntercept = (childIndex != getCurrentItem());
                    break;
                }
            }
        }

        return shouldIntercept;
    }
}
