/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.compatibility.common.tradefed.testtype;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildProvider;
import com.android.compatibility.common.tradefed.testtype.ModuleRepo.ConfigFilter;
import com.android.compatibility.common.tradefed.testtype.IModuleDef;
import com.android.compatibility.common.util.AbiUtils;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IShardableTest;
import com.android.tradefed.util.FileUtil;

import junit.framework.TestCase;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class ModuleRepoTest extends TestCase {

    private static final String TOKEN =
            "<target_preparer class=\"com.android.compatibility.common.tradefed.targetprep.TokenRequirement\">\n"
            + "<option name=\"token\" value=\"%s\" />\n"
            + "</target_preparer>\n";
    private static final String CONFIG =
            "<configuration description=\"Auto Generated File\">\n" +
            "%s" +
            "<test class=\"com.android.compatibility.common.tradefed.testtype.%s\">\n" +
            "<option name=\"module\" value=\"%s\" />" +
            "</test>\n" +
            "</configuration>";
    private static final String FOOBAR_TOKEN = "foobar";
    private static final String SERIAL1 = "abc";
    private static final String SERIAL2 = "def";
    private static final String SERIAL3 = "ghi";
    private static final Set<String> SERIALS = new HashSet<>();
    private static final Set<IAbi> ABIS = new HashSet<>();
    private static final List<String> DEVICE_TOKENS = new ArrayList<>();
    private static final List<String> TEST_ARGS= new ArrayList<>();
    private static final List<String> MODULE_ARGS = new ArrayList<>();
    private static final List<String> INCLUDES = new ArrayList<>();
    private static final List<String> EXCLUDES = new ArrayList<>();
    private static final Set<String> FILES = new HashSet<>();
    private static final String FILENAME = "%s.config";
    private static final String ABI_32 = "armeabi-v7a";
    private static final String ABI_64 = "arm64-v8a";
    private static final String MODULE_NAME_A = "FooModuleA";
    private static final String MODULE_NAME_B = "FooModuleB";
    private static final String MODULE_NAME_C = "FooModuleC";
    private static final String ID_A_32 = AbiUtils.createId(ABI_32, MODULE_NAME_A);
    private static final String ID_A_64 = AbiUtils.createId(ABI_64, MODULE_NAME_A);
    private static final String ID_B_32 = AbiUtils.createId(ABI_32, MODULE_NAME_B);
    private static final String ID_B_64 = AbiUtils.createId(ABI_64, MODULE_NAME_B);
    private static final String ID_C_32 = AbiUtils.createId(ABI_32, MODULE_NAME_C);
    private static final String ID_C_64 = AbiUtils.createId(ABI_64, MODULE_NAME_C);
    private static final String TEST_ARG = TestStub.class.getName() + ":foo:bar";
    private static final String MODULE_ARG = "%s:blah:foobar";
    private static final String TEST_STUB = "TestStub"; // Trivial test stub
    private static final String SHARDABLE_TEST_STUB = "ShardableTestStub"; // Shardable and IBuildReceiver
    private static final String [] EXPECTED_MODULE_IDS = new String[] {
        "arm64-v8a FooModuleB",
        "arm64-v8a FooModuleC",
        "armeabi-v7a FooModuleA",
        "arm64-v8a FooModuleA",
        "armeabi-v7a FooModuleC",
        "armeabi-v7a FooModuleB"
    };

    static {
        SERIALS.add(SERIAL1);
        SERIALS.add(SERIAL2);
        SERIALS.add(SERIAL3);
        ABIS.add(new Abi(ABI_32, "32"));
        ABIS.add(new Abi(ABI_64, "64"));
        DEVICE_TOKENS.add(String.format("%s:%s", SERIAL3, FOOBAR_TOKEN));
        TEST_ARGS.add(TEST_ARG);
        MODULE_ARGS.add(String.format(MODULE_ARG, MODULE_NAME_A));
        MODULE_ARGS.add(String.format(MODULE_ARG, MODULE_NAME_B));
        MODULE_ARGS.add(String.format(MODULE_ARG, MODULE_NAME_C));
        FILES.add(String.format(FILENAME, MODULE_NAME_A));
        FILES.add(String.format(FILENAME, MODULE_NAME_B));
        FILES.add(String.format(FILENAME, MODULE_NAME_C));
    }
    private IModuleRepo mRepo;
    private File mTestsDir;
    private IBuildInfo mBuild;

    @Override
    public void setUp() throws Exception {
        mTestsDir = setUpConfigs();
        mRepo = new ModuleRepo();
        mBuild = new CompatibilityBuildProvider().getBuild();
    }

    private File setUpConfigs() throws IOException {
        File testsDir = FileUtil.createNamedTempDir("testcases");
        createConfig(testsDir, MODULE_NAME_A, null);
        createConfig(testsDir, MODULE_NAME_B, null);
        createConfig(testsDir, MODULE_NAME_C, FOOBAR_TOKEN);
        return testsDir;
    }

    private void createConfig(File testsDir, String name, String token) throws IOException {
        createConfig(testsDir, name, token, TEST_STUB);
    }

    private void createConfig(File testsDir, String name, String token, String moduleClass) throws IOException {
        File config = new File(testsDir, String.format(FILENAME, name));
        String preparer = "";
        if (token != null) {
            preparer = String.format(TOKEN, token);
        }
        FileUtil.writeToFile(String.format(CONFIG, preparer, moduleClass, name), config);
    }

    @Override
    public void tearDown() throws Exception {
        tearDownConfigs(mTestsDir);
    }

    private void tearDownConfigs(File testsDir) {
        FileUtil.recursiveDelete(testsDir);
    }

    public void testInitialization() throws Exception {
        mRepo.initialize(3, mTestsDir, ABIS, DEVICE_TOKENS, TEST_ARGS, MODULE_ARGS, INCLUDES,
                EXCLUDES, mBuild);
        assertTrue("Should be initialized", mRepo.isInitialized());
        assertEquals("Wrong number of shards", 3, mRepo.getNumberOfShards());
        assertEquals("Wrong number of modules per shard", 2, mRepo.getModulesPerShard());
        Map<String, Set<String>> deviceTokens = mRepo.getDeviceTokens();
        assertEquals("Wrong number of devices with tokens", 1, deviceTokens.size());
        Set<String> tokens = deviceTokens.get(SERIAL3);
        assertEquals("Wrong number of tokens", 1, tokens.size());
        assertTrue("Unexpected device token", tokens.contains(FOOBAR_TOKEN));
        assertEquals("Wrong number of modules", 0, mRepo.getLargeModules().size());
        assertEquals("Wrong number of modules", 0, mRepo.getMediumModules().size());
        assertEquals("Wrong number of modules", 4, mRepo.getSmallModules().size());
        List<IModuleDef> tokenModules = mRepo.getTokenModules();
        assertEquals("Wrong number of modules with tokens", 2, tokenModules.size());
        List<IModuleDef> serial1Modules = mRepo.getModules(SERIAL1);
        assertEquals("Wrong number of modules", 2, serial1Modules.size());
        List<IModuleDef> serial2Modules = mRepo.getModules(SERIAL2);
        assertEquals("Wrong number of modules", 2, serial2Modules.size());
        List<IModuleDef> serial3Modules = mRepo.getModules(SERIAL3);
        assertEquals("Wrong number of modules", 2, serial3Modules.size());
        // Serial 3 should have the modules with tokens
        for (IModuleDef module : serial3Modules) {
            assertEquals("Wrong module", MODULE_NAME_C, module.getName());
        }
        Set<String> serials = mRepo.getSerials();
        assertEquals("Wrong number of serials", 3, serials.size());
        assertTrue("Unexpected device serial", serials.containsAll(SERIALS));
    }

    public void testConfigFilter() throws Exception {
        File[] configFiles = mTestsDir.listFiles(new ConfigFilter());
        assertEquals("Wrong number of config files found.", 3, configFiles.length);
        for (File file : configFiles) {
            assertTrue(String.format("Unrecognised file: %s", file.getAbsolutePath()),
                    FILES.contains(file.getName()));
        }
    }

    public void testFiltering() throws Exception {
        List<String> includeFilters = new ArrayList<>();
        includeFilters.add(MODULE_NAME_A);
        List<String> excludeFilters = new ArrayList<>();
        excludeFilters.add(ID_A_32);
        excludeFilters.add(MODULE_NAME_B);
        mRepo.initialize(1, mTestsDir, ABIS, DEVICE_TOKENS, TEST_ARGS, MODULE_ARGS, includeFilters,
                excludeFilters, mBuild);
        List<IModuleDef> modules = mRepo.getModules(SERIAL1);
        assertEquals("Incorrect number of modules", 1, modules.size());
        IModuleDef module = modules.get(0);
        assertEquals("Incorrect ID", ID_A_64, module.getId());
        checkArgs(module);
    }

    public void testParsing() throws Exception {
        mRepo.initialize(1, mTestsDir, ABIS, DEVICE_TOKENS, TEST_ARGS, MODULE_ARGS, INCLUDES,
                EXCLUDES, mBuild);
        List<IModuleDef> modules = mRepo.getModules(SERIAL3);
        Set<String> idSet = new HashSet<>();
        for (IModuleDef module : modules) {
            idSet.add(module.getId());
        }
        assertEquals("Incorrect number of IDs", 6, idSet.size());
        assertTrue("Missing ID_A_32", idSet.contains(ID_A_32));
        assertTrue("Missing ID_A_64", idSet.contains(ID_A_64));
        assertTrue("Missing ID_B_32", idSet.contains(ID_B_32));
        assertTrue("Missing ID_B_64", idSet.contains(ID_B_64));
        assertTrue("Missing ID_C_32", idSet.contains(ID_C_32));
        assertTrue("Missing ID_C_64", idSet.contains(ID_C_64));
        for (IModuleDef module : modules) {
            checkArgs(module);
        }
    }

    private void checkArgs(IModuleDef module) {
        IRemoteTest test = module.getTest();
        assertTrue("Incorrect test type", test instanceof TestStub);
        TestStub stub = (TestStub) test;
        assertEquals("Incorrect test arg", "bar", stub.mFoo);
        assertEquals("Incorrect module arg", "foobar", stub.mBlah);
    }

    public void testSplit() throws Exception {
        createConfig(mTestsDir, "sharder_1", null, SHARDABLE_TEST_STUB);
        createConfig(mTestsDir, "sharder_2", null, SHARDABLE_TEST_STUB);
        createConfig(mTestsDir, "sharder_3", null, SHARDABLE_TEST_STUB);
        Set<IAbi> abis = new HashSet<>();
        abis.add(new Abi(ABI_64, "64"));
        ArrayList<String> emptyList = new ArrayList<>();

        mRepo.initialize(3, mTestsDir, abis, DEVICE_TOKENS, emptyList, emptyList, emptyList,
                         emptyList, mBuild);

        List<IModuleDef> modules = new ArrayList<>();
        modules.addAll(mRepo.getLargeModules());
        modules.addAll(mRepo.getMediumModules());
        modules.addAll(mRepo.getSmallModules());
        modules.addAll(mRepo.getTokenModules());

        int shardableCount = 0;
        for (IModuleDef def : modules) {
            IRemoteTest test = def.getTest();
            if (test instanceof IShardableTest) {
                assertNotNull("Build not set", ((ShardableTestStub)test).mBuildInfo);
                shardableCount++;
            }
        }
        assertEquals("Shards wrong", 3*3, shardableCount);
    }

    public void testGetModuleIds() {
        mRepo.initialize(3, mTestsDir, ABIS, DEVICE_TOKENS, TEST_ARGS, MODULE_ARGS, INCLUDES,
                EXCLUDES, mBuild);
        assertTrue("Should be initialized", mRepo.isInitialized());

        assertArrayEquals(EXPECTED_MODULE_IDS, mRepo.getModuleIds());
    }

    private void assertArrayEquals(Object[] expected, Object[] actual) {
        assertEquals(Arrays.asList(expected), Arrays.asList(actual));
    }
}
