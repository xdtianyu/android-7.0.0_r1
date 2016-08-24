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
import android.animation.AnimatorListenerAdapter;
import android.animation.ValueAnimator;
import android.content.Context;
import android.graphics.Outline;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewOutlineProvider;
import android.widget.LinearLayout;

import com.android.tv.R;

/**
 * A base class to render a card.
 */
public abstract class BaseCardView<T> extends LinearLayout implements ItemListRowView.CardView<T> {
    private static final String TAG = "BaseCardView";
    private static final boolean DEBUG = false;

    private static final float SCALE_FACTOR_0F = 0f;
    private static final float SCALE_FACTOR_1F = 1f;

    private ValueAnimator mFocusAnimator;
    private final int mFocusAnimDuration;
    private final float mFocusTranslationZ;
    private final float mVerticalCardMargin;
    private final float mCardCornerRadius;
    private float mFocusAnimatedValue;

    public BaseCardView(Context context) {
        this(context, null);
    }

    public BaseCardView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public BaseCardView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        setClipToOutline(true);
        mFocusAnimDuration = getResources().getInteger(R.integer.menu_focus_anim_duration);
        mFocusTranslationZ = getResources().getDimension(R.dimen.channel_card_elevation_focused)
                - getResources().getDimension(R.dimen.card_elevation_normal);
        mVerticalCardMargin = 2 * (
                getResources().getDimensionPixelOffset(R.dimen.menu_list_padding_top)
                + getResources().getDimensionPixelOffset(R.dimen.menu_list_margin_top));
        // Ensure the same elevation and focus animation for all subclasses.
        setElevation(getResources().getDimension(R.dimen.card_elevation_normal));
        mCardCornerRadius = getResources().getDimensionPixelSize(R.dimen.channel_card_round_radius);
        setOutlineProvider(new ViewOutlineProvider() {
            @Override
            public void getOutline(View view, Outline outline) {
                outline.setRoundRect(0, 0, view.getWidth(), view.getHeight(), mCardCornerRadius);
            }
        });
    }

    /**
     * Called when the view is displayed.
     */
    @Override
    public void onBind(T item, boolean selected) {
        // Note that getCardHeight() will be called by setFocusAnimatedValue().
        // Therefore, be sure that getCardHeight() has a proper value before this method is called.
       setFocusAnimatedValue(selected ? SCALE_FACTOR_1F : SCALE_FACTOR_0F);
    }

    @Override
    public void onRecycled() { }

    @Override
    public void onSelected() {
        if (isAttachedToWindow() && getVisibility() == View.VISIBLE) {
            startFocusAnimation(SCALE_FACTOR_1F);
        } else {
            cancelFocusAnimationIfAny();
            setFocusAnimatedValue(SCALE_FACTOR_1F);
        }
    }

    @Override
    public void onDeselected() {
        if (isAttachedToWindow() && getVisibility() == View.VISIBLE) {
            startFocusAnimation(SCALE_FACTOR_0F);
        } else {
            cancelFocusAnimationIfAny();
            setFocusAnimatedValue(SCALE_FACTOR_0F);
        }
    }

    /**
     * Called when the focus animation started.
     */
    protected void onFocusAnimationStart(boolean selected) {
        // do nothing.
    }

    /**
     * Called when the focus animation ended.
     */
    protected void onFocusAnimationEnd(boolean selected) {
        // do nothing.
    }

    /**
     * Called when the view is bound, or while focus animation is running with a value
     * between {@code SCALE_FACTOR_0F} and {@code SCALE_FACTOR_1F}.
     */
    protected void onSetFocusAnimatedValue(float animatedValue) {
        float scale = 1f + (mVerticalCardMargin / getCardHeight()) * animatedValue;
        setScaleX(scale);
        setScaleY(scale);
        setTranslationZ(mFocusTranslationZ * animatedValue);
    }

    private void setFocusAnimatedValue(float animatedValue) {
        mFocusAnimatedValue = animatedValue;
        onSetFocusAnimatedValue(animatedValue);
    }

    private void startFocusAnimation(final float targetAnimatedValue) {
        cancelFocusAnimationIfAny();
        final boolean selected = targetAnimatedValue == SCALE_FACTOR_1F;
        mFocusAnimator = ValueAnimator.ofFloat(mFocusAnimatedValue, targetAnimatedValue);
        mFocusAnimator.setDuration(mFocusAnimDuration);
        mFocusAnimator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationStart(Animator animation) {
                setHasTransientState(true);
                onFocusAnimationStart(selected);
            }

            @Override
            public void onAnimationEnd(Animator animation) {
                setHasTransientState(false);
                onFocusAnimationEnd(selected);
            }
        });
        mFocusAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                setFocusAnimatedValue((Float) animation.getAnimatedValue());
            }
        });
        mFocusAnimator.start();
    }

    private void cancelFocusAnimationIfAny() {
        if (mFocusAnimator != null) {
            mFocusAnimator.cancel();
            mFocusAnimator = null;
        }
    }

    /**
     * The implementation should return the height of the card.
     */
    protected abstract float getCardHeight();
}
