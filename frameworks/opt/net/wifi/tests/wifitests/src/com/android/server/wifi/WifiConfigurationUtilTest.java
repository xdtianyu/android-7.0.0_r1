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

package com.android.server.wifi;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.pm.UserInfo;
import android.net.wifi.WifiConfiguration;
import android.os.UserHandle;
import android.test.suitebuilder.annotation.SmallTest;

import org.junit.Test;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link com.android.server.wifi.WifiConfigurationUtil}.
 */
@SmallTest
public class WifiConfigurationUtilTest {
    static final int CURRENT_USER_ID = 0;
    static final int CURRENT_USER_MANAGED_PROFILE_USER_ID = 10;
    static final int OTHER_USER_ID = 11;
    static final List<UserInfo> PROFILES = Arrays.asList(
            new UserInfo(CURRENT_USER_ID, "owner", 0),
            new UserInfo(CURRENT_USER_MANAGED_PROFILE_USER_ID, "managed profile", 0));

    /**
     * Test for {@link WifiConfigurationUtil.isVisibleToAnyProfile}.
     */
    @Test
    public void isVisibleToAnyProfile() {
        // Shared network configuration created by another user.
        final WifiConfiguration configuration = new WifiConfiguration();
        configuration.creatorUid = UserHandle.getUid(OTHER_USER_ID, 0);
        assertTrue(WifiConfigurationUtil.isVisibleToAnyProfile(configuration, PROFILES));

        // Private network configuration created by another user.
        configuration.shared = false;
        assertFalse(WifiConfigurationUtil.isVisibleToAnyProfile(configuration, PROFILES));

        // Private network configuration created by the current user.
        configuration.creatorUid = UserHandle.getUid(CURRENT_USER_ID, 0);
        assertTrue(WifiConfigurationUtil.isVisibleToAnyProfile(configuration, PROFILES));

        // Private network configuration created by the current user's managed profile.
        configuration.creatorUid = UserHandle.getUid(CURRENT_USER_MANAGED_PROFILE_USER_ID, 0);
        assertTrue(WifiConfigurationUtil.isVisibleToAnyProfile(configuration, PROFILES));
    }
}
