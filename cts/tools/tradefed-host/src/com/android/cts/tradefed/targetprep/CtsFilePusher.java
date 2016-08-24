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
package com.android.cts.tradefed.targetprep;

import com.android.cts.tradefed.build.CtsBuildHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.targetprep.PushFilePreparer;

import java.io.File;

/**
 * Pushes specified testing artifacts from CTS test case repository
 */
@OptionClass(alias="cts-file-pusher")
public class CtsFilePusher extends PushFilePreparer {

    private CtsBuildHelper mBuildHelper;

    protected CtsBuildHelper getCtsBuildHelper(IBuildInfo buildInfo) {
        if (mBuildHelper == null) {
            mBuildHelper = CtsBuildHelper.createBuildHelper(buildInfo);
        }
        return mBuildHelper;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public File resolveRelativeFilePath(IBuildInfo buildInfo, String fileName) {
        return new File(getCtsBuildHelper(buildInfo).getTestCasesDir(), fileName);
    }
}
