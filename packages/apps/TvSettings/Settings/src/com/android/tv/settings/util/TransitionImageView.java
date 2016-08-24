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

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region.Op;
import android.graphics.drawable.BitmapDrawable;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup.LayoutParams;

import com.android.tv.settings.widget.RefcountBitmapDrawable;

/**
 * Util widget that can animate the following factors
 * - scale of the view
 * - position of the view
 * - background color of the view
 * - unclipped bounds of bitmap
 * - clipping bounds of bitmap
 * - alpha of bitmap
 * - saturation of bitmap
 */
class TransitionImageView extends View {

    private TransitionImage mSrc;
    private TransitionImage mDst;

    private BitmapDrawable mBitmapDrawable;

    /**
     * values for difference between src and dst
     */
    private float mScaleX;
    private float mScaleY;
    private float mScaleXDiff;
    private float mScaleYDiff;
    private float mTranslationXDiff;
    private float mTranslationYDiff;
    private float mClipLeftDiff;
    private float mClipRightDiff;
    private float mClipTopDiff;
    private float mClipBottomDiff;
    private float mUnclipCenterXDiff;
    private float mUnclipCenterYDiff;
    private float mUnclipWidthDiffBeforeScale;
    private float mUnclipHeightDiffBeforeScale;
    private float mSaturationDiff;
    private float mAlphaDiff;
    private int mBgAlphaDiff;
    private int mBgRedDiff;
    private int mBgGreenDiff;
    private int mBgBlueDiff;
    private boolean mBgHasDiff;

    private float mProgress;
    private final Rect mSrcRect = new Rect();
    private final RectF mSrcUnclipRect = new RectF();
    private final RectF mSrcClipRect = new RectF();
    private final Rect mDstRect = new Rect();

    private final RectF mClipRect = new RectF();
    private final Rect mUnclipRect = new Rect();
    private int mSrcBgColor;
    private final ColorMatrix mColorMatrix = new ColorMatrix();

    private RectF mExcludeRect;

    public TransitionImageView(Context context) {
        this(context, null);
    }

    public TransitionImageView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public TransitionImageView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        // the scale and translation is based on left/up corner of the view
        setPivotX(0f);
        setPivotY(0f);
        setWillNotDraw(false);
    }

    public void setSourceTransition(TransitionImage src) {
        mSrc = src;
        initializeView();
    }

    public void setDestTransition(TransitionImage dst) {
        mDst = dst;
        calculateDiffs();
    }

    @Override
    protected void onDetachedFromWindow() {
        if (mBitmapDrawable instanceof RefcountBitmapDrawable) {
            ((RefcountBitmapDrawable) mBitmapDrawable).getRefcountObject().releaseRef();
        }
        super.onDetachedFromWindow();
    }

    private void initializeView() {
        mBitmapDrawable = mSrc.getBitmap();
        mBitmapDrawable.mutate();

        mSrc.getOptimizedRect(mSrcRect);
        // initialize size
        LayoutParams params = getLayoutParams();
        params.width = mSrcRect.width();
        params.height = mSrcRect.height();

        // get src clip rect relative to the view
        mSrcClipRect.set(mSrc.getClippedRect());
        mSrcClipRect.offset(-mSrcRect.left, -mSrcRect.top);

        // get src rect relative to the view
        mSrcUnclipRect.set(mSrc.getUnclippedRect());
        mSrcUnclipRect.offset(-mSrcRect.left, -mSrcRect.top);

        // initialize alpha, saturation, background color
        if (mSrc.getAlpha() != 1f) {
            mBitmapDrawable.setAlpha((int) (mSrc.getAlpha() * 255));
        }
        if (mSrc.getSaturation() != 1f) {
            mColorMatrix.setSaturation(mSrc.getSaturation());
            mBitmapDrawable.setColorFilter(new ColorMatrixColorFilter(mColorMatrix));
        }
        mSrcBgColor = mSrc.getBackground();
        if (mSrcBgColor != Color.TRANSPARENT) {
            setBackgroundColor(mSrcBgColor);
            getBackground().setAlpha((int) (mSrc.getAlpha() * 255));
        }

        invalidate();
    }

    private void calculateDiffs() {
        mDst.getOptimizedRect(mDstRect);
        mScaleX = (float) mDstRect.width() / mSrcRect.width();
        mScaleY = (float) mDstRect.height() / mSrcRect.height();
        mScaleXDiff = mScaleX - 1f;
        mScaleYDiff = mScaleY - 1f;
        mTranslationXDiff = mDstRect.left - mSrcRect.left;
        mTranslationYDiff = mDstRect.top - mSrcRect.top;

        RectF dstClipRect = new RectF();
        // get dst clip rect relative to the view
        dstClipRect.set(mDst.getClippedRect());
        dstClipRect.offset(-mDstRect.left, -mDstRect.top);
        // get dst clip rect before scaling
        dstClipRect.left /= mScaleX;
        dstClipRect.right /= mScaleX;
        dstClipRect.top /= mScaleY;
        dstClipRect.bottom /= mScaleY;
        mClipLeftDiff = dstClipRect.left - mSrcClipRect.left;
        mClipRightDiff = dstClipRect.right - mSrcClipRect.right;
        mClipTopDiff = dstClipRect.top - mSrcClipRect.top;
        mClipBottomDiff = dstClipRect.bottom - mSrcClipRect.bottom;

        RectF dstUnclipRect = new RectF();
        // get dst rect relative to the view
        dstUnclipRect.set(mDst.getUnclippedRect());
        dstUnclipRect.offset(-mDstRect.left, -mDstRect.top);
        mUnclipWidthDiffBeforeScale = dstUnclipRect.width() - mSrcUnclipRect.width();
        mUnclipHeightDiffBeforeScale = dstUnclipRect.height() - mSrcUnclipRect.height();
        // get dst clip rect before scaling
        dstUnclipRect.left /= mScaleX;
        dstUnclipRect.right /= mScaleX;
        dstUnclipRect.top /= mScaleY;
        dstUnclipRect.bottom /= mScaleY;
        mUnclipCenterXDiff = dstUnclipRect.centerX() - mSrcUnclipRect.centerX();
        mUnclipCenterYDiff = dstUnclipRect.centerY() - mSrcUnclipRect.centerY();

        mAlphaDiff = mDst.getAlpha() - mSrc.getAlpha();
        int srcColor = mSrc.getBackground();
        int dstColor = mDst.getBackground();
        mBgAlphaDiff = Color.alpha(dstColor) - Color.alpha(srcColor);
        mBgRedDiff = Color.red(dstColor) - Color.red(srcColor);
        mBgGreenDiff = Color.green(dstColor) - Color.green(srcColor);
        mBgBlueDiff = Color.blue(dstColor) - Color.blue(srcColor);
        mSaturationDiff = mDst.getSaturation() - mSrc.getSaturation();
        mBgHasDiff = mBgAlphaDiff != 0 || mBgRedDiff != 0 || mBgGreenDiff != 0
                || mBgBlueDiff != 0;
    }

    public TransitionImage getSourceTransition() {
        return mSrc;
    }

    public TransitionImage getDestTransition() {
        return mDst;
    }

    public void setProgress(float progress) {
        mProgress = progress;

        // animating scale factor
        setScaleX(1f + mScaleXDiff * mProgress);
        setScaleY(1f + mScaleYDiff * mProgress);

        // animating view position
        setTranslationX(mSrcRect.left + mProgress * mTranslationXDiff);
        setTranslationY(mSrcRect.top + mProgress * mTranslationYDiff);

        // animating unclipped bitmap bounds
        float unclipCenterX = mSrcUnclipRect.centerX() + mUnclipCenterXDiff * mProgress;
        float unclipCenterY = mSrcUnclipRect.centerY() + mUnclipCenterYDiff * mProgress;
        float unclipWidthBeforeScale =
                mSrcUnclipRect.width() + mUnclipWidthDiffBeforeScale * mProgress;
        float unclipHeightBeforeScale =
                mSrcUnclipRect.height() + mUnclipHeightDiffBeforeScale * mProgress;
        float unclipWidth = unclipWidthBeforeScale / getScaleX();
        float unclipHeight = unclipHeightBeforeScale / getScaleY();
        mUnclipRect.left = (int) (unclipCenterX - unclipWidth * 0.5f);
        mUnclipRect.top = (int) (unclipCenterY - unclipHeight * 0.5f);
        mUnclipRect.right = (int) (unclipCenterX + unclipWidth * 0.5f);
        mUnclipRect.bottom = (int) (unclipCenterY + unclipHeight * 0.5f);
        // rounding to integer will cause a shaking effect if the target unclip rect is much
        // smaller than view bounds; e.g. a portrait bitmap inside a landscape imageView.
        mBitmapDrawable.setBounds(mUnclipRect);

        // animate clip bounds
        mClipRect.left = mSrcClipRect.left + mClipLeftDiff * mProgress;
        mClipRect.top = mSrcClipRect.top + mClipTopDiff * mProgress;
        mClipRect.right = mSrcClipRect.right + mClipRightDiff * mProgress;
        mClipRect.bottom = mSrcClipRect.bottom + mClipBottomDiff * mProgress;

        // animate bitmap alpha, bitmap saturation
        if (mAlphaDiff != 0f) {
            int alpha = (int) ((mSrc.getAlpha() + mAlphaDiff * mProgress) * 255);
            mBitmapDrawable.setAlpha(alpha);
            if (getBackground() != null) {
                getBackground().setAlpha(alpha);
            }
        }
        if (mSaturationDiff != 0f) {
            mColorMatrix.setSaturation(mSrc.getSaturation() + mSaturationDiff * mProgress);
            mBitmapDrawable.setColorFilter(new ColorMatrixColorFilter(mColorMatrix));
        }

        // animate background color
        if (mBgHasDiff) {
            setBackgroundColor(Color.argb(
                    Color.alpha(mSrcBgColor) + (int) (mBgAlphaDiff * mProgress),
                    Color.red(mSrcBgColor) + (int) (mBgRedDiff * mProgress),
                    Color.green(mSrcBgColor) + (int) (mBgGreenDiff * mProgress),
                    Color.blue(mSrcBgColor) + (int) (mBgBlueDiff * mProgress)));
        }

        invalidate();
    }

    public float getProgress() {
        return mProgress;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (mBitmapDrawable == null) {
            return;
        }
        int count = canvas.save();
        canvas.clipRect(mClipRect);
        if (mExcludeRect != null) {
            canvas.clipRect(mExcludeRect, Op.DIFFERENCE);
        }
        mBitmapDrawable.draw(canvas);
        canvas.restoreToCount(count);
    }

    public void setExcludeClipRect(RectF rect) {
        if (mExcludeRect == null) {
            mExcludeRect = new RectF();
        }
        mExcludeRect.set(rect);
        // get rect relative to left/top corner of the view
        mExcludeRect.offset(-getX(), -getY());
        // get locations before scale applied
        mExcludeRect.left /= (1f + mScaleXDiff * mProgress);
        mExcludeRect.right /= (1f + mScaleXDiff * mProgress);
        mExcludeRect.top /= (1f + mScaleYDiff * mProgress);
        mExcludeRect.bottom /= (1f + mScaleYDiff * mProgress);
    }

    public void clearExcludeClipRect() {
        mExcludeRect = null;
    }
}
