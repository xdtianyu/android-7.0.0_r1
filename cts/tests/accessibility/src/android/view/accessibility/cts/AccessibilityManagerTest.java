/*
 * Copyright (C) 2012 The Android Open Source Project
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

package android.view.accessibility.cts;

import android.accessibilityservice.AccessibilityServiceInfo;
import android.app.Service;
import android.content.pm.ServiceInfo;
import android.cts.util.PollingCheck;
import android.test.InstrumentationTestCase;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityManager.AccessibilityStateChangeListener;
import android.view.accessibility.AccessibilityManager.TouchExplorationStateChangeListener;

import java.util.List;

/**
 * Class for testing {@link AccessibilityManager}.
 */
public class AccessibilityManagerTest extends InstrumentationTestCase {

    private static final String SPEAKING_ACCESSIBLITY_SERVICE_NAME =
        "android.view.accessibility.cts.SpeakingAccessibilityService";

    private static final String VIBRATING_ACCESSIBLITY_SERVICE_NAME =
        "android.view.accessibility.cts.VibratingAccessibilityService";

    private static final long WAIT_FOR_ACCESSIBILITY_ENABLED_TIMEOUT = 3000; // 3s

    private AccessibilityManager mAccessibilityManager;

    @Override
    public void setUp() throws Exception {
        mAccessibilityManager = (AccessibilityManager)
                getInstrumentation().getContext().getSystemService(Service.ACCESSIBILITY_SERVICE);
        ServiceControlUtils.enableSpeakingAndVibratingServices(getInstrumentation());
    }

    @Override
    public void tearDown() throws Exception {
        ServiceControlUtils.turnAccessibilityOff(getInstrumentation());
    }

    public void testAddAndRemoveAccessibilityStateChangeListener() throws Exception {
        AccessibilityStateChangeListener listener = new AccessibilityStateChangeListener() {
            @Override
            public void onAccessibilityStateChanged(boolean enabled) {
                /* do nothing */
            }
        };
        assertTrue(mAccessibilityManager.addAccessibilityStateChangeListener(listener));
        assertTrue(mAccessibilityManager.removeAccessibilityStateChangeListener(listener));
        assertFalse(mAccessibilityManager.removeAccessibilityStateChangeListener(listener));
    }

    public void testAddAndRemoveTouchExplorationStateChangeListener() throws Exception {
        TouchExplorationStateChangeListener listener = new TouchExplorationStateChangeListener() {
            @Override
            public void onTouchExplorationStateChanged(boolean enabled) {
                // Do nothing.
            }
        };
        assertTrue(mAccessibilityManager.addTouchExplorationStateChangeListener(listener));
        assertTrue(mAccessibilityManager.removeTouchExplorationStateChangeListener(listener));
        assertFalse(mAccessibilityManager.removeTouchExplorationStateChangeListener(listener));
    }

    public void testIsTouchExplorationEnabled() throws Exception {
        new PollingCheck() {
            @Override
            protected boolean check() {
                return mAccessibilityManager.isTouchExplorationEnabled();
            }
        }.run();
    }

    public void testGetInstalledAccessibilityServicesList() throws Exception {
        List<AccessibilityServiceInfo> installedServices =
            mAccessibilityManager.getInstalledAccessibilityServiceList();
        assertFalse("There must be at least one installed service.", installedServices.isEmpty());
        boolean speakingServiceInstalled = false;
        boolean vibratingServiceInstalled = false;
        final int serviceCount = installedServices.size();
        for (int i = 0; i < serviceCount; i++) {
            AccessibilityServiceInfo installedService = installedServices.get(i);
            ServiceInfo serviceInfo = installedService.getResolveInfo().serviceInfo;
            if (getClass().getPackage().getName().equals(serviceInfo.packageName)
                    && SPEAKING_ACCESSIBLITY_SERVICE_NAME.equals(serviceInfo.name)) {
                speakingServiceInstalled = true;
            }
            if (getClass().getPackage().getName().equals(serviceInfo.packageName)
                    && VIBRATING_ACCESSIBLITY_SERVICE_NAME.equals(serviceInfo.name)) {
                vibratingServiceInstalled = true;
            }
        }
        assertTrue("The speaking service should be installed.", speakingServiceInstalled);
        assertTrue("The vibrating service should be installed.", vibratingServiceInstalled);
    }

    public void testGetEnabledAccessibilityServiceList() throws Exception {
        List<AccessibilityServiceInfo> enabledServices =
            mAccessibilityManager.getEnabledAccessibilityServiceList(
                    AccessibilityServiceInfo.FEEDBACK_ALL_MASK);
        boolean speakingServiceEnabled = false;
        boolean vibratingServiceEnabled = false;
        final int serviceCount = enabledServices.size();
        for (int i = 0; i < serviceCount; i++) {
            AccessibilityServiceInfo enabledService = enabledServices.get(i);
            ServiceInfo serviceInfo = enabledService.getResolveInfo().serviceInfo;
            if (getClass().getPackage().getName().equals(serviceInfo.packageName)
                    && SPEAKING_ACCESSIBLITY_SERVICE_NAME.equals(serviceInfo.name)) {
                speakingServiceEnabled = true;
            }
            if (getClass().getPackage().getName().equals(serviceInfo.packageName)
                    && VIBRATING_ACCESSIBLITY_SERVICE_NAME.equals(serviceInfo.name)) {
                vibratingServiceEnabled = true;
            }
        }
        assertTrue("The speaking service should be enabled.", speakingServiceEnabled);
        assertTrue("The vibrating service should be enabled.", vibratingServiceEnabled);
    }

    public void testGetEnabledAccessibilityServiceListForType() throws Exception {
        List<AccessibilityServiceInfo> enabledServices =
            mAccessibilityManager.getEnabledAccessibilityServiceList(
                    AccessibilityServiceInfo.FEEDBACK_SPOKEN);
        assertSame("There should be only one enabled speaking service.", 1, enabledServices.size());
        final int serviceCount = enabledServices.size();
        for (int i = 0; i < serviceCount; i++) {
            AccessibilityServiceInfo enabledService = enabledServices.get(i);
            ServiceInfo serviceInfo = enabledService.getResolveInfo().serviceInfo;
            if (getClass().getPackage().getName().equals(serviceInfo.packageName)
                    && SPEAKING_ACCESSIBLITY_SERVICE_NAME.equals(serviceInfo.name)) {
                return;
            }
        }
        fail("The speaking service is not enabled.");
    }

    @SuppressWarnings("deprecation")
    public void testGetAccessibilityServiceList() throws Exception {
        List<ServiceInfo> services = mAccessibilityManager.getAccessibilityServiceList();
        boolean speakingServiceInstalled = false;
        boolean vibratingServiceInstalled = false;
        final int serviceCount = services.size();
        for (int i = 0; i < serviceCount; i++) {
            ServiceInfo serviceInfo = services.get(i);
            if (getClass().getPackage().getName().equals(serviceInfo.packageName)
                    && SPEAKING_ACCESSIBLITY_SERVICE_NAME.equals(serviceInfo.name)) {
                speakingServiceInstalled = true;
            }
            if (getClass().getPackage().getName().equals(serviceInfo.packageName)
                    && VIBRATING_ACCESSIBLITY_SERVICE_NAME.equals(serviceInfo.name)) {
                vibratingServiceInstalled = true;
            }
        }
        assertTrue("The speaking service should be installed.", speakingServiceInstalled);
        assertTrue("The vibrating service should be installed.", vibratingServiceInstalled);
    }

    public void testInterrupt() throws Exception {
        // The APIs are heavily tested in the android.accessibiliyservice package.
        // This just makes sure the call does not throw an exception.
        waitForAccessibilityEnabled();
        mAccessibilityManager.interrupt();
    }

    public void testSendAccessibilityEvent() throws Exception {
        // The APIs are heavily tested in the android.accessibiliyservice package.
        // This just makes sure the call does not throw an exception.
        waitForAccessibilityEnabled();
        mAccessibilityManager.sendAccessibilityEvent(AccessibilityEvent.obtain(
                AccessibilityEvent.TYPE_VIEW_CLICKED));
    }

    public void testTouchExplorationStateChanged() throws Exception {
        waitForTouchExplorationEnabled();
    }

    private void waitForAccessibilityEnabled() throws InterruptedException {
        final Object waitObject = new Object();

        AccessibilityStateChangeListener listener = new AccessibilityStateChangeListener() {
            @Override
            public void onAccessibilityStateChanged(boolean b) {
                synchronized (waitObject) {
                    waitObject.notifyAll();
                }
            }
        };
        mAccessibilityManager.addAccessibilityStateChangeListener(listener);
        long timeoutTime = System.currentTimeMillis() + WAIT_FOR_ACCESSIBILITY_ENABLED_TIMEOUT;
        synchronized (waitObject) {
            while (!mAccessibilityManager.isEnabled()
                    && (System.currentTimeMillis() < timeoutTime)) {
                waitObject.wait(timeoutTime - System.currentTimeMillis());
            }
        }
        mAccessibilityManager.removeAccessibilityStateChangeListener(listener);
        assertTrue("Timed out enabling accessibility", mAccessibilityManager.isEnabled());
    }

    private void waitForTouchExplorationEnabled() throws InterruptedException {
        final Object waitObject = new Object();

        TouchExplorationStateChangeListener listener = new TouchExplorationStateChangeListener() {
            @Override
            public void onTouchExplorationStateChanged(boolean b) {
                synchronized (waitObject) {
                    waitObject.notifyAll();
                }
            }
        };
        mAccessibilityManager.addTouchExplorationStateChangeListener(listener);
        long timeoutTime = System.currentTimeMillis() + WAIT_FOR_ACCESSIBILITY_ENABLED_TIMEOUT;
        synchronized (waitObject) {
            while (!mAccessibilityManager.isTouchExplorationEnabled()
                    && (System.currentTimeMillis() < timeoutTime)) {
                waitObject.wait(timeoutTime - System.currentTimeMillis());
            }
        }
        mAccessibilityManager.removeTouchExplorationStateChangeListener(listener);
        assertTrue("Timed out enabling touch exploration",
                mAccessibilityManager.isTouchExplorationEnabled());
    }
}
