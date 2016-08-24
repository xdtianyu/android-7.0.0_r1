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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.testtype.IAbi;

import java.io.File;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Interface for accessing tests from the Compatibility repository.
 */
public interface IModuleRepo {

    /**
     * @return true if this repository has been initialized.
     */
    boolean isInitialized();

    /**
     * Initializes the repository.
     */
    void initialize(int shards, File testsDir, Set<IAbi> abis, List<String> deviceTokens,
            List<String> testArgs, List<String> moduleArgs, List<String> mIncludeFilters,
            List<String> mExcludeFilters, IBuildInfo buildInfo);

    /**
     * @return a {@link Map} of all modules to run on the device referenced by the given serial.
     */
    List<IModuleDef> getModules(String serial);

    /**
     * @return the number of shards this repo is initialized for.
     */
    int getNumberOfShards();

    /**
     * @return the maximum number of modules a shard will run.
     */
    int getModulesPerShard();

    /**
     * @return the {@link Map} of device serials to tokens.
     */
    Map<String, Set<String>> getDeviceTokens();

    /**
     * @return the {@link Set} of device serials that have taken their workload.
     */
    Set<String> getSerials();

    /**
     * @return the small modules that don't have tokens but have not been assigned to a device.
     */
    List<IModuleDef> getSmallModules();

    /**
     * @return the medium modules that don't have tokens but have not been assigned to a device.
     */
    List<IModuleDef> getMediumModules();

    /**
     * @return the large modules that don't have tokens but have not been assigned to a device.
     */
    List<IModuleDef> getLargeModules();

    /**
     * @return the modules which have token and have not been assigned to a device.
     */
    List<IModuleDef> getTokenModules();

    /**
     * @return An array of all module ids in the repo.
     */
    String[] getModuleIds();
}
