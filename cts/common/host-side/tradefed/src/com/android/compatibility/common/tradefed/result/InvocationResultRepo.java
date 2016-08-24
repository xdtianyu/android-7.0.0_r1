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
import com.android.compatibility.common.util.ResultHandler;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * An implementation of {@link IInvocationResultRepo}.
 */
public class InvocationResultRepo implements IInvocationResultRepo {

    /**
     * Ordered list of result directories. the index of each file is its session id.
     */
    private List<IInvocationResult> mResults;

    /**
     * Create a {@link InvocationResultRepo} from a directory of results
     *
     * @param testResultDir the parent directory of results
     */
    public InvocationResultRepo(File testResultDir) {
        mResults = ResultHandler.getResults(testResultDir);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public List<IInvocationResult> getResults() {
        return new ArrayList<IInvocationResult>(mResults);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IInvocationResult getResult(int sessionId) {
        if (sessionId < 0 || sessionId >= mResults.size()) {
            return null;
        }
        return mResults.get(sessionId);
    }

}
