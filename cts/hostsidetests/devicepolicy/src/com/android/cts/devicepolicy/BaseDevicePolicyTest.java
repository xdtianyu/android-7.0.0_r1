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

package com.android.cts.devicepolicy;

import com.android.cts.migration.MigrationHelper;
import com.android.ddmlib.Log.LogLevel;
import com.android.ddmlib.testrunner.InstrumentationResultParser;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.ddmlib.testrunner.TestResult;
import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.ddmlib.testrunner.TestRunResult;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IBuildReceiver;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.annotation.Nullable;

/**
 * Base class for device policy tests. It offers utility methods to run tests, set device or profile
 * owner, etc.
 */
public class BaseDevicePolicyTest extends DeviceTestCase implements IBuildReceiver {

    private static final String RUNNER = "android.support.test.runner.AndroidJUnitRunner";

    protected static final int USER_SYSTEM = 0; // From the UserHandle class.

    protected static final int USER_OWNER = 0;

    // From the UserInfo class
    protected static final int FLAG_PRIMARY = 0x00000001;
    protected static final int FLAG_GUEST = 0x00000004;
    protected static final int FLAG_EPHEMERAL = 0x00000100;

    protected static interface Settings {
        public static final String GLOBAL_NAMESPACE = "global";
        public static interface Global {
            public static final String DEVICE_PROVISIONED = "device_provisioned";
        }
    }

    protected IBuildInfo mCtsBuild;

    private String mPackageVerifier;
    private HashSet<String> mAvailableFeatures;

    /** Whether DPM is supported. */
    protected boolean mHasFeature;
    protected int mPrimaryUserId;

    /** Whether multi-user is supported. */
    protected boolean mSupportsMultiUser;

    /** Users we shouldn't delete in the tests */
    private ArrayList<Integer> mFixedUsers;

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mCtsBuild = buildInfo;
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        assertNotNull(mCtsBuild);  // ensure build has been set before test is run.
        mHasFeature = getDevice().getApiLevel() >= 21 /* Build.VERSION_CODES.L */
                && hasDeviceFeature("android.software.device_admin");
        mSupportsMultiUser = getMaxNumberOfUsersSupported() > 1;

        // disable the package verifier to avoid the dialog when installing an app
        mPackageVerifier = getDevice().executeShellCommand(
                "settings get global package_verifier_enable");
        getDevice().executeShellCommand("settings put global package_verifier_enable 0");

        mFixedUsers = new ArrayList();
        mPrimaryUserId = getPrimaryUser();
        mFixedUsers.add(mPrimaryUserId);
        if (mPrimaryUserId != USER_SYSTEM) {
            mFixedUsers.add(USER_SYSTEM);
        }
        switchUser(mPrimaryUserId);
        removeOwners();
        removeTestUsers();
    }

    @Override
    protected void tearDown() throws Exception {
        // reset the package verifier setting to its original value
        getDevice().executeShellCommand("settings put global package_verifier_enable "
                + mPackageVerifier);
        removeOwners();
        removeTestUsers();
        super.tearDown();
    }

    protected void installAppAsUser(String appFileName, int userId) throws FileNotFoundException,
            DeviceNotAvailableException {
        installAppAsUser(appFileName, true, userId);
    }

    protected void installAppAsUser(String appFileName, boolean grantPermissions, int userId)
            throws FileNotFoundException, DeviceNotAvailableException {
        CLog.d("Installing app " + appFileName + " for user " + userId);
        String result = getDevice().installPackageForUser(
                MigrationHelper.getTestFile(mCtsBuild, appFileName), true, grantPermissions,
                userId, "-t");
        assertNull("Failed to install " + appFileName + " for user " + userId + ": " + result,
                result);
    }

    /** Initializes the user with the given id. This is required so that apps can run on it. */
    protected void startUser(int userId) throws Exception {
        getDevice().startUser(userId);
    }

    protected void switchUser(int userId) throws Exception {
        // TODO Move this logic to ITestDevice
        String command = "am switch-user " + userId;
        CLog.d("Starting command " + command);
        String commandOutput = getDevice().executeShellCommand(command);
        CLog.d("Output for command " + command + ": " + commandOutput);
    }

    protected int getMaxNumberOfUsersSupported() throws DeviceNotAvailableException {
        return getDevice().getMaxNumberOfUsersSupported();
    }

    protected int getUserFlags(int userId) throws DeviceNotAvailableException {
        String command = "pm list users";
        String commandOutput = getDevice().executeShellCommand(command);
        CLog.i("Output for command " + command + ": " + commandOutput);

        String[] lines = commandOutput.split("\\r?\\n");
        assertTrue(commandOutput + " should contain at least one line", lines.length >= 1);
        for (int i = 1; i < lines.length; i++) {
            // Individual user is printed out like this:
            // \tUserInfo{$id$:$name$:$Integer.toHexString(flags)$} [running]
            String[] tokens = lines[i].split("\\{|\\}|:");
            assertTrue(lines[i] + " doesn't contain 4 or 5 tokens",
                    tokens.length == 4 || tokens.length == 5);
            // If the user IDs match, return the flags.
            if (Integer.parseInt(tokens[1]) == userId) {
                return Integer.parseInt(tokens[3], 16);
            }
        }
        fail("User not found");
        return 0;
    }

    protected ArrayList<Integer> listUsers() throws DeviceNotAvailableException {
        return getDevice().listUsers();
    }

    protected void stopUser(int userId) throws Exception  {
        String stopUserCommand = "am stop-user -w -f " + userId;
        CLog.d("starting command \"" + stopUserCommand + "\" and waiting.");
        CLog.d("Output for command " + stopUserCommand + ": "
                + getDevice().executeShellCommand(stopUserCommand));
    }

    protected void removeUser(int userId) throws Exception  {
        if (listUsers().contains(userId) && userId != USER_SYSTEM) {
            // Don't log output, as tests sometimes set no debug user restriction, which
            // causes this to fail, we should still continue and remove the user.
            String stopUserCommand = "am stop-user -w -f " + userId;
            CLog.d("stopping and removing user " + userId);
            getDevice().executeShellCommand(stopUserCommand);
            assertTrue("Couldn't remove user", getDevice().removeUser(userId));
        }
    }

    protected void removeTestUsers() throws Exception {
        for (int userId : listUsers()) {
            if (!mFixedUsers.contains(userId)) {
                removeUser(userId);
            }
        }
    }

    /** Returns true if the specified tests passed. Tests are run as given user. */
    protected boolean runDeviceTestsAsUser(
            String pkgName, @Nullable String testClassName, int userId)
            throws DeviceNotAvailableException {
        return runDeviceTestsAsUser(pkgName, testClassName, null /*testMethodName*/, userId);
    }

    /** Returns true if the specified tests passed. Tests are run as given user. */
    protected boolean runDeviceTestsAsUser(
            String pkgName, @Nullable String testClassName, String testMethodName, int userId)
            throws DeviceNotAvailableException {
        Map<String, String> params = Collections.emptyMap();
        return runDeviceTestsAsUser(pkgName, testClassName, testMethodName, userId, params);
    }

    protected boolean runDeviceTestsAsUser(String pkgName, @Nullable String testClassName,
            @Nullable String testMethodName, int userId,
            Map<String, String> params) throws DeviceNotAvailableException {
        if (testClassName != null && testClassName.startsWith(".")) {
            testClassName = pkgName + testClassName;
        }

        RemoteAndroidTestRunner testRunner = new RemoteAndroidTestRunner(
                pkgName, RUNNER, getDevice().getIDevice());
        if (testClassName != null && testMethodName != null) {
            testRunner.setMethodName(testClassName, testMethodName);
        } else if (testClassName != null) {
            testRunner.setClassName(testClassName);
        }

        for (Map.Entry<String, String> param : params.entrySet()) {
            testRunner.addInstrumentationArg(param.getKey(), param.getValue());
        }

        CollectingTestListener listener = new CollectingTestListener();
        assertTrue(getDevice().runInstrumentationTestsAsUser(testRunner, userId, listener));

        TestRunResult runResult = listener.getCurrentRunResults();
        printTestResult(runResult);
        return !runResult.hasFailedTests() && runResult.getNumTestsInState(TestStatus.PASSED) > 0;
    }

    /** Returns true if the system supports the split between system and primary user. */
    protected boolean hasUserSplit() throws DeviceNotAvailableException {
        return getBooleanSystemProperty("ro.fw.system_user_split", false);
    }

    /** Returns a boolean value of the system property with the specified key. */
    protected boolean getBooleanSystemProperty(String key, boolean defaultValue)
            throws DeviceNotAvailableException {
        final String[] positiveValues = {"1", "y", "yes", "true", "on"};
        final String[] negativeValues = {"0", "n", "no", "false", "off"};
        String propertyValue = getDevice().getProperty(key);
        if (propertyValue == null || propertyValue.isEmpty()) {
            return defaultValue;
        }
        if (Arrays.asList(positiveValues).contains(propertyValue)) {
            return true;
        }
        if (Arrays.asList(negativeValues).contains(propertyValue)) {
            return false;
        }
        fail("Unexpected value of boolean system property '" + key + "': " + propertyValue);
        return false;
    }

    /** Checks whether it is possible to create the desired number of users. */
    protected boolean canCreateAdditionalUsers(int numberOfUsers)
            throws DeviceNotAvailableException {
        return listUsers().size() + numberOfUsers <= getMaxNumberOfUsersSupported();
    }

    private void printTestResult(TestRunResult runResult) {
        for (Map.Entry<TestIdentifier, TestResult> testEntry :
                runResult.getTestResults().entrySet()) {
            TestResult testResult = testEntry.getValue();
            CLog.d("Test " + testEntry.getKey() + ": " + testResult.getStatus());
            if (testResult.getStatus() != TestStatus.PASSED) {
                CLog.d(testResult.getStackTrace());
            }
        }
    }

    protected boolean hasDeviceFeature(String requiredFeature) throws DeviceNotAvailableException {
        if (mAvailableFeatures == null) {
            // TODO: Move this logic to ITestDevice.
            String command = "pm list features";
            String commandOutput = getDevice().executeShellCommand(command);
            CLog.i("Output for command " + command + ": " + commandOutput);

            // Extract the id of the new user.
            mAvailableFeatures = new HashSet<>();
            for (String feature: commandOutput.split("\\s+")) {
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
            CLog.d("Device doesn't have required feature "
            + requiredFeature + ". Test won't run.");
        }
        return result;
    }

    protected int createUser() throws Exception {
        int userId = createUser(0);
        // TODO remove this and audit tests so they start users as necessary
        startUser(userId);
        return userId;
    }

    protected int createUser(int flags) throws Exception {
        boolean guest = FLAG_GUEST == (flags & FLAG_GUEST);
        boolean ephemeral = FLAG_EPHEMERAL == (flags & FLAG_EPHEMERAL);
        // TODO Use ITestDevice.createUser() when guest and ephemeral is available
        String command ="pm create-user " + (guest ? "--guest " : "")
                + (ephemeral ? "--ephemeral " : "") + "TestUser_" + System.currentTimeMillis();
        CLog.d("Starting command " + command);
        String commandOutput = getDevice().executeShellCommand(command);
        CLog.d("Output for command " + command + ": " + commandOutput);

        // Extract the id of the new user.
        String[] tokens = commandOutput.split("\\s+");
        assertTrue(tokens.length > 0);
        assertEquals("Success:", tokens[0]);
        return Integer.parseInt(tokens[tokens.length-1]);
    }

    protected int createManagedProfile(int parentUserId) throws DeviceNotAvailableException {
        String command = "pm create-user --profileOf " + parentUserId + " --managed "
                + "TestProfile_" + System.currentTimeMillis();
        CLog.d("Starting command " + command);
        String commandOutput = getDevice().executeShellCommand(command);
        CLog.d("Output for command " + command + ": " + commandOutput);

        // Extract the id of the new user.
        String[] tokens = commandOutput.split("\\s+");
        assertTrue(commandOutput + " expected to have format \"Success: {USER_ID}\"",
                tokens.length > 0);
        assertEquals(commandOutput, "Success:", tokens[0]);
        return Integer.parseInt(tokens[tokens.length-1]);
    }

    protected int getPrimaryUser() throws DeviceNotAvailableException {
        return getDevice().getPrimaryUserId();
    }

    protected int getUserSerialNumber(int userId) throws DeviceNotAvailableException{
        // TODO: Move this logic to ITestDevice.
        // dumpsys user return lines like "UserInfo{0:Owner:13} serialNo=0"
        String commandOutput = getDevice().executeShellCommand("dumpsys user");
        String[] tokens = commandOutput.split("\\n");
        for (String token : tokens) {
            token = token.trim();
            if (token.contains("UserInfo{" + userId + ":")) {
                String[] split = token.split("serialNo=");
                assertTrue(split.length == 2);
                int serialNumber = Integer.parseInt(split[1]);
                CLog.d("Serial number of user " + userId + ": "
                        + serialNumber);
                return serialNumber;
            }
        }
        fail("Couldn't find user " + userId);
        return -1;
    }

    protected boolean setProfileOwner(String componentName, int userId, boolean expectFailure)
            throws DeviceNotAvailableException {
        String command = "dpm set-profile-owner --user " + userId + " '" + componentName + "'";
        String commandOutput = getDevice().executeShellCommand(command);
        boolean success = commandOutput.startsWith("Success:");
        // If we succeeded always log, if we are expecting failure don't log failures
        // as call stacks for passing tests confuse the logs.
        if (success || !expectFailure) {
            CLog.d("Output for command " + command + ": " + commandOutput);
        } else {
            CLog.d("Command Failed " + command);
        }
        return success;
    }

    protected void setProfileOwnerOrFail(String componentName, int userId)
            throws Exception {
        if (!setProfileOwner(componentName, userId, /*expectFailure*/ false)) {
            removeUser(userId);
            fail("Failed to set profile owner");
        }
    }

    private String setDeviceAdminInner(String componentName, int userId)
            throws DeviceNotAvailableException {
        String command = "dpm set-active-admin --user " + userId + " '" + componentName + "'";
        String commandOutput = getDevice().executeShellCommand(command);
        return commandOutput;
    }

    protected void setDeviceAdmin(String componentName, int userId)
            throws DeviceNotAvailableException {
        String commandOutput = setDeviceAdminInner(componentName, userId);
        CLog.d("Output for command " + commandOutput
                + ": " + commandOutput);
        assertTrue(commandOutput + " expected to start with \"Success:\"",
                commandOutput.startsWith("Success:"));
    }

    protected void setDeviceAdminExpectingFailure(String componentName, int userId,
            String errorMessage) throws DeviceNotAvailableException {
        String commandOutput = setDeviceAdminInner(componentName, userId);
        if (!commandOutput.contains(errorMessage)) {
            fail(commandOutput + " expected to contain \"" + errorMessage + "\"");
        }
    }

    protected boolean setDeviceOwner(String componentName, int userId, boolean expectFailure)
            throws DeviceNotAvailableException {
        String command = "dpm set-device-owner --user " + userId + " '" + componentName + "'";
        String commandOutput = getDevice().executeShellCommand(command);
        boolean success = commandOutput.startsWith("Success:");
        // If we succeeded always log, if we are expecting failure don't log failures
        // as call stacks for passing tests confuse the logs.
        if (success || !expectFailure) {
            CLog.d("Output for command " + command + ": " + commandOutput);
        } else {
            CLog.d("Command Failed " + command);
        }
        return success;
    }

    protected String getSettings(String namespace, String name, int userId)
            throws DeviceNotAvailableException {
        String command = "settings --user " + userId + " get " + namespace + " " + name;
        String commandOutput = getDevice().executeShellCommand(command);
        CLog.d("Output for command " + command + ": " + commandOutput);
        return commandOutput.replace("\n", "").replace("\r", "");
    }

    protected void putSettings(String namespace, String name, String value, int userId)
            throws DeviceNotAvailableException {
        String command = "settings --user " + userId + " put " + namespace + " " + name
                + " " + value;
        String commandOutput = getDevice().executeShellCommand(command);
        CLog.d("Output for command " + command + ": " + commandOutput);
    }

    protected boolean removeAdmin(String componentName, int userId)
            throws DeviceNotAvailableException {
        String command = "dpm remove-active-admin --user " + userId + " '" + componentName + "'";
        String commandOutput = getDevice().executeShellCommand(command);
        CLog.d("Output for command " + command + ": " + commandOutput);
        return commandOutput.startsWith("Success:");
    }

    // Tries to remove and profile or device owners it finds.
    protected void removeOwners() throws DeviceNotAvailableException {
        String command = "dumpsys device_policy";
        String commandOutput = getDevice().executeShellCommand(command);
        String[] lines = commandOutput.split("\\r?\\n");
        for (int i = 0; i < lines.length; ++i) {
            String line = lines[i].trim();
            if (line.contains("Profile Owner")) {
                // Line is "Profile owner (User <id>):
                String[] tokens = line.split("\\(|\\)| ");
                int userId = Integer.parseInt(tokens[4]);
                i++;
                line = lines[i].trim();
                // Line is admin=ComponentInfo{<component>}
                tokens = line.split("\\{|\\}");
                String componentName = tokens[1];
                CLog.w("Cleaning up profile owner " + userId + " " + componentName);
                removeAdmin(componentName, userId);
            } else if (line.contains("Device Owner:")) {
                i++;
                line = lines[i].trim();
                // Line is admin=ComponentInfo{<component>}
                String[] tokens = line.split("\\{|\\}");
                String componentName = tokens[1];
                // Skip to user id line.
                i += 3;
                line = lines[i].trim();
                // Line is User ID: <N>
                tokens = line.split(":");
                int userId = Integer.parseInt(tokens[1].trim());
                CLog.w("Cleaning up device owner " + userId + " " + componentName);
                removeAdmin(componentName, userId);
            }
        }
    }
}
