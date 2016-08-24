/*
 * Copyright 2014, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.managedprovisioning.task;

import android.app.AppGlobals;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ComponentInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;

import com.android.managedprovisioning.ProvisionLogger;
import com.android.managedprovisioning.common.Utils;

import java.util.List;
import java.util.Set;


/**
 * Disables all system app components that listen to ACTION_INSTALL_SHORTCUT.
 */
public class DisableInstallShortcutListenersTask {
    private final PackageManager mPm;
    private final int mUserId;

    private final Utils mUtils = new Utils();

    public DisableInstallShortcutListenersTask(Context context, int userId) {
        mUserId = userId;
        mPm = context.getPackageManager();
    }

    public void run() {
        ProvisionLogger.logd("Disabling install shortcut listeners.");
        Intent actionShortcut = new Intent("com.android.launcher.action.INSTALL_SHORTCUT");
        Set<String> systemApps = mUtils.getCurrentSystemApps(AppGlobals.getPackageManager(),
                mUserId);
        for (String systemApp : systemApps) {
            actionShortcut.setPackage(systemApp);
            disableReceivers(actionShortcut);
        }
    }

    /**
     * Disable all components that can handle the specified broadcast intent.
     */
    private void disableReceivers(Intent intent) {
        List<ResolveInfo> receivers = mPm.queryBroadcastReceiversAsUser(intent, 0, mUserId);
        for (ResolveInfo ri : receivers) {
            // One of ri.activityInfo, ri.serviceInfo, ri.providerInfo is not null. Let's find which
            // one.
            ComponentInfo ci;
            if (ri.activityInfo != null) {
                ci = ri.activityInfo;
            } else if (ri.serviceInfo != null) {
                ci = ri.serviceInfo;
            } else {
                ci = ri.providerInfo;
            }
            mUtils.disableComponent(new ComponentName(ci.packageName, ci.name), mUserId);
        }
    }
}
