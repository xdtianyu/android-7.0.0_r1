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
package android.transition.cts;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.graphics.Matrix;
import android.graphics.drawable.Drawable;
import android.transition.ChangeImageTransform;
import android.transition.TransitionManager;
import android.transition.TransitionValues;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;

public class ChangeImageTransformTest extends BaseTransitionTest {
    ChangeImageTransform mChangeImageTransform;
    Matrix mStartMatrix;
    Matrix mEndMatrix;
    Drawable mImage;
    ImageView mImageView;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        resetTransition();
        mStartMatrix = null;
        mEndMatrix = null;
        mImage = null;
        mImageView = null;
    }

    private void resetTransition() {
        mChangeImageTransform = new CaptureMatrix();
        mChangeImageTransform.setDuration(100);
        mTransition = mChangeImageTransform;
        resetListener();
    }

    public void testCenterToFitXY() throws Throwable {
        transformImage(ScaleType.CENTER, ScaleType.FIT_XY);
        assertMatrixMatches(centerMatrix(), mStartMatrix);
        assertMatrixMatches(fitXYMatrix(), mEndMatrix);
    }

    public void testCenterCropToFitCenter() throws Throwable {
        transformImage(ScaleType.CENTER_CROP, ScaleType.FIT_CENTER);
        assertMatrixMatches(centerCropMatrix(), mStartMatrix);
        assertMatrixMatches(fitCenterMatrix(), mEndMatrix);
    }

    public void testCenterInsideToFitEnd() throws Throwable {
        transformImage(ScaleType.CENTER_INSIDE, ScaleType.FIT_END);
        // CENTER_INSIDE and CENTER are the same when the image is smaller than the View
        assertMatrixMatches(centerMatrix(), mStartMatrix);
        assertMatrixMatches(fitEndMatrix(), mEndMatrix);
    }

    public void testFitStartToCenter() throws Throwable {
        transformImage(ScaleType.FIT_START, ScaleType.CENTER);
        assertMatrixMatches(fitStartMatrix(), mStartMatrix);
        assertMatrixMatches(centerMatrix(), mEndMatrix);
    }

    private Matrix centerMatrix() {
        int imageWidth = mImage.getIntrinsicWidth();
        int imageViewWidth = mImageView.getWidth();
        float tx = Math.round((imageViewWidth - imageWidth)/2f);

        int imageHeight = mImage.getIntrinsicHeight();
        int imageViewHeight = mImageView.getHeight();
        float ty = Math.round((imageViewHeight - imageHeight)/2f);

        Matrix matrix = new Matrix();
        matrix.postTranslate(tx, ty);
        return matrix;
    }

    private Matrix fitXYMatrix() {
        int imageWidth = mImage.getIntrinsicWidth();
        int imageViewWidth = mImageView.getWidth();
        float scaleX = ((float)imageViewWidth)/imageWidth;

        int imageHeight = mImage.getIntrinsicHeight();
        int imageViewHeight = mImageView.getHeight();
        float scaleY = ((float)imageViewHeight)/imageHeight;

        Matrix matrix = new Matrix();
        matrix.postScale(scaleX, scaleY);
        return matrix;
    }

    private Matrix centerCropMatrix() {
        int imageWidth = mImage.getIntrinsicWidth();
        int imageViewWidth = mImageView.getWidth();
        float scaleX = ((float)imageViewWidth)/imageWidth;

        int imageHeight = mImage.getIntrinsicHeight();
        int imageViewHeight = mImageView.getHeight();
        float scaleY = ((float)imageViewHeight)/imageHeight;

        float maxScale = Math.max(scaleX, scaleY);

        float width = imageWidth * maxScale;
        float height = imageHeight * maxScale;
        float tx = Math.round((imageViewWidth - width) / 2f);
        float ty = Math.round((imageViewHeight - height) / 2f);

        Matrix matrix = new Matrix();
        matrix.postScale(maxScale, maxScale);
        matrix.postTranslate(tx, ty);
        return matrix;
    }

    private Matrix fitCenterMatrix() {
        int imageWidth = mImage.getIntrinsicWidth();
        int imageViewWidth = mImageView.getWidth();
        float scaleX = ((float)imageViewWidth)/imageWidth;

        int imageHeight = mImage.getIntrinsicHeight();
        int imageViewHeight = mImageView.getHeight();
        float scaleY = ((float)imageViewHeight)/imageHeight;

        float minScale = Math.min(scaleX, scaleY);

        float width = imageWidth * minScale;
        float height = imageHeight * minScale;
        float tx = (imageViewWidth - width) / 2f;
        float ty = (imageViewHeight - height) / 2f;

        Matrix matrix = new Matrix();
        matrix.postScale(minScale, minScale);
        matrix.postTranslate(tx, ty);
        return matrix;
    }

    private Matrix fitStartMatrix() {
        int imageWidth = mImage.getIntrinsicWidth();
        int imageViewWidth = mImageView.getWidth();
        float scaleX = ((float)imageViewWidth)/imageWidth;

        int imageHeight = mImage.getIntrinsicHeight();
        int imageViewHeight = mImageView.getHeight();
        float scaleY = ((float)imageViewHeight)/imageHeight;

        float minScale = Math.min(scaleX, scaleY);

        Matrix matrix = new Matrix();
        matrix.postScale(minScale, minScale);
        return matrix;
    }

    private Matrix fitEndMatrix() {
        int imageWidth = mImage.getIntrinsicWidth();
        int imageViewWidth = mImageView.getWidth();
        float scaleX = ((float)imageViewWidth)/imageWidth;

        int imageHeight = mImage.getIntrinsicHeight();
        int imageViewHeight = mImageView.getHeight();
        float scaleY = ((float)imageViewHeight)/imageHeight;

        float minScale = Math.min(scaleX, scaleY);

        float width = imageWidth * minScale;
        float height = imageHeight * minScale;
        float tx = imageViewWidth - width;
        float ty = imageViewHeight - height;

        Matrix matrix = new Matrix();
        matrix.postScale(minScale, minScale);
        matrix.postTranslate(tx, ty);
        return matrix;
    }

    private void assertMatrixMatches(Matrix expected, Matrix matrix) {
        if (expected == null) {
            assertNull(matrix);
            return;
        }
        assertNotNull(matrix);
        float[] expectedValues = new float[9];
        expected.getValues(expectedValues);

        float[] values = new float[9];
        matrix.getValues(values);

        for (int i = 0; i < values.length; i++) {
            final float expectedValue = expectedValues[i];
            final float value = values[i];
            assertEquals("Value [" + i + "]", expectedValue, value, 0.01f);
        }
    }

    private void transformImage(ScaleType startScale, final ScaleType endScale) throws Throwable {
        final ImageView imageView = enterImageViewScene(startScale);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                TransitionManager.beginDelayedTransition(mSceneRoot, mChangeImageTransform);
                imageView.setScaleType(endScale);
            }
        });
        waitForStart();
        int expectedEndCount = (startScale == endScale) ? 0 : 1;
        assertEquals(expectedEndCount, mListener.endLatch.getCount());
        waitForEnd(200);
    }

    private ImageView enterImageViewScene(final ScaleType scaleType) throws Throwable {
        enterScene(R.layout.scene4);
        final ViewGroup container = (ViewGroup) mActivity.findViewById(R.id.holder);
        final ImageView[] imageViews = new ImageView[1];
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mImageView = new ImageView(mActivity);
                mImage = mActivity.getDrawable(android.R.drawable.ic_media_play);
                mImageView.setImageDrawable(mImage);
                mImageView.setScaleType(scaleType);
                imageViews[0] = mImageView;
                container.addView(mImageView);
                LayoutParams layoutParams = mImageView.getLayoutParams();
                DisplayMetrics metrics = mActivity.getResources().getDisplayMetrics();
                float size = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 50, metrics);
                layoutParams.width = Math.round(size);
                layoutParams.height = Math.round(size * 2);
                mImageView.setLayoutParams(layoutParams);
            }
        });
        getInstrumentation().waitForIdleSync();
        return imageViews[0];
    }

    private class CaptureMatrix extends ChangeImageTransform {
        @Override
        public Animator createAnimator(ViewGroup sceneRoot, TransitionValues startValues,
                TransitionValues endValues) {
            Animator animator = super.createAnimator(sceneRoot, startValues, endValues);
            animator.addListener(new CaptureMatrixListener((ImageView) endValues.view));
            return animator;
        }
    }

    private class CaptureMatrixListener extends AnimatorListenerAdapter {
        private final ImageView mImageView;

        public CaptureMatrixListener(ImageView view) {
            mImageView = view;
        }

        @Override
        public void onAnimationStart(Animator animation) {
            mStartMatrix = copyMatrix();
        }

        @Override
        public void onAnimationEnd(Animator animation) {
            mEndMatrix = copyMatrix();
        }

        private Matrix copyMatrix() {
            Matrix matrix = mImageView.getImageMatrix();
            if (matrix != null) {
                matrix = new Matrix(matrix);
            }
            return matrix;
        }
    }
}

