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
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorInflater;
import android.animation.AnimatorListenerAdapter;
import android.content.Context;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.android.tv.R;
import com.android.tv.ui.TunableTvView.BlockScreenType;

public class BlockScreenView extends LinearLayout {
    private View mContainerView;
    private View mImageContainer;
    private ImageView mNormalImageView;
    private ImageView mShrunkenImageView;
    private View mSpace;
    private TextView mTextView;

    private final int mSpacingNormal;
    private final int mSpacingShrunken;

    // Animators used for fade in/out of block screen icon.
    private Animator mFadeIn;
    private Animator mFadeOut;

    public BlockScreenView(Context context) {
        this(context, null, 0);
    }

    public BlockScreenView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public BlockScreenView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mSpacingNormal = getResources().getDimensionPixelOffset(
                R.dimen.tvview_block_vertical_spacing);
        mSpacingShrunken = getResources().getDimensionPixelOffset(
                R.dimen.shrunken_tvview_block_vertical_spacing);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mContainerView = findViewById(R.id.block_screen_container);
        mImageContainer = findViewById(R.id.image_container);
        mNormalImageView = (ImageView) findViewById(R.id.block_screen_icon);
        mShrunkenImageView = (ImageView) findViewById(R.id.block_screen_shrunken_icon);
        mSpace = findViewById(R.id.space);
        mTextView = (TextView) findViewById(R.id.block_screen_text);
        mFadeIn = AnimatorInflater.loadAnimator(getContext(),
                R.animator.tvview_block_screen_fade_in);
        mFadeIn.setTarget(mContainerView);
        mFadeOut = AnimatorInflater.loadAnimator(getContext(),
                R.animator.tvview_block_screen_fade_out);
        mFadeOut.setTarget(mContainerView);
        mFadeOut.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                mContainerView.setVisibility(GONE);
                mContainerView.setAlpha(1f);
            }
        });
    }

    /**
     * Sets the normal image.
     */
    public void setImage(int resId) {
        mNormalImageView.setImageResource(resId);
        updateSpaceVisibility();
    }

    /**
     * Sets the scale type of the normal image.
     */
    public void setScaleType(ScaleType scaleType) {
        mNormalImageView.setScaleType(scaleType);
        updateSpaceVisibility();
    }

    /**
     * Sets the shrunken image.
     */
    public void setShrunkenImage(int resId) {
        mShrunkenImageView.setImageResource(resId);
        updateSpaceVisibility();
    }

    /**
     * Show or hide the image of this view.
     */
    public void setImageVisibility(boolean visible) {
        mImageContainer.setVisibility(visible ? VISIBLE : GONE);
        updateSpaceVisibility();
    }

    /**
     * Sets the text message.
     */
    public void setText(int resId) {
        mTextView.setText(resId);
        updateSpaceVisibility();
    }

    /**
     * Sets the text message.
     */
    public void setText(String text) {
        mTextView.setText(text);
        updateSpaceVisibility();
    }

    private void updateSpaceVisibility() {
        if (isImageViewVisible() && isTextViewVisible(mTextView)) {
            mSpace.setVisibility(VISIBLE);
        } else {
            mSpace.setVisibility(GONE);
        }
    }

    private boolean isImageViewVisible() {
        return mImageContainer.getVisibility() == VISIBLE
                && (isImageViewVisible(mNormalImageView) || isImageViewVisible(mShrunkenImageView));
    }

    private static boolean isImageViewVisible(ImageView imageView) {
        return imageView.getVisibility() != GONE && imageView.getDrawable() != null;
    }

    private static boolean isTextViewVisible(TextView textView) {
        return textView.getVisibility() != GONE && !TextUtils.isEmpty(textView.getText());
    }

    /**
     * Changes the spacing between the image view and the text view according to the
     * {@code blockScreenType}.
     */
    public void setSpacing(@BlockScreenType int blockScreenType) {
        mSpace.getLayoutParams().height =
                blockScreenType == TunableTvView.BLOCK_SCREEN_TYPE_SHRUNKEN_TV_VIEW
                ? mSpacingShrunken : mSpacingNormal;
        requestLayout();
    }

    /**
     * Changes the view layout according to the {@code blockScreenType}.
     */
    public void onBlockStatusChanged(@BlockScreenType int blockScreenType, boolean withAnimation) {
        if (!withAnimation) {
            switch (blockScreenType) {
                case TunableTvView.BLOCK_SCREEN_TYPE_NO_UI:
                    mContainerView.setVisibility(GONE);
                    break;
                case TunableTvView.BLOCK_SCREEN_TYPE_SHRUNKEN_TV_VIEW:
                    mNormalImageView.setVisibility(GONE);
                    mShrunkenImageView.setVisibility(VISIBLE);
                    mContainerView.setVisibility(VISIBLE);
                    break;
                case TunableTvView.BLOCK_SCREEN_TYPE_NORMAL:
                    mNormalImageView.setVisibility(VISIBLE);
                    mShrunkenImageView.setVisibility(GONE);
                    mContainerView.setVisibility(VISIBLE);
                    break;
            }
        } else {
            switch (blockScreenType) {
                case TunableTvView.BLOCK_SCREEN_TYPE_NO_UI:
                    if (mContainerView.getVisibility() == VISIBLE) {
                        mFadeOut.start();
                    }
                    break;
                case TunableTvView.BLOCK_SCREEN_TYPE_SHRUNKEN_TV_VIEW:
                    mNormalImageView.setVisibility(GONE);
                    mShrunkenImageView.setVisibility(VISIBLE);
                    mContainerView.setVisibility(VISIBLE);
                    if (mContainerView.getVisibility() == GONE) {
                        mFadeIn.start();
                    }
                    break;
                case TunableTvView.BLOCK_SCREEN_TYPE_NORMAL:
                    mNormalImageView.setVisibility(VISIBLE);
                    mShrunkenImageView.setVisibility(GONE);
                    mContainerView.setVisibility(VISIBLE);
                    if (mContainerView.getVisibility() == GONE) {
                        mFadeIn.start();
                    }
                    break;
            }
        }
        updateSpaceVisibility();
    }

    /**
     * Scales the contents view by the given {@code scale}.
     */
    public void scaleContainerView(float scale) {
        mContainerView.setScaleX(scale);
        mContainerView.setScaleY(scale);
    }

    public void addFadeOutAnimationListener(AnimatorListener listener) {
        mFadeOut.addListener(listener);
    }

    /**
     * Ends the currently running animations.
     */
    public void endAnimations() {
        if (mFadeIn != null && mFadeIn.isRunning()) {
            mFadeIn.end();
        }
        if (mFadeOut != null && mFadeOut.isRunning()) {
            mFadeOut.end();
        }
    }
}
