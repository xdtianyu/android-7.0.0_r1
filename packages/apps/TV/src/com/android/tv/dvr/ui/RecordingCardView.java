/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tv.dvr.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.support.annotation.Nullable;
import android.support.v17.leanback.widget.BaseCardView;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.tv.R;
import com.android.tv.util.ImageLoader;

/**
 * A CardView for displaying info about a {@link com.android.tv.dvr.ScheduledRecording} or
 * {@link com.android.tv.common.recording.RecordedProgram}
 */
class RecordingCardView extends BaseCardView {
    private final ImageView mImageView;
    private final int mImageWidth;
    private final int mImageHeight;
    private String mImageUri;
    private final TextView mTitleView;
    private final TextView mContentView;
    private final Drawable mDefaultImage;

    RecordingCardView(Context context) {
        super(context);
        //TODO(dvr): move these to the layout XML.
        setCardType(BaseCardView.CARD_TYPE_INFO_UNDER_WITH_EXTRA);
        setFocusable(true);
        setFocusableInTouchMode(true);
        mDefaultImage = getResources().getDrawable(R.drawable.default_now_card, null);

        LayoutInflater inflater = LayoutInflater.from(getContext());
        inflater.inflate(R.layout.dvr_recording_card_view, this);

        mImageView = (ImageView) findViewById(R.id.image);
        mImageWidth = getResources().getDimensionPixelSize(R.dimen.dvr_card_image_layout_width);
        mImageHeight = getResources().getDimensionPixelSize(R.dimen.dvr_card_image_layout_width);
        mTitleView = (TextView) findViewById(R.id.title);
        mContentView = (TextView) findViewById(R.id.content);
    }

    void setTitle(CharSequence title) {
        mTitleView.setText(title);
    }

    void setContent(CharSequence content) {
        mContentView.setText(content);
    }

    void setImageUri(String uri) {
        mImageUri = uri;
        if (TextUtils.isEmpty(uri)) {
            mImageView.setImageDrawable(mDefaultImage);
        } else {
            ImageLoader.loadBitmap(getContext(), uri, mImageWidth, mImageHeight,
                    new RecordingCardImageLoaderCallback(this, uri));
        }
    }

    public void setImageUri(Uri uri) {
        if (uri != null) {
            setImageUri(uri.toString());
        } else {
            setImageUri("");
        }
    }

    private static class RecordingCardImageLoaderCallback
            extends ImageLoader.ImageLoaderCallback<RecordingCardView> {
        private final String mUri;

        RecordingCardImageLoaderCallback(RecordingCardView referent, String uri) {
            super(referent);
            mUri = uri;
        }

        @Override
        public void onBitmapLoaded(RecordingCardView view, @Nullable Bitmap bitmap) {
            if (bitmap == null || !mUri.equals(view.mImageUri)) {
                view.mImageView.setImageDrawable(view.mDefaultImage);
            } else {
                view.mImageView.setImageDrawable(new BitmapDrawable(view.getResources(), bitmap));
            }
        }
    }

    public void reset() {
        mTitleView.setText("");
        mContentView.setText("");
        mImageView.setImageDrawable(mDefaultImage);
    }
}
