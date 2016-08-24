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

package com.android.managedprovisioning.parser;

import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_PROFILE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_SHAREABLE_DEVICE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_USER;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_COMPONENT_NAME;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_MINIMUM_VERSION_CODE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_CHECKSUM;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_COOKIE_HEADER;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_LOCATION;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_SIGNATURE_CHECKSUM;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_LOCALE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_LOCAL_TIME;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_MAIN_COLOR;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_SKIP_ENCRYPTION;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_SKIP_USER_SETUP;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_TIME_ZONE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_HIDDEN;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_PAC_URL;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_PASSWORD;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_PROXY_BYPASS;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_PROXY_HOST;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_PROXY_PORT;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_SECURITY_TYPE;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_WIFI_SSID;
import static android.app.admin.DevicePolicyManager.EXTRA_PROVISIONING_LOGO_URI;
import static com.android.internal.util.Preconditions.checkNotNull;
import static com.android.managedprovisioning.common.Globals.ACTION_RESUME_PROVISIONING;
import static com.android.managedprovisioning.parser.MessageParser.EXTRA_PROVISIONING_ACTION;
import static com.android.managedprovisioning.parser.MessageParser.EXTRA_PROVISIONING_DEVICE_ADMIN_SUPPORT_SHA1_PACKAGE_CHECKSUM;
import static com.android.managedprovisioning.parser.MessageParser.EXTRA_PROVISIONING_STARTED_BY_TRUSTED_SOURCE;

import android.accounts.Account;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.graphics.Color;
import android.os.Bundle;
import android.os.PersistableBundle;
import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;

import com.android.managedprovisioning.LogoUtils;
import com.android.managedprovisioning.ProvisionLogger;
import com.android.managedprovisioning.common.IllegalProvisioningArgumentException;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.PackageDownloadInfo;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.model.WifiInfo;

import java.util.Arrays;
import java.util.HashSet;
import java.util.IllformedLocaleException;
import java.util.Locale;
import java.util.Properties;
import java.util.Set;

/**
 * A parser which parses provisioning data from intent which stores in {@link Bundle} extras.
 */

@VisibleForTesting
public class ExtrasProvisioningDataParser implements ProvisioningDataParser {
    private static final Set<String> PROVISIONING_ACTIONS_SUPPORT_ALL_PROVISIONING_DATA =
            new HashSet(Arrays.asList(
                    ACTION_RESUME_PROVISIONING,
                    ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE));

    private static final Set<String> PROVISIONING_ACTIONS_SUPPORT_MIN_PROVISIONING_DATA =
            new HashSet(Arrays.asList(
                    ACTION_PROVISION_MANAGED_DEVICE,
                    ACTION_PROVISION_MANAGED_SHAREABLE_DEVICE,
                    ACTION_PROVISION_MANAGED_USER,
                    ACTION_PROVISION_MANAGED_PROFILE));

    private final Utils mUtils;

    ExtrasProvisioningDataParser(Utils utils) {
        mUtils = checkNotNull(utils);
    }

    @Override
    public ProvisioningParams parse(Intent provisioningIntent, Context context)
            throws IllegalProvisioningArgumentException{
        String provisioningAction = provisioningIntent.getAction();

        if (PROVISIONING_ACTIONS_SUPPORT_MIN_PROVISIONING_DATA.contains(provisioningAction)) {
            ProvisionLogger.logi("Processing mininalist extras intent.");
            return parseMinimalistSupportedProvisioningDataInternal(provisioningIntent, context)
                    .build();
        } else if (PROVISIONING_ACTIONS_SUPPORT_ALL_PROVISIONING_DATA.contains(
                provisioningAction)) {
            return parseAllSupportedProvisioningData(provisioningIntent, context);
        } else {
            throw new IllegalProvisioningArgumentException("Unsupported provisioning action: "
                    + provisioningAction);
        }
    }

    /**
     * Parses minimal supported set of parameters from bundle extras of a provisioning intent.
     *
     * <p>Here is the list of supported parameters.
     * <ul>
     *     <li>{@link EXTRA_PROVISIONING_DEVICE_ADMIN_COMPONENT_NAME}</li>
     *     <li>
     *         {@link EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME} only in
     *         {@link ACTION_PROVISION_MANAGED_PROFILE}.
     *     </li>
     *     <li>{@link EXTRA_PROVISIONING_LOGO_URI}</li>
     *     <li>{@link EXTRA_PROVISIONING_MAIN_COLOR}</li>
     *     <li>
     *         {@link EXTRA_PROVISIONING_SKIP_USER_SETUP} only in
     *         {@link ACTION_PROVISION_MANAGED_USER} and {@link ACTION_PROVISION_MANAGED_DEVICE}.
     *     </li>
     *     <li>{@link EXTRA_PROVISIONING_SKIP_ENCRYPTION}</li>
     *     <li>{@link EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED}</li>
     *     <li>{@link EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE}</li>
     *     <li>{@link EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE}</li>
     * </ul>
     */
    private ProvisioningParams.Builder parseMinimalistSupportedProvisioningDataInternal(
            Intent intent, Context context)
            throws IllegalProvisioningArgumentException {
        boolean isProvisionManagedDeviceFromTrustedSourceIntent =
                ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE.equals(intent.getAction());
        try {
            String provisioningAction = isResumeProvisioningIntent(intent)
                    ? intent.getStringExtra(EXTRA_PROVISIONING_ACTION)
                    : mUtils.mapIntentToDpmAction(intent);

            // Parse device admin package name and component name.
            ComponentName deviceAdminComponentName = (ComponentName) intent.getParcelableExtra(
                    EXTRA_PROVISIONING_DEVICE_ADMIN_COMPONENT_NAME);
            // Device admin package name is deprecated. It is only supported in Profile Owner
            // provisioning and when resuming NFC provisioning.
            String deviceAdminPackageName = null;
            if (ACTION_PROVISION_MANAGED_PROFILE.equals(provisioningAction)) {
                // In L, we only support package name. This means some DPC may still send us the
                // device admin package name only. Attempts to obtain the package name from extras.
                deviceAdminPackageName = intent.getStringExtra(
                        EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME);
                // For profile owner, the device admin package should be installed. Verify the
                // device admin package.
                deviceAdminComponentName = mUtils.findDeviceAdmin(
                        deviceAdminPackageName, deviceAdminComponentName, context);
                // Since the device admin package must be installed at this point and its component
                // name has been obtained, it should be safe to set the deprecated package name
                // value to null.
                deviceAdminPackageName = null;
            } else if (isResumeProvisioningIntent(intent)) {
                deviceAdminPackageName = intent.getStringExtra(
                        EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_NAME);
            }

            // Parse skip user setup in ACTION_PROVISION_MANAGED_USER and
            // ACTION_PROVISION_MANAGED_DEVICE (sync auth) only. This extra is not supported if
            // provisioning was started by trusted source, as it is not clear where SUW should
            // continue from.
            boolean skipUserSetup = ProvisioningParams.DEFAULT_SKIP_USER_SETUP;
            if (!isProvisionManagedDeviceFromTrustedSourceIntent
                    && (provisioningAction.equals(ACTION_PROVISION_MANAGED_USER)
                            || provisioningAction.equals(ACTION_PROVISION_MANAGED_DEVICE))) {
                skipUserSetup = intent.getBooleanExtra(EXTRA_PROVISIONING_SKIP_USER_SETUP,
                        ProvisioningParams.DEFAULT_SKIP_USER_SETUP);
            }

            // Parse main color and organization's logo. This is not supported in managed device
            // from trusted source provisioning because, currently, there is no way to send
            // organization logo to the device at this stage.
            Integer mainColor = ProvisioningParams.DEFAULT_MAIN_COLOR;
            if (!isProvisionManagedDeviceFromTrustedSourceIntent) {
                if (intent.hasExtra(EXTRA_PROVISIONING_MAIN_COLOR)) {
                    mainColor = intent.getIntExtra(EXTRA_PROVISIONING_MAIN_COLOR, 0 /* not used */);
                }
                parseOrganizationLogoUrlFromExtras(context, intent);
            }

            return ProvisioningParams.Builder.builder()
                    .setProvisioningAction(provisioningAction)
                    .setDeviceAdminComponentName(deviceAdminComponentName)
                    .setDeviceAdminPackageName(deviceAdminPackageName)
                    .setSkipEncryption(intent.getBooleanExtra(EXTRA_PROVISIONING_SKIP_ENCRYPTION,
                            ProvisioningParams.DEFAULT_EXTRA_PROVISIONING_SKIP_ENCRYPTION))
                    .setLeaveAllSystemAppsEnabled(intent.getBooleanExtra(
                            EXTRA_PROVISIONING_LEAVE_ALL_SYSTEM_APPS_ENABLED,
                            ProvisioningParams.DEFAULT_LEAVE_ALL_SYSTEM_APPS_ENABLED))
                    .setAdminExtrasBundle((PersistableBundle) intent.getParcelableExtra(
                            EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE))
                    .setMainColor(mainColor)
                    .setSkipUserSetup(skipUserSetup)
                    .setAccountToMigrate((Account) intent.getParcelableExtra(
                            EXTRA_PROVISIONING_ACCOUNT_TO_MIGRATE));
        } catch (ClassCastException e) {
            throw new IllegalProvisioningArgumentException("Extra "
                    + EXTRA_PROVISIONING_ADMIN_EXTRAS_BUNDLE
                    + " must be of type PersistableBundle.", e);
        } catch (IllegalArgumentException e) {
            throw new IllegalProvisioningArgumentException("Invalid parameter found!", e);
        } catch (NullPointerException e) {
            throw new IllegalProvisioningArgumentException("Compulsory parameter not found!", e);
        }
    }

    /**
     * Parses an intent and return a corresponding {@link ProvisioningParams} object.
     *
     * @param intent intent to be parsed.
     * @param context a context
     */
    private ProvisioningParams parseAllSupportedProvisioningData(Intent intent, Context context)
            throws IllegalProvisioningArgumentException {
        try {
            ProvisionLogger.logi("Processing all supported extras intent: " + intent.getAction());
            return parseMinimalistSupportedProvisioningDataInternal(intent, context)
                    // Parse time zone, local time and locale.
                    .setTimeZone(intent.getStringExtra(EXTRA_PROVISIONING_TIME_ZONE))
                    .setLocalTime(intent.getLongExtra(EXTRA_PROVISIONING_LOCAL_TIME,
                            ProvisioningParams.DEFAULT_LOCAL_TIME))
                    .setLocale(MessageParser.stringToLocale(
                            intent.getStringExtra(EXTRA_PROVISIONING_LOCALE)))
                    // Parse WiFi configuration.
                    .setWifiInfo(parseWifiInfoFromExtras(intent))
                    // Parse device admin package download info.
                    .setDeviceAdminDownloadInfo(parsePackageDownloadInfoFromExtras(intent))
                    // Cases where startedByTrustedSource can be true are
                    // 1. We are reloading a stored provisioning intent, either Nfc bump or
                    //    PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE, after encryption reboot,
                    //    which is a self-originated intent.
                    // 2. the intent is from a trusted source, for example QR provisioning.
                    .setStartedByTrustedSource(isResumeProvisioningIntent(intent)
                            ? intent.getBooleanExtra(EXTRA_PROVISIONING_STARTED_BY_TRUSTED_SOURCE,
                                    ProvisioningParams.DEFAULT_STARTED_BY_TRUSTED_SOURCE)
                            : ACTION_PROVISION_MANAGED_DEVICE_FROM_TRUSTED_SOURCE.equals(
                                    intent.getAction()))
                    .build();
        }  catch (IllegalArgumentException e) {
            throw new IllegalProvisioningArgumentException("Invalid parameter found!", e);
        } catch (NullPointerException e) {
            throw new IllegalProvisioningArgumentException("Compulsory parameter not found!", e);
        }
    }

    /**
     * Parses Wifi configuration from an Intent and returns the result in {@link WifiInfo}.
     */
    @Nullable
    private WifiInfo parseWifiInfoFromExtras(Intent intent) {
        if (intent.getStringExtra(EXTRA_PROVISIONING_WIFI_SSID) == null) {
            return null;
        }
        return WifiInfo.Builder.builder()
                .setSsid(intent.getStringExtra(EXTRA_PROVISIONING_WIFI_SSID))
                .setSecurityType(intent.getStringExtra(EXTRA_PROVISIONING_WIFI_SECURITY_TYPE))
                .setPassword(intent.getStringExtra(EXTRA_PROVISIONING_WIFI_PASSWORD))
                .setProxyHost(intent.getStringExtra(EXTRA_PROVISIONING_WIFI_PROXY_HOST))
                .setProxyBypassHosts(intent.getStringExtra(EXTRA_PROVISIONING_WIFI_PROXY_BYPASS))
                .setPacUrl(intent.getStringExtra(EXTRA_PROVISIONING_WIFI_PAC_URL))
                .setProxyPort(intent.getIntExtra(EXTRA_PROVISIONING_WIFI_PROXY_PORT,
                        WifiInfo.DEFAULT_WIFI_PROXY_PORT))
                .setHidden(intent.getBooleanExtra(EXTRA_PROVISIONING_WIFI_HIDDEN,
                        WifiInfo.DEFAULT_WIFI_HIDDEN))
                .build();
    }

    /**
     * Parses device admin package download info configuration from an Intent and returns the result
     * in {@link PackageDownloadInfo}.
     */
    @Nullable
    private PackageDownloadInfo parsePackageDownloadInfoFromExtras(Intent intent) {
        if (intent.getStringExtra(EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_LOCATION)
                == null) {
            return null;
        }
        PackageDownloadInfo.Builder downloadInfoBuilder = PackageDownloadInfo.Builder.builder()
                .setMinVersion(intent.getIntExtra(
                        EXTRA_PROVISIONING_DEVICE_ADMIN_MINIMUM_VERSION_CODE,
                        PackageDownloadInfo.DEFAULT_MINIMUM_VERSION))
                .setLocation(intent.getStringExtra(
                        EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_LOCATION))
                .setCookieHeader(intent.getStringExtra(
                        EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_DOWNLOAD_COOKIE_HEADER));
        String packageHash =
                intent.getStringExtra(EXTRA_PROVISIONING_DEVICE_ADMIN_PACKAGE_CHECKSUM);
        if (packageHash != null) {
            downloadInfoBuilder.setPackageChecksum(mUtils.stringToByteArray(packageHash));
            if (isResumeProvisioningIntent(intent)) {
                // PackageChecksumSupportsSha1 is only supported in NFC provisioning. But if the
                // device is rebooted after encryption as part of the NFC provisioning flow, the
                // value should be restored.
                downloadInfoBuilder.setPackageChecksumSupportsSha1(
                        intent.getBooleanExtra(
                                EXTRA_PROVISIONING_DEVICE_ADMIN_SUPPORT_SHA1_PACKAGE_CHECKSUM,
                                false));
            }
        }
        String sigHash = intent.getStringExtra(EXTRA_PROVISIONING_DEVICE_ADMIN_SIGNATURE_CHECKSUM);
        if (sigHash != null) {
            downloadInfoBuilder.setSignatureChecksum(mUtils.stringToByteArray(sigHash));
        }
        return downloadInfoBuilder.build();
    }

    /**
     * Parses the organization logo url from intent.
     */
    private void parseOrganizationLogoUrlFromExtras(Context context, Intent intent) {
        Uri logoUri = intent.getParcelableExtra(EXTRA_PROVISIONING_LOGO_URI);
        if (logoUri != null) {
            // If we go through encryption, and if the uri is a content uri:
            // We'll lose the grant to this uri. So we need to save it to a local file.
            LogoUtils.saveOrganisationLogo(context, logoUri);
        } else if (!isResumeProvisioningIntent(intent)) {
            // If the intent is not from managed provisioning app, there is a slight possibility
            // that the logo is still kept on the file system from a previous provisioning. In
            // this case, remove it.
            LogoUtils.cleanUp(context);
        }
    }

    private boolean isResumeProvisioningIntent(Intent intent) {
        return ACTION_RESUME_PROVISIONING.equals(intent.getAction());
    }
}
