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
package com.android.compatibility.common.tradefed.command;

import com.android.compatibility.SuiteInfo;
import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.compatibility.common.tradefed.build.CompatibilityBuildProvider;
import com.android.compatibility.common.tradefed.result.IInvocationResultRepo;
import com.android.compatibility.common.tradefed.result.InvocationResultRepo;
import com.android.compatibility.common.tradefed.testtype.ModuleRepo;
import com.android.compatibility.common.util.IInvocationResult;
import com.android.compatibility.common.util.TestStatus;
import com.android.tradefed.command.Console;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationFactory;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IRuntimeHintProvider;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.Pair;
import com.android.tradefed.util.RegexTrie;
import com.android.tradefed.util.TableFormatter;
import com.android.tradefed.util.TimeUtil;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * An extension of Tradefed's console which adds features specific to compatibility testing.
 */
public class CompatibilityConsole extends Console {

    /**
     * Hard coded list of modules to be excluded from manual module sharding
     * @see #splitModules(int)
     */
    private final static Set<String> MODULE_SPLIT_EXCLUSIONS = new HashSet<>();
    static {
        MODULE_SPLIT_EXCLUSIONS.add("CtsDeqpTestCases");
    }
    private CompatibilityBuildHelper mBuildHelper;

    /**
     * {@inheritDoc}
     */
    @Override
    public void run() {
        printLine(String.format("Android %s %s (%s)", SuiteInfo.FULLNAME, SuiteInfo.VERSION,
                SuiteInfo.BUILD_NUMBER));
        super.run();
    }

    /**
     * Adds the 'list plans', 'list modules' and 'list results' commands
     */
    @Override
    protected void setCustomCommands(RegexTrie<Runnable> trie, List<String> genericHelp,
            Map<String, String> commandHelp) {
        trie.put(new Runnable() {
            @Override
            public void run() {
                listPlans();
            }
        }, LIST_PATTERN, "p(?:lans)?");
        trie.put(new Runnable() {
            @Override
            public void run() {
                listModules();
            }
        }, LIST_PATTERN, "m(?:odules)?");
        trie.put(new Runnable() {
            @Override
            public void run() {
                listResults();
            }
        }, LIST_PATTERN, "r(?:esults)?");
        trie.put(new ArgRunnable<CaptureList>() {
            @Override
            public void run(CaptureList args) {
                // Skip 2 tokens to get past split and modules pattern
                String arg = args.get(2).get(0);
                int shards = Integer.parseInt(arg);
                if (shards <= 1) {
                    printLine("number of shards should be more than 1");
                    return;
                }
                splitModules(shards);
            }
        }, "split", "m(?:odules)?", "(\\d+)");

        // find existing help for 'LIST_PATTERN' commands, and append these commands help
        String listHelp = commandHelp.get(LIST_PATTERN);
        if (listHelp == null) {
            // no help? Unexpected, but soldier on
            listHelp = new String();
        }
        String combinedHelp = listHelp +
                "\tp[lans]\tList all plans" + LINE_SEPARATOR +
                "\tm[odules]\tList all modules" + LINE_SEPARATOR +
                "\tr[esults]\tList all results" + LINE_SEPARATOR;
        commandHelp.put(LIST_PATTERN, combinedHelp);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected String getConsolePrompt() {
        return String.format("%s-tf > ", SuiteInfo.NAME.toLowerCase());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected String getGenericHelpString(List<String> genericHelp) {
        StringBuilder helpBuilder = new StringBuilder();
        helpBuilder.append(SuiteInfo.FULLNAME);
        helpBuilder.append("\n\n");
        helpBuilder.append(SuiteInfo.NAME);
        helpBuilder.append(" is the test harness for running the Android Compatibility Suite, ");
        helpBuilder.append("built on top of Trade Federation.\n\n");
        helpBuilder.append("Available commands and options\n");
        helpBuilder.append("Host:\n");
        helpBuilder.append("  help: show this message.\n");
        helpBuilder.append("  help all: show the complete tradefed help.\n");
        helpBuilder.append("  version: show the version.\n");
        helpBuilder.append("  exit: gracefully exit the compatibiltiy console, waiting until all ");
        helpBuilder.append("invocations have completed.\n");
        helpBuilder.append("Run:\n");
        final String runPrompt = "  run <plan> ";
        helpBuilder.append(runPrompt);
        helpBuilder.append("--module/-m <module>: run a test module.\n");
        helpBuilder.append(runPrompt);
        helpBuilder.append("--module/-m <module> --test/-t <test_name>: run a specific test from");
        helpBuilder.append(" the module. Test name can be <package>.<class>, ");
        helpBuilder.append("<package>.<class>#<method> or <native_name>.\n");
        helpBuilder.append(runPrompt);
        helpBuilder.append("--retry <session_id>: run all failed tests from a previous session.\n");
        helpBuilder.append(runPrompt);
        helpBuilder.append("--help/--help-all: get help for ");
        helpBuilder.append(SuiteInfo.FULLNAME);
        helpBuilder.append(".\n");
        helpBuilder.append("Options:\n");
        helpBuilder.append("  --serial/-s <device_id>: The device to run the test on.\n");
        helpBuilder.append("  --abi/-a <abi>: The ABI to run the test against.\n");
        helpBuilder.append("  --shards <shards>: Shards a run into the given number of independant");
        helpBuilder.append(" chunks, to run on multiple devices in parallel.\n");
        helpBuilder.append("  --logcat-on-failure: Capture logcat when a test fails.\n");
        helpBuilder.append("  --bugreport-on-failure: Capture a bugreport when a test fails.\n");
        helpBuilder.append("  --screenshot-on-failure: Capture a screenshot when a test fails.\n");
        helpBuilder.append("List:\n");
        helpBuilder.append("  l/list d/devices: list connected devices and their state\n");
        helpBuilder.append("  l/list m/modules: list test modules\n");
        helpBuilder.append("  l/list i/invocations: list invocations aka test runs currently in ");
        helpBuilder.append("progress\n");
        helpBuilder.append("  l/list c/commands: list commands aka test run commands currently");
        helpBuilder.append(" in the queue waiting to be allocated devices\n");
        helpBuilder.append("  l/list r/results: list results currently in the repository\n");
        helpBuilder.append("Dump:\n");
        helpBuilder.append("  d/dump l/logs: dump the tradefed logs for all running invocations\n");
        return helpBuilder.toString();
    }

    private void listModules() {
        File[] files = null;
        try {
            files = getBuildHelper().getTestsDir().listFiles(new ModuleRepo.ConfigFilter());
        } catch (FileNotFoundException e) {
            printLine(e.getMessage());
            e.printStackTrace();
        }
        if (files != null && files.length > 0) {
            List<String> modules = new ArrayList<>();
            for (File moduleFile : files) {
                modules.add(FileUtil.getBaseName(moduleFile.getName()));
            }
            Collections.sort(modules);
            for (String module : modules) {
                printLine(module);
            }
        } else {
            printLine("No modules found");
        }
    }

    private void listPlans() {
        printLine("Available plans include:");
        ConfigurationFactory.getInstance().printHelp(System.out);
    }

    private void splitModules(int shards) {
        File[] files = null;
        try {
            files = getBuildHelper().getTestsDir().listFiles(new ModuleRepo.ConfigFilter());
        } catch (FileNotFoundException e) {
            printLine(e.getMessage());
            e.printStackTrace();
        }
        // parse through all config files to get runtime hints
        if (files != null && files.length > 0) {
            IConfigurationFactory configFactory = ConfigurationFactory.getInstance();
            List<Pair<String, Long>> moduleRuntime = new ArrayList<>();
            // parse through all config files to calculate module execution time
            for (File file : files) {
                IConfiguration config = null;
                String moduleName = file.getName().split("\\.")[0];
                if (MODULE_SPLIT_EXCLUSIONS.contains(moduleName)) {
                    continue;
                }
                try {
                    config = configFactory.createConfigurationFromArgs(new String[]{
                            file.getAbsolutePath(),
                    });
                } catch (ConfigurationException ce) {
                    printLine("Error loading config file: " + file.getAbsolutePath());
                    CLog.e(ce);
                    continue;
                }
                long runtime = 0;
                for (IRemoteTest test : config.getTests()) {
                    if (test instanceof IRuntimeHintProvider) {
                        runtime += ((IRuntimeHintProvider) test).getRuntimeHint();
                    } else {
                        CLog.w("Using default 1m runtime estimation for test type %s",
                                test.getClass().getSimpleName());
                        runtime += 60 * 1000;
                    }
                }
                moduleRuntime.add(new Pair<String, Long>(moduleName, runtime));
            }
            // sort list modules in descending order of runtime hint
            Collections.sort(moduleRuntime, new Comparator<Pair<String, Long>>() {
                @Override
                public int compare(Pair<String, Long> o1, Pair<String, Long> o2) {
                    return o2.second.compareTo(o1.second);
                }
            });
            // partition list of modules based on the runtime hint
            List<List<Pair<String, Long>>> splittedModules = new ArrayList<>();
            for (int i = 0; i < shards; i++) {
                splittedModules.add(new ArrayList<>());
            }
            int shardIndex = 0;
            int increment = 1;
            long[] shardTimes = new long[shards];
            // go through the sorted list, distribute modules into shards in zig-zag pattern to get
            // an even execution time among shards
            for (Pair<String, Long> module : moduleRuntime) {
                splittedModules.get(shardIndex).add(module);
                // also collect total runtime per shard
                shardTimes[shardIndex] += module.second;
                shardIndex += increment;
                // zig-zagging: first distribute modules from shard 0 to N, then N down to 0, repeat
                if (shardIndex == shards) {
                    increment = -1;
                    shardIndex = shards - 1;
                }
                if (shardIndex == -1) {
                    increment = 1;
                    shardIndex = 0;
                }
            }
            shardIndex = 0;
            // print the final shared lists
            for (List<Pair<String, Long>> shardedModules : splittedModules) {
                StringBuilder lineBuffer = new StringBuilder();
                lineBuffer.append(String.format("shard #%d (%s):",
                        shardIndex, TimeUtil.formatElapsedTime(shardTimes[shardIndex])));
                Iterator<Pair<String, Long>> itr = shardedModules.iterator();
                lineBuffer.append(itr.next().first);
                while (itr.hasNext()) {
                    lineBuffer.append(',');
                    lineBuffer.append(itr.next().first);
                }
                shardIndex++;
                printLine(lineBuffer.toString());
            }
        } else {
            printLine("No modules found");
        }
    }

    private void listResults() {
        TableFormatter tableFormatter = new TableFormatter();
        List<List<String>> table = new ArrayList<>();
        IInvocationResultRepo testResultRepo = null;
        List<IInvocationResult> results = null;
        try {
            testResultRepo = new InvocationResultRepo(getBuildHelper().getResultsDir());
            results = testResultRepo.getResults();
        } catch (FileNotFoundException e) {
            printLine(e.getMessage());
            e.printStackTrace();
        }
        if (testResultRepo != null && results.size() > 0) {
            for (int i = 0; i < results.size(); i++) {
                IInvocationResult result = results.get(i);
                Map<String, String> invocationInfo = result.getInvocationInfo();

                // invocation attributes are not always present (e.g. in the case of halted runs)
                // replace null entries with the string "Unknown"
                for (Map.Entry<String, String> entry : invocationInfo.entrySet()) {
                    if (entry.getValue() == null) {
                        invocationInfo.put(entry.getKey(), "Unknown");
                    }
                }

                String moduleProgress = String.format("%d of %d",
                        result.getModuleCompleteCount(), result.getModules().size());

                table.add(Arrays.asList(
                        Integer.toString(i),
                        Integer.toString(result.countResults(TestStatus.PASS)),
                        Integer.toString(result.countResults(TestStatus.FAIL)),
                        moduleProgress,
                        CompatibilityBuildHelper.getDirSuffix(result.getStartTime()),
                        result.getTestPlan(),
                        ArrayUtil.join(", ", result.getDeviceSerials()),
                        invocationInfo.get("build_id"),
                        invocationInfo.get("build_product")
                        ));
            }


            // add the table header to the beginning of the list
            table.add(0, Arrays.asList("Session", "Pass", "Fail", "Modules Complete", "Result Directory",
                    "Test Plan", "Device serial(s)", "Build ID", "Product"));
            tableFormatter.displayTable(table, new PrintWriter(System.out, true));
        } else {
            printLine(String.format("No results found"));
        }
    }

    private CompatibilityBuildHelper getBuildHelper() {
        if (mBuildHelper == null) {
            CompatibilityBuildProvider buildProvider = new CompatibilityBuildProvider();
            mBuildHelper = new CompatibilityBuildHelper(buildProvider.getBuild());
            mBuildHelper.init(
                "" /* suite plan */, "" /* dynamic config url */, -1 /*startTimeMs*/);
        }
        return mBuildHelper;
    }

    public static void main(String[] args) throws InterruptedException, ConfigurationException {
        Console console = new CompatibilityConsole();
        Console.startConsole(console, args);
    }
}
