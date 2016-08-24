/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.widget.cts;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.content.res.ColorStateList;
import android.graphics.Color;

import android.graphics.drawable.ColorDrawable;
import android.test.suitebuilder.annotation.SmallTest;
import android.widget.cts.util.TestUtils;
import android.widget.cts.util.ViewTestUtils;
import org.junit.Assert;
import org.xmlpull.v1.XmlPullParser;

import android.app.Activity;
import android.content.Context;
import android.cts.util.WidgetTestUtils;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Matrix;
import android.graphics.PorterDuff;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.Icon;
import android.graphics.drawable.PaintDrawable;
import android.net.Uri;
import android.test.ActivityInstrumentationTestCase;
import android.test.UiThreadTest;
import android.util.AttributeSet;
import android.util.StateSet;
import android.util.Xml;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;

import android.widget.cts.R;

import static org.mockito.Matchers.any;
import static org.mockito.Mockito.*;


/**
 * Test {@link ImageView}.
 */
@SmallTest
public class ImageViewTest extends ActivityInstrumentationTestCase<ImageViewCtsActivity> {
    private ImageView mImageView;
    private Activity mActivity;

    public ImageViewTest() {
        super("android.widget.cts", ImageViewCtsActivity.class);
    }

    /**
     * Find the ImageView specified by id.
     *
     * @param id the id
     * @return the ImageView
     */
    private ImageView findImageViewById(int id) {
        return (ImageView) mActivity.findViewById(id);
    }

    private void createSampleImage(File imagefile, int resid) {
        InputStream source = null;
        OutputStream target = null;

        try {
            source = mActivity.getResources().openRawResource(resid);
            target = new FileOutputStream(imagefile);

            byte[] buffer = new byte[1024];
            for (int len = source.read(buffer); len > 0; len = source.read(buffer)) {
                target.write(buffer, 0, len);
            }
        } catch (IOException e) {
            fail(e.getMessage());
        } finally {
            try {
                if (source != null) {
                    source.close();
                }
                if (target != null) {
                    target.close();
                }
            } catch (IOException ignored) {
                // Ignore the IOException.
            }
        }
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mImageView = null;
        mActivity = getActivity();
    }

    public void testConstructor() {
        new ImageView(mActivity);

        new ImageView(mActivity, null);

        new ImageView(mActivity, null, 0);

        new ImageView(mActivity, null, 0, 0);

        XmlPullParser parser = mActivity.getResources().getXml(R.layout.imageview_layout);
        AttributeSet attrs = Xml.asAttributeSet(parser);
        new ImageView(mActivity, attrs);
        new ImageView(mActivity, attrs, 0);

        try {
            new ImageView(null, null);
            fail("should throw NullPointerException.");
        } catch (NullPointerException e) {
        }

        try {
            new ImageView(null, null, 0);
            fail("should throw NullPointerException.");
        } catch (NullPointerException e) {
        }
    }

    public void testInvalidateDrawable() {
        ImageView imageView = new ImageView(mActivity);
        imageView.invalidateDrawable(null);
    }

    public void testSetAdjustViewBounds() {
        ImageView imageView = new ImageView(mActivity);
        imageView.setScaleType(ScaleType.FIT_XY);

        imageView.setAdjustViewBounds(false);
        assertFalse(imageView.getAdjustViewBounds());
        assertEquals(ScaleType.FIT_XY, imageView.getScaleType());

        imageView.setAdjustViewBounds(true);
        assertTrue(imageView.getAdjustViewBounds());
        assertEquals(ScaleType.FIT_CENTER, imageView.getScaleType());
    }

    public void testSetMaxWidth() {
        ImageView imageView = new ImageView(mActivity);
        imageView.setMaxWidth(120);
        imageView.setMaxWidth(-1);
    }

    public void testSetMaxHeight() {
        ImageView imageView = new ImageView(mActivity);
        imageView.setMaxHeight(120);
        imageView.setMaxHeight(-1);
    }

    public void testGetDrawable() {
        final ImageView imageView = new ImageView(mActivity);
        final PaintDrawable drawable1 = new PaintDrawable();
        final PaintDrawable drawable2 = new PaintDrawable();

        assertNull(imageView.getDrawable());

        imageView.setImageDrawable(drawable1);
        assertEquals(drawable1, imageView.getDrawable());
        assertNotSame(drawable2, imageView.getDrawable());
    }

    @UiThreadTest
    public void testSetImageIcon() {
        mImageView = findImageViewById(R.id.imageview);
        mImageView.setImageIcon(null);
        assertNull(mImageView.getDrawable());

        Icon icon = Icon.createWithResource(mActivity, R.drawable.testimage);
        mImageView.setImageIcon(icon);
        assertTrue(mImageView.isLayoutRequested());
        assertNotNull(mImageView.getDrawable());
        Drawable drawable = mActivity.getDrawable(R.drawable.testimage);
        BitmapDrawable testimageBitmap = (BitmapDrawable) drawable;
        Drawable imageViewDrawable = mImageView.getDrawable();
        BitmapDrawable imageViewBitmap = (BitmapDrawable) imageViewDrawable;
        WidgetTestUtils.assertEquals(testimageBitmap.getBitmap(), imageViewBitmap.getBitmap());
    }

    @UiThreadTest
    public void testSetImageResource() {
        mImageView = findImageViewById(R.id.imageview);
        mImageView.setImageResource(-1);
        assertNull(mImageView.getDrawable());

        mImageView.setImageResource(R.drawable.testimage);
        assertTrue(mImageView.isLayoutRequested());
        assertNotNull(mImageView.getDrawable());
        Drawable drawable = mActivity.getDrawable(R.drawable.testimage);
        BitmapDrawable testimageBitmap = (BitmapDrawable) drawable;
        Drawable imageViewDrawable = mImageView.getDrawable();
        BitmapDrawable imageViewBitmap = (BitmapDrawable) imageViewDrawable;
        WidgetTestUtils.assertEquals(testimageBitmap.getBitmap(), imageViewBitmap.getBitmap());
    }

    @UiThreadTest
    public void testSetImageURI() {
        mImageView = findImageViewById(R.id.imageview);
        mImageView.setImageURI(null);
        assertNull(mImageView.getDrawable());

        File dbDir = getInstrumentation().getTargetContext().getDir("tests",
                Context.MODE_PRIVATE);
        File imagefile = new File(dbDir, "tempimage.jpg");
        if (imagefile.exists()) {
            imagefile.delete();
        }
        createSampleImage(imagefile, R.raw.testimage);
        final String path = imagefile.getPath();
        mImageView.setImageURI(Uri.parse(path));
        assertTrue(mImageView.isLayoutRequested());
        assertNotNull(mImageView.getDrawable());

        Drawable imageViewDrawable = mImageView.getDrawable();
        BitmapDrawable imageViewBitmap = (BitmapDrawable) imageViewDrawable;
        Bitmap.Config viewConfig = imageViewBitmap.getBitmap().getConfig();
        Bitmap testimageBitmap = WidgetTestUtils.getUnscaledAndDitheredBitmap(
                mActivity.getResources(), R.raw.testimage, viewConfig);

        WidgetTestUtils.assertEquals(testimageBitmap, imageViewBitmap.getBitmap());
    }

    @UiThreadTest
    public void testSetImageDrawable() {
        mImageView = findImageViewById(R.id.imageview);

        mImageView.setImageDrawable(null);
        assertNull(mImageView.getDrawable());

        final Drawable drawable = mActivity.getDrawable(R.drawable.testimage);
        mImageView.setImageDrawable(drawable);
        assertTrue(mImageView.isLayoutRequested());
        assertNotNull(mImageView.getDrawable());
        BitmapDrawable testimageBitmap = (BitmapDrawable) drawable;
        Drawable imageViewDrawable = mImageView.getDrawable();
        BitmapDrawable imageViewBitmap = (BitmapDrawable) imageViewDrawable;
        WidgetTestUtils.assertEquals(testimageBitmap.getBitmap(), imageViewBitmap.getBitmap());
    }

    @UiThreadTest
    public void testSetImageBitmap() {
        mImageView = findImageViewById(R.id.imageview);

        mImageView.setImageBitmap(null);
        // A BitmapDrawable is always created for the ImageView.
        assertNotNull(mImageView.getDrawable());

        final Bitmap bitmap =
            BitmapFactory.decodeResource(mActivity.getResources(), R.drawable.testimage);
        mImageView.setImageBitmap(bitmap);
        assertTrue(mImageView.isLayoutRequested());
        assertNotNull(mImageView.getDrawable());
        Drawable imageViewDrawable = mImageView.getDrawable();
        BitmapDrawable imageViewBitmap = (BitmapDrawable) imageViewDrawable;
        WidgetTestUtils.assertEquals(bitmap, imageViewBitmap.getBitmap());
    }

    public void testSetImageState() {
        mImageView = new ImageView(mActivity);
        int[] state = new int[8];
        mImageView.setImageState(state, false);
        assertSame(state, mImageView.onCreateDrawableState(0));
    }

    public void testSetSelected() {
        mImageView = new ImageView(mActivity);
        assertFalse(mImageView.isSelected());

        mImageView.setSelected(true);
        assertTrue(mImageView.isSelected());

        mImageView.setSelected(false);
        assertFalse(mImageView.isSelected());
    }

    public void testSetImageLevel() {
        PaintDrawable drawable = new PaintDrawable();
        drawable.setLevel(0);

        ImageView imageView = new ImageView(mActivity);
        imageView.setImageDrawable(drawable);
        imageView.setImageLevel(1);
        assertEquals(1, drawable.getLevel());
    }

    public void testAccessScaleType() {
        final ImageView imageView = new ImageView(mActivity);

        try {
            imageView.setScaleType(null);
            fail("should throw NullPointerException.");
        } catch (NullPointerException e) {
        }
        assertNotNull(imageView.getScaleType());

        imageView.setScaleType(ImageView.ScaleType.CENTER);
        assertEquals(ImageView.ScaleType.CENTER, imageView.getScaleType());

        imageView.setScaleType(ImageView.ScaleType.MATRIX);
        assertEquals(ImageView.ScaleType.MATRIX, imageView.getScaleType());

        imageView.setScaleType(ImageView.ScaleType.FIT_START);
        assertEquals(ImageView.ScaleType.FIT_START, imageView.getScaleType());

        imageView.setScaleType(ImageView.ScaleType.FIT_END);
        assertEquals(ImageView.ScaleType.FIT_END, imageView.getScaleType());

        imageView.setScaleType(ImageView.ScaleType.CENTER_CROP);
        assertEquals(ImageView.ScaleType.CENTER_CROP, imageView.getScaleType());

        imageView.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
        assertEquals(ImageView.ScaleType.CENTER_INSIDE, imageView.getScaleType());
    }

    public void testAccessImageMatrix() {
        final ImageView imageView = new ImageView(mActivity);

        imageView.setImageMatrix(null);
        assertNotNull(imageView.getImageMatrix());

        final Matrix matrix = new Matrix();
        imageView.setImageMatrix(matrix);
        assertEquals(matrix, imageView.getImageMatrix());
    }

    @UiThreadTest
    public void testAccessBaseline() {
        mImageView = findImageViewById(R.id.imageview);

        mImageView.setImageDrawable(null);
        assertNull(mImageView.getDrawable());

        final Drawable drawable = mActivity.getDrawable(R.drawable.testimage);
        mImageView.setImageDrawable(drawable);

        assertEquals(-1, mImageView.getBaseline());

        mImageView.setBaseline(50);
        assertEquals(50, mImageView.getBaseline());

        mImageView.setBaselineAlignBottom(true);
        assertTrue(mImageView.getBaselineAlignBottom());
        assertEquals(mImageView.getMeasuredHeight(), mImageView.getBaseline());

        mImageView.setBaselineAlignBottom(false);
        assertFalse(mImageView.getBaselineAlignBottom());
        assertEquals(50, mImageView.getBaseline());
    }

    @UiThreadTest
    public void testSetColorFilter1() {
        mImageView = findImageViewById(R.id.imageview);

        final Drawable drawable = mActivity.getDrawable(R.drawable.testimage);
        mImageView.setImageDrawable(drawable);

        mImageView.setColorFilter(null);
        assertNull(drawable.getColorFilter());

        mImageView.setColorFilter(0, PorterDuff.Mode.CLEAR);
        assertNotNull(drawable.getColorFilter());
        assertNotNull(mImageView.getColorFilter());
    }

    @UiThreadTest
    public void testClearColorFilter() {
        mImageView = findImageViewById(R.id.imageview);

        final Drawable drawable = mActivity.getDrawable(R.drawable.testimage);
        mImageView.setImageDrawable(drawable);

        ColorFilter cf = new ColorFilter();
        mImageView.setColorFilter(cf);

        mImageView.clearColorFilter();
        assertNull(drawable.getColorFilter());
        assertNull(mImageView.getColorFilter());
    }

    @UiThreadTest
    public void testSetColorFilter2() {
        mImageView = findImageViewById(R.id.imageview);

        final Drawable drawable = mActivity.getDrawable(R.drawable.testimage);
        mImageView.setImageDrawable(drawable);

        mImageView.setColorFilter(null);
        assertNull(drawable.getColorFilter());
        assertNull(mImageView.getColorFilter());

        ColorFilter cf = new ColorFilter();
        mImageView.setColorFilter(cf);
        assertSame(cf, drawable.getColorFilter());
        assertSame(cf, mImageView.getColorFilter());
    }

    public void testDrawableStateChanged() {
        MockImageView imageView = spy(new MockImageView(mActivity));
        Drawable selectorDrawable = mActivity.getDrawable(R.drawable.statelistdrawable);
        imageView.setImageDrawable(selectorDrawable);

        // We shouldn't have been called on state change yet
        verify(imageView, never()).drawableStateChanged();
        // Mark image view as selected. Since our selector drawable has an "entry" for selected
        // state, that should cause a call to drawableStateChanged()
        imageView.setSelected(true);
        // Test that our image view has indeed called its own drawableStateChanged()
        verify(imageView, times(1)).drawableStateChanged();
        // And verify that image view's state matches that of our drawable
        Assert.assertArrayEquals(imageView.getDrawableState(), selectorDrawable.getState());
    }

    public void testOnCreateDrawableState() {
        MockImageView mockImageView = new MockImageView(mActivity);

        assertEquals(MockImageView.getEnabledStateSet(), mockImageView.onCreateDrawableState(0));

        int[] expected = new int[]{1, 2, 3};
        mockImageView.setImageState(expected, false);
        assertSame(expected, mockImageView.onCreateDrawableState(1));

        mockImageView.setImageState(expected, true);
        try {
            mockImageView.onCreateDrawableState(-1);
            fail("should throw IndexOutOfBoundsException");
        } catch (IndexOutOfBoundsException e) {
        }
    }

    public void testOnDraw() {
        MockImageView mockImageView = new MockImageView(mActivity);
        Drawable drawable = spy(mActivity.getDrawable(R.drawable.icon_red));
        mockImageView.setImageDrawable(drawable);
        mockImageView.onDraw(new Canvas());

        verify(drawable, atLeastOnce()).draw(any(Canvas.class));
    }

    public void testOnMeasure() {
        mImageView = findImageViewById(R.id.imageview);
        mImageView.measure(200, 150);
        assertTrue(mImageView.getMeasuredWidth() <= 200);
        assertTrue(mImageView.getMeasuredHeight() <= 150);
    }

    public void testSetFrame() {
        MockImageView mockImageView = spy(new MockImageView(mActivity));
        verify(mockImageView, never()).onSizeChanged(anyInt(), anyInt(), anyInt(), anyInt());

        assertTrue(mockImageView.setFrame(5, 10, 100, 200));
        assertEquals(5, mockImageView.getLeft());
        assertEquals(10, mockImageView.getTop());
        assertEquals(100, mockImageView.getRight());
        assertEquals(200, mockImageView.getBottom());
        verify(mockImageView, times(1)).onSizeChanged(95, 190, 0, 0);

        assertFalse(mockImageView.setFrame(5, 10, 100, 200));
        // Verify that there were no more calls to onSizeChanged (since the new frame is the
        // same frame as we had before).
        verify(mockImageView, times(1)).onSizeChanged(anyInt(), anyInt(), anyInt(), anyInt());
    }

    public void testVerifyDrawable() {
        MockImageView mockImageView = new MockImageView(mActivity);
        Drawable drawable = new ColorDrawable(0xFFFF0000);
        mockImageView.setImageDrawable(drawable);
        Drawable backgroundDrawable = new ColorDrawable(0xFF0000FF);
        mockImageView.setBackgroundDrawable(backgroundDrawable);

        assertFalse(mockImageView.verifyDrawable(null));
        assertFalse(mockImageView.verifyDrawable(new ColorDrawable(0xFF00FF00)));
        assertTrue(mockImageView.verifyDrawable(drawable));
        assertTrue(mockImageView.verifyDrawable(backgroundDrawable));
    }

    @UiThreadTest
    public void testImageTintBasics() {
        mImageView = findImageViewById(R.id.image_tint);

        assertEquals("Image tint inflated correctly",
                Color.WHITE, mImageView.getImageTintList().getDefaultColor());
        assertEquals("Image tint mode inflated correctly",
                PorterDuff.Mode.SRC_OVER, mImageView.getImageTintMode());

        mImageView.setImageTintMode(PorterDuff.Mode.SRC_IN);
        assertEquals(PorterDuff.Mode.SRC_IN, mImageView.getImageTintMode());
    }

    public void testImageTintDrawableUpdates() {
        Drawable drawable = spy(mActivity.getDrawable(R.drawable.icon_red));

        ImageView view = new ImageView(mActivity);
        view.setImageDrawable(drawable);
        // No image tint applied by default
        verify(drawable, never()).setTintList(any(ColorStateList.class));

        view.setImageTintList(ColorStateList.valueOf(Color.WHITE));
        // Image tint applied when setImageTintList() called after setImageDrawable()
        verify(drawable, times(1)).setTintList(any(ColorStateList.class));

        view.setImageDrawable(null);
        view.setImageDrawable(drawable);
        // Image tint applied when setImageTintList() called before setImageDrawable()
        verify(drawable, times(2)).setTintList(any(ColorStateList.class));
    }

    @UiThreadTest
    public void testImageTintVisuals() {
        mImageView = findImageViewById(R.id.image_tint_with_source);
        TestUtils.assertAllPixelsOfColor("All pixels should be white", mImageView,
                0xFFFFFFFF, 1, false);

        // Use translucent white tint. Together with SRC_OVER mode (defined in XML) the end
        // result should be a fully opaque image view with solid fill color in between red
        // and white.
        mImageView.setImageTintList(ColorStateList.valueOf(0x80FFFFFF));
        TestUtils.assertAllPixelsOfColor("All pixels should be light red", mImageView,
                0xFFFF8080, 1, false);

        // Switch to SRC_IN mode. This should completely ignore the original drawable set on
        // the image view and use the last set tint color (50% alpha white).
        mImageView.setImageTintMode(PorterDuff.Mode.SRC_IN);
        TestUtils.assertAllPixelsOfColor("All pixels should be 50% alpha white", mImageView,
                0x80FFFFFF, 1, false);

        // Switch to DST mode. This should completely ignore the last set tint color and use the
        // the original drawable set on the image view.
        mImageView.setImageTintMode(PorterDuff.Mode.DST);
        TestUtils.assertAllPixelsOfColor("All pixels should be red", mImageView,
                0xFFFF0000, 1, false);
    }

    @UiThreadTest
    public void testAlpha() {
        mImageView = findImageViewById(R.id.imageview);
        mImageView.setImageResource(R.drawable.blue_fill);

        TestUtils.assertAllPixelsOfColor("All pixels should be blue", mImageView,
                0xFF0000FF, 1, false);

        mImageView.setAlpha(128);
        TestUtils.assertAllPixelsOfColor("All pixels should be 50% alpha blue", mImageView,
                0x800000FF, 1, false);

        mImageView.setAlpha(0);
        TestUtils.assertAllPixelsOfColor("All pixels should be transparent", mImageView,
                0x00000000, 1, false);

        mImageView.setAlpha(255);
        TestUtils.assertAllPixelsOfColor("All pixels should be blue", mImageView,
                0xFF0000FF, 1, false);
    }

    @UiThreadTest
    public void testImageAlpha() {
        mImageView = findImageViewById(R.id.imageview);
        mImageView.setImageResource(R.drawable.blue_fill);

        assertEquals(255, mImageView.getImageAlpha());
        TestUtils.assertAllPixelsOfColor("All pixels should be blue", mImageView,
                0xFF0000FF, 1, false);

        mImageView.setImageAlpha(128);
        assertEquals(128, mImageView.getImageAlpha());
        TestUtils.assertAllPixelsOfColor("All pixels should be 50% alpha blue", mImageView,
                0x800000FF, 1, false);

        mImageView.setImageAlpha(0);
        assertEquals(0, mImageView.getImageAlpha());
        TestUtils.assertAllPixelsOfColor("All pixels should be transparent", mImageView,
                0x00000000, 1, false);

        mImageView.setImageAlpha(255);
        assertEquals(255, mImageView.getImageAlpha());
        TestUtils.assertAllPixelsOfColor("All pixels should be blue", mImageView,
                0xFF0000FF, 1, false);
    }

    protected static class MockImageView extends ImageView {
        public MockImageView(Context context) {
            super(context);
        }

        public MockImageView(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        public MockImageView(Context context, AttributeSet attrs, int defStyle) {
            super(context, attrs, defStyle);
        }

        public static int[] getEnabledStateSet() {
            return ENABLED_STATE_SET;
        }

        public static int[] getPressedEnabledStateSet() {
            return PRESSED_ENABLED_STATE_SET;
        }
        @Override
        protected void drawableStateChanged() {
            super.drawableStateChanged();
        }
        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
        }
        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        }
        @Override
        protected boolean onSetAlpha(int alpha) {
            return super.onSetAlpha(alpha);
        }
        @Override
        protected boolean setFrame(int l, int t, int r, int b) {
            return super.setFrame(l, t, r, b);
        }
        @Override
        protected boolean verifyDrawable(Drawable dr) {
            return super.verifyDrawable(dr);
        }

        protected void onSizeChanged(int w, int h, int oldw, int oldh) {
            super.onSizeChanged(w, h, oldw, oldh);
        }
    }
}
