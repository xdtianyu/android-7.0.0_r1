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

package android.hardware.camera2.cts;

import static android.hardware.camera2.cts.CameraTestUtils.*;

import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.cts.CameraTestUtils.ImageVerifierListener;
import android.hardware.camera2.cts.testcases.Camera2MultiViewTestCase;
import android.hardware.camera2.cts.testcases.Camera2MultiViewTestCase.CameraPreviewListener;
import android.media.ImageReader;
import android.os.SystemClock;
import android.util.Log;
import android.util.Size;
import android.view.Surface;
import android.view.TextureView;

import com.android.ex.camera2.blocking.BlockingCameraManager.BlockingOpenException;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * CameraDevice test by using combination of SurfaceView, TextureView and ImageReader
 */
public class MultiViewTest extends Camera2MultiViewTestCase {
    private static final String TAG = "MultiViewTest";
    private final static long WAIT_FOR_COMMAND_TO_COMPLETE = 2000;
    private final static long PREVIEW_TIME_MS = 2000;

    public void testTextureViewPreview() throws Exception {
        for (String cameraId : mCameraIds) {
            Exception prior = null;

            try {
                openCamera(cameraId);
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId + " does not support color outputs, skipping");
                    continue;
                }
                List<TextureView> views = Arrays.asList(mTextureView[0]);
                textureViewPreview(cameraId, views, /*ImageReader*/null);
            } catch (Exception e) {
                prior = e;
            } finally {
                try {
                    closeCamera(cameraId);
                } catch (Exception e) {
                    if (prior != null) {
                        Log.e(TAG, "Prior exception received: " + prior);
                    }
                    prior = e;
                }
                if (prior != null) throw prior; // Rethrow last exception.
            }
        }
    }

    public void testTextureViewPreviewWithImageReader() throws Exception {
        for (String cameraId : mCameraIds) {
            Exception prior = null;

            ImageVerifierListener yuvListener;
            ImageReader yuvReader = null;

            try {
                openCamera(cameraId);
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId + " does not support color outputs, skipping");
                    continue;
                }
                Size previewSize = getOrderedPreviewSizes(cameraId).get(0);
                yuvListener =
                        new ImageVerifierListener(previewSize, ImageFormat.YUV_420_888);
                yuvReader = makeImageReader(previewSize,
                        ImageFormat.YUV_420_888, MAX_READER_IMAGES, yuvListener, mHandler);
                int maxNumStreamsProc =
                        getStaticInfo(cameraId).getMaxNumOutputStreamsProcessedChecked();
                if (maxNumStreamsProc < 2) {
                    continue;
                }
                List<TextureView> views = Arrays.asList(mTextureView[0]);
                textureViewPreview(cameraId, views, yuvReader);
            } catch (Exception e) {
                prior = e;
            } finally {
                try {
                    // Close camera device first. This will give some more time for
                    // ImageVerifierListener to finish the validation before yuvReader is closed
                    // (all image will be closed after that)
                    closeCamera(cameraId);
                    if (yuvReader != null) {
                        yuvReader.close();
                    }
                } catch (Exception e) {
                    if (prior != null) {
                        Log.e(TAG, "Prior exception received: " + prior);
                    }
                    prior = e;
                }
                if (prior != null) throw prior; // Rethrow last exception.
            }
        }
    }

    public void testDualTextureViewPreview() throws Exception {
        for (String cameraId : mCameraIds) {
            Exception prior = null;
            try {
                openCamera(cameraId);
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId + " does not support color outputs, skipping");
                    continue;
                }
                int maxNumStreamsProc =
                        getStaticInfo(cameraId).getMaxNumOutputStreamsProcessedChecked();
                if (maxNumStreamsProc < 2) {
                    continue;
                }
                List<TextureView> views = Arrays.asList(mTextureView[0], mTextureView[1]);
                textureViewPreview(cameraId, views, /*ImageReader*/null);
            } catch (Exception e) {
                prior = e;
            } finally {
                try {
                    closeCamera(cameraId);
                } catch (Exception e) {
                    if (prior != null) {
                        Log.e(TAG, "Prior exception received: " + prior);
                    }
                    prior = e;
                }
                if (prior != null) throw prior; // Rethrow last exception.
            }
        }
    }

    public void testDualTextureViewAndImageReaderPreview() throws Exception {
        for (String cameraId : mCameraIds) {
            Exception prior = null;

            ImageVerifierListener yuvListener;
            ImageReader yuvReader = null;

            try {
                openCamera(cameraId);
                if (!getStaticInfo(cameraId).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + cameraId + " does not support color outputs, skipping");
                    continue;
                }
                Size previewSize = getOrderedPreviewSizes(cameraId).get(0);
                yuvListener =
                        new ImageVerifierListener(previewSize, ImageFormat.YUV_420_888);
                yuvReader = makeImageReader(previewSize,
                        ImageFormat.YUV_420_888, MAX_READER_IMAGES, yuvListener, mHandler);
                int maxNumStreamsProc =
                        getStaticInfo(cameraId).getMaxNumOutputStreamsProcessedChecked();
                if (maxNumStreamsProc < 3) {
                    continue;
                }
                List<TextureView> views = Arrays.asList(mTextureView[0], mTextureView[1]);
                textureViewPreview(cameraId, views, yuvReader);
            } catch (Exception e) {
                prior = e;
            } finally {
                try {
                    if (yuvReader != null) {
                        yuvReader.close();
                    }
                    closeCamera(cameraId);
                } catch (Exception e) {
                    if (prior != null) {
                        Log.e(TAG, "Prior exception received: " + prior);
                    }
                    prior = e;
                }
                if (prior != null) throw prior; // Rethrow last exception.
            }
        }
    }

    public void testDualCameraPreview() throws Exception {
        final int NUM_CAMERAS_TESTED = 2;
        if (mCameraIds.length < NUM_CAMERAS_TESTED) {
            return;
        }

        try {
            for (int i = 0; i < NUM_CAMERAS_TESTED; i++) {
                openCamera(mCameraIds[i]);
                if (!getStaticInfo(mCameraIds[i]).isColorOutputSupported()) {
                    Log.i(TAG, "Camera " + mCameraIds[i] +
                            " does not support color outputs, skipping");
                    continue;
                }
                List<TextureView> views = Arrays.asList(mTextureView[i]);

                startTextureViewPreview(mCameraIds[i], views, /*ImageReader*/null);
            }
            // TODO: check the framerate is correct
            SystemClock.sleep(PREVIEW_TIME_MS);
            for (int i = 0; i < NUM_CAMERAS_TESTED; i++) {
                stopPreview(mCameraIds[i]);
            }
        } catch (BlockingOpenException e) {
            // The only error accepted is ERROR_MAX_CAMERAS_IN_USE, which means HAL doesn't support
            // concurrent camera streaming
            assertEquals("Camera device open failed",
                    CameraDevice.StateCallback.ERROR_MAX_CAMERAS_IN_USE, e.getCode());
            Log.i(TAG, "Camera HAL does not support dual camera preview. Skip the test");
        } finally {
            for (int i = 0; i < NUM_CAMERAS_TESTED; i++) {
                closeCamera(mCameraIds[i]);
            }
        }
    }

    /**
     * Start camera preview using input texture views and/or one image reader
     */
    private void startTextureViewPreview(
            String cameraId, List<TextureView> views, ImageReader imageReader)
            throws Exception {
        int numPreview = views.size();
        Size previewSize = getOrderedPreviewSizes(cameraId).get(0);
        CameraPreviewListener[] previewListener =
                new CameraPreviewListener[numPreview];
        SurfaceTexture[] previewTexture = new SurfaceTexture[numPreview];
        List<Surface> surfaces = new ArrayList<Surface>();

        // Prepare preview surface.
        int i = 0;
        for (TextureView view : views) {
            previewListener[i] = new CameraPreviewListener();
            view.setSurfaceTextureListener(previewListener[i]);
            previewTexture[i] = getAvailableSurfaceTexture(WAIT_FOR_COMMAND_TO_COMPLETE, view);
            assertNotNull("Unable to get preview surface texture", previewTexture[i]);
            previewTexture[i].setDefaultBufferSize(previewSize.getWidth(), previewSize.getHeight());
            // Correct the preview display rotation.
            updatePreviewDisplayRotation(previewSize, view);
            surfaces.add(new Surface(previewTexture[i]));
            i++;
        }
        if (imageReader != null) {
            surfaces.add(imageReader.getSurface());
        }

        startPreview(cameraId, surfaces, null);

        i = 0;
        for (TextureView view : views) {
            boolean previewDone =
                    previewListener[i].waitForPreviewDone(WAIT_FOR_COMMAND_TO_COMPLETE);
            assertTrue("Unable to start preview " + i, previewDone);
            view.setSurfaceTextureListener(null);
            i++;
        }
    }

    /**
     * Test camera preview using input texture views and/or one image reader
     */
    private void textureViewPreview(
            String cameraId, List<TextureView> views, ImageReader testImagerReader)
            throws Exception {
        startTextureViewPreview(cameraId, views, testImagerReader);

        // TODO: check the framerate is correct
        SystemClock.sleep(PREVIEW_TIME_MS);

        stopPreview(cameraId);
    }
}
