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
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.IPackageManager;
import android.content.pm.PackageManager;
import android.os.RemoteException;
import android.os.UserHandle;

import com.android.internal.annotations.VisibleForTesting;
import com.android.managedprovisioning.ProvisionLogger;

/**
 * This tasks sets a given component as the owner of the device.
 */
public class SetDevicePolicyTask {

    private final Callback mCallback;
    private final Context mContext;
    private String mAdminPackage;
    private ComponentName mAdminComponent;
    private final String mOwnerName;
    private final int mUserId;

    private PackageManager mPackageManager;
    private DevicePolicyManager mDevicePolicyManager;

    public SetDevicePolicyTask(Context context, String ownerName, Callback callback) {
        this(context, ownerName, callback, UserHandle.myUserId());
    }

    @VisibleForTesting
    /* package */ SetDevicePolicyTask(Context context, String ownerName, Callback callback,
            int userId) {
        mCallback = callback;
        mContext = context;
        mOwnerName = ownerName;
        mUserId = userId;

        mPackageManager = mContext.getPackageManager();
        mDevicePolicyManager = (DevicePolicyManager) mContext.
                getSystemService(Context.DEVICE_POLICY_SERVICE);
    }

    public void run(ComponentName adminComponent) {
        boolean success = true;
        try {
            mAdminComponent = adminComponent;
            mAdminPackage = mAdminComponent.getPackageName();

            enableDevicePolicyApp(mAdminPackage);
            setActiveAdmin(mAdminComponent);
            success = setDeviceOwner(mAdminComponent, mOwnerName);
        } catch (Exception e) {
            ProvisionLogger.loge("Failure setting device or profile owner", e);
            mCallback.onError();
            return;
        }
        if (success) {
            mCallback.onSuccess();
        } else {
            ProvisionLogger.loge("Error when setting device or profile owner.");
            mCallback.onError();
        }
    }

    private void enableDevicePolicyApp(String packageName) {
        int enabledSetting = mPackageManager.getApplicationEnabledSetting(packageName);
        if (enabledSetting != PackageManager.COMPONENT_ENABLED_STATE_DEFAULT) {
            mPackageManager.setApplicationEnabledSetting(packageName,
                    PackageManager.COMPONENT_ENABLED_STATE_DEFAULT,
                    // Device policy app may have launched ManagedProvisioning, play nice and don't
                    // kill it as a side-effect of this call.
                    PackageManager.DONT_KILL_APP);
        }
    }

    private void setActiveAdmin(ComponentName component) {
        ProvisionLogger.logd("Setting " + component + " as active admin.");
        mDevicePolicyManager.setActiveAdmin(component, true, mUserId);
    }

    private boolean setDeviceOwner(ComponentName component, String owner) {
        ProvisionLogger.logd("Setting " + component + " as device owner " + owner + ".");
        if (!component.equals(mDevicePolicyManager.getDeviceOwnerComponentOnCallingUser())) {
            return mDevicePolicyManager.setDeviceOwner(component, owner, mUserId);
        }
        return true;
    }

    public abstract static class Callback {
        public abstract void onSuccess();
        public abstract void onError();
    }
}
