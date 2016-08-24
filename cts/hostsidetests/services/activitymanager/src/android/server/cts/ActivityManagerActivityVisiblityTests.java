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
 * limitations under the License
 */

package android.server.cts;

import com.android.tradefed.device.DeviceNotAvailableException;

import java.lang.Exception;
import java.lang.String;

public class ActivityManagerActivityVisiblityTests extends ActivityManagerTestBase {
    private static final String TRANSLUCENT_ACTIVITY = "AlwaysFocusablePipActivity";
    private static final String VISIBLE_BEHIND_ACTIVITY = "VisibleBehindActivity";
    private static final String PIP_ON_PIP_ACTIVITY = "LaunchPipOnPipActivity";
    private static final String TEST_ACTIVITY_NAME = "TestActivity";
    private static final String TRANSLUCENT_ACTIVITY_NAME = "TranslucentActivity";
    private static final String DOCKED_ACTIVITY_NAME = "DockedActivity";
    private static final String TURN_SCREEN_ON_ACTIVITY_NAME = "TurnScreenOnActivity";

    public void testVisibleBehindHomeActivity() throws Exception {
        executeShellCommand(getAmStartCmd(VISIBLE_BEHIND_ACTIVITY));
        mAmWmState.waitForValidState(mDevice, true, new String[] {VISIBLE_BEHIND_ACTIVITY},
                new int[] {FULLSCREEN_WORKSPACE_STACK_ID}, false /* compareTaskAndStackBounds */);

        executeShellCommand(AM_START_HOME_ACTIVITY_COMMAND);
        mAmWmState.waitForHomeActivityVisible(mDevice);

        mAmWmState.computeState(mDevice, null);
        mAmWmState.assertContainsStack(
                "Must contain fullscreen stack.", FULLSCREEN_WORKSPACE_STACK_ID);
        mAmWmState.assertFrontStack("Home stack must be the front stack.", HOME_STACK_ID);
        mAmWmState.assertVisibility(
                VISIBLE_BEHIND_ACTIVITY, hasDeviceFeature("android.software.leanback"));
    }

    public void testVisibleBehindOtherActivity_NotOverHome() throws Exception {
        executeShellCommand(getAmStartCmd(VISIBLE_BEHIND_ACTIVITY));
        executeShellCommand(getAmStartCmd(TRANSLUCENT_ACTIVITY));

        mAmWmState.computeState(mDevice,
                new String[] {VISIBLE_BEHIND_ACTIVITY, TRANSLUCENT_ACTIVITY});
        mAmWmState.assertVisibility(VISIBLE_BEHIND_ACTIVITY, true);
        mAmWmState.assertVisibility(TRANSLUCENT_ACTIVITY, true);
    }

    public void testVisibleBehindOtherActivity_OverHome() throws Exception {
        executeShellCommand(getAmStartCmdOverHome(VISIBLE_BEHIND_ACTIVITY));
        executeShellCommand(getAmStartCmdOverHome(TRANSLUCENT_ACTIVITY));

        mAmWmState.computeState(mDevice,
                new String[] {VISIBLE_BEHIND_ACTIVITY, TRANSLUCENT_ACTIVITY});
        mAmWmState.assertVisibility(VISIBLE_BEHIND_ACTIVITY, true);
        mAmWmState.assertVisibility(TRANSLUCENT_ACTIVITY, true);
    }

    public void testTranslucentActivityOnTopOfPinnedStack() throws Exception {
        if (!supportsPip()) {
            return;
        }

        executeShellCommand(getAmStartCmdOverHome(PIP_ON_PIP_ACTIVITY));
        // NOTE: moving to pinned stack will trigger the pip-on-pip activity to launch the
        // translucent activity.
        executeShellCommand(AM_MOVE_TOP_ACTIVITY_TO_PINNED_STACK_COMMAND);

        mAmWmState.computeState(mDevice, new String[] {PIP_ON_PIP_ACTIVITY, TRANSLUCENT_ACTIVITY});
        mAmWmState.assertFrontStack("Pinned stack must be the front stack.", PINNED_STACK_ID);
        mAmWmState.assertVisibility(PIP_ON_PIP_ACTIVITY, true);
        mAmWmState.assertVisibility(TRANSLUCENT_ACTIVITY, true);
    }

    /**
     * Asserts that the home activity is visible when a translucent activity is launched in the
     * fullscreen stack over the home activity.
     */
    public void testTranslucentActivityOnTopOfHome() throws Exception {
        executeShellCommand(AM_START_HOME_ACTIVITY_COMMAND);
        executeShellCommand(getAmStartCmd(TRANSLUCENT_ACTIVITY));

        mAmWmState.computeState(mDevice, new String[]{TRANSLUCENT_ACTIVITY});
        mAmWmState.assertSanity();
        mAmWmState.assertFrontStack(
                "Fullscreen stack must be the front stack.", FULLSCREEN_WORKSPACE_STACK_ID);
        mAmWmState.assertVisibility(TRANSLUCENT_ACTIVITY, true);
        mAmWmState.assertHomeActivityVisible(true);
    }

    /**
     * Assert that the home activity is visible if a task that was launched from home is pinned
     * and also assert the next task in the fullscreen stack isn't visible.
     */
    public void testHomeVisibleOnActivityTaskPinned() throws Exception {
        if (!supportsPip()) {
            return;
        }

        executeShellCommand(AM_START_HOME_ACTIVITY_COMMAND);
        executeShellCommand(getAmStartCmd(TEST_ACTIVITY_NAME));
        executeShellCommand(AM_START_HOME_ACTIVITY_COMMAND);
        executeShellCommand(getAmStartCmd(TRANSLUCENT_ACTIVITY));
        executeShellCommand(AM_MOVE_TOP_ACTIVITY_TO_PINNED_STACK_COMMAND);

        mAmWmState.computeState(mDevice, new String[]{TRANSLUCENT_ACTIVITY});
        mAmWmState.assertSanity();

        mAmWmState.assertVisibility(TRANSLUCENT_ACTIVITY, true);
        mAmWmState.assertVisibility(TEST_ACTIVITY_NAME, false);
        mAmWmState.assertHomeActivityVisible(true);
    }

    public void testTranslucentActivityOverDockedStack() throws Exception {
        launchActivityInDockStack(DOCKED_ACTIVITY_NAME);
        launchActivityInStack(TEST_ACTIVITY_NAME, FULLSCREEN_WORKSPACE_STACK_ID);
        launchActivityInStack(TRANSLUCENT_ACTIVITY_NAME, DOCKED_STACK_ID);
        mAmWmState.computeState(mDevice, new String[] {TEST_ACTIVITY_NAME, DOCKED_ACTIVITY_NAME,
                TRANSLUCENT_ACTIVITY_NAME}, false /* compareTaskAndStackBounds */);
        mAmWmState.assertContainsStack("Must contain docked stack", DOCKED_STACK_ID);
        mAmWmState.assertContainsStack("Must contain fullscreen stack",
                FULLSCREEN_WORKSPACE_STACK_ID);
        mAmWmState.assertVisibility(DOCKED_ACTIVITY_NAME, true);
        mAmWmState.assertVisibility(TEST_ACTIVITY_NAME, true);
        mAmWmState.assertVisibility(TRANSLUCENT_ACTIVITY_NAME, true);
    }

    public void testTurnScreenOnActivity() throws Exception {
        lockDevice();
        executeShellCommand(getAmStartCmd(TURN_SCREEN_ON_ACTIVITY_NAME));
        mAmWmState.computeState(mDevice, new String[] { TURN_SCREEN_ON_ACTIVITY_NAME });
        mAmWmState.assertSanity();
        mAmWmState.assertVisibility(TURN_SCREEN_ON_ACTIVITY_NAME, true);
    }
}
