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
package android.car.cluster.demorenderer;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.content.Context;
import android.graphics.Bitmap;
import android.util.Log;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

/**
 * This class is responsible for drawing the whole instrument cluster.
 */
public class DemoInstrumentClusterView extends FrameLayout {

    private final String TAG = DemoInstrumentClusterView.class.getSimpleName();

    private TextView mSpeedView;
    private TextView mEventTitleView;
    private TextView mDistanceView;
    private View mNavPanel;
    private TextView mMediaArtistView;
    private TextView mMediaAlbumView;
    private TextView mMediaTrackView;
    private ImageView mMediaImageView;
    private View mMediaPanel;

    private View mPhonePanel;
    private TextView mPhoneTitle;
    private TextView mPhoneSubtitle;
    private ImageView mPhoneImage;

    private final Integer mAnimationDurationMs;

    public DemoInstrumentClusterView(Context context) {
        super(context);
        mAnimationDurationMs = getResources().getInteger(android.R.integer.config_longAnimTime);
        init();
    }

    public void setSpeed(String speed) {
        Log.d(TAG, "setSpeed, meterPerSecond: " + speed);
        mSpeedView.setText(speed);
    }

    public void showNavigation() {
        Log.d(TAG, "showNavigation");
        mEventTitleView.setText("");
        mDistanceView.setText("");
        mNavPanel.setVisibility(VISIBLE);
    }

    public void hideNavigation() {
        Log.d(TAG, "hideNavigation");
        mNavPanel.setVisibility(INVISIBLE);
    }

    public void setNextTurn(Bitmap image, String title) {
        Log.d(TAG, "setNextTurn, image: " + image + ", title: " + title);
        mEventTitleView.setText(title);
    }

    public void setNextTurnDistance(String distance) {
        Log.d(TAG, "setNextTurnDistance, distance: " + distance);
        mDistanceView.setText(distance);
    }

    public void setMediaData(final CharSequence artist, final CharSequence album,
            final CharSequence track, final Bitmap image) {
        Log.d(TAG, "setMediaData" + " artist = " + artist + ", album: " + album + ", track: " +
                track + ", bitmap: " + image);

        mMediaArtistView.setText(artist);
        mMediaAlbumView.setText(album);
        mMediaTrackView.setText(track);
        mMediaImageView.setImageBitmap(image);
    }

    private void showAnimated(final View view) {
        if (view.getVisibility() == VISIBLE && view.getAlpha() > 0) {
            return;
        }
        view.setAlpha(0);
        view.setVisibility(VISIBLE);
        view.animate()
                .alpha(1f)
                .setDuration(mAnimationDurationMs)
                .setListener(null);
    }

    private void hideAnimated(final View view) {
        if (view.getVisibility() == GONE) {
            return;
        }
        view.animate()
                .alpha(0f)
                .setDuration(mAnimationDurationMs)
                .setListener(new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator animation) {
                        view.setVisibility(GONE);
                    }
                });
    }

    public void showMedia() {
        Log.d(TAG, "showMedia");
        showAnimated(mMediaPanel);
    }

    public void hideMedia() {
        Log.d(TAG, "hideMedia");
        hideAnimated(mMediaPanel);
    }

    public void showPhone() {
        Log.d(TAG, "showPhone");
        mPhoneSubtitle.setText("");
        mPhoneImage.setImageResource(0); // To clear previous contact photo (if any).
        mPhoneTitle.setText("");
        showAnimated(mPhonePanel);
    }

    public void hidePhone() {
        Log.d(TAG, "hidePhone");
        hideAnimated(mPhonePanel);
    }

    public void setPhoneTitle(String number) {
        Log.d(TAG, "setPhoneTitle, number: " + number);
        mPhoneTitle.setText(number);
    }

    public void setPhoneSubtitle(String contact) {
        Log.d(TAG, "setPhoneContact, contact: " + contact);
        mPhoneSubtitle.setText(contact);
    }

    public void setPhoneImage(Bitmap photo) {
        Log.d(TAG, "setPhoneImage, photo: " + photo);
        mPhoneImage.setImageBitmap(photo);
    }

    private void init() {
        Log.d(TAG, "init");
        View rootView = inflate(getContext(), R.layout.instrument_cluster, null);
        mSpeedView = (TextView) rootView.findViewById(R.id.speed);
        mEventTitleView = (TextView) rootView.findViewById(R.id.nav_event_title);
        mDistanceView = (TextView) rootView.findViewById(R.id.nav_distance);
        mNavPanel = rootView.findViewById(R.id.nav_layout);

        mMediaPanel = rootView.findViewById(R.id.media_layout);
        mMediaArtistView = (TextView) rootView.findViewById(R.id.media_artist);
        mMediaAlbumView = (TextView) rootView.findViewById(R.id.media_album);
        mMediaTrackView = (TextView) rootView.findViewById(R.id.media_track);
        mMediaImageView = (ImageView) rootView.findViewById(R.id.media_image);

        mPhonePanel = rootView.findViewById(R.id.phone_layout);
        mPhoneImage = (ImageView) rootView.findViewById(R.id.phone_contact_photo);
        mPhoneSubtitle = (TextView) rootView.findViewById(R.id.phone_subtitle);
        mPhoneTitle = (TextView) rootView.findViewById(R.id.phone_title);

        setSpeed("0");

        addView(rootView);
    }
}
