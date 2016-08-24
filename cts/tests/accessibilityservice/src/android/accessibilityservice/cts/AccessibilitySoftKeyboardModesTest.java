/**
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.accessibilityservice.cts;

import android.accessibilityservice.AccessibilityServiceInfo;
import android.accessibilityservice.AccessibilityService.SoftKeyboardController;
import android.app.UiAutomation;
import android.content.ContentResolver;
import android.content.Context;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
import android.provider.Settings;
import android.test.ActivityInstrumentationTestCase2;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;

import android.accessibilityservice.cts.R;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.concurrent.TimeoutException;

/**
 * Test cases for testing the accessibility APIs for interacting with the soft keyboard show mode.
 */
public class AccessibilitySoftKeyboardModesTest extends ActivityInstrumentationTestCase2
        <AccessibilitySoftKeyboardModesTest.SoftKeyboardModesActivity> {

    private static final long TIMEOUT_PROPAGATE_SETTING = 5000;

    /**
     * Timeout required for pending Binder calls or event processing to
     * complete.
     */
    private static final long TIMEOUT_ASYNC_PROCESSING = 5000;

    /**
     * The timeout since the last accessibility event to consider the device idle.
     */
    private static final long TIMEOUT_ACCESSIBILITY_STATE_IDLE = 500;

    private static final int SHOW_MODE_AUTO = 0;
    private static final int SHOW_MODE_HIDDEN = 1;

    private int mLastCallbackValue;

    private InstrumentedAccessibilityService mService;
    private SoftKeyboardController mKeyboardController;
    private UiAutomation mUiAutomation;

    private Object mLock = new Object();

    public AccessibilitySoftKeyboardModesTest() {
        super(SoftKeyboardModesActivity.class);
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();

        // If we don't call getActivity(), we get an empty list when requesting the number of
        // windows on screen.
        getActivity();

        mService = InstrumentedAccessibilityService.enableService(
                this, InstrumentedAccessibilityService.class);
        mKeyboardController = mService.getSoftKeyboardController();
        mUiAutomation = getInstrumentation()
                .getUiAutomation(UiAutomation.FLAG_DONT_SUPPRESS_ACCESSIBILITY_SERVICES);
        AccessibilityServiceInfo info = mUiAutomation.getServiceInfo();
        info.flags |= AccessibilityServiceInfo.FLAG_RETRIEVE_INTERACTIVE_WINDOWS;
        mUiAutomation.setServiceInfo(info);
    }

    @Override
    public void tearDown() throws Exception {
        mService.runOnServiceSync(() -> mService.disableSelf());
    }

    public void testApiReturnValues_shouldChangeValueOnRequestAndSendCallback() throws Exception {
        mLastCallbackValue = -1;

        final SoftKeyboardController.OnShowModeChangedListener listener =
                new SoftKeyboardController.OnShowModeChangedListener() {
                    @Override
                    public void onShowModeChanged(SoftKeyboardController controller, int showMode) {
                        synchronized (mLock) {
                            mLastCallbackValue = showMode;
                            mLock.notifyAll();
                        }
                    }
                };
        mKeyboardController.addOnShowModeChangedListener(listener);

        // The soft keyboard should be in its' default mode.
        assertEquals(SHOW_MODE_AUTO, mKeyboardController.getShowMode());

        // Set the show mode to SHOW_MODE_HIDDEN.
        assertTrue(mKeyboardController.setShowMode(SHOW_MODE_HIDDEN));

        // Make sure the mode was changed.
        assertEquals(SHOW_MODE_HIDDEN, mKeyboardController.getShowMode());

        // Make sure we're getting the callback with the proper value.
        waitForCallbackValueWithLock(SHOW_MODE_HIDDEN);

        // Make sure we can set the value back to the default.
        assertTrue(mKeyboardController.setShowMode(SHOW_MODE_AUTO));
        assertEquals(SHOW_MODE_AUTO, mKeyboardController.getShowMode());
        waitForCallbackValueWithLock(SHOW_MODE_AUTO);

        // Make sure we can remove our listener.
        assertTrue(mKeyboardController.removeOnShowModeChangedListener(listener));
    }

    public void testHideSoftKeyboard_shouldHideAndShowKeyboardOnRequest() throws Exception {
        // The soft keyboard should be in its' default mode.
        assertEquals(SHOW_MODE_AUTO, mKeyboardController.getShowMode());

        // Note: This Activity always has a visible keyboard (due to windowSoftInputMode being set
        // to stateAlwaysVisible).
        waitForIdle();
        int numWindowsWithIme = mUiAutomation.getWindows().size();

        // Request the keyboard be hidden.
        assertTrue(mKeyboardController.setShowMode(SHOW_MODE_HIDDEN));
        waitForWindowStateChanged();
        waitForIdle();

        // Make sure the keyboard is hidden.
        assertEquals(numWindowsWithIme - 1, mUiAutomation.getWindows().size());

        // Request the default keyboard mode.
        assertTrue(mKeyboardController.setShowMode(SHOW_MODE_AUTO));
        waitForWindowStateChanged();
        waitForIdle();

        // Make sure the keyboard is visible.
        assertEquals(numWindowsWithIme, mUiAutomation.getWindows().size());
    }

    public void testHideSoftKeyboard_shouldHideKeyboardUntilServiceIsDisabled() throws Exception {
        // The soft keyboard should be in its' default mode.
        assertEquals(SHOW_MODE_AUTO, mKeyboardController.getShowMode());

        // Note: This Activity always has a visible keyboard (due to windowSoftInputMode being set
        // to stateAlwaysVisible).
        waitForIdle();
        int numWindowsWithIme = mUiAutomation.getWindows().size();

        // Set the show mode to SHOW_MODE_HIDDEN.
        assertTrue(mKeyboardController.setShowMode(SHOW_MODE_HIDDEN));
        waitForWindowStateChanged();
        waitForIdle();

        // Make sure the keyboard is hidden.
        assertEquals(numWindowsWithIme - 1, mUiAutomation.getWindows().size());

        // Make sure we can see the soft keyboard once all Accessibility Services are disabled.
        mService.disableSelf();
        waitForWindowStateChanged();
        waitForIdle();

        // See how many windows are present.
        assertEquals(numWindowsWithIme, mUiAutomation.getWindows().size());
    }

    private void waitForCallbackValueWithLock(int expectedValue) throws Exception {
        long timeoutTimeMillis = SystemClock.uptimeMillis() + TIMEOUT_PROPAGATE_SETTING;

        while (SystemClock.uptimeMillis() < timeoutTimeMillis) {
            synchronized(mLock) {
                if (mLastCallbackValue == expectedValue) {
                    return;
                }
                try {
                    mLock.wait(timeoutTimeMillis - SystemClock.uptimeMillis());
                } catch (InterruptedException e) {
                    // Wait until timeout.
                }
            }
        }

        throw new IllegalStateException("last callback value <" + mLastCallbackValue
                + "> does not match expected value < " + expectedValue + ">");
    }

    private void waitForWindowStateChanged() throws Exception {
        try {
            mUiAutomation.executeAndWaitForEvent(new Runnable() {
                @Override
                public void run() {
                    // Do nothing.
                }
            },
            new UiAutomation.AccessibilityEventFilter() {
                @Override
                public boolean accept (AccessibilityEvent event) {
                    return event.getEventType() == AccessibilityEvent.TYPE_WINDOWS_CHANGED;
                }
            },
            TIMEOUT_PROPAGATE_SETTING);
        } catch (TimeoutException ignored) {
            // Ignore since the event could have occurred before this method was called. There
            // should be a check after this method returns to catch incorrect values.
        }
    }

    private void waitForIdle() throws TimeoutException {
        mUiAutomation.waitForIdle(TIMEOUT_ACCESSIBILITY_STATE_IDLE, TIMEOUT_ASYNC_PROCESSING);
    }

    /**
     * Activity for testing the AccessibilityService API for hiding and showring the soft keyboard.
     */
    public static class SoftKeyboardModesActivity extends AccessibilityTestActivity {
        public SoftKeyboardModesActivity() {
            super();
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            setContentView(R.layout.accessibility_soft_keyboard_modes_test);
        }
    }
}
