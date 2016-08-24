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

import android.content.Context;
import android.content.pm.UserInfo;
import android.os.UserHandle;
import android.os.UserManager;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.Arrays;
import java.util.Collections;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyBoolean;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyZeroInteractions;
import static org.mockito.Mockito.when;

/**
 * Unit-tests for {@link DisallowAddUserTask}.
 */
public class DisallowAddUserTaskTest extends AndroidTestCase {
    @Mock private Context mockContext;
    @Mock private UserManager mockUserManager;

    // Normal cases.
    private UserInfo primaryUser = new UserInfo(0, "Primary",
            UserInfo.FLAG_PRIMARY | UserInfo.FLAG_ADMIN);

    // Split-system-user cases.
    private UserInfo systemUser = new UserInfo(UserHandle.USER_SYSTEM, "System", 0 /* flags */);
    private UserInfo meatUser = new UserInfo(10, "Primary",
            UserInfo.FLAG_PRIMARY | UserInfo.FLAG_ADMIN);

    @Override
    public void setUp() {
        // this is necessary for mockito to work
        System.setProperty("dexmaker.dexcache", getContext().getCacheDir().toString());

        MockitoAnnotations.initMocks(this);

        // Setup sensible default responses.
        when(mockUserManager.hasUserRestriction(anyString(), any(UserHandle.class)))
                .thenReturn(false);
    }

    @SmallTest
    public void testMaybeDisallowAddUsers_normalSystem() {
        when(mockUserManager.getUsers()).thenReturn(Collections.singletonList(primaryUser));

        new DisallowAddUserTask(mockUserManager, primaryUser.id, false /* isSplitSystemUser */)
                .maybeDisallowAddUsers();

        verify(mockUserManager).setUserRestriction(UserManager.DISALLOW_ADD_USER, true,
                primaryUser.getUserHandle());
    }

    @SmallTest
    public void testMaybeDisallowAddUsers_normalSystem_restrictionAlreadySetupForOneUser() {
        when(mockUserManager.getUsers()).thenReturn(Collections.singletonList(primaryUser));

        when(mockUserManager.hasUserRestriction(UserManager.DISALLOW_ADD_USER,
                primaryUser.getUserHandle()))
                .thenReturn(true);

        new DisallowAddUserTask(mockUserManager, meatUser.id, false /* isSplitSystemUser */)
                .maybeDisallowAddUsers();

        verify(mockUserManager, never()).setUserRestriction(eq(UserManager.DISALLOW_ADD_USER),
                anyBoolean(), eq(primaryUser.getUserHandle()));
    }

    @SmallTest
    public void testMaybeDisallowAddUsers_splitUserSystem_meatUserDeviceOwner() {
        when(mockUserManager.getUsers()).thenReturn(Arrays.asList(new UserInfo[]{
                systemUser, meatUser}));

        new DisallowAddUserTask(mockUserManager, meatUser.id, true /* isSplitSystemUser */)
                .maybeDisallowAddUsers();

        verify(mockUserManager).setUserRestriction(UserManager.DISALLOW_ADD_USER, true,
                systemUser.getUserHandle());
        verify(mockUserManager).setUserRestriction(UserManager.DISALLOW_ADD_USER, true,
                meatUser.getUserHandle());
    }

    @SmallTest
    public void testMaybeDisallowAddUsers_splitUserSystem_systemDeviceOwner() {
        when(mockUserManager.getUsers()).thenReturn(Collections.singletonList(systemUser));

        new DisallowAddUserTask(mockUserManager, systemUser.id, true /* isSplitSystemUser */)
                .maybeDisallowAddUsers();

        verifyZeroInteractions(mockUserManager);
    }
}