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

package com.android.managedprovisioning.task;

import android.content.ContentResolver;
import android.content.Context;
import android.provider.Settings;

import static android.provider.Settings.Secure.MANAGED_PROFILE_CONTACT_REMOTE_SEARCH;

public class ManagedProfileSettingsTask {

    private final int mUserId;
    private final ContentResolver mContentResolver;

    public ManagedProfileSettingsTask(Context context, int userId) {
        mContentResolver = context.getContentResolver();
        mUserId = userId;
    }

    public void run() {
        // Turn on managed profile contacts remote search.
        Settings.Secure.putIntForUser(mContentResolver, MANAGED_PROFILE_CONTACT_REMOTE_SEARCH,
                1, mUserId);
    }
}
