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

import java.awt.Rectangle;
import java.util.ArrayList;

public class ActivityManagerFreeformStackTests extends ActivityManagerTestBase {

    private static final String TEST_ACTIVITY = "TestActivity";
    private static final int TEST_TASK_OFFSET = 20;
    private static final int TEST_TASK_OFFSET_2 = 100;
    private static final int TEST_TASK_SIZE_1 = 500;
    private static final int TEST_TASK_SIZE_2 = TEST_TASK_SIZE_1 * 2;
    // NOTE: Launching the FreeformActivity will automatically launch the TestActivity
    // with bounds (0, 0, 500, 500)
    private static final String FREEFORM_ACTIVITY = "FreeformActivity";
    private static final String NON_RESIZEABLE_ACTIVITY = "NonResizeableActivity";
    private static final String NO_RELAUNCH_ACTIVITY = "NoRelaunchActivity";

    public void testFreeformWindowManagementSupport() throws Exception {

        launchActivityInStack(FREEFORM_ACTIVITY, FREEFORM_WORKSPACE_STACK_ID);

        mAmWmState.computeState(mDevice, new String[] {FREEFORM_ACTIVITY, TEST_ACTIVITY});

        if (!supportsFreeform()) {
            mAmWmState.assertDoesNotContainStack(
                    "Must not contain freeform stack.", FREEFORM_WORKSPACE_STACK_ID);
            return;
        }

        mAmWmState.assertFrontStack(
                "Freeform stack must be the front stack.", FREEFORM_WORKSPACE_STACK_ID);
        mAmWmState.assertVisibility(FREEFORM_ACTIVITY, true);
        mAmWmState.assertVisibility(TEST_ACTIVITY, true);
        mAmWmState.assertFocusedActivity(
                TEST_ACTIVITY + " must be focused Activity", TEST_ACTIVITY);
        assertEquals(new Rectangle(0, 0, TEST_TASK_SIZE_1, TEST_TASK_SIZE_1),
                mAmWmState.getAmState().getTaskByActivityName(TEST_ACTIVITY).getBounds());
    }

    public void testNonResizeableActivityNotLaunchedToFreeform() throws Exception {
        launchActivityInStack(NON_RESIZEABLE_ACTIVITY, FREEFORM_WORKSPACE_STACK_ID);

        mAmWmState.computeState(mDevice, new String[] {NON_RESIZEABLE_ACTIVITY});

        mAmWmState.assertFrontStack(
                "Fullscreen stack must be the front stack.", FULLSCREEN_WORKSPACE_STACK_ID);
        mAmWmState.assertDoesNotContainStack(
                "Must not contain freeform stack.", FREEFORM_WORKSPACE_STACK_ID);
    }

    public void testActivityLifeCycleOnResizeFreeformTask() throws Exception {
        launchActivityInStack(TEST_ACTIVITY, FREEFORM_WORKSPACE_STACK_ID);
        launchActivityInStack(NO_RELAUNCH_ACTIVITY, FREEFORM_WORKSPACE_STACK_ID);

        mAmWmState.computeState(mDevice, new String[]{TEST_ACTIVITY, NO_RELAUNCH_ACTIVITY});

        if (!supportsFreeform()) {
            mAmWmState.assertDoesNotContainStack(
                    "Must not contain freeform stack.", FREEFORM_WORKSPACE_STACK_ID);
            return;
        }

        resizeActivityTask(TEST_ACTIVITY,
                TEST_TASK_OFFSET, TEST_TASK_OFFSET, TEST_TASK_SIZE_1, TEST_TASK_SIZE_2);
        resizeActivityTask(NO_RELAUNCH_ACTIVITY,
                TEST_TASK_OFFSET_2, TEST_TASK_OFFSET_2, TEST_TASK_SIZE_1, TEST_TASK_SIZE_2);

        mAmWmState.computeState(mDevice, new String[]{TEST_ACTIVITY, NO_RELAUNCH_ACTIVITY});

        clearLogcat();
        resizeActivityTask(TEST_ACTIVITY,
                TEST_TASK_OFFSET, TEST_TASK_OFFSET, TEST_TASK_SIZE_2, TEST_TASK_SIZE_1);
        resizeActivityTask(NO_RELAUNCH_ACTIVITY,
                TEST_TASK_OFFSET_2, TEST_TASK_OFFSET_2, TEST_TASK_SIZE_2, TEST_TASK_SIZE_1);
        mAmWmState.computeState(mDevice, new String[]{TEST_ACTIVITY, NO_RELAUNCH_ACTIVITY});

        assertActivityLifecycle(TEST_ACTIVITY, true);
        assertActivityLifecycle(NO_RELAUNCH_ACTIVITY, false);
    }
}
