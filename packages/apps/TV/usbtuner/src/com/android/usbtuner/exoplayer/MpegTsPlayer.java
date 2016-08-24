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

package com.android.usbtuner.exoplayer;

import android.media.AudioFormat;
import android.media.MediaCodec.CryptoException;
import android.media.MediaDataSource;
import android.os.Handler;
import android.os.Looper;
import android.support.annotation.IntDef;
import android.view.Surface;

import com.google.android.exoplayer.DummyTrackRenderer;
import com.google.android.exoplayer.ExoPlaybackException;
import com.google.android.exoplayer.ExoPlayer;
import com.google.android.exoplayer.MediaCodecAudioTrackRenderer;
import com.google.android.exoplayer.MediaCodecTrackRenderer.DecoderInitializationException;
import com.google.android.exoplayer.MediaCodecVideoTrackRenderer;
import com.google.android.exoplayer.TrackRenderer;
import com.google.android.exoplayer.audio.AudioCapabilities;
import com.google.android.exoplayer.audio.AudioTrack;
import com.android.usbtuner.data.Cea708Data;
import com.android.usbtuner.data.Cea708Data.CaptionEvent;
import com.android.usbtuner.exoplayer.Cea708TextTrackRenderer.CcListener;
import com.android.usbtuner.exoplayer.ac3.Ac3TrackRenderer;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * MPEG-2 TS stream player implementation using ExoPlayer.
 */
public class MpegTsPlayer implements ExoPlayer.Listener,
        MediaCodecVideoTrackRenderer.EventListener, Ac3TrackRenderer.EventListener {
    private int mCaptionServiceNumber = Cea708Data.EMPTY_SERVICE_NUMBER;

    /**
     * Interface definition for building specific track renderers.
     */
    public interface RendererBuilder {
        void buildRenderers(MpegTsPlayer mpegTsPlayer, MediaDataSource dataSource,
                RendererBuilderCallback callback);
    }

    /**
     * Interface definition for {@link RendererBuilder#buildRenderers} to notify the result.
     */
    public interface RendererBuilderCallback {
        void onRenderers(String[][] trackNames, TrackRenderer[] renderers);
        void onRenderersError(Exception e);
    }

    /**
     * Interface definition for a callback to be notified of changes in player state.
     */
    public interface Listener {
        void onStateChanged(int generation, boolean playWhenReady, int playbackState);
        void onError(int generation, Exception e);
        void onVideoSizeChanged(int generation, int width, int height,
                float pixelWidthHeightRatio);
        void onDrawnToSurface(MpegTsPlayer player, Surface surface);
        void onAudioUnplayable(int generation);
    }

    /**
     * Interface definition for a callback to be notified of changes on video display.
     */
    public interface VideoEventListener {
        /**
         * Notifies the caption event.
         */
        void onEmitCaptionEvent(CaptionEvent event);

        /**
         * Notifies the discovered caption service number.
         */
        void onDiscoverCaptionServiceNumber(int serviceNumber);
    }

    // Constants pulled into this class for convenience.
    @IntDef({STATE_IDLE, STATE_PREPARING, STATE_BUFFERING, STATE_READY, STATE_ENDED})
    @Retention(RetentionPolicy.SOURCE)
    public @interface PlaybackState {}
    public static final int STATE_IDLE = ExoPlayer.STATE_IDLE;
    public static final int STATE_PREPARING = ExoPlayer.STATE_PREPARING;
    public static final int STATE_BUFFERING = ExoPlayer.STATE_BUFFERING;
    public static final int STATE_READY = ExoPlayer.STATE_READY;
    public static final int STATE_ENDED = ExoPlayer.STATE_ENDED;

    public static final int RENDERER_COUNT = 3;
    public static final int MIN_BUFFER_MS = 200;
    public static final int MIN_REBUFFER_MS = 500;

    @IntDef({TRACK_TYPE_VIDEO, TRACK_TYPE_AUDIO, TRACK_TYPE_TEXT})
    @Retention(RetentionPolicy.SOURCE)
    public @interface TrackType {}
    public static final int TRACK_TYPE_VIDEO = 0;
    public static final int TRACK_TYPE_AUDIO = 1;
    public static final int TRACK_TYPE_TEXT = 2;

    @IntDef({RENDERER_BUILDING_STATE_IDLE, RENDERER_BUILDING_STATE_BUILDING,
        RENDERER_BUILDING_STATE_BUILT})
    @Retention(RetentionPolicy.SOURCE)
    public @interface RendererBuildingState {}
    private static final int RENDERER_BUILDING_STATE_IDLE = 1;
    private static final int RENDERER_BUILDING_STATE_BUILDING = 2;
    private static final int RENDERER_BUILDING_STATE_BUILT = 3;

    private final RendererBuilder mRendererBuilder;
    private final ExoPlayer mPlayer;
    private final Handler mMainHandler;
    private final int mPlayerGeneration;
    private final AudioCapabilities mAudioCapabilities;

    private Listener mListener;
    @RendererBuildingState private int mRendererBuildingState;
    @PlaybackState private int mLastReportedPlaybackState;
    private boolean mLastReportedPlayWhenReady;

    private Surface mSurface;
    private InternalRendererBuilderCallback mBuilderCallback;
    private TrackRenderer mVideoRenderer;
    private TrackRenderer mAudioRenderer;

    private String[][] mTrackNames;
    private int[] mSelectedTracks;

    private Cea708TextTrackRenderer mTextRenderer;
    private CcListener mCcListener;
    private VideoEventListener mVideoEventListener;

    public MpegTsPlayer(int playerGeneration, RendererBuilder rendererBuilder, Handler handler,
            AudioCapabilities capabilities, Listener listener) {
        mRendererBuilder = rendererBuilder;
        mPlayer = ExoPlayer.Factory.newInstance(RENDERER_COUNT, MIN_BUFFER_MS, MIN_REBUFFER_MS);
        mPlayer.addListener(this);
        mMainHandler = handler;
        mPlayerGeneration = playerGeneration;
        mAudioCapabilities = capabilities;
        mLastReportedPlaybackState = STATE_IDLE;
        mRendererBuildingState = RENDERER_BUILDING_STATE_IDLE;
        mSelectedTracks = new int[RENDERER_COUNT];
        mCcListener = new MpegTsCcListener();
        mListener = listener;
    }

    public void setVideoEventListener(VideoEventListener videoEventListener) {
        mVideoEventListener = videoEventListener;
    }

    public void setCaptionServiceNumber(int captionServiceNumber) {
        mCaptionServiceNumber = captionServiceNumber;
        if (mTextRenderer != null) {
            mPlayer.sendMessage(mTextRenderer,
                    Cea708TextTrackRenderer.MSG_SERVICE_NUMBER, mCaptionServiceNumber);
        }
    }

    public void setSurface(Surface surface) {
        mSurface = surface;
        pushSurface(false);
    }

    public Surface getSurface() {
        return mSurface;
    }

    public void blockingClearSurface() {
        mSurface = null;
        pushSurface(true);
    }

    public String[] getTracks(int type) {
        return mTrackNames == null ? null : mTrackNames[type];
    }

    public int getSelectedTrackIndex(int type) {
        return mSelectedTracks[type];
    }

    public void selectTrack(int type, int index) {
        if (mSelectedTracks[type] == index) {
            return;
        }
        mSelectedTracks[type] = index;
        pushTrackSelection(type, true);
    }

    public void prepare(MediaDataSource source) {
        if (mRendererBuildingState == RENDERER_BUILDING_STATE_BUILT) {
            mPlayer.stop();
        }
        if (mBuilderCallback != null) {
            mBuilderCallback.cancel();
        }
        mRendererBuildingState = RENDERER_BUILDING_STATE_BUILDING;
        maybeReportPlayerState();
        mBuilderCallback = new InternalRendererBuilderCallback();
        mRendererBuilder.buildRenderers(this, source, mBuilderCallback);
    }

    /* package */ void onRenderers(String[][] trackNames, TrackRenderer[] renderers) {
        mBuilderCallback = null;

        // Normalize the results.
        if (trackNames == null) {
            trackNames = new String[RENDERER_COUNT][];
        }
        for (int i = 0; i < RENDERER_COUNT; i++) {
            if (renderers[i] == null) {
                // Convert a null renderer to a dummy renderer.
                renderers[i] = new DummyTrackRenderer();
            }
        }
        mVideoRenderer = renderers[TRACK_TYPE_VIDEO];
        mAudioRenderer = renderers[TRACK_TYPE_AUDIO];
        mTextRenderer = (Cea708TextTrackRenderer) renderers[TRACK_TYPE_TEXT];
        mTextRenderer.setCcListener(mCcListener);
        mPlayer.sendMessage(
                mTextRenderer, Cea708TextTrackRenderer.MSG_SERVICE_NUMBER, mCaptionServiceNumber);
        mTrackNames = trackNames;
        mRendererBuildingState = RENDERER_BUILDING_STATE_BUILT;
        pushSurface(false);
        mPlayer.prepare(renderers);
        pushTrackSelection(TRACK_TYPE_VIDEO, true);
        pushTrackSelection(TRACK_TYPE_AUDIO, true);
        pushTrackSelection(TRACK_TYPE_TEXT, true);
    }

    /* package */ void onRenderersError(Exception e) {
        mBuilderCallback = null;
        if (mListener != null) {
            mListener.onError(mPlayerGeneration, e);
        }
        mRendererBuildingState = RENDERER_BUILDING_STATE_IDLE;
        maybeReportPlayerState();
    }

    public void setPlayWhenReady(boolean playWhenReady) {
        mPlayer.setPlayWhenReady(playWhenReady);
    }

    public void seekTo(long positionMs) {
        mPlayer.seekTo(positionMs);
    }

    public void release() {
        if (mBuilderCallback != null) {
            mBuilderCallback.cancel();
            mBuilderCallback = null;
        }
        mRendererBuildingState = RENDERER_BUILDING_STATE_IDLE;
        mSurface = null;
        mListener = null;
        mPlayer.release();
    }

    @PlaybackState public int getPlaybackState() {
        if (mRendererBuildingState == RENDERER_BUILDING_STATE_BUILDING) {
            return STATE_PREPARING;
        }
        return mPlayer.getPlaybackState();
    }

    public boolean isPlaying() {
        @PlaybackState int state = getPlaybackState();
        return (state == STATE_READY || state == STATE_BUFFERING)
                && mPlayer.getPlayWhenReady();
    }

    public boolean isBuffering() {
        return getPlaybackState() == STATE_BUFFERING;
    }

    public long getCurrentPosition() {
        return mPlayer.getCurrentPosition();
    }

    public long getDuration() {
        return mPlayer.getDuration();
    }

    public int getBufferedPercentage() {
        return mPlayer.getBufferedPercentage();
    }

    public boolean getPlayWhenReady() {
        return mPlayer.getPlayWhenReady();
    }

    public void setVolume(float volume) {
        mPlayer.sendMessage(mAudioRenderer, MediaCodecAudioTrackRenderer.MSG_SET_VOLUME, volume);
    }

    public void setAudioTrack(boolean enable) {
        mPlayer.sendMessage(mAudioRenderer, Ac3TrackRenderer.MSG_SET_AUDIO_TRACK, enable ? 1 : 0);
    }

    public boolean isAc3Playable() {
        return mAudioCapabilities != null
                && mAudioCapabilities.supportsEncoding(AudioFormat.ENCODING_AC3);
    }

    public void onAudioUnplayable() {
        if (mListener != null) {
            mListener.onAudioUnplayable(mPlayerGeneration);
        }
    }

    /* package */ Looper getPlaybackLooper() {
        return mPlayer.getPlaybackLooper();
    }

    /* package */ Handler getMainHandler() {
        return mMainHandler;
    }

    @Override
    public void onPlayerStateChanged(boolean playWhenReady, int state) {
        maybeReportPlayerState();
    }

    @Override
    public void onPlayerError(ExoPlaybackException exception) {
        mRendererBuildingState = RENDERER_BUILDING_STATE_IDLE;
        if (mListener != null) {
            mListener.onError(mPlayerGeneration, exception);
        }
    }

    @Override
    public void onVideoSizeChanged(int width, int height, int unappliedRotationDegrees,
            float pixelWidthHeightRatio) {
        if (mListener != null) {
            mListener.onVideoSizeChanged(mPlayerGeneration, width, height, pixelWidthHeightRatio);
        }
    }

    @Override
    public void onDecoderInitialized(String decoderName, long elapsedRealtimeMs,
            long initializationDurationMs) {
        // TODO
    }

    @Override
    public void onDecoderInitializationError(DecoderInitializationException e) {
        // Do nothing.
    }

    @Override
    public void onAudioTrackInitializationError(AudioTrack.InitializationException e) {
        onAudioUnplayable();
    }

    @Override
    public void onAudioTrackWriteError(AudioTrack.WriteException e) {
        // Do nothing.
    }

    @Override
    public void onCryptoError(CryptoException e) {
        // Do nothing.
    }

    @Override
    public void onPlayWhenReadyCommitted() {
        // Do nothing.
    }

    @Override
    public void onDrawnToSurface(Surface surface) {
        if (mListener != null) {
            mListener.onDrawnToSurface(this, surface);
        }
    }

    @Override
    public void onDroppedFrames(int count, long elapsed) {
        // Do nothing.
    }

    private void maybeReportPlayerState() {
        boolean playWhenReady = mPlayer.getPlayWhenReady();
        @PlaybackState int playbackState = getPlaybackState();
        if (mLastReportedPlayWhenReady != playWhenReady
                || mLastReportedPlaybackState != playbackState) {
            if (mListener != null) {
                if (playbackState == STATE_ENDED) {
                    mListener.onStateChanged(mPlayerGeneration, playWhenReady, STATE_ENDED);
                }
                else if (playbackState == STATE_READY) {
                    mListener.onStateChanged(mPlayerGeneration, playWhenReady, STATE_READY);
                }
            }
            mLastReportedPlayWhenReady = playWhenReady;
            mLastReportedPlaybackState = playbackState;
        }
    }

    private void pushSurface(boolean blockForSurfacePush) {
        if (mRendererBuildingState != RENDERER_BUILDING_STATE_BUILT) {
            return;
        }

        if (blockForSurfacePush) {
            mPlayer.blockingSendMessage(
                    mVideoRenderer, MediaCodecVideoTrackRenderer.MSG_SET_SURFACE, mSurface);
        } else {
            mPlayer.sendMessage(
                    mVideoRenderer, MediaCodecVideoTrackRenderer.MSG_SET_SURFACE, mSurface);
        }
    }

    private void pushTrackSelection(@TrackType int type, boolean allowRendererEnable) {
        if (mRendererBuildingState != RENDERER_BUILDING_STATE_BUILT) {
            return;
        }
        mPlayer.setSelectedTrack(type, allowRendererEnable ? 0 : -1);
    }

    private class MpegTsCcListener implements CcListener {

        @Override
        public void emitEvent(CaptionEvent captionEvent) {
            if (mVideoEventListener != null) {
                mVideoEventListener.onEmitCaptionEvent(captionEvent);
            }
        }

        @Override
        public void discoverServiceNumber(int serviceNumber) {
            if (mVideoEventListener != null) {
                mVideoEventListener.onDiscoverCaptionServiceNumber(serviceNumber);
            }
        }
    }

    private class InternalRendererBuilderCallback implements RendererBuilderCallback {
        private boolean canceled;

        public void cancel() {
            canceled = true;
        }

        @Override
        public void onRenderers(String[][] trackNames, TrackRenderer[] renderers) {
            if (!canceled) {
                MpegTsPlayer.this.onRenderers(trackNames, renderers);
            }
        }

        @Override
        public void onRenderersError(Exception e) {
            if (!canceled) {
                MpegTsPlayer.this.onRenderersError(e);
            }
        }
    }
}
