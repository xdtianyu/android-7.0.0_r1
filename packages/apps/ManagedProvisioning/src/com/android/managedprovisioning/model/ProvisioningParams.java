/*
 * Copyright 2016, The Android Open Source Project
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

package com.android.managedprovisioning.model;

import static com.android.internal.util.Preconditions.checkArgument;
import static com.android.internal.util.Preconditions.checkNotNull;


import android.accounts.Account;
import android.content.Context;
import android.content.ComponentName;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.PersistableBundle;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import java.util.Arrays;
import java.util.Locale;
import java.util.Objects;
import java.util.Set;

import com.android.internal.annotations.Immutable;
import com.android.managedprovisioning.common.IllegalProvisioningArgumentException;
import com.android.managedprovisioning.common.Utils;

/**
 * Provisioning parameters for Device Owner and Profile Owner provisioning.
 */
public final class ProvisioningParams implements Parcelable {
    public static final long DEFAULT_LOCAL_TIME = -1;
    public static final Integer DEFAULT_MAIN_COLOR = null;
    public static final boolean DEFAULT_STARTED_BY_TRUSTED_SOURCE = false;
    public static final boolean DEFAULT_LEAVE_ALL_SYSTEM_APPS_ENABLED = false;
    public static final boolean DEFAULT_EXTRA_PROVISIONING_SKIP_ENCRYPTION = false;
    public static final boolean DEFAULT_SKIP_USER_SETUP = true;
    // Intent extra used internally for passing data between activities and service.
    public static final String EXTRA_PROVISIONING_PARAMS = "provisioningParams";

    public static final Parcelable.Creator<ProvisioningParams> CREATOR
            = new Parcelable.Creator<ProvisioningParams>() {
        @Override
        public ProvisioningParams createFromParcel(Parcel in) {
            return new ProvisioningParams(in);
        }

        @Override
        public ProvisioningParams[] newArray(int size) {
            return new ProvisioningParams[size];
        }
    };

    @Nullable
    public final String timeZone;

    public final long localTime;

    @Nullable
    public final Locale locale;

    /** WiFi configuration. */
    @Nullable
    public final WifiInfo wifiInfo;

    /**
     * Package name of the device admin package.
     *
     * <p>At least one one of deviceAdminPackageName and deviceAdminComponentName should be
     * non-null.
     */
    @Deprecated
    public final String deviceAdminPackageName;

    /**
     * {@link ComponentName} of the device admin package.
     *
     * <p>At least one one of deviceAdminPackageName and deviceAdminComponentName should be
     * non-null.
     */
    public final ComponentName deviceAdminComponentName;

    /** {@link Account} that should be migrated to the managed profile. */
    @Nullable
    public final Account accountToMigrate;

    /** Provisioning action comes along with the provisioning data. */
    public final String provisioningAction;

    /**
     * The main color theme used in managed profile only.
     *
     * <p>{@code null} means the default value.
     */
    @Nullable
    public final Integer mainColor;

    /** The download information of device admin package. */
    @Nullable
    public final PackageDownloadInfo deviceAdminDownloadInfo;

    /**
     * Custom key-value pairs from enterprise mobility management which are passed to device admin
     * package after provisioning.
     *
     * <p>Note that {@link ProvisioningParams} is not immutable because this field is mutable.
     */
    @Nullable
    public final PersistableBundle adminExtrasBundle;

    /**
     * True iff provisioning flow was started by a trusted app. This includes Nfc bump and QR code.
     */
    public final boolean startedByTrustedSource;

    /** True if all system apps should be enabled after provisioning. */
    public final boolean leaveAllSystemAppsEnabled;

    /** True if device encryption should be skipped. */
    public final boolean skipEncryption;

    /** True if user setup can be skipped. */
    public final boolean skipUserSetup;

    // TODO (stevenckng): This shouldn't belong here. Remove this logic from ProvisioningParams.
    private ComponentName inferedDeviceAdminComponentName;

    private final Utils mUtils = new Utils();

    public String inferDeviceAdminPackageName() {
        if (deviceAdminComponentName != null) {
            return deviceAdminComponentName.getPackageName();
        }
        return deviceAdminPackageName;
    }

    // This should not be called if the app has not been installed yet.
    public ComponentName inferDeviceAdminComponentName(Context c)
            throws IllegalProvisioningArgumentException {
        if (inferedDeviceAdminComponentName == null) {
            inferedDeviceAdminComponentName = mUtils.findDeviceAdmin(
                    deviceAdminPackageName, deviceAdminComponentName, c);
        }
        return inferedDeviceAdminComponentName;
    }

    private ProvisioningParams(Builder builder) {
        timeZone = builder.mTimeZone;
        localTime = builder.mLocalTime;
        locale = builder.mLocale;

        wifiInfo = builder.mWifiInfo;

        deviceAdminComponentName = builder.mDeviceAdminComponentName;
        deviceAdminPackageName = builder.mDeviceAdminPackageName;

        deviceAdminDownloadInfo = builder.mDeviceAdminDownloadInfo;

        adminExtrasBundle = builder.mAdminExtrasBundle;

        startedByTrustedSource = builder.mStartedByTrustedSource;
        leaveAllSystemAppsEnabled = builder.mLeaveAllSystemAppsEnabled;
        skipEncryption = builder.mSkipEncryption;
        accountToMigrate = builder.mAccountToMigrate;
        provisioningAction = checkNotNull(builder.mProvisioningAction);
        mainColor = builder.mMainColor;
        skipUserSetup = builder.mSkipUserSetup;

        validateFields();
    }

    private ProvisioningParams(Parcel in) {
        timeZone = in.readString();
        localTime = in.readLong();
        locale = (Locale) in.readSerializable();

        wifiInfo = (WifiInfo) in.readParcelable(WifiInfo.class.getClassLoader());

        deviceAdminPackageName = in.readString();
        deviceAdminComponentName = (ComponentName)
                in.readParcelable(null /* use default classloader */);

        deviceAdminDownloadInfo =
                (PackageDownloadInfo) in.readParcelable(PackageDownloadInfo.class.getClassLoader());

        adminExtrasBundle = in.readParcelable(null /* use default classloader */);

        startedByTrustedSource = in.readInt() == 1;
        leaveAllSystemAppsEnabled = in.readInt() == 1;
        skipEncryption = in.readInt() == 1;
        accountToMigrate = (Account) in.readParcelable(null /* use default classloader */);
        provisioningAction = checkNotNull(in.readString());
        if (in.readInt() != 0) {
            mainColor = in.readInt();
        } else {
            mainColor = null;
        }
        skipUserSetup = in.readInt() == 1;

        validateFields();
    }

    private void validateFields() {
        checkArgument(deviceAdminPackageName != null || deviceAdminComponentName != null);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeString(timeZone);
        out.writeLong(localTime);
        out.writeSerializable(locale);

        out.writeParcelable(wifiInfo, 0 /* default */ );

        out.writeString(deviceAdminPackageName);
        out.writeParcelable(deviceAdminComponentName, 0 /* default */);

        out.writeParcelable(deviceAdminDownloadInfo, 0 /* default */);

        out.writeParcelable(adminExtrasBundle, 0 /* default */);

        out.writeInt(startedByTrustedSource ? 1 : 0);
        out.writeInt(leaveAllSystemAppsEnabled ? 1 : 0);
        out.writeInt(skipEncryption ? 1 : 0);
        out.writeParcelable(accountToMigrate, 0 /* default */);
        out.writeString(provisioningAction);
        if (mainColor != null) {
            out.writeInt(1);
            out.writeInt(mainColor);
        } else {
            out.writeInt(0);
        }
        out.writeInt(skipUserSetup ? 1 : 0);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }
        ProvisioningParams that = (ProvisioningParams) o;
        return localTime == that.localTime
                && startedByTrustedSource == that.startedByTrustedSource
                && leaveAllSystemAppsEnabled == that.leaveAllSystemAppsEnabled
                && skipEncryption == that.skipEncryption
                && skipUserSetup == that.skipUserSetup
                && Objects.equals(timeZone, that.timeZone)
                && Objects.equals(locale, that.locale)
                && Objects.equals(wifiInfo, that.wifiInfo)
                && Objects.equals(deviceAdminPackageName, that.deviceAdminPackageName)
                && Objects.equals(deviceAdminComponentName, that.deviceAdminComponentName)
                && Objects.equals(accountToMigrate, that.accountToMigrate)
                && Objects.equals(provisioningAction, that.provisioningAction)
                && Objects.equals(mainColor, that.mainColor)
                && Objects.equals(deviceAdminDownloadInfo, that.deviceAdminDownloadInfo)
                && isPersistableBundleEquals(adminExtrasBundle, that.adminExtrasBundle)
                && Objects.equals(
                        inferedDeviceAdminComponentName, that.inferedDeviceAdminComponentName);
    }

    /**
     * Compares two {@link PersistableBundle} objects are equals.
     */
    private static boolean isPersistableBundleEquals(
            PersistableBundle obj1, PersistableBundle obj2) {
        if (obj1 == obj2) {
            return true;
        }
        if (obj1 == null || obj2 == null || obj1.size() != obj2.size()) {
            return false;
        }
        Set<String> keys = obj1.keySet();
        for (String key : keys) {
            Object val1 = obj1.get(key);
            Object val2 = obj2.get(key);
            if (!isPersistableBundleSupportedValueEquals(val1, val2)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Compares two values which type is supported by {@link PersistableBundle}.
     *
     * <p>If the type isn't supported. The equality is done by {@link Object#equals(Object)}.
     */
    private static boolean isPersistableBundleSupportedValueEquals(Object val1, Object val2) {
        if (val1 == val2) {
            return true;
        } else if (val1 == null || val2 == null || !val1.getClass().equals(val2.getClass())) {
            return false;
        } else if (val1 instanceof PersistableBundle && val2 instanceof PersistableBundle) {
            return isPersistableBundleEquals((PersistableBundle) val1, (PersistableBundle) val2);
        } else if (val1 instanceof int[]) {
            return Arrays.equals((int[]) val1, (int[]) val2);
        } else if (val1 instanceof long[]) {
            return Arrays.equals((long[]) val1, (long[]) val2);
        } else if (val1 instanceof double[]) {
            return Arrays.equals((double[]) val1, (double[]) val2);
        } else if (val1 instanceof boolean[]) {
            return Arrays.equals((boolean[]) val1, (boolean[]) val2);
        } else if (val1 instanceof String[]) {
            return Arrays.equals((String[]) val1, (String[]) val2);
        } else {
            return Objects.equals(val1, val2);
        }
    }

    public final static class Builder {
        private String mTimeZone;
        private long mLocalTime = DEFAULT_LOCAL_TIME;
        private Locale mLocale;
        private WifiInfo mWifiInfo;
        private String mDeviceAdminPackageName;
        private ComponentName mDeviceAdminComponentName;
        private Account mAccountToMigrate;
        private String mProvisioningAction;
        private Integer mMainColor = DEFAULT_MAIN_COLOR;
        private PackageDownloadInfo mDeviceAdminDownloadInfo;
        private PersistableBundle mAdminExtrasBundle;
        private boolean mStartedByTrustedSource = DEFAULT_STARTED_BY_TRUSTED_SOURCE;
        private boolean mLeaveAllSystemAppsEnabled = DEFAULT_LEAVE_ALL_SYSTEM_APPS_ENABLED;
        private boolean mSkipEncryption = DEFAULT_EXTRA_PROVISIONING_SKIP_ENCRYPTION;
        private boolean mSkipUserSetup = DEFAULT_SKIP_USER_SETUP;

        public Builder setTimeZone(String timeZone) {
            mTimeZone = timeZone;
            return this;
        }

        public Builder setLocalTime(long localTime) {
            mLocalTime = localTime;
            return this;
        }

        public Builder setLocale(Locale locale) {
            mLocale = locale;
            return this;
        }

        public Builder setWifiInfo(WifiInfo wifiInfo) {
            mWifiInfo = wifiInfo;
            return this;
        }

        @Deprecated
        public Builder setDeviceAdminPackageName(String deviceAdminPackageName) {
            mDeviceAdminPackageName = deviceAdminPackageName;
            return this;
        }

        public Builder setDeviceAdminComponentName(ComponentName deviceAdminComponentName) {
            mDeviceAdminComponentName = deviceAdminComponentName;
            return this;
        }

        public Builder setAccountToMigrate(Account accountToMigrate) {
            mAccountToMigrate = accountToMigrate;
            return this;
        }

        public Builder setProvisioningAction(String provisioningAction) {
            mProvisioningAction = provisioningAction;
            return this;
        }

        public Builder setMainColor(Integer mainColor) {
            mMainColor = mainColor;
            return this;
        }

        public Builder setDeviceAdminDownloadInfo(PackageDownloadInfo deviceAdminDownloadInfo) {
            mDeviceAdminDownloadInfo = deviceAdminDownloadInfo;
            return this;
        }

        public Builder setAdminExtrasBundle(PersistableBundle adminExtrasBundle) {
            mAdminExtrasBundle = adminExtrasBundle;
            return this;
        }

        public Builder setStartedByTrustedSource(boolean startedByTrustedSource) {
            mStartedByTrustedSource = startedByTrustedSource;
            return this;
        }

        public Builder setLeaveAllSystemAppsEnabled(boolean leaveAllSystemAppsEnabled) {
            mLeaveAllSystemAppsEnabled = leaveAllSystemAppsEnabled;
            return this;
        }

        public Builder setSkipEncryption(boolean skipEncryption) {
            mSkipEncryption = skipEncryption;
            return this;
        }

        public Builder setSkipUserSetup(boolean skipUserSetup) {
            mSkipUserSetup = skipUserSetup;
            return this;
        }

        public ProvisioningParams build() {
            return new ProvisioningParams(this);
        }

        public static Builder builder() {
            return new Builder();
        }
    }
}
