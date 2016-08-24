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

package com.android.rs.test;

import android.content.Context;
import android.content.res.Resources;
import android.renderscript.*;

public class UT_single_source_ref_count extends UnitTest {
    private Resources mRes;

    protected UT_single_source_ref_count(RSTestCore rstc, Resources res, Context ctx) {
        super(rstc, "SingleSourceRefCount", ctx);
        mRes = res;
    }

    public void run() {
        RenderScript pRS = RenderScript.create(mCtx);
        ScriptC_single_source_ref_count s = new ScriptC_single_source_ref_count(pRS);
        pRS.setMessageHandler(mRsMessage);

        s.invoke_entrypoint();

        pRS.finish();
        waitForMessage();
        pRS.destroy();
    }
}
