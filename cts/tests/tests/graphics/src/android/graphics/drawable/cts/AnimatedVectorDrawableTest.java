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

package android.graphics.drawable.cts;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.graphics.cts.R;
import android.graphics.drawable.Animatable2;
import android.graphics.drawable.AnimatedVectorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.Drawable.ConstantState;
import android.test.ActivityInstrumentationTestCase2;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Xml;
import android.widget.ImageView;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import static java.lang.Thread.sleep;

public class AnimatedVectorDrawableTest extends ActivityInstrumentationTestCase2<DrawableStubActivity> {
    private static final String LOGTAG = AnimatedVectorDrawableTest.class.getSimpleName();

    private static final int IMAGE_WIDTH = 64;
    private static final int IMAGE_HEIGHT = 64;

    private DrawableStubActivity mActivity;
    private Resources mResources;
    private Bitmap mBitmap;
    private Canvas mCanvas;
    private static final boolean DBG_DUMP_PNG = false;
    private final int mResId = R.drawable.animation_vector_drawable_grouping_1;
    private final int mLayoutId = R.layout.animated_vector_drawable_source;
    private final int mImageViewId = R.id.avd_view;


    public AnimatedVectorDrawableTest() {
        super(DrawableStubActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mBitmap = Bitmap.createBitmap(IMAGE_WIDTH, IMAGE_HEIGHT, Bitmap.Config.ARGB_8888);
        mCanvas = new Canvas(mBitmap);

        mActivity = getActivity();
        mResources = mActivity.getResources();
    }

    // This is only for debugging or golden image (re)generation purpose.
    private void saveVectorDrawableIntoPNG(Bitmap bitmap, int resId) throws IOException {
        // Save the image to the disk.
        FileOutputStream out = null;
        try {
            String outputFolder = "/sdcard/temp/";
            File folder = new File(outputFolder);
            if (!folder.exists()) {
                folder.mkdir();
            }
            String originalFilePath = mResources.getString(resId);
            File originalFile = new File(originalFilePath);
            String fileFullName = originalFile.getName();
            String fileTitle = fileFullName.substring(0, fileFullName.lastIndexOf("."));
            String outputFilename = outputFolder + fileTitle + "_golden.png";
            File outputFile = new File(outputFilename);
            if (!outputFile.exists()) {
                outputFile.createNewFile();
            }

            out = new FileOutputStream(outputFile, false);
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, out);
            Log.v(LOGTAG, "Write test No." + outputFilename + " to file successfully.");
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (out != null) {
                out.close();
            }
        }
    }

    @MediumTest
    public void testInflate() throws Exception {
        // Setup AnimatedVectorDrawable from xml file
        XmlPullParser parser = mResources.getXml(mResId);
        AttributeSet attrs = Xml.asAttributeSet(parser);

        int type;
        while ((type=parser.next()) != XmlPullParser.START_TAG &&
                type != XmlPullParser.END_DOCUMENT) {
            // Empty loop
        }

        if (type != XmlPullParser.START_TAG) {
            throw new XmlPullParserException("No start tag found");
        }
        AnimatedVectorDrawable drawable = new AnimatedVectorDrawable();
        drawable.inflate(mResources, parser, attrs);
        drawable.setBounds(0, 0, IMAGE_WIDTH, IMAGE_HEIGHT);
        mBitmap.eraseColor(0);
        drawable.draw(mCanvas);
        int sunColor = mBitmap.getPixel(IMAGE_WIDTH / 2, IMAGE_HEIGHT / 2);
        int earthColor = mBitmap.getPixel(IMAGE_WIDTH * 3 / 4 + 2, IMAGE_HEIGHT / 2);
        assertTrue(sunColor == 0xFFFF8000);
        assertTrue(earthColor == 0xFF5656EA);

        if (DBG_DUMP_PNG) {
            saveVectorDrawableIntoPNG(mBitmap, mResId);
        }
    }

    @MediumTest
    public void testSingleFrameAnimation() throws Exception {
        int resId = R.drawable.avd_single_frame;
        final AnimatedVectorDrawable d1 =
                (AnimatedVectorDrawable) mResources.getDrawable(resId);
        // The AVD has a duration as 16ms.
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                d1.start();
                d1.stop();

            }
        });
        getInstrumentation().waitForIdleSync();

        d1.setBounds(0, 0, IMAGE_WIDTH, IMAGE_HEIGHT);
        mBitmap.eraseColor(0);
        d1.draw(mCanvas);

        int endColor = mBitmap.getPixel(IMAGE_WIDTH / 2, IMAGE_HEIGHT / 2);

        assertEquals("Center point's color must be green", 0xFF00FF00, endColor);

        if (DBG_DUMP_PNG) {
            saveVectorDrawableIntoPNG(mBitmap, resId);
        }
    }

    @SmallTest
    public void testGetChangingConfigurations() {
        AnimatedVectorDrawable avd = new AnimatedVectorDrawable();
        ConstantState constantState = avd.getConstantState();

        // default
        assertEquals(0, constantState.getChangingConfigurations());
        assertEquals(0, avd.getChangingConfigurations());

        // change the drawable's configuration does not affect the state's configuration
        avd.setChangingConfigurations(0xff);
        assertEquals(0xff, avd.getChangingConfigurations());
        assertEquals(0, constantState.getChangingConfigurations());

        // the state's configuration get refreshed
        constantState = avd.getConstantState();
        assertEquals(0xff,  constantState.getChangingConfigurations());

        // set a new configuration to drawable
        avd.setChangingConfigurations(0xff00);
        assertEquals(0xff,  constantState.getChangingConfigurations());
        assertEquals(0xffff,  avd.getChangingConfigurations());
    }

    @SmallTest
    public void testGetConstantState() {
        AnimatedVectorDrawable AnimatedVectorDrawable = new AnimatedVectorDrawable();
        ConstantState constantState = AnimatedVectorDrawable.getConstantState();
        assertNotNull(constantState);
        assertEquals(0, constantState.getChangingConfigurations());

        AnimatedVectorDrawable.setChangingConfigurations(1);
        constantState = AnimatedVectorDrawable.getConstantState();
        assertNotNull(constantState);
        assertEquals(1, constantState.getChangingConfigurations());
    }

    @SmallTest
    public void testMutate() {
        AnimatedVectorDrawable d1 = (AnimatedVectorDrawable) mResources.getDrawable(mResId);
        AnimatedVectorDrawable d2 = (AnimatedVectorDrawable) mResources.getDrawable(mResId);
        AnimatedVectorDrawable d3 = (AnimatedVectorDrawable) mResources.getDrawable(mResId);
        int originalAlpha = d2.getAlpha();
        int newAlpha = (originalAlpha + 1) % 255;

        // AVD is different than VectorDrawable. Every instance of it is a deep copy
        // of the VectorDrawable.
        // So every setAlpha operation will happen only to that specific object.
        d1.setAlpha(newAlpha);
        assertEquals(newAlpha, d1.getAlpha());
        assertEquals(originalAlpha, d2.getAlpha());
        assertEquals(originalAlpha, d3.getAlpha());

        d1.mutate();
        d1.setAlpha(0x40);
        assertEquals(0x40, d1.getAlpha());
        assertEquals(originalAlpha, d2.getAlpha());
        assertEquals(originalAlpha, d3.getAlpha());

        d2.setAlpha(0x20);
        assertEquals(0x40, d1.getAlpha());
        assertEquals(0x20, d2.getAlpha());
        assertEquals(originalAlpha, d3.getAlpha());
    }

    @SmallTest
    public void testGetOpacity() {
        AnimatedVectorDrawable d1 = (AnimatedVectorDrawable) mResources.getDrawable(mResId);
        assertEquals("Default is translucent", PixelFormat.TRANSLUCENT, d1.getOpacity());
        d1.setAlpha(0);
        assertEquals("Still translucent", PixelFormat.TRANSLUCENT, d1.getOpacity());
    }

    @SmallTest
    public void testColorFilter() {
        PorterDuffColorFilter filter = new PorterDuffColorFilter(Color.RED, PorterDuff.Mode.SRC_IN);
        AnimatedVectorDrawable d1 = (AnimatedVectorDrawable) mResources.getDrawable(mResId);
        d1.setColorFilter(filter);

        assertEquals(filter, d1.getColorFilter());
    }

    @MediumTest
    public void testReset() {
        final AnimatedVectorDrawable d1 = (AnimatedVectorDrawable) mResources.getDrawable(mResId);
        // The AVD has a duration as 100ms.
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                d1.start();
                d1.reset();
            }
        });
        getInstrumentation().waitForIdleSync();
        assertFalse(d1.isRunning());

    }

    @MediumTest
    public void testStop() {
        final AnimatedVectorDrawable d1 = (AnimatedVectorDrawable) mResources.getDrawable(mResId);
        // The AVD has a duration as 100ms.
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                d1.start();
                d1.stop();

            }
        });
        getInstrumentation().waitForIdleSync();
        assertFalse(d1.isRunning());
    }

    @MediumTest
    public void testAddCallbackBeforeStart() throws InterruptedException {
        final MyCallback callback = new MyCallback();
        // The AVD has a duration as 100ms.
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.setContentView(mLayoutId);
                ImageView imageView = (ImageView) mActivity.findViewById(mImageViewId);
                AnimatedVectorDrawable d1 = (AnimatedVectorDrawable) imageView.getDrawable();
                d1.registerAnimationCallback(callback);
                d1.start();
            }
        });
        getInstrumentation().waitForIdleSync();
        sleep(200);
        assertTrue(callback.mStart);
        assertTrue(callback.mEnd);
    }

    @MediumTest
    public void testAddCallbackAfterTrigger() throws InterruptedException {
        final MyCallback callback = new MyCallback();
        // The AVD has a duration as 100ms.
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.setContentView(mLayoutId);
                ImageView imageView = (ImageView) mActivity.findViewById(mImageViewId);
                AnimatedVectorDrawable d1 = (AnimatedVectorDrawable) imageView.getDrawable();
                // This reset call can enforce the AnimatorSet is setup properly in AVD, when
                // running on UI thread.
                d1.reset();
                d1.registerAnimationCallback(callback);
                d1.start();
            }
        });
        getInstrumentation().waitForIdleSync();
        sleep(200);
        assertTrue(callback.mStart);
        assertTrue(callback.mEnd);
    }

    @MediumTest
    public void testAddCallbackAfterStart() throws InterruptedException {
        final MyCallback callback = new MyCallback();
        // The AVD has a duration as 100ms.
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.setContentView(mLayoutId);
                ImageView imageView = (ImageView) mActivity.findViewById(mImageViewId);
                AnimatedVectorDrawable d1 = (AnimatedVectorDrawable) imageView.getDrawable();
                d1.start();
                d1.registerAnimationCallback(callback);
            }
        });
        getInstrumentation().waitForIdleSync();
        sleep(200);
        // Whether or not the callback.start is true could vary when running on Render Thread.
        // Therefore, we don't make assertion here. The most useful flag is the callback.mEnd.
        assertTrue(callback.mEnd);
    }

    @MediumTest
    public void testRemoveCallback() {
        final MyCallback callback = new MyCallback();
        // The AVD has a duration as 100ms.
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.setContentView(mLayoutId);
                ImageView imageView = (ImageView) mActivity.findViewById(mImageViewId);
                AnimatedVectorDrawable d1 = (AnimatedVectorDrawable) imageView.getDrawable();
                d1.registerAnimationCallback(callback);
                assertTrue(d1.unregisterAnimationCallback(callback));
                d1.start();
            }
        });
        getInstrumentation().waitForIdleSync();

        assertFalse(callback.mStart);
        assertFalse(callback.mEnd);
    }

    @MediumTest
    public void testClearCallback() {
        final MyCallback callback = new MyCallback();

        // The AVD has a duration as 100ms.
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.setContentView(mLayoutId);
                ImageView imageView = (ImageView) mActivity.findViewById(mImageViewId);
                AnimatedVectorDrawable d1 = (AnimatedVectorDrawable) imageView.getDrawable();
                d1.registerAnimationCallback(callback);
                d1.clearAnimationCallbacks();
                d1.start();
            }
        });

        getInstrumentation().waitForIdleSync();

        assertFalse(callback.mStart);
        assertFalse(callback.mEnd);
    }

    class MyCallback extends Animatable2.AnimationCallback {
        boolean mStart = false;
        boolean mEnd = false;

        @Override
        public void onAnimationStart(Drawable drawable) {
            mStart = true;
        }

        @Override
        public void onAnimationEnd(Drawable drawable) {
            mEnd = true;
        }
    }
}
