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

package android.appsecurity.cts;

import com.android.ddmlib.Log;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.TestIdentifier;
import com.android.ddmlib.testrunner.TestResult;
import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.ddmlib.testrunner.TestRunResult;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.result.CollectingTestListener;

import java.util.Map;

public class Utils {
    private static final String TAG = "AppSecurity";

    public static final int USER_OWNER = 0;

    public static void runDeviceTests(ITestDevice device, String packageName)
            throws DeviceNotAvailableException {
        runDeviceTests(device, packageName, null, null, USER_OWNER);
    }

    public static void runDeviceTests(ITestDevice device, String packageName, int userId)
            throws DeviceNotAvailableException {
        runDeviceTests(device, packageName, null, null, userId);
    }

    public static void runDeviceTests(ITestDevice device, String packageName, String testClassName)
            throws DeviceNotAvailableException {
        runDeviceTests(device, packageName, testClassName, null, USER_OWNER);
    }

    public static void runDeviceTests(ITestDevice device, String packageName, String testClassName,
            int userId) throws DeviceNotAvailableException {
        runDeviceTests(device, packageName, testClassName, null, userId);
    }

    public static void runDeviceTests(ITestDevice device, String packageName, String testClassName,
            String testMethodName) throws DeviceNotAvailableException {
        runDeviceTests(device, packageName, testClassName, testMethodName, USER_OWNER);
    }

    public static void runDeviceTests(ITestDevice device, String packageName, String testClassName,
            String testMethodName, int userId) throws DeviceNotAvailableException {
        if (testClassName != null && testClassName.startsWith(".")) {
            testClassName = packageName + testClassName;
        }

        RemoteAndroidTestRunner testRunner = new RemoteAndroidTestRunner(packageName,
                "android.support.test.runner.AndroidJUnitRunner", device.getIDevice());
        if (testClassName != null && testMethodName != null) {
            testRunner.setMethodName(testClassName, testMethodName);
        } else if (testClassName != null) {
            testRunner.setClassName(testClassName);
        }

        if (userId != USER_OWNER) {
            // TODO: move this to RemoteAndroidTestRunner once it supports users
            testRunner.addInstrumentationArg("hack_key", "hack_value --user " + userId);
        }

        final CollectingTestListener listener = new CollectingTestListener();
        device.runInstrumentationTests(testRunner, listener);

        final TestRunResult result = listener.getCurrentRunResults();
        if (result.isRunFailure()) {
            throw new AssertionError("Failed to successfully run device tests for "
                    + result.getName() + ": " + result.getRunFailureMessage());
        }
        if (result.getNumTests() == 0) {
            throw new AssertionError("No tests were run on the device");
        }
        if (result.hasFailedTests()) {
            // build a meaningful error message
            StringBuilder errorBuilder = new StringBuilder("on-device tests failed:\n");
            for (Map.Entry<TestIdentifier, TestResult> resultEntry :
                result.getTestResults().entrySet()) {
                if (!resultEntry.getValue().getStatus().equals(TestStatus.PASSED)) {
                    errorBuilder.append(resultEntry.getKey().toString());
                    errorBuilder.append(":\n");
                    errorBuilder.append(resultEntry.getValue().getStackTrace());
                }
            }
            throw new AssertionError(errorBuilder.toString());
        }
    }

    private static boolean isMultiUserSupportedOnDevice(ITestDevice device)
            throws DeviceNotAvailableException {
        // TODO: move this to ITestDevice once it supports users
        final String output = device.executeShellCommand("pm get-max-users");
        try {
            return Integer.parseInt(output.substring(output.lastIndexOf(" ")).trim()) > 1;
        } catch (NumberFormatException e) {
            throw new AssertionError("Failed to parse result: " + output);
        }
    }

    /**
     * Return set of users that test should be run for, creating a secondary
     * user if the device supports it. Always call
     * {@link #removeUsersForTest(ITestDevice, int[])} when finished.
     */
    public static int[] createUsersForTest(ITestDevice device) throws DeviceNotAvailableException {
        if (isMultiUserSupportedOnDevice(device)) {
            return new int[] { USER_OWNER, createUserOnDevice(device) };
        } else {
            Log.d(TAG, "Single user device; skipping isolated storage tests");
            return new int[] { USER_OWNER };
        }
    }

    public static void removeUsersForTest(ITestDevice device, int[] users)
            throws DeviceNotAvailableException {
        for (int user : users) {
            if (user != USER_OWNER) {
                removeUserOnDevice(device, user);
            }
        }
    }

    private static int createUserOnDevice(ITestDevice device) throws DeviceNotAvailableException {
        // TODO: move this to ITestDevice once it supports users
        final String name = "CTS_" + System.currentTimeMillis();
        final String output = device.executeShellCommand("pm create-user " + name);
        if (output.startsWith("Success")) {
            try {
                final int userId = Integer.parseInt(
                        output.substring(output.lastIndexOf(" ")).trim());
                device.executeShellCommand("am start-user " + userId);
                return userId;
            } catch (NumberFormatException e) {
                throw new AssertionError("Failed to parse result: " + output);
            }
        } else {
            throw new AssertionError("Failed to create user: " + output);
        }
    }

    private static void removeUserOnDevice(ITestDevice device, int userId)
            throws DeviceNotAvailableException {
        // TODO: move this to ITestDevice once it supports users
        final String output = device.executeShellCommand("pm remove-user " + userId);
        if (output.startsWith("Error")) {
            throw new AssertionError("Failed to remove user: " + output);
        }
    }

}
