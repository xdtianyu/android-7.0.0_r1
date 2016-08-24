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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConfiguration.KeyMgmt;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.internal.R;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.io.File;
import java.lang.reflect.Method;

/**
 * Unit tests for {@link com.android.server.wifi.WifiApConfigStore}.
 */
@SmallTest
public class WifiApConfigStoreTest {

    private static final String TAG = "WifiApConfigStoreTest";

    private static final String TEST_AP_CONFIG_FILE_PREFIX = "APConfig_";
    private static final String TEST_DEFAULT_2G_CHANNEL_LIST = "1,2,3,4,5,6";
    private static final String TEST_DEFAULT_AP_SSID = "TestAP";
    private static final String TEST_CONFIGURED_AP_SSID = "ConfiguredAP";

    @Mock Context mContext;
    @Mock BackupManagerProxy mBackupManagerProxy;
    File mApConfigFile;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        /* Create a temporary file for AP config file storage. */
        mApConfigFile = File.createTempFile(TEST_AP_CONFIG_FILE_PREFIX, "");

        /* Setup expectations for Resources to return some default settings. */
        MockResources resources = new MockResources();
        resources.setString(R.string.config_wifi_framework_sap_2G_channel_list,
                            TEST_DEFAULT_2G_CHANNEL_LIST);
        resources.setString(R.string.wifi_tether_configure_ssid_default,
                            TEST_DEFAULT_AP_SSID);
        when(mContext.getResources()).thenReturn(resources);
    }

    @After
    public void cleanUp() {
        /* Remove the temporary AP config file. */
        mApConfigFile.delete();
    }

    /**
     * Generate a WifiConfiguration based on the specified parameters.
     */
    private WifiConfiguration setupApConfig(
            String ssid, String preSharedKey, int keyManagement, int band, int channel) {
        WifiConfiguration config = new WifiConfiguration();
        config.SSID = ssid;
        config.preSharedKey = preSharedKey;
        config.allowedKeyManagement.set(keyManagement);
        config.apBand = band;
        config.apChannel = channel;
        return config;
    }

    private void writeApConfigFile(WifiConfiguration config) throws Exception {
        Method m = WifiApConfigStore.class.getDeclaredMethod(
                "writeApConfiguration", String.class, WifiConfiguration.class);
        m.setAccessible(true);
        m.invoke(null, mApConfigFile.getPath(), config);
    }

    private void verifyApConfig(WifiConfiguration config1, WifiConfiguration config2) {
        assertEquals(config1.SSID, config2.SSID);
        assertEquals(config1.preSharedKey, config2.preSharedKey);
        assertEquals(config1.getAuthType(), config2.getAuthType());
        assertEquals(config1.apBand, config2.apBand);
        assertEquals(config1.apChannel, config2.apChannel);
    }

    private void verifyDefaultApConfig(WifiConfiguration config) {
        assertEquals(TEST_DEFAULT_AP_SSID, config.SSID);
        assertTrue(config.allowedKeyManagement.get(KeyMgmt.WPA2_PSK));
    }

    /**
     * AP Configuration is not specified in the config file,
     * WifiApConfigStore should fallback to use the default configuration.
     */
    @Test
    public void initWithDefaultConfiguration() throws Exception {
        WifiApConfigStore store = new WifiApConfigStore(
                mContext, mBackupManagerProxy, mApConfigFile.getPath());
        verifyDefaultApConfig(store.getApConfiguration());
    }

    /**
     * Verify WifiApConfigStore can correctly load the existing configuration
     * from the config file.
     */
    @Test
    public void initWithExistingConfiguration() throws Exception {
        WifiConfiguration expectedConfig = setupApConfig(
                "ConfiguredAP",    /* SSID */
                "randomKey",       /* preshared key */
                KeyMgmt.WPA_EAP,   /* key management */
                1,                 /* AP band (5GHz) */
                40                 /* AP channel */);
        writeApConfigFile(expectedConfig);
        WifiApConfigStore store = new WifiApConfigStore(
                mContext, mBackupManagerProxy, mApConfigFile.getPath());
        verifyApConfig(expectedConfig, store.getApConfiguration());
    }

    /**
     * Verify the handling of setting a null ap configuration.
     * WifiApConfigStore should fallback to the default configuration when
     * null ap configuration is provided.
     */
    @Test
    public void setNullApConfiguration() throws Exception {
        /* Initialize WifiApConfigStore with existing configuration. */
        WifiConfiguration expectedConfig = setupApConfig(
                "ConfiguredAP",    /* SSID */
                "randomKey",       /* preshared key */
                KeyMgmt.WPA_EAP,   /* key management */
                1,                 /* AP band (5GHz) */
                40                 /* AP channel */);
        writeApConfigFile(expectedConfig);
        WifiApConfigStore store = new WifiApConfigStore(
                mContext, mBackupManagerProxy, mApConfigFile.getPath());
        verifyApConfig(expectedConfig, store.getApConfiguration());

        store.setApConfiguration(null);
        verifyDefaultApConfig(store.getApConfiguration());
        verify(mBackupManagerProxy).notifyDataChanged();
    }

    /**
     * Verify AP configuration is correctly updated via setApConfiguration call.
     */
    @Test
    public void updateApConfiguration() throws Exception {
        /* Initialize WifiApConfigStore with default configuration. */
        WifiApConfigStore store = new WifiApConfigStore(
                mContext, mBackupManagerProxy, mApConfigFile.getPath());
        verifyDefaultApConfig(store.getApConfiguration());

        /* Update with a valid configuration. */
        WifiConfiguration expectedConfig = setupApConfig(
                "ConfiguredAP",    /* SSID */
                "randomKey",       /* preshared key */
                KeyMgmt.WPA_EAP,   /* key management */
                1,                 /* AP band (5GHz) */
                40                 /* AP channel */);
        store.setApConfiguration(expectedConfig);
        verifyApConfig(expectedConfig, store.getApConfiguration());
        verify(mBackupManagerProxy).notifyDataChanged();
    }
}
