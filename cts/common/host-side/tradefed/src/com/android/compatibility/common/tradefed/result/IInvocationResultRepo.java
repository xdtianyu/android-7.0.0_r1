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
package com.android.compatibility.common.tradefed.result;

import com.android.compatibility.common.util.IInvocationResult;
import com.android.compatibility.common.util.InvocationResult;

import java.util.List;

/**
 * Repository for Compatibility results.
 */
public interface IInvocationResultRepo {

    /**
     * @return the list of {@link IInvocationResult}s. The index is its session id
     */
    List<IInvocationResult> getResults();

    /**
     * Get the {@link IInvocationResult} for given session id.
     *
     * @param sessionId the session id
     * @return the {@link InvocationResult} or <code>null</null> if the result with that session id
     * cannot be retrieved
     */
    IInvocationResult getResult(int sessionId);

}
