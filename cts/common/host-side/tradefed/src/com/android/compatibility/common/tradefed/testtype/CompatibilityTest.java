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

import com.android.compatibility.SuiteInfo;
import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.compatibility.common.tradefed.targetprep.NetworkConnectivityChecker;
import com.android.compatibility.common.tradefed.targetprep.SystemStatusChecker;
import com.android.compatibility.common.tradefed.util.OptionHelper;
import com.android.compatibility.common.util.AbiUtils;
import com.android.compatibility.common.util.ICaseResult;
import com.android.compatibility.common.util.IInvocationResult;
import com.android.compatibility.common.util.IModuleResult;
import com.android.compatibility.common.util.ITestResult;
import com.android.compatibility.common.util.ResultHandler;
import com.android.compatibility.common.util.TestFilter;
import com.android.compatibility.common.util.TestStatus;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.ArgsOptionParser;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationFactory;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.config.OptionCopier;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.DeviceUnresponsiveException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.ITestLogger;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IShardableTest;
import com.android.tradefed.util.AbiFormatter;
import com.android.tradefed.util.ArrayUtil;
import com.android.tradefed.util.TimeUtil;

import java.io.ByteArrayOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

/**
 * A Test for running Compatibility Suites
 */
@OptionClass(alias = "compatibility")
public class CompatibilityTest implements IDeviceTest, IShardableTest, IBuildReceiver {

    public static final String INCLUDE_FILTER_OPTION = "include-filter";
    public static final String EXCLUDE_FILTER_OPTION = "exclude-filter";
    private static final String PLAN_OPTION = "plan";
    private static final String MODULE_OPTION = "module";
    private static final String TEST_OPTION = "test";
    private static final String MODULE_ARG_OPTION = "module-arg";
    private static final String TEST_ARG_OPTION = "test-arg";
    public static final String RETRY_OPTION = "retry";
    private static final String ABI_OPTION = "abi";
    private static final String SHARD_OPTION = "shards";
    public static final String SKIP_DEVICE_INFO_OPTION = "skip-device-info";
    public static final String SKIP_PRECONDITIONS_OPTION = "skip-preconditions";
    public static final String DEVICE_TOKEN_OPTION = "device-token";
    private static final String URL = "dynamic-config-url";

    /* API Key for compatibility test project, used for dynamic configuration */
    private static final String API_KEY = "AIzaSyAbwX5JRlmsLeygY2WWihpIJPXFLueOQ3U";


    @Option(name = PLAN_OPTION,
            description = "the test suite plan to run, such as \"everything\" or \"cts\"",
            importance = Importance.ALWAYS)
    private String mSuitePlan;

    @Option(name = INCLUDE_FILTER_OPTION,
            description = "the include module filters to apply.",
            importance = Importance.ALWAYS)
    private List<String> mIncludeFilters = new ArrayList<>();

    @Option(name = EXCLUDE_FILTER_OPTION,
            description = "the exclude module filters to apply.",
            importance = Importance.ALWAYS)
    private List<String> mExcludeFilters = new ArrayList<>();

    @Option(name = MODULE_OPTION,
            shortName = 'm',
            description = "the test module to run.",
            importance = Importance.IF_UNSET)
    private String mModuleName = null;

    @Option(name = TEST_OPTION,
            shortName = 't',
            description = "the test run.",
            importance = Importance.IF_UNSET)
    private String mTestName = null;

    @Option(name = MODULE_ARG_OPTION,
            description = "the arguments to pass to a module. The expected format is"
                    + "\"<module-name>:<arg-name>:<arg-value>\"",
            importance = Importance.ALWAYS)
    private List<String> mModuleArgs = new ArrayList<>();

    @Option(name = TEST_ARG_OPTION,
            description = "the arguments to pass to a test. The expected format is"
                    + "\"<test-class>:<arg-name>:<arg-value>\"",
            importance = Importance.ALWAYS)
    private List<String> mTestArgs = new ArrayList<>();

    @Option(name = RETRY_OPTION,
            shortName = 'r',
            description = "retry a previous session.",
            importance = Importance.IF_UNSET)
    private Integer mRetrySessionId = null;

    @Option(name = ABI_OPTION,
            shortName = 'a',
            description = "the abi to test.",
            importance = Importance.IF_UNSET)
    private String mAbiName = null;

    @Option(name = SHARD_OPTION,
            description = "split the modules up to run on multiple devices concurrently.")
    private int mShards = 1;

    @Option(name = URL,
            description = "Specify the url for override config")
    private String mURL = "https://androidpartner.googleapis.com/v1/dynamicconfig/"
            + "suites/{suite-name}/modules/{module}/version/{version}?key=" + API_KEY;

    @Option(name = SKIP_DEVICE_INFO_OPTION,
            shortName = 'd',
            description = "Whether device info collection should be skipped")
    private boolean mSkipDeviceInfo = false;

    @Option(name = SKIP_PRECONDITIONS_OPTION,
            shortName = 'o',
            description = "Whether preconditions should be skipped")
    private boolean mSkipPreconditions = false;

    @Option(name = DEVICE_TOKEN_OPTION,
            description = "Holds the devices' tokens, used when scheduling tests that have"
                    + "prerequisits such as requiring a SIM card. Format is <serial>:<token>",
            importance = Importance.ALWAYS)
    private List<String> mDeviceTokens = new ArrayList<>();

    @Option(name = "bugreport-on-failure",
            description = "Take a bugreport on every test failure. " +
                    "Warning: can potentially use a lot of disk space.")
    private boolean mBugReportOnFailure = false;

    @Option(name = "logcat-on-failure",
            description = "Take a logcat snapshot on every test failure.")
    private boolean mLogcatOnFailure = false;

    @Option(name = "screenshot-on-failure",
            description = "Take a screenshot on every test failure.")
    private boolean mScreenshotOnFailure = false;

    @Option(name = "reboot-before-test",
            description = "Reboot the device before the test suite starts.")
    private boolean mRebootBeforeTest = false;

    @Option(name = "reboot-on-failure",
            description = "Reboot the device after every test failure.")
    private boolean mRebootOnFailure = false;

    @Option(name = "reboot-per-module",
            description = "Reboot the device before every module run.")
    private boolean mRebootPerModule = false;

    @Option(name = "skip-connectivity-check",
            description = "Don't verify device connectivity between module execution.")
    private boolean mSkipConnectivityCheck = false;

    @Option(name = "preparer-whitelist",
            description = "Only run specific preparers."
            + "Specify zero or more ITargetPreparers as canonical class names. "
            + "e.g. \"com.android.compatibility.common.tradefed.targetprep.ApkInstaller\" "
            + "If not specified, all configured preparers are run.")
    private Set<String> mPreparerWhitelist = new HashSet<>();

    @Option(name = "skip-all-system-status-check",
            description = "Whether all system status check between modules should be skipped")
    private boolean mSkipAllSystemStatusCheck = false;

    @Option(name = "skip-system-status-check",
            description = "Disable specific system status checkers."
            + "Specify zero or more SystemStatusChecker as canonical class names. e.g. "
            + "\"com.android.compatibility.common.tradefed.targetprep.NetworkConnectivityChecker\" "
            + "If not specified, all configured or whitelisted system status checkers are run.")
    private Set<String> mSystemStatusCheckBlacklist = new HashSet<>();

    @Option(name = "system-status-check-whitelist",
            description = "Only run specific system status checkers."
            + "Specify zero or more SystemStatusChecker as canonical class names. e.g. "
            + "\"com.android.compatibility.common.tradefed.targetprep.NetworkConnectivityChecker\" "
            + "If not specified, all configured system status checkers are run.")
    private Set<String> mSystemStatusCheckWhitelist = new HashSet<>();

    @Option(name = "system-status-checker-config", description = "Configuration file for system "
            + "status checkers invoked between module execution.")
    private String mSystemStatusCheckerConfig = "system-status-checkers";

    private int mTotalShards;
    private IModuleRepo mModuleRepo;
    private ITestDevice mDevice;
    private CompatibilityBuildHelper mBuildHelper;

    /**
     * Create a new {@link CompatibilityTest} that will run the default list of
     * modules.
     */
    public CompatibilityTest() {
        this(1 /* totalShards */, new ModuleRepo());
    }

    /**
     * Create a new {@link CompatibilityTest} that will run a sublist of
     * modules.
     */
    public CompatibilityTest(int totalShards, IModuleRepo moduleRepo) {
        if (totalShards < 1) {
            throw new IllegalArgumentException(
                    "Must be at least 1 shard. Given:" + totalShards);
        }
        mTotalShards = totalShards;
        mModuleRepo = moduleRepo;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public ITestDevice getDevice() {
        return mDevice;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        mDevice = device;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildHelper = new CompatibilityBuildHelper(buildInfo);
        // Initializing the mBuildHelper also updates properties in buildInfo.
        // TODO(nicksauer): Keeping invocation properties around via buildInfo
        // is confusing and would be better done in an "InvocationInfo".
        // Note, the current time is used to generated the result directory.
        mBuildHelper.init(mSuitePlan, mURL, System.currentTimeMillis());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        try {
            // Synchronized so only one shard enters and sets up the moduleRepo. When the other
            // shards enter after this, moduleRepo is already initialized so they dont do anything
            synchronized (mModuleRepo) {
                if (!mModuleRepo.isInitialized()) {
                    setupFilters();
                    // Initialize the repository, {@link CompatibilityBuildHelper#getTestsDir} can
                    // throw a {@link FileNotFoundException}
                    mModuleRepo.initialize(mTotalShards, mBuildHelper.getTestsDir(), getAbis(),
                            mDeviceTokens, mTestArgs, mModuleArgs, mIncludeFilters,
                            mExcludeFilters, mBuildHelper.getBuildInfo());

                    // Add the entire list of modules to the CompatibilityBuildHelper for reporting
                    mBuildHelper.setModuleIds(mModuleRepo.getModuleIds());
                }

            }
            // Get the tests to run in this shard
            List<IModuleDef> modules = mModuleRepo.getModules(getDevice().getSerialNumber());

            listener = new FailureListener(listener, getDevice(), mBugReportOnFailure,
                    mLogcatOnFailure, mScreenshotOnFailure, mRebootOnFailure);
            int moduleCount = modules.size();
            CLog.logAndDisplay(LogLevel.INFO, "Starting %d module%s on %s", moduleCount,
                    (moduleCount > 1) ? "s" : "", mDevice.getSerialNumber());
            if (mRebootBeforeTest) {
                CLog.d("Rebooting device before test starts as requested.");
                mDevice.reboot();
            }

            if (mSkipConnectivityCheck) {
                String clazz = NetworkConnectivityChecker.class.getCanonicalName();
                CLog.logAndDisplay(LogLevel.INFO, "\"--skip-connectivity-check\" is deprecated, "
                        + "please use \"--skip-system-status-check %s\" instead", clazz);
                mSystemStatusCheckBlacklist.add(clazz);
            }

            // Get system status checkers
            List<SystemStatusChecker> checkers = null;
            if (!mSkipAllSystemStatusCheck) {
                try {
                    checkers = initSystemStatusCheckers();
                } catch (ConfigurationException ce) {
                    throw new RuntimeException("failed to load system status checker config", ce);
                }
            }

            // Set values and run preconditions
            for (int i = 0; i < moduleCount; i++) {
                IModuleDef module = modules.get(i);
                module.setBuild(mBuildHelper.getBuildInfo());
                module.setDevice(mDevice);
                module.setPreparerWhitelist(mPreparerWhitelist);
                module.prepare(mSkipPreconditions);
            }
            // Run the tests
            for (int i = 0; i < moduleCount; i++) {
                IModuleDef module = modules.get(i);
                long start = System.currentTimeMillis();

                if (mRebootPerModule) {
                    if ("user".equals(mDevice.getProperty("ro.build.type"))) {
                        CLog.e("reboot-per-module should only be used during development, "
                            + "this is a\" user\" build device");
                    } else {
                        CLog.logAndDisplay(LogLevel.INFO, "Rebooting device before starting next "
                            + "module");
                        mDevice.reboot();
                    }
                }

                // execute pre module execution checker
                runPreModuleCheck(module.getName(), checkers, mDevice, listener);
                try {
                    module.run(listener);
                } catch (DeviceUnresponsiveException due) {
                    // being able to catch a DeviceUnresponsiveException here implies that recovery
                    // was successful, and test execution should proceed to next module
                    ByteArrayOutputStream stack = new ByteArrayOutputStream();
                    due.printStackTrace(new PrintWriter(stack, true));
                    try {
                        stack.close();
                    } catch (IOException ioe) {
                        // won't happen on BAOS
                    }
                    CLog.w("Ignored DeviceUnresponsiveException because recovery was successful, "
                            + "proceeding with next module. Stack trace: %s",
                            stack.toString());
                    CLog.w("This may be due to incorrect timeout setting on module %s",
                            module.getName());
                }
                long duration = System.currentTimeMillis() - start;
                long expected = module.getRuntimeHint();
                long delta = Math.abs(duration - expected);
                // Show warning if delta is more than 10% of expected
                if (expected > 0 && ((float)delta / (float)expected) > 0.1f) {
                    CLog.logAndDisplay(LogLevel.WARN,
                            "Inaccurate runtime hint for %s, expected %s was %s",
                            module.getId(),
                            TimeUtil.formatElapsedTime(expected),
                            TimeUtil.formatElapsedTime(duration));
                }
                runPostModuleCheck(module.getName(), checkers, mDevice, listener);
            }
        } catch (FileNotFoundException fnfe) {
            throw new RuntimeException("Failed to initialize modules", fnfe);
        }
    }

    /**
     * Gets the set of ABIs supported by both Compatibility and the device under test
     *
     * @return The set of ABIs to run the tests on
     * @throws DeviceNotAvailableException
     */
    Set<IAbi> getAbis() throws DeviceNotAvailableException {
        Set<IAbi> abis = new HashSet<>();
        Set<String> archAbis = AbiUtils.getAbisForArch(SuiteInfo.TARGET_ARCH);
        for (String abi : AbiFormatter.getSupportedAbis(mDevice, "")) {
            // Only test against ABIs supported by Compatibility, and if the
            // --abi option was given, it must match.
            if (AbiUtils.isAbiSupportedByCompatibility(abi) && archAbis.contains(abi)
                    && (mAbiName == null || mAbiName.equals(abi))) {
                abis.add(new Abi(abi, AbiUtils.getBitness(abi)));
            }
        }
        if (abis == null || abis.isEmpty()) {
            if (mAbiName == null) {
                throw new IllegalArgumentException("Could not get device's ABIs");
            } else {
                throw new IllegalArgumentException(String.format(
                        "Device %s doesn't support %s", mDevice.getSerialNumber(), mAbiName));
            }
        }
        return abis;
    }

    private List<SystemStatusChecker> initSystemStatusCheckers() throws ConfigurationException {
        IConfigurationFactory cf = ConfigurationFactory.getInstance();
        IConfiguration config = cf.createConfigurationFromArgs(
                new String[]{mSystemStatusCheckerConfig});
        // only checks the target preparers from the config
        List<ITargetPreparer> preparers = config.getTargetPreparers();
        List<SystemStatusChecker> checkers = new ArrayList<>();
        for (ITargetPreparer p : preparers) {
            if (p instanceof SystemStatusChecker) {
                SystemStatusChecker s = (SystemStatusChecker)p;
                if (shouldIncludeSystemStatusChecker(s)) {
                    checkers.add(s);
                } else {
                    CLog.i("%s skipped because it's not whitelisted.",
                            s.getClass().getCanonicalName());
                }
            } else {
                CLog.w("Preparer %s does not have type %s, ignored ",
                        p.getClass().getCanonicalName(),
                        SystemStatusChecker.class.getCanonicalName());
            }
        }
        return checkers;
    }

    /**
     * Resolve the inclusion and exclusion logic of system status checkers
     *
     * @param s the {@link SystemStatusChecker} to perform filtering logic on
     * @return
     */
    private boolean shouldIncludeSystemStatusChecker(SystemStatusChecker s) {
        String clazz = s.getClass().getCanonicalName();
        boolean shouldInclude = mSystemStatusCheckWhitelist.isEmpty()
                || mSystemStatusCheckWhitelist.contains(clazz);
        boolean shouldExclude = !mSystemStatusCheckBlacklist.isEmpty()
                && mSystemStatusCheckBlacklist.contains(clazz);
        return shouldInclude && !shouldExclude;
    }

    private void runPreModuleCheck(String moduleName, List<SystemStatusChecker> checkers,
            ITestDevice device, ITestLogger logger) throws DeviceNotAvailableException {
        CLog.i("Running system status checker before module execution: %s", moduleName);
        List<String> failures = new ArrayList<>();
        for (SystemStatusChecker checker : checkers) {
            boolean result = checker.preExecutionCheck(device);
            if (!result) {
                failures.add(checker.getClass().getCanonicalName());
                CLog.w("System status checker [%s] failed with message: %s",
                        checker.getClass().getCanonicalName(), checker.getFailureMessage());
            }
        }
        if (!failures.isEmpty()) {
            CLog.w("There are failed system status checkers: %s capturing a bugreport",
                    failures.toString());
            InputStreamSource bugSource = device.getBugreport();
            logger.testLog(String.format("bugreport-checker-pre-module-%s", moduleName),
                    LogDataType.TEXT, bugSource);
            bugSource.cancel();
        }
    }

    private void runPostModuleCheck(String moduleName, List<SystemStatusChecker> checkers,
            ITestDevice device, ITestLogger logger) throws DeviceNotAvailableException {
        CLog.i("Running system status checker after module execution: %s", moduleName);
        List<String> failures = new ArrayList<>();
        for (SystemStatusChecker checker : checkers) {
            boolean result = checker.postExecutionCheck(device);
            if (!result) {
                failures.add(checker.getClass().getCanonicalName());
                CLog.w("System status checker [%s] failed with message: %s",
                        checker.getClass().getCanonicalName(), checker.getFailureMessage());
            }
        }
        if (!failures.isEmpty()) {
            CLog.w("There are failed system status checkers: %s capturing a bugreport",
                    failures.toString());
            InputStreamSource bugSource = device.getBugreport();
            logger.testLog(String.format("bugreport-checker-post-module-%s", moduleName),
                    LogDataType.TEXT, bugSource);
            bugSource.cancel();
        }
    }

    /**
     * Sets the include/exclude filters up based on if a module name was given or whether this is a
     * retry run.
     */
    void setupFilters() throws DeviceNotAvailableException {
        if (mRetrySessionId != null) {
            // We're retrying so clear -m and -t options
            // eventually reset these options with values given in the previous session
            mModuleName = null;
            mTestName = null;
            // Load the invocation result
            IInvocationResult result = null;
            try {
                result = ResultHandler.findResult(mBuildHelper.getResultsDir(), mRetrySessionId);
            } catch (FileNotFoundException e) {
                throw new RuntimeException(e);
            }
            if (result == null) {
                throw new IllegalArgumentException(String.format(
                        "Could not find session with id %d", mRetrySessionId));
            }

            String oldBuildFingerprint = result.getBuildFingerprint();
            String currentBuildFingerprint = mDevice.getProperty("ro.build.fingerprint");
            if (oldBuildFingerprint.equals(currentBuildFingerprint)) {
                CLog.logAndDisplay(LogLevel.INFO, "Retrying session from: %s",
                        CompatibilityBuildHelper.getDirSuffix(result.getStartTime()));
            } else {
                throw new IllegalArgumentException(String.format(
                        "Device build fingerprint must match %s to retry session %d",
                        oldBuildFingerprint, mRetrySessionId));
            }

            String retryCommandLineArgs = result.getCommandLineArgs();
            if (retryCommandLineArgs != null) {
                // Copy the original command into the build helper so it can be serialized later
                mBuildHelper.setRetryCommandLineArgs(retryCommandLineArgs);
                try {
                    // parse the command-line string from the result file and set options
                    ArgsOptionParser parser = new ArgsOptionParser(this);
                    parser.parse(OptionHelper.getValidCliArgs(retryCommandLineArgs, this));
                } catch (ConfigurationException e) {
                    throw new RuntimeException(e);
                }
            }
            // Append each test that failed or was not executed to the filters
            for (IModuleResult module : result.getModules()) {
                for (ICaseResult testResultList : module.getResults()) {
                    for (ITestResult testResult : testResultList.getResults(TestStatus.PASS)) {
                        // Create the filter for the test to be run.
                        TestFilter filter = new TestFilter(
                                module.getAbi(), module.getName(), testResult.getFullName());
                        mExcludeFilters.add(filter.toString());
                    }
                }
            }
        }
        if (mModuleName != null) {
            mIncludeFilters.clear();
            try {
                List<String> modules = ModuleRepo.getModuleNamesMatching(
                        mBuildHelper.getTestsDir(), mModuleName);
                if (modules.size() == 0) {
                    throw new IllegalArgumentException(
                            String.format("No modules found matching %s", mModuleName));
                } else if (modules.size() > 1) {
                    throw new IllegalArgumentException(String.format(
                            "Multiple modules found matching %s:\n%s\nWhich one did you mean?\n",
                            mModuleName, ArrayUtil.join("\n", modules)));
                } else {
                    String module = modules.get(0);
                    mIncludeFilters.add(new TestFilter(mAbiName, module, mTestName).toString());
                    // We will run this module with previous exclusions,
                    // unless they exclude the whole module.
                    List<String> excludeFilters = new ArrayList<>();
                    for (String excludeFilter : mExcludeFilters) {
                        TestFilter filter = TestFilter.createFrom(excludeFilter);
                        String name = filter.getName();
                        // Add the filter if it applies to this module
                        if (module.equals(name)) {
                            excludeFilters.add(excludeFilter);
                        }
                    }
                    mExcludeFilters = excludeFilters;
                }
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            }
        } else if (mTestName != null) {
            throw new IllegalArgumentException(
                    "Test name given without module name. Add --module <module-name>");
        } else {
            // If a module has an arg, assume it's included
            for (String arg : mModuleArgs) {
                mIncludeFilters.add(arg.split(":")[0]);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Collection<IRemoteTest> split() {
        if (mShards <= 1) {
            return null;
        }

        List<IRemoteTest> shardQueue = new LinkedList<>();
        for (int i = 0; i < mShards; i++) {
            CompatibilityTest test = new CompatibilityTest(mShards, mModuleRepo);
            OptionCopier.copyOptionsNoThrow(this, test);
            // Set the shard count because the copy option on the previous line
            // copies over the mShard value
            test.mShards = 0;
            shardQueue.add(test);
        }

        return shardQueue;
    }

}
