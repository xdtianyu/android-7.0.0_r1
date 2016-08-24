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
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcelable;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;

import com.android.tv.settings.widget.BitmapWorkerOptions;
import com.android.tv.settings.widget.DrawableDownloader;

import java.util.ArrayList;
import java.util.List;

/**
 * Initiator calls {@link #createFromImageView(ImageView)} and
 * {@link #writeMultipleToIntent(List, Intent)}.
 * <p>
 * Receiver calls {@link #readMultipleFromIntent(Context, Intent)} to read source and
 * {@link #createFromImageView(ImageView)} for target image view;  then start animation
 * using {@link TransitionImageView} between these two.<p>
 * The matching of Uri is up to receiver, typically using {@link TransitionImageMatcher}
 * <p>
 * The transition image has three bounds, all relative to window<p>
 * - {@link #setRect(Rect)}  bounds of the image view, including background color<p>
 * - {@link #setUnclippedRect(RectF)} bounds of original bitmap without clipping,  the rect
 *   might be bigger than the image view<p>
 * - {@link #setClippedRect(RectF)} bounds of clipping<p>
 */
public class TransitionImage {

    private Uri mUri;
    private BitmapDrawable mBitmap;
    private final Rect mRect = new Rect();
    private int mBackground = Color.TRANSPARENT;
    private float mAlpha = 1f;
    private float mSaturation = 1f;
    private final RectF mUnclippedRect = new RectF();
    private final RectF mClippedRect = new RectF();
    private boolean mUseClippedRectOnTransparent = true;

    public static final String EXTRA_TRANSITION_BITMAP =
            "com.android.tv.settings.transition_bitmap";
    public static final String EXTRA_TRANSITION_BITMAP_RECT =
            "com.android.tv.settings.transition_bmp_rect";
    public static final String EXTRA_TRANSITION_BITMAP_URI =
            "com.android.tv.settings.transition_bmp_uri";
    public static final String EXTRA_TRANSITION_BITMAP_ALPHA =
            "com.android.tv.settings.transition_bmp_alpha";
    public static final String EXTRA_TRANSITION_BITMAP_SATURATION =
            "com.android.tv.settings.transition_bmp_saturation";
    public static final String EXTRA_TRANSITION_BITMAP_BACKGROUND =
            "com.android.tv.settings.transition_bmp_background";
    public static final String EXTRA_TRANSITION_BITMAP_UNCLIPPED_RECT =
            "com.android.tv.settings.transition_bmp_unclipped_rect";
    public static final String EXTRA_TRANSITION_BITMAP_CLIPPED_RECT =
            "com.android.tv.settings.transition_bmp_clipped_rect";
    public static final String EXTRA_TRANSITION_MULTIPLE_BITMAP =
            "com.android.tv.settings.transition_multiple_bitmap";

    public TransitionImage() {
    }

    public Uri getUri() {
        return mUri;
    }

    public void setUri(Uri uri) {
        mUri = uri;
    }

    public BitmapDrawable getBitmap() {
        return mBitmap;
    }

    public void setBitmap(BitmapDrawable bitmap) {
        mBitmap = bitmap;
    }

    public Rect getRect() {
        return mRect;
    }

    public void setRect(Rect rect) {
        mRect.set(rect);
    }

    public int getBackground() {
        return mBackground;
    }

    public void setBackground(int color) {
        mBackground = color;
    }

    public float getAlpha() {
        return mAlpha;
    }

    public void setAlpha(float alpha) {
        mAlpha = alpha;
    }

    public float getSaturation() {
        return mSaturation;
    }

    public void setSaturation(float saturation) {
        mSaturation = saturation;
    }

    public RectF getUnclippedRect() {
        return mUnclippedRect;
    }

    public void setUnclippedRect(RectF rect) {
        mUnclippedRect.set(rect);
    }

    public RectF getClippedRect() {
        return mClippedRect;
    }

    public void setClippedRect(RectF rect) {
        mClippedRect.set(rect);
    }

    public static List<TransitionImage> readMultipleFromIntent(Context context, Intent intent) {
        ArrayList<TransitionImage> transitions = new ArrayList<>();
        Bundle extras = intent.getExtras();
        if (extras == null) {
            return transitions;
        }
        TransitionImage image = new TransitionImage();
        if (image.readFromBundle(context, intent.getSourceBounds(), extras)) {
            transitions.add(image);
        }
        Parcelable[] multiple =
                intent.getParcelableArrayExtra(EXTRA_TRANSITION_MULTIPLE_BITMAP);
        if (multiple != null) {
            for (int i = 0, size = multiple.length; i < size; i++) {
                if (!(multiple[i] instanceof Bundle)) {
                    break;
                }
                image = new TransitionImage();
                if (image.readFromBundle(context, null, (Bundle) multiple[i])) {
                    transitions.add(image);
                }
            }
        }
        return transitions;
    }

    public static void writeMultipleToIntent(List<TransitionImage> transitions, Intent intent) {
        if (transitions == null || transitions.size() == 0) {
            return;
        }
        int size = transitions.size();
        if (size == 1) {
            TransitionImage image = transitions.get(0);
            image.writeToIntent(intent);
            return;
        }
        Parcelable[] multipleBundle = new Parcelable[size];
        for (int i = 0; i < size; i++) {
            Bundle b = new Bundle();
            transitions.get(i).writeToBundle(b);
            multipleBundle[i] = b;
        }
        intent.putExtra(EXTRA_TRANSITION_MULTIPLE_BITMAP, multipleBundle);
    }

    public boolean readFromBundle(Context context, Rect intentSourceBounds, Bundle bundle) {
        setBitmap(null);
        if (bundle == null) {
            return false;
        }
        mUri = bundle.getParcelable(EXTRA_TRANSITION_BITMAP_URI);
        BitmapDrawable bitmap = null;
        if (mUri != null) {
            DrawableDownloader downloader = DrawableDownloader.getInstance(context);
            BitmapWorkerOptions key = new BitmapWorkerOptions.Builder(context)
                    .resource(mUri).build();
            bitmap = downloader.getLargestBitmapFromMemCache(key);
        }
        if (bitmap == null) {
            if (bundle.containsKey(EXTRA_TRANSITION_BITMAP)) {
                bitmap = new BitmapDrawable(context.getResources(),
                        ActivityTransitionBitmapHelper.getBitmapFromBinderBundle(
                        bundle.getBundle(EXTRA_TRANSITION_BITMAP)));
            }
            if (bitmap == null) {
                return false;
            }
        }
        Rect rect = null;
        String bitmapRectStr = bundle.getString(EXTRA_TRANSITION_BITMAP_RECT);
        if (!TextUtils.isEmpty(bitmapRectStr)) {
            rect = Rect.unflattenFromString(bitmapRectStr);
        }
        if (rect == null) {
            rect = intentSourceBounds;
        }
        if (rect == null) {
            return false;
        }
        setBitmap(bitmap);
        setRect(rect);
        if (!readRectF(bundle.getFloatArray(EXTRA_TRANSITION_BITMAP_CLIPPED_RECT),
                mClippedRect)) {
            mClippedRect.set(rect);
        }
        if (!readRectF(bundle.getFloatArray(EXTRA_TRANSITION_BITMAP_UNCLIPPED_RECT),
                mUnclippedRect)) {
            mUnclippedRect.set(rect);
        }
        setAlpha(bundle.getFloat(EXTRA_TRANSITION_BITMAP_ALPHA, 1f));
        setSaturation(bundle.getFloat(EXTRA_TRANSITION_BITMAP_SATURATION, 1f));
        setBackground(bundle.getInt(EXTRA_TRANSITION_BITMAP_BACKGROUND, 0));
        return true;
    }

    public void writeToBundle(Bundle bundle) {
        bundle.putParcelable(EXTRA_TRANSITION_BITMAP_URI, mUri);
        bundle.putString(EXTRA_TRANSITION_BITMAP_RECT, mRect.flattenToString());
        if (mBitmap != null) {
            bundle.putBundle(EXTRA_TRANSITION_BITMAP,
                    ActivityTransitionBitmapHelper.bitmapAsBinderBundle(mBitmap.getBitmap()));
        }
        bundle.putFloatArray(EXTRA_TRANSITION_BITMAP_CLIPPED_RECT,
                writeRectF(mClippedRect, new float[4]));
        bundle.putFloatArray(EXTRA_TRANSITION_BITMAP_UNCLIPPED_RECT,
                writeRectF(mUnclippedRect, new float[4]));
        bundle.putFloat(EXTRA_TRANSITION_BITMAP_ALPHA, mAlpha);
        bundle.putFloat(EXTRA_TRANSITION_BITMAP_SATURATION, mSaturation);
        bundle.putInt(EXTRA_TRANSITION_BITMAP_BACKGROUND, mBackground);
    }

    public void writeToIntent(Intent intent) {
        intent.setSourceBounds(mRect);
        intent.putExtra(EXTRA_TRANSITION_BITMAP_URI, mUri);
        intent.putExtra(EXTRA_TRANSITION_BITMAP_RECT, mRect.flattenToString());
        if (mBitmap != null) {
            intent.putExtra(EXTRA_TRANSITION_BITMAP,
                    ActivityTransitionBitmapHelper.bitmapAsBinderBundle(mBitmap.getBitmap()));
        }
        intent.putExtra(EXTRA_TRANSITION_BITMAP_CLIPPED_RECT,
                writeRectF(mClippedRect, new float[4]));
        intent.putExtra(EXTRA_TRANSITION_BITMAP_UNCLIPPED_RECT,
                writeRectF(mUnclippedRect, new float[4]));
        intent.putExtra(EXTRA_TRANSITION_BITMAP_ALPHA, mAlpha);
        intent.putExtra(EXTRA_TRANSITION_BITMAP_SATURATION, mSaturation);
        intent.putExtra(EXTRA_TRANSITION_BITMAP_BACKGROUND, mBackground);
    }

    public static boolean readRectF(float[] values, RectF f) {
        if (values == null || values.length != 4) {
            return false;
        }
        f.set(values[0], values[1], values[2], values[3]);
        return true;
    }

    public static float[] writeRectF(RectF f, float[] values) {
        values[0] = f.left;
        values[1] = f.top;
        values[2] = f.right;
        values[3] = f.bottom;
        return values;
    }

    /**
     * set bounds and bitmap
     */
    public void createFromImageView(ImageView imageView) {
        createFromImageView(imageView, imageView);
    }

    /**
     * set bounds and bitmap
     *
     * @param backgroundView background view can be larger than the image view that will
     * be drawn with background color
     */
    public void createFromImageView(ImageView view, View backgroundView) {
        Drawable drawable = view.getDrawable();
        if (drawable instanceof BitmapDrawable) {
            setBitmap((BitmapDrawable) drawable);
        }
        // use background View as the outside bounds and we can fill
        // background color in it
        mClippedRect.set(0, 0, backgroundView.getWidth(), backgroundView.getHeight());
        WindowLocationUtil.getLocationsInWindow(backgroundView, mClippedRect);
        mClippedRect.round(mRect);
        // get image view rects
        WindowLocationUtil.getImageLocationsInWindow(view, mClippedRect, mUnclippedRect);
    }

    /**
     * set if background is transparent, set if we want to use {@link #setClippedRect(RectF)}
     * instead of {@link #setRect(Rect)}.  Default value is true,  and the value is not
     * serialized.  User should call it before using TransitionImageAnimation.
     */
    public void setUseClippedRectOnTransparent(boolean ignoreBackground) {
        mUseClippedRectOnTransparent = ignoreBackground;
    }

    /**
     * get if background is not transparent, set if we want to use {@link #setClippedRect(RectF)}
     * instead of {@link #setRect(Rect)}
     */
    public boolean getUseClippedRectOnTransparent() {
        return mUseClippedRectOnTransparent;
    }

    /**
     * Get optimized rect depending on the background color
     */
    public void getOptimizedRect(Rect rect) {
        if (mUseClippedRectOnTransparent && mBackground == Color.TRANSPARENT) {
            mClippedRect.round(rect);
        } else {
            rect.set(mRect);
        }
    }

    @Override
    public String toString() {
        return "{TransitionImage Uri=" + mUri + " rect=" + mRect
                + " unclipRect=" + mUnclippedRect + " clipRect=" + mClippedRect
                + " bitmap=" + mBitmap + " alpha=" + mAlpha + " saturation=" + mSaturation
                + " background=" + mBackground;
    }
}
