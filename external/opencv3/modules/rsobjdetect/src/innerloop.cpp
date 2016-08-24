#include "RenderScript.h"
#include "ScriptC_detectAt.h"
#include "../../objdetect/src/HaarStructs.h"

using namespace android;
using namespace RSC;
using namespace std;

static sp<RS> rs;
static sp<ScriptC_detectAt> sc;

void initInnerLoop(HaarVars hf, int origWidth, int origHeight) {
    rs = new RS();
    bool r = rs->init("/data/data/com.example.noahp.facialrecogrs/cache");

    sc = new ScriptC_detectAt(rs);

    sc->set_origWidth(origWidth);
    sc->set_origHeight(origHeight);
    sc->set_sqofs(hf.sqofs);
    sc->set_normRectArea(hf.normRectArea);
    sc->set_stagesSize(hf.stagesSize);

    sc->invoke_initCurr();

    const HaarStump* cascadeStumps = &hf.stumps[0];
    const HaarStage* cascadeStages = &hf.stages[0];

    for( int i = 0; i < hf.nStumps; i++ )
    {
        const HaarStump stump = cascadeStumps[i];
        sc->invoke_addStump(i, stump.threshold, stump.left, stump.right);
    }

    for(int stageIdx = 0; stageIdx < hf.stagesSize; stageIdx++) {
        const HaarStage stage = cascadeStages[stageIdx];
        sc->invoke_addStage(stage.first, stage.ntrees, stage.threshold);
        int ntrees = stage.ntrees;
    }

    for( int i = 0; i < hf.nFeatures; i++ )
    {
        const HaarFeature f = hf.haarFeatures[i];
        sc->invoke_addHF(f.x[0],f.y[0],f.width[0],f.height[0],
                            f.x[1],f.y[1],f.width[1],f.height[1],
                            f.x[2],f.y[2],f.width[2],f.height[2],
                            f.weight0, f.weight1, f.weight2);
    }

    sc->set_nrect(UInt4(hf.nrect.x, hf.nrect.y, hf.nrect.width, hf.nrect.height));
}

void innerloops(const int height, const int width, const int* inArr, const int* inArrSq, const int yStep, bool* outData) {
    sp<Allocation> outAllocation;
    sp<const Element> e2 = Element::BOOLEAN(rs);
    Type::Builder tb2(rs, e2);
    tb2.setX(width*height);
    sp<const Type> t2 = tb2.create();
    outAllocation = Allocation::createTyped(rs,t2);

    sp<Allocation> inAllocation;
    sp<const Element> e = Element::I32(rs);
    Type::Builder tb(rs, e);
    tb.setX(width*height);
    sp<const Type> t = tb.create();
    inAllocation = Allocation::createTyped(rs,t);
    inAllocation->copy1DRangeFrom(0,width*height,inArr);
    sc->set_inAlloc(inAllocation);

    sp<Allocation> inAllocationSq;
    sp<const Element> e3 = Element::I32(rs);
    inAllocationSq = Allocation::createTyped(rs,t);
    inAllocationSq->copy1DRangeFrom(0,width*height,inArrSq);
    sc->set_inAllocSq(inAllocationSq);

    sc->set_width(width);
    sc->set_height(height);
    sc->set_yStep(yStep);

    sc->forEach_runAtHaarKernel(inAllocation, outAllocation);
    outAllocation->copy1DRangeTo(0,width*height,outData);
}

void cleanUpInnerLoops() {
    rs->finish();
}