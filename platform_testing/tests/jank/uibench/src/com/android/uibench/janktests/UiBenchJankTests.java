/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.uibench.janktests;

import static com.android.uibench.janktests.UiBenchJankTestsHelper.EXPECTED_FRAMES;
import static com.android.uibench.janktests.UiBenchJankTestsHelper.PACKAGE_NAME;

import android.os.SystemClock;
import android.support.test.jank.GfxMonitor;
import android.support.test.jank.JankTest;
import android.support.test.jank.JankTestBase;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.Until;
import android.widget.ListView;
import junit.framework.Assert;

/**
 * Jank benchmark General tests for UiBench app
 */

public class UiBenchJankTests extends JankTestBase {

    private UiDevice mDevice;
    private UiBenchJankTestsHelper mHelper;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mDevice.setOrientationNatural();
        mHelper = UiBenchJankTestsHelper.getInstance(
                this.getInstrumentation().getContext(), mDevice);
    }

    @Override
    protected void tearDown() throws Exception {
        mDevice.unfreezeRotation();
        super.tearDown();
    }

    // Open dialog list from General
    public void openDialogList() {
        mHelper.launchActivity("DialogListActivity", "Dialog");
        mHelper.mContents = mDevice.wait(Until.findObject(
                By.clazz(ListView.class)), mHelper.TIMEOUT);
        Assert.assertNotNull("Dialog List View isn't found", mHelper.mContents);
    }

    // Test dialoglist fling
    @JankTest(beforeTest = "openDialogList", expectedFrames = EXPECTED_FRAMES)
    @GfxMonitor(processName = PACKAGE_NAME)
    public void testDialogListFling() {
        mHelper.flingUpDown(mHelper.mContents, mHelper.SHORT_TIMEOUT, 1);
    }

    // Open Fullscreen Overdraw from General
    public void openFullscreenOverdraw() {
        mHelper.launchActivity("FullscreenOverdrawActivity",
                "General/Fullscreen Overdraw");
    }

    // Measure fullscreen overdraw jank
    @JankTest(beforeTest = "openFullscreenOverdraw", expectedFrames = EXPECTED_FRAMES)
    @GfxMonitor(processName = PACKAGE_NAME)
    public void testFullscreenOverdraw() {
        SystemClock.sleep(mHelper.LONG_TIMEOUT * 5);
    }

    // Open GL TextureView from General
    public void openGLTextureView() {
        mHelper.launchActivity("GlTextureViewActivity",
                "General/GL TextureView");
    }

    // Measure GL TextureView jank metrics
    @JankTest(beforeTest = "openGLTextureView", expectedFrames = EXPECTED_FRAMES)
    @GfxMonitor(processName = PACKAGE_NAME)
    public void testGLTextureView() {
        SystemClock.sleep(mHelper.LONG_TIMEOUT * 5);
    }

    // Open Invalidate from General
    public void openInvalidate() {
        mHelper.launchActivity("InvalidateActivity",
                "General/Invalidate");
    }

    // Measure Invalidate jank metrics
    @JankTest(beforeTest = "openInvalidate", expectedFrames = EXPECTED_FRAMES)
    @GfxMonitor(processName = PACKAGE_NAME)
    public void testInvalidate() {
        SystemClock.sleep(mHelper.LONG_TIMEOUT * 5);
    }

    // Open Trivial Animation from General
    public void openTrivialAnimation() {
        mHelper.launchActivity("TrivialAnimationActivity",
                "General/Trivial Animation");
    }

    // Measure TrivialAnimation jank metrics
    @JankTest(beforeTest = "openTrivialAnimation", expectedFrames = EXPECTED_FRAMES)
    @GfxMonitor(processName = PACKAGE_NAME)
    public void testTrivialAnimation() {
        SystemClock.sleep(mHelper.LONG_TIMEOUT * 5);
    }

    // Open Trivial listview from General
    public void openTrivialListView() {
        mHelper.launchActivity("TrivialListActivity",
                "General/Trivial ListView");
        mHelper.mContents = mDevice.wait(Until.findObject(
                By.res("android", "content")), mHelper.TIMEOUT);
        Assert.assertNotNull("Trivial ListView isn't found in General", mHelper.mContents);
    }

    // Test trivialListView fling
    @JankTest(beforeTest = "openTrivialListView", expectedFrames = EXPECTED_FRAMES)
    @GfxMonitor(processName = PACKAGE_NAME)
    public void testTrivialListViewFling() {
        mHelper.flingUpDown(mHelper.mContents, mHelper.SHORT_TIMEOUT, 2);
    }

    // Open Trivial Recycler List View from General
    public void openTrivialRecyclerListView() {
        mHelper.launchActivity("TrivialRecyclerViewActivity",
                "General/Trivial Recycler ListView");
        mHelper.mContents = mDevice.wait(Until.findObject(
                By.res("android", "content")), mHelper.TIMEOUT);
        Assert.assertNotNull("Trivial Recycler ListView isn't found in General",
                mHelper.mContents);
    }

    // Test trivialRecyclerListView fling
    @JankTest(beforeTest = "openTrivialRecyclerListView", expectedFrames = EXPECTED_FRAMES)
    @GfxMonitor(processName = PACKAGE_NAME)
    public void testTrivialRecyclerListViewFling() {
        mHelper.flingUpDown(mHelper.mContents, mHelper.SHORT_TIMEOUT, 2);
    }

    // Open Inflation Listview contents
    public void openInflatingListView() {
        mHelper.launchActivity("InflatingListActivity",
                "Inflation/Inflating ListView");
        mHelper.mContents = mDevice.wait(Until.findObject(
                By.res("android", "content")), mHelper.TIMEOUT);
        Assert.assertNotNull("Inflating ListView isn't found in Inflation",
                mHelper.mContents);
    }

    // Test Inflating List View fling
    @JankTest(beforeTest = "openInflatingListView", expectedFrames = EXPECTED_FRAMES)
    @GfxMonitor(processName = PACKAGE_NAME)
    public void testInflatingListViewFling() {
        mHelper.flingUpDown(mHelper.mContents, mHelper.SHORT_TIMEOUT, 2);
    }

}
