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

package com.android.managedprovisioning.task;

import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageDeleteObserver;
import android.content.pm.IPackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ParceledListSlice;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.res.Resources;
import android.content.Context;
import android.content.Intent;
import android.os.RemoteException;
import android.test.AndroidTestCase;
import android.test.mock.MockPackageManager;
import android.test.suitebuilder.annotation.SmallTest;
import android.view.inputmethod.InputMethodInfo;

import com.android.internal.view.IInputMethodManager;

import java.io.File;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

public class DeleteNonRequiredAppsTaskTest extends AndroidTestCase {

    private @Mock Resources mResources;
    private @Mock IPackageManager mIPackageManager;
    private @Mock IInputMethodManager mIInputMethodManager;
    private @Mock DeleteNonRequiredAppsTask.Callback mCallback;
    private @Mock Context mTestContext;

    private FakePackageManager mPackageManager;

    private static final String TEST_MDM_PACKAGE_NAME = "mdm.package.name";
    private static final int TEST_USER_ID = 0;
    private Set<String> mDeletedApps;
    private String[] mSystemAppsWithLauncher;
    private Set<String> mInstalledApplications;

    @Override
    protected void setUp() throws Exception {
        // this is necessary for mockito to work
        System.setProperty("dexmaker.dexcache", getContext().getCacheDir().toString());

        MockitoAnnotations.initMocks(this);

        mPackageManager = new FakePackageManager();

        when(mTestContext.getResources()).thenReturn(mResources);
        when(mTestContext.getPackageManager()).thenReturn(mPackageManager);
        when(mTestContext.getFilesDir()).thenReturn(getContext().getFilesDir());

        mDeletedApps = new HashSet<String>();

        setSystemInputMethods();
        setRequiredAppsManagedDevice();
        setVendorRequiredAppsManagedDevice();
        setDisallowedAppsManagedDevice();
        setVendorDisallowedAppsManagedDevice();
        setRequiredAppsManagedProfile();
        setVendorRequiredAppsManagedProfile();
        setDisallowedAppsManagedProfile();
        setVendorDisallowedAppsManagedProfile();
        setRequiredAppsManagedUser();
        setVendorRequiredAppsManagedUser();
        setDisallowedAppsManagedUser();
        setVendorDisallowedAppsManagedUser();
    }

    // We run most methods for device owner only, and we'll assume they also work for profile owner.
    @SmallTest
    public void testOnlyAppsWithLauncherDeletedByDefault() {
        setSystemAppsWithLauncher("app.a");
        setInstalledSystemApps( "app.a", "app.b");

        runTask(DeleteNonRequiredAppsTask.DEVICE_OWNER, true, false);

        assertDeletedApps("app.a");
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testDeviceOwnerRequiredAppsNotDeleted() {
        setSystemAppsWithLauncher("app.a", "app.b", "app.c");
        setInstalledSystemApps("app.a", "app.b", "app.c");
        setRequiredAppsManagedDevice("app.a");
        setVendorRequiredAppsManagedDevice("app.b");

        runTask(DeleteNonRequiredAppsTask.DEVICE_OWNER, true, false);

        assertDeletedApps("app.c");
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testProfileOwnerRequiredAppsNotDeleted() {
        setSystemAppsWithLauncher("app.a", "app.b", "app.c");
        setInstalledSystemApps("app.a", "app.b", "app.c");
        setRequiredAppsManagedProfile("app.a");
        setVendorRequiredAppsManagedProfile("app.b");

        runTask(DeleteNonRequiredAppsTask.PROFILE_OWNER, true, false);

        assertDeletedApps("app.c");
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testManagedUserRequiredAppsNotDeleted() {
        setSystemAppsWithLauncher("app.a", "app.b", "app.c");
        setInstalledSystemApps("app.a", "app.b", "app.c");
        setRequiredAppsManagedUser("app.a");
        setVendorRequiredAppsManagedUser("app.b");

        runTask(DeleteNonRequiredAppsTask.MANAGED_USER, true, false);

        assertDeletedApps("app.c");
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testMdmNotDeleted() {
        setSystemAppsWithLauncher(TEST_MDM_PACKAGE_NAME);
        setInstalledSystemApps(TEST_MDM_PACKAGE_NAME);

        runTask(DeleteNonRequiredAppsTask.DEVICE_OWNER, true, false);

        assertDeletedApps();
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testDisallowedAppsDeletedEvenIfNoLauncher() {
        setSystemAppsWithLauncher();
        setInstalledSystemApps("app.a", "app.b", "app.c");
        setDisallowedAppsManagedDevice("app.a");
        setVendorDisallowedAppsManagedDevice("app.b");

        runTask(DeleteNonRequiredAppsTask.DEVICE_OWNER, true, false);

        assertDeletedApps("app.a", "app.b");
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testDeviceOwnerImesNotDeleted() {
        setSystemAppsWithLauncher("app.a", "app.b");
        setInstalledSystemApps("app.a", "app.b");
        setSystemInputMethods("app.a");

        runTask(DeleteNonRequiredAppsTask.DEVICE_OWNER, true, false);

        assertDeletedApps("app.b");
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testProfileOwnerImesStillDeleted() {
        setSystemAppsWithLauncher("app.a", "app.b");
        setInstalledSystemApps("app.a", "app.b");
        setSystemInputMethods("app.a");

        runTask(DeleteNonRequiredAppsTask.PROFILE_OWNER, true, false);

        assertDeletedApps("app.a", "app.b");
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testManagedUserImesNotDeleted() {
        setSystemAppsWithLauncher("app.a", "app.b");
        setInstalledSystemApps("app.a", "app.b");
        setSystemInputMethods("app.a");

        runTask(DeleteNonRequiredAppsTask.MANAGED_USER, true, false);

        assertDeletedApps("app.b");
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testLeaveAllAppsEnabled() {
        setSystemAppsWithLauncher("app.a");
        setInstalledSystemApps("app.a");

        runTask(DeleteNonRequiredAppsTask.DEVICE_OWNER, true, true);

        assertDeletedApps(); //assert that no app has been deleted.
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testNewAppsDeletedAfterOta() {
        setSystemAppsWithLauncher();
        setInstalledSystemApps("app.a");

        runTask(DeleteNonRequiredAppsTask.DEVICE_OWNER, true, false);

        verify(mCallback, times(1)).onSuccess();
        assertDeletedApps(); //assert that no app has been deleted.

        // Now, an OTA happens and installs app.b with a launcher
        setSystemAppsWithLauncher("app.b");
        setInstalledSystemApps("app.a", "app.b");

        runTask(DeleteNonRequiredAppsTask.DEVICE_OWNER, false, false);

        assertDeletedApps("app.b");
        verify(mCallback, times(2)).onSuccess();
    }

    @SmallTest
    public void testExistingAppsNotDeletedAgainAfterOta() {
        setSystemAppsWithLauncher("app.a");
        setInstalledSystemApps("app.a");

        runTask(DeleteNonRequiredAppsTask.DEVICE_OWNER, true, false);

        verify(mCallback, times(1)).onSuccess();
        assertDeletedApps("app.a");
        mDeletedApps.clear();

        runTask(DeleteNonRequiredAppsTask.DEVICE_OWNER, false, false);

        assertDeletedApps();
        verify(mCallback, times(2)).onSuccess();
    }

    @SmallTest
    public void testWhenNoSystemAppsFileFound() {
        setSystemAppsWithLauncher("app.a");
        setInstalledSystemApps("app.a");

        runTask(DeleteNonRequiredAppsTask.DEVICE_OWNER, true, false);

        verify(mCallback, times(1)).onSuccess();
        assertDeletedApps("app.a");
        mDeletedApps.clear();

        setSystemAppsWithLauncher("app.a", "app.b");
        setInstalledSystemApps("app.a", "app.b");

        // Now, we set a wrong value to mTestContext.getFilesDir. So it should not find the system apps
        // file. So it should not delete any app, but call onError().
        when(mTestContext.getFilesDir()).thenReturn(new File(""));
        runTask(DeleteNonRequiredAppsTask.DEVICE_OWNER, false, false);

        assertDeletedApps();
        verify(mCallback, times(1)).onError();
    }

    @SmallTest
    public void testWhenDeletionFails() {
        setSystemAppsWithLauncher("app.a");
        setInstalledSystemApps("app.a");
        mPackageManager.setDeletionSucceeds(false);
        runTask(DeleteNonRequiredAppsTask.DEVICE_OWNER, true, false);
        verify(mCallback, times(1)).onError();
    }

    private void setRequiredAppsManagedDevice(String... apps) {
        setStringArray(com.android.managedprovisioning.R.array.required_apps_managed_device,
                apps);
    }

    private void setVendorRequiredAppsManagedDevice(String... apps) {
        setStringArray(com.android.managedprovisioning.R.array.vendor_required_apps_managed_device,
                apps);
    }

    private void setDisallowedAppsManagedDevice(String... apps) {
        setStringArray(com.android.managedprovisioning.R.array.disallowed_apps_managed_device,
                apps);
    }

    private void setVendorDisallowedAppsManagedDevice(String... apps) {
        setStringArray(
                com.android.managedprovisioning.R.array.vendor_disallowed_apps_managed_device,
                apps);
    }

    private void setRequiredAppsManagedProfile(String... apps) {
        setStringArray(com.android.managedprovisioning.R.array.required_apps_managed_profile,
                apps);
    }

    private void setVendorRequiredAppsManagedProfile(String... apps) {
        setStringArray(com.android.managedprovisioning.R.array.vendor_required_apps_managed_profile,
                apps);
    }

    private void setDisallowedAppsManagedProfile(String... apps) {
        setStringArray(com.android.managedprovisioning.R.array.disallowed_apps_managed_profile,
                apps);
    }

    private void setVendorDisallowedAppsManagedProfile(String... apps) {
        setStringArray(
                com.android.managedprovisioning.R.array.vendor_disallowed_apps_managed_profile,
                apps);
    }

    private void setRequiredAppsManagedUser(String... apps) {
        setStringArray(com.android.managedprovisioning.R.array.required_apps_managed_user,
                apps);
    }

    private void setVendorRequiredAppsManagedUser(String... apps) {
        setStringArray(com.android.managedprovisioning.R.array.vendor_required_apps_managed_user,
                apps);
    }

    private void setDisallowedAppsManagedUser(String... apps) {
        setStringArray(com.android.managedprovisioning.R.array.disallowed_apps_managed_user,
                apps);
    }

    private void setVendorDisallowedAppsManagedUser(String... apps) {
        setStringArray(
                com.android.managedprovisioning.R.array.vendor_disallowed_apps_managed_user,
                apps);
    }

    private void runTask(int type, boolean newProfile, boolean leaveAllSystemAppsEnabled) {
        DeleteNonRequiredAppsTask task = new DeleteNonRequiredAppsTask(mTestContext,
                mIPackageManager, mIInputMethodManager, TEST_MDM_PACKAGE_NAME, type, newProfile,
                TEST_USER_ID, leaveAllSystemAppsEnabled, mCallback);
        task.run();
    }

    private void setStringArray(int resourceId, String[] strs) {
        when(mResources.getStringArray(eq(resourceId)))
                .thenReturn(strs);
    }

    private void assertDeletedApps(String... appArray) {
        assertEquals(setFromArray(appArray), mDeletedApps);
    }

    private void setInstalledSystemApps(String... installedApps) {
        List<ApplicationInfo> applications = new ArrayList<ApplicationInfo>();
        for (String app : installedApps) {
            ApplicationInfo aInfo = new ApplicationInfo();
            aInfo.flags = ApplicationInfo.FLAG_SYSTEM;
            aInfo.packageName = app;
            applications.add(aInfo);
        }
        try {
            when(mIPackageManager.getInstalledApplications(PackageManager.GET_UNINSTALLED_PACKAGES,
                    TEST_USER_ID)).thenReturn(new ParceledListSlice(applications));
        } catch (RemoteException e) {
            fail(e.toString());
        }
        mInstalledApplications = setFromArray(installedApps);
    }

    private void setSystemInputMethods(String... packageNames) {
        List<InputMethodInfo> inputMethods = new ArrayList<InputMethodInfo>();
        for (String packageName : packageNames) {
            ApplicationInfo aInfo = new ApplicationInfo();
            aInfo.flags = ApplicationInfo.FLAG_SYSTEM;
            ServiceInfo serviceInfo = new ServiceInfo();
            serviceInfo.applicationInfo = aInfo;
            serviceInfo.packageName = packageName;
            serviceInfo.name = "";
            ResolveInfo ri = new ResolveInfo();
            ri.serviceInfo = serviceInfo;
            InputMethodInfo inputMethodInfo = new InputMethodInfo(ri, false, null, null, 0, false);
            inputMethods.add(inputMethodInfo);
        }
        try {
            when(mIInputMethodManager.getInputMethodList()).thenReturn(inputMethods);
        } catch (RemoteException e) {
            fail(e.toString());
        }
    }

    private void setSystemAppsWithLauncher(String... apps) {
        mSystemAppsWithLauncher = apps;
    }

    private <T> Set<T> setFromArray(T[] array) {
        return new HashSet<T>(Arrays.asList(array));
    }

    class FakePackageManager extends MockPackageManager {
        private boolean mDeletionSucceeds = true;

        void setDeletionSucceeds(boolean deletionSucceeds) {
            mDeletionSucceeds = deletionSucceeds;
        }

        @Override
        public void deletePackageAsUser(String packageName, IPackageDeleteObserver observer,
                int flags, int userId) {
            if (mDeletionSucceeds) {
                mDeletedApps.add(packageName);
            }
            assertTrue((flags & PackageManager.DELETE_SYSTEM_APP) != 0);
            assertEquals(TEST_USER_ID, userId);

            int resultCode;
            if (mDeletionSucceeds) {
                resultCode = PackageManager.DELETE_SUCCEEDED;
            } else {
                resultCode = PackageManager.DELETE_FAILED_INTERNAL_ERROR;
            }

            try {
                observer.packageDeleted(packageName, resultCode);
            } catch (RemoteException e) {
                fail(e.toString());
            }
        }

        @Override
        public List<ResolveInfo> queryIntentActivitiesAsUser(Intent intent, int flags, int userId) {
            assertTrue("Expected an intent with action ACTION_MAIN and category CATEGORY_LAUNCHER",
                    intent.filterEquals(
                    new Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_LAUNCHER)));
            assertTrue("Expected the flag MATCH_UNINSTALLED_PACKAGES",
                    (flags & PackageManager.MATCH_UNINSTALLED_PACKAGES) != 0);
            assertTrue("Expected the flag MATCH_DISABLED_COMPONENTS",
                    (flags & PackageManager.MATCH_DISABLED_COMPONENTS) != 0);
            assertTrue("Expected the flag MATCH_ENCRYPTION_AWARE_AND_UNAWARE",
                    (flags & PackageManager.MATCH_ENCRYPTION_AWARE_AND_UNAWARE) != 0);
            assertEquals(userId, TEST_USER_ID);
            List<ResolveInfo> result = new ArrayList<ResolveInfo>();
            for (String packageName : mSystemAppsWithLauncher) {
                ActivityInfo ai = new ActivityInfo();
                ai.packageName = packageName;
                ResolveInfo ri = new ResolveInfo();
                ri.activityInfo  = ai;
                result.add(ri);
            }
            return result;
        }

        @Override
        public PackageInfo getPackageInfoAsUser(String packageName, int flags, int userId)
                throws NameNotFoundException {
            if (mInstalledApplications.contains(packageName)) {
                return new PackageInfo();
            }
            throw new NameNotFoundException();
        }
    }
}

