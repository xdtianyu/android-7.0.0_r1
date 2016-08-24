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
package com.android.compatibility.common.util;

import java.util.List;

/**
 * Data structure for a Compatibility test module result.
 */
public interface IModuleResult extends Comparable<IModuleResult> {

    String getId();

    String getName();

    String getAbi();

    void addRuntime(long elapsedTime);

    long getRuntime();

    boolean isDone();

    void setDone(boolean done);

    /**
     * Gets a {@link ICaseResult} for the given testcase, creating it if it doesn't exist.
     *
     * @param caseName the name of the testcase eg &lt;package-name&gt;&lt;class-name&gt;
     * @return the {@link ICaseResult} or <code>null</code>
     */
    ICaseResult getOrCreateResult(String caseName);

    /**
     * Gets the {@link ICaseResult} result for given testcase.
     *
     * @param caseName the name of the testcase eg &lt;package-name&gt;&lt;class-name&gt;
     * @return the {@link ITestResult} or <code>null</code>
     */
    ICaseResult getResult(String caseName);

    /**
     * Gets all results sorted by name.
     */
    List<ICaseResult> getResults();

    /**
     * Counts the number of results which have the given status.
     */
    int countResults(TestStatus status);

    /**
     * Merge the module results from otherModuleResult into this moduleResult.
     */
    void mergeFrom(IModuleResult otherModuleResult);
}
