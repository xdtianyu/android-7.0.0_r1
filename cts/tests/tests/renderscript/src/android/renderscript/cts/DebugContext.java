/*
 * Copyright (C) 2013 The Android Open Source Project
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

package android.renderscript.cts;

import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.RenderScript;
import android.renderscript.RenderScript.RSErrorHandler;
import android.renderscript.RSRuntimeException;
import android.renderscript.Type;
import android.util.Log;

public class DebugContext extends RSBaseCompute {
    Allocation AUnused;
    Allocation AInt;
    ScriptC_oob Soob;

    boolean mRanErrorHandler = false;
    RSErrorHandler mRsError = new RSErrorHandler() {
        public void run() {
            mRanErrorHandler = true;
            Log.e("RenderscriptCTS", mErrorMessage);
        }
    };

    protected void setupDebugContext() {
        mRS.destroy();
        mRS = RenderScript.create(mCtx, RenderScript.ContextType.DEBUG);
        mRS.setMessageHandler(mRsMessage);
        mRS.setErrorHandler(mRsError);

        Soob = new ScriptC_oob(mRS);

        Type.Builder typeBuilder = new Type.Builder(mRS, Element.I32(mRS));
        Type t = typeBuilder.setX(1).create();

        AInt = Allocation.createTyped(mRS, t);
        Soob.set_aInt(AInt);

        AUnused = Allocation.createTyped(mRS, t);
    }

    /**
     * Test whether we are detect out-of-bounds allocation accesses
     * from an invokable.
     */
    public void testDebugContextI() {
        setupDebugContext();
        Soob.invoke_write_i(7, 1);  // Write to invalid location.

        // Flush messages through the pipeline.
        mRS.sendMessage(RS_MSG_TEST_FLUSH, null);
        waitForMessage();

        Soob.destroy();
        assertTrue(mRanErrorHandler);

        // The context is dead at this point so make sure it's not reused
        RenderScript.releaseAllContexts();
    }

    /**
     * Test whether we are detect out-of-bounds allocation accesses
     * from a kernel.
     */
    public void testDebugContextK() {
        setupDebugContext();
        Soob.forEach_write_k(AUnused);  // Write to invalid location.

        // Flush messages through the pipeline.
        mRS.sendMessage(RS_MSG_TEST_FLUSH, null);
        waitForMessage();

        Soob.destroy();
        assertTrue(mRanErrorHandler);

        // The context is dead at this point so make sure it's not reused
        RenderScript.releaseAllContexts();
    }
}
