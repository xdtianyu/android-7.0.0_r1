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
package android.uirendering.cts.testinfrastructure;

import android.annotation.Nullable;
import android.app.Instrumentation;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.renderscript.Allocation;
import android.renderscript.RenderScript;
import android.support.test.rule.ActivityTestRule;
import android.uirendering.cts.bitmapcomparers.BitmapComparer;
import android.uirendering.cts.bitmapverifiers.BitmapVerifier;
import android.uirendering.cts.differencevisualizers.DifferenceVisualizer;
import android.uirendering.cts.differencevisualizers.PassFailVisualizer;
import android.uirendering.cts.util.BitmapDumper;
import android.util.Log;

import android.support.test.InstrumentationRegistry;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.rules.TestName;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertTrue;

/**
 * This class contains the basis for the graphics hardware test classes. Contained within this class
 * are several methods that help with the execution of tests, and should be extended to gain the
 * functionality built in.
 */
public abstract class ActivityTestBase {
    public static final String TAG = "ActivityTestBase";
    public static final boolean DEBUG = false;
    public static final boolean USE_RS = false;

    //The minimum height and width of a device
    public static final int TEST_WIDTH = 90;
    public static final int TEST_HEIGHT = 90;

    private int[] mHardwareArray = new int[TEST_HEIGHT * TEST_WIDTH];
    private int[] mSoftwareArray = new int[TEST_HEIGHT * TEST_WIDTH];
    private DifferenceVisualizer mDifferenceVisualizer;
    private RenderScript mRenderScript;
    private TestCaseBuilder mTestCaseBuilder;

    @Rule
    public ActivityTestRule<DrawActivity> mActivityRule = new ActivityTestRule<>(
            DrawActivity.class);

    @Rule
    public TestName name = new TestName();

    /**
     * The default constructor creates the package name and sets the DrawActivity as the class that
     * we would use.
     */
    public ActivityTestBase() {
        mDifferenceVisualizer = new PassFailVisualizer();

        // Create a location for the files to be held, if it doesn't exist already
        BitmapDumper.createSubDirectory(this.getClass().getSimpleName());

        // If we have a test currently, let's remove the older files if they exist
        if (getName() != null) {
            BitmapDumper.deleteFileInClassFolder(this.getClass().getSimpleName(), getName());
        }
    }

    protected DrawActivity getActivity() {
        return mActivityRule.getActivity();
    }

    protected String getName() {
        return name.getMethodName();
    }

    protected Instrumentation getInstrumentation() {
        return InstrumentationRegistry.getInstrumentation();
    }

    @Before
    public void setUp() {
        mDifferenceVisualizer = new PassFailVisualizer();
        if (USE_RS) {
            mRenderScript = RenderScript.create(getActivity().getApplicationContext());
        }
    }

    @After
    public void tearDown() {
        if (mTestCaseBuilder != null) {
            List<TestCase> testCases = mTestCaseBuilder.getTestCases();

            if (testCases.size() == 0) {
                throw new IllegalStateException("Must have at least one test case");
            }

            for (TestCase testCase : testCases) {
                if (!testCase.wasTestRan) {
                    Log.w(TAG, getName() + " not all of the tests ran");
                    break;
                }
            }
            mTestCaseBuilder = null;
        }
    }

    public Bitmap takeScreenshot(Point testOffset) {
        getInstrumentation().waitForIdleSync();
        Bitmap source = getInstrumentation().getUiAutomation().takeScreenshot();
        return Bitmap.createBitmap(source, testOffset.x, testOffset.y, TEST_WIDTH, TEST_HEIGHT);
    }

    protected Point runRenderSpec(TestCase testCase) {
        Point testOffset = getActivity().enqueueRenderSpecAndWait(
                testCase.layoutID, testCase.canvasClient,
                null, testCase.viewInitializer, testCase.useHardware);
        testCase.wasTestRan = true;
        if (testCase.readyFence != null) {
            try {
                testCase.readyFence.await(5, TimeUnit.SECONDS);
            } catch (InterruptedException e) {
                throw new RuntimeException("readyFence didn't signal within 5 seconds");
            }
        }
        return testOffset;
    }

    /**
     * Used to execute a specific part of a test and get the resultant bitmap
     */
    protected Bitmap captureRenderSpec(TestCase testCase) {
        Point testOffset = runRenderSpec(testCase);
        return takeScreenshot(testOffset);
    }

    /**
     * Compares the two bitmaps saved using the given test. If they fail, the files are saved using
     * the test name.
     */
    protected void assertBitmapsAreSimilar(Bitmap bitmap1, Bitmap bitmap2,
            BitmapComparer comparer, String debugMessage) {
        boolean success;

        if (USE_RS && comparer.supportsRenderScript()) {
            Allocation idealAllocation = Allocation.createFromBitmap(mRenderScript, bitmap1,
                    Allocation.MipmapControl.MIPMAP_NONE, Allocation.USAGE_SCRIPT);
            Allocation givenAllocation = Allocation.createFromBitmap(mRenderScript, bitmap2,
                    Allocation.MipmapControl.MIPMAP_NONE, Allocation.USAGE_SCRIPT);
            success = comparer.verifySameRS(getActivity().getResources(), idealAllocation,
                    givenAllocation, 0, TEST_WIDTH, TEST_WIDTH, TEST_HEIGHT, mRenderScript);
        } else {
            bitmap1.getPixels(mSoftwareArray, 0, TEST_WIDTH, 0, 0, TEST_WIDTH, TEST_HEIGHT);
            bitmap2.getPixels(mHardwareArray, 0, TEST_WIDTH, 0, 0, TEST_WIDTH, TEST_HEIGHT);
            success = comparer.verifySame(mSoftwareArray, mHardwareArray, 0, TEST_WIDTH, TEST_WIDTH,
                    TEST_HEIGHT);
        }

        if (!success) {
            BitmapDumper.dumpBitmaps(bitmap1, bitmap2, getName(), this.getClass().getSimpleName(),
                    mDifferenceVisualizer);
        }

        assertTrue(debugMessage, success);
    }

    /**
     * Tests to see if a bitmap passes a verifier's test. If it doesn't the bitmap is saved to the
     * sdcard.
     */
    protected void assertBitmapIsVerified(Bitmap bitmap, BitmapVerifier bitmapVerifier,
            String debugMessage) {
        bitmap.getPixels(mSoftwareArray, 0, TEST_WIDTH, 0, 0,
                TEST_WIDTH, TEST_HEIGHT);
        boolean success = bitmapVerifier.verify(mSoftwareArray, 0, TEST_WIDTH, TEST_WIDTH, TEST_HEIGHT);
        if (!success) {
            Bitmap croppedBitmap = Bitmap.createBitmap(bitmap, 0, 0, TEST_WIDTH, TEST_HEIGHT);
            BitmapDumper.dumpBitmap(croppedBitmap, getName(), this.getClass().getSimpleName());
            BitmapDumper.dumpBitmap(bitmapVerifier.getDifferenceBitmap(), getName() + "_verifier",
                    this.getClass().getSimpleName());
        }
        assertTrue(debugMessage, success);
    }

    protected TestCaseBuilder createTest() {
        mTestCaseBuilder = new TestCaseBuilder();
        return mTestCaseBuilder;
    }

    /**
     * Defines a group of CanvasClients, XML layouts, and WebView html files for testing.
     */
    protected class TestCaseBuilder {
        private List<TestCase> mTestCases;

        private TestCaseBuilder() {
            mTestCases = new ArrayList<>();
        }

        /**
         * Runs a test where the first test case is considered the "ideal" image and from there,
         * every test case is tested against it.
         */
        public void runWithComparer(BitmapComparer bitmapComparer) {
            if (mTestCases.size() == 0) {
                throw new IllegalStateException("Need at least one test to run");
            }

            Bitmap idealBitmap = captureRenderSpec(mTestCases.remove(0));

            for (TestCase testCase : mTestCases) {
                Bitmap testCaseBitmap = captureRenderSpec(testCase);
                assertBitmapsAreSimilar(idealBitmap, testCaseBitmap, bitmapComparer,
                        testCase.getDebugString());
            }
        }

        /**
         * Runs a test where each testcase is independent of the others and each is checked against
         * the verifier given.
         */
        public void runWithVerifier(BitmapVerifier bitmapVerifier) {
            if (mTestCases.size() == 0) {
                throw new IllegalStateException("Need at least one test to run");
            }

            for (TestCase testCase : mTestCases) {
                Bitmap testCaseBitmap = captureRenderSpec(testCase);
                assertBitmapIsVerified(testCaseBitmap, bitmapVerifier, testCase.getDebugString());
            }
        }

        private static final int VERIFY_ANIMATION_LOOP_COUNT = 20;
        private static final int VERIFY_ANIMATION_SLEEP_MS = 100;

        /**
         * Runs a test where each testcase is independent of the others and each is checked against
         * the verifier given in a loop.
         *
         * A screenshot is captured several times in a loop, to ensure that valid output is produced
         * at many different times during the animation.
         */
        public void runWithAnimationVerifier(BitmapVerifier bitmapVerifier) {
            if (mTestCases.size() == 0) {
                throw new IllegalStateException("Need at least one test to run");
            }

            for (TestCase testCase : mTestCases) {
                Point testOffset = runRenderSpec(testCase);

                for (int i = 0; i < VERIFY_ANIMATION_LOOP_COUNT; i++) {
                    try {
                        Thread.sleep(VERIFY_ANIMATION_SLEEP_MS);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    Bitmap testCaseBitmap = takeScreenshot(testOffset);
                    assertBitmapIsVerified(testCaseBitmap, bitmapVerifier,
                            testCase.getDebugString());
                }
            }
        }

        /**
         * Runs a test where each testcase is run without verification. Should only be used
         * where custom CanvasClients, Views, or ViewInitializers do their own internal
         * test assertions.
         */
        public void runWithoutVerification() {
            runWithVerifier(new BitmapVerifier() {
                @Override
                public boolean verify(int[] bitmap, int offset, int stride, int width, int height) {
                    return true;
                }
            });
        }

        public TestCaseBuilder addLayout(int layoutId, @Nullable ViewInitializer viewInitializer) {
            return addLayout(layoutId, viewInitializer, false)
                    .addLayout(layoutId, viewInitializer, true);
        }

        public TestCaseBuilder addLayout(int layoutId, @Nullable ViewInitializer viewInitializer,
                                         boolean useHardware) {
            mTestCases.add(new TestCase(layoutId, viewInitializer, useHardware));
            return this;
        }

        public TestCaseBuilder addLayout(int layoutId, @Nullable ViewInitializer viewInitializer,
                boolean useHardware, CountDownLatch readyFence) {
            TestCase test = new TestCase(layoutId, viewInitializer, useHardware);
            test.readyFence = readyFence;
            mTestCases.add(test);
            return this;
        }

        public TestCaseBuilder addCanvasClient(CanvasClient canvasClient) {
            return addCanvasClient(null, canvasClient);
        }

        public TestCaseBuilder addCanvasClient(CanvasClient canvasClient, boolean useHardware) {
            return addCanvasClient(null, canvasClient, useHardware);
        }

        public TestCaseBuilder addCanvasClient(String debugString, CanvasClient canvasClient) {
            return addCanvasClient(debugString, canvasClient, false)
                    .addCanvasClient(debugString, canvasClient, true);
        }

        public TestCaseBuilder addCanvasClient(String debugString,
                    CanvasClient canvasClient, boolean useHardware) {
            mTestCases.add(new TestCase(canvasClient, debugString, useHardware));
            return this;
        }

        private List<TestCase> getTestCases() {
            return mTestCases;
        }
    }

    private class TestCase {
        public int layoutID;
        public ViewInitializer viewInitializer;
        /** After launching the test case this fence is used to signal when
         * to proceed with capture & verification. If this is null the test
         * proceeds immediately to verification */
        @Nullable
        public CountDownLatch readyFence;

        public CanvasClient canvasClient;
        public String canvasClientDebugString;

        public boolean useHardware;
        public boolean wasTestRan = false;

        public TestCase(int layoutId, ViewInitializer viewInitializer, boolean useHardware) {
            this.layoutID = layoutId;
            this.viewInitializer = viewInitializer;
            this.useHardware = useHardware;
        }

        public TestCase(CanvasClient client, String debugString, boolean useHardware) {
            this.canvasClient = client;
            this.canvasClientDebugString = debugString;
            this.useHardware = useHardware;
        }

        public String getDebugString() {
            String debug = "";
            if (canvasClient != null) {
                debug += "CanvasClient : ";
                if (canvasClientDebugString != null) {
                    debug += canvasClientDebugString;
                } else {
                    debug += "no debug string given";
                }
            } else {
                debug += "Layout resource : " +
                        getActivity().getResources().getResourceName(layoutID);
            }
            debug += "\nTest ran in " + (useHardware ? "hardware" : "software") + "\n";
            return debug;
        }
    }
}
