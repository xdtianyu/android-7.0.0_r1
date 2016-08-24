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
 * limitations under the License
 */

package com.android.server.wifi;

import static org.hamcrest.CoreMatchers.not;
import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.content.Context;
import android.content.pm.UserInfo;
import android.net.wifi.FakeKeys;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConfiguration.AuthAlgorithm;
import android.net.wifi.WifiConfiguration.GroupCipher;
import android.net.wifi.WifiConfiguration.KeyMgmt;
import android.net.wifi.WifiConfiguration.PairwiseCipher;
import android.net.wifi.WifiConfiguration.Protocol;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiEnterpriseConfig.Eap;
import android.net.wifi.WifiEnterpriseConfig.Phase2;
import android.net.wifi.WifiScanner;
import android.os.Process;
import android.os.UserHandle;
import android.os.UserManager;
import android.security.Credentials;
import android.security.KeyStore;
import android.support.test.InstrumentationRegistry;
import android.test.suitebuilder.annotation.SmallTest;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;

import com.android.server.net.DelayedDiskWrite;
import com.android.server.wifi.MockAnswerUtil.AnswerWithArguments;
import com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager;
import com.android.server.wifi.hotspot2.pps.Credential;
import com.android.server.wifi.hotspot2.pps.HomeSP;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.Field;
import java.math.BigInteger;
import java.security.cert.CertificateEncodingException;
import java.security.cert.X509Certificate;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.BitSet;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.Set;
import java.util.TreeMap;

/**
 * Unit tests for {@link com.android.server.wifi.WifiConfigManager}.
 */
@SmallTest
public class WifiConfigManagerTest {
    private static final List<WifiConfiguration> CONFIGS = Arrays.asList(
            WifiConfigurationTestUtil.generateWifiConfig(
                    0, 1000000, "\"red\"", true, true, null, null),
            WifiConfigurationTestUtil.generateWifiConfig(
                    1, 1000001, "\"green\"", true, true, "example.com", "Green"),
            WifiConfigurationTestUtil.generateWifiConfig(
                    2, 1100000, "\"blue\"", false, true, "example.org", "Blue"),
            WifiConfigurationTestUtil.generateWifiConfig(
                    3, 1200000, "\"cyan\"", false, true, null, null));

    private static final int[] USER_IDS = {0, 10, 11};
    private static final int MANAGED_PROFILE_USER_ID = 12;
    private static final int MANAGED_PROFILE_PARENT_USER_ID = 0;
    private static final SparseArray<List<UserInfo>> USER_PROFILES = new SparseArray<>();
    static {
        USER_PROFILES.put(0, Arrays.asList(new UserInfo(0, "Owner", 0),
                new UserInfo(12, "Managed Profile", 0)));
        USER_PROFILES.put(10, Arrays.asList(new UserInfo(10, "Alice", 0)));
        USER_PROFILES.put(11, Arrays.asList(new UserInfo(11, "Bob", 0)));
    }

    private static final Map<Integer, List<WifiConfiguration>> VISIBLE_CONFIGS = new HashMap<>();
    static {
        for (int userId : USER_IDS) {
            List<WifiConfiguration> configs = new ArrayList<>();
            for (int i = 0; i < CONFIGS.size(); ++i) {
                if (WifiConfigurationUtil.isVisibleToAnyProfile(CONFIGS.get(i),
                        USER_PROFILES.get(userId))) {
                    configs.add(CONFIGS.get(i));
                }
            }
            VISIBLE_CONFIGS.put(userId, configs);
        }
    }

    /**
     * Set of WifiConfigs for HasEverConnected tests.
     */
    private static final int HAS_EVER_CONNECTED_USER = 20;
    private static final WifiConfiguration BASE_HAS_EVER_CONNECTED_CONFIG =
            WifiConfigurationTestUtil.generateWifiConfig(
                    0, HAS_EVER_CONNECTED_USER, "testHasEverConnected", false, true, null, null, 0);

    public static final String TAG = "WifiConfigManagerTest";
    @Mock private Context mContext;
    @Mock private WifiNative mWifiNative;
    @Mock private FrameworkFacade mFrameworkFacade;
    @Mock private UserManager mUserManager;
    @Mock private DelayedDiskWrite mWriter;
    @Mock private PasspointManagementObjectManager mMOManager;
    @Mock private Clock mClock;
    private WifiConfigManager mWifiConfigManager;
    private ConfigurationMap mConfiguredNetworks;
    public byte[] mNetworkHistoryBytes;
    private MockKeyStore mMockKeyStore;
    private KeyStore mKeyStore;

    /**
     * Called before each test
     */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        final Context realContext = InstrumentationRegistry.getContext();
        when(mContext.getPackageName()).thenReturn(realContext.getPackageName());
        when(mContext.getResources()).thenReturn(realContext.getResources());
        when(mContext.getPackageManager()).thenReturn(realContext.getPackageManager());

        when(mUserManager.getProfiles(UserHandle.USER_SYSTEM))
                .thenReturn(USER_PROFILES.get(UserHandle.USER_SYSTEM));

        for (int userId : USER_IDS) {
            when(mUserManager.getProfiles(userId)).thenReturn(USER_PROFILES.get(userId));
        }

        mMockKeyStore = new MockKeyStore();

        mWifiConfigManager = new WifiConfigManager(mContext, mWifiNative, mFrameworkFacade, mClock,
                mUserManager, mMockKeyStore.createMock());

        final Field configuredNetworksField =
                WifiConfigManager.class.getDeclaredField("mConfiguredNetworks");
        configuredNetworksField.setAccessible(true);
        mConfiguredNetworks = (ConfigurationMap) configuredNetworksField.get(mWifiConfigManager);

        // Intercept writes to networkHistory.txt.
        doAnswer(new AnswerWithArguments() {
            public void answer(String filePath, DelayedDiskWrite.Writer writer) throws Exception {
                final ByteArrayOutputStream buffer = new ByteArrayOutputStream();
                final DataOutputStream stream = new DataOutputStream(buffer);
                writer.onWriteCalled(stream);
                stream.close();
                mNetworkHistoryBytes = buffer.toByteArray();
            }}).when(mWriter).write(anyString(), (DelayedDiskWrite.Writer) anyObject());
        final Field writerField = WifiConfigManager.class.getDeclaredField("mWriter");
        writerField.setAccessible(true);
        writerField.set(mWifiConfigManager, mWriter);
        final Field networkHistoryField =
                WifiConfigManager.class.getDeclaredField("mWifiNetworkHistory");
        networkHistoryField.setAccessible(true);
        WifiNetworkHistory wifiNetworkHistory =
                (WifiNetworkHistory) networkHistoryField.get(mWifiConfigManager);
        final Field networkHistoryWriterField =
                WifiNetworkHistory.class.getDeclaredField("mWriter");
        networkHistoryWriterField.setAccessible(true);
        networkHistoryWriterField.set(wifiNetworkHistory, mWriter);

        when(mMOManager.isEnabled()).thenReturn(true);
        final Field moManagerField = WifiConfigManager.class.getDeclaredField("mMOManager");
        moManagerField.setAccessible(true);
        moManagerField.set(mWifiConfigManager, mMOManager);
    }

    private void switchUser(int newUserId) {
        when(mUserManager.getProfiles(newUserId))
                .thenReturn(USER_PROFILES.get(newUserId));
        mWifiConfigManager.handleUserSwitch(newUserId);
    }

    private void switchUserToCreatorOrParentOf(WifiConfiguration config) {
        final int creatorUserId = UserHandle.getUserId(config.creatorUid);
        if (creatorUserId == MANAGED_PROFILE_USER_ID) {
            switchUser(MANAGED_PROFILE_PARENT_USER_ID);
        } else {
            switchUser(creatorUserId);
        }
    }

    private void addNetworks() throws Exception {
        for (int i = 0; i < CONFIGS.size(); ++i) {
            assertEquals(i, CONFIGS.get(i).networkId);
            addNetwork(CONFIGS.get(i));
        }
    }

    private void addNetwork(WifiConfiguration config) throws Exception {
        final int originalUserId = mWifiConfigManager.getCurrentUserId();

        when(mWifiNative.setNetworkVariable(anyInt(), anyString(), anyString())).thenReturn(true);
        when(mWifiNative.setNetworkExtra(anyInt(), anyString(), (Map<String, String>) anyObject()))
                .thenReturn(true);

        switchUserToCreatorOrParentOf(config);
        final WifiConfiguration configCopy = new WifiConfiguration(config);
        int networkId = config.networkId;
        config.networkId = -1;
        when(mWifiNative.addNetwork()).thenReturn(networkId);
        when(mWifiNative.getNetworkVariable(networkId, WifiConfiguration.ssidVarName))
                .thenReturn(encodeConfigSSID(config));
        mWifiConfigManager.saveNetwork(config, configCopy.creatorUid);

        switchUser(originalUserId);
    }

    private String encodeConfigSSID(WifiConfiguration config) throws Exception {
        return new BigInteger(1, config.SSID.substring(1, config.SSID.length() - 1)
                .getBytes("UTF-8")).toString(16);
    }

    /**
     * Verifies that getConfiguredNetworksSize() returns the number of network configurations
     * visible to the current user.
     */
    @Test
    public void testGetConfiguredNetworksSize() throws Exception {
        addNetworks();
        for (Map.Entry<Integer, List<WifiConfiguration>> entry : VISIBLE_CONFIGS.entrySet()) {
            switchUser(entry.getKey());
            assertEquals(entry.getValue().size(), mWifiConfigManager.getConfiguredNetworksSize());
        }
    }

    private void verifyNetworkConfig(WifiConfiguration expectedConfig,
            WifiConfiguration actualConfig) {
        assertNotNull(actualConfig);
        assertEquals(expectedConfig.SSID, actualConfig.SSID);
        assertEquals(expectedConfig.FQDN, actualConfig.FQDN);
        assertEquals(expectedConfig.providerFriendlyName,
                actualConfig.providerFriendlyName);
        assertEquals(expectedConfig.configKey(), actualConfig.configKey(false));
    }

    private void verifyNetworkConfigs(Collection<WifiConfiguration> expectedConfigs,
            Collection<WifiConfiguration> actualConfigs) {
        assertEquals(expectedConfigs.size(), actualConfigs.size());
        for (WifiConfiguration expectedConfig : expectedConfigs) {
            WifiConfiguration actualConfig = null;
            // Find the network configuration to test (assume that |actualConfigs| contains them in
            // undefined order).
            for (final WifiConfiguration candidate : actualConfigs) {
                if (candidate.networkId == expectedConfig.networkId) {
                    actualConfig = candidate;
                    break;
                }
            }
            verifyNetworkConfig(expectedConfig, actualConfig);
        }
    }

    /**
     * Verifies that getConfiguredNetworksSize() returns the network configurations visible to the
     * current user.
     */
    @Test
    public void testGetConfiguredNetworks() throws Exception {
        addNetworks();
        for (Map.Entry<Integer, List<WifiConfiguration>> entry : VISIBLE_CONFIGS.entrySet()) {
            switchUser(entry.getKey());
            verifyNetworkConfigs(entry.getValue(), mWifiConfigManager.getSavedNetworks());
        }
    }

    /**
     * Verifies that getPrivilegedConfiguredNetworks() returns the network configurations visible to
     * the current user.
     */
    @Test
    public void testGetPrivilegedConfiguredNetworks() throws Exception {
        addNetworks();
        for (Map.Entry<Integer, List<WifiConfiguration>> entry : VISIBLE_CONFIGS.entrySet()) {
            switchUser(entry.getKey());
            verifyNetworkConfigs(entry.getValue(),
                    mWifiConfigManager.getPrivilegedSavedNetworks());
        }
    }

    /**
     * Verifies that getWifiConfiguration(int netId) can be used to access network configurations
     * visible to the current user only.
     */
    @Test
    public void testGetWifiConfigurationByNetworkId() throws Exception {
        addNetworks();
        for (int userId : USER_IDS) {
            switchUser(userId);
            for (WifiConfiguration expectedConfig: CONFIGS) {
                final WifiConfiguration actualConfig =
                        mWifiConfigManager.getWifiConfiguration(expectedConfig.networkId);
                if (WifiConfigurationUtil.isVisibleToAnyProfile(expectedConfig,
                        USER_PROFILES.get(userId))) {
                    verifyNetworkConfig(expectedConfig, actualConfig);
                } else {
                    assertNull(actualConfig);
                }
            }
        }
    }

    /**
     * Verifies that getWifiConfiguration(String key) can be used to access network configurations
     * visible to the current user only.
     */
    @Test
    public void testGetWifiConfigurationByConfigKey() throws Exception {
        addNetworks();
        for (int userId : USER_IDS) {
            switchUser(userId);
            for (WifiConfiguration expectedConfig: CONFIGS) {
                final WifiConfiguration actualConfig =
                        mWifiConfigManager.getWifiConfiguration(expectedConfig.configKey());
                if (WifiConfigurationUtil.isVisibleToAnyProfile(expectedConfig,
                        USER_PROFILES.get(userId))) {
                    verifyNetworkConfig(expectedConfig, actualConfig);
                } else {
                    assertNull(actualConfig);
                }
            }
        }
    }

    /**
     * Verifies that enableAllNetworks() enables all temporarily disabled network configurations
     * visible to the current user.
     */
    @Test
    public void testEnableAllNetworks() throws Exception {
        addNetworks();
        for (int userId : USER_IDS) {
            switchUser(userId);

            for (WifiConfiguration config : mConfiguredNetworks.valuesForAllUsers()) {
                final WifiConfiguration.NetworkSelectionStatus status =
                        config.getNetworkSelectionStatus();
                status.setNetworkSelectionStatus(WifiConfiguration.NetworkSelectionStatus
                        .NETWORK_SELECTION_TEMPORARY_DISABLED);
                status.setNetworkSelectionDisableReason(
                        WifiConfiguration.NetworkSelectionStatus.DISABLED_AUTHENTICATION_FAILURE);
                status.setDisableTime(System.currentTimeMillis() - 60 * 60 * 1000);
            }

            mWifiConfigManager.enableAllNetworks();

            for (WifiConfiguration config : mConfiguredNetworks.valuesForAllUsers()) {
                assertEquals(WifiConfigurationUtil.isVisibleToAnyProfile(config,
                        USER_PROFILES.get(userId)),
                        config.getNetworkSelectionStatus().isNetworkEnabled());
            }
        }
    }

    /**
     * Verifies that selectNetwork() disables all network configurations visible to the current user
     * except the selected one.
     */
    @Test
    public void testSelectNetwork() throws Exception {
        addNetworks();

        for (int userId : USER_IDS) {
            switchUser(userId);

            for (WifiConfiguration config : mConfiguredNetworks.valuesForAllUsers()) {
                // Enable all network configurations.
                for (WifiConfiguration config2 : mConfiguredNetworks.valuesForAllUsers()) {
                    config2.status = WifiConfiguration.Status.ENABLED;
                }

                // Try to select a network configuration.
                reset(mWifiNative);
                when(mWifiNative.selectNetwork(config.networkId)).thenReturn(true);
                final boolean success =
                        mWifiConfigManager.selectNetwork(config, false, config.creatorUid);
                if (!WifiConfigurationUtil.isVisibleToAnyProfile(config,
                        USER_PROFILES.get(userId))) {
                    // If the network configuration is not visible to the current user, verify that
                    // nothing changed.
                    assertFalse(success);
                    verify(mWifiNative, never()).selectNetwork(anyInt());
                    verify(mWifiNative, never()).enableNetwork(anyInt());
                    for (WifiConfiguration config2 : mConfiguredNetworks.valuesForAllUsers()) {
                        assertEquals(WifiConfiguration.Status.ENABLED, config2.status);
                    }
                } else {
                    // If the network configuration is visible to the current user, verify that it
                    // was enabled and all other network configurations visible to the user were
                    // disabled.
                    assertTrue(success);
                    verify(mWifiNative).selectNetwork(config.networkId);
                    verify(mWifiNative, never()).selectNetwork(intThat(not(config.networkId)));
                    verify(mWifiNative, never()).enableNetwork(config.networkId);
                    verify(mWifiNative, never()).enableNetwork(intThat(not(config.networkId)));
                    for (WifiConfiguration config2 : mConfiguredNetworks.valuesForAllUsers()) {
                        if (WifiConfigurationUtil.isVisibleToAnyProfile(config2,
                                USER_PROFILES.get(userId))
                                && config2.networkId != config.networkId) {
                            assertEquals(WifiConfiguration.Status.DISABLED, config2.status);
                        } else {
                            assertEquals(WifiConfiguration.Status.ENABLED, config2.status);
                        }
                    }
                }
            }
        }
    }

    /**
     * Verifies that saveNetwork() correctly stores a network configuration in wpa_supplicant
     * variables and the networkHistory.txt file.
     * TODO: Test all variables. Currently, only the following variables are tested:
     * - In the wpa_supplicant: "ssid", "id_str"
     * - In networkHistory.txt: "CONFIG", "CREATOR_UID_KEY", "SHARED"
     */
    private void verifySaveNetwork(int network) throws Exception {
        // Switch to the correct user.
        switchUserToCreatorOrParentOf(CONFIGS.get(network));

        // Set up wpa_supplicant.
        when(mWifiNative.addNetwork()).thenReturn(0);
        when(mWifiNative.setNetworkVariable(eq(network), anyString(), anyString()))
                .thenReturn(true);
        when(mWifiNative.setNetworkExtra(eq(network), anyString(),
                (Map<String, String>) anyObject())).thenReturn(true);
        when(mWifiNative.getNetworkVariable(network, WifiConfiguration.ssidVarName))
                .thenReturn(encodeConfigSSID(CONFIGS.get(network)));
        when(mWifiNative.getNetworkVariable(network, WifiConfiguration.pmfVarName))
                .thenReturn("");

        // Store a network configuration.
        mWifiConfigManager.saveNetwork(CONFIGS.get(network), CONFIGS.get(network).creatorUid);

        // Verify that wpa_supplicant variables were written correctly for the network
        // configuration.
        final Map<String, String> metadata = new HashMap<String, String>();
        if (CONFIGS.get(network).FQDN != null) {
            metadata.put(WifiConfigStore.ID_STRING_KEY_FQDN, CONFIGS.get(network).FQDN);
        }
        metadata.put(WifiConfigStore.ID_STRING_KEY_CONFIG_KEY, CONFIGS.get(network).configKey());
        metadata.put(WifiConfigStore.ID_STRING_KEY_CREATOR_UID,
                Integer.toString(CONFIGS.get(network).creatorUid));
        verify(mWifiNative).setNetworkExtra(network, WifiConfigStore.ID_STRING_VAR_NAME,
                metadata);

        // Verify that an attempt to read back the requirePMF variable was made.
        verify(mWifiNative).getNetworkVariable(network, WifiConfiguration.pmfVarName);

        // Verify that no wpa_supplicant variables were read or written for any other network
        // configurations.
        verify(mWifiNative, never()).setNetworkExtra(intThat(not(network)), anyString(),
                (Map<String, String>) anyObject());
        verify(mWifiNative, never()).setNetworkVariable(intThat(not(network)), anyString(),
                anyString());
        verify(mWifiNative, never()).getNetworkVariable(intThat(not(network)), anyString());

        // Parse networkHistory.txt.
        assertNotNull(mNetworkHistoryBytes);
        final DataInputStream stream =
                new DataInputStream(new ByteArrayInputStream(mNetworkHistoryBytes));
        List<String> keys = new ArrayList<>();
        List<String> values = new ArrayList<>();
        try {
            while (true) {
                final String[] tokens = stream.readUTF().split(":", 2);
                if (tokens.length == 2) {
                    keys.add(tokens[0].trim());
                    values.add(tokens[1].trim());
                }
            }
        } catch (EOFException e) {
            // Ignore. This is expected.
        }

        // Verify that a networkHistory.txt entry was written correctly for the network
        // configuration.
        assertTrue(keys.size() >= 3);
        assertEquals(WifiNetworkHistory.CONFIG_KEY, keys.get(0));
        assertEquals(CONFIGS.get(network).configKey(), values.get(0));
        final int creatorUidIndex = keys.indexOf(WifiNetworkHistory.CREATOR_UID_KEY);
        assertTrue(creatorUidIndex != -1);
        assertEquals(Integer.toString(CONFIGS.get(network).creatorUid),
                values.get(creatorUidIndex));
        final int sharedIndex = keys.indexOf(WifiNetworkHistory.SHARED_KEY);
        assertTrue(sharedIndex != -1);
        assertEquals(Boolean.toString(CONFIGS.get(network).shared), values.get(sharedIndex));

        // Verify that no networkHistory.txt entries were written for any other network
        // configurations.
        final int lastConfigIndex = keys.lastIndexOf(WifiNetworkHistory.CONFIG_KEY);
        assertEquals(0, lastConfigIndex);
    }

    /**
     * Verifies that saveNetwork() correctly stores a regular network configuration.
     */
    @Test
    public void testSaveNetworkRegular() throws Exception {
        verifySaveNetwork(0);
    }

    /**
     * Verifies that saveNetwork() correctly stores a HotSpot 2.0 network configuration.
     */
    @Test
    public void testSaveNetworkHotspot20() throws Exception {
        verifySaveNetwork(1);
    }

    /**
     * Verifies that saveNetwork() correctly stores a private network configuration.
     */
    @Test
    public void testSaveNetworkPrivate() throws Exception {
        verifySaveNetwork(2);
    }

    /**
     * Verifies that loadConfiguredNetworks() correctly reads data from the wpa_supplicant, the
     * networkHistory.txt file and the MOManager, correlating the three sources based on the
     * configKey and the FQDN for HotSpot 2.0 networks.
     * TODO: Test all variables. Currently, only the following variables are tested:
     * - In the wpa_supplicant: "ssid", "id_str"
     * - In networkHistory.txt: "CONFIG", "CREATOR_UID_KEY", "SHARED"
     */
    @Test
    public void testLoadConfiguredNetworks() throws Exception {
        // Set up list of network configurations returned by wpa_supplicant.
        final String header = "network id / ssid / bssid / flags";
        String networks = header;
        for (WifiConfiguration config : CONFIGS) {
            networks += "\n" + Integer.toString(config.networkId) + "\t" + config.SSID + "\tany";
        }
        when(mWifiNative.listNetworks(anyInt())).thenReturn(header);
        when(mWifiNative.listNetworks(-1)).thenReturn(networks);

        // Set up variables returned by wpa_supplicant for the individual network configurations.
        for (int i = 0; i < CONFIGS.size(); ++i) {
            when(mWifiNative.getNetworkVariable(i, WifiConfiguration.ssidVarName))
                .thenReturn(encodeConfigSSID(CONFIGS.get(i)));
        }
        // Legacy regular network configuration: No "id_str".
        when(mWifiNative.getNetworkExtra(0, WifiConfigStore.ID_STRING_VAR_NAME))
            .thenReturn(null);
        // Legacy Hotspot 2.0 network configuration: Quoted FQDN in "id_str".
        when(mWifiNative.getNetworkExtra(1, WifiConfigStore.ID_STRING_VAR_NAME))
            .thenReturn(null);
        when(mWifiNative.getNetworkVariable(1, WifiConfigStore.ID_STRING_VAR_NAME))
            .thenReturn('"' + CONFIGS.get(1).FQDN + '"');
        // Up-to-date Hotspot 2.0 network configuration: Metadata in "id_str".
        Map<String, String> metadata = new HashMap<String, String>();
        metadata.put(WifiConfigStore.ID_STRING_KEY_CONFIG_KEY, CONFIGS.get(2).configKey());
        metadata.put(WifiConfigStore.ID_STRING_KEY_CREATOR_UID,
                Integer.toString(CONFIGS.get(2).creatorUid));
        metadata.put(WifiConfigStore.ID_STRING_KEY_FQDN, CONFIGS.get(2).FQDN);
        when(mWifiNative.getNetworkExtra(2, WifiConfigStore.ID_STRING_VAR_NAME))
            .thenReturn(metadata);
        // Up-to-date regular network configuration: Metadata in "id_str".
        metadata = new HashMap<String, String>();
        metadata.put(WifiConfigStore.ID_STRING_KEY_CONFIG_KEY, CONFIGS.get(3).configKey());
        metadata.put(WifiConfigStore.ID_STRING_KEY_CREATOR_UID,
                Integer.toString(CONFIGS.get(3).creatorUid));
        when(mWifiNative.getNetworkExtra(3, WifiConfigStore.ID_STRING_VAR_NAME))
            .thenReturn(metadata);

        // Set up networkHistory.txt file.
        final File file = File.createTempFile("networkHistory.txt", null);
        file.deleteOnExit();

        Field wifiNetworkHistoryConfigFile =
                WifiNetworkHistory.class.getDeclaredField("NETWORK_HISTORY_CONFIG_FILE");
        wifiNetworkHistoryConfigFile.setAccessible(true);
        wifiNetworkHistoryConfigFile.set(null, file.getAbsolutePath());

        final DataOutputStream stream = new DataOutputStream(new FileOutputStream(file));
        for (WifiConfiguration config : CONFIGS) {
            stream.writeUTF(WifiNetworkHistory.CONFIG_KEY + ":  " + config.configKey() + '\n');
            stream.writeUTF(WifiNetworkHistory.CREATOR_UID_KEY + ":  "
                    + Integer.toString(config.creatorUid) + '\n');
            stream.writeUTF(WifiNetworkHistory.SHARED_KEY + ":  "
                    + Boolean.toString(config.shared) + '\n');
        }
        stream.close();

        // Set up list of home service providers returned by MOManager.
        final List<HomeSP> homeSPs = new ArrayList<HomeSP>();
        for (WifiConfiguration config : CONFIGS) {
            if (config.FQDN != null) {
                homeSPs.add(new HomeSP(null, config.FQDN, new HashSet<Long>(),
                        new HashSet<String>(),
                        new HashSet<Long>(), new ArrayList<Long>(),
                        config.providerFriendlyName, null,
                        new Credential(0, 0, null, false, null, null),
                        null, 0, null, null, null, 0));
            }
        }
        when(mMOManager.loadAllSPs()).thenReturn(homeSPs);

        // Load network configurations.
        mWifiConfigManager.loadConfiguredNetworks();

        // Verify that network configurations were loaded and correlated correctly across the three
        // sources.
        verifyNetworkConfigs(CONFIGS, mConfiguredNetworks.valuesForAllUsers());
    }

    /**
     * Verifies that loadConfiguredNetworks() correctly handles duplicates when reading network
     * configurations from the wpa_supplicant: The second configuration overwrites the first.
     */
    @Test
    public void testLoadConfiguredNetworksEliminatesDuplicates() throws Exception {
        final WifiConfiguration config = new WifiConfiguration(CONFIGS.get(0));
        config.networkId = 1;

        // Set up list of network configurations returned by wpa_supplicant. The two configurations
        // are identical except for their network IDs.
        final String header = "network id / ssid / bssid / flags";
        final String networks =
                header + "\n0\t" + config.SSID + "\tany\n1\t" + config.SSID + "\tany";
        when(mWifiNative.listNetworks(anyInt())).thenReturn(header);
        when(mWifiNative.listNetworks(-1)).thenReturn(networks);

        // Set up variables returned by wpa_supplicant.
        when(mWifiNative.getNetworkVariable(anyInt(), eq(WifiConfiguration.ssidVarName)))
            .thenReturn(encodeConfigSSID(config));
        final Map<String, String> metadata = new HashMap<String, String>();
        metadata.put(WifiConfigStore.ID_STRING_KEY_CONFIG_KEY, config.configKey());
        metadata.put(WifiConfigStore.ID_STRING_KEY_CREATOR_UID,
                Integer.toString(config.creatorUid));
        when(mWifiNative.getNetworkExtra(anyInt(), eq(WifiConfigStore.ID_STRING_VAR_NAME)))
            .thenReturn(metadata);

        // Load network configurations.
        mWifiConfigManager.loadConfiguredNetworks();

        // Verify that the second network configuration (network ID 1) overwrote the first (network
        // ID 0).
        verifyNetworkConfigs(Arrays.asList(config), mConfiguredNetworks.valuesForAllUsers());
    }

    /**
     * Verifies that handleUserSwitch() removes ephemeral network configurations, disables network
     * configurations that should no longer be visible and enables network configurations that
     * should become visible.
     */
    private void verifyHandleUserSwitch(int oldUserId, int newUserId,
            boolean makeOneConfigEphemeral) throws Exception {
        addNetworks();
        switchUser(oldUserId);

        reset(mWifiNative);
        final Field lastSelectedConfigurationField =
                WifiConfigManager.class.getDeclaredField("mLastSelectedConfiguration");
        lastSelectedConfigurationField.setAccessible(true);
        WifiConfiguration removedEphemeralConfig = null;
        final Set<WifiConfiguration> oldUserOnlyConfigs = new HashSet<>();
        final Set<WifiConfiguration> newUserOnlyConfigs = new HashSet<>();
        final Set<WifiConfiguration> neitherUserConfigs = new HashSet<>();
        final Collection<WifiConfiguration> oldConfigs = mConfiguredNetworks.valuesForAllUsers();
        int expectedNumberOfConfigs = oldConfigs.size();
        for (WifiConfiguration config : oldConfigs) {
            if (WifiConfigurationUtil.isVisibleToAnyProfile(config, USER_PROFILES.get(oldUserId))) {
                config.status = WifiConfiguration.Status.ENABLED;
                if (WifiConfigurationUtil.isVisibleToAnyProfile(config,
                        USER_PROFILES.get(newUserId))) {
                    if (makeOneConfigEphemeral && removedEphemeralConfig == null) {
                        config.ephemeral = true;
                        lastSelectedConfigurationField.set(mWifiConfigManager, config.configKey());
                        removedEphemeralConfig = config;
                    }
                } else {
                    oldUserOnlyConfigs.add(config);
                }
            } else {
                config.status = WifiConfiguration.Status.DISABLED;
                if (WifiConfigurationUtil.isVisibleToAnyProfile(config,
                        USER_PROFILES.get(newUserId))) {
                    newUserOnlyConfigs.add(config);
                } else {
                    neitherUserConfigs.add(config);
                }
            }
        }

        when(mWifiNative.disableNetwork(anyInt())).thenReturn(true);
        when(mWifiNative.removeNetwork(anyInt())).thenReturn(true);

        switchUser(newUserId);
        if (makeOneConfigEphemeral) {
            // Verify that the ephemeral network configuration was removed.
            assertNotNull(removedEphemeralConfig);
            assertNull(mConfiguredNetworks.getForAllUsers(removedEphemeralConfig.networkId));
            assertNull(lastSelectedConfigurationField.get(mWifiConfigManager));
            verify(mWifiNative).removeNetwork(removedEphemeralConfig.networkId);
            --expectedNumberOfConfigs;
        } else {
            assertNull(removedEphemeralConfig);
        }

        // Verify that the other network configurations were revealed/hidden and enabled/disabled as
        // appropriate.
        final Collection<WifiConfiguration> newConfigs = mConfiguredNetworks.valuesForAllUsers();
        assertEquals(expectedNumberOfConfigs, newConfigs.size());
        for (WifiConfiguration config : newConfigs) {
            if (oldUserOnlyConfigs.contains(config)) {
                verify(mWifiNative).disableNetwork(config.networkId);
                assertEquals(WifiConfiguration.Status.DISABLED, config.status);
            } else {
                verify(mWifiNative, never()).disableNetwork(config.networkId);
                if (neitherUserConfigs.contains(config)) {
                    assertEquals(WifiConfiguration.Status.DISABLED, config.status);
                } else {
                    // Only enabled in networkSelection.
                    assertTrue(config.getNetworkSelectionStatus().isNetworkEnabled());
                }

            }
        }
    }

    /**
     * Verifies that handleUserSwitch() behaves correctly when the user switch removes an ephemeral
     * network configuration and reveals a private network configuration.
     */
    @Test
    public void testHandleUserSwitchWithEphemeral() throws Exception {
        verifyHandleUserSwitch(USER_IDS[2], USER_IDS[0], true);
    }

    /**
     * Verifies that handleUserSwitch() behaves correctly when the user switch hides a private
     * network configuration.
     */
    @Test
    public void testHandleUserSwitchWithoutEphemeral() throws Exception {
        verifyHandleUserSwitch(USER_IDS[0], USER_IDS[2], false);
    }

    @Test
    public void testSaveLoadEapNetworks() {
        testSaveLoadSingleEapNetwork("eap network", new EnterpriseConfig(Eap.TTLS)
                .setPhase2(Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[] {FakeKeys.CA_CERT0}));
        testSaveLoadSingleEapNetwork("eap network", new EnterpriseConfig(Eap.TTLS)
                .setPhase2(Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[] {FakeKeys.CA_CERT1, FakeKeys.CA_CERT0}));

    }

    private void testSaveLoadSingleEapNetwork(String ssid, EnterpriseConfig eapConfig) {
        final HashMap<String, String> networkVariables = new HashMap<String, String>();
        reset(mWifiNative);
        when(mWifiNative.addNetwork()).thenReturn(0);
        when(mWifiNative.setNetworkVariable(anyInt(), anyString(), anyString())).thenAnswer(
                new AnswerWithArguments() {
                    public boolean answer(int netId, String name, String value) {
                        // Verify that no wpa_supplicant variables were written for any other
                        // network configurations.
                        assertEquals(netId, 0);
                        networkVariables.put(name, value);
                        return true;
                    }
                });
        when(mWifiNative.getNetworkVariable(anyInt(), anyString())).then(
                new AnswerWithArguments() {
                    public String answer(int netId, String name) {
                        // Verify that no wpa_supplicant variables were read for any other
                        // network configurations.
                        assertEquals(netId, 0);
                        return networkVariables.get(name);
                    }
                });
        when(mWifiNative.setNetworkExtra(eq(0), anyString(), (Map<String, String>) anyObject()))
                .thenReturn(true);

        WifiConfiguration config = new WifiConfiguration();
        config.SSID = ssid;
        config.creatorUid = Process.WIFI_UID;
        config.allowedKeyManagement.set(KeyMgmt.WPA_EAP);
        config.enterpriseConfig = eapConfig.enterpriseConfig;

        // Store a network configuration.
        mWifiConfigManager.saveNetwork(config, Process.WIFI_UID);

        // Verify that wpa_supplicant variables were written correctly for the network
        // configuration.
        verify(mWifiNative).addNetwork();
        assertEquals(eapConfig.eap,
                unquote(networkVariables.get(WifiEnterpriseConfig.EAP_KEY)));
        assertEquals(eapConfig.phase2,
                unquote(networkVariables.get(WifiEnterpriseConfig.PHASE2_KEY)));
        assertEquals(eapConfig.identity,
                unquote(networkVariables.get(WifiEnterpriseConfig.IDENTITY_KEY)));
        assertEquals(eapConfig.password,
                unquote(networkVariables.get(WifiEnterpriseConfig.PASSWORD_KEY)));
        assertSavedCaCerts(eapConfig,
                unquote(networkVariables.get(WifiEnterpriseConfig.CA_CERT_KEY)));

        // Prepare the scan result.
        final String header = "network id / ssid / bssid / flags";
        String networks = header + "\n" + Integer.toString(0) + "\t" + ssid + "\tany";
        when(mWifiNative.listNetworks(anyInt())).thenReturn(header);
        when(mWifiNative.listNetworks(-1)).thenReturn(networks);

        // Load back the configuration.
        mWifiConfigManager.loadConfiguredNetworks();
        List<WifiConfiguration> configs = mWifiConfigManager.getSavedNetworks();
        assertEquals(1, configs.size());
        WifiConfiguration loadedConfig = configs.get(0);
        assertEquals(ssid, unquote(loadedConfig.SSID));
        BitSet keyMgmt = new BitSet();
        keyMgmt.set(KeyMgmt.WPA_EAP);
        assertEquals(keyMgmt, loadedConfig.allowedKeyManagement);
        assertEquals(eapConfig.enterpriseConfig.getEapMethod(),
                loadedConfig.enterpriseConfig.getEapMethod());
        assertEquals(eapConfig.enterpriseConfig.getPhase2Method(),
                loadedConfig.enterpriseConfig.getPhase2Method());
        assertEquals(eapConfig.enterpriseConfig.getIdentity(),
                loadedConfig.enterpriseConfig.getIdentity());
        assertEquals(eapConfig.enterpriseConfig.getPassword(),
                loadedConfig.enterpriseConfig.getPassword());
        asserCaCertsAliasesMatch(eapConfig.caCerts,
                loadedConfig.enterpriseConfig.getCaCertificateAliases());
    }

    private String unquote(String value) {
        if (value == null) {
            return null;
        }
        int length = value.length();
        if ((length > 1) && (value.charAt(0) == '"')
                && (value.charAt(length - 1) == '"')) {
            return value.substring(1, length - 1);
        } else {
            return value;
        }
    }

    private void asserCaCertsAliasesMatch(X509Certificate[] certs, String[] aliases) {
        assertEquals(certs.length, aliases.length);
        List<String> aliasList = new ArrayList<String>(Arrays.asList(aliases));
        try {
            for (int i = 0; i < certs.length; i++) {
                byte[] certPem = Credentials.convertToPem(certs[i]);
                boolean found = false;
                for (int j = 0; j < aliasList.size(); j++) {
                    byte[] keystoreCert = mMockKeyStore.getKeyBlob(Process.WIFI_UID,
                            Credentials.CA_CERTIFICATE + aliasList.get(j)).blob;
                    if (Arrays.equals(keystoreCert, certPem)) {
                        found = true;
                        aliasList.remove(j);
                        break;
                    }
                }
                assertTrue(found);
            }
        } catch (CertificateEncodingException | IOException e) {
            fail("Cannot convert CA certificate to encoded form.");
        }
    }

    private void assertSavedCaCerts(EnterpriseConfig eapConfig, String caCertVariable) {
        ArrayList<String> aliases = new ArrayList<String>();
        if (TextUtils.isEmpty(caCertVariable)) {
            // Do nothing.
        } else if (caCertVariable.startsWith(WifiEnterpriseConfig.CA_CERT_PREFIX)) {
            aliases.add(caCertVariable.substring(WifiEnterpriseConfig.CA_CERT_PREFIX.length()));
        } else if (caCertVariable.startsWith(WifiEnterpriseConfig.KEYSTORES_URI)) {
            String[] encodedAliases = TextUtils.split(
                    caCertVariable.substring(WifiEnterpriseConfig.KEYSTORES_URI.length()),
                    WifiEnterpriseConfig.CA_CERT_ALIAS_DELIMITER);
            for (String encodedAlias : encodedAliases) {
                String alias = WifiEnterpriseConfig.decodeCaCertificateAlias(encodedAlias);
                assertTrue(alias.startsWith(Credentials.CA_CERTIFICATE));
                aliases.add(alias.substring(Credentials.CA_CERTIFICATE.length()));
            }
        } else {
            fail("Unrecognized ca_cert variable: " + caCertVariable);
        }
        asserCaCertsAliasesMatch(eapConfig.caCerts, aliases.toArray(new String[aliases.size()]));
    }

    private static class EnterpriseConfig {
        public String eap;
        public String phase2;
        public String identity;
        public String password;
        public X509Certificate[] caCerts;
        public WifiEnterpriseConfig enterpriseConfig;

        public EnterpriseConfig(int eapMethod) {
            enterpriseConfig = new WifiEnterpriseConfig();
            enterpriseConfig.setEapMethod(eapMethod);
            eap = Eap.strings[eapMethod];
        }
        public EnterpriseConfig setPhase2(int phase2Method) {
            enterpriseConfig.setPhase2Method(phase2Method);
            phase2 = "auth=" + Phase2.strings[phase2Method];
            return this;
        }
        public EnterpriseConfig setIdentity(String identity, String password) {
            enterpriseConfig.setIdentity(identity);
            enterpriseConfig.setPassword(password);
            this.identity = identity;
            this.password = password;
            return this;
        }
        public EnterpriseConfig setCaCerts(X509Certificate[] certs) {
            enterpriseConfig.setCaCertificates(certs);
            caCerts = certs;
            return this;
        }
    }

    /**
     * Generates an array of unique random numbers below the specified maxValue.
     * Values range from 0 to maxValue-1.
     */
    private static ArrayDeque<Integer> getUniqueRandomNumberValues(
            int seed,
            int maxValue,
            int numValues) {
        assertTrue(numValues <= maxValue);
        Random rand = new Random(WifiTestUtil.getTestMethod().hashCode() + seed);
        ArrayDeque<Integer> randomNumberList = new ArrayDeque<>();
        for (int i = 0; i < numValues; i++) {
            int num = rand.nextInt(maxValue);
            while (randomNumberList.contains(num)) {
                num = rand.nextInt(maxValue);
            }
            randomNumberList.push(num);
        }
        return randomNumberList;
    }

    /**
     * Verifies that the networks in pnoNetworkList is sorted in the same order as the
     * network in expectedNetworkIDOrder list.
     */
    private static void verifyPnoNetworkListOrder(
            ArrayList<WifiScanner.PnoSettings.PnoNetwork> pnoNetworkList,
            ArrayList<Integer> expectedNetworkIdOrder) throws Exception  {
        int i = 0;
        for (WifiScanner.PnoSettings.PnoNetwork pnoNetwork : pnoNetworkList) {
            Log.i(TAG, "PNO Network List Index: " + i + ", networkID: " + pnoNetwork.networkId);
            assertEquals("Expected network ID: " + pnoNetwork.networkId,
                    pnoNetwork.networkId, expectedNetworkIdOrder.get(i++).intValue());
        }
    }

    /**
     * Verifies the retrieveDisconnectedPnoNetworkList API. The test verifies that the list
     * returned from the API is sorted as expected.
     */
    @Test
    public void testDisconnectedPnoNetworkListCreation() throws Exception {
        addNetworks();

        Random rand = new Random(WifiTestUtil.getTestMethod().hashCode());

        // First assign random |numAssociation| values and verify that the list is sorted
        // in descending order of |numAssociation| values. Keep NetworkSelectionStatus
        // values constant.
        for (int userId : USER_IDS) {
            switchUser(userId);
            TreeMap<Integer, Integer> numAssociationToNetworkIdMap =
                    new TreeMap<>(Collections.reverseOrder());
            ArrayDeque<Integer> numAssociationValues =
                    getUniqueRandomNumberValues(
                            1, 10000, mConfiguredNetworks.valuesForCurrentUser().size());
            for (WifiConfiguration config : mConfiguredNetworks.valuesForCurrentUser()) {
                config.numAssociation = numAssociationValues.pop();
                config.priority = rand.nextInt(10000);
                config.getNetworkSelectionStatus().setNetworkSelectionStatus(
                        WifiConfiguration.NetworkSelectionStatus.NETWORK_SELECTION_ENABLED);
                numAssociationToNetworkIdMap.put(config.numAssociation, config.networkId);
                Log.i(TAG, "networkID: " + config.networkId + ", numAssociation: "
                        + config.numAssociation);
            }
            ArrayList<WifiScanner.PnoSettings.PnoNetwork> pnoNetworkList =
                    mWifiConfigManager.retrieveDisconnectedPnoNetworkList();
            verifyPnoNetworkListOrder(pnoNetworkList,
                    new ArrayList(numAssociationToNetworkIdMap.values()));
        }

        // Assign random |priority| values and verify that the list is sorted in descending order
        // of |priority| values. Keep numAssociation/NetworkSelectionStatus values constant.
        for (int userId : USER_IDS) {
            switchUser(userId);
            TreeMap<Integer, Integer> priorityToNetworkIdMap =
                    new TreeMap<>(Collections.reverseOrder());
            ArrayDeque<Integer> priorityValues =
                    getUniqueRandomNumberValues(
                            2, 10000, mConfiguredNetworks.valuesForCurrentUser().size());
            for (WifiConfiguration config : mConfiguredNetworks.valuesForCurrentUser()) {
                config.numAssociation = 0;
                config.priority = priorityValues.pop();
                config.getNetworkSelectionStatus().setNetworkSelectionStatus(
                        WifiConfiguration.NetworkSelectionStatus.NETWORK_SELECTION_ENABLED);
                priorityToNetworkIdMap.put(config.priority, config.networkId);
                Log.i(TAG, "networkID: " + config.networkId + ", priority: " + config.priority);
            }
            ArrayList<WifiScanner.PnoSettings.PnoNetwork> pnoNetworkList =
                    mWifiConfigManager.retrieveDisconnectedPnoNetworkList();
            verifyPnoNetworkListOrder(pnoNetworkList,
                    new ArrayList(priorityToNetworkIdMap.values()));
        }

        // Now assign random |NetworkSelectionStatus| values and verify that the list is sorted in
        // ascending order of |NetworkSelectionStatus| values.
        for (int userId : USER_IDS) {
            switchUser(userId);
            TreeMap<Integer, Integer> networkSelectionStatusToNetworkIdMap = new TreeMap<>();
            ArrayDeque<Integer> networkSelectionStatusValues =
                    getUniqueRandomNumberValues(
                            3,
                            WifiConfiguration.NetworkSelectionStatus.NETWORK_SELECTION_STATUS_MAX,
                            mConfiguredNetworks.valuesForCurrentUser().size());
            for (WifiConfiguration config : mConfiguredNetworks.valuesForCurrentUser()) {
                config.numAssociation = rand.nextInt(10000);
                config.priority = rand.nextInt(10000);
                config.getNetworkSelectionStatus().setNetworkSelectionStatus(
                        networkSelectionStatusValues.pop());
                networkSelectionStatusToNetworkIdMap.put(
                        config.getNetworkSelectionStatus().getNetworkSelectionStatus(),
                        config.networkId);
                Log.i(TAG, "networkID: " + config.networkId + ", NetworkSelectionStatus: "
                        + config.getNetworkSelectionStatus().getNetworkSelectionStatus());
            }
            ArrayList<WifiScanner.PnoSettings.PnoNetwork> pnoNetworkList =
                    mWifiConfigManager.retrieveDisconnectedPnoNetworkList();
            verifyPnoNetworkListOrder(pnoNetworkList,
                    new ArrayList(networkSelectionStatusToNetworkIdMap.values()));
        }
    }

    /**
     * Verifies the retrieveConnectedPnoNetworkList API. The test verifies that the list
     * returned from the API is sorted as expected.
     */
    @Test
    public void testConnectedPnoNetworkListCreation() throws Exception {
        addNetworks();

        Random rand = new Random(WifiTestUtil.getTestMethod().hashCode());

        // First assign |lastSeen| values and verify that the list is sorted
        // in descending order of |lastSeen| values. Keep NetworkSelectionStatus
        // values constant.
        for (int userId : USER_IDS) {
            switchUser(userId);
            TreeMap<Boolean, Integer> lastSeenToNetworkIdMap =
                    new TreeMap<>(Collections.reverseOrder());
            ArrayDeque<Integer> lastSeenValues = getUniqueRandomNumberValues(1, 2, 2);
            if (mConfiguredNetworks.valuesForCurrentUser().size() > 2) continue;
            for (WifiConfiguration config : mConfiguredNetworks.valuesForCurrentUser()) {
                config.numAssociation = rand.nextInt(10000);
                config.priority = rand.nextInt(10000);
                config.getNetworkSelectionStatus().setNetworkSelectionStatus(
                        WifiConfiguration.NetworkSelectionStatus.NETWORK_SELECTION_ENABLED);
                boolean lastSeenValue = (lastSeenValues.pop()  == 1);
                config.getNetworkSelectionStatus().setSeenInLastQualifiedNetworkSelection(
                        lastSeenValue);
                lastSeenToNetworkIdMap.put(lastSeenValue, config.networkId);
                Log.i(TAG, "networkID: " + config.networkId + ", lastSeen: " + lastSeenValue);
            }
            ArrayList<WifiScanner.PnoSettings.PnoNetwork> pnoNetworkList =
                    mWifiConfigManager.retrieveConnectedPnoNetworkList();
            verifyPnoNetworkListOrder(pnoNetworkList,
                    new ArrayList(lastSeenToNetworkIdMap.values()));
        }

        // Assign random |numAssociation| values and verify that the list is sorted
        // in descending order of |numAssociation| values. Keep NetworkSelectionStatus/lastSeen
        // values constant.
        for (int userId : USER_IDS) {
            switchUser(userId);
            TreeMap<Integer, Integer> numAssociationToNetworkIdMap =
                    new TreeMap<>(Collections.reverseOrder());
            ArrayDeque<Integer> numAssociationValues =
                    getUniqueRandomNumberValues(
                            1, 10000, mConfiguredNetworks.valuesForCurrentUser().size());
            for (WifiConfiguration config : mConfiguredNetworks.valuesForCurrentUser()) {
                config.numAssociation = numAssociationValues.pop();
                config.priority = rand.nextInt(10000);
                config.getNetworkSelectionStatus().setNetworkSelectionStatus(
                        WifiConfiguration.NetworkSelectionStatus.NETWORK_SELECTION_ENABLED);
                config.getNetworkSelectionStatus().setSeenInLastQualifiedNetworkSelection(true);
                numAssociationToNetworkIdMap.put(config.numAssociation, config.networkId);
                Log.i(TAG, "networkID: " + config.networkId + ", numAssociation: "
                        + config.numAssociation);
            }
            ArrayList<WifiScanner.PnoSettings.PnoNetwork> pnoNetworkList =
                    mWifiConfigManager.retrieveConnectedPnoNetworkList();
            verifyPnoNetworkListOrder(pnoNetworkList,
                    new ArrayList(numAssociationToNetworkIdMap.values()));
        }

        // Now assign random |NetworkSelectionStatus| values and verify that the list is sorted in
        // ascending order of |NetworkSelectionStatus| values.
        for (int userId : USER_IDS) {
            switchUser(userId);
            TreeMap<Integer, Integer> networkSelectionStatusToNetworkIdMap = new TreeMap<>();
            ArrayDeque<Integer> networkSelectionStatusValues =
                    getUniqueRandomNumberValues(
                            3,
                            WifiConfiguration.NetworkSelectionStatus.NETWORK_SELECTION_STATUS_MAX,
                            mConfiguredNetworks.valuesForCurrentUser().size());
            for (WifiConfiguration config : mConfiguredNetworks.valuesForCurrentUser()) {
                config.numAssociation = rand.nextInt(10000);
                config.priority = rand.nextInt(10000);
                config.getNetworkSelectionStatus().setNetworkSelectionStatus(
                        networkSelectionStatusValues.pop());
                networkSelectionStatusToNetworkIdMap.put(
                        config.getNetworkSelectionStatus().getNetworkSelectionStatus(),
                        config.networkId);
                Log.i(TAG, "networkID: " + config.networkId + ", NetworkSelectionStatus: "
                        + config.getNetworkSelectionStatus().getNetworkSelectionStatus());
            }
            ArrayList<WifiScanner.PnoSettings.PnoNetwork> pnoNetworkList =
                    mWifiConfigManager.retrieveConnectedPnoNetworkList();
            verifyPnoNetworkListOrder(pnoNetworkList,
                    new ArrayList(networkSelectionStatusToNetworkIdMap.values()));
        }
    }

    /**
     * Verifies that hasEverConnected is false for a newly added network
     */
    @Test
    public void testAddNetworkHasEverConnectedFalse() throws Exception {
        addNetwork(BASE_HAS_EVER_CONNECTED_CONFIG);
        WifiConfiguration checkConfig = mWifiConfigManager.getWifiConfiguration(
                BASE_HAS_EVER_CONNECTED_CONFIG.networkId);
        assertFalse("Adding a new network should not have hasEverConnected set to true.",
                checkConfig.getNetworkSelectionStatus().getHasEverConnected());
    }


    /**
     * Verifies that hasEverConnected is false for a newly added network even when new config has
     * mistakenly set HasEverConnected to true.
    */
    @Test
    public void testAddNetworkOverridesHasEverConnectedWhenTrueInNewConfig() throws Exception {
        WifiConfiguration newNetworkWithHasEverConnectedTrue =
                new WifiConfiguration(BASE_HAS_EVER_CONNECTED_CONFIG);
        newNetworkWithHasEverConnectedTrue.getNetworkSelectionStatus().setHasEverConnected(true);
        addNetwork(newNetworkWithHasEverConnectedTrue);
        // check if addNetwork clears the bit.
        WifiConfiguration checkConfig = mWifiConfigManager.getWifiConfiguration(
                newNetworkWithHasEverConnectedTrue.networkId);
        assertFalse("Adding a new network should not have hasEverConnected set to true.",
                checkConfig.getNetworkSelectionStatus().getHasEverConnected());
    }


    /**
     * Verify that setting HasEverConnected with a config update can be read back.
     */
    @Test
    public void testUpdateConfigToHasEverConnectedTrue() throws Exception {
        addNetwork(BASE_HAS_EVER_CONNECTED_CONFIG);

        // Get the newly saved config and update HasEverConnected
        WifiConfiguration checkConfig = mWifiConfigManager.getWifiConfiguration(
                BASE_HAS_EVER_CONNECTED_CONFIG.networkId);
        assertFalse("Adding a new network should not have hasEverConnected set to true.",
                checkConfig.getNetworkSelectionStatus().getHasEverConnected());
        checkConfig.getNetworkSelectionStatus().setHasEverConnected(true);
        mWifiConfigManager.addOrUpdateNetwork(checkConfig, HAS_EVER_CONNECTED_USER);

        // verify that HasEverConnected was properly written and read back
        checkHasEverConnectedTrue(BASE_HAS_EVER_CONNECTED_CONFIG.networkId);
    }


    /**
     * Verifies that hasEverConnected is cleared when a network config preSharedKey is updated.
     */
    @Test
    public void testUpdatePreSharedKeyClearsHasEverConnected() throws Exception {
        final int originalUserId = mWifiConfigManager.getCurrentUserId();

        testUpdateConfigToHasEverConnectedTrue();

        WifiConfiguration original = mWifiConfigManager.getWifiConfiguration(
                BASE_HAS_EVER_CONNECTED_CONFIG.networkId);

        WifiConfiguration updatePreSharedKeyConfig = new WifiConfiguration();
        updatePreSharedKeyConfig.networkId = BASE_HAS_EVER_CONNECTED_CONFIG.networkId;
        updatePreSharedKeyConfig.SSID = original.SSID;
        updatePreSharedKeyConfig.preSharedKey = "newpassword";
        switchUserToCreatorOrParentOf(original);
        mWifiConfigManager.addOrUpdateNetwork(updatePreSharedKeyConfig,
                BASE_HAS_EVER_CONNECTED_CONFIG.networkId);

        checkHasEverConnectedFalse(BASE_HAS_EVER_CONNECTED_CONFIG.networkId);
        switchUser(originalUserId);
    }

    /**
     * Verifies that hasEverConnected is cleared when a network config allowedKeyManagement is
     * updated.
     */
    @Test
    public void testUpdateAllowedKeyManagementChanged() throws Exception {
        final int originalUserId = mWifiConfigManager.getCurrentUserId();

        testUpdateConfigToHasEverConnectedTrue();

        WifiConfiguration updateAllowedKeyManagementConfig = new WifiConfiguration();
        updateAllowedKeyManagementConfig.networkId = BASE_HAS_EVER_CONNECTED_CONFIG.networkId;
        updateAllowedKeyManagementConfig.SSID = BASE_HAS_EVER_CONNECTED_CONFIG.SSID;
        updateAllowedKeyManagementConfig.allowedKeyManagement.set(KeyMgmt.WPA_PSK);

        // Set up mock to allow the new value to be read back into the config
        String allowedKeyManagementString = makeString(
                updateAllowedKeyManagementConfig.allowedKeyManagement,
                    WifiConfiguration.KeyMgmt.strings);
        when(mWifiNative.getNetworkVariable(BASE_HAS_EVER_CONNECTED_CONFIG.networkId,
                KeyMgmt.varName)).thenReturn(allowedKeyManagementString);

        switchUserToCreatorOrParentOf(BASE_HAS_EVER_CONNECTED_CONFIG);
        mWifiConfigManager.addOrUpdateNetwork(updateAllowedKeyManagementConfig,
                BASE_HAS_EVER_CONNECTED_CONFIG.networkId);

        checkHasEverConnectedFalse(BASE_HAS_EVER_CONNECTED_CONFIG.networkId);
        switchUser(originalUserId);
    }

    /**
     * Verifies that hasEverConnected is cleared when a network config allowedProtocols is
     * updated.
     */
    @Test
    public void testUpdateAllowedProtocolsChanged() throws Exception {
        final int originalUserId = mWifiConfigManager.getCurrentUserId();

        testUpdateConfigToHasEverConnectedTrue();

        WifiConfiguration updateAllowedProtocolsConfig = new WifiConfiguration();
        updateAllowedProtocolsConfig.networkId = BASE_HAS_EVER_CONNECTED_CONFIG.networkId;
        updateAllowedProtocolsConfig.SSID = BASE_HAS_EVER_CONNECTED_CONFIG.SSID;
        updateAllowedProtocolsConfig.allowedProtocols.set(
                WifiConfiguration.Protocol.RSN);

        // Set up mock to allow the new value to be read back into the config
        String allowedProtocolsString = makeString(
                updateAllowedProtocolsConfig.allowedProtocols,
                    WifiConfiguration.Protocol.strings);
        when(mWifiNative.getNetworkVariable(BASE_HAS_EVER_CONNECTED_CONFIG.networkId,
                Protocol.varName)).thenReturn(allowedProtocolsString);

        switchUserToCreatorOrParentOf(BASE_HAS_EVER_CONNECTED_CONFIG);
        mWifiConfigManager.addOrUpdateNetwork(updateAllowedProtocolsConfig,
                BASE_HAS_EVER_CONNECTED_CONFIG.networkId);

        checkHasEverConnectedFalse(BASE_HAS_EVER_CONNECTED_CONFIG.networkId);
        switchUser(originalUserId);
    }

    /**
     * Verifies that hasEverConnected is cleared when a network config allowedAuthAlgorithms is
     * updated.
     */
    @Test
    public void testUpdateAllowedAuthAlgorithmsChanged() throws Exception {
        final int originalUserId = mWifiConfigManager.getCurrentUserId();

        testUpdateConfigToHasEverConnectedTrue();

        WifiConfiguration updateAllowedAuthAlgorithmsConfig = new WifiConfiguration();
        updateAllowedAuthAlgorithmsConfig.networkId = BASE_HAS_EVER_CONNECTED_CONFIG.networkId;
        updateAllowedAuthAlgorithmsConfig.SSID = BASE_HAS_EVER_CONNECTED_CONFIG.SSID;
        updateAllowedAuthAlgorithmsConfig.allowedAuthAlgorithms.set(
                WifiConfiguration.AuthAlgorithm.SHARED);

        // Set up mock to allow the new value to be read back into the config
        String allowedAuthAlgorithmsString = makeString(
                updateAllowedAuthAlgorithmsConfig.allowedAuthAlgorithms,
                    WifiConfiguration.AuthAlgorithm.strings);
        when(mWifiNative.getNetworkVariable(BASE_HAS_EVER_CONNECTED_CONFIG.networkId,
                AuthAlgorithm.varName)).thenReturn(allowedAuthAlgorithmsString);

        switchUserToCreatorOrParentOf(BASE_HAS_EVER_CONNECTED_CONFIG);
        mWifiConfigManager.addOrUpdateNetwork(updateAllowedAuthAlgorithmsConfig,
                BASE_HAS_EVER_CONNECTED_CONFIG.networkId);

        checkHasEverConnectedFalse(BASE_HAS_EVER_CONNECTED_CONFIG.networkId);
        switchUser(originalUserId);
    }

    /**
     * Verifies that hasEverConnected is cleared when a network config allowedPairwiseCiphers is
     * updated.
     */
    @Test
    public void testUpdateAllowedPairwiseCiphersChanged() throws Exception {
        final int originalUserId = mWifiConfigManager.getCurrentUserId();

        testUpdateConfigToHasEverConnectedTrue();

        WifiConfiguration updateAllowedPairwiseCiphersConfig = new WifiConfiguration();
        updateAllowedPairwiseCiphersConfig.networkId = BASE_HAS_EVER_CONNECTED_CONFIG.networkId;
        updateAllowedPairwiseCiphersConfig.SSID = BASE_HAS_EVER_CONNECTED_CONFIG.SSID;
        updateAllowedPairwiseCiphersConfig.allowedPairwiseCiphers.set(
                WifiConfiguration.PairwiseCipher.CCMP);

        // Set up mock to allow the new value to be read back into the config
        String allowedPairwiseCiphersString = makeString(
                updateAllowedPairwiseCiphersConfig.allowedPairwiseCiphers,
                    WifiConfiguration.PairwiseCipher.strings);
        when(mWifiNative.getNetworkVariable(BASE_HAS_EVER_CONNECTED_CONFIG.networkId,
                PairwiseCipher.varName)).thenReturn(allowedPairwiseCiphersString);

        switchUserToCreatorOrParentOf(BASE_HAS_EVER_CONNECTED_CONFIG);
        mWifiConfigManager.addOrUpdateNetwork(updateAllowedPairwiseCiphersConfig,
                BASE_HAS_EVER_CONNECTED_CONFIG.networkId);

        checkHasEverConnectedFalse(BASE_HAS_EVER_CONNECTED_CONFIG.networkId);
        switchUser(originalUserId);
    }

    /**
     * Verifies that hasEverConnected is cleared when a network config allowedGroupCiphers is
     * updated.
     */
    @Test
    public void testUpdateAllowedGroupCiphersChanged() throws Exception {
        final int originalUserId = mWifiConfigManager.getCurrentUserId();

        testUpdateConfigToHasEverConnectedTrue();

        WifiConfiguration updateAllowedGroupCiphersConfig = new WifiConfiguration();
        updateAllowedGroupCiphersConfig.networkId = BASE_HAS_EVER_CONNECTED_CONFIG.networkId;
        updateAllowedGroupCiphersConfig.SSID = BASE_HAS_EVER_CONNECTED_CONFIG.SSID;
        updateAllowedGroupCiphersConfig.allowedGroupCiphers.set(
                WifiConfiguration.GroupCipher.CCMP);

        // Set up mock to allow the new value to be read back into the config
        String allowedGroupCiphersString = makeString(
                updateAllowedGroupCiphersConfig.allowedGroupCiphers,
                    WifiConfiguration.GroupCipher.strings);
        when(mWifiNative.getNetworkVariable(BASE_HAS_EVER_CONNECTED_CONFIG.networkId,
                GroupCipher.varName)).thenReturn(allowedGroupCiphersString);

        switchUserToCreatorOrParentOf(BASE_HAS_EVER_CONNECTED_CONFIG);
        mWifiConfigManager.addOrUpdateNetwork(updateAllowedGroupCiphersConfig,
                BASE_HAS_EVER_CONNECTED_CONFIG.networkId);

        checkHasEverConnectedFalse(BASE_HAS_EVER_CONNECTED_CONFIG.networkId);
        switchUser(originalUserId);
    }

    /**
     * Verifies that hasEverConnected is cleared when a network config wepKeys is
     * updated.
     */
    @Test
    public void testUpdateWepKeysChanged() throws Exception {
        final int originalUserId = mWifiConfigManager.getCurrentUserId();

        testUpdateConfigToHasEverConnectedTrue();

        String tempKey = "hereisakey";
        WifiConfiguration updateWepKeysConfig = new WifiConfiguration();
        updateWepKeysConfig.networkId = BASE_HAS_EVER_CONNECTED_CONFIG.networkId;
        updateWepKeysConfig.SSID = BASE_HAS_EVER_CONNECTED_CONFIG.SSID;
        updateWepKeysConfig.wepKeys = new String[] {tempKey};

        // Set up mock to allow the new value to be read back into the config
        when(mWifiNative.getNetworkVariable(BASE_HAS_EVER_CONNECTED_CONFIG.networkId,
                WifiConfiguration.wepKeyVarNames[0])).thenReturn(tempKey);

        switchUserToCreatorOrParentOf(BASE_HAS_EVER_CONNECTED_CONFIG);
        mWifiConfigManager.addOrUpdateNetwork(updateWepKeysConfig,
                BASE_HAS_EVER_CONNECTED_CONFIG.networkId);

        checkHasEverConnectedFalse(BASE_HAS_EVER_CONNECTED_CONFIG.networkId);
        switchUser(originalUserId);
    }

    /**
     * Verifies that hasEverConnected is cleared when a network config hiddenSSID is
     * updated.
     */
    @Test
    public void testUpdateHiddenSSIDChanged() throws Exception {
        final int originalUserId = mWifiConfigManager.getCurrentUserId();

        testUpdateConfigToHasEverConnectedTrue();

        WifiConfiguration updateHiddenSSIDConfig = new WifiConfiguration();
        updateHiddenSSIDConfig.networkId = BASE_HAS_EVER_CONNECTED_CONFIG.networkId;
        updateHiddenSSIDConfig.SSID = BASE_HAS_EVER_CONNECTED_CONFIG.SSID;
        updateHiddenSSIDConfig.hiddenSSID = true;

        // Set up mock to allow the new value to be read back into the config
        when(mWifiNative.getNetworkVariable(BASE_HAS_EVER_CONNECTED_CONFIG.networkId,
                WifiConfiguration.hiddenSSIDVarName)).thenReturn("1");

        switchUserToCreatorOrParentOf(BASE_HAS_EVER_CONNECTED_CONFIG);
        mWifiConfigManager.addOrUpdateNetwork(updateHiddenSSIDConfig,
                BASE_HAS_EVER_CONNECTED_CONFIG.networkId);

        checkHasEverConnectedFalse(BASE_HAS_EVER_CONNECTED_CONFIG.networkId);
        switchUser(originalUserId);
    }

    /**
     * Verifies that hasEverConnected is cleared when a network config pmfVarName is
     * updated.
     */
    @Test
    public void testUpdateRequirePMFChanged() throws Exception {
        final int originalUserId = mWifiConfigManager.getCurrentUserId();

        testUpdateConfigToHasEverConnectedTrue();

        WifiConfiguration updateRequirePMFConfig = new WifiConfiguration();
        updateRequirePMFConfig.networkId = BASE_HAS_EVER_CONNECTED_CONFIG.networkId;
        updateRequirePMFConfig.SSID = BASE_HAS_EVER_CONNECTED_CONFIG.SSID;
        updateRequirePMFConfig.requirePMF = true;

        // Set up mock to allow the new value to be read back into the config
        // TODO: please see b/28088226  - this test is implemented as if WifiConfigStore correctly
        // read back the boolean value.  When fixed, uncomment the following line and the
        // checkHasEverConnectedFalse below.
        //when(mWifiNative.getNetworkVariable(BASE_HAS_EVER_CONNECTED_CONFIG.networkId,
        //        WifiConfiguration.pmfVarName)).thenReturn("2");

        switchUserToCreatorOrParentOf(BASE_HAS_EVER_CONNECTED_CONFIG);
        mWifiConfigManager.addOrUpdateNetwork(updateRequirePMFConfig,
                BASE_HAS_EVER_CONNECTED_CONFIG.networkId);

        //checkHasEverConnectedFalse(BASE_HAS_EVER_CONNECTED_CONFIG.networkId);
        checkHasEverConnectedTrue(BASE_HAS_EVER_CONNECTED_CONFIG.networkId);
        switchUser(originalUserId);
    }

    /**
     * Verify WifiEnterpriseConfig changes are detected in WifiConfigManager.
     */
    @Test
    public void testEnterpriseConfigAdded() {
        EnterpriseConfig eapConfig =  new EnterpriseConfig(Eap.TTLS)
                .setPhase2(Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[] {FakeKeys.CA_CERT0});

        assertTrue(mWifiConfigManager.wasEnterpriseConfigChange(null, eapConfig.enterpriseConfig));
    }

    /**
     * Verify WifiEnterpriseConfig eap change is detected.
     */
    @Test
    public void testEnterpriseConfigEapChangeDetected() {
        EnterpriseConfig eapConfig = new EnterpriseConfig(Eap.TTLS);
        EnterpriseConfig peapConfig = new EnterpriseConfig(Eap.PEAP);

        assertTrue(mWifiConfigManager.wasEnterpriseConfigChange(eapConfig.enterpriseConfig,
                peapConfig.enterpriseConfig));
    }

    /**
     * Verify WifiEnterpriseConfig phase2 method change is detected.
     */
    @Test
    public void testEnterpriseConfigPhase2ChangeDetected() {
        EnterpriseConfig eapConfig = new EnterpriseConfig(Eap.TTLS).setPhase2(Phase2.MSCHAPV2);
        EnterpriseConfig papConfig = new EnterpriseConfig(Eap.TTLS).setPhase2(Phase2.PAP);

        assertTrue(mWifiConfigManager.wasEnterpriseConfigChange(eapConfig.enterpriseConfig,
                papConfig.enterpriseConfig));
    }

    /**
     * Verify WifiEnterpriseConfig added Certificate is detected.
     */
    @Test
    public void testCaCertificateAddedDetected() {
        EnterpriseConfig eapConfigNoCerts =  new EnterpriseConfig(Eap.TTLS)
                .setPhase2(Phase2.MSCHAPV2)
                .setIdentity("username", "password");

        EnterpriseConfig eapConfig1Cert =  new EnterpriseConfig(Eap.TTLS)
                .setPhase2(Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[] {FakeKeys.CA_CERT0});

        assertTrue(mWifiConfigManager.wasEnterpriseConfigChange(eapConfigNoCerts.enterpriseConfig,
                eapConfig1Cert.enterpriseConfig));
    }

    /**
     * Verify WifiEnterpriseConfig Certificate change is detected.
     */
    @Test
    public void testDifferentCaCertificateDetected() {
        EnterpriseConfig eapConfig =  new EnterpriseConfig(Eap.TTLS)
                .setPhase2(Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[] {FakeKeys.CA_CERT0});

        EnterpriseConfig eapConfigNewCert =  new EnterpriseConfig(Eap.TTLS)
                .setPhase2(Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[] {FakeKeys.CA_CERT1});

        assertTrue(mWifiConfigManager.wasEnterpriseConfigChange(eapConfig.enterpriseConfig,
                eapConfigNewCert.enterpriseConfig));
    }

    /**
     * Verify WifiEnterpriseConfig added Certificate changes are detected.
     */
    @Test
    public void testCaCertificateChangesDetected() {
        EnterpriseConfig eapConfig =  new EnterpriseConfig(Eap.TTLS)
                .setPhase2(Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[] {FakeKeys.CA_CERT0});

        EnterpriseConfig eapConfigAddedCert =  new EnterpriseConfig(Eap.TTLS)
                .setPhase2(Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[] {FakeKeys.CA_CERT0, FakeKeys.CA_CERT1});

        assertTrue(mWifiConfigManager.wasEnterpriseConfigChange(eapConfig.enterpriseConfig,
                eapConfigAddedCert.enterpriseConfig));
    }

    /**
     * Verify that WifiEnterpriseConfig does not detect changes for identical configs.
     */
    @Test
    public void testWifiEnterpriseConfigNoChanges() {
        EnterpriseConfig eapConfig =  new EnterpriseConfig(Eap.TTLS)
                .setPhase2(Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[] {FakeKeys.CA_CERT0, FakeKeys.CA_CERT1});

        // Just to be clear that check is not against the same object
        EnterpriseConfig eapConfigSame =  new EnterpriseConfig(Eap.TTLS)
                .setPhase2(Phase2.MSCHAPV2)
                .setIdentity("username", "password")
                .setCaCerts(new X509Certificate[] {FakeKeys.CA_CERT0, FakeKeys.CA_CERT1});

        assertFalse(mWifiConfigManager.wasEnterpriseConfigChange(eapConfig.enterpriseConfig,
                eapConfigSame.enterpriseConfig));
    }


    private void checkHasEverConnectedTrue(int networkId) {
        WifiConfiguration checkConfig = mWifiConfigManager.getWifiConfiguration(networkId);
        assertTrue("hasEverConnected expected to be true.",
                checkConfig.getNetworkSelectionStatus().getHasEverConnected());
    }

    private void checkHasEverConnectedFalse(int networkId) {
        WifiConfiguration checkConfig = mWifiConfigManager.getWifiConfiguration(networkId);
        assertFalse("Updating credentials network config should clear hasEverConnected.",
                checkConfig.getNetworkSelectionStatus().getHasEverConnected());
    }

    /**
     *  Helper function to translate from WifiConfiguration BitSet to String.
     */
    private static String makeString(BitSet set, String[] strings) {
        StringBuffer buf = new StringBuffer();
        int nextSetBit = -1;

        /* Make sure all set bits are in [0, strings.length) to avoid
         * going out of bounds on strings.  (Shouldn't happen, but...) */
        set = set.get(0, strings.length);

        while ((nextSetBit = set.nextSetBit(nextSetBit + 1)) != -1) {
            buf.append(strings[nextSetBit].replace('_', '-')).append(' ');
        }

        // remove trailing space
        if (set.cardinality() > 0) {
            buf.setLength(buf.length() - 1);
        }

        return buf.toString();
    }


}
