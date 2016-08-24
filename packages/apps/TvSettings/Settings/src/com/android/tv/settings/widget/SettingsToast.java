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

package com.android.tv.settings.widget;

import android.content.Context;
import android.graphics.Bitmap;
import android.net.Uri;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.android.tv.settings.R;
import com.android.tv.settings.widget.BitmapDownloader.BitmapCallback;

/**
 * Implementation of the SettingsToast notification.
 */
public class SettingsToast extends Toast {

    protected final Context mContext;
    protected final TextView mTextView;
    protected final ImageView mIconView;
    protected BitmapCallback mBitmapCallBack;

    /**
     * Constructs a SettingsToast message with a text message.
     *
     * @param context  The context to use.  Usually your {@link android.app.Application}
     *                 or {@link android.app.Activity} object.
     * @param text     The text to show.  Can be formatted text.
     */
    public SettingsToast(Context context, CharSequence text) {
        super(context);

        mContext = context;

        LayoutInflater inflater = (LayoutInflater) mContext.getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        View layout = inflater.inflate(R.layout.toast_notification, null);

        mTextView = (TextView) layout.findViewById(R.id.text);
        if (mTextView != null) {
            mTextView.setText(text);
        }

        mIconView = (ImageView) layout.findViewById(R.id.icon);

        setGravity(Gravity.CENTER_HORIZONTAL | Gravity.TOP | Gravity.FILL_HORIZONTAL, 0, 0);
        setView(layout);
    }

    /**
     * Constructs a SettingsToast message with a text message and an icon.
     *
     * @param context The context to use. Usually your
     *            {@link android.app.Application} or
     *            {@link android.app.Activity} object.
     * @param text The text to show. Can be formatted text.
     * @param iconUri URI String identifying the Icon to be used in this
     *            notification.
     */
    public SettingsToast(Context context, CharSequence text, String iconUri) {
        this(context, text);

        if (mIconView != null && iconUri != null) {
            mIconView.setVisibility(View.INVISIBLE);

            BitmapDownloader bitmapDownloader = BitmapDownloader.getInstance(mContext);
            mBitmapCallBack = new BitmapCallback() {
                    @Override
                public void onBitmapRetrieved(Bitmap bitmap) {
                    mIconView.setImageBitmap(bitmap);
                    mIconView.setVisibility(View.VISIBLE);
                }
            };

            bitmapDownloader.getBitmap(new BitmapWorkerOptions.Builder(mContext).resource(
                    Uri.parse(iconUri)).width(mIconView.getLayoutParams().width)
                    .height(mIconView.getLayoutParams().height).build(), mBitmapCallBack);
        }
    }

    /**
     * Constructs a SettingsToast message with a text message and a Bitmap icon.
     *
     * @param context The context to use. Usually your
     *            {@link android.app.Application} or
     *            {@link android.app.Activity} object.
     * @param text The text to show. Can be formatted text.
     * @param iconBitmap Bitmap Icon to be used in this toast notification.
     */
    public SettingsToast(Context context, CharSequence text, Bitmap iconBitmap) {
        this(context, text);

        if (mIconView != null && iconBitmap != null) {
            mIconView.setImageBitmap(iconBitmap);
            mIconView.setVisibility(View.VISIBLE);
        }
    }

    @Override
    public void finalize() throws Throwable {
        if (mBitmapCallBack != null) {
            BitmapDownloader bitmapDownloader = BitmapDownloader.getInstance(mContext);
            bitmapDownloader.cancelDownload(mBitmapCallBack);
        }
        super.finalize();
    }
}
