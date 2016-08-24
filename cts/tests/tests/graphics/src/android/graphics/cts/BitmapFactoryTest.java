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

package android.graphics.cts;

import android.graphics.cts.R;


import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory.Options;
import android.os.Parcel;
import android.os.ParcelFileDescriptor;
import android.test.InstrumentationTestCase;
import android.util.DisplayMetrics;
import android.util.TypedValue;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;

public class BitmapFactoryTest extends InstrumentationTestCase {
    private Resources mRes;
    // opt for non-null
    private BitmapFactory.Options mOpt1;
    // opt for null
    private BitmapFactory.Options mOpt2;
    // height and width of start.jpg
    private static final int START_HEIGHT = 31;
    private static final int START_WIDTH = 31;
    private int mDefaultDensity;
    private int mTargetDensity;

    // The test images, including baseline JPEG, a PNG, a GIF, a BMP AND a WEBP.
    private static int[] RES_IDS = new int[] {
            R.drawable.baseline_jpeg, R.drawable.png_test, R.drawable.gif_test,
            R.drawable.bmp_test, R.drawable.webp_test
    };

    // The width and height of the above image.
    private static int WIDTHS[] = new int[] { 1280, 640, 320, 320, 640 };
    private static int HEIGHTS[] = new int[] { 960, 480, 240, 240, 480 };

    // Configurations for BitmapFactory.Options
    private static Config[] COLOR_CONFIGS = new Config[] {Config.ARGB_8888, Config.RGB_565};
    private static int[] COLOR_TOLS = new int[] {16, 49, 576};

    private static Config[] COLOR_CONFIGS_RGBA = new Config[] {Config.ARGB_8888};
    private static int[] COLOR_TOLS_RGBA = new int[] {72, 124};

    private static int[] RAW_COLORS = new int[] {
        // raw data from R.drawable.premul_data
        Color.argb(255, 0, 0, 0),
        Color.argb(128, 255, 0, 0),
        Color.argb(128, 25, 26, 27),
        Color.argb(2, 255, 254, 253),
    };

    private static int[] DEPREMUL_COLORS = new int[] {
        // data from R.drawable.premul_data, after premultiplied store + un-premultiplied load
        Color.argb(255, 0, 0, 0),
        Color.argb(128, 255, 0, 0),
        Color.argb(128, 26, 26, 28),
        Color.argb(2, 255, 255, 255),
    };

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mRes = getInstrumentation().getTargetContext().getResources();
        mDefaultDensity = DisplayMetrics.DENSITY_DEFAULT;
        mTargetDensity = mRes.getDisplayMetrics().densityDpi;

        mOpt1 = new BitmapFactory.Options();
        mOpt1.inScaled = false;
        mOpt2 = new BitmapFactory.Options();
        mOpt2.inScaled = false;
        mOpt2.inJustDecodeBounds = true;
    }

    public void testConstructor() {
        // new the BitmapFactory instance
        new BitmapFactory();
    }

    public void testDecodeResource1() {
        Bitmap b = BitmapFactory.decodeResource(mRes, R.drawable.start,
                mOpt1);
        assertNotNull(b);
        // Test the bitmap size
        assertEquals(START_HEIGHT, b.getHeight());
        assertEquals(START_WIDTH, b.getWidth());
        // Test if no bitmap
        assertNull(BitmapFactory.decodeResource(mRes, R.drawable.start, mOpt2));
    }

    public void testDecodeResource2() {
        Bitmap b = BitmapFactory.decodeResource(mRes, R.drawable.start);
        assertNotNull(b);
        // Test the bitmap size
        assertEquals(START_HEIGHT * mTargetDensity / mDefaultDensity, b.getHeight(), 1.1);
        assertEquals(START_WIDTH * mTargetDensity / mDefaultDensity, b.getWidth(), 1.1);
    }

    public void testDecodeResourceStream() {
        InputStream is = obtainInputStream();
        Rect r = new Rect(1, 1, 1, 1);
        TypedValue value = new TypedValue();
        Bitmap b = BitmapFactory.decodeResourceStream(mRes, value, is, r, mOpt1);
        assertNotNull(b);
        // Test the bitmap size
        assertEquals(START_HEIGHT, b.getHeight());
        assertEquals(START_WIDTH, b.getWidth());
    }

    public void testDecodeByteArray1() {
        byte[] array = obtainArray();
        Bitmap b = BitmapFactory.decodeByteArray(array, 0, array.length, mOpt1);
        assertNotNull(b);
        // Test the bitmap size
        assertEquals(START_HEIGHT, b.getHeight());
        assertEquals(START_WIDTH, b.getWidth());
        // Test if no bitmap
        assertNull(BitmapFactory.decodeByteArray(array, 0, array.length, mOpt2));
    }

    public void testDecodeByteArray2() {
        byte[] array = obtainArray();
        Bitmap b = BitmapFactory.decodeByteArray(array, 0, array.length);
        assertNotNull(b);
        // Test the bitmap size
        assertEquals(START_HEIGHT, b.getHeight());
        assertEquals(START_WIDTH, b.getWidth());
    }

    public void testDecodeStream1() {
        InputStream is = obtainInputStream();
        Rect r = new Rect(1, 1, 1, 1);
        Bitmap b = BitmapFactory.decodeStream(is, r, mOpt1);
        assertNotNull(b);
        // Test the bitmap size
        assertEquals(START_HEIGHT, b.getHeight());
        assertEquals(START_WIDTH, b.getWidth());
        // Test if no bitmap
        assertNull(BitmapFactory.decodeStream(is, r, mOpt2));
    }

    public void testDecodeStream2() {
        InputStream is = obtainInputStream();
        Bitmap b = BitmapFactory.decodeStream(is);
        assertNotNull(b);
        // Test the bitmap size
        assertEquals(START_HEIGHT, b.getHeight());
        assertEquals(START_WIDTH, b.getWidth());
    }

    public void testDecodeStream3() throws IOException {
        for (int i = 0; i < RES_IDS.length; ++i) {
            InputStream is = obtainInputStream(RES_IDS[i]);
            Bitmap b = BitmapFactory.decodeStream(is);
            assertNotNull(b);
            // Test the bitmap size
            assertEquals(WIDTHS[i], b.getWidth());
            assertEquals(HEIGHTS[i], b.getHeight());
        }
    }

    public void testDecodeStream4() throws IOException {
        BitmapFactory.Options options = new BitmapFactory.Options();
        for (int k = 0; k < COLOR_CONFIGS.length; ++k) {
            options.inPreferredConfig = COLOR_CONFIGS[k];

            // Decode the PNG & WebP test images. The WebP test image has been encoded from PNG test
            // image and should have same similar (within some error-tolerance) Bitmap data.
            InputStream iStreamPng = obtainInputStream(R.drawable.png_test);
            Bitmap bPng = BitmapFactory.decodeStream(iStreamPng, null, options);
            assertNotNull(bPng);
            assertEquals(bPng.getConfig(), COLOR_CONFIGS[k]);
            assertFalse(bPng.isPremultiplied());
            assertFalse(bPng.hasAlpha());

            InputStream iStreamWebp1 = obtainInputStream(R.drawable.webp_test);
            Bitmap bWebp1 = BitmapFactory.decodeStream(iStreamWebp1, null, options);
            assertNotNull(bWebp1);
            assertFalse(bWebp1.isPremultiplied());
            assertFalse(bWebp1.hasAlpha());
            compareBitmaps(bPng, bWebp1, COLOR_TOLS[k], true, bPng.isPremultiplied());

            // Compress the PNG image to WebP format (Quality=90) and decode it back.
            // This will test end-to-end WebP encoding and decoding.
            ByteArrayOutputStream oStreamWebp = new ByteArrayOutputStream();
            assertTrue(bPng.compress(CompressFormat.WEBP, 90, oStreamWebp));
            InputStream iStreamWebp2 = new ByteArrayInputStream(oStreamWebp.toByteArray());
            Bitmap bWebp2 = BitmapFactory.decodeStream(iStreamWebp2, null, options);
            assertNotNull(bWebp2);
            assertFalse(bWebp2.isPremultiplied());
            assertFalse(bWebp2.hasAlpha());
            compareBitmaps(bPng, bWebp2, COLOR_TOLS[k], true, bPng.isPremultiplied());
        }
    }

    public void testDecodeStream5() throws IOException {
        BitmapFactory.Options options = new BitmapFactory.Options();
        for (int k = 0; k < COLOR_CONFIGS_RGBA.length; ++k) {
            options.inPreferredConfig = COLOR_CONFIGS_RGBA[k];

            // Decode the PNG & WebP (google_logo) images. The WebP image has
            // been encoded from PNG image.
            InputStream iStreamPng = obtainInputStream(R.drawable.google_logo_1);
            Bitmap bPng = BitmapFactory.decodeStream(iStreamPng, null, options);
            assertNotNull(bPng);
            assertEquals(bPng.getConfig(), COLOR_CONFIGS_RGBA[k]);
            assertTrue(bPng.isPremultiplied());
            assertTrue(bPng.hasAlpha());

            // Decode the corresponding WebP (transparent) image (google_logo_2.webp).
            InputStream iStreamWebP1 = obtainInputStream(R.drawable.google_logo_2);
            Bitmap bWebP1 = BitmapFactory.decodeStream(iStreamWebP1, null, options);
            assertNotNull(bWebP1);
            assertEquals(bWebP1.getConfig(), COLOR_CONFIGS_RGBA[k]);
            assertTrue(bWebP1.isPremultiplied());
            assertTrue(bWebP1.hasAlpha());
            compareBitmaps(bPng, bWebP1, COLOR_TOLS_RGBA[k], true, bPng.isPremultiplied());

            // Compress the PNG image to WebP format (Quality=90) and decode it back.
            // This will test end-to-end WebP encoding and decoding.
            ByteArrayOutputStream oStreamWebp = new ByteArrayOutputStream();
            assertTrue(bPng.compress(CompressFormat.WEBP, 90, oStreamWebp));
            InputStream iStreamWebp2 = new ByteArrayInputStream(oStreamWebp.toByteArray());
            Bitmap bWebP2 = BitmapFactory.decodeStream(iStreamWebp2, null, options);
            assertNotNull(bWebP2);
            assertEquals(bWebP2.getConfig(), COLOR_CONFIGS_RGBA[k]);
            assertTrue(bWebP2.isPremultiplied());
            assertTrue(bWebP2.hasAlpha());
            compareBitmaps(bPng, bWebP2, COLOR_TOLS_RGBA[k], true, bPng.isPremultiplied());
        }
    }

    public void testDecodeFileDescriptor1() throws IOException {
        ParcelFileDescriptor pfd = obtainParcelDescriptor(obtainPath());
        FileDescriptor input = pfd.getFileDescriptor();
        Rect r = new Rect(1, 1, 1, 1);
        Bitmap b = BitmapFactory.decodeFileDescriptor(input, r, mOpt1);
        assertNotNull(b);
        // Test the bitmap size
        assertEquals(START_HEIGHT, b.getHeight());
        assertEquals(START_WIDTH, b.getWidth());
        // Test if no bitmap
        assertNull(BitmapFactory.decodeFileDescriptor(input, r, mOpt2));
    }

    public void testDecodeFileDescriptor2() throws IOException {
        ParcelFileDescriptor pfd = obtainParcelDescriptor(obtainPath());
        FileDescriptor input = pfd.getFileDescriptor();
        Bitmap b = BitmapFactory.decodeFileDescriptor(input);
        assertNotNull(b);
        // Test the bitmap size
        assertEquals(START_HEIGHT, b.getHeight());
        assertEquals(START_WIDTH, b.getWidth());
    }

    public void testDecodeFileDescriptor3() throws IOException {
        // Arbitrary offsets to use. If the offset of the FD matches the offset of the image,
        // decoding should succeed, but if they do not match, decoding should fail.
        long ACTUAL_OFFSETS[] = new long[] { 0, 17 };
        for (int RES_ID : RES_IDS) {
            for (int j = 0; j < ACTUAL_OFFSETS.length; ++j) {
                // FIXME: The purgeable test should attempt to purge the memory
                // to force a re-decode.
                for (boolean TEST_PURGEABLE : new boolean[] { false, true }) {
                    BitmapFactory.Options opts = new BitmapFactory.Options();
                    opts.inPurgeable = TEST_PURGEABLE;
                    opts.inInputShareable = TEST_PURGEABLE;

                    long actualOffset = ACTUAL_OFFSETS[j];
                    String path = obtainPath(RES_ID, actualOffset);
                    RandomAccessFile file = new RandomAccessFile(path, "r");
                    FileDescriptor fd = file.getFD();
                    assertTrue(fd.valid());

                    // Set the offset to ACTUAL_OFFSET
                    file.seek(actualOffset);
                    assertEquals(file.getFilePointer(), actualOffset);

                    // Now decode. This should be successful and leave the offset
                    // unchanged.
                    Bitmap b = BitmapFactory.decodeFileDescriptor(fd, null, opts);
                    assertNotNull(b);
                    assertEquals(file.getFilePointer(), actualOffset);

                    // Now use the other offset. It should fail to decode, and
                    // the offset should remain unchanged.
                    long otherOffset = ACTUAL_OFFSETS[(j + 1) % ACTUAL_OFFSETS.length];
                    assertFalse(otherOffset == actualOffset);
                    file.seek(otherOffset);
                    assertEquals(file.getFilePointer(), otherOffset);

                    b = BitmapFactory.decodeFileDescriptor(fd, null, opts);
                    assertNull(b);
                    assertEquals(file.getFilePointer(), otherOffset);
                }
            }
        }
    }

    public void testDecodeFile1() throws IOException {
        Bitmap b = BitmapFactory.decodeFile(obtainPath(), mOpt1);
        assertNotNull(b);
        // Test the bitmap size
        assertEquals(START_HEIGHT, b.getHeight());
        assertEquals(START_WIDTH, b.getWidth());
        // Test if no bitmap
        assertNull(BitmapFactory.decodeFile(obtainPath(), mOpt2));
    }

    public void testDecodeFile2() throws IOException {
        Bitmap b = BitmapFactory.decodeFile(obtainPath());
        assertNotNull(b);
        // Test the bitmap size
        assertEquals(START_HEIGHT, b.getHeight());
        assertEquals(START_WIDTH, b.getWidth());
    }

    public void testDecodeReuseBasic() throws IOException {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inMutable = true;
        options.inSampleSize = 0; // treated as 1
        options.inScaled = false;
        Bitmap start = BitmapFactory.decodeResource(mRes, R.drawable.start, options);
        int originalSize = start.getByteCount();
        assertEquals(originalSize, start.getAllocationByteCount());

        options.inBitmap = start;
        options.inMutable = false; // will be overridden by non-null inBitmap
        options.inSampleSize = -42; // treated as 1
        Bitmap pass = BitmapFactory.decodeResource(mRes, R.drawable.pass, options);

        assertEquals(originalSize, pass.getByteCount());
        assertEquals(originalSize, pass.getAllocationByteCount());
        assertSame(start, pass);
        assertTrue(pass.isMutable());
    }

    /**
     * Create bitmap sized to load unscaled resources: start, pass, and alpha
     */
    private Bitmap createBitmapForReuse(int pixelCount) {
        Bitmap bitmap = Bitmap.createBitmap(pixelCount, 1, Config.ARGB_8888);
        bitmap.eraseColor(Color.BLACK);
        bitmap.setHasAlpha(false);
        return bitmap;
    }

    /**
     * Decode resource with ResId into reuse bitmap without scaling, verifying expected hasAlpha
     */
    private void decodeResourceWithReuse(Bitmap reuse, int resId, boolean hasAlpha) {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inMutable = true;
        options.inSampleSize = 1;
        options.inScaled = false;
        options.inBitmap = reuse;
        Bitmap output = BitmapFactory.decodeResource(mRes, resId, options);
        assertSame(reuse, output);
        assertEquals(output.hasAlpha(), hasAlpha);
    }

    public void testDecodeReuseHasAlpha() throws IOException {
        final int bitmapSize = 31; // size in pixels of start, pass, and alpha resources
        final int pixelCount = bitmapSize * bitmapSize;

        // Test reuse, hasAlpha false and true
        Bitmap bitmap = createBitmapForReuse(pixelCount);
        decodeResourceWithReuse(bitmap, R.drawable.start, false);
        decodeResourceWithReuse(bitmap, R.drawable.alpha, true);

        // Test pre-reconfigure, hasAlpha false and true
        bitmap = createBitmapForReuse(pixelCount);
        bitmap.reconfigure(bitmapSize, bitmapSize, Config.ARGB_8888);
        bitmap.setHasAlpha(true);
        decodeResourceWithReuse(bitmap, R.drawable.start, false);

        bitmap = createBitmapForReuse(pixelCount);
        bitmap.reconfigure(bitmapSize, bitmapSize, Config.ARGB_8888);
        decodeResourceWithReuse(bitmap, R.drawable.alpha, true);
    }

    public void testDecodeReuseFormats() throws IOException {
        // reuse should support all image formats
        for (int i = 0; i < RES_IDS.length; ++i) {
            Bitmap reuseBuffer = Bitmap.createBitmap(1000000, 1, Bitmap.Config.ALPHA_8);

            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inBitmap = reuseBuffer;
            options.inSampleSize = 4;
            options.inScaled = false;
            Bitmap decoded = BitmapFactory.decodeResource(mRes, RES_IDS[i], options);
            assertSame(reuseBuffer, decoded);
        }
    }

    public void testDecodeReuseFailure() throws IOException {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inMutable = true;
        options.inScaled = false;
        options.inSampleSize = 4;
        Bitmap reduced = BitmapFactory.decodeResource(mRes, R.drawable.robot, options);

        options.inBitmap = reduced;
        options.inSampleSize = 1;
        try {
            Bitmap original = BitmapFactory.decodeResource(mRes, R.drawable.robot, options);
            fail("should throw exception due to lack of space");
        } catch (IllegalArgumentException e) {
        }
    }

    public void testDecodeReuseScaling() throws IOException {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inMutable = true;
        options.inScaled = false;
        Bitmap original = BitmapFactory.decodeResource(mRes, R.drawable.robot, options);
        int originalSize = original.getByteCount();
        assertEquals(originalSize, original.getAllocationByteCount());

        options.inBitmap = original;
        options.inSampleSize = 4;
        Bitmap reduced = BitmapFactory.decodeResource(mRes, R.drawable.robot, options);

        assertSame(original, reduced);
        assertEquals(originalSize, reduced.getAllocationByteCount());
        assertEquals(originalSize, reduced.getByteCount() * 16);
    }

    public void testDecodeReuseDoubleScaling() throws IOException {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inMutable = true;
        options.inScaled = false;
        options.inSampleSize = 1;
        Bitmap original = BitmapFactory.decodeResource(mRes, R.drawable.robot, options);
        int originalSize = original.getByteCount();

        // Verify that inSampleSize and density scaling both work with reuse concurrently
        options.inBitmap = original;
        options.inScaled = true;
        options.inSampleSize = 2;
        options.inDensity = 2;
        options.inTargetDensity = 4;
        Bitmap doubleScaled = BitmapFactory.decodeResource(mRes, R.drawable.robot, options);

        assertSame(original, doubleScaled);
        assertEquals(4, doubleScaled.getDensity());
        assertEquals(originalSize, doubleScaled.getByteCount());
    }

    public void testDecodeReuseEquivalentScaling() throws IOException {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inMutable = true;
        options.inScaled = true;
        options.inDensity = 4;
        options.inTargetDensity = 2;
        Bitmap densityReduced = BitmapFactory.decodeResource(mRes, R.drawable.robot, options);
        assertEquals(2, densityReduced.getDensity());
        options.inBitmap = densityReduced;
        options.inDensity = 0;
        options.inScaled = false;
        options.inSampleSize = 2;
        Bitmap scaleReduced = BitmapFactory.decodeResource(mRes, R.drawable.robot, options);
        // verify that density isn't incorrectly carried over during bitmap reuse
        assertFalse(densityReduced.getDensity() == 2);
        assertFalse(densityReduced.getDensity() == 0);
        assertSame(densityReduced, scaleReduced);
    }

    public void testDecodePremultipliedDefault() throws IOException {
        Bitmap simplePremul = BitmapFactory.decodeResource(mRes, R.drawable.premul_data);
        assertTrue(simplePremul.isPremultiplied());
    }

    public void testDecodePremultipliedData() throws IOException {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        Bitmap premul = BitmapFactory.decodeResource(mRes, R.drawable.premul_data, options);
        options.inPremultiplied = false;
        Bitmap unpremul = BitmapFactory.decodeResource(mRes, R.drawable.premul_data, options);
        assertEquals(premul.getConfig(), Bitmap.Config.ARGB_8888);
        assertEquals(unpremul.getConfig(), Bitmap.Config.ARGB_8888);
        assertTrue(premul.getHeight() == 1 && unpremul.getHeight() == 1);
        assertTrue(premul.getWidth() == unpremul.getWidth() &&
                   DEPREMUL_COLORS.length == RAW_COLORS.length &&
                   premul.getWidth() == DEPREMUL_COLORS.length);

        // verify pixel data - unpremul should have raw values, premul will have rounding errors
        for (int i = 0; i < premul.getWidth(); i++) {
            assertEquals(premul.getPixel(i, 0), DEPREMUL_COLORS[i]);
            assertEquals(unpremul.getPixel(i, 0), RAW_COLORS[i]);
        }
    }

    public void testDecodeInPurgeableAllocationCount() {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inSampleSize = 1;
        options.inJustDecodeBounds = false;
        options.inPurgeable = true;
        options.inInputShareable = false;
        byte[] array = obtainArray();
        Bitmap purgeableBitmap = BitmapFactory.decodeByteArray(array, 0, array.length, options);
        assertFalse(purgeableBitmap.getAllocationByteCount() == 0);
    }

    private int mDefaultCreationDensity;
    private void verifyScaled(Bitmap b) {
        assertEquals(b.getWidth(), START_WIDTH * 2);
        assertEquals(b.getDensity(), 2);
    }

    private void verifyUnscaled(Bitmap b) {
        assertEquals(b.getWidth(), START_WIDTH);
        assertEquals(b.getDensity(), mDefaultCreationDensity);
    }

    public void testDecodeScaling() {
        BitmapFactory.Options defaultOpt = new BitmapFactory.Options();

        BitmapFactory.Options unscaledOpt = new BitmapFactory.Options();
        unscaledOpt.inScaled = false;

        BitmapFactory.Options scaledOpt = new BitmapFactory.Options();
        scaledOpt.inScaled = true;
        scaledOpt.inDensity = 1;
        scaledOpt.inTargetDensity = 2;

        mDefaultCreationDensity = Bitmap.createBitmap(1, 1, Config.ARGB_8888).getDensity();

        byte[] bytes = obtainArray();

        verifyUnscaled(BitmapFactory.decodeByteArray(bytes, 0, bytes.length));
        verifyUnscaled(BitmapFactory.decodeByteArray(bytes, 0, bytes.length, null));
        verifyUnscaled(BitmapFactory.decodeByteArray(bytes, 0, bytes.length, unscaledOpt));
        verifyUnscaled(BitmapFactory.decodeByteArray(bytes, 0, bytes.length, defaultOpt));

        verifyUnscaled(BitmapFactory.decodeStream(obtainInputStream()));
        verifyUnscaled(BitmapFactory.decodeStream(obtainInputStream(), null, null));
        verifyUnscaled(BitmapFactory.decodeStream(obtainInputStream(), null, unscaledOpt));
        verifyUnscaled(BitmapFactory.decodeStream(obtainInputStream(), null, defaultOpt));

        // scaling should only occur if Options are passed with inScaled=true
        verifyScaled(BitmapFactory.decodeByteArray(bytes, 0, bytes.length, scaledOpt));
        verifyScaled(BitmapFactory.decodeStream(obtainInputStream(), null, scaledOpt));
    }

    // Test that writing an index8 bitmap to a Parcel succeeds.
    public void testParcel() {
        // Turn off scaling, which would convert to an 8888 bitmap, which does not expose
        // the bug.
        BitmapFactory.Options opts = new BitmapFactory.Options();
        opts.inScaled = false;
        Bitmap b = BitmapFactory.decodeResource(mRes, R.drawable.gif_test, opts);
        assertNotNull(b);

        // index8 has no Java equivalent, so the Config will be null.
        assertNull(b.getConfig());

        Parcel p = Parcel.obtain();
        b.writeToParcel(p, 0);

        p.setDataPosition(0);
        Bitmap b2 = Bitmap.CREATOR.createFromParcel(p);
        compareBitmaps(b, b2, 0, true, true);

        // When this failed previously, the bitmap was missing a colortable, resulting in a crash
        // attempting to compress by dereferencing a null pointer. Compress to verify that we do
        // not crash, but succeed instead.
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        assertTrue(b2.compress(Bitmap.CompressFormat.JPEG, 50, baos));
    }

    public void testConfigs() {
        // The output Config of a BitmapFactory decode depends on the request from the
        // client and the properties of the image to be decoded.
        //
        // Options.inPreferredConfig = Config.ARGB_8888
        //     This is the default value of inPreferredConfig.  In this case, the image
        //     will always be decoded to Config.ARGB_8888.
        // Options.inPreferredConfig = Config.RGB_565
        //     If the encoded image is opaque, we will decode to Config.RGB_565,
        //     otherwise we will decode to whichever color type is the most natural match
        //     for the encoded data.
        // Options.inPreferredConfig = Config.ARGB_4444
        //     This is deprecated and will always decode to Config.ARGB_8888.
        // Options.inPreferredConfig = Config.ALPHA_8
        //     If the encoded image is gray, we will decode to 8-bit grayscale values
        //     and indicate that the output bitmap is Config.ALPHA_8.  This is somewhat
        //     misleading because the image is really opaque and grayscale, but we are
        //     labeling each pixel as if it is a translucency (alpha) value.  If the
        //     encoded image is not gray, we will decode to whichever color type is the
        //     most natural match for the encoded data.
        // Options.inPreferredConfig = null
        //     We will decode to whichever Config is the most natural match with the
        //     encoded data.  This could be 8-bit indices into a color table (call this
        //     INDEX_8), ALPHA_8 (gray), or ARGB_8888.
        //
        // This test ensures that images are decoded to the intended Config and that the
        // decodes match regardless of the Config.
        decodeConfigs(R.drawable.alpha, 31, 31, true, false, false);
        decodeConfigs(R.drawable.baseline_jpeg, 1280, 960, false, false, false);
        decodeConfigs(R.drawable.bmp_test, 320, 240, false, false, false);
        decodeConfigs(R.drawable.scaled2, 6, 8, false, false, true);
        decodeConfigs(R.drawable.grayscale_jpg, 128, 128, false, true, false);
        decodeConfigs(R.drawable.grayscale_png, 128, 128, false, true, false);
    }

    private void decodeConfigs(int id, int width, int height, boolean hasAlpha, boolean isGray,
            boolean hasColorTable) {
        Options opts = new BitmapFactory.Options();
        opts.inScaled = false;
        assertEquals(Config.ARGB_8888, opts.inPreferredConfig);
        Bitmap reference = BitmapFactory.decodeResource(mRes, id, opts);
        assertNotNull(reference);
        assertEquals(width, reference.getWidth());
        assertEquals(height, reference.getHeight());
        assertEquals(Config.ARGB_8888, reference.getConfig());

        opts.inPreferredConfig = Config.ARGB_4444;
        Bitmap argb4444 = BitmapFactory.decodeResource(mRes, id, opts);
        assertNotNull(argb4444);
        assertEquals(width, argb4444.getWidth());
        assertEquals(height, argb4444.getHeight());
        // ARGB_4444 is deprecated and we should decode to ARGB_8888.
        assertEquals(Config.ARGB_8888, argb4444.getConfig());
        compareBitmaps(reference, argb4444, 0, true, true);

        opts.inPreferredConfig = Config.RGB_565;
        Bitmap rgb565 = BitmapFactory.decodeResource(mRes, id, opts);
        assertNotNull(rgb565);
        assertEquals(width, rgb565.getWidth());
        assertEquals(height, rgb565.getHeight());
        if (!hasAlpha) {
            assertEquals(Config.RGB_565, rgb565.getConfig());
            // Convert the RGB_565 bitmap to ARGB_8888 and test that it is similar to
            // the reference.  We lose information when decoding to 565, so there must
            // be some tolerance.  The tolerance is intentionally loose to allow us some
            // flexibility regarding if we dither and how we color convert.
            compareBitmaps(reference, rgb565.copy(Config.ARGB_8888, false), 30, true, true);
        }

        opts.inPreferredConfig = Config.ALPHA_8;
        Bitmap alpha8 = BitmapFactory.decodeResource(mRes, id, opts);
        assertNotNull(alpha8);
        assertEquals(width, reference.getWidth());
        assertEquals(height, reference.getHeight());
        if (isGray) {
            assertEquals(Config.ALPHA_8, alpha8.getConfig());
            // Convert the ALPHA_8 bitmap to ARGB_8888 and test that it is identical to
            // the reference.  We must do this manually because we are abusing ALPHA_8
            // in order to represent grayscale.
            compareBitmaps(reference, grayToARGB(alpha8), 0, true, true);
        }

        // Setting inPreferredConfig to null selects the most natural color type for
        // the encoded data.  If the image has a color table, this should be INDEX_8.
        // If we decode to INDEX_8, the output bitmap will report that the Config is
        // null.
        opts.inPreferredConfig = null;
        Bitmap index8 = BitmapFactory.decodeResource(mRes, id, opts);
        assertNotNull(index8);
        assertEquals(width, index8.getWidth());
        assertEquals(height, index8.getHeight());
        if (hasColorTable) {
            assertEquals(null, index8.getConfig());
            // Convert the INDEX_8 bitmap to ARGB_8888 and test that it is identical to
            // the reference.
            compareBitmaps(reference, index8.copy(Config.ARGB_8888, false), 0, true, true);
        }
    }

    private Bitmap grayToARGB(Bitmap gray) {
        Bitmap argb = Bitmap.createBitmap(gray.getWidth(), gray.getHeight(), Config.ARGB_8888);
        for (int y = 0; y < argb.getHeight(); y++) {
            for (int x = 0; x < argb.getWidth(); x++) {
                int grayByte = Color.alpha(gray.getPixel(x, y));
                argb.setPixel(x, y, Color.rgb(grayByte, grayByte, grayByte));
            }
        }
        return argb;
    }

    private byte[] obtainArray() {
        ByteArrayOutputStream stm = new ByteArrayOutputStream();
        Options opt = new BitmapFactory.Options();
        opt.inScaled = false;
        Bitmap bitmap = BitmapFactory.decodeResource(mRes, R.drawable.start, opt);
        bitmap.compress(Bitmap.CompressFormat.JPEG, 0, stm);
        return(stm.toByteArray());
    }

    private InputStream obtainInputStream() {
        return mRes.openRawResource(R.drawable.start);
    }

    private InputStream obtainInputStream(int resId) {
        return mRes.openRawResource(resId);
    }

    private ParcelFileDescriptor obtainParcelDescriptor(String path)
            throws IOException {
        File file = new File(path);
        return(ParcelFileDescriptor.open(file,
                ParcelFileDescriptor.MODE_READ_ONLY));
    }

    private String obtainPath() throws IOException {
        return obtainPath(R.drawable.start, 0);
    }

    /*
     * Create a new file and return a path to it.
     * @param resId Original file. It will be copied into the new file.
     * @param offset Number of zeroes to write to the new file before the
     *               copied file. This allows testing decodeFileDescriptor
     *               with an offset. Must be less than or equal to 1024
     */
    private String obtainPath(int resId, long offset) throws IOException {
        File dir = getInstrumentation().getTargetContext().getFilesDir();
        dir.mkdirs();
        // The suffix does not necessarily represent theactual file type.
        File file = new File(dir, "test.jpg");
        if (!file.createNewFile()) {
            if (!file.exists()) {
                fail("Failed to create new File!");
            }
        }
        InputStream is = obtainInputStream(resId);
        FileOutputStream fOutput = new FileOutputStream(file);
        byte[] dataBuffer = new byte[1024];
        // Write a bunch of zeroes before the image.
        assertTrue(offset <= 1024);
        fOutput.write(dataBuffer, 0, (int) offset);
        int readLength = 0;
        while ((readLength = is.read(dataBuffer)) != -1) {
            fOutput.write(dataBuffer, 0, readLength);
        }
        is.close();
        fOutput.close();
        return (file.getPath());
    }

    // Compare expected to actual to see if their diff is less then mseMargin.
    // lessThanMargin is to indicate whether we expect the mean square error
    // to be "less than" or "no less than".
    private void compareBitmaps(Bitmap expected, Bitmap actual,
            int mseMargin, boolean lessThanMargin, boolean isPremultiplied) {
        final int width = expected.getWidth();
        final int height = expected.getHeight();

        assertEquals("mismatching widths", width, actual.getWidth());
        assertEquals("mismatching heights", height, actual.getHeight());
        assertEquals("mismatching configs", expected.getConfig(),
                actual.getConfig());

        double mse = 0;
        int[] expectedColors = new int [width * height];
        int[] actualColors = new int [width * height];

        // Bitmap.getPixels() returns colors with non-premultiplied ARGB values.
        expected.getPixels(expectedColors, 0, width, 0, 0, width, height);
        actual.getPixels(actualColors, 0, width, 0, 0, width, height);

        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                int idx = row * width + col;
                mse += distance(expectedColors[idx], actualColors[idx], isPremultiplied);
            }
        }
        mse /= width * height;

        if (lessThanMargin) {
            assertTrue("MSE " + mse +  "larger than the threshold: " + mseMargin,
                    mse <= mseMargin);
        } else {
            assertFalse("MSE " + mse +  "smaller than the threshold: " + mseMargin,
                    mse <= mseMargin);
        }
    }

    private int multiplyAlpha(int color, int alpha) {
        return (color * alpha + 127) / 255;
    }

    // For the Bitmap with Alpha, multiply the Alpha values to get the effective
    // RGB colors and then compute the color-distance.
    private double distance(int expect, int actual, boolean isPremultiplied) {
        if (isPremultiplied) {
            final int a1 = Color.alpha(actual);
            final int a2 = Color.alpha(expect);
            final int r = multiplyAlpha(Color.red(actual), a1) -
                    multiplyAlpha(Color.red(expect), a2);
            final int g = multiplyAlpha(Color.green(actual), a1) -
                    multiplyAlpha(Color.green(expect), a2);
            final int b = multiplyAlpha(Color.blue(actual), a1) -
                    multiplyAlpha(Color.blue(expect), a2);
            return r * r + g * g + b * b;
        } else {
            final int r = Color.red(actual) - Color.red(expect);
            final int g = Color.green(actual) - Color.green(expect);
            final int b = Color.blue(actual) - Color.blue(expect);
            return r * r + g * g + b * b;
        }
    }
}
