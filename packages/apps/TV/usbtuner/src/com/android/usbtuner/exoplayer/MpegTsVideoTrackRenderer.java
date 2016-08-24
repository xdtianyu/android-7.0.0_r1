package com.android.usbtuner.exoplayer;

import android.content.Context;
import android.media.MediaCodec;
import android.os.Handler;

import com.google.android.exoplayer.DecoderInfo;
import com.google.android.exoplayer.ExoPlaybackException;
import com.google.android.exoplayer.MediaCodecSelector;
import com.google.android.exoplayer.MediaCodecUtil;
import com.google.android.exoplayer.MediaCodecVideoTrackRenderer;
import com.google.android.exoplayer.MediaFormatHolder;
import com.google.android.exoplayer.MediaSoftwareCodecUtil;
import com.google.android.exoplayer.SampleSource;
import com.android.tv.common.feature.CommonFeatures;

/**
 * MPEG-2 TS video track renderer
 */
public class MpegTsVideoTrackRenderer extends MediaCodecVideoTrackRenderer {

    private static final int VIDEO_PLAYBACK_DEADLINE_IN_MS = 5000;
    private static final int DROPPED_FRAMES_NOTIFICATION_THRESHOLD = 50;
    private static final int MIN_HD_HEIGHT = 720;
    private static final String MIMETYPE_MPEG2 = "video/mpeg2";

    private final boolean mIsSwCodecEnabled;
    private boolean mCodecIsSwPreferred;

    public MpegTsVideoTrackRenderer(Context context, SampleSource source, Handler handler,
            MediaCodecVideoTrackRenderer.EventListener listener) {
        super(context, source, MediaCodecSelector.DEFAULT,
                MediaCodec.VIDEO_SCALING_MODE_SCALE_TO_FIT, VIDEO_PLAYBACK_DEADLINE_IN_MS, handler,
                listener, DROPPED_FRAMES_NOTIFICATION_THRESHOLD);
        mIsSwCodecEnabled = CommonFeatures.USE_SW_CODEC_FOR_SD.isEnabled(context);
    }

    @Override
    protected DecoderInfo getDecoderInfo(MediaCodecSelector codecSelector, String mimeType,
            boolean requiresSecureDecoder) throws MediaCodecUtil.DecoderQueryException {
        try {
            if (mIsSwCodecEnabled && mCodecIsSwPreferred) {
                DecoderInfo swCodec = MediaSoftwareCodecUtil.getSoftwareDecoderInfo(
                        mimeType, requiresSecureDecoder);
                if (swCodec != null) {
                    return swCodec;
                }
            }
        } catch (MediaSoftwareCodecUtil.DecoderQueryException e) {
        }
        return super.getDecoderInfo(codecSelector, mimeType,requiresSecureDecoder);
    }

    @Override
    protected void onInputFormatChanged(MediaFormatHolder holder) throws ExoPlaybackException {
        mCodecIsSwPreferred = MIMETYPE_MPEG2.equalsIgnoreCase(holder.format.mimeType)
                && holder.format.height < MIN_HD_HEIGHT;
        super.onInputFormatChanged(holder);
    }
}
