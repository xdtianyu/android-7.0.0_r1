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

import android.content.Context;
import android.graphics.Bitmap;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.data.Channel;
import com.android.tv.data.Program;
import com.android.tv.parental.ParentalControlSettings;
import com.android.tv.util.ImageLoader;

/**
 * A view to render channel card.
 */
public class ChannelCardView extends BaseCardView<Channel> {
    private static final String TAG = MenuView.TAG;
    private static final boolean DEBUG = MenuView.DEBUG;

    private final float mCardHeight;
    private final float mExtendedCardHeight;
    private final float mProgramNameViewHeight;
    private final float mExtendedTextViewCardHeight;
    private final int mCardImageWidth;
    private final int mCardImageHeight;

    private ImageView mImageView;
    private View mGradientView;
    private TextView mChannelNumberNameView;
    private ProgressBar mProgressBar;
    private TextView mMetaViewFocused;
    private TextView mMetaViewUnfocused;
    private Channel mChannel;
    private Program mProgram;
    private boolean mExtendViewOnFocus;
    private final MainActivity mMainActivity;

    public ChannelCardView(Context context) {
        this(context, null);
    }

    public ChannelCardView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ChannelCardView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        mCardImageWidth = getResources().getDimensionPixelSize(R.dimen.card_image_layout_width);
        mCardImageHeight = getResources().getDimensionPixelSize(R.dimen.card_image_layout_height);
        mCardHeight = getResources().getDimensionPixelSize(R.dimen.card_layout_height);
        mExtendedCardHeight = getResources().getDimensionPixelSize(
                R.dimen.card_layout_height_extended);
        mProgramNameViewHeight = getResources().getDimensionPixelSize(
                R.dimen.card_meta_layout_height);
        mExtendedTextViewCardHeight = getResources().getDimensionPixelOffset(
                R.dimen.card_meta_layout_height_extended);

        mMainActivity = (MainActivity) context;
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mImageView = (ImageView) findViewById(R.id.image);
        mGradientView = findViewById(R.id.image_gradient);
        mChannelNumberNameView = (TextView) findViewById(R.id.channel_number_and_name);
        mMetaViewFocused = (TextView) findViewById(R.id.channel_title_focused);
        mMetaViewUnfocused = (TextView) findViewById(R.id.channel_title_unfocused);
        mProgressBar = (ProgressBar) findViewById(R.id.progress);
    }

    @Override
    public void onBind(Channel channel, boolean selected) {
        if (DEBUG) {
            Log.d(TAG, "onBind(channelName=" + channel.getDisplayName() + ", selected=" + selected
                    + ")");
        }
        mChannel = channel;
        mProgram = null;
        if (TextUtils.isEmpty(mChannel.getDisplayName())) {
            mChannelNumberNameView.setText(mChannel.getDisplayNumber());
        } else {
            mChannelNumberNameView.setText(mChannel.getDisplayNumber() + " "
                    + mChannel.getDisplayName());
        }
        mChannelNumberNameView.setVisibility(VISIBLE);
        mImageView.setImageResource(R.drawable.ic_recent_thumbnail_default);
        mImageView.setBackgroundResource(R.color.channel_card);
        mGradientView.setVisibility(View.GONE);
        mProgressBar.setVisibility(GONE);

        setMetaViewEnabled(true);
        if (mMainActivity.getParentalControlSettings().isParentalControlsEnabled()
                && mChannel.isLocked()) {
            setMetaViewText(R.string.program_title_for_blocked_channel);
            return;
        } else {
            setMetaViewText("");
        }

        updateProgramInformation();
        mMetaViewFocused.measure(
                MeasureSpec.makeMeasureSpec(mCardImageWidth, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
        if (mExtendViewOnFocus = mMetaViewFocused.getLineCount() > 1) {
            setMetaViewFocusedAlpha(selected ? 1f : 0f);
        } else {
            setMetaViewFocusedAlpha(1f);
        }

        // Call super.onBind() at the end in order to make getCardHeight() return a proper value.
        super.onBind(channel, selected);
    }

    private static ImageLoader.ImageLoaderCallback<ChannelCardView> createProgramPosterArtCallback(
            ChannelCardView cardView, final Program program) {
        return new ImageLoader.ImageLoaderCallback<ChannelCardView>(cardView) {
            @Override
            public void onBitmapLoaded(ChannelCardView cardView, @Nullable Bitmap posterArt) {
                if (posterArt == null || cardView.mProgram == null
                        || program.getChannelId() != cardView.mProgram.getChannelId()
                        || program.getChannelId() != cardView.mChannel.getId()) {
                    return;
                }
                cardView.updatePosterArt(posterArt);
            }
        };
    }

    private void updatePosterArt(Bitmap posterArt) {
        mImageView.setImageBitmap(posterArt);
        mGradientView.setVisibility(View.VISIBLE);
    }

    @Override
    protected void onFocusAnimationStart(boolean selected) {
        if (mExtendViewOnFocus) {
            setMetaViewFocusedAlpha(selected ? 1f : 0f);
        }
    }

    @Override
    protected void onSetFocusAnimatedValue(float animatedValue) {
        super.onSetFocusAnimatedValue(animatedValue);
        if (mExtendViewOnFocus) {
            ViewGroup.LayoutParams params = mMetaViewUnfocused.getLayoutParams();
            params.height = Math.round(mProgramNameViewHeight
                    + (mExtendedTextViewCardHeight - mProgramNameViewHeight) * animatedValue);
            setMetaViewLayoutParams(params);
            setMetaViewFocusedAlpha(animatedValue);
        }
    }

    @Override
    protected float getCardHeight() {
        return (mExtendViewOnFocus && isFocused()) ? mExtendedCardHeight : mCardHeight;
    }

    private void updateProgramInformation() {
        if (mChannel == null) {
            return;
        }
        mProgram = mMainActivity.getProgramDataManager().getCurrentProgram(mChannel.getId());
        if (mProgram == null || TextUtils.isEmpty(mProgram.getTitle())) {
            setMetaViewEnabled(false);
            setMetaViewText(R.string.program_title_for_no_information);
        } else {
            setMetaViewText(mProgram.getTitle());
        }

        if (mProgram == null) {
            return;
        }

        long startTime = mProgram.getStartTimeUtcMillis();
        long endTime = mProgram.getEndTimeUtcMillis();
        long currTime = System.currentTimeMillis();
        mProgressBar.setVisibility(View.VISIBLE);
        if (currTime <= startTime) {
            mProgressBar.setProgress(0);
        } else if (currTime >= endTime) {
            mProgressBar.setProgress(100);
        } else {
            mProgressBar.setProgress((int) (100 * (currTime - startTime) / (endTime - startTime)));
        }

        if (!(getContext() instanceof MainActivity)) {
            Log.e(TAG, "Fails to check program's content rating.");
            return;
        }
        ParentalControlSettings parental = mMainActivity.getParentalControlSettings();
        if ((!parental.isParentalControlsEnabled()
                || !parental.isRatingBlocked(mProgram.getContentRatings()))
                && !TextUtils.isEmpty(mProgram.getPosterArtUri())) {
            mProgram.loadPosterArt(getContext(), mCardImageWidth, mCardImageHeight,
                    createProgramPosterArtCallback(this, mProgram));
        }
    }

    private void setMetaViewLayoutParams(ViewGroup.LayoutParams params) {
        mMetaViewFocused.setLayoutParams(params);
        mMetaViewUnfocused.setLayoutParams(params);
    }

    private void setMetaViewText(String text) {
        mMetaViewFocused.setText(text);
        mMetaViewUnfocused.setText(text);
    }

    private void setMetaViewText(int resId) {
        mMetaViewFocused.setText(resId);
        mMetaViewUnfocused.setText(resId);
    }

    private void setMetaViewEnabled(boolean enabled) {
        mMetaViewFocused.setEnabled(enabled);
        mMetaViewUnfocused.setEnabled(enabled);
    }

    private void setMetaViewFocusedAlpha(float focusedAlpha) {
        mMetaViewFocused.setAlpha(focusedAlpha);
        mMetaViewUnfocused.setAlpha(1f - focusedAlpha);
    }
}
