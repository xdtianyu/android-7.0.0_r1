/*
 * Copyright (C) 2009 The Android Open Source Project
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

import android.content.pm.PackageManager;
import android.cts.util.MediaUtils;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.hardware.Camera;
import android.media.CamcorderProfile;
import android.media.EncoderCapabilities;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.media.MediaMetadataRetriever;
import android.media.MediaRecorder;
import android.media.EncoderCapabilities.VideoEncoderCap;
import android.media.MediaRecorder.OnErrorListener;
import android.media.MediaRecorder.OnInfoListener;
import android.media.MediaMetadataRetriever;
import android.os.Environment;
import android.os.ConditionVariable;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;
import android.view.Surface;

import android.util.Log;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.lang.InterruptedException;
import java.lang.Runnable;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class MediaRecorderTest extends ActivityInstrumentationTestCase2<MediaStubActivity> {
    private final String TAG = "MediaRecorderTest";
    private final String OUTPUT_PATH;
    private final String OUTPUT_PATH2;
    private static final float TOLERANCE = 0.0002f;
    private static final int RECORD_TIME_MS = 3000;
    private static final int RECORD_TIME_LAPSE_MS = 6000;
    private static final int RECORD_TIME_LONG_MS = 20000;
    private static final int RECORDED_DUR_TOLERANCE_MS = 1000;
    // Tolerate 4 frames off at maximum
    private static final float RECORDED_DUR_TOLERANCE_FRAMES = 4f;
    private static final int VIDEO_WIDTH = 176;
    private static final int VIDEO_HEIGHT = 144;
    private static int mVideoWidth = VIDEO_WIDTH;
    private static int mVideoHeight = VIDEO_HEIGHT;
    private static final int VIDEO_BIT_RATE_IN_BPS = 128000;
    private static final double VIDEO_TIMELAPSE_CAPTURE_RATE_FPS = 1.0;
    private static final int AUDIO_BIT_RATE_IN_BPS = 12200;
    private static final int AUDIO_NUM_CHANNELS = 1;
    private static final int AUDIO_SAMPLE_RATE_HZ = 8000;
    private static final long MAX_FILE_SIZE = 5000;
    private static final int MAX_FILE_SIZE_TIMEOUT_MS = 5 * 60 * 1000;
    private static final int MAX_DURATION_MSEC = 2000;
    private static final float LATITUDE = 0.0000f;
    private static final float LONGITUDE  = -180.0f;
    private static final int NORMAL_FPS = 30;
    private static final int TIME_LAPSE_FPS = 5;
    private static final int SLOW_MOTION_FPS = 120;
    private static final List<VideoEncoderCap> mVideoEncoders =
            EncoderCapabilities.getVideoEncoders();

    private boolean mOnInfoCalled;
    private boolean mOnErrorCalled;
    private File mOutFile;
    private File mOutFile2;
    private Camera mCamera;
    private MediaStubActivity mActivity = null;

    private MediaRecorder mMediaRecorder;
    private ConditionVariable mMaxDurationCond;
    private ConditionVariable mMaxFileSizeCond;

    public MediaRecorderTest() {
        super("android.media.cts", MediaStubActivity.class);
        OUTPUT_PATH = new File(Environment.getExternalStorageDirectory(),
                "record.out").getAbsolutePath();
        OUTPUT_PATH2 = new File(Environment.getExternalStorageDirectory(),
                "record2.out").getAbsolutePath();
    }

    private void completeOnUiThread(final Runnable runnable) {
        final CountDownLatch latch = new CountDownLatch(1);
        getActivity().runOnUiThread(new Runnable() {
            @Override
            public void run() {
                runnable.run();
                latch.countDown();
            }
        });
        try {
            // if UI thread does not run, things will fail anyway
            assertTrue(latch.await(10, TimeUnit.SECONDS));
        } catch (java.lang.InterruptedException e) {
            fail("should not be interrupted");
        }
    }

    @Override
    protected void setUp() throws Exception {
        mActivity = getActivity();
        completeOnUiThread(new Runnable() {
            @Override
            public void run() {
                mMediaRecorder = new MediaRecorder();
                mOutFile = new File(OUTPUT_PATH);
                mOutFile2 = new File(OUTPUT_PATH2);

                mMaxDurationCond = new ConditionVariable();
                mMaxFileSizeCond = new ConditionVariable();

                mMediaRecorder.setOutputFile(OUTPUT_PATH);
                mMediaRecorder.setOnInfoListener(new OnInfoListener() {
                    public void onInfo(MediaRecorder mr, int what, int extra) {
                        mOnInfoCalled = true;
                        if (what ==
                            MediaRecorder.MEDIA_RECORDER_INFO_MAX_DURATION_REACHED) {
                            Log.v(TAG, "max duration reached");
                            mMaxDurationCond.open();
                        } else if (what ==
                            MediaRecorder.MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED) {
                            Log.v(TAG, "max file size reached");
                            mMaxFileSizeCond.open();
                        }
                    }
                });
                mMediaRecorder.setOnErrorListener(new OnErrorListener() {
                    public void onError(MediaRecorder mr, int what, int extra) {
                        mOnErrorCalled = true;
                    }
                });
            }
        });
        super.setUp();
    }

    @Override
    protected void tearDown() throws Exception {
        if (mMediaRecorder != null) {
            mMediaRecorder.release();
            mMediaRecorder = null;
        }
        if (mOutFile != null && mOutFile.exists()) {
            mOutFile.delete();
        }
        if (mOutFile2 != null && mOutFile2.exists()) {
            mOutFile2.delete();
        }
        if (mCamera != null)  {
            mCamera.release();
            mCamera = null;
        }
        mMaxDurationCond.close();
        mMaxDurationCond = null;
        mMaxFileSizeCond.close();
        mMaxFileSizeCond = null;
        mActivity = null;
        super.tearDown();
    }

    public void testRecorderCamera() throws Exception {
        int width;
        int height;
        Camera camera = null;
        if (!hasCamera()) {
            return;
        }
        // Try to get camera profile for QUALITY_LOW; if unavailable,
        // set the video size to default value.
        CamcorderProfile profile = CamcorderProfile.get(
                0 /* cameraId */, CamcorderProfile.QUALITY_LOW);
        if (profile != null) {
            width = profile.videoFrameWidth;
            height = profile.videoFrameHeight;
        } else {
            width = VIDEO_WIDTH;
            height = VIDEO_HEIGHT;
        }
        mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.DEFAULT);
        mMediaRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.DEFAULT);
        mMediaRecorder.setVideoSize(width, height);
        mMediaRecorder.setVideoEncodingBitRate(VIDEO_BIT_RATE_IN_BPS);
        mMediaRecorder.setPreviewDisplay(mActivity.getSurfaceHolder().getSurface());
        mMediaRecorder.prepare();
        mMediaRecorder.start();
        Thread.sleep(RECORD_TIME_MS);
        mMediaRecorder.stop();
        checkOutputExist();
    }

    @UiThreadTest
    public void testSetCamera() throws Exception {
        recordVideoUsingCamera(false, false);
    }

    public void testRecorderTimelapsedVideo() throws Exception {
        recordVideoUsingCamera(true, false);
    }

    public void testRecorderPauseResume() throws Exception {
        recordVideoUsingCamera(false, true);
    }

    public void testRecorderPauseResumeOnTimeLapse() throws Exception {
        recordVideoUsingCamera(true, true);
    }

    private void recordVideoUsingCamera(boolean timelapse, boolean pause) throws Exception {
        int nCamera = Camera.getNumberOfCameras();
        int durMs = timelapse? RECORD_TIME_LAPSE_MS: RECORD_TIME_MS;
        for (int cameraId = 0; cameraId < nCamera; cameraId++) {
            mCamera = Camera.open(cameraId);
            setSupportedResolution(mCamera);
            recordVideoUsingCamera(mCamera, OUTPUT_PATH, durMs, timelapse, pause);
            mCamera.release();
            mCamera = null;
            assertTrue(checkLocationInFile(OUTPUT_PATH));
        }
    }

    private void setSupportedResolution(Camera camera) {
        Camera.Parameters parameters = camera.getParameters();
        List<Camera.Size> videoSizes = parameters.getSupportedVideoSizes();
        // getSupportedVideoSizes returns null when separate video/preview size
        // is not supported.
        if (videoSizes == null) {
            videoSizes = parameters.getSupportedPreviewSizes();
        }
        for (Camera.Size size : videoSizes)
        {
            if (size.width == VIDEO_WIDTH && size.height == VIDEO_HEIGHT) {
                mVideoWidth = VIDEO_WIDTH;
                mVideoHeight = VIDEO_HEIGHT;
                return;
            }
        }
        mVideoWidth = videoSizes.get(0).width;
        mVideoHeight = videoSizes.get(0).height;
    }

    private void recordVideoUsingCamera(
            Camera camera, String fileName, int durMs, boolean timelapse, boolean pause)
        throws Exception {
        // FIXME:
        // We should add some test case to use Camera.Parameters.getPreviewFpsRange()
        // to get the supported video frame rate range.
        Camera.Parameters params = camera.getParameters();
        int frameRate = params.getPreviewFrameRate();

        camera.unlock();
        mMediaRecorder.setCamera(camera);
        mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.DEFAULT);
        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.DEFAULT);
        mMediaRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.DEFAULT);
        mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.DEFAULT);
        mMediaRecorder.setVideoFrameRate(frameRate);
        mMediaRecorder.setVideoSize(mVideoWidth, mVideoHeight);
        mMediaRecorder.setPreviewDisplay(mActivity.getSurfaceHolder().getSurface());
        mMediaRecorder.setOutputFile(fileName);
        mMediaRecorder.setLocation(LATITUDE, LONGITUDE);
        final double captureRate = VIDEO_TIMELAPSE_CAPTURE_RATE_FPS;
        if (timelapse) {
            mMediaRecorder.setCaptureRate(captureRate);
        }

        mMediaRecorder.prepare();
        mMediaRecorder.start();
        if (pause) {
            Thread.sleep(durMs / 2);
            mMediaRecorder.pause();
            Thread.sleep(durMs / 2);
            mMediaRecorder.resume();
            Thread.sleep(durMs / 2);
        } else {
            Thread.sleep(durMs);
        }
        mMediaRecorder.stop();
        assertTrue(mOutFile.exists());

        int targetDurMs = timelapse? ((int) (durMs * (captureRate / frameRate))): durMs;
        boolean hasVideo = true;
        boolean hasAudio = timelapse? false: true;
        checkTracksAndDuration(targetDurMs, hasVideo, hasAudio, fileName, frameRate);
    }

    private void checkTracksAndDuration(
            int targetMs, boolean hasVideo, boolean hasAudio, String fileName,
            float frameRate) throws Exception {
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        retriever.setDataSource(fileName);
        String hasVideoStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_HAS_VIDEO);
        String hasAudioStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_HAS_AUDIO);
        assertTrue(hasVideo? hasVideoStr != null : hasVideoStr == null);
        assertTrue(hasAudio? hasAudioStr != null : hasAudioStr == null);
        // FIXME:
        // If we could use fixed frame rate for video recording, we could also do more accurate
        // check on the duration.
        String durStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION);
        assertTrue(durStr != null);
        int duration = Integer.parseInt(durStr);
        assertTrue("duration is non-positive: dur = " + duration, duration > 0);
        if (targetMs != 0) {
            float toleranceMs = RECORDED_DUR_TOLERANCE_FRAMES * (1000f / frameRate);
            assertTrue(String.format("duration is too large: dur = %d, target = %d, tolerance = %f",
                        duration, targetMs, toleranceMs),
                    duration <= targetMs + toleranceMs);
        }

        retriever.release();
        retriever = null;
    }

    private boolean checkLocationInFile(String fileName) {
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        retriever.setDataSource(fileName);
        String location = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_LOCATION);
        if (location == null) {
            retriever.release();
            Log.v(TAG, "No location information found in file " + fileName);
            return false;
        }

        // parsing String location and recover the location inforamtion in floats
        // Make sure the tolerance is very small - due to rounding errors?.
        Log.v(TAG, "location: " + location);

        // Get the position of the -/+ sign in location String, which indicates
        // the beginning of the longtitude.
        int index = location.lastIndexOf('-');
        if (index == -1) {
            index = location.lastIndexOf('+');
        }
        assertTrue("+ or - is not found", index != -1);
        assertTrue("+ or - is only found at the beginning", index != 0);
        float latitude = Float.parseFloat(location.substring(0, index - 1));
        float longitude = Float.parseFloat(location.substring(index));
        assertTrue("Incorrect latitude: " + latitude, Math.abs(latitude - LATITUDE) <= TOLERANCE);
        assertTrue("Incorrect longitude: " + longitude, Math.abs(longitude - LONGITUDE) <= TOLERANCE);
        retriever.release();
        return true;
    }

    private void checkOutputExist() {
        assertTrue(mOutFile.exists());
        assertTrue(mOutFile.length() > 0);
        assertTrue(mOutFile.delete());
    }

    public void testRecorderVideo() throws Exception {
        if (!hasCamera()) {
            return;
        }
        mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.DEFAULT);
        mMediaRecorder.setOutputFile(OUTPUT_PATH2);
        mMediaRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.DEFAULT);
        mMediaRecorder.setPreviewDisplay(mActivity.getSurfaceHolder().getSurface());
        mMediaRecorder.setVideoSize(VIDEO_WIDTH, VIDEO_HEIGHT);

        FileOutputStream fos = new FileOutputStream(OUTPUT_PATH2);
        FileDescriptor fd = fos.getFD();
        mMediaRecorder.setOutputFile(fd);
        long maxFileSize = MAX_FILE_SIZE * 10;
        recordMedia(maxFileSize, mOutFile2);
        assertFalse(checkLocationInFile(OUTPUT_PATH2));
        fos.close();
    }

    public void testRecordingAudioInRawFormats() throws Exception {
        int testsRun = 0;
        if (hasAmrNb()) {
            testsRun += testRecordAudioInRawFormat(
                    MediaRecorder.OutputFormat.AMR_NB,
                    MediaRecorder.AudioEncoder.AMR_NB);
        }

        if (hasAmrWb()) {
            testsRun += testRecordAudioInRawFormat(
                    MediaRecorder.OutputFormat.AMR_WB,
                    MediaRecorder.AudioEncoder.AMR_WB);
        }

        if (hasAac()) {
            testsRun += testRecordAudioInRawFormat(
                    MediaRecorder.OutputFormat.AAC_ADTS,
                    MediaRecorder.AudioEncoder.AAC);
        }
        if (testsRun == 0) {
            MediaUtils.skipTest("no audio codecs or microphone");
        }
    }

    private int testRecordAudioInRawFormat(
            int fileFormat, int codec) throws Exception {
        if (!hasMicrophone()) {
            return 0; // skip
        }
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        mMediaRecorder.setOutputFormat(fileFormat);
        mMediaRecorder.setOutputFile(OUTPUT_PATH);
        mMediaRecorder.setAudioEncoder(codec);
        recordMedia(MAX_FILE_SIZE, mOutFile);
        return 1;
    }

    public void testRecordAudioFromAudioSourceUnprocessed() throws Exception {
        if (!hasMicrophone()) {
            return; // skip
        }
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.UNPROCESSED);
        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.DEFAULT);
        mMediaRecorder.setOutputFile(OUTPUT_PATH);
        mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.DEFAULT);
        recordMedia(MAX_FILE_SIZE, mOutFile);
    }

    public void testGetAudioSourceMax() throws Exception {
        final int max = MediaRecorder.getAudioSourceMax();
        assertTrue(MediaRecorder.AudioSource.DEFAULT <= max);
        assertTrue(MediaRecorder.AudioSource.MIC <= max);
        assertTrue(MediaRecorder.AudioSource.CAMCORDER <= max);
        assertTrue(MediaRecorder.AudioSource.VOICE_CALL <= max);
        assertTrue(MediaRecorder.AudioSource.VOICE_COMMUNICATION <= max);
        assertTrue(MediaRecorder.AudioSource.VOICE_DOWNLINK <= max);
        assertTrue(MediaRecorder.AudioSource.VOICE_RECOGNITION <= max);
        assertTrue(MediaRecorder.AudioSource.VOICE_UPLINK <= max);
        assertTrue(MediaRecorder.AudioSource.UNPROCESSED <= max);
    }

    public void testRecorderAudio() throws Exception {
        if (!hasMicrophone() || !hasAmrNb()) {
            MediaUtils.skipTest("no audio codecs or microphone");
            return;
        }
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        assertEquals(0, mMediaRecorder.getMaxAmplitude());
        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
        mMediaRecorder.setOutputFile(OUTPUT_PATH);
        mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);
        mMediaRecorder.setAudioChannels(AUDIO_NUM_CHANNELS);
        mMediaRecorder.setAudioSamplingRate(AUDIO_SAMPLE_RATE_HZ);
        mMediaRecorder.setAudioEncodingBitRate(AUDIO_BIT_RATE_IN_BPS);
        recordMedia(MAX_FILE_SIZE, mOutFile);
    }

    public void testOnInfoListener() throws Exception {
        if (!hasMicrophone() || !hasAmrNb()) {
            MediaUtils.skipTest("no audio codecs or microphone");
            return;
        }
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
        mMediaRecorder.setMaxDuration(MAX_DURATION_MSEC);
        mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);
        mMediaRecorder.prepare();
        mMediaRecorder.start();
        Thread.sleep(RECORD_TIME_MS);
        assertTrue(mOnInfoCalled);
    }

    public void testSetMaxDuration() throws Exception {
        if (!hasMicrophone() || !hasAmrNb()) {
            MediaUtils.skipTest("no audio codecs or microphone");
            return;
        }
        testSetMaxDuration(RECORD_TIME_LONG_MS, RECORDED_DUR_TOLERANCE_MS);
    }

    private void testSetMaxDuration(long durationMs, long toleranceMs) throws Exception {
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
        mMediaRecorder.setMaxDuration((int)durationMs);
        mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);
        mMediaRecorder.prepare();
        mMediaRecorder.start();
        long startTimeMs = System.currentTimeMillis();
        if (!mMaxDurationCond.block(durationMs + toleranceMs)) {
            fail("timed out waiting for MEDIA_RECORDER_INFO_MAX_DURATION_REACHED");
        }
        long endTimeMs = System.currentTimeMillis();
        long actualDurationMs = endTimeMs - startTimeMs;
        mMediaRecorder.stop();
        checkRecordedTime(durationMs, actualDurationMs, toleranceMs);
    }

    private void checkRecordedTime(long expectedMs, long actualMs, long tolerance) {
        assertEquals(expectedMs, actualMs, tolerance);
        long actualFileDurationMs = getRecordedFileDurationMs(OUTPUT_PATH);
        assertEquals(actualFileDurationMs, actualMs, tolerance);
    }

    private int getRecordedFileDurationMs(final String fileName) {
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        retriever.setDataSource(fileName);
        String durationStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION);
        assertNotNull(durationStr);
        return Integer.parseInt(durationStr);
    }

    public void testSetMaxFileSize() throws Exception {
        testSetMaxFileSize(512 * 1024, 50 * 1024);
    }

    private void testSetMaxFileSize(
            long fileSize, long tolerance) throws Exception {
        if (!hasMicrophone() || !hasCamera() || !hasAmrNb() || !hasH264()) {
            MediaUtils.skipTest("no microphone, camera, or codecs");
            return;
        }
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
        mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);
        mMediaRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);
        mMediaRecorder.setVideoSize(VIDEO_WIDTH, VIDEO_HEIGHT);
        mMediaRecorder.setVideoEncodingBitRate(256000);
        mMediaRecorder.setPreviewDisplay(mActivity.getSurfaceHolder().getSurface());
        mMediaRecorder.setMaxFileSize(fileSize);
        mMediaRecorder.prepare();
        mMediaRecorder.start();

        // Recording a scene with moving objects would greatly help reduce
        // the time for waiting.
        if (!mMaxFileSizeCond.block(MAX_FILE_SIZE_TIMEOUT_MS)) {
            fail("timed out waiting for MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED");
        }
        mMediaRecorder.stop();
        checkOutputFileSize(OUTPUT_PATH, fileSize, tolerance);
    }

    private void checkOutputFileSize(final String fileName, long fileSize, long tolerance) {
        assertTrue(mOutFile.exists());
        assertEquals(fileSize, mOutFile.length(), tolerance);
        assertTrue(mOutFile.delete());
    }

    public void testOnErrorListener() throws Exception {
        if (!hasMicrophone() || !hasAmrNb()) {
            MediaUtils.skipTest("no audio codecs or microphone");
            return;
        }
        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.DEFAULT);
        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
        mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);

        recordMedia(MAX_FILE_SIZE, mOutFile);
        // TODO: how can we trigger a recording error?
        assertFalse(mOnErrorCalled);
    }

    private void setupRecorder(String filename, boolean useSurface, boolean hasAudio)
            throws Exception {
        int codec = MediaRecorder.VideoEncoder.H264;
        int frameRate = getMaxFrameRateForCodec(codec);
        if (mMediaRecorder == null) {
            mMediaRecorder = new MediaRecorder();
        }

        if (!useSurface) {
            mCamera = Camera.open(0);
            Camera.Parameters params = mCamera.getParameters();
            frameRate = params.getPreviewFrameRate();
            mCamera.unlock();
            mMediaRecorder.setCamera(mCamera);
            mMediaRecorder.setPreviewDisplay(mActivity.getSurfaceHolder().getSurface());
        }

        mMediaRecorder.setVideoSource(useSurface ?
                MediaRecorder.VideoSource.SURFACE : MediaRecorder.VideoSource.CAMERA);

        if (hasAudio) {
            mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        }

        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
        mMediaRecorder.setOutputFile(filename);

        mMediaRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);
        mMediaRecorder.setVideoFrameRate(frameRate);
        mMediaRecorder.setVideoSize(VIDEO_WIDTH, VIDEO_HEIGHT);

        if (hasAudio) {
            mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);
        }
    }

    private Surface tryGetSurface(boolean shouldThrow) throws Exception {
        Surface surface = null;
        try {
            surface = mMediaRecorder.getSurface();
            assertFalse("failed to throw IllegalStateException", shouldThrow);
        } catch (IllegalStateException e) {
            assertTrue("threw unexpected exception: " + e, shouldThrow);
        }
        return surface;
    }

    private boolean validateGetSurface(boolean useSurface) {
        Log.v(TAG,"validateGetSurface, useSurface=" + useSurface);
        if (!useSurface && !hasCamera()) {
            // pass if testing camera source but no hardware
            return true;
        }
        Surface surface = null;
        boolean success = true;
        try {
            setupRecorder(OUTPUT_PATH, useSurface, false /* hasAudio */);

            /* Test: getSurface() before prepare()
             * should throw IllegalStateException
             */
            surface = tryGetSurface(true /* shouldThow */);

            mMediaRecorder.prepare();

            /* Test: getSurface() after prepare()
             * should succeed for surface source
             * should fail for camera source
             */
            surface = tryGetSurface(!useSurface);

            mMediaRecorder.start();

            /* Test: getSurface() after start()
             * should succeed for surface source
             * should fail for camera source
             */
            surface = tryGetSurface(!useSurface);

            try {
                mMediaRecorder.stop();
            } catch (Exception e) {
                // stop() could fail if the recording is empty, as we didn't render anything.
                // ignore any failure in stop, we just want it stopped.
            }

            /* Test: getSurface() after stop()
             * should throw IllegalStateException
             */
            surface = tryGetSurface(true /* shouldThow */);
        } catch (Exception e) {
            Log.d(TAG, e.toString());
            success = false;
        } finally {
            // reset to clear states, as stop() might have failed
            mMediaRecorder.reset();

            if (mCamera != null) {
                mCamera.release();
                mCamera = null;
            }
            if (surface != null) {
                surface.release();
                surface = null;
            }
        }

        return success;
    }

    private void trySetInputSurface(Surface surface) throws Exception {
        boolean testBadArgument = (surface == null);
        try {
            mMediaRecorder.setInputSurface(testBadArgument ? new Surface() : surface);
            fail("failed to throw exception");
        } catch (IllegalArgumentException e) {
            // OK only if testing bad arg
            assertTrue("threw unexpected exception: " + e, testBadArgument);
        } catch (IllegalStateException e) {
            // OK only if testing error case other than bad arg
            assertFalse("threw unexpected exception: " + e, testBadArgument);
        }
    }

    private boolean validatePersistentSurface(boolean errorCase) {
        Log.v(TAG, "validatePersistentSurface, errorCase=" + errorCase);

        Surface surface = MediaCodec.createPersistentInputSurface();
        if (surface == null) {
            return false;
        }
        Surface dummy = null;

        boolean success = true;
        try {
            setupRecorder(OUTPUT_PATH, true /* useSurface */, false /* hasAudio */);

            if (errorCase) {
                /*
                 * Test: should throw if called with non-persistent surface
                 */
                trySetInputSurface(null);
            } else {
                /*
                 * Test: should succeed if called with a persistent surface before prepare()
                 */
                mMediaRecorder.setInputSurface(surface);
            }

            /*
             * Test: getSurface() should fail before prepare
             */
            dummy = tryGetSurface(true /* shouldThow */);

            mMediaRecorder.prepare();

            /*
             * Test: setInputSurface() should fail after prepare
             */
            trySetInputSurface(surface);

            /*
             * Test: getSurface() should fail if setInputSurface() succeeded
             */
            dummy = tryGetSurface(!errorCase /* shouldThow */);

            mMediaRecorder.start();

            /*
             * Test: setInputSurface() should fail after start
             */
            trySetInputSurface(surface);

            /*
             * Test: getSurface() should fail if setInputSurface() succeeded
             */
            dummy = tryGetSurface(!errorCase /* shouldThow */);

            try {
                mMediaRecorder.stop();
            } catch (Exception e) {
                // stop() could fail if the recording is empty, as we didn't render anything.
                // ignore any failure in stop, we just want it stopped.
            }

            /*
             * Test: getSurface() should fail after stop
             */
            dummy = tryGetSurface(true /* shouldThow */);
        } catch (Exception e) {
            Log.d(TAG, e.toString());
            success = false;
        } finally {
            // reset to clear states, as stop() might have failed
            mMediaRecorder.reset();

            if (mCamera != null) {
                mCamera.release();
                mCamera = null;
            }
            if (surface != null) {
                surface.release();
                surface = null;
            }
            if (dummy != null) {
                dummy.release();
                dummy = null;
            }
        }

        return success;
    }

    public void testGetSurfaceApi() {
        if (!hasH264()) {
            MediaUtils.skipTest("no codecs");
            return;
        }

        if (hasCamera()) {
            // validate getSurface() with CAMERA source
            assertTrue(validateGetSurface(false /* useSurface */));
        }

        // validate getSurface() with SURFACE source
        assertTrue(validateGetSurface(true /* useSurface */));
    }

    public void testPersistentSurfaceApi() {
        if (!hasH264()) {
            MediaUtils.skipTest("no codecs");
            return;
        }

        // test valid use case
        assertTrue(validatePersistentSurface(false /* errorCase */));

        // test invalid use case
        assertTrue(validatePersistentSurface(true /* errorCase */));
    }

    private static int getMaxFrameRateForCodec(int codec) {
        for (VideoEncoderCap cap : mVideoEncoders) {
            if (cap.mCodec == codec) {
                return cap.mMaxFrameRate < NORMAL_FPS ? cap.mMaxFrameRate : NORMAL_FPS;
            }
        }
        fail("didn't find max FPS for codec");
        return -1;
    }

    private boolean recordFromSurface(
            String filename,
            int captureRate,
            boolean hasAudio,
            Surface persistentSurface) {
        Log.v(TAG, "recordFromSurface");
        Surface surface = null;
        try {
            setupRecorder(filename, true /* useSurface */, hasAudio);

            int sleepTimeMs;
            if (captureRate > 0) {
                mMediaRecorder.setCaptureRate(captureRate);
                sleepTimeMs = 1000 / captureRate;
            } else {
                sleepTimeMs = 1000 / getMaxFrameRateForCodec(MediaRecorder.VideoEncoder.H264);
            }

            if (persistentSurface != null) {
                Log.v(TAG, "using persistent surface");
                surface = persistentSurface;
                mMediaRecorder.setInputSurface(surface);
            }

            mMediaRecorder.prepare();

            if (persistentSurface == null) {
                surface = mMediaRecorder.getSurface();
            }

            Paint paint = new Paint();
            paint.setTextSize(16);
            paint.setColor(Color.RED);
            int i;

            /* Test: draw 10 frames at 30fps before start
             * these should be dropped and not causing malformed stream.
             */
            for(i = 0; i < 10; i++) {
                Canvas canvas = surface.lockCanvas(null);
                int background = (i * 255 / 99);
                canvas.drawARGB(255, background, background, background);
                String text = "Frame #" + i;
                canvas.drawText(text, 50, 50, paint);
                surface.unlockCanvasAndPost(canvas);
                Thread.sleep(sleepTimeMs);
            }

            Log.v(TAG, "start");
            mMediaRecorder.start();

            /* Test: draw another 90 frames at 30fps after start */
            for(i = 10; i < 100; i++) {
                Canvas canvas = surface.lockCanvas(null);
                int background = (i * 255 / 99);
                canvas.drawARGB(255, background, background, background);
                String text = "Frame #" + i;
                canvas.drawText(text, 50, 50, paint);
                surface.unlockCanvasAndPost(canvas);
                Thread.sleep(sleepTimeMs);
            }

            Log.v(TAG, "stop");
            mMediaRecorder.stop();
        } catch (Exception e) {
            Log.v(TAG, "record video failed: " + e.toString());
            return false;
        } finally {
            // We need to test persistent surface across multiple MediaRecorder
            // instances, so must destroy mMediaRecorder here.
            if (mMediaRecorder != null) {
                mMediaRecorder.release();
                mMediaRecorder = null;
            }

            // release surface if not using persistent surface
            if (persistentSurface == null && surface != null) {
                surface.release();
                surface = null;
            }
        }
        return true;
    }

    private boolean checkCaptureFps(String filename, int captureRate) {
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();

        retriever.setDataSource(filename);

        // verify capture rate meta key is present and correct
        String captureFps = retriever.extractMetadata(
                MediaMetadataRetriever.METADATA_KEY_CAPTURE_FRAMERATE);

        if (captureFps == null) {
            Log.d(TAG, "METADATA_KEY_CAPTURE_FRAMERATE is missing");
            return false;
        }

        if (Math.abs(Float.parseFloat(captureFps) - captureRate) > 0.001) {
            Log.d(TAG, "METADATA_KEY_CAPTURE_FRAMERATE is incorrect: "
                    + captureFps + "vs. " + captureRate);
            return false;
        }

        // verify other meta keys here if necessary
        return true;
    }

    private boolean testRecordFromSurface(boolean persistent, boolean timelapse) {
        Log.v(TAG, "testRecordFromSurface: " +
                   "persistent=" + persistent + ", timelapse=" + timelapse);
        boolean success = false;
        Surface surface = null;
        int noOfFailure = 0;
        final float frameRate = getMaxFrameRateForCodec(MediaRecorder.VideoEncoder.H264);

        if (!hasH264()) {
            MediaUtils.skipTest("no codecs");
            return true;
        }

        try {
            if (persistent) {
                surface = MediaCodec.createPersistentInputSurface();
            }

            for (int k = 0; k < 2; k++) {
                String filename = (k == 0) ? OUTPUT_PATH : OUTPUT_PATH2;
                boolean hasAudio = false;
                int captureRate = 0;

                if (timelapse) {
                    // if timelapse/slow-mo, k chooses between low/high capture fps
                    captureRate = (k == 0) ? TIME_LAPSE_FPS : SLOW_MOTION_FPS;
                } else {
                    // otherwise k chooses between no-audio and audio
                    hasAudio = (k == 0) ? false : true;
                }

                if (hasAudio && (!hasMicrophone() || !hasAmrNb())) {
                    // audio test waived if no audio support
                    continue;
                }

                Log.v(TAG, "testRecordFromSurface - round " + k);
                success = recordFromSurface(filename, captureRate, hasAudio, surface);
                if (success) {
                    checkTracksAndDuration(0, true /* hasVideo */, hasAudio, filename, frameRate);

                    // verify capture fps meta key
                    if (timelapse && !checkCaptureFps(filename, captureRate)) {
                        noOfFailure++;
                    }
                }
                if (!success) {
                    noOfFailure++;
                }
            }
        } catch (Exception e) {
            Log.v(TAG, e.toString());
            noOfFailure++;
        } finally {
            if (surface != null) {
                Log.v(TAG, "releasing persistent surface");
                surface.release();
                surface = null;
            }
        }
        return (noOfFailure == 0);
    }

    // Test recording from surface source with/without audio)
    public void testSurfaceRecording() {
        assertTrue(testRecordFromSurface(false /* persistent */, false /* timelapse */));
    }

    // Test recording from persistent surface source with/without audio
    public void testPersistentSurfaceRecording() {
        assertTrue(testRecordFromSurface(true /* persistent */, false /* timelapse */));
    }

    // Test timelapse recording from surface without audio
    public void testSurfaceRecordingTimeLapse() {
        assertTrue(testRecordFromSurface(false /* persistent */, true /* timelapse */));
    }

    // Test timelapse recording from persisent surface without audio
    public void testPersistentSurfaceRecordingTimeLapse() {
        assertTrue(testRecordFromSurface(true /* persistent */, true /* timelapse */));
    }

    private void recordMedia(long maxFileSize, File outFile) throws Exception {
        mMediaRecorder.setMaxFileSize(maxFileSize);
        mMediaRecorder.prepare();
        mMediaRecorder.start();
        Thread.sleep(RECORD_TIME_MS);
        mMediaRecorder.stop();

        assertTrue(outFile.exists());

        // The max file size is always guaranteed.
        // We just make sure that the margin is not too big
        assertTrue(outFile.length() < 1.1 * maxFileSize);
        assertTrue(outFile.length() > 0);
    }

    private boolean hasCamera() {
        return mActivity.getPackageManager().hasSystemFeature(PackageManager.FEATURE_CAMERA);
    }

    private boolean hasMicrophone() {
        return mActivity.getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_MICROPHONE);
    }

    private static boolean hasAmrNb() {
        return MediaUtils.hasEncoder(MediaFormat.MIMETYPE_AUDIO_AMR_NB);
    }

    private static boolean hasAmrWb() {
        return MediaUtils.hasEncoder(MediaFormat.MIMETYPE_AUDIO_AMR_WB);
    }

    private static boolean hasAac() {
        return MediaUtils.hasEncoder(MediaFormat.MIMETYPE_AUDIO_AAC);
    }

    private static boolean hasH264() {
        return MediaUtils.hasEncoder(MediaFormat.MIMETYPE_VIDEO_AVC);
    }
}
