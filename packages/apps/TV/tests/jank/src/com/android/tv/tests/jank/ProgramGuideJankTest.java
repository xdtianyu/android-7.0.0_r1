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
package com.android.tv.tests.jank;

import static com.android.tv.testing.uihelper.UiDeviceAsserts.assertWaitForCondition;

import android.content.res.Resources;
import android.os.Build;
import android.support.test.filters.SdkSuppress;
import android.support.test.jank.GfxMonitor;
import android.support.test.jank.JankTest;
import android.support.test.jank.JankTestBase;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.Until;
import android.test.suitebuilder.annotation.MediumTest;

import com.android.tv.R;
import com.android.tv.testing.uihelper.ByResource;
import com.android.tv.testing.uihelper.Constants;
import com.android.tv.testing.uihelper.LiveChannelsUiDeviceHelper;
import com.android.tv.testing.uihelper.MenuHelper;
import com.android.tv.testing.uihelper.UiDeviceUtils;

/**
 * Jank tests for the program guide.
 */
@MediumTest
@SdkSuppress(minSdkVersion = Build.VERSION_CODES.M)
public class ProgramGuideJankTest extends JankTestBase {
    private static final boolean DEBUG = false;
    private static final String TAG = "ProgramGuideJank";

    private static final String STARTING_CHANNEL = "13";
    public static final String LIVE_CHANNELS_PROCESS_NAME = "com.android.tv";

    /**
     * The minimum number of frames expected during each jank test.
     * If there is less the test will fail.  To be safe we loop the action in each test to create
     * twice this many frames under normal conditions.
     * <p>200 is chosen so there will be enough frame for the 90th, 95th, and 98th percentile
     * measurements are significant.
     *
     * @see <a href="http://go/janktesthelper-best-practices">Jank Test Helper Best Practices</a>
     */
    private static final int EXPECTED_FRAMES = 200;
    public static final String LIVE_CHANNELS_PACKAGE = "com.android.tv";

    protected UiDevice mDevice;

    protected Resources mTargetResources;
    protected MenuHelper mMenuHelper;
    protected LiveChannelsUiDeviceHelper mLiveChannelsHelper;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mTargetResources = getInstrumentation().getTargetContext().getResources();
        mMenuHelper = new MenuHelper(mDevice, mTargetResources);
        mLiveChannelsHelper = new LiveChannelsUiDeviceHelper(mDevice, mTargetResources,
                getInstrumentation().getContext());
        mLiveChannelsHelper.assertAppStarted();
        pressKeysForChannelNumber(STARTING_CHANNEL);
    }


    @JankTest(expectedFrames = EXPECTED_FRAMES,
            beforeTest = "warmProgramGuide")
    @GfxMonitor(processName = LIVE_CHANNELS_PACKAGE)
    public void testShowClearProgramGuide() {
        int frames = 53; // measured by hand
        int repeat = EXPECTED_FRAMES * 2 / frames;
        for (int i = 0; i < repeat; i++) {
            showProgramGuide();
            clearProgramGuide();
        }
    }

    @JankTest(expectedFrames = EXPECTED_FRAMES,
            beforeLoop = "showProgramGuide",
            afterLoop = "clearProgramGuide")
    @GfxMonitor(processName = LIVE_CHANNELS_PROCESS_NAME)
    public void testScrollDown() {
        int frames = 20;  // measured by hand
        int repeat = EXPECTED_FRAMES * 2 / frames;
        for (int i = 0; i < repeat; i++) {
            mDevice.pressDPadDown();
        }
    }

    @JankTest(expectedFrames = EXPECTED_FRAMES,
            beforeLoop = "showProgramGuide",
            afterLoop = "clearProgramGuide")
    @GfxMonitor(processName = LIVE_CHANNELS_PROCESS_NAME)
    public void testScrollRight() {
        int frames = 30;  // measured by hand
        int repeat = EXPECTED_FRAMES * 2 / frames;
        for (int i = 0; i < repeat; i++) {
            mDevice.pressDPadRight();
        }
    }

    //TODO: move to a mixin/helper
    protected void pressKeysForChannelNumber(String channel) {
        UiDeviceUtils.pressKeys(mDevice, channel);
        mDevice.pressDPadCenter();
    }

    public void selectProgramGuideMenuItem() {
        mMenuHelper.showMenu();
        mMenuHelper.assertNavigateToMenuItem(R.string.menu_title_channels,
                R.string.channels_item_program_guide);
        mDevice.waitForIdle();
    }

    public void warmProgramGuide() {
        // TODO: b/21078199  First time Program Guide is opened there is a noticeable delay
        selectProgramGuideMenuItem();
        mDevice.pressDPadCenter();
        assertWaitForCondition(mDevice, Until.hasObject(Constants.PROGRAM_GUIDE));
        mDevice.pressBack();

    }

    public void clearProgramGuide() {
        mDevice.pressBack();
        assertWaitForCondition(mDevice, Until.gone(Constants.PROGRAM_GUIDE));
    }

    public void showProgramGuide() {
        selectProgramGuideMenuItem();
        mDevice.pressDPadCenter();
        assertWaitForCondition(mDevice, Until.hasObject(Constants.PROGRAM_GUIDE));
        // If the side panel grid is visible (and thus has focus), move right to clear it.
        if (mDevice.hasObject(
                ByResource.id(mTargetResources, R.id.program_guide_side_panel_grid_view))) {
            mDevice.pressDPadRight();
        }
    }
}
