/*
 * Copyright (C) 2014 The Android Open Source Project
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
package android.uirendering.cts.bitmapcomparers;

import android.uirendering.cts.R;
import android.uirendering.cts.ScriptC_ExactComparer;

import android.content.res.Resources;
import android.renderscript.Allocation;
import android.renderscript.RenderScript;
import android.util.Log;

/**
 * This class does an exact comparison of the pixels in a bitmap.
 */
public class ExactComparer extends BaseRenderScriptComparer {
    private static final String TAG = "ExactComparer";
    private ScriptC_ExactComparer mScript;

    /**
     * This method does an exact 1 to 1 comparison of the two bitmaps
     */
    public boolean verifySame(int[] ideal, int[] given, int offset, int stride, int width,
            int height) {
        int count = 0;

        for (int y = 0 ; y < height ; y++) {
            for (int x = 0 ; x < width ; x++) {
                int index = indexFromXAndY(x, y, stride, offset);
                if (ideal[index] != given[index]) {
                    if (count < 50) {
                        Log.d(TAG, "Failure on position x = " + x + " y = " + y);
                        Log.d(TAG, "Expected color : " + Integer.toHexString(ideal[index]) +
                                " given color : " + Integer.toHexString(given[index]));
                    }
                    count++;
                }
            }
        }
        Log.d(TAG, "Number of different pixels : " + count);

        return (count == 0);
    }

    @Override
    public boolean verifySameRowsRS(Resources resources, Allocation ideal,
            Allocation given, int offset, int stride, int width, int height,
            RenderScript renderScript, Allocation inputAllocation, Allocation outputAllocation) {
        if (mScript == null) {
            mScript = new ScriptC_ExactComparer(renderScript);
        }
        mScript.set_WIDTH(width);
        mScript.set_OFFSET(offset);

        //Set the bitmap allocations
        mScript.set_ideal(ideal);
        mScript.set_given(given);

        //Call the renderscript function on each row
        mScript.forEach_exactCompare(inputAllocation, outputAllocation);

        float val = sum1DFloatAllocation(outputAllocation);
        Log.d(TAG, "Number of different pixels RS : " + val);

        return val == 0;
    }
}
