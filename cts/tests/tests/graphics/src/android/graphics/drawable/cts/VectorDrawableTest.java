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

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.content.res.Resources;
import android.content.res.Resources.Theme;
import android.content.res.XmlResourceParser;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff.Mode;
import android.graphics.PorterDuffColorFilter;
import android.graphics.cts.R;
import android.graphics.drawable.Drawable.ConstantState;
import android.graphics.drawable.VectorDrawable;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Xml;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

public class VectorDrawableTest extends AndroidTestCase {
    private static final String LOGTAG = "VectorDrawableTest";

    // Separate the test assets into different groups such that we could isolate the issue faster.
    // Some new APIs or bug fixes only exist in particular os version, such that we name the tests
    // and associated assets with OS code name L, M, N etc...
    private static final int[] BASIC_ICON_RES_IDS = new int[]{
            R.drawable.vector_icon_create,
            R.drawable.vector_icon_delete,
            R.drawable.vector_icon_heart,
            R.drawable.vector_icon_schedule,
            R.drawable.vector_icon_settings,
            R.drawable.vector_icon_random_path_1,
            R.drawable.vector_icon_random_path_2,
            R.drawable.vector_icon_repeated_cq,
            R.drawable.vector_icon_repeated_st,
            R.drawable.vector_icon_repeated_a_1,
            R.drawable.vector_icon_repeated_a_2,
            R.drawable.vector_icon_clip_path_1,
    };

    private static final int[] BASIC_GOLDEN_IMAGES = new int[] {
            R.drawable.vector_icon_create_golden,
            R.drawable.vector_icon_delete_golden,
            R.drawable.vector_icon_heart_golden,
            R.drawable.vector_icon_schedule_golden,
            R.drawable.vector_icon_settings_golden,
            R.drawable.vector_icon_random_path_1_golden,
            R.drawable.vector_icon_random_path_2_golden,
            R.drawable.vector_icon_repeated_cq_golden,
            R.drawable.vector_icon_repeated_st_golden,
            R.drawable.vector_icon_repeated_a_1_golden,
            R.drawable.vector_icon_repeated_a_2_golden,
            R.drawable.vector_icon_clip_path_1_golden,
    };

    private static final int[] L_M_ICON_RES_IDS = new int[] {
            R.drawable.vector_icon_transformation_1,
            R.drawable.vector_icon_transformation_2,
            R.drawable.vector_icon_transformation_3,
            R.drawable.vector_icon_transformation_4,
            R.drawable.vector_icon_transformation_5,
            R.drawable.vector_icon_transformation_6,
            R.drawable.vector_icon_render_order_1,
            R.drawable.vector_icon_render_order_2,
            R.drawable.vector_icon_stroke_1,
            R.drawable.vector_icon_stroke_2,
            R.drawable.vector_icon_stroke_3,
            R.drawable.vector_icon_scale_1,
            R.drawable.vector_icon_scale_2,
    };

    private static final int[] L_M_GOLDEN_IMAGES = new int[] {
            R.drawable.vector_icon_transformation_1_golden,
            R.drawable.vector_icon_transformation_2_golden,
            R.drawable.vector_icon_transformation_3_golden,
            R.drawable.vector_icon_transformation_4_golden,
            R.drawable.vector_icon_transformation_5_golden,
            R.drawable.vector_icon_transformation_6_golden,
            R.drawable.vector_icon_render_order_1_golden,
            R.drawable.vector_icon_render_order_2_golden,
            R.drawable.vector_icon_stroke_1_golden,
            R.drawable.vector_icon_stroke_2_golden,
            R.drawable.vector_icon_stroke_3_golden,
            R.drawable.vector_icon_scale_1_golden,
            R.drawable.vector_icon_scale_2_golden,
    };

    private static final int[] N_ICON_RES_IDS = new int[] {
            R.drawable.vector_icon_implicit_lineto,
            R.drawable.vector_icon_arcto,
            R.drawable.vector_icon_filltype_nonzero,
            R.drawable.vector_icon_filltype_evenodd,
    };

    private static final int[] N_GOLDEN_IMAGES = new int[] {
            R.drawable.vector_icon_implicit_lineto_golden,
            R.drawable.vector_icon_arcto_golden,
            R.drawable.vector_icon_filltype_nonzero_golden,
            R.drawable.vector_icon_filltype_evenodd_golden,
    };

    private static final int[] GRADIENT_ICON_RES_IDS = new int[] {
            R.drawable.vector_icon_gradient_1,
            R.drawable.vector_icon_gradient_2,
            R.drawable.vector_icon_gradient_3,
            R.drawable.vector_icon_gradient_1_clamp,
            R.drawable.vector_icon_gradient_2_repeat,
            R.drawable.vector_icon_gradient_3_mirror,
    };

    private static final int[] GRADIENT_GOLDEN_IMAGES = new int[] {
            R.drawable.vector_icon_gradient_1_golden,
            R.drawable.vector_icon_gradient_2_golden,
            R.drawable.vector_icon_gradient_3_golden,
            R.drawable.vector_icon_gradient_1_clamp_golden,
            R.drawable.vector_icon_gradient_2_repeat_golden,
            R.drawable.vector_icon_gradient_3_mirror_golden,
    };

    private static final int[] STATEFUL_RES_IDS = new int[] {
            // All these icons are using the same color state list, make sure it works for either
            // the same drawable ID or different ID but same content.
            R.drawable.vector_icon_state_list,
            R.drawable.vector_icon_state_list,
            R.drawable.vector_icon_state_list_2,
    };

    private static final int[][] STATEFUL_GOLDEN_IMAGES = new int[][] {
            {
                    R.drawable.vector_icon_state_list_golden,
                    R.drawable.vector_icon_state_list_golden,
                    R.drawable.vector_icon_state_list_2_golden
            },
            {
                    R.drawable.vector_icon_state_list_golden_pressed,
                    R.drawable.vector_icon_state_list_golden_pressed,
                    R.drawable.vector_icon_state_list_2_golden_pressed
            }
    };

    private static final int[][] STATEFUL_STATE_SETS = new int[][] {
            {},
            { android.R.attr.state_pressed }
    };

    private static final int IMAGE_WIDTH = 64;
    private static final int IMAGE_HEIGHT = 64;
    // A small value is actually making sure that the values are matching
    // exactly with the golden image.
    // We can increase the threshold if the Skia is drawing with some variance
    // on different devices. So far, the tests show they are matching correctly.
    private static final float PIXEL_ERROR_THRESHOLD = 0.03f;
    private static final float PIXEL_ERROR_COUNT_THRESHOLD = 0.005f;
    private static final int PIXEL_ERROR_TOLERANCE = 3;

    private static final boolean DBG_DUMP_PNG = false;

    private Resources mResources;
    private Bitmap mBitmap;
    private Canvas mCanvas;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        final int width = IMAGE_WIDTH;
        final int height = IMAGE_HEIGHT;

        mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        mCanvas = new Canvas(mBitmap);
        mResources = mContext.getResources();
    }

    @MediumTest
    public void testBasicVectorDrawables() throws XmlPullParserException, IOException {
        verifyVectorDrawables(BASIC_ICON_RES_IDS, BASIC_GOLDEN_IMAGES, null);
    }

    @MediumTest
    public void testLMVectorDrawables() throws XmlPullParserException, IOException {
        verifyVectorDrawables(L_M_ICON_RES_IDS, L_M_GOLDEN_IMAGES, null);
    }

    @MediumTest
    public void testNVectorDrawables() throws XmlPullParserException, IOException {
        verifyVectorDrawables(N_ICON_RES_IDS, N_GOLDEN_IMAGES, null);
    }

    @MediumTest
    public void testVectorDrawableGradient() throws XmlPullParserException, IOException {
        verifyVectorDrawables(GRADIENT_ICON_RES_IDS, GRADIENT_GOLDEN_IMAGES, null);
    }

    @MediumTest
    public void testColorStateList() throws XmlPullParserException, IOException {
        for (int i = 0; i < STATEFUL_STATE_SETS.length; i++) {
            verifyVectorDrawables(
                    STATEFUL_RES_IDS, STATEFUL_GOLDEN_IMAGES[i], STATEFUL_STATE_SETS[i]);
        }
    }

    private void verifyVectorDrawables(int[] resIds, int[] goldenImages, int[] stateSet)
            throws XmlPullParserException, IOException {
        for (int i = 0; i < resIds.length; i++) {
            VectorDrawable vectorDrawable = new VectorDrawable();
            vectorDrawable.setBounds(0, 0, IMAGE_WIDTH, IMAGE_HEIGHT);


            // Setup VectorDrawable from xml file and draw into the bitmap.
            XmlPullParser parser = mResources.getXml(resIds[i]);
            AttributeSet attrs = Xml.asAttributeSet(parser);

            int type;
            while ((type=parser.next()) != XmlPullParser.START_TAG
                    && type != XmlPullParser.END_DOCUMENT) {
                // Empty loop
            }

            if (type != XmlPullParser.START_TAG) {
                throw new XmlPullParserException("No start tag found");
            }

            Theme theme = mResources.newTheme();
            theme.applyStyle(R.style.Theme_ThemedDrawableTest, true);
            vectorDrawable.inflate(mResources, parser, attrs, theme);

            if (stateSet != null) {
                vectorDrawable.setState(stateSet);
            }

            mBitmap.eraseColor(0);
            vectorDrawable.draw(mCanvas);

            if (DBG_DUMP_PNG) {
                saveVectorDrawableIntoPNG(mBitmap, resIds, i, stateSet);
            } else {
                // Start to compare
                Bitmap golden = BitmapFactory.decodeResource(mResources, goldenImages[i]);
                DrawableTestUtils.compareImages(mResources.getString(resIds[i]), mBitmap, golden,
                        VectorDrawableTest.PIXEL_ERROR_THRESHOLD,
                        VectorDrawableTest.PIXEL_ERROR_COUNT_THRESHOLD,
                        VectorDrawableTest.PIXEL_ERROR_TOLERANCE);

            }
        }
    }

    // This is only for debugging or golden image (re)generation purpose.
    private void saveVectorDrawableIntoPNG(Bitmap bitmap, int[] resIds, int index, int[] stateSet)
            throws IOException {
        // Save the image to the disk.
        FileOutputStream out = null;
        try {
            String outputFolder = "/sdcard/temp/";
            File folder = new File(outputFolder);
            if (!folder.exists()) {
                folder.mkdir();
            }
            String originalFilePath = mResources.getString(resIds[index]);
            File originalFile = new File(originalFilePath);
            String fileFullName = originalFile.getName();
            String fileTitle = fileFullName.substring(0, fileFullName.lastIndexOf("."));
            String stateSetTitle = getTitleForStateSet(stateSet);
            String outputFilename = outputFolder + fileTitle + "_golden" + stateSetTitle + ".png";
            File outputFile = new File(outputFilename);
            if (!outputFile.exists()) {
                outputFile.createNewFile();
            }

            out = new FileOutputStream(outputFile, false);
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, out);
            Log.v(LOGTAG, "Write test No." + index + " to file successfully.");
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (out != null) {
                out.close();
            }
        }
    }

    /**
     * Generates an underline-delimited list of states in a given state set.
     * <p>
     * For example, the array {@code {R.attr.state_pressed}} would return
     * {@code "_pressed"}.
     *
     * @param stateSet a state set
     * @return a string representing the state set, or the empty string if the
     *         state set is empty or {@code null}
     */
    private String getTitleForStateSet(int[] stateSet) {
        if (stateSet == null || stateSet.length == 0) {
            return "";
        }

        final Resources res = getContext().getResources();
        final StringBuilder builder = new StringBuilder();
        for (int i = 0; i < stateSet.length; i++) {
            builder.append('_');

            final String state = res.getResourceName(stateSet[i]);
            final int stateIndex = state.indexOf("state_");
            if (stateIndex >= 0) {
                builder.append(state.substring(stateIndex + 6));
            } else {
                builder.append(stateSet[i]);
            }
        }

        return builder.toString();
    }

    @SmallTest
    public void testGetChangingConfigurations() {
        VectorDrawable vectorDrawable = new VectorDrawable();
        ConstantState constantState = vectorDrawable.getConstantState();

        // default
        assertEquals(0, constantState.getChangingConfigurations());
        assertEquals(0, vectorDrawable.getChangingConfigurations());

        // change the drawable's configuration does not affect the state's configuration
        vectorDrawable.setChangingConfigurations(0xff);
        assertEquals(0xff, vectorDrawable.getChangingConfigurations());
        assertEquals(0, constantState.getChangingConfigurations());

        // the state's configuration get refreshed
        constantState = vectorDrawable.getConstantState();
        assertEquals(0xff,  constantState.getChangingConfigurations());

        // set a new configuration to drawable
        vectorDrawable.setChangingConfigurations(0xff00);
        assertEquals(0xff,  constantState.getChangingConfigurations());
        assertEquals(0xffff,  vectorDrawable.getChangingConfigurations());
    }

    @SmallTest
    public void testGetConstantState() {
        VectorDrawable vectorDrawable = new VectorDrawable();
        ConstantState constantState = vectorDrawable.getConstantState();
        assertNotNull(constantState);
        assertEquals(0, constantState.getChangingConfigurations());

        vectorDrawable.setChangingConfigurations(1);
        constantState = vectorDrawable.getConstantState();
        assertNotNull(constantState);
        assertEquals(1, constantState.getChangingConfigurations());
    }

    @SmallTest
    public void testMutate() {
        Resources resources = mContext.getResources();
        // d1 and d2 will be mutated, while d3 will not.
        VectorDrawable d1 = (VectorDrawable) resources.getDrawable(R.drawable.vector_icon_create);
        VectorDrawable d2 = (VectorDrawable) resources.getDrawable(R.drawable.vector_icon_create);
        VectorDrawable d3 = (VectorDrawable) resources.getDrawable(R.drawable.vector_icon_create);
        int originalAlpha = d2.getAlpha();

        d1.setAlpha(0x80);
        assertEquals(0x80, d1.getAlpha());
        assertEquals(0x80, d2.getAlpha());
        assertEquals(0x80, d3.getAlpha());

        d1.mutate();
        d1.setAlpha(0x40);
        assertEquals(0x40, d1.getAlpha());
        assertEquals(0x80, d2.getAlpha());
        assertEquals(0x80, d3.getAlpha());

        d2.setAlpha(0x00);
        d2.mutate();
        // Test that after mutating, the alpha value is copied over.
        assertEquals(0x00, d2.getAlpha());

        d2.setAlpha(0x20);
        assertEquals(0x40, d1.getAlpha());
        assertEquals(0x20, d2.getAlpha());
        assertEquals(0x00, d3.getAlpha());

        d3.setAlpha(originalAlpha);
    }

    @SmallTest
    public void testColorFilter() {
        PorterDuffColorFilter filter = new PorterDuffColorFilter(Color.RED, Mode.SRC_IN);
        VectorDrawable vectorDrawable = new VectorDrawable();
        vectorDrawable.setColorFilter(filter);

        assertEquals(filter, vectorDrawable.getColorFilter());
    }

    @SmallTest
    public void testPreloadDensity() throws XmlPullParserException, IOException {
        final Resources res = getContext().getResources();
        final int densityDpi = res.getConfiguration().densityDpi;
        try {
            testPreloadDensityInner(res, densityDpi);
        } finally {
            DrawableTestUtils.setResourcesDensity(res, densityDpi);
        }
    }

    @SmallTest
    public void testGetOpacity () throws XmlPullParserException, IOException {
        VectorDrawable vectorDrawable = new VectorDrawable();

        assertEquals("Default alpha should be 255", 255, vectorDrawable.getAlpha());
        assertEquals("Default opacity should be TRANSLUCENT", PixelFormat.TRANSLUCENT,
                vectorDrawable.getOpacity());

        vectorDrawable.setAlpha(0);
        assertEquals("Alpha should be 0 now", 0, vectorDrawable.getAlpha());
        assertEquals("Opacity should be TRANSPARENT now", PixelFormat.TRANSPARENT,
                vectorDrawable.getOpacity());
    }

    private void testPreloadDensityInner(Resources res, int densityDpi)
            throws XmlPullParserException, IOException {
        // Capture initial state at default density.
        final XmlResourceParser parser = DrawableTestUtils.getResourceParser(
                res, R.drawable.vector_density);
        final VectorDrawable preloadedDrawable = new VectorDrawable();
        preloadedDrawable.inflate(mResources, parser, Xml.asAttributeSet(parser));
        final ConstantState preloadedConstantState = preloadedDrawable.getConstantState();
        final int origWidth = preloadedDrawable.getIntrinsicWidth();

        // Set density to half of original. Unlike offsets, which are
        // truncated, dimensions are rounded to the nearest pixel.
        DrawableTestUtils.setResourcesDensity(res, densityDpi / 2);
        final VectorDrawable halfDrawable =
                (VectorDrawable) preloadedConstantState.newDrawable(res);
        assertEquals(Math.round(origWidth / 2f), halfDrawable.getIntrinsicWidth());

        // Set density to double original.
        DrawableTestUtils.setResourcesDensity(res, densityDpi * 2);
        final VectorDrawable doubleDrawable =
                (VectorDrawable) preloadedConstantState.newDrawable(res);
        assertEquals(origWidth * 2, doubleDrawable.getIntrinsicWidth());

        // Restore original density.
        DrawableTestUtils.setResourcesDensity(res, densityDpi);
        final VectorDrawable origDrawable =
                (VectorDrawable) preloadedConstantState.newDrawable();
        assertEquals(origWidth, origDrawable.getIntrinsicWidth());

        // Ensure theme density is applied correctly.
        final Theme t = res.newTheme();
        halfDrawable.applyTheme(t);
        assertEquals(origWidth, halfDrawable.getIntrinsicWidth());
        doubleDrawable.applyTheme(t);
        assertEquals(origWidth, doubleDrawable.getIntrinsicWidth());
    }
}
