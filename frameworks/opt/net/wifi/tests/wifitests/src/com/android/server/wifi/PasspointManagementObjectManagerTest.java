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
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import android.net.wifi.PasspointManagementObjectDefinition;
import android.net.wifi.WifiEnterpriseConfig;
import android.security.KeyStore;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.server.wifi.hotspot2.omadm.MOTree;
import com.android.server.wifi.hotspot2.omadm.OMAParser;
import com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager;
import com.android.server.wifi.hotspot2.omadm.XMLNode;
import com.android.server.wifi.hotspot2.pps.Credential;
import com.android.server.wifi.hotspot2.pps.HomeSP;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.xml.sax.SAXException;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Unit tests for {@link com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager}.
 */
@SmallTest
public class PasspointManagementObjectManagerTest {

    private static final String TAG = "PasspointManagementObjectManagerTest";

    @Rule
    public TemporaryFolder tempFolder = new TemporaryFolder();

    private File createFileFromResource(String configFile) throws Exception {
        InputStream in = getClass().getClassLoader().getResourceAsStream(configFile);
        File file = tempFolder.newFile(configFile.split("/")[1]);

        BufferedReader reader = new BufferedReader(new InputStreamReader(in));
        FileOutputStream out = new FileOutputStream(file);

        String line;

        while ((line = reader.readLine()) != null) {
            out.write(line.getBytes(StandardCharsets.UTF_8));
        }

        out.flush();
        out.close();
        return file;
    }

    private void addMoFromWifiConfig(PasspointManagementObjectManager moMgr) throws Exception {
        HashMap<String, Long> ssidMap = new HashMap<>();
        String fqdn = "tunisia.org";
        HashSet<Long> roamingConsortiums = new HashSet<Long>();
        HashSet<String> otherHomePartners = new HashSet<String>();
        Set<Long> matchAnyOIs = new HashSet<Long>();
        List<Long> matchAllOIs = new ArrayList<Long>();
        String friendlyName = "Tunisian Passpoint Provider";
        String iconUrl = "http://www.tunisia.org/icons/base_icon.png";

        WifiEnterpriseConfig enterpriseConfig = new WifiEnterpriseConfig();
        enterpriseConfig.setEapMethod(WifiEnterpriseConfig.Eap.TTLS);
        enterpriseConfig.setPhase2Method(WifiEnterpriseConfig.Phase2.PAP);
        enterpriseConfig.setIdentity("testIdentity1");
        enterpriseConfig.setPassword("AnDrO1D");

        KeyStore keyStore = mock(KeyStore.class);
        Credential credential = new Credential(enterpriseConfig, keyStore, false);

        HomeSP newHomeSP = new HomeSP(Collections.<String, Long>emptyMap(), fqdn,
                roamingConsortiums, Collections.<String>emptySet(),
                Collections.<Long>emptySet(), Collections.<Long>emptyList(),
                friendlyName, null, credential);

        moMgr.addSP(newHomeSP);
    }

    private void addMoFromXml(PasspointManagementObjectManager moMgr) throws Exception {
        InputStream in = getClass().getClassLoader().getResourceAsStream(R2_TTLS_XML_FILE);
        BufferedReader reader = new BufferedReader(new InputStreamReader(in));
        StringBuilder builder = new StringBuilder();
        String line;
        while ((line = reader.readLine()) != null) {
            builder.append(line).append("\n");
        }

        String xml = builder.toString();
        moMgr.addSP(xml);
    }

    private static final String R1_CONFIG_FILE     = "assets/r1.PerProviderSubscription.conf";
    private static final String R2_CONFIG_FILE     = "assets/r2.PerProviderSubscription.conf";
    private static final String R2_TTLS_XML_FILE   = "assets/r2-ttls-tree.xml";
    private static final String RUCKUS_CONFIG_FILE = "assets/ruckus.PerProviderSubscription.conf";

    /** ensure that loading R1 configs works */
    @Test
    public void loadR1Configs() throws Exception {
        File file = createFileFromResource(R1_CONFIG_FILE);
        PasspointManagementObjectManager moMgr = new PasspointManagementObjectManager(file, true);
        List<HomeSP> homeSPs = moMgr.loadAllSPs();
        assertEquals(2, homeSPs.size());

        /* TODO: Verify more attributes */
        HomeSP homeSP = moMgr.getHomeSP("twcwifi.com");
        assertNotNull(homeSP);
        assertEquals("TWC-WiFi", homeSP.getFriendlyName());
        assertEquals("tushar4", homeSP.getCredential().getUserName());

        homeSP = moMgr.getHomeSP("wi-fi.org");
        assertNotNull(homeSP);
        assertEquals("Wi-Fi Alliance", homeSP.getFriendlyName());
        assertEquals("sta006", homeSP.getCredential().getUserName());
    }

    /** ensure that loading R2 configs works */
    @Test
    public void loadR2Configs() throws Exception {
        File file = createFileFromResource(R2_CONFIG_FILE);
        PasspointManagementObjectManager moMgr = new PasspointManagementObjectManager(file, true);
        List<HomeSP> homeSPs = moMgr.loadAllSPs();
        assertEquals(2, homeSPs.size());

        /* TODO: Verify more attributes */
        HomeSP homeSP = moMgr.getHomeSP("twcwifi.com");
        assertNotNull(homeSP);
        assertEquals("TWC-WiFi", homeSP.getFriendlyName());
        assertEquals("tushar4", homeSP.getCredential().getUserName());

        homeSP = moMgr.getHomeSP("wi-fi.org");
        assertNotNull(homeSP);
        assertEquals("Wi-Fi Alliance", homeSP.getFriendlyName());
        assertEquals("sta015", homeSP.getCredential().getUserName());
    }

    /** verify adding a new service provider works */
    @Test
    public void addSP() throws Exception {
        File file = tempFolder.newFile("PerProviderSubscription.conf");
        PasspointManagementObjectManager moMgr = new PasspointManagementObjectManager(file, true);
        List<HomeSP> homeSPs = moMgr.loadAllSPs();
        assertEquals(0, homeSPs.size());

        addMoFromWifiConfig(moMgr);
        addMoFromXml(moMgr);

        PasspointManagementObjectManager moMgr2 = new PasspointManagementObjectManager(file, true);
        homeSPs = moMgr2.loadAllSPs();
        assertEquals(2, homeSPs.size());

        /* TODO: Verify more attributes */
        HomeSP homeSP = moMgr2.getHomeSP("rk-ttls.org");
        assertNotNull(homeSP);
        assertEquals("RK TTLS", homeSP.getFriendlyName());
        assertEquals("sta020", homeSP.getCredential().getUserName());

        homeSP = moMgr.getHomeSP("tunisia.org");
        assertNotNull(homeSP);
        assertEquals("Tunisian Passpoint Provider", homeSP.getFriendlyName());
        assertEquals("testIdentity1", homeSP.getCredential().getUserName());
    }

    /** verify that xml serialization/deserialization works */
    public void checkXml() throws Exception {
        InputStream in = getClass().getClassLoader().getResourceAsStream(R2_TTLS_XML_FILE);
        BufferedReader reader = new BufferedReader(new InputStreamReader(in));
        StringBuilder builder = new StringBuilder();
        String line;
        while ((line = reader.readLine()) != null) {
            builder.append(line).append("\n");
        }

        String xmlIn = builder.toString();

        try {
            // Parse the file content:
            OMAParser parser = new OMAParser();
            MOTree moTree = parser.parse(xmlIn, "");
            XMLNode rootIn = parser.getRoot();

            // Serialize it back out:
            String xmlOut = moTree.toXml();
            parser = new OMAParser();
            // And parse it again:
            parser.parse(xmlOut, "");
            XMLNode rootOut = parser.getRoot();

            // Compare the two roots:
            assertTrue("Checking serialized XML", rootIn.equals(rootOut));
        } catch (SAXException se) {
            throw new IOException(se);
        }
    }

    /** verify modifying an existing service provider works */
    @Test
    public void modifySP() throws Exception {
        File file = createFileFromResource(R2_CONFIG_FILE);
        PasspointManagementObjectManager moMgr = new PasspointManagementObjectManager(file, true);
        List<HomeSP> homeSPs = moMgr.loadAllSPs();
        assertEquals(2, homeSPs.size());

        /* verify that wi-fi.org has update identifier of 1 */
        HomeSP homeSP = moMgr.getHomeSP("wi-fi.org");
        assertNotNull(homeSP);
        assertEquals("Wi-Fi Alliance", homeSP.getFriendlyName());
        assertEquals("sta015", homeSP.getCredential().getUserName());
        assertEquals(1, homeSP.getUpdateIdentifier());

        /* PasspointManagementObjectDefinition to change update identifier */
        String urn = "wfa:mo:hotspot2dot0-perprovidersubscription:1.0";
        String baseUri = "./Wi-Fi/wi-fi.org/PerProviderSubscription/UpdateIdentifier";
        String xmlTree =
                  "<MgmtTree>\n"
                + "     <VerDTD>1.2</VerDTD>\n"
                + "     <Node>\n"
                + "         <NodeName>UpdateIdentifier</NodeName>\n"
                + "         <Value>9</Value>\n"
                + "     </Node>\n"
                + "</MgmtTree>";

        PasspointManagementObjectDefinition moDef =
                new PasspointManagementObjectDefinition(baseUri, urn, xmlTree);
        List<PasspointManagementObjectDefinition> moDefs =
                new ArrayList<PasspointManagementObjectDefinition>();
        moDefs.add(moDef);
        moMgr.modifySP("wi-fi.org", moDefs);

        /* reload all the SPs again */
        moMgr.loadAllSPs();

        homeSP = moMgr.getHomeSP("wi-fi.org");
        assertEquals("Wi-Fi Alliance", homeSP.getFriendlyName());
        assertEquals("sta015", homeSP.getCredential().getUserName());
        assertEquals(9, homeSP.getUpdateIdentifier());
    }

    /** verify removing an existing service provider works */
    @Test
    public void removeSP() throws Exception {
        File file = createFileFromResource(R2_CONFIG_FILE);
        PasspointManagementObjectManager moMgr = new PasspointManagementObjectManager(file, true);
        List<HomeSP> homeSPs = moMgr.loadAllSPs();
        assertEquals(2, homeSPs.size());

        moMgr.removeSP("wi-fi.org");

        homeSPs = moMgr.loadAllSPs();
        assertEquals(null, moMgr.getHomeSP("wi-fi.org"));
    }
}









