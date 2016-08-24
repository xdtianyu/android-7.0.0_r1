package android.uirendering.cts.bitmapcomparers;

import android.content.res.Resources;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.RenderScript;

/**
 * Base class for calculators that want to implement renderscript
 */
public abstract class BaseRenderScriptComparer extends BitmapComparer {
    private Allocation mRowInputs;
    private Allocation mRowOutputs;
    private int mHeight;

    public abstract boolean verifySame(int[] ideal, int[] given, int offset, int stride, int width,
            int height);

    /**
     * The subclasses must implement this method, which will say that the rows follow their specific
     * algorithm
     */
    public abstract boolean verifySameRowsRS(Resources resources, Allocation ideal,
            Allocation given, int offset, int stride, int width, int height,
            RenderScript renderScript, Allocation inputAllocation, Allocation outputAllocation);

    public boolean verifySameRS(Resources resources, Allocation ideal,
            Allocation given, int offset, int stride, int width, int height,
            RenderScript renderScript) {
        if (mRowInputs == null) {
            mHeight = height;
            mRowInputs = createInputRowIndexAllocation(renderScript);
            mRowOutputs = createOutputRowAllocation(renderScript);
        }
        return verifySameRowsRS(resources, ideal, given, offset, stride, width, height,
                renderScript, mRowInputs, mRowOutputs);
    }

    public boolean supportsRenderScript() {
        return true;
    }

    /**
     * Sums the values in the output Allocation
     */
    public float sum1DFloatAllocation(Allocation rowOutputs) {
        //Get the values returned from the function
        float[] returnValue = new float[mHeight];
        rowOutputs.copyTo(returnValue);
        float sum = 0;
        //If any row had any different pixels, then it fails
        for (int i = 0; i < mHeight; i++) {
            sum += returnValue[i];
        }
        return sum;
    }

    /**
     * Creates an allocation where the values in it are the indices of each row
     */
    private Allocation createInputRowIndexAllocation(RenderScript renderScript) {
        //Create an array with the index of each row
        int[] inputIndices = new int[mHeight];
        for (int i = 0; i < mHeight; i++) {
            inputIndices[i] = i;
        }
        //Create the allocation from that given array
        Allocation inputAllocation = Allocation.createSized(renderScript, Element.I32(renderScript),
                inputIndices.length, Allocation.USAGE_SCRIPT);
        inputAllocation.copyFrom(inputIndices);
        return inputAllocation;
    }

    private Allocation createOutputRowAllocation(RenderScript renderScript) {
        return Allocation.createSized(renderScript, Element.F32(renderScript), mHeight,
                Allocation.USAGE_SCRIPT);
    }
}
