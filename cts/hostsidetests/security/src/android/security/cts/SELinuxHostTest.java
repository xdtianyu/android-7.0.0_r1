/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.security.cts;

import android.platform.test.annotations.RestrictedBuildTest;

import com.android.cts.migration.MigrationHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.String;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.Scanner;
import java.util.Set;

/**
 * Host-side SELinux tests.
 *
 * These tests analyze the policy file in use on the subject device directly or
 * run as the shell user to evaluate aspects of the state of SELinux on the test
 * device which otherwise would not be available to a normal apk.
 */
public class SELinuxHostTest extends DeviceTestCase implements IBuildReceiver, IDeviceTest {

    private File sepolicyAnalyze;
    private File checkSeapp;
    private File checkFc;
    private File aospSeappFile;
    private File aospFcFile;
    private File aospPcFile;
    private File aospSvcFile;
    private File devicePolicyFile;
    private File deviceSeappFile;
    private File deviceFcFile;
    private File devicePcFile;
    private File deviceSvcFile;
    private File seappNeverAllowFile;

    private IBuildInfo mBuild;

    /**
     * A reference to the device under test.
     */
    private ITestDevice mDevice;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo build) {
        mBuild = build;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        super.setDevice(device);
        mDevice = device;
    }

    private File copyResourceToTempFile(String resName) throws IOException {
        InputStream is = this.getClass().getResourceAsStream(resName);
        File tempFile = File.createTempFile("SELinuxHostTest", ".tmp");
        FileOutputStream os = new FileOutputStream(tempFile);
        int rByte = 0;
        while ((rByte = is.read()) != -1) {
            os.write(rByte);
        }
        os.flush();
        os.close();
        tempFile.deleteOnExit();
        return tempFile;
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        sepolicyAnalyze = MigrationHelper.getTestFile(mBuild, "sepolicy-analyze");
        sepolicyAnalyze.setExecutable(true);

        /* obtain sepolicy file from running device */
        devicePolicyFile = File.createTempFile("sepolicy", ".tmp");
        devicePolicyFile.deleteOnExit();
        mDevice.pullFile("/sys/fs/selinux/policy", devicePolicyFile);
    }

    /**
     * Tests that the kernel is enforcing selinux policy globally.
     *
     * @throws Exception
     */
    public void testGlobalEnforcing() throws Exception {
        CollectingOutputReceiver out = new CollectingOutputReceiver();
        mDevice.executeShellCommand("cat /sys/fs/selinux/enforce", out);
        assertEquals("SELinux policy is not being enforced!", "1", out.getOutput());
    }

    /**
     * Tests that all domains in the running policy file are in enforcing mode
     *
     * @throws Exception
     */
    @RestrictedBuildTest
    public void testAllDomainsEnforcing() throws Exception {

        /* run sepolicy-analyze permissive check on policy file */
        ProcessBuilder pb = new ProcessBuilder(sepolicyAnalyze.getAbsolutePath(),
                devicePolicyFile.getAbsolutePath(), "permissive");
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue("The following SELinux domains were found to be in permissive mode:\n"
                   + errorString, errorString.length() == 0);
    }

    /**
     * Asserts that specified type is not associated with the specified
     * attribute.
     *
     * @param attribute
     *  The attribute name.
     * @param type
     *  The type name.
     */
    private void assertNotInAttribute(String attribute, String badtype) throws Exception {
        /* run sepolicy-analyze attribute check on policy file */
        ProcessBuilder pb = new ProcessBuilder(sepolicyAnalyze.getAbsolutePath(),
                devicePolicyFile.getAbsolutePath(), "attribute", attribute);
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String type;
        while ((type = result.readLine()) != null) {
            assertFalse("Attribute " + attribute + " includes " + type + "\n",
                        type.equals(badtype));
        }
    }

    /**
     * Tests that mlstrustedsubject does not include untrusted_app
     * and that mlstrustedobject does not include app_data_file.
     * This helps prevent circumventing the per-user isolation of
     * normal apps via levelFrom=user.
     *
     * @throws Exception
     */
    public void testMLSAttributes() throws Exception {
        assertNotInAttribute("mlstrustedsubject", "untrusted_app");
        assertNotInAttribute("mlstrustedobject", "app_data_file");
    }

    /**
     * Tests that the seapp_contexts file on the device is valid.
     *
     * @throws Exception
     */
    public void testValidSeappContexts() throws Exception {

        /* obtain seapp_contexts file from running device */
        deviceSeappFile = File.createTempFile("seapp_contexts", ".tmp");
        deviceSeappFile.deleteOnExit();
        mDevice.pullFile("/seapp_contexts", deviceSeappFile);

        /* retrieve the checkseapp executable from jar */
        checkSeapp = copyResourceToTempFile("/checkseapp");
        checkSeapp.setExecutable(true);

        /* retrieve the AOSP seapp_neverallows file from jar */
        seappNeverAllowFile = copyResourceToTempFile("/general_seapp_neverallows");

        /* run checkseapp on seapp_contexts */
        ProcessBuilder pb = new ProcessBuilder(checkSeapp.getAbsolutePath(),
                "-p", devicePolicyFile.getAbsolutePath(),
                seappNeverAllowFile.getAbsolutePath(),
                deviceSeappFile.getAbsolutePath());
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue("The seapp_contexts file was invalid:\n"
                   + errorString, errorString.length() == 0);
    }

    /**
     * Asserts that the actual file contents starts with the expected file
     * contents.
     *
     * @param expectedFile
     *  The file with the expected contents.
     * @param actualFile
     *  The actual file being checked.
     */
    private void assertFileStartsWith(File expectedFile, File actualFile) throws Exception {
        BufferedReader expectedReader = new BufferedReader(new FileReader(expectedFile.getAbsolutePath()));
        BufferedReader actualReader = new BufferedReader(new FileReader(actualFile.getAbsolutePath()));
        String expectedLine, actualLine;
        while ((expectedLine = expectedReader.readLine()) != null) {
            actualLine = actualReader.readLine();
            assertEquals("Lines do not match:", expectedLine, actualLine);
        }
    }

    /**
     * Tests that the seapp_contexts file on the device contains
     * the standard AOSP entries.
     *
     * @throws Exception
     */
    public void testAospSeappContexts() throws Exception {

        /* obtain seapp_contexts file from running device */
        deviceSeappFile = File.createTempFile("seapp_contexts", ".tmp");
        deviceSeappFile.deleteOnExit();
        mDevice.pullFile("/seapp_contexts", deviceSeappFile);

        /* retrieve the AOSP seapp_contexts file from jar */
        aospSeappFile = copyResourceToTempFile("/general_seapp_contexts");

        assertFileStartsWith(aospSeappFile, deviceSeappFile);
    }

    /**
     * Tests that the file_contexts.bin file on the device contains
     * the standard AOSP entries.
     *
     * @throws Exception
     */
    public void testAospFileContexts() throws Exception {

        /* retrieve the checkfc executable from jar */
        checkFc = copyResourceToTempFile("/checkfc");
        checkFc.setExecutable(true);

        /* obtain file_contexts.bin file from running device */
        deviceFcFile = File.createTempFile("file_contexts", ".bin");
        deviceFcFile.deleteOnExit();
        mDevice.pullFile("/file_contexts.bin", deviceFcFile);

        /* retrieve the AOSP file_contexts file from jar */
        aospFcFile = copyResourceToTempFile("/general_file_contexts.bin");

        /* run checkfc -c general_file_contexts.bin file_contexts.bin */
        ProcessBuilder pb = new ProcessBuilder(checkFc.getAbsolutePath(),
                "-c", aospFcFile.getAbsolutePath(),
                deviceFcFile.getAbsolutePath());
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line = result.readLine();
        assertTrue("The file_contexts.bin file did not include the AOSP entries:\n"
                   + line + "\n",
                   line.equals("equal") || line.equals("subset"));
    }

    /**
     * Tests that the property_contexts file on the device contains
     * the standard AOSP entries.
     *
     * @throws Exception
     */
    public void testAospPropertyContexts() throws Exception {

        /* obtain property_contexts file from running device */
        devicePcFile = File.createTempFile("property_contexts", ".tmp");
        devicePcFile.deleteOnExit();
        mDevice.pullFile("/property_contexts", devicePcFile);

        /* retrieve the AOSP property_contexts file from jar */
        aospPcFile = copyResourceToTempFile("/general_property_contexts");

        assertFileStartsWith(aospPcFile, devicePcFile);
    }

    /**
     * Tests that the service_contexts file on the device contains
     * the standard AOSP entries.
     *
     * @throws Exception
     */
    public void testAospServiceContexts() throws Exception {

        /* obtain service_contexts file from running device */
        deviceSvcFile = File.createTempFile("service_contexts", ".tmp");
        deviceSvcFile.deleteOnExit();
        mDevice.pullFile("/service_contexts", deviceSvcFile);

        /* retrieve the AOSP service_contexts file from jar */
        aospSvcFile = copyResourceToTempFile("/general_service_contexts");

        assertFileStartsWith(aospSvcFile, deviceSvcFile);
    }

    /**
     * Tests that the file_contexts.bin file on the device is valid.
     *
     * @throws Exception
     */
    public void testValidFileContexts() throws Exception {

        /* retrieve the checkfc executable from jar */
        checkFc = copyResourceToTempFile("/checkfc");
        checkFc.setExecutable(true);

        /* obtain file_contexts.bin file from running device */
        deviceFcFile = File.createTempFile("file_contexts", ".bin");
        deviceFcFile.deleteOnExit();
        mDevice.pullFile("/file_contexts.bin", deviceFcFile);

        /* run checkfc sepolicy file_contexts.bin */
        ProcessBuilder pb = new ProcessBuilder(checkFc.getAbsolutePath(),
                devicePolicyFile.getAbsolutePath(),
                deviceFcFile.getAbsolutePath());
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue("The file_contexts.bin file was invalid:\n"
                   + errorString, errorString.length() == 0);
    }

    /**
     * Tests that the property_contexts file on the device is valid.
     *
     * @throws Exception
     */
    public void testValidPropertyContexts() throws Exception {

        /* retrieve the checkfc executable from jar */
        checkFc = copyResourceToTempFile("/checkfc");
        checkFc.setExecutable(true);

        /* obtain property_contexts file from running device */
        devicePcFile = File.createTempFile("property_contexts", ".tmp");
        devicePcFile.deleteOnExit();
        mDevice.pullFile("/property_contexts", devicePcFile);

        /* run checkfc -p on property_contexts */
        ProcessBuilder pb = new ProcessBuilder(checkFc.getAbsolutePath(),
                "-p", devicePolicyFile.getAbsolutePath(),
                devicePcFile.getAbsolutePath());
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue("The property_contexts file was invalid:\n"
                   + errorString, errorString.length() == 0);
    }

    /**
     * Tests that the service_contexts file on the device is valid.
     *
     * @throws Exception
     */
    public void testValidServiceContexts() throws Exception {

        /* retrieve the checkfc executable from jar */
        checkFc = copyResourceToTempFile("/checkfc");
        checkFc.setExecutable(true);

        /* obtain service_contexts file from running device */
        deviceSvcFile = File.createTempFile("service_contexts", ".tmp");
        deviceSvcFile.deleteOnExit();
        mDevice.pullFile("/service_contexts", deviceSvcFile);

        /* run checkfc -s on service_contexts */
        ProcessBuilder pb = new ProcessBuilder(checkFc.getAbsolutePath(),
                "-s", devicePolicyFile.getAbsolutePath(),
                deviceSvcFile.getAbsolutePath());
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue("The service_contexts file was invalid:\n"
                   + errorString, errorString.length() == 0);
    }

   /**
     * Tests that the policy defines no booleans (runtime conditional policy).
     *
     * @throws Exception
     */
    public void testNoBooleans() throws Exception {

        /* run sepolicy-analyze booleans check on policy file */
        ProcessBuilder pb = new ProcessBuilder(sepolicyAnalyze.getAbsolutePath(),
                devicePolicyFile.getAbsolutePath(), "booleans");
        pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
        pb.redirectErrorStream(true);
        Process p = pb.start();
        p.waitFor();
        BufferedReader result = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String line;
        StringBuilder errorString = new StringBuilder();
        while ((line = result.readLine()) != null) {
            errorString.append(line);
            errorString.append("\n");
        }
        assertTrue("The policy contained booleans:\n"
                   + errorString, errorString.length() == 0);
    }

    /**
     * Tests that important domain labels are being appropriately applied.
     */

    /**
     * Asserts that no processes are running in a domain.
     *
     * @param domain
     *  The domain or SELinux context to check.
     */
    private void assertDomainEmpty(String domain) throws DeviceNotAvailableException {
        List<ProcessDetails> procs = ProcessDetails.getProcMap(mDevice).get(domain);
        String msg = "Expected no processes in SELinux domain \"" + domain + "\""
            + " Found: \"" + procs + "\"";
        assertNull(msg, procs);
    }

    /**
     * Asserts that a domain exists and that only one, well defined, process is
     * running in that domain.
     *
     * @param domain
     *  The domain or SELinux context to check.
     * @param executable
     *  The path of the executable or application package name.
     */
    private void assertDomainOne(String domain, String executable) throws DeviceNotAvailableException {
        List<ProcessDetails> procs = ProcessDetails.getProcMap(mDevice).get(domain);
        List<ProcessDetails> exeProcs = ProcessDetails.getExeMap(mDevice).get(executable);
        String msg = "Expected 1 process in SELinux domain \"" + domain + "\""
            + " Found \"" + procs + "\"";
        assertNotNull(msg, procs);
        assertEquals(msg, 1, procs.size());

        msg = "Expected executable \"" + executable + "\" in SELinux domain \"" + domain + "\""
            + "Found: \"" + procs + "\"";
        assertEquals(msg, executable, procs.get(0).procTitle);

        msg = "Expected 1 process with executable \"" + executable + "\""
            + " Found \"" + procs + "\"";
        assertNotNull(msg, exeProcs);
        assertEquals(msg, 1, exeProcs.size());

        msg = "Expected executable \"" + executable + "\" in SELinux domain \"" + domain + "\""
            + "Found: \"" + procs + "\"";
        assertEquals(msg, domain, exeProcs.get(0).label);
    }

    /**
     * Asserts that a domain may exist. If a domain exists, the cardinality of
     * the domain is verified to be 1 and that the correct process is running in
     * that domain.
     *
     * @param domain
     *  The domain or SELinux context to check.
     * @param executable
     *  The path of the executable or application package name.
     */
    private void assertDomainZeroOrOne(String domain, String executable)
        throws DeviceNotAvailableException {
        List<ProcessDetails> procs = ProcessDetails.getProcMap(mDevice).get(domain);
        List<ProcessDetails> exeProcs = ProcessDetails.getExeMap(mDevice).get(executable);

        if (procs != null) {
            String msg = "Expected 1 process in SELinux domain \"" + domain + "\""
            + " Found: \"" + procs + "\"";
            assertEquals(msg, 1, procs.size());

            msg = "Expected executable \"" + executable + "\" in SELinux domain \"" + domain + "\""
                + "Found: \"" + procs.get(0) + "\"";
            assertEquals(msg, executable, procs.get(0).procTitle);
        }

        if (exeProcs != null) {
            String msg = "Expected 1 process with executable \"" + executable + "\""
            + " Found: \"" + procs + "\"";
            assertEquals(msg, 1, exeProcs.size());

            msg = "Expected executable \"" + executable + "\" in SELinux domain \"" + domain + "\""
                + "Found: \"" + procs.get(0) + "\"";
            assertEquals(msg, domain, exeProcs.get(0).label);
        }
    }

    /**
     * Asserts that a domain must exist, and that the cardinality is greater
     * than or equal to 1.
     *
     * @param domain
     *  The domain or SELinux context to check.
     * @param executables
     *  The path of the allowed executables or application package names.
     */
    private void assertDomainN(String domain, String... executables)
        throws DeviceNotAvailableException {
        List<ProcessDetails> procs = ProcessDetails.getProcMap(mDevice).get(domain);
        String msg = "Expected 1 or more processes in SELinux domain but found none.";
        assertNotNull(msg, procs);

        Set<String> execList = new HashSet<String>(Arrays.asList(executables));

        for (ProcessDetails p : procs) {
            msg = "Expected one of \"" + execList + "\" in SELinux domain \"" + domain + "\""
                + " Found: \"" + p + "\"";
            assertTrue(msg, execList.contains(p.procTitle));
        }

        for (String exe : executables) {
            List<ProcessDetails> exeProcs = ProcessDetails.getExeMap(mDevice).get(exe);

            if (exeProcs != null) {
                for (ProcessDetails p : exeProcs) {
                    msg = "Expected executable \"" + exe + "\" in SELinux domain \""
                        + domain + "\"" + " Found: \"" + p + "\"";
                    assertEquals(msg, domain, p.label);
                }
            }
        }
    }

    /**
     * Asserts that a domain, if it exists, is only running the listed executables.
     *
     * @param domain
     *  The domain or SELinux context to check.
     * @param executables
     *  The path of the allowed executables or application package names.
     */
    private void assertDomainHasExecutable(String domain, String... executables)
        throws DeviceNotAvailableException {
        List<ProcessDetails> procs = ProcessDetails.getProcMap(mDevice).get(domain);

        if (procs != null) {
            Set<String> execList = new HashSet<String>(Arrays.asList(executables));

            for (ProcessDetails p : procs) {
                String msg = "Expected one of \"" + execList + "\" in SELinux domain \""
                    + domain + "\"" + " Found: \"" + p + "\"";
                assertTrue(msg, execList.contains(p.procTitle));
            }
        }

        for (String exe : executables) {
            List<ProcessDetails> exeProcs = ProcessDetails.getExeMap(mDevice).get(exe);

            if (exeProcs != null) {
                for (ProcessDetails p : exeProcs) {
                    String msg = "Expected executable \"" + exe + "\" in SELinux domain \""
                        + domain + "\"" + " Found: \"" + p + "\"";
                    assertEquals(msg, domain, p.label);
                }
            }
        }
    }

    /* Init is always there */
    public void testInitDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:init:s0", "/init");
    }

    /* Ueventd is always there */
    public void testUeventdDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:ueventd:s0", "/sbin/ueventd");
    }

    /* Devices always have healthd */
    public void testHealthdDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:healthd:s0", "/sbin/healthd");
    }

    /* Servicemanager is always there */
    public void testServicemanagerDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:servicemanager:s0", "/system/bin/servicemanager");
    }

    /* Vold is always there */
    public void testVoldDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:vold:s0", "/system/bin/vold");
    }

    /* netd is always there */
    public void testNetdDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:netd:s0", "/system/bin/netd");
    }

    /* Debuggerd is always there */
    public void testDebuggerdDomain() throws DeviceNotAvailableException {
        assertDomainN("u:r:debuggerd:s0", "/system/bin/debuggerd", "/system/bin/debuggerd64",
                "debuggerd:signaller", "debuggerd64:signaller");
    }

    /* Surface flinger is always there */
    public void testSurfaceflingerDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:surfaceflinger:s0", "/system/bin/surfaceflinger");
    }

    /* Zygote is always running */
    public void testZygoteDomain() throws DeviceNotAvailableException {
        assertDomainN("u:r:zygote:s0", "zygote", "zygote64");
    }

    /* Checks drmserver for devices that require it */
    public void testDrmServerDomain() throws DeviceNotAvailableException {
        assertDomainZeroOrOne("u:r:drmserver:s0", "/system/bin/drmserver");
    }

    /* Installd is always running */
    public void testInstalldDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:installd:s0", "/system/bin/installd");
    }

    /* keystore is always running */
    public void testKeystoreDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:keystore:s0", "/system/bin/keystore");
    }

    /* System server better be running :-P */
    public void testSystemServerDomain() throws DeviceNotAvailableException {
        assertDomainOne("u:r:system_server:s0", "system_server");
    }

    /*
     * Some OEMs do not use sdcardd so transient. Other OEMs have multiple sdcards
     * so they run the daemon multiple times.
     */
    public void testSdcarddDomain() throws DeviceNotAvailableException {
        assertDomainHasExecutable("u:r:sdcardd:s0", "/system/bin/sdcard");
    }

    /* Watchdogd may or may not be there */
    public void testWatchdogdDomain() throws DeviceNotAvailableException {
        assertDomainZeroOrOne("u:r:watchdogd:s0", "/sbin/watchdogd");
    }

    /* logd may or may not be there */
    public void testLogdDomain() throws DeviceNotAvailableException {
        assertDomainZeroOrOne("u:r:logd:s0", "/system/bin/logd");
    }

    /* lmkd may or may not be there */
    public void testLmkdDomain() throws DeviceNotAvailableException {
        assertDomainZeroOrOne("u:r:lmkd:s0", "/system/bin/lmkd");
    }

    /* Wifi may be off so cardinality of 0 or 1 is ok */
    public void testWpaDomain() throws DeviceNotAvailableException {
        assertDomainZeroOrOne("u:r:wpa:s0", "/system/bin/wpa_supplicant");
    }

    /*
     * Nothing should be running in this domain, cardinality test is all thats
     * needed
     */
    public void testInitShellDomain() throws DeviceNotAvailableException {
        assertDomainEmpty("u:r:init_shell:s0");
    }

    /*
     * Nothing should be running in this domain, cardinality test is all thats
     * needed
     */
    public void testRecoveryDomain() throws DeviceNotAvailableException {
        assertDomainEmpty("u:r:recovery:s0");
    }

    /*
     * Nothing should be running in this domain, cardinality test is all thats
     * needed
     */
    @RestrictedBuildTest
    public void testSuDomain() throws DeviceNotAvailableException {
        assertDomainEmpty("u:r:su:s0");
    }

    /*
     * All kthreads should be in kernel context.
     */
    public void testKernelDomain() throws DeviceNotAvailableException {
        String domain = "u:r:kernel:s0";
        List<ProcessDetails> procs = ProcessDetails.getProcMap(mDevice).get(domain);
        if (procs != null) {
            for (ProcessDetails p : procs) {
                assertTrue("Non Kernel thread \"" + p + "\" found!", p.isKernel());
            }
        }
    }

    private static class ProcessDetails {
        public String label;
        public String user;
        public int pid;
        public int ppid;
        public String procTitle;

        private static HashMap<String, ArrayList<ProcessDetails>> procMap;
        private static HashMap<String, ArrayList<ProcessDetails>> exeMap;
        private static int kernelParentThreadpid = -1;

        ProcessDetails(String label, String user, int pid, int ppid, String procTitle) {
            this.label = label;
            this.user = user;
            this.pid = pid;
            this.ppid = ppid;
            this.procTitle = procTitle;
        }

        @Override
        public String toString() {
            return "label: " + label
                    + " user: " + user
                    + " pid: " + pid
                    + " ppid: " + ppid
                    + " cmd: " + procTitle;
        }


        private static void createProcMap(ITestDevice tDevice) throws DeviceNotAvailableException {

            /* take the output of a ps -Z to do our analysis */
            CollectingOutputReceiver psOut = new CollectingOutputReceiver();
            // TODO: remove "toybox" below and just run "ps"
            tDevice.executeShellCommand("toybox ps -A -o label,user,pid,ppid,cmdline", psOut);
            String psOutString = psOut.getOutput();
            Pattern p = Pattern.compile(
                    "^([\\w_:]+)\\s+([\\w_]+)\\s+(\\d+)\\s+(\\d+)\\s+(\\p{Graph}+)(\\s\\p{Graph}+)*\\s*$",
                    Pattern.MULTILINE);
            Matcher m = p.matcher(psOutString);
            procMap = new HashMap<String, ArrayList<ProcessDetails>>();
            exeMap = new HashMap<String, ArrayList<ProcessDetails>>();
            while(m.find()) {
                String domainLabel = m.group(1);
                String user = m.group(2);
                int pid = Integer.parseInt(m.group(3));
                int ppid = Integer.parseInt(m.group(4));
                String procTitle = m.group(5);
                ProcessDetails proc = new ProcessDetails(domainLabel, user, pid, ppid, procTitle);
                if (procMap.get(domainLabel) == null) {
                    procMap.put(domainLabel, new ArrayList<ProcessDetails>());
                }
                procMap.get(domainLabel).add(proc);
                if (procTitle.equals("[kthreadd]") && ppid == 0) {
                    kernelParentThreadpid = pid;
                }
                if (exeMap.get(procTitle) == null) {
                    exeMap.put(procTitle, new ArrayList<ProcessDetails>());
                }
                exeMap.get(procTitle).add(proc);
            }
        }

        public static HashMap<String, ArrayList<ProcessDetails>> getProcMap(ITestDevice tDevice)
                throws DeviceNotAvailableException{
            if (procMap == null) {
                createProcMap(tDevice);
            }
            return procMap;
        }

        public static HashMap<String, ArrayList<ProcessDetails>> getExeMap(ITestDevice tDevice)
                throws DeviceNotAvailableException{
            if (exeMap == null) {
                createProcMap(tDevice);
            }
            return exeMap;
        }

        public boolean isKernel() {
            return (pid == kernelParentThreadpid || ppid == kernelParentThreadpid);
        }
    }
}
