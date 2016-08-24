#pragma version(1)
#pragma rs java_package_name(unused)
#pragma rs_fp_relaxed

#define numStumps 5000
#define numStages 22
#define numRect 3
#define numFeatures 10000

typedef struct stag
{
    int first;
    int ntrees;
    float threshold;
} HaarStage;

typedef struct stum
{
    int featureIdx;
    float threshold;
    float left;
    float right;
} HaarStump;

typedef struct optFe
{
    uint4 ofs0;
    uint4 ofs1;
    uint4 ofs2;
    float4 weight;
} HaarOptFeature;

typedef struct Fe
{
    int x[numRect];
    int y[numRect];
    int width[numRect];
    int height[numRect];
    float weight[numRect];
} HaarFeature;

int sqofs;
uint4 nrect;
float normRectArea;
int stagesSize;
int width;
int height;
int origWidth;
int origHeight;
int yStep;
rs_allocation inAlloc;
rs_allocation inAllocSq;

static HaarStump stumps[numStumps];
static HaarStage stages[numStages];
static HaarFeature haarFeatures[numFeatures];
static int currStage;
static int currStump;
static int currHf;

static int calcSumOfs(const int x, const int of0, const int of1, const int of2, const int of3, const rs_allocation in) {
    int t1 = rsGetElementAt_int(in, x + of0);
    int t2 = rsGetElementAt_int(in, x + of1);
    int t3 = rsGetElementAt_int(in, x + of2);
    int t4 = rsGetElementAt_int(in, x + of3);
    return t1 - t2 - t3 + t4;
}

static int evaluateIntegral(const int x, const int imgWidth, const int offNum, const HaarFeature _f) {
    int of0 = _f.x[offNum] + imgWidth * _f.y[offNum];
    int of1 = _f.x[offNum] + _f.width[offNum] + imgWidth * _f.y[offNum];
    int of2 = _f.x[offNum] + imgWidth * (_f.y[offNum] + _f.height[offNum]);
    int of3 = _f.x[offNum] + _f.width[offNum] + imgWidth * (_f.y[offNum] + _f.height[offNum]);
    return calcSumOfs(x,of0,of1,of2,of3, inAlloc);
}

static int evaluateIntegralNof(const rs_allocation in, const int x, const int imgWidth) {
    int of0 = nrect.s0 + imgWidth * nrect.s1;
    int of1 = nrect.s0 + nrect.s2 + imgWidth * nrect.s1;
    int of2 = nrect.s0 + imgWidth * (nrect.s1 + nrect.s3);
    int of3 = nrect.s0 + nrect.s2 + imgWidth * (nrect.s1 + nrect.s3);
    return calcSumOfs(x,of0,of1,of2,of3,in);
}

bool RS_KERNEL runAtHaarKernel(const int in, const int x)
{
    int x_check = x % width;
    int y_check = x / width;
    if (!(x_check % yStep == 0 && y_check % yStep == 0 ))
        return false;
    if( !(x_check < 0 || y_check < 0 ||
      x_check + origWidth >= width ||
      y_check + origHeight >= height )) {
        float varianceNormFactor;
        int valsum = evaluateIntegralNof(inAlloc,x, width);
        unsigned valsqsum = (unsigned) evaluateIntegralNof(inAllocSq, x, width);
        float area = normRectArea;
        float nf = area * valsqsum - (float)valsum * valsum;

        if( nf > 0.f ) {
           nf = sqrt((float)nf);
           varianceNormFactor = (float)(1./nf);
           if(!(area*varianceNormFactor < 0.1f)) return false;
        }
        else {
            varianceNormFactor = 1.0f;
            return false;
        }

        int nstages = currStage;
        float tmp = 0.f;
        int stumpOfs = 0;

        for( int stageIdx = 0; stageIdx < nstages; stageIdx++ ) {
             const HaarStage stage = stages[stageIdx];
             tmp = 0.f;
             int ntrees = stage.ntrees;

             for( int i = 0; i < ntrees; i++ ) {
               const HaarStump stump = stumps[i + stumpOfs];
               float ret = haarFeatures[stump.featureIdx].weight[0]
                    * evaluateIntegral(x, width, 0, haarFeatures[stump.featureIdx])
                    + haarFeatures[stump.featureIdx].weight[1]
                    * evaluateIntegral(x, width, 1, haarFeatures[stump.featureIdx]);
               if( haarFeatures[stump.featureIdx].weight[2] != 0.0f )
                   ret += haarFeatures[stump.featureIdx].weight[2]
                        * evaluateIntegral(x, width, 2, haarFeatures[stump.featureIdx]);
                ret *= varianceNormFactor;
                tmp += ret < stump.threshold ? stump.left : stump.right;
             }

             if( tmp < stage.threshold ) return false;
             stumpOfs += ntrees;
        }
        return true;
    }
    return false;
}

void initCurr() {
    currStump = 0;
    currStage = 0;
    currHf = 0;
}

void addStage(const int first, const int ntrees, const float threshold) {
    HaarStage h;
    h.first = first;
    h.ntrees = ntrees;
    h.threshold = threshold;
    stages[currStage] = h;
    currStage++;
}

void addStump(const int featureIdx, const float threshold, const float left, const float right) {
    HaarStump h;
    h.featureIdx = featureIdx;
    h.threshold = threshold;
    h.left = left;
    h.right = right;
    stumps[currStump] = h;
    currStump++;
}

void addHF(const int x0, const int y0, const int w0, const int h0,
            const int x1, const int y1, const int w1, const int h1,
            const int x2, const int y2, const int w2, const int h2,
            const float we0, const float we1, const float we2) {
    HaarFeature f;
    f.x[0] = x0;
    f.x[1] = x1;
    f.x[2] = x2;

    f.y[0] = y0;
    f.y[1] = y1;
    f.y[2] = y2;

    f.width[0] = w0;
    f.width[1] = w1;
    f.width[2] = w2;

    f.height[0] = h0;
    f.height[1] = h1;
    f.height[2] = h2;

    f.weight[0] = we0;
    f.weight[1] = we1;
    f.weight[2] = we2;

    haarFeatures[currHf] = f;
    currHf++;
}