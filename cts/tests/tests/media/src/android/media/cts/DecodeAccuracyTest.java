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
package android.media.cts;

import android.media.cts.R;

import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.Bitmap;
import android.view.View;

@TargetApi(24)
public class DecodeAccuracyTest extends DecodeAccuracyTestBase {

    private static final String TAG = DecodeAccuracyTest.class.getSimpleName();
    private static final String H264_VIDEO_FILE_NAME = "480ph264decodertest.mp4";
    private static final String VP9_VIDEO_FILE_NAME = "360pvp9decodertest.webm";
    private static final String H264_CROPPED_VIDEO_FILE_NAME = "520x360h264decodertest.mp4";
    private static final int ALLOWED_GREATEST_PIXEL_DIFFERENCE = 90;
    private static final int OFFSET = 10;

    /* <------------- Tests Using H264 -------------> */
    public void testH264GLViewVideoDecode() throws Exception {
        runH264DecodeAccuracyTest(
                new GLSurfaceViewFactory(),
                new VideoFormat(H264_VIDEO_FILE_NAME));
    }

    public void testH264GLViewLargerHeightVideoDecode() throws Exception {
        runH264DecodeAccuracyTest(
                new GLSurfaceViewFactory(),
                getLargerHeightVideoFormat(new VideoFormat(H264_VIDEO_FILE_NAME)));
    }

    public void testH264GLViewLargerWidthVideoDecode() throws Exception {
        runH264DecodeAccuracyTest(
                new GLSurfaceViewFactory(),
                getLargerWidthVideoFormat(new VideoFormat(H264_VIDEO_FILE_NAME)));
    }

    public void testH264SurfaceViewVideoDecode() throws Exception {
        runH264DecodeAccuracyTest(
                new SurfaceViewFactory(),
                new VideoFormat(H264_VIDEO_FILE_NAME));
    }

    public void testH264SurfaceViewLargerHeightVideoDecode() throws Exception {
        runH264DecodeAccuracyTest(
                new SurfaceViewFactory(),
                getLargerHeightVideoFormat(new VideoFormat(H264_VIDEO_FILE_NAME)));
    }

    public void testH264SurfaceViewLargerWidthVideoDecode() throws Exception {
        runH264DecodeAccuracyTest(
                new SurfaceViewFactory(),
                getLargerWidthVideoFormat(new VideoFormat(H264_VIDEO_FILE_NAME)));
    }

    /* <------------- Tests Using VP9 -------------> */
    public void testVP9GLViewVideoDecode() throws Exception {
        runVP9DecodeAccuracyTest(
                new GLSurfaceViewFactory(),
                new VideoFormat(VP9_VIDEO_FILE_NAME));
    }

    public void testVP9GLViewLargerHeightVideoDecode() throws Exception {
        runVP9DecodeAccuracyTest(
                new GLSurfaceViewFactory(),
                getLargerHeightVideoFormat(new VideoFormat(VP9_VIDEO_FILE_NAME)));
    }

    public void testVP9GLViewLargerWidthVideoDecode() throws Exception {
        runVP9DecodeAccuracyTest(
                new GLSurfaceViewFactory(),
                getLargerWidthVideoFormat(new VideoFormat(VP9_VIDEO_FILE_NAME)));
    }

    public void testVP9SurfaceViewVideoDecode() throws Exception {
        runVP9DecodeAccuracyTest(
                new SurfaceViewFactory(),
                new VideoFormat(VP9_VIDEO_FILE_NAME));
    }

    public void testVP9SurfaceViewLargerHeightVideoDecode() throws Exception {
        runVP9DecodeAccuracyTest(
                new SurfaceViewFactory(),
                getLargerHeightVideoFormat(new VideoFormat(VP9_VIDEO_FILE_NAME)));
    }

    public void testVP9SurfaceViewLargerWidthVideoDecode() throws Exception {
        runVP9DecodeAccuracyTest(
                new SurfaceViewFactory(),
                getLargerWidthVideoFormat(new VideoFormat(VP9_VIDEO_FILE_NAME)));
    }

    /* <------------- Tests H264 with cropping -------------> */
    public void testH264GLViewCroppedVideoDecode() throws Exception {
        runH264DecodeCroppedTest(
                new GLSurfaceViewFactory(),
                new VideoFormat(H264_CROPPED_VIDEO_FILE_NAME));
    }

    public void testH264SurfaceViewCroppedVideoDecode() throws Exception {
        runH264DecodeCroppedTest(
                new SurfaceViewFactory(),
                new VideoFormat(H264_CROPPED_VIDEO_FILE_NAME));
    }

    private void runH264DecodeAccuracyTest(
            VideoViewFactory videoViewFactory, VideoFormat videoFormat) {
        runDecodeAccuracyTest(videoViewFactory, videoFormat, R.raw.h264decodertestgolden);
    }

    private void runVP9DecodeAccuracyTest(
            VideoViewFactory videoViewFactory, VideoFormat videoFormat) {
        runDecodeAccuracyTest(videoViewFactory, videoFormat, R.raw.vp9decodertestgolden);
    }

    private void runH264DecodeCroppedTest(
            VideoViewFactory videoViewFactory, VideoFormat videoFormat) {
        runDecodeAccuracyTest(videoViewFactory, videoFormat, R.raw.h264decodertest520x360golden);
    }

    private void runDecodeAccuracyTest(
            VideoViewFactory videoViewFactory, VideoFormat videoFormat, int goldenResId) {
        checkNotNull(videoViewFactory);
        checkNotNull(videoFormat);
        View videoView = videoViewFactory.createView(getHelper().getContext());
        // If view is intended and available to display.
        if (videoView != null) {
            getHelper().generateView(videoView);
        }
        videoViewFactory.waitForViewIsAvailable();
        // In the case of SurfaceView, VideoViewSnapshot can only capture incoming frames,
        // so it needs to be created before start decoding.
        final VideoViewSnapshot videoViewSnapshot = videoViewFactory.getVideoViewSnapshot();
        decodeVideo(videoFormat, videoViewFactory);
        validateResult(videoFormat, videoViewSnapshot, goldenResId);

        if (videoView != null) {
            getHelper().cleanUpView(videoView);
        }
        videoViewFactory.release();
    }

    private void decodeVideo(VideoFormat videoFormat, VideoViewFactory videoViewFactory) {
        final SimplePlayer player = new SimplePlayer(getHelper().getContext());
        final SimplePlayer.PlayerResult playerResult = player.decodeVideoFrames(
                videoViewFactory.getSurface(), videoFormat, 10);
        assertTrue("Failed to configure video decoder.", playerResult.isConfigureSuccess());
        assertTrue("Failed to start video decoder.", playerResult.isStartSuccess());
        assertTrue("Failed to decode the video.", playerResult.isSuccess());
    }

    private void validateResult(
            VideoFormat videoFormat, VideoViewSnapshot videoViewSnapshot, int goldenResId) {
        final Bitmap result = getHelper().generateBitmapFromVideoViewSnapshot(videoViewSnapshot);
        final Bitmap golden = getHelper().generateBitmapFromImageResourceId(goldenResId);
        final BitmapCompare.Difference difference = BitmapCompare.computeMinimumDifference(
                result, golden, videoFormat.getOriginalWidth(), videoFormat.getOriginalHeight());
        assertTrue("With the best matched border crop ("
                    + difference.bestMatchBorderCrop.first + ", "
                    + difference.bestMatchBorderCrop.second + "), "
                    + "greatest pixel difference is "
                    + difference.greatestPixelDifference
                    + (difference.greatestPixelDifferenceCoordinates != null
                    ? " at (" + difference.greatestPixelDifferenceCoordinates.first + ", "
                    + difference.greatestPixelDifferenceCoordinates.second + ")" : "")
                    + " which is over the allowed difference " + ALLOWED_GREATEST_PIXEL_DIFFERENCE,
                    difference.greatestPixelDifference <= ALLOWED_GREATEST_PIXEL_DIFFERENCE);
    }

    private static VideoFormat getLargerHeightVideoFormat(VideoFormat videoFormat) {
        return new VideoFormat(videoFormat) {
            @Override
            public int getHeight() {
                return super.getHeight() + OFFSET;
            }
            @Override
            public int getMaxHeight() {
                return super.getHeight() * 2 + OFFSET;
            }
            @Override
            public int getMaxWidth() {
                return super.getWidth() * 2 + OFFSET;
            }
        };
    }

    private static VideoFormat getLargerWidthVideoFormat(VideoFormat videoFormat) {
        return new VideoFormat(videoFormat) {
            @Override
            public int getWidth() {
                return super.getWidth() + OFFSET;
            }
            @Override
            public int getMaxHeight() {
                return super.getHeight() * 2 + OFFSET;
            }
            @Override
            public int getMaxWidth() {
                return super.getWidth() * 2 + OFFSET;
            }
        };
    }

}
