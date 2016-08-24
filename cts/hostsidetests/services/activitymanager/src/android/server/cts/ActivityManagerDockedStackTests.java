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

package android.server.cts;

import java.awt.Rectangle;

public class ActivityManagerDockedStackTests extends ActivityManagerTestBase {

    private static final String TEST_ACTIVITY_NAME = "TestActivity";
    private static final String NON_RESIZEABLE_ACTIVITY_NAME = "NonResizeableActivity";
    private static final String DOCKED_ACTIVITY_NAME = "DockedActivity";
    private static final String LAUNCH_TO_SIDE_ACTIVITY_NAME = "LaunchToSideActivity";
    private static final String NO_RELAUNCH_ACTIVITY_NAME = "NoRelaunchActivity";
    private static final String SINGLE_INSTANCE_ACTIVITY_NAME = "SingleInstanceActivity";
    private static final String SINGLE_TASK_ACTIVITY_NAME = "SingleTaskActivity";

    private static final int TASK_SIZE = 600;
    private static final int STACK_SIZE = 300;

    public void testStackList() throws Exception {
        executeShellCommand(getAmStartCmd(TEST_ACTIVITY_NAME));
        mAmWmState.computeState(mDevice, new String[] {TEST_ACTIVITY_NAME});
        mAmWmState.assertContainsStack("Must contain home stack.", HOME_STACK_ID);
        mAmWmState.assertContainsStack(
                "Must contain fullscreen stack.", FULLSCREEN_WORKSPACE_STACK_ID);
        mAmWmState.assertDoesNotContainStack("Must not contain docked stack.", DOCKED_STACK_ID);
    }

    public void testDockActivity() throws Exception {
        launchActivityInDockStack(TEST_ACTIVITY_NAME);
        mAmWmState.computeState(mDevice, new String[] {TEST_ACTIVITY_NAME});
        mAmWmState.assertContainsStack("Must contain home stack.", HOME_STACK_ID);
        mAmWmState.assertContainsStack("Must contain docked stack.", DOCKED_STACK_ID);
    }

    public void testNonResizeableNotDocked() throws Exception {
        launchActivityInDockStack(NON_RESIZEABLE_ACTIVITY_NAME);
        mAmWmState.computeState(mDevice, new String[] {NON_RESIZEABLE_ACTIVITY_NAME});

        mAmWmState.assertContainsStack("Must contain home stack.", HOME_STACK_ID);
        mAmWmState.assertDoesNotContainStack("Must not contain docked stack.", DOCKED_STACK_ID);
        mAmWmState.assertFrontStack(
                "Fullscreen stack must be front stack.", FULLSCREEN_WORKSPACE_STACK_ID);
    }

    public void testLaunchToSide() throws Exception {
        launchActivityInDockStack(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        launchActivityToSide(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        mAmWmState.computeState(mDevice, new String[] {LAUNCH_TO_SIDE_ACTIVITY_NAME});

        mAmWmState.assertContainsStack(
                "Must contain fullscreen stack.", FULLSCREEN_WORKSPACE_STACK_ID);
        mAmWmState.assertContainsStack("Must contain docked stack.", DOCKED_STACK_ID);
    }

    public void testLaunchToSideAndBringToFront() throws Exception {
        launchActivityInDockStack(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        final String[] waitForFirstVisible = new String[] {TEST_ACTIVITY_NAME};
        final String[] waitForSecondVisible = new String[] {NO_RELAUNCH_ACTIVITY_NAME};

        // Launch activity to side.
        launchActivityToSide(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        mAmWmState.computeState(mDevice, waitForFirstVisible);
        int taskNumberInitial = mAmWmState.getAmState().getStackById(FULLSCREEN_WORKSPACE_STACK_ID)
                .getTasks().size();
        mAmWmState.assertFocusedActivity("Launched to side activity must be in front.",
                TEST_ACTIVITY_NAME);

        // Launch another activity to side to cover first one.
        launchActivityInStack(NO_RELAUNCH_ACTIVITY_NAME, FULLSCREEN_WORKSPACE_STACK_ID);
        mAmWmState.computeState(mDevice, waitForSecondVisible);
        int taskNumberCovered = mAmWmState.getAmState().getStackById(FULLSCREEN_WORKSPACE_STACK_ID)
                .getTasks().size();
        mAmWmState.assertEquals("Fullscreen stack must have one task added.",
                taskNumberInitial + 1, taskNumberCovered);
        mAmWmState.assertFocusedActivity("Launched to side covering activity must be in front.",
                NO_RELAUNCH_ACTIVITY_NAME);

        // Launch activity that was first launched to side. It should be brought to front.
        launchActivityToSide(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        mAmWmState.computeState(mDevice, waitForFirstVisible);
        int taskNumberFinal = mAmWmState.getAmState().getStackById(FULLSCREEN_WORKSPACE_STACK_ID)
                .getTasks().size();
        mAmWmState.assertEquals("Task number in fullscreen stack must remain the same.",
                taskNumberCovered, taskNumberFinal);
        mAmWmState.assertFocusedActivity("Launched to side covering activity must be in front.",
                TEST_ACTIVITY_NAME);
    }

    public void testLaunchToSideMultiple() throws Exception {
        launchActivityInDockStack(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        final String[] waitForActivitiesVisible = new String[] {TEST_ACTIVITY_NAME};

        // Launch activity to side.
        launchActivityToSide(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        int taskNumberInitial = mAmWmState.getAmState().getStackById(FULLSCREEN_WORKSPACE_STACK_ID)
                .getTasks().size();
        mAmWmState.assertNotNull("Launched to side activity must be in fullscreen stack.",
                mAmWmState.getAmState()
                        .getTaskByActivityName(TEST_ACTIVITY_NAME, FULLSCREEN_WORKSPACE_STACK_ID));

        // Try to launch to side same activity again.
        launchActivityToSide(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        int taskNumberFinal = mAmWmState.getAmState().getStackById(FULLSCREEN_WORKSPACE_STACK_ID)
                .getTasks().size();
        mAmWmState.assertEquals("Task number mustn't change.", taskNumberInitial, taskNumberFinal);
        mAmWmState.assertFocusedActivity("Launched to side activity must remain in front.",
                TEST_ACTIVITY_NAME);
        mAmWmState.assertNotNull("Launched to side activity must remain in fullscreen stack.",
                mAmWmState.getAmState()
                        .getTaskByActivityName(TEST_ACTIVITY_NAME, FULLSCREEN_WORKSPACE_STACK_ID));
    }

    public void testLaunchToSideSingleInstance() throws Exception {
        launchTargetToSide(SINGLE_INSTANCE_ACTIVITY_NAME, false);
    }

    public void testLaunchToSideSingleTask() throws Exception {
        launchTargetToSide(SINGLE_TASK_ACTIVITY_NAME, false);
    }

    public void testLaunchToSideMultipleWithDifferentIntent() throws Exception {
        launchTargetToSide(TEST_ACTIVITY_NAME, true);
    }

    private void launchTargetToSide(String targetActivityName,
                                    boolean taskCountMustIncrement) throws Exception {
        launchActivityInDockStack(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        final String[] waitForActivitiesVisible = new String[] {targetActivityName};

        // Launch activity to side with data.
        launchActivityToSide(LAUNCH_TO_SIDE_ACTIVITY_NAME, true, false, targetActivityName);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        int taskNumberInitial = mAmWmState.getAmState().getStackById(FULLSCREEN_WORKSPACE_STACK_ID)
                .getTasks().size();
        mAmWmState.assertNotNull("Launched to side activity must be in fullscreen stack.",
                mAmWmState.getAmState()
                        .getTaskByActivityName(targetActivityName, FULLSCREEN_WORKSPACE_STACK_ID));

        // Try to launch to side same activity again with different data.
        launchActivityToSide(LAUNCH_TO_SIDE_ACTIVITY_NAME, true, false, targetActivityName);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        int taskNumberSecondLaunch = mAmWmState.getAmState()
                .getStackById(FULLSCREEN_WORKSPACE_STACK_ID).getTasks().size();
        if (taskCountMustIncrement) {
            mAmWmState.assertEquals("Task number must be incremented.", taskNumberInitial + 1,
                    taskNumberSecondLaunch);
        } else {
            mAmWmState.assertEquals("Task number must not change.", taskNumberInitial,
                    taskNumberSecondLaunch);
        }
        mAmWmState.assertFocusedActivity("Launched to side activity must be in front.",
                targetActivityName);
        mAmWmState.assertNotNull("Launched to side activity must be launched in fullscreen stack.",
                mAmWmState.getAmState()
                        .getTaskByActivityName(targetActivityName, FULLSCREEN_WORKSPACE_STACK_ID));

        // Try to launch to side same activity again with no data.
        launchActivityToSide(LAUNCH_TO_SIDE_ACTIVITY_NAME, false, false, targetActivityName);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        int taskNumberFinal = mAmWmState.getAmState().getStackById(FULLSCREEN_WORKSPACE_STACK_ID)
                .getTasks().size();
        if (taskCountMustIncrement) {
            mAmWmState.assertEquals("Task number must be incremented.", taskNumberSecondLaunch + 1,
                    taskNumberFinal);
        } else {
            mAmWmState.assertEquals("Task number must not change.", taskNumberSecondLaunch,
                    taskNumberFinal);
        }
        mAmWmState.assertFocusedActivity("Launched to side activity must be in front.",
                targetActivityName);
        mAmWmState.assertNotNull("Launched to side activity must be launched in fullscreen stack.",
                mAmWmState.getAmState()
                        .getTaskByActivityName(targetActivityName, FULLSCREEN_WORKSPACE_STACK_ID));
    }

    public void testLaunchToSideMultipleWithFlag() throws Exception {
        launchActivityInDockStack(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        final String[] waitForActivitiesVisible = new String[] {TEST_ACTIVITY_NAME};

        // Launch activity to side.
        launchActivityToSide(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        int taskNumberInitial = mAmWmState.getAmState().getStackById(FULLSCREEN_WORKSPACE_STACK_ID)
                .getTasks().size();
        mAmWmState.assertNotNull("Launched to side activity must be in fullscreen stack.",
                mAmWmState.getAmState()
                        .getTaskByActivityName(TEST_ACTIVITY_NAME, FULLSCREEN_WORKSPACE_STACK_ID));

        // Try to launch to side same activity again, but with Intent#FLAG_ACTIVITY_MULTIPLE_TASK.
        launchActivityToSide(LAUNCH_TO_SIDE_ACTIVITY_NAME, false, true);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        int taskNumberFinal = mAmWmState.getAmState().getStackById(FULLSCREEN_WORKSPACE_STACK_ID)
                .getTasks().size();
        mAmWmState.assertEquals("Task number must be incremented.", taskNumberInitial + 1,
                taskNumberFinal);
        mAmWmState.assertFocusedActivity("Launched to side activity must be in front.",
                TEST_ACTIVITY_NAME);
        mAmWmState.assertNotNull("Launched to side activity must remain in fullscreen stack.",
                mAmWmState.getAmState()
                        .getTaskByActivityName(TEST_ACTIVITY_NAME, FULLSCREEN_WORKSPACE_STACK_ID));
    }

    public void testRotationWhenDocked() throws Exception {
        launchActivityInDockStack(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        launchActivityToSide(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        final String[] waitForActivitiesVisible = new String[] {LAUNCH_TO_SIDE_ACTIVITY_NAME};
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        mAmWmState.assertContainsStack(
                "Must contain fullscreen stack.", FULLSCREEN_WORKSPACE_STACK_ID);
        mAmWmState.assertContainsStack("Must contain docked stack.", DOCKED_STACK_ID);

        // Rotate device single steps (90°) 0-1-2-3.
        // Each time we compute the state we implicitly assert valid bounds.
        setDeviceRotation(0);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        setDeviceRotation(1);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        setDeviceRotation(2);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        setDeviceRotation(3);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        // Double steps (180°) We ended the single step at 3. So, we jump directly to 1 for double
        // step. So, we are testing 3-1-3 for one side and 0-2-0 for the other side.
        setDeviceRotation(1);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        setDeviceRotation(3);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        setDeviceRotation(0);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        setDeviceRotation(2);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        setDeviceRotation(0);
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
    }

    public void testRotationWhenDockedWhileLocked() throws Exception {
        launchActivityInDockStack(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        launchActivityToSide(LAUNCH_TO_SIDE_ACTIVITY_NAME);
        final String[] waitForActivitiesVisible = new String[] {LAUNCH_TO_SIDE_ACTIVITY_NAME};
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
        mAmWmState.assertSanity();
        mAmWmState.assertContainsStack(
                "Must contain fullscreen stack.", FULLSCREEN_WORKSPACE_STACK_ID);
        mAmWmState.assertContainsStack("Must contain docked stack.", DOCKED_STACK_ID);

        lockDevice();
        setDeviceRotation(0);
        unlockDevice();
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);

        lockDevice();
        setDeviceRotation(1);
        unlockDevice();
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);

        lockDevice();
        setDeviceRotation(2);
        unlockDevice();
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);

        lockDevice();
        setDeviceRotation(3);
        unlockDevice();
        mAmWmState.computeState(mDevice, waitForActivitiesVisible);
    }

    public void testResizeDockedStack() throws Exception {
        launchActivityInDockStack(DOCKED_ACTIVITY_NAME);
        launchActivityInStack(TEST_ACTIVITY_NAME, FULLSCREEN_WORKSPACE_STACK_ID);
        resizeDockedStack(STACK_SIZE, STACK_SIZE, TASK_SIZE, TASK_SIZE);
        mAmWmState.computeState(mDevice, new String[] {TEST_ACTIVITY_NAME, DOCKED_ACTIVITY_NAME},
                false /* compareTaskAndStackBounds */);
        mAmWmState.assertContainsStack("Must contain docked stack", DOCKED_STACK_ID);
        mAmWmState.assertContainsStack("Must contain fullscreen stack",
                FULLSCREEN_WORKSPACE_STACK_ID);
        assertEquals(new Rectangle(0, 0, STACK_SIZE, STACK_SIZE),
                mAmWmState.getAmState().getStackById(DOCKED_STACK_ID).getBounds());
        mAmWmState.assertDockedTaskBounds(TASK_SIZE, DOCKED_ACTIVITY_NAME);
        mAmWmState.assertVisibility(DOCKED_ACTIVITY_NAME, true);
        mAmWmState.assertVisibility(TEST_ACTIVITY_NAME, true);
    }

    public void testActivityLifeCycleOnResizeDockedStack() throws Exception {
        executeShellCommand(getAmStartCmd(TEST_ACTIVITY_NAME));
        mAmWmState.computeState(mDevice, new String[] {TEST_ACTIVITY_NAME});
        final Rectangle fullScreenBounds =
                mAmWmState.getWmState().getStack(FULLSCREEN_WORKSPACE_STACK_ID).getBounds();

        moveActivityToDockStack(TEST_ACTIVITY_NAME);
        launchActivityInStack(NO_RELAUNCH_ACTIVITY_NAME, FULLSCREEN_WORKSPACE_STACK_ID);

        mAmWmState.computeState(mDevice,
                new String[]{TEST_ACTIVITY_NAME, NO_RELAUNCH_ACTIVITY_NAME});
        final Rectangle initialDockBounds =
                mAmWmState.getWmState().getStack(DOCKED_STACK_ID).getBounds();

        clearLogcat();

        Rectangle newBounds = computeNewDockBounds(fullScreenBounds, initialDockBounds, true);
        resizeDockedStack(newBounds.width, newBounds.height, newBounds.width, newBounds.height);

        // We resize twice to make sure we cross an orientation change threshold for both
        // activities.
        newBounds = computeNewDockBounds(fullScreenBounds, initialDockBounds, false);
        resizeDockedStack(newBounds.width, newBounds.height, newBounds.width, newBounds.height);

        mAmWmState.computeState(mDevice,
                new String[]{TEST_ACTIVITY_NAME, NO_RELAUNCH_ACTIVITY_NAME});

        assertActivityLifecycle(TEST_ACTIVITY_NAME, true);
        assertActivityLifecycle(NO_RELAUNCH_ACTIVITY_NAME, false);
    }

    private Rectangle computeNewDockBounds(
            Rectangle fullscreenBounds, Rectangle dockBounds, boolean reduceSize) {
        final boolean inLandscape = fullscreenBounds.width > dockBounds.width;
        // We are either increasing size or reducing it.
        final float sizeChangeFactor = reduceSize ? 0.5f : 1.5f;
        final Rectangle newBounds = new Rectangle(dockBounds);
        if (inLandscape) {
            // In landscape we change the width.
            newBounds.width *= sizeChangeFactor;
        } else {
            // In portrait we change the height
            newBounds.height *= sizeChangeFactor;
        }

        return newBounds;
    }

    private void launchActivityToSide(String activityName) throws Exception {
        launchActivityToSide(activityName, false, false);
    }

    private void launchActivityToSide(String activityName, boolean randomData,
                                      boolean multipleTaskFlag) throws Exception {
        launchActivityToSide(activityName, randomData, multipleTaskFlag, null);
    }

    private void launchActivityToSide(String activityName, boolean randomData,
                                      boolean multipleTaskFlag, String targetActivityName)
            throws Exception {
        StringBuilder commandBuilder = new StringBuilder(getAmStartCmd(activityName));
        commandBuilder.append(" -f 0x20000000 --ez launch_to_the_side true");
        if (randomData) {
            commandBuilder.append(" --ez random_data true");
        }
        if (multipleTaskFlag) {
            commandBuilder.append(" --ez multiple_task true");
        }
        if (targetActivityName != null) {
            commandBuilder.append(" --es target_activity ").append(targetActivityName);
        }
        executeShellCommand(commandBuilder.toString());
    }
}
