/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
package android.server.cts;

public class ActivityManagerAppConfigurationTests extends ActivityManagerTestBase {
    private static final String TEST_ACTIVITY_NAME = "ResizeableActivity";

    /**
     * Tests that the WindowManager#getDefaultDisplay() and the Configuration of the Activity
     * has an updated size when the Activity is resized from fullscreen to docked state.
     *
     * The Activity handles configuration changes, so it will not be restarted between resizes.
     * On Configuration changes, the Activity logs the Display size and Configuration width
     * and heights. The values reported in fullscreen should be larger than those reported in
     * docked state.
     */
    public void testConfigurationUpdatesWhenResizedFromFullscreen() throws Exception {
        launchActivityInStack(TEST_ACTIVITY_NAME, FULLSCREEN_WORKSPACE_STACK_ID);
        final ReportedSizes fullscreenSizes = getActivityDisplaySize(TEST_ACTIVITY_NAME,
                FULLSCREEN_WORKSPACE_STACK_ID);

        moveActivityToStack(TEST_ACTIVITY_NAME, DOCKED_STACK_ID);
        final ReportedSizes dockedSizes = getActivityDisplaySize(TEST_ACTIVITY_NAME,
                DOCKED_STACK_ID);

        assertSizesAreSane(fullscreenSizes, dockedSizes);
    }

    /**
     * Same as {@link #testConfigurationUpdatesWhenResizedFromFullscreen()} but resizing
     * from docked state to fullscreen (reverse).
     */
    public void testConfigurationUpdatesWhenResizedFromDockedStack() throws Exception {
        launchActivityInStack(TEST_ACTIVITY_NAME, DOCKED_STACK_ID);
        final ReportedSizes dockedSizes = getActivityDisplaySize(TEST_ACTIVITY_NAME,
                DOCKED_STACK_ID);

        moveActivityToStack(TEST_ACTIVITY_NAME, FULLSCREEN_WORKSPACE_STACK_ID);
        final ReportedSizes fullscreenSizes = getActivityDisplaySize(TEST_ACTIVITY_NAME,
                FULLSCREEN_WORKSPACE_STACK_ID);

        assertSizesAreSane(fullscreenSizes, dockedSizes);
    }

    /**
     * Tests whether the Display sizes change when rotating the device.
     */
    public void testConfigurationUpdatesWhenRotatingWhileFullscreen() throws Exception {
        setDeviceRotation(0);
        launchActivityInStack(TEST_ACTIVITY_NAME, FULLSCREEN_WORKSPACE_STACK_ID);
        final ReportedSizes orientationASizes = getActivityDisplaySize(TEST_ACTIVITY_NAME,
                FULLSCREEN_WORKSPACE_STACK_ID);

        setDeviceRotation(1);
        final ReportedSizes orientationBSizes = getActivityDisplaySize(TEST_ACTIVITY_NAME,
                FULLSCREEN_WORKSPACE_STACK_ID);
        assertSizesRotate(orientationASizes, orientationBSizes);
    }


    /**
     * Same as {@link #testConfigurationUpdatesWhenRotatingWhileFullscreen()} but when the Activity
     * is in the docked stack.
     */
    public void testConfigurationUpdatesWhenRotatingWhileDocked() throws Exception {
        setDeviceRotation(0);
        launchActivityInStack(TEST_ACTIVITY_NAME, DOCKED_STACK_ID);
        final ReportedSizes orientationASizes = getActivityDisplaySize(TEST_ACTIVITY_NAME,
                DOCKED_STACK_ID);

        setDeviceRotation(1);
        final ReportedSizes orientationBSizes = getActivityDisplaySize(TEST_ACTIVITY_NAME,
                DOCKED_STACK_ID);
        assertSizesRotate(orientationASizes, orientationBSizes);
    }

    /**
     * Asserts that after rotation, the aspect ratios of display size, metrics, and configuration
     * have flipped.
     */
    private static void assertSizesRotate(ReportedSizes rotationA, ReportedSizes rotationB)
            throws Exception {
        assertEquals(rotationA.displayWidth, rotationA.metricsWidth);
        assertEquals(rotationA.displayHeight, rotationA.metricsHeight);
        assertEquals(rotationB.displayWidth, rotationB.metricsWidth);
        assertEquals(rotationB.displayHeight, rotationB.metricsHeight);

        final boolean beforePortrait = rotationA.displayWidth < rotationA.displayHeight;
        final boolean afterPortrait = rotationB.displayWidth < rotationB.displayHeight;
        assertFalse(beforePortrait == afterPortrait);

        final boolean beforeConfigPortrait = rotationA.widthDp < rotationA.heightDp;
        final boolean afterConfigPortrait = rotationB.widthDp < rotationB.heightDp;
        assertEquals(beforePortrait, beforeConfigPortrait);
        assertEquals(afterPortrait, afterConfigPortrait);
    }

    /**
     * Throws an AssertionError if fullscreenSizes has widths/heights (depending on aspect ratio)
     * that are smaller than the dockedSizes.
     */
    private static void assertSizesAreSane(ReportedSizes fullscreenSizes, ReportedSizes dockedSizes)
            throws Exception {
        final boolean portrait = fullscreenSizes.displayWidth < fullscreenSizes.displayHeight;
        if (portrait) {
            assertTrue(dockedSizes.displayHeight < fullscreenSizes.displayHeight);
            assertTrue(dockedSizes.heightDp < fullscreenSizes.heightDp);
            assertTrue(dockedSizes.metricsHeight < fullscreenSizes.metricsHeight);
        } else {
            assertTrue(dockedSizes.displayWidth < fullscreenSizes.displayWidth);
            assertTrue(dockedSizes.widthDp < fullscreenSizes.widthDp);
            assertTrue(dockedSizes.metricsWidth < fullscreenSizes.metricsWidth);
        }
    }

    private ReportedSizes getActivityDisplaySize(String activityName, int stackId)
            throws Exception {
        mAmWmState.computeState(mDevice, new String[] { activityName },
                false /* compareTaskAndStackBounds */);
        mAmWmState.assertContainsStack("Must contain stack " + stackId, stackId);
        final ReportedSizes details = getLastReportedSizesForActivity(activityName);
        assertNotNull(details);
        return details;
    }
}
