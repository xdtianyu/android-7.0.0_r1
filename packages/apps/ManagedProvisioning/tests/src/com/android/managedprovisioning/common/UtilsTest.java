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
 * limitations under the License.
 */

package com.android.managedprovisioning.common;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AccountManagerCallback;
import android.accounts.AccountManagerFuture;
import android.accounts.OperationCanceledException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.PackageInfo;
import android.content.pm.ParceledListSlice;
import android.content.pm.ResolveInfo;
import android.graphics.Color;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.os.Handler;
import android.os.UserHandle;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.Arrays;
import java.util.concurrent.TimeUnit;
import java.util.List;
import java.util.Set;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.anyLong;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.verifyZeroInteractions;
import static org.mockito.Mockito.when;

/**
 * Unit-tests for {@link Utils}.
 */
@SmallTest
public class UtilsTest extends AndroidTestCase {
    private static final String TEST_PACKAGE_NAME_1 = "com.test.packagea";
    private static final String TEST_PACKAGE_NAME_2 = "com.test.packageb";
    private static final ComponentName TEST_COMPONENT_NAME = new ComponentName(TEST_PACKAGE_NAME_1,
            ".MainActivity");
    private static final int TEST_USER_ID = 10;

    @Mock private Context mockContext;
    @Mock private AccountManager mockAccountManager;
    @Mock private IPackageManager mockIPackageManager;
    @Mock private PackageManager mockPackageManager;
    @Mock private ConnectivityManager mockConnectivityManager;

    private Utils mUtils;

    @Override
    public void setUp() {
        // this is necessary for mockito to work
        System.setProperty("dexmaker.dexcache", getContext().getCacheDir().toString());

        MockitoAnnotations.initMocks(this);

        when(mockContext.getSystemService(Context.ACCOUNT_SERVICE)).thenReturn(mockAccountManager);
        when(mockContext.getPackageManager()).thenReturn(mockPackageManager);
        when(mockContext.getSystemService(Context.CONNECTIVITY_SERVICE))
                .thenReturn(mockConnectivityManager);

        mUtils = new Utils();
    }

    public void testGetCurrentSystemApps() throws Exception {
        // GIVEN two currently installed apps, one of which is system
        List<ApplicationInfo> appList = Arrays.asList(
                createApplicationInfo(TEST_PACKAGE_NAME_1, false),
                createApplicationInfo(TEST_PACKAGE_NAME_2, true));
        when(mockIPackageManager.getInstalledApplications(
                PackageManager.GET_UNINSTALLED_PACKAGES, TEST_USER_ID))
                .thenReturn(new ParceledListSlice(appList));
        // WHEN requesting the current system apps
        Set<String> res = mUtils.getCurrentSystemApps(mockIPackageManager, TEST_USER_ID);
        // THEN the one system app should be returned
        assertEquals(1, res.size());
        assertTrue(res.contains(TEST_PACKAGE_NAME_2));
    }

    public void testSetComponentEnabledSetting() throws Exception {
        // GIVEN a component name and a user id
        // WHEN disabling a component
        mUtils.setComponentEnabledSetting(mockIPackageManager, TEST_COMPONENT_NAME,
                PackageManager.COMPONENT_ENABLED_STATE_DISABLED, TEST_USER_ID);
        // THEN the correct method on mockIPackageManager gets invoked
        verify(mockIPackageManager).setComponentEnabledSetting(eq(TEST_COMPONENT_NAME),
                eq(PackageManager.COMPONENT_ENABLED_STATE_DISABLED),
                eq(PackageManager.DONT_KILL_APP),
                eq(TEST_USER_ID));
        verifyNoMoreInteractions(mockIPackageManager);
    }

    public void testPackageRequiresUpdate_notPresent() throws Exception {
        // GIVEN that the requested package is not present on the device
        // WHEN checking whether an update is required
        when(mockPackageManager.getPackageInfo(TEST_PACKAGE_NAME_1, 0))
                .thenThrow(new NameNotFoundException());
        // THEN an update is required
        assertTrue(mUtils.packageRequiresUpdate(TEST_PACKAGE_NAME_1, 0, mockContext));
    }

    public void testPackageRequiresUpdate() throws Exception {
        // GIVEN a package that is installed on the device
        PackageInfo pi = new PackageInfo();
        pi.packageName = TEST_PACKAGE_NAME_1;
        pi.versionCode = 1;
        when(mockPackageManager.getPackageInfo(TEST_PACKAGE_NAME_1, 0)).thenReturn(pi);
        // WHEN checking whether an update is required
        // THEN verify that update required returns the correct result depending on the minimum
        // version code requested.
        assertFalse(mUtils.packageRequiresUpdate(TEST_PACKAGE_NAME_1, 0, mockContext));
        assertFalse(mUtils.packageRequiresUpdate(TEST_PACKAGE_NAME_1, 1, mockContext));
        assertTrue(mUtils.packageRequiresUpdate(TEST_PACKAGE_NAME_1, 2, mockContext));
    }

    public void testMaybeCopyAccount_success() throws Exception {
        // GIVEN an account on the source user and a managed profile present and no timeout
        // or error during migration
        Account testAccount = new Account("test@afw-test.com", "com.google");
        UserHandle primaryUser = UserHandle.of(0);
        UserHandle managedProfile = UserHandle.of(10);

        AccountManagerFuture mockResult = mock(AccountManagerFuture.class);
        when(mockResult.getResult(anyLong(), any(TimeUnit.class))).thenReturn(true);
        when(mockAccountManager.copyAccountToUser(any(Account.class), any(UserHandle.class),
                any(UserHandle.class), any(AccountManagerCallback.class), any(Handler.class)))
                .thenReturn(mockResult);

        // WHEN copying the account from the source user to the target user
        // THEN the account migration succeeds
        assertTrue(mUtils.maybeCopyAccount(mockContext, testAccount, primaryUser, managedProfile));
        verify(mockAccountManager).copyAccountToUser(eq(testAccount), eq(primaryUser),
                eq(managedProfile), any(AccountManagerCallback.class), any(Handler.class));
        verify(mockResult).getResult(anyLong(), any(TimeUnit.class));
    }

    public void testMaybeCopyAccount_error() throws Exception {
        // GIVEN an account on the source user and a target user present and an error occurs
        // during migration
        Account testAccount = new Account("test@afw-test.com", "com.google");
        UserHandle primaryUser = UserHandle.of(0);
        UserHandle managedProfile = UserHandle.of(10);

        AccountManagerFuture mockResult = mock(AccountManagerFuture.class);
        when(mockResult.getResult(anyLong(), any(TimeUnit.class))).thenReturn(false);
        when(mockAccountManager.copyAccountToUser(any(Account.class), any(UserHandle.class),
                any(UserHandle.class), any(AccountManagerCallback.class), any(Handler.class)))
                .thenReturn(mockResult);

        // WHEN copying the account from the source user to the target user
        // THEN the account migration fails
        assertFalse(mUtils.maybeCopyAccount(mockContext, testAccount, primaryUser, managedProfile));
        verify(mockAccountManager).copyAccountToUser(eq(testAccount), eq(primaryUser),
                eq(managedProfile), any(AccountManagerCallback.class), any(Handler.class));
        verify(mockResult).getResult(anyLong(), any(TimeUnit.class));
    }

    public void testMaybeCopyAccount_timeout() throws Exception {
        // GIVEN an account on the source user and a target user present and a timeout occurs
        // during migration
        Account testAccount = new Account("test@afw-test.com", "com.google");
        UserHandle primaryUser = UserHandle.of(0);
        UserHandle managedProfile = UserHandle.of(10);

        AccountManagerFuture mockResult = mock(AccountManagerFuture.class);
        // the AccountManagerFuture throws an OperationCanceledException after timeout
        when(mockResult.getResult(anyLong(), any(TimeUnit.class)))
                .thenThrow(new OperationCanceledException());
        when(mockAccountManager.copyAccountToUser(any(Account.class), any(UserHandle.class),
                any(UserHandle.class), any(AccountManagerCallback.class), any(Handler.class)))
                .thenReturn(mockResult);

        // WHEN copying the account from the source user to the target user
        // THEN the account migration fails
        assertFalse(mUtils.maybeCopyAccount(mockContext, testAccount, primaryUser, managedProfile));
        verify(mockAccountManager).copyAccountToUser(eq(testAccount), eq(primaryUser),
                eq(managedProfile), any(AccountManagerCallback.class), any(Handler.class));
        verify(mockResult).getResult(anyLong(), any(TimeUnit.class));
    }

    public void testMaybeCopyAccount_nullAccount() {
        // GIVEN a device with two users present
        UserHandle primaryUser = UserHandle.of(0);
        UserHandle managedProfile = UserHandle.of(10);

        // WHEN trying to copy a null account from the source user to the target user
        // THEN request is ignored
        assertFalse(mUtils.maybeCopyAccount(mockContext, null /* accountToMigrate */, primaryUser,
                managedProfile));
        verifyZeroInteractions(mockAccountManager);
    }

    public void testMaybeCopyAccount_sameUser() {
        // GIVEN an account on a user
        Account testAccount = new Account("test@afw-test.com", "com.google");
        UserHandle primaryUser = UserHandle.of(0);

        // WHEN trying to invoke copying an account with the same user id for source and target user
        // THEN request is ignored
        assertFalse(mUtils.maybeCopyAccount(mockContext, testAccount, primaryUser, primaryUser));
        verifyZeroInteractions(mockAccountManager);
    }

    public void testIsConnectedToNetwork() throws Exception {
        // GIVEN the device is currently connected to mobile network
        setCurrentNetworkMock(ConnectivityManager.TYPE_MOBILE, true);
        // WHEN checking connectivity
        // THEN utils should return true
        assertTrue(mUtils.isConnectedToNetwork(mockContext));

        // GIVEN the device is currently connected to wifi
        setCurrentNetworkMock(ConnectivityManager.TYPE_WIFI, true);
        // WHEN checking connectivity
        // THEN utils should return true
        assertTrue(mUtils.isConnectedToNetwork(mockContext));

        // GIVEN the device is currently disconnected on wifi
        setCurrentNetworkMock(ConnectivityManager.TYPE_WIFI, false);
        // WHEN checking connectivity
        // THEN utils should return false
        assertFalse(mUtils.isConnectedToNetwork(mockContext));
    }

    public void testIsConnectedToWifi() throws Exception {
        // GIVEN the device is currently connected to mobile network
        setCurrentNetworkMock(ConnectivityManager.TYPE_MOBILE, true);
        // WHEN checking whether connected to wifi
        // THEN utils should return false
        assertFalse(mUtils.isConnectedToWifi(mockContext));

        // GIVEN the device is currently connected to wifi
        setCurrentNetworkMock(ConnectivityManager.TYPE_WIFI, true);
        // WHEN checking whether connected to wifi
        // THEN utils should return true
        assertTrue(mUtils.isConnectedToWifi(mockContext));

        // GIVEN the device is currently disconnected on wifi
        setCurrentNetworkMock(ConnectivityManager.TYPE_WIFI, false);
        // WHEN checking whether connected to wifi
        // THEN utils should return false
        assertFalse(mUtils.isConnectedToWifi(mockContext));
    }

    public void testCurrentLauncherSupportsManagedProfiles_noLauncherSet() throws Exception {
        // GIVEN there currently is no default launcher set
        when(mockPackageManager.resolveActivity(any(Intent.class), anyInt()))
                .thenReturn(null);
        // WHEN checking whether the current launcher support managed profiles
        // THEN utils should return false
        assertFalse(mUtils.currentLauncherSupportsManagedProfiles(mockContext));
    }

    public void testCurrentLauncherSupportsManagedProfiles() throws Exception {
        // GIVEN the current default launcher is built against lollipop
        setLauncherMock(Build.VERSION_CODES.LOLLIPOP);
        // WHEN checking whether the current launcher support managed profiles
        // THEN utils should return true
        assertTrue(mUtils.currentLauncherSupportsManagedProfiles(mockContext));

        // GIVEN the current default launcher is built against kitkat
        setLauncherMock(Build.VERSION_CODES.KITKAT);
        // WHEN checking whether the current launcher support managed profiles
        // THEN utils should return false
        assertFalse(mUtils.currentLauncherSupportsManagedProfiles(mockContext));
    }

    public void testBrightness() {
        assertTrue(mUtils.isBrightColor(Color.WHITE));
        assertTrue(mUtils.isBrightColor(Color.YELLOW));
        assertFalse(mUtils.isBrightColor(Color.BLACK));
        assertFalse(mUtils.isBrightColor(Color.BLUE));
    }

    private ApplicationInfo createApplicationInfo(String packageName, boolean system) {
        ApplicationInfo ai = new ApplicationInfo();
        ai.packageName = packageName;
        if (system) {
            ai.flags = ApplicationInfo.FLAG_SYSTEM;
        }
        return ai;
    }

    private void setCurrentNetworkMock(int type, boolean connected) {
        NetworkInfo networkInfo = new NetworkInfo(type, 0, null, null);
        networkInfo.setDetailedState(
                connected ? NetworkInfo.DetailedState.CONNECTED
                        : NetworkInfo.DetailedState.DISCONNECTED,
                null, null);
        when(mockConnectivityManager.getActiveNetworkInfo()).thenReturn(networkInfo);
    }

    private void setLauncherMock(int targetSdkVersion) throws Exception {
        ApplicationInfo appInfo = new ApplicationInfo();
        appInfo.targetSdkVersion = targetSdkVersion;
        ActivityInfo actInfo = new ActivityInfo();
        actInfo.packageName = TEST_PACKAGE_NAME_1;
        ResolveInfo resInfo = new ResolveInfo();
        resInfo.activityInfo = actInfo;

        when(mockPackageManager.resolveActivity(any(Intent.class), anyInt())).thenReturn(resInfo);
        when(mockPackageManager.getApplicationInfo(TEST_PACKAGE_NAME_1, 0)).thenReturn(appInfo);
    }
}
