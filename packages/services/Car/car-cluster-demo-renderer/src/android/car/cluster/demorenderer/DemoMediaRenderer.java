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

import android.car.cluster.demorenderer.MediaStateMonitor.MediaStateListener;
import android.graphics.Bitmap;
import android.media.MediaMetadata;
import android.media.session.PlaybackState;
import android.util.Log;

/**
 * Demo of rendering media data in instrument cluster.
 */
public class DemoMediaRenderer implements MediaStateListener {

    private static final String TAG = DemoMediaRenderer.class.getSimpleName();


    private final DemoInstrumentClusterView mView;

    private static final String[] PREFERRED_BITMAP_ORDER = {
            MediaMetadata.METADATA_KEY_ALBUM_ART,
            MediaMetadata.METADATA_KEY_ART,
            MediaMetadata.METADATA_KEY_DISPLAY_ICON
    };


    public DemoMediaRenderer(DemoInstrumentClusterView view) {
        mView = view;
    }

    @Override
    public void onPlaybackStateChanged(PlaybackState playbackState) {
        Log.d(TAG, "onPlaybackStateChanged: " + playbackState);
    }

    @Override
    public void onMetadataChanged(MediaMetadata metadata) {
        Log.d(TAG, "onMetadataChanged: " + metadata);
        if (metadata != null) {
            CharSequence artist = metadata.getText(MediaMetadata.METADATA_KEY_ARTIST);
            CharSequence album = metadata.getText(MediaMetadata.METADATA_KEY_ALBUM);
            CharSequence track = metadata.getText(MediaMetadata.METADATA_KEY_TITLE);
            Bitmap bitmap = getMetadataBitmap(metadata);

            mView.setMediaData(artist, album, track, bitmap);
            mView.showMedia();
        } else {
            mView.setMediaData(null, null, null, null);
            mView.hideMedia();
        }
    }

    private Bitmap getMetadataBitmap(MediaMetadata metadata) {
        // Get the best art bitmap we can find
        for (int i = 0; i < PREFERRED_BITMAP_ORDER.length; i++) {
            Bitmap bitmap = metadata.getBitmap(PREFERRED_BITMAP_ORDER[i]);
            if (bitmap != null) {
                return bitmap;
            }
        }
        return null;
    }

}
