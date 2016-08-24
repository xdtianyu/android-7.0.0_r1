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
import android.uirendering.cts.ScriptC_MeanSquaredComparer;

import android.content.res.Resources;
import android.graphics.Color;
import android.renderscript.Allocation;
import android.renderscript.RenderScript;
import android.util.Log;

/**
 * Finds the MSE using two images.
 */
public class MeanSquaredComparer extends BaseRenderScriptComparer {
    private static final String TAG = "MeanSquared";
    private ScriptC_MeanSquaredComparer mScript;
    private float mErrorPerPixel;

    /**
     * @param errorPerPixel threshold for which the test will pass/fail. This is the mean-squared
     *                      error averaged across all of those before comparing.
     */
    public MeanSquaredComparer(float errorPerPixel) {
        mErrorPerPixel = errorPerPixel;
    }

    @Override
    public boolean verifySame(int[] ideal, int[] given, int offset, int stride, int width,
            int height) {
        float totalError = getMSE(ideal, given, offset, stride, width, height);
        Log.d(TAG, "Error : " + totalError);
        return (totalError < (mErrorPerPixel));
    }

    @Override
    public boolean verifySameRowsRS(Resources resources, Allocation ideal,
            Allocation given, int offset, int stride, int width, int height,
            RenderScript renderScript, Allocation inputAllocation, Allocation outputAllocation) {
        if (mScript == null) {
            mScript = new ScriptC_MeanSquaredComparer(renderScript);
        }
        mScript.set_WIDTH(width);

        //Set the bitmap allocations
        mScript.set_ideal(ideal);
        mScript.set_given(given);

        //Call the renderscript function on each row
        mScript.forEach_calcMSE(inputAllocation, outputAllocation);

        float error = sum1DFloatAllocation(outputAllocation);
        error /= (height * width);

        Log.d(TAG, "Error RS : " + error);

        return (error < mErrorPerPixel);
    }

    /**
     * Gets the Mean Squared Error between two data sets.
     */
    public static float getMSE(int[] ideal, int[] given, int offset, int stride, int width,
            int height) {
        float totalError = 0;

        for (int y = 0 ; y < height ; y++) {
            for (int x = 0 ; x < width ; x++) {
                int index = indexFromXAndY(x, y, stride, offset);
                float idealSum = getColorSum(ideal[index]);
                float givenSum = getColorSum(given[index]);
                float difference = idealSum - givenSum;
                totalError += (difference * difference);
            }
        }

        totalError /= (width * height);
        return totalError;
    }

    private static float getColorSum(int color) {
        float red = Color.red(color) / 255.0f;
        float green = Color.green(color) / 255.0f;
        float blue = Color.blue(color) / 255.0f;
        return (red + green + blue);
    }
}
