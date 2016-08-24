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

import com.android.compatibility.common.tradefed.result.IModuleListener;
import com.android.compatibility.common.tradefed.result.ModuleListener;
import com.android.compatibility.common.tradefed.targetprep.PreconditionPreparer;
import com.android.compatibility.common.tradefed.targetprep.TokenRequirement;
import com.android.compatibility.common.util.AbiUtils;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.ITargetCleaner;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.IRuntimeHintProvider;
import com.android.tradefed.testtype.ITestCollector;
import com.android.tradefed.testtype.ITestFilterReceiver;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * Container for Compatibility test module info.
 */
public class ModuleDef implements IModuleDef {

    private final String mId;
    private final String mName;
    private final IAbi mAbi;
    private final Set<String> mTokens = new HashSet<>();
    private IRemoteTest mTest = null;
    private List<ITargetPreparer> mPreconditions = new ArrayList<>();
    private List<ITargetPreparer> mPreparers = new ArrayList<>();
    private List<ITargetCleaner> mCleaners = new ArrayList<>();
    private IBuildInfo mBuild;
    private ITestDevice mDevice;
    private Set<String> mPreparerWhitelist = new HashSet<>();

    public ModuleDef(String name, IAbi abi, IRemoteTest test,
            List<ITargetPreparer> preparers) {
        mId = AbiUtils.createId(abi.getName(), name);
        mName = name;
        mAbi = abi;
        mTest = test;
        boolean hasAbiReceiver = false;
        for (ITargetPreparer preparer : preparers) {
            if (preparer instanceof IAbiReceiver) {
                hasAbiReceiver = true;
            }
            // Separate preconditions from target preparers.
            if (preparer instanceof PreconditionPreparer) {
                mPreconditions.add(preparer);
            } else if (preparer instanceof TokenRequirement) {
                mTokens.addAll(((TokenRequirement) preparer).getTokens());
            } else {
                mPreparers.add(preparer);
            }
            if (preparer instanceof ITargetCleaner) {
                mCleaners.add((ITargetCleaner) preparer);
            }
        }
        // Reverse cleaner order
        Collections.reverse(mCleaners);

        // Required interfaces:
        if (!hasAbiReceiver && !(test instanceof IAbiReceiver)) {
            throw new IllegalArgumentException(test
                    + "does not implement IAbiReceiver"
                    + " - for multi-abi testing (64bit)");
        } else if (!(test instanceof IRuntimeHintProvider)) {
            throw new IllegalArgumentException(test
                    + " does not implement IRuntimeHintProvider"
                    + " - to provide estimates of test invocation time");
        } else if (!(test instanceof ITestCollector)) {
            throw new IllegalArgumentException(test
                    + " does not implement ITestCollector"
                    + " - for test list collection");
        } else if (!(test instanceof ITestFilterReceiver)) {
            throw new IllegalArgumentException(test
                    + " does not implement ITestFilterReceiver"
                    + " - to allow tests to be filtered");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String toString() {
        return mId;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getId() {
        return mId;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getName() {
        return mName;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IAbi getAbi() {
        return mAbi;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Set<String> getTokens() {
        return mTokens;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public long getRuntimeHint() {
        if (mTest instanceof IRuntimeHintProvider) {
            return ((IRuntimeHintProvider) mTest).getRuntimeHint();
        }
        return TimeUnit.MINUTES.toMillis(1); // Default 1 minute.
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IRemoteTest getTest() {
        return mTest;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setPreparerWhitelist(Set<String> preparerWhitelist) {
        mPreparerWhitelist.addAll(preparerWhitelist);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int compareTo(IModuleDef moduleDef) {
        return getName().compareTo(moduleDef.getName());
    }

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
    public void run(ITestInvocationListener listener) throws DeviceNotAvailableException {
        IModuleListener moduleListener = new ModuleListener(this, listener);

        // Setup
        for (ITargetPreparer preparer : mPreparers) {
            String preparerName = preparer.getClass().getCanonicalName();
            if (!mPreparerWhitelist.isEmpty() && !mPreparerWhitelist.contains(preparerName)) {
                CLog.w("Skipping Preparer: %s since it is not in the whitelist %s",
                        preparerName, mPreparerWhitelist);
                continue;
            }
            CLog.d("Preparer: %s", preparer.getClass().getSimpleName());
            if (preparer instanceof IAbiReceiver) {
                ((IAbiReceiver) preparer).setAbi(mAbi);
            }
            try {
                preparer.setUp(mDevice, mBuild);
            } catch (BuildError e) {
                // This should only happen for flashing new build
                CLog.e("Unexpected BuildError from precondition: %s",
                        preparer.getClass().getCanonicalName());
            } catch (TargetSetupError e) {
                // log precondition class then rethrow & let caller handle
                CLog.e("TargetSetupError in precondition: %s",
                        preparer.getClass().getCanonicalName());
                throw new RuntimeException(e);
            }
        }


        CLog.d("Test: %s", mTest.getClass().getSimpleName());
        if (mTest instanceof IAbiReceiver) {
            ((IAbiReceiver) mTest).setAbi(mAbi);
        }
        if (mTest instanceof IBuildReceiver) {
            ((IBuildReceiver) mTest).setBuild(mBuild);
        }
        if (mTest instanceof IDeviceTest) {
            ((IDeviceTest) mTest).setDevice(mDevice);
        }

        mTest.run(moduleListener);

        // Tear down
        for (ITargetCleaner cleaner : mCleaners) {
            CLog.d("Cleaner: %s", cleaner.getClass().getSimpleName());
            cleaner.tearDown(mDevice, mBuild, null);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void prepare(boolean skipPrep) throws DeviceNotAvailableException {
        for (ITargetPreparer preparer : mPreconditions) {
            CLog.d("Preparer: %s", preparer.getClass().getSimpleName());
            if (preparer instanceof IAbiReceiver) {
                ((IAbiReceiver) preparer).setAbi(mAbi);
            }
            setOption(preparer, CompatibilityTest.SKIP_PRECONDITIONS_OPTION,
                    Boolean.toString(skipPrep));
            try {
                preparer.setUp(mDevice, mBuild);
            } catch (BuildError e) {
                // This should only happen for flashing new build
                CLog.e("Unexpected BuildError from precondition: %s",
                        preparer.getClass().getCanonicalName());
            } catch (TargetSetupError e) {
                // log precondition class then rethrow & let caller handle
                CLog.e("TargetSetupError in precondition: %s",
                        preparer.getClass().getCanonicalName());
                throw new RuntimeException(e);
            }
        }
    }

    private void setOption(Object target, String option, String value) {
        try {
            OptionSetter setter = new OptionSetter(target);
            setter.setOptionValue(option, value);
        } catch (ConfigurationException e) {
            e.printStackTrace();
        }
    }
}
