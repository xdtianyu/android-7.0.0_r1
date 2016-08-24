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

import com.android.compatibility.common.util.AbiUtils;
import com.android.compatibility.common.util.TestFilter;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationFactory;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IShardableTest;
import com.android.tradefed.testtype.ITestFileFilterReceiver;
import com.android.tradefed.testtype.ITestFilterReceiver;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.TimeUtil;

import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * Retrieves Compatibility test module definitions from the repository.
 */
public class ModuleRepo implements IModuleRepo {

    private static final String CONFIG_EXT = ".config";
    private static final Map<String, Integer> ENDING_MODULES = new HashMap<>();
    static {
        ENDING_MODULES.put("CtsMonkeyTestCases", 1);
    }
    private static final long SMALL_TEST = TimeUnit.MINUTES.toMillis(2); // Small tests < 2mins
    private static final long MEDIUM_TEST = TimeUnit.MINUTES.toMillis(10); // Medium tests < 10mins

    private int mShards;
    private int mModulesPerShard;
    private int mSmallModulesPerShard;
    private int mMediumModulesPerShard;
    private int mLargeModulesPerShard;
    private int mModuleCount = 0;
    private Set<String> mSerials = new HashSet<>();
    private Map<String, Set<String>> mDeviceTokens = new HashMap<>();
    private Map<String, Map<String, String>> mTestArgs = new HashMap<>();
    private Map<String, Map<String, String>> mModuleArgs = new HashMap<>();
    private boolean mIncludeAll;
    private Map<String, List<TestFilter>> mIncludeFilters = new HashMap<>();
    private Map<String, List<TestFilter>> mExcludeFilters = new HashMap<>();
    private IConfigurationFactory mConfigFactory = ConfigurationFactory.getInstance();

    private volatile boolean mInitialized = false;

    // Holds all the small tests waiting to be run.
    private List<IModuleDef> mSmallModules = new ArrayList<>();
    // Holds all the medium tests waiting to be run.
    private List<IModuleDef> mMediumModules = new ArrayList<>();
    // Holds all the large tests waiting to be run.
    private List<IModuleDef> mLargeModules = new ArrayList<>();
    // Holds all the tests with tokens waiting to be run. Meaning the DUT must have a specific token.
    private List<IModuleDef> mTokenModules = new ArrayList<>();

    /**
     * {@inheritDoc}
     */
    @Override
    public int getNumberOfShards() {
        return mShards;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int getModulesPerShard() {
        return mModulesPerShard;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Map<String, Set<String>> getDeviceTokens() {
        return mDeviceTokens;
    }

    /**
     * A {@link FilenameFilter} to find all modules in a directory who match the given pattern.
     */
    public static class NameFilter implements FilenameFilter {

        private String mPattern;

        public NameFilter(String pattern) {
            mPattern = pattern;
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean accept(File dir, String name) {
            return name.contains(mPattern) && name.endsWith(CONFIG_EXT);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Set<String> getSerials() {
        return mSerials;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<IModuleDef> getSmallModules() {
        return mSmallModules;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<IModuleDef> getMediumModules() {
        return mMediumModules;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<IModuleDef> getLargeModules() {
        return mLargeModules;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<IModuleDef> getTokenModules() {
        return mTokenModules;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String[] getModuleIds() {
        Set<String> moduleIdSet = new HashSet<>();
        for (IModuleDef moduleDef : mSmallModules) {
            moduleIdSet.add(moduleDef.getId());
        }
        for (IModuleDef moduleDef : mMediumModules) {
            moduleIdSet.add(moduleDef.getId());
        }
        for (IModuleDef moduleDef : mLargeModules) {
            moduleIdSet.add(moduleDef.getId());
        }
        for (IModuleDef moduleDef : mTokenModules) {
            moduleIdSet.add(moduleDef.getId());
        }
        return moduleIdSet.toArray(new String[moduleIdSet.size()]);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isInitialized() {
        return mInitialized;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void initialize(int shards, File testsDir, Set<IAbi> abis, List<String> deviceTokens,
            List<String> testArgs, List<String> moduleArgs, List<String> includeFilters,
            List<String> excludeFilters, IBuildInfo buildInfo) {
        CLog.d("Initializing ModuleRepo\nShards:%d\nTests Dir:%s\nABIs:%s\nDevice Tokens:%s\n" +
                "Test Args:%s\nModule Args:%s\nIncludes:%s\nExcludes:%s",
                shards, testsDir.getAbsolutePath(), abis, deviceTokens, testArgs, moduleArgs,
                includeFilters, excludeFilters);
        mInitialized = true;
        mShards = shards;
        for (String line : deviceTokens) {
            String[] parts = line.split(":");
            if (parts.length == 2) {
                String key = parts[0];
                String value = parts[1];
                Set<String> list = mDeviceTokens.get(key);
                if (list == null) {
                    list = new HashSet<>();
                    mDeviceTokens.put(key, list);
                }
                list.add(value);
            } else {
                throw new IllegalArgumentException(
                        String.format("Could not parse device token: %s", line));
            }
        }
        putArgs(testArgs, mTestArgs);
        putArgs(moduleArgs, mModuleArgs);
        mIncludeAll = includeFilters.isEmpty();
        // Include all the inclusions
        addFilters(includeFilters, mIncludeFilters, abis);
        // Exclude all the exclusions
        addFilters(excludeFilters, mExcludeFilters, abis);

        File[] configFiles = testsDir.listFiles(new ConfigFilter());
        if (configFiles.length == 0) {
            throw new IllegalArgumentException(
                    String.format("No config files found in %s", testsDir.getAbsolutePath()));
        }
        for (File configFile : configFiles) {
            final String name = configFile.getName().replace(CONFIG_EXT, "");
            final String[] pathArg = new String[] { configFile.getAbsolutePath() };
            try {
                // Invokes parser to process the test module config file
                // Need to generate a different config for each ABI as we cannot guarantee the
                // configs are idempotent. This however means we parse the same file multiple times
                for (IAbi abi : abis) {
                    IConfiguration config = mConfigFactory.createConfigurationFromArgs(pathArg);
                    String id = AbiUtils.createId(abi.getName(), name);
                    if (!shouldRunModule(id)) {
                        // If the module should not run tests based on the state of filters,
                        // skip this name/abi combination.
                        continue;
                    }
                    {
                        Map<String, String> args = new HashMap<>();
                        if (mModuleArgs.containsKey(name)) {
                            args.putAll(mModuleArgs.get(name));
                        }
                        if (mModuleArgs.containsKey(id)) {
                            args.putAll(mModuleArgs.get(id));
                        }
                        if (args != null && args.size() > 0) {
                            for (Entry<String, String> entry : args.entrySet()) {
                                config.injectOptionValue(entry.getKey(), entry.getValue());
                            }
                        }
                    }
                    List<IRemoteTest> tests = config.getTests();
                    for (IRemoteTest test : tests) {
                        String className = test.getClass().getName();
                        Map<String, String> args = new HashMap<>();
                        if (mTestArgs.containsKey(className)) {
                            args.putAll(mTestArgs.get(className));
                        }
                        if (args != null && args.size() > 0) {
                            for (Entry<String, String> entry : args.entrySet()) {
                                config.injectOptionValue(entry.getKey(), entry.getValue());
                            }
                        }
                        addFiltersToTest(test, abi, name);
                    }
                    List<IRemoteTest> shardedTests = tests;
                    if (mShards > 1) {
                         shardedTests = splitShardableTests(tests, buildInfo);
                    }
                    for (IRemoteTest test : shardedTests) {
                        if (test instanceof IBuildReceiver) {
                            ((IBuildReceiver)test).setBuild(buildInfo);
                        }
                        addModuleDef(name, abi, test, pathArg);
                    }
                }
            } catch (ConfigurationException e) {
                throw new RuntimeException(String.format("error parsing config file: %s",
                        configFile.getName()), e);
            }
        }
        mModulesPerShard = mModuleCount / shards;
        if (mModuleCount % shards != 0) {
            mModulesPerShard++; // Round up
        }
        mSmallModulesPerShard = mSmallModules.size() / shards;
        mMediumModulesPerShard = mMediumModules.size() / shards;
        mLargeModulesPerShard = mLargeModules.size() / shards;
    }

    private static List<IRemoteTest> splitShardableTests(List<IRemoteTest> tests,
            IBuildInfo buildInfo) {
        ArrayList<IRemoteTest> shardedList = new ArrayList<>(tests.size());
        for (IRemoteTest test : tests) {
            if (test instanceof IShardableTest) {
                if (test instanceof IBuildReceiver) {
                    ((IBuildReceiver)test).setBuild(buildInfo);
                }
                shardedList.addAll(((IShardableTest)test).split());
            } else {
                shardedList.add(test);
            }
        }
        return shardedList;
    }

    private static void addFilters(List<String> stringFilters,
            Map<String, List<TestFilter>> filters, Set<IAbi> abis) {
        for (String filterString : stringFilters) {
            TestFilter filter = TestFilter.createFrom(filterString);
            String abi = filter.getAbi();
            if (abi == null) {
                for (IAbi a : abis) {
                    addFilter(a.getName(), filter, filters);
                }
            } else {
                addFilter(abi, filter, filters);
            }
        }
    }

    private static void addFilter(String abi, TestFilter filter,
            Map<String, List<TestFilter>> filters) {
        getFilter(filters, AbiUtils.createId(abi, filter.getName())).add(filter);
    }

    private static List<TestFilter> getFilter(Map<String, List<TestFilter>> filters, String id) {
        List<TestFilter> fs = filters.get(id);
        if (fs == null) {
            fs = new ArrayList<>();
            filters.put(id, fs);
        }
        return fs;
    }

    private void addModuleDef(String name, IAbi abi, IRemoteTest test,
            String[] configPaths) throws ConfigurationException {
        // Invokes parser to process the test module config file
        IConfiguration config = mConfigFactory.createConfigurationFromArgs(configPaths);
        addModuleDef(new ModuleDef(name, abi, test, config.getTargetPreparers()));
    }

    private void addModuleDef(IModuleDef moduleDef) {
        Set<String> tokens = moduleDef.getTokens();
        if (tokens != null && !tokens.isEmpty()) {
            mTokenModules.add(moduleDef);
        } else if (moduleDef.getRuntimeHint() < SMALL_TEST) {
            mSmallModules.add(moduleDef);
        } else if (moduleDef.getRuntimeHint() < MEDIUM_TEST) {
            mMediumModules.add(moduleDef);
        } else {
            mLargeModules.add(moduleDef);
        }
        mModuleCount++;
    }

    private void addFiltersToTest(IRemoteTest test, IAbi abi, String name) {
        String moduleId = AbiUtils.createId(abi.getName(), name);
        if (!(test instanceof ITestFilterReceiver)) {
            throw new IllegalArgumentException(String.format(
                    "Test in module %s must implement ITestFilterReceiver.", moduleId));
        }
        List<TestFilter> mdIncludes = getFilter(mIncludeFilters, moduleId);
        List<TestFilter> mdExcludes = getFilter(mExcludeFilters, moduleId);
        if (!mdIncludes.isEmpty()) {
            addTestIncludes((ITestFilterReceiver) test, mdIncludes, name);
        }
        if (!mdExcludes.isEmpty()) {
            addTestExcludes((ITestFilterReceiver) test, mdExcludes, name);
        }
    }

    private boolean shouldRunModule(String moduleId) {
        List<TestFilter> mdIncludes = getFilter(mIncludeFilters, moduleId);
        List<TestFilter> mdExcludes = getFilter(mExcludeFilters, moduleId);
        // if including all modules or includes exist for this module, and there are not excludes
        // for the entire module, this module should be run.
        return (mIncludeAll || !mdIncludes.isEmpty()) && !containsModuleExclude(mdExcludes);
    }

    private void addTestIncludes(ITestFilterReceiver test, List<TestFilter> includes,
            String name) {
        if (test instanceof ITestFileFilterReceiver) {
            File includeFile = createFilterFile(name, ".include", includes);
            ((ITestFileFilterReceiver)test).setIncludeTestFile(includeFile);
        } else {
            // add test includes one at a time
            for (TestFilter include : includes) {
                String filterTestName = include.getTest();
                if (filterTestName != null) {
                    test.addIncludeFilter(filterTestName);
                }
            }
        }
    }

    private void addTestExcludes(ITestFilterReceiver test, List<TestFilter> excludes,
            String name) {
        if (test instanceof ITestFileFilterReceiver) {
            File excludeFile = createFilterFile(name, ".exclude", excludes);
            ((ITestFileFilterReceiver)test).setExcludeTestFile(excludeFile);
        } else {
            // add test excludes one at a time
            for (TestFilter exclude : excludes) {
                test.addExcludeFilter(exclude.getTest());
            }
        }
    }

    private File createFilterFile(String prefix, String suffix, List<TestFilter> filters) {
        File filterFile = null;
        PrintWriter out = null;
        try {
            filterFile = FileUtil.createTempFile(prefix, suffix);
            out = new PrintWriter(filterFile);
            for (TestFilter filter : filters) {
                String filterTest = filter.getTest();
                if (filterTest != null) {
                    out.println(filterTest);
                }
            }
            out.flush();
        } catch (IOException e) {
            throw new RuntimeException("Failed to create filter file");
        } finally {
            if (out != null) {
                out.close();
            }
        }
        filterFile.deleteOnExit();
        return filterFile;
    }

    /*
     * Returns true iff one or more test filters in excludes apply to the entire module.
     */
    private boolean containsModuleExclude(Collection<TestFilter> excludes) {
        for (TestFilter exclude : excludes) {
            if (exclude.getTest() == null) {
                return true;
            }
        }
        return false;
    }

    /**
     * A {@link FilenameFilter} to find all the config files in a directory.
     */
    public static class ConfigFilter implements FilenameFilter {

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean accept(File dir, String name) {
            CLog.d("%s/%s", dir.getAbsolutePath(), name);
            return name.endsWith(CONFIG_EXT);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public synchronized List<IModuleDef> getModules(String serial) {
        List<IModuleDef> modules = new ArrayList<>(mModulesPerShard);
        Set<String> tokens = mDeviceTokens.get(serial);
        getModulesWithTokens(tokens, modules);
        getModules(modules);
        mSerials.add(serial);
        if (mSerials.size() == mShards) {
            for (IModuleDef def : mTokenModules) {
                CLog.logAndDisplay(LogLevel.WARN,
                        String.format("No devices found with %s, running %s on %s",
                                def.getTokens(), def.getId(), serial));
                modules.add(def);
            }
            // Add left over modules
            modules.addAll(mLargeModules);
            modules.addAll(mMediumModules);
            modules.addAll(mSmallModules);
        }
        long estimatedTime = 0;
        for (IModuleDef def : modules) {
            estimatedTime += def.getRuntimeHint();
        }
        Collections.sort(modules, new ExecutionOrderComparator());
        CLog.logAndDisplay(LogLevel.INFO, String.format(
                "%s running %s modules, expected to complete in %s",
                serial, modules.size(), TimeUtil.formatElapsedTime(estimatedTime)));
        return modules;
    }

    /**
     * Iterates through the remaining tests that require tokens and if the device has all the
     * required tokens it will queue that module to run on that device, else the module gets put
     * back into the list.
     */
    private void getModulesWithTokens(Set<String> tokens, List<IModuleDef> modules) {
        if (tokens != null) {
            List<IModuleDef> copy = mTokenModules;
            mTokenModules = new ArrayList<>();
            for (IModuleDef module : copy) {
                // If a device has all the tokens required by the module then it can run it.
                if (tokens.containsAll(module.getTokens())) {
                    modules.add(module);
                } else {
                    mTokenModules.add(module);
                }
            }
        }
    }

    /**
     * Adds count modules that do not require tokens, to run on a device.
     */
    private void getModules(List<IModuleDef> modules) {
        // Take the normal share of modules unless the device already has token modules.
        takeModule(mSmallModules, modules, mSmallModulesPerShard - modules.size());
        takeModule(mMediumModules, modules, mMediumModulesPerShard);
        takeModule(mLargeModules, modules, mLargeModulesPerShard);
        // If one bucket runs out, take from any of the others.
        boolean success = true;
        while (success && modules.size() < mModulesPerShard) {
            // Take modules from the buckets until it has enough, or there are no more modules.
            success = takeModule(mSmallModules, modules, 1)
                    || takeModule(mMediumModules, modules, 1)
                    || takeModule(mLargeModules, modules, 1);
        }
    }

    /**
     * Takes count modules from the first list and move it to the second.
     */
    private static boolean takeModule(
            List<IModuleDef> source, List<IModuleDef> destination, int count) {
        if (source.isEmpty()) {
            return false;
        }
        if (count > source.size()) {
            count = source.size();
        }
        for (int i = 0; i < count; i++) {
            destination.add(source.remove(source.size() - 1));// Take from the end of the arraylist.
        }
        return true;
    }

    /**
     * @return the {@link List} of modules whose name contains the given pattern.
     */
    public static List<String> getModuleNamesMatching(File directory, String pattern) {
        String[] names = directory.list(new NameFilter(pattern));
        List<String> modules = new ArrayList<String>(names.length);
        for (String name : names) {
            int index = name.indexOf(CONFIG_EXT);
            if (index > 0) {
                String module = name.substring(0, index);
                if (module.equals(pattern)) {
                    // Pattern represents a single module, just return a single-item list
                    modules = new ArrayList<>(1);
                    modules.add(module);
                    return modules;
                }
                modules.add(module);
            }
        }
        return modules;
    }

    private static void putArgs(List<String> args, Map<String, Map<String, String>> argsMap) {
        for (String arg : args) {
            String[] parts = arg.split(":");
            String target = parts[0];
            String key = parts[1];
            String value = parts[2];
            Map<String, String> map = argsMap.get(target);
            if (map == null) {
                map = new HashMap<>();
                argsMap.put(target, map);
            }
            map.put(key, value);
        }
    }

    private static class ExecutionOrderComparator implements Comparator<IModuleDef> {

        @Override
        public int compare(IModuleDef def1, IModuleDef def2) {
            int value1 = 0;
            int value2 = 0;
            if (ENDING_MODULES.containsKey(def1.getName())) {
                value1 = ENDING_MODULES.get(def1.getName());
            }
            if (ENDING_MODULES.containsKey(def2.getName())) {
                value2 = ENDING_MODULES.get(def2.getName());
            }
            if (value1 == 0 && value2 == 0) {
                return (int) Math.signum(def2.getRuntimeHint() - def1.getRuntimeHint());
            }
            return (int) Math.signum(value1 - value2);
        }
    }
}
