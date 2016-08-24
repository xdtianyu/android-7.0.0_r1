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

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.IPackageInstallObserver;
import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;
import android.net.Uri;
import android.os.Bundle;
import android.test.AndroidTestCase;
import android.test.mock.MockContentProvider;
import android.test.mock.MockContentResolver;
import android.test.suitebuilder.annotation.SmallTest;

import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

public class InstallPackageTaskTest extends AndroidTestCase {
    private static final String TEST_PACKAGE_NAME = "com.android.test";
    private static final String TEST_PACKAGE_LOCATION = "/sdcard/TestPackage.apk";

    private static final String PACKAGE_NAME = InstallPackageTask.class.getPackage().getName();

    private Context mContext;
    private PackageManager mPackageManager;
    private InstallPackageTask.Callback mCallback;
    private InstallPackageTask mTask;
    private PackageInfo mPackageInfo;
    private ActivityInfo mActivityInfo;
    private MockContentResolver mContentResolver;
    private MockContentProvider mContentProvider;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        // this is necessary for mockito to work
        System.setProperty("dexmaker.dexcache", getContext().getCacheDir().toString());

        mContext = mock(Context.class);
        mActivityInfo = new ActivityInfo();
        mPackageInfo = new PackageInfo();
        mPackageManager = mock(PackageManager.class);
        mContentProvider = new MyMockContentProvider();
        mContentResolver = new MockContentResolver();

        mActivityInfo.permission = android.Manifest.permission.BIND_DEVICE_ADMIN;

        mPackageInfo.packageName = TEST_PACKAGE_NAME;
        mPackageInfo.receivers = new ActivityInfo[] {mActivityInfo};

        mCallback = mock(InstallPackageTask.Callback.class);

        when(mPackageManager.getPackageArchiveInfo(eq(TEST_PACKAGE_LOCATION), anyInt())).
                thenReturn(mPackageInfo);

        mContentResolver.addProvider("settings", mContentProvider);

        when(mContext.getPackageManager()).thenReturn(mPackageManager);
        when(mContext.getPackageName()).thenReturn(PACKAGE_NAME);
        when(mContext.getContentResolver()).thenReturn(mContentResolver);

        mTask = new InstallPackageTask(mContext, mCallback);
    }

    @SmallTest
    public void testInstall_NoPackages() {
        mTask.run();
        verify(mPackageManager, never()).installPackage(
                any(Uri.class),
                any(IPackageInstallObserver.class),
                anyInt(),
                anyString());
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testInstall_OnePackage() {
        mTask.addInstallIfNecessary(TEST_PACKAGE_NAME, TEST_PACKAGE_LOCATION);
        Answer installerAnswer = new Answer() {
                @Override
                public Object answer(InvocationOnMock invocation) throws Throwable {
                    Object[] arguments = invocation.getArguments();
                    IPackageInstallObserver observer = (IPackageInstallObserver)
                            arguments[1];
                    int flags = (Integer) arguments[2];
                    // make sure that the flags value has been set
                    assertTrue(0 != (flags & PackageManager.INSTALL_REPLACE_EXISTING));
                    observer.packageInstalled(TEST_PACKAGE_NAME,
                            PackageManager.INSTALL_SUCCEEDED);
                    return null;
                }
        };
        doAnswer(installerAnswer).when(mPackageManager).installPackage(
                eq(Uri.parse("file://" + TEST_PACKAGE_LOCATION)),
                any(IPackageInstallObserver.class),
                anyInt(),
                eq(PACKAGE_NAME));
        mTask.run();
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testInstall_InstallFailedVersionDowngrade() {
        mTask.addInstallIfNecessary(TEST_PACKAGE_NAME, TEST_PACKAGE_LOCATION);
        Answer installerAnswer = new Answer() {
                @Override
                public Object answer(InvocationOnMock invocation) throws Throwable {
                    Object[] arguments = invocation.getArguments();
                    IPackageInstallObserver observer = (IPackageInstallObserver)
                            arguments[1];
                    observer.packageInstalled(null,
                            PackageManager.INSTALL_FAILED_VERSION_DOWNGRADE);
                    return null;
                }
        };
        doAnswer(installerAnswer).when(mPackageManager).installPackage(
                eq(Uri.parse("file://" + TEST_PACKAGE_LOCATION)),
                any(IPackageInstallObserver.class),
                anyInt(),
                eq(PACKAGE_NAME));
        mTask.run();
        verify(mCallback, times(1)).onSuccess();
    }

    @SmallTest
    public void testPackageHasNoReceivers() {
        mPackageInfo.receivers = null;
        mTask.addInstallIfNecessary(TEST_PACKAGE_NAME, TEST_PACKAGE_LOCATION);
        mTask.run();
        verify(mPackageManager, never()).installPackage(
                any(Uri.class),
                any(IPackageInstallObserver.class),
                anyInt(),
                anyString());
        verify(mCallback, times(1)).onError(InstallPackageTask.ERROR_PACKAGE_INVALID);
    }

    private static class MyMockContentProvider extends MockContentProvider {
        public MyMockContentProvider() {
        }

        @Override
        public Bundle call(String method, String request, Bundle args) {
            return new Bundle();
        }
    }
}
