/*
 * Copyright 2014, The Android Open Source Project
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

import android.app.DownloadManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.IPackageInstallObserver;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.provider.Settings.Global;
import android.text.TextUtils;
import android.Manifest.permission;

import com.android.managedprovisioning.ProvisionLogger;
import com.android.managedprovisioning.model.ProvisioningParams;

import java.io.File;
import java.util.HashSet;
import java.util.Set;

/**
 * Installs all packages that were added. Can install a downloaded apk, or install an existing
 * package which is already installed for a different user.
 * <p>
 * Before installing from a downloaded file, each file is checked to ensure it contains the correct
 * package and admin receiver.
 * </p>
 */
public class InstallPackageTask {
    public static final int ERROR_PACKAGE_INVALID = 0;
    public static final int ERROR_INSTALLATION_FAILED = 1;
    public static final int ERROR_PACKAGE_NAME_INVALID = 2;

    private final Context mContext;
    private final Callback mCallback;

    private PackageManager mPm;
    private int mPackageVerifierEnable;
    private Set<InstallInfo> mPackagesToInstall;

    /**
     * Create an InstallPackageTask. When run, this will attempt to install the device admin
     * packages if it is non-null.
     *
     * {@see #run(String, String)} for more detail on package installation.
     */
    public InstallPackageTask (Context context, Callback callback) {
        mCallback = callback;
        mContext = context;
        mPackagesToInstall = new HashSet<InstallInfo>();
        mPm = mContext.getPackageManager();
    }

    /**
     * Should be called before {@link #run}.
     */
    public void addInstallIfNecessary(String packageName, String packageLocation) {
        if (!TextUtils.isEmpty(packageName)) {
            mPackagesToInstall.add(new InstallInfo(packageName, packageLocation));
        }
    }

    /**
     * Install all packages given by {@link #addPackageToInstall}. Each package will be installed
     * from the given location if one is provided. If a null or empty location is provided, and the
     * package is installed for a different user, it will be enabled for the calling user. If the
     * package location is not provided and the package is not installed for any other users, this
     * task will produce an error.
     *
     * Errors will be indicated if a downloaded package is invalid, or installation fails.
     */
    public void run() {
        if (mPackagesToInstall.size() == 0) {
            ProvisionLogger.loge("No downloaded packages to install");
            mCallback.onSuccess();
            return;
        }
        ProvisionLogger.logi("Installing package(s)");

        for (InstallInfo info : mPackagesToInstall) {
            if (TextUtils.isEmpty(info.location)) {
                installExistingPackage(info);

            } else if (packageContentIsCorrect(info.packageName, info.location)) {
                // Temporarily turn off package verification.
                mPackageVerifierEnable = Global.getInt(mContext.getContentResolver(),
                        Global.PACKAGE_VERIFIER_ENABLE, 1);
                Global.putInt(mContext.getContentResolver(), Global.PACKAGE_VERIFIER_ENABLE, 0);

                // Allow for replacing an existing package.
                // Needed in case this task is performed multiple times.
                mPm.installPackage(Uri.parse("file://" + info.location),
                        new PackageInstallObserver(info),
                        /* flags */ PackageManager.INSTALL_REPLACE_EXISTING,
                        mContext.getPackageName());
            } else {
                // Error should have been reported in packageContentIsCorrect().
                return;
            }
        }
    }

    private boolean packageContentIsCorrect(String packageName, String packageLocation) {
        PackageInfo pi = mPm.getPackageArchiveInfo(packageLocation, PackageManager.GET_RECEIVERS);
        if (pi == null) {
            ProvisionLogger.loge("Package could not be parsed successfully.");
            mCallback.onError(ERROR_PACKAGE_INVALID);
            return false;
        }
        if (!pi.packageName.equals(packageName)) {
            ProvisionLogger.loge("Package name in apk (" + pi.packageName
                    + ") does not match package name specified by programmer ("
                    + packageName + ").");
            mCallback.onError(ERROR_PACKAGE_INVALID);
            return false;
        }
        if (pi.receivers != null) {
            for (ActivityInfo ai : pi.receivers) {
                if (!TextUtils.isEmpty(ai.permission) &&
                        ai.permission.equals(android.Manifest.permission.BIND_DEVICE_ADMIN)) {
                    return true;
                }
            }
        }
        ProvisionLogger.loge("Installed package has no admin receiver.");
        mCallback.onError(ERROR_PACKAGE_INVALID);
        return false;
    }

    private class PackageInstallObserver extends IPackageInstallObserver.Stub {
        private final InstallInfo mInstallInfo;

        public PackageInstallObserver(InstallInfo installInfo) {
            mInstallInfo = installInfo;
        }

        @Override
        public void packageInstalled(String packageName, int returnCode) {
            if (packageName != null && !packageName.equals(mInstallInfo.packageName))  {
                ProvisionLogger.loge("Package doesn't have expected package name.");
                mCallback.onError(ERROR_PACKAGE_INVALID);
                return;
            }
            if (returnCode == PackageManager.INSTALL_SUCCEEDED) {
                mInstallInfo.doneInstalling = true;
                ProvisionLogger.logd(
                        "Package " + mInstallInfo.packageName + " is succesfully installed.");
                checkSuccess();
            } else if (returnCode == PackageManager.INSTALL_FAILED_VERSION_DOWNGRADE) {
                mInstallInfo.doneInstalling = true;
                ProvisionLogger.logd("Current version of " + mInstallInfo.packageName
                        + " higher than the version to be installed. It was not reinstalled.");
                checkSuccess();
            } else {
                ProvisionLogger.logd(
                        "Installing package " + mInstallInfo.packageName + " failed.");
                ProvisionLogger.logd(
                        "Errorcode returned by IPackageInstallObserver = " + returnCode);
                mCallback.onError(ERROR_INSTALLATION_FAILED);
            }
            // remove the file containing the apk in order not to use too much space.
            new File(mInstallInfo.location).delete();
        }
    }

    /**
     * Calls the success callback once all of the packages that needed to be installed are
     * successfully installed.
     */
    private void checkSuccess() {
        for (InstallInfo info : mPackagesToInstall) {
            if (!info.doneInstalling) {
                return;
            }
        }
        // Set package verification flag to its original value.
        Global.putInt(mContext.getContentResolver(), Global.PACKAGE_VERIFIER_ENABLE,
                mPackageVerifierEnable);
        mCallback.onSuccess();
    }

    /**
     * Attempt to install this package from an existing package installed under a different user.
     * If this package is already installed for this user, this is a no-op. If it is not installed
     * for another user, this will produce an error.
     * @param info The package to install
     */
    private void installExistingPackage(InstallInfo info) {
        try {
            ProvisionLogger.logi("Installing existing package " + info.packageName);
            mPm.installExistingPackage(info.packageName);
            info.doneInstalling = true;
        } catch (PackageManager.NameNotFoundException e) {
            mCallback.onError(ERROR_PACKAGE_INVALID);
            return;
        }
        checkSuccess();
    }

    public abstract static class Callback {
        public abstract void onSuccess();
        public abstract void onError(int errorCode);
    }

    private static class InstallInfo {
        public String packageName;
        public String location;
        public boolean doneInstalling;

        public InstallInfo(String packageName, String location) {
            this.packageName = packageName;
            this.location = location;
        }
    }
}
