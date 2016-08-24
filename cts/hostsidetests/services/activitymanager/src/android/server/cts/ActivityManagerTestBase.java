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

import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.DeviceTestCase;

import java.lang.Exception;
import java.lang.Integer;
import java.lang.String;
import java.util.HashSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static android.server.cts.StateLogger.log;

public abstract class ActivityManagerTestBase extends DeviceTestCase {
    private static final boolean PRETEND_DEVICE_SUPPORTS_PIP = false;
    private static final boolean PRETEND_DEVICE_SUPPORTS_FREEFORM = false;

    // Constants copied from ActivityManager.StackId. If they are changed there, these must be
    // updated.
    /** First static stack ID. */
    public static final int FIRST_STATIC_STACK_ID = 0;

    /** Home activity stack ID. */
    public static final int HOME_STACK_ID = FIRST_STATIC_STACK_ID;

    /** ID of stack where fullscreen activities are normally launched into. */
    public static final int FULLSCREEN_WORKSPACE_STACK_ID = 1;

    /** ID of stack where freeform/resized activities are normally launched into. */
    public static final int FREEFORM_WORKSPACE_STACK_ID = FULLSCREEN_WORKSPACE_STACK_ID + 1;

    /** ID of stack that occupies a dedicated region of the screen. */
    public static final int DOCKED_STACK_ID = FREEFORM_WORKSPACE_STACK_ID + 1;

    /** ID of stack that always on top (always visible) when it exist. */
    public static final int PINNED_STACK_ID = DOCKED_STACK_ID + 1;

    private static final String TASK_ID_PREFIX = "taskId";

    private static final String AM_STACK_LIST = "am stack list";

    private static final String AM_FORCE_STOP_TEST_PACKAGE = "am force-stop android.server.app";

    private static final String AM_REMOVE_STACK = "am stack remove ";

    protected static final String AM_START_HOME_ACTIVITY_COMMAND =
            "am start -a android.intent.action.MAIN -c android.intent.category.HOME";

    protected static final String AM_MOVE_TOP_ACTIVITY_TO_PINNED_STACK_COMMAND =
            "am stack move-top-activity-to-pinned-stack 1 0 0 500 500";

    private static final String AM_RESIZE_DOCKED_STACK = "am stack resize-docked-stack ";

    private static final String AM_MOVE_TASK = "am stack movetask ";

    /** A reference to the device under test. */
    protected ITestDevice mDevice;

    private HashSet<String> mAvailableFeatures;

    protected static String getAmStartCmd(final String activityName) {
        return "am start -n " + getActivityComponentName(activityName);
    }

    protected static String getAmStartCmdOverHome(final String activityName) {
        return "am start --activity-task-on-home -n " + getActivityComponentName(activityName);
    }

    static String getActivityComponentName(final String activityName) {
        return "android.server.app/." + activityName;
    }

    static String getWindowName(final String activityName) {
        return "android.server.app/android.server.app." + activityName;
    }

    protected ActivityAndWindowManagersState mAmWmState = new ActivityAndWindowManagersState();

    private int mInitialAccelerometerRotation;
    private int mUserRotation;

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        // Get the device, this gives a handle to run commands and install APKs.
        mDevice = getDevice();
        unlockDevice();
        // Remove special stacks.
        executeShellCommand(AM_REMOVE_STACK + PINNED_STACK_ID);
        executeShellCommand(AM_REMOVE_STACK + DOCKED_STACK_ID);
        executeShellCommand(AM_REMOVE_STACK + FREEFORM_WORKSPACE_STACK_ID);
        // Store rotation settings.
        mInitialAccelerometerRotation = getAccelerometerRotation();
        mUserRotation = getUserRotation();
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        try {
            unlockDevice();
            executeShellCommand(AM_FORCE_STOP_TEST_PACKAGE);
            // Restore rotation settings to the state they were before test.
            setAccelerometerRotation(mInitialAccelerometerRotation);
            setUserRotation(mUserRotation);
            // Remove special stacks.
            executeShellCommand(AM_REMOVE_STACK + PINNED_STACK_ID);
            executeShellCommand(AM_REMOVE_STACK + DOCKED_STACK_ID);
            executeShellCommand(AM_REMOVE_STACK + FREEFORM_WORKSPACE_STACK_ID);
        } catch (DeviceNotAvailableException e) {
        }
    }

    protected String executeShellCommand(String command) throws DeviceNotAvailableException {
        log("adb shell " + command);
        return mDevice.executeShellCommand(command);
    }

    protected void executeShellCommand(String command, CollectingOutputReceiver outputReceiver)
            throws DeviceNotAvailableException {
        log("adb shell " + command);
        mDevice.executeShellCommand(command, outputReceiver);
    }

    protected void launchActivityInStack(String activityName, int stackId) throws Exception {
        executeShellCommand(getAmStartCmd(activityName) + " --stack " + stackId);
    }

    protected void launchActivityInDockStack(String activityName) throws Exception {
        executeShellCommand(getAmStartCmd(activityName));
        moveActivityToDockStack(activityName);
    }

    protected void moveActivityToDockStack(String activityName) throws Exception {
        moveActivityToStack(activityName, DOCKED_STACK_ID);
    }

    protected void moveActivityToStack(String activityName, int stackId) throws Exception {
        final int taskId = getActivityTaskId(activityName);
        final String cmd = AM_MOVE_TASK + taskId + " " + stackId + " true";
        executeShellCommand(cmd);
    }

    protected void resizeActivityTask(String activityName, int left, int top, int right, int bottom)
            throws Exception {
        final int taskId = getActivityTaskId(activityName);
        final String cmd = "am task resize "
                + taskId + " " + left + " " + top + " " + right + " " + bottom;
        executeShellCommand(cmd);
    }

    protected void resizeDockedStack(
            int stackWidth, int stackHeight, int taskWidth, int taskHeight)
                    throws DeviceNotAvailableException {
        executeShellCommand(AM_RESIZE_DOCKED_STACK
                + "0 0 " + stackWidth + " " + stackHeight
                + " 0 0 " + taskWidth + " " + taskHeight);
    }

    // Utility method for debugging, not used directly here, but useful, so kept around.
    protected void printStacksAndTasks() throws DeviceNotAvailableException {
        CollectingOutputReceiver outputReceiver = new CollectingOutputReceiver();
        executeShellCommand(AM_STACK_LIST, outputReceiver);
        String output = outputReceiver.getOutput();
        for (String line : output.split("\\n")) {
            CLog.logAndDisplay(LogLevel.INFO, line);
        }
    }

    protected int getActivityTaskId(String name) throws DeviceNotAvailableException {
        CollectingOutputReceiver outputReceiver = new CollectingOutputReceiver();
        executeShellCommand(AM_STACK_LIST, outputReceiver);
        final String output = outputReceiver.getOutput();
        final Pattern activityPattern = Pattern.compile("(.*) " + getWindowName(name) + " (.*)");
        for (String line : output.split("\\n")) {
            Matcher matcher = activityPattern.matcher(line);
            if (matcher.matches()) {
                for (String word : line.split("\\s+")) {
                    if (word.startsWith(TASK_ID_PREFIX)) {
                        final String withColon = word.split("=")[1];
                        return Integer.parseInt(withColon.substring(0, withColon.length() - 1));
                    }
                }
            }
        }
        return -1;
    }

    protected boolean supportsPip() throws DeviceNotAvailableException {
        return hasDeviceFeature("android.software.picture_in_picture")
                || PRETEND_DEVICE_SUPPORTS_PIP;
    }

    protected boolean supportsFreeform() throws DeviceNotAvailableException {
        return hasDeviceFeature("android.software.freeform_window_management")
                || PRETEND_DEVICE_SUPPORTS_FREEFORM;
    }

    protected boolean hasDeviceFeature(String requiredFeature) throws DeviceNotAvailableException {
        if (mAvailableFeatures == null) {
            // TODO: Move this logic to ITestDevice.
            final String output = runCommandAndPrintOutput("pm list features");

            // Extract the id of the new user.
            mAvailableFeatures = new HashSet<>();
            for (String feature: output.split("\\s+")) {
                // Each line in the output of the command has the format "feature:{FEATURE_VALUE}".
                String[] tokens = feature.split(":");
                assertTrue("\"" + feature + "\" expected to have format feature:{FEATURE_VALUE}",
                        tokens.length > 1);
                assertEquals(feature, "feature", tokens[0]);
                mAvailableFeatures.add(tokens[1]);
            }
        }
        boolean result = mAvailableFeatures.contains(requiredFeature);
        if (!result) {
            CLog.logAndDisplay(LogLevel.INFO, "Device doesn't support " + requiredFeature);
        }
        return result;
    }

    private boolean isDisplayOn() throws DeviceNotAvailableException {
        final CollectingOutputReceiver outputReceiver = new CollectingOutputReceiver();
        mDevice.executeShellCommand("dumpsys power", outputReceiver);

        for (String line : outputReceiver.getOutput().split("\\n")) {
            line = line.trim();

            final Matcher matcher = sDisplayStatePattern.matcher(line);
            if (matcher.matches()) {
                final String state = matcher.group(1);
                log("power state=" + state);
                return "ON".equals(state);
            }
        }
        log("power state :(");
        return false;
    }

    protected void lockDevice() throws DeviceNotAvailableException {
        int retriesLeft = 5;
        runCommandAndPrintOutput("input keyevent 26");
        do {
            if (isDisplayOn()) {
                log("***Waiting for display to turn off...");
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    log(e.toString());
                    // Well I guess we are not waiting...
                }
            } else {
                break;
            }
        } while (retriesLeft-- > 0);
    }

    protected void unlockDevice() throws DeviceNotAvailableException {
        if (!isDisplayOn()) {
            runCommandAndPrintOutput("input keyevent 224");
            runCommandAndPrintOutput("input keyevent 82");
        }
    }

    protected void setDeviceRotation(int rotation) throws DeviceNotAvailableException {
        setAccelerometerRotation(0);
        setUserRotation(rotation);
    }

    private int getAccelerometerRotation() throws DeviceNotAvailableException {
        final String rotation =
                runCommandAndPrintOutput("settings get system accelerometer_rotation");
        return Integer.parseInt(rotation.trim());
    }

    private void setAccelerometerRotation(int rotation) throws DeviceNotAvailableException {
        runCommandAndPrintOutput(
                "settings put system accelerometer_rotation " + rotation);
    }

    private int getUserRotation() throws DeviceNotAvailableException {
        final String rotation =
                runCommandAndPrintOutput("settings get system user_rotation").trim();
        if ("null".equals(rotation)) {
            return -1;
        }
        return Integer.parseInt(rotation);
    }

    private void setUserRotation(int rotation) throws DeviceNotAvailableException {
        if (rotation == -1) {
            runCommandAndPrintOutput(
                    "settings delete system user_rotation");
        } else {
            runCommandAndPrintOutput(
                    "settings put system user_rotation " + rotation);
        }
    }

    protected String runCommandAndPrintOutput(String command) throws DeviceNotAvailableException {
        final String output = executeShellCommand(command);
        log(output);
        return output;
    }

    protected void clearLogcat() throws DeviceNotAvailableException {
        mDevice.executeAdbCommand("logcat", "-c");
    }

    protected void assertActivityLifecycle(String activityName, boolean relaunched)
            throws DeviceNotAvailableException {
        final ActivityLifecycleCounts lifecycleCounts = new ActivityLifecycleCounts(activityName);

        if (relaunched) {
            if (lifecycleCounts.mDestroyCount < 1) {
                fail(activityName + " must have been destroyed. mDestroyCount="
                        + lifecycleCounts.mDestroyCount);
            }
            if (lifecycleCounts.mCreateCount < 1) {
                fail(activityName + " must have been (re)created. mCreateCount="
                        + lifecycleCounts.mCreateCount);
            }
        } else {
            if (lifecycleCounts.mDestroyCount > 0) {
                fail(activityName + " must *NOT* have been destroyed. mDestroyCount="
                        + lifecycleCounts.mDestroyCount);
            }
            if (lifecycleCounts.mCreateCount > 0) {
                fail(activityName + " must *NOT* have been (re)created. mCreateCount="
                        + lifecycleCounts.mCreateCount);
            }
            if (lifecycleCounts.mConfigurationChangedCount < 1) {
                fail(activityName + " must have received configuration changed. "
                        + "mConfigurationChangedCount="
                        + lifecycleCounts.mConfigurationChangedCount);
            }
        }
    }

    private String[] getDeviceLogsForActivity(String activityName)
            throws DeviceNotAvailableException {
        return mDevice.executeAdbCommand("logcat", "-v", "brief", "-d", activityName + ":I", "*:S")
                .split("\\n");
    }

    private static final Pattern sCreatePattern = Pattern.compile("(.+): onCreate");
    private static final Pattern sConfigurationChangedPattern =
            Pattern.compile("(.+): onConfigurationChanged");
    private static final Pattern sDestroyPattern = Pattern.compile("(.+): onDestroy");
    private static final Pattern sNewConfigPattern = Pattern.compile(
            "(.+): config size=\\((\\d+),(\\d+)\\) displaySize=\\((\\d+),(\\d+)\\)" +
            " metricsSize=\\((\\d+),(\\d+)\\)");
    private static final Pattern sDisplayStatePattern =
            Pattern.compile("Display Power: state=(.+)");

    protected class ReportedSizes {
        int widthDp;
        int heightDp;
        int displayWidth;
        int displayHeight;
        int metricsWidth;
        int metricsHeight;
    }

    protected ReportedSizes getLastReportedSizesForActivity(String activityName)
            throws DeviceNotAvailableException {
        final String[] lines = getDeviceLogsForActivity(activityName);
        for (int i = lines.length - 1; i >= 0; i--) {
            final String line = lines[i].trim();
            final Matcher matcher = sNewConfigPattern.matcher(line);
            if (matcher.matches()) {
                ReportedSizes details = new ReportedSizes();
                details.widthDp = Integer.parseInt(matcher.group(2));
                details.heightDp = Integer.parseInt(matcher.group(3));
                details.displayWidth = Integer.parseInt(matcher.group(4));
                details.displayHeight = Integer.parseInt(matcher.group(5));
                details.metricsWidth = Integer.parseInt(matcher.group(6));
                details.metricsHeight = Integer.parseInt(matcher.group(7));
                return details;
            }
        }
        return null;
    }

    private class ActivityLifecycleCounts {
        int mCreateCount;
        int mConfigurationChangedCount;
        int mDestroyCount;

        public ActivityLifecycleCounts(String activityName) throws DeviceNotAvailableException {
            for (String line : getDeviceLogsForActivity(activityName)) {
                line = line.trim();

                Matcher matcher = sCreatePattern.matcher(line);
                if (matcher.matches()) {
                    mCreateCount++;
                    continue;
                }

                matcher = sConfigurationChangedPattern.matcher(line);
                if (matcher.matches()) {
                    mConfigurationChangedCount++;
                    continue;
                }

                matcher = sDestroyPattern.matcher(line);
                if (matcher.matches()) {
                    mDestroyCount++;
                    continue;
                }
            }
        }
    }
}
