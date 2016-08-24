#include "shared.rsh"

// Has the same kernels as reduce_backward.rs, plus some others.
//
// This test case places the pragmas before the functions (forward
// reference), and the other test case places the pragmas after the
// functions (backward reference).

float negInf, posInf;

/////////////////////////////////////////////////////////////////////////

#pragma rs reduce(addint) \
  accumulator(aiAccum)

static void aiAccum(int *accum, int val) { *accum += val; }

/////////////////////////////////////////////////////////////////////////

#pragma rs reduce(findMinAndMax) \
  initializer(fMMInit) accumulator(fMMAccumulator) \
  combiner(fMMCombiner) outconverter(fMMOutConverter)

typedef struct {
  float val;
  int idx;
} IndexedVal;

typedef struct {
  IndexedVal min, max;
} MinAndMax;

static void fMMInit(MinAndMax *accum) {
  accum->min.val = posInf;
  accum->min.idx = -1;
  accum->max.val = negInf;
  accum->max.idx = -1;
}

static void fMMAccumulator(MinAndMax *accum, float in, int x) {
  IndexedVal me;
  me.val = in;
  me.idx = x;

  if (me.val < accum->min.val)
    accum->min = me;
  if (me.val > accum->max.val)
    accum->max = me;
}

static void fMMCombiner(MinAndMax *accum,
                        const MinAndMax *val) {
  if (val->min.val < accum->min.val)
    accum->min = val->min;
  if (val->max.val > accum->max.val)
    accum->max = val->max;
}

static void fMMOutConverter(int2 *result,
                            const MinAndMax *val) {
  result->x = val->min.idx;
  result->y = val->max.idx;
}

/////////////////////////////////////////////////////////////////////////

#pragma rs reduce(fz) \
  initializer(fzInit) \
  accumulator(fzAccum) combiner(fzCombine)

static void fzInit(int *accumIdx) { *accumIdx = -1; }

static void fzAccum(int *accumIdx,
                    int inVal, int x /* special arg */) {
  if (inVal==0) *accumIdx = x;
}

static void fzCombine(int *accumIdx, const int *accumIdx2) {
  if (*accumIdx2 >= 0) *accumIdx = *accumIdx2;
}

/////////////////////////////////////////////////////////////////////////

#pragma rs reduce(fz2) \
  initializer(fz2Init) \
  accumulator(fz2Accum) combiner(fz2Combine)

static void fz2Init(int2 *accum) { accum->x = accum->y = -1; }

static void fz2Accum(int2 *accum,
                     int inVal,
                     int x /* special arg */,
                     int y /* special arg */) {
  if (inVal==0) {
    accum->x = x;
    accum->y = y;
  }
}

static void fz2Combine(int2 *accum, const int2 *accum2) {
  if (accum2->x >= 0) *accum = *accum2;
}

/////////////////////////////////////////////////////////////////////////

#pragma rs reduce(fz3) \
  initializer(fz3Init) \
  accumulator(fz3Accum) combiner(fz3Combine)

static void fz3Init(int3 *accum) { accum->x = accum->y = accum->z = -1; }

static void fz3Accum(int3 *accum,
                     int inVal,
                     int x /* special arg */,
                     int y /* special arg */,
                     int z /* special arg */) {
  if (inVal==0) {
    accum->x = x;
    accum->y = y;
    accum->z = z;
  }
}

static void fz3Combine(int3 *accum, const int3 *accum2) {
  if (accum2->x >= 0) *accum = *accum2;
}

/////////////////////////////////////////////////////////////////////////

#pragma rs reduce(histogram) \
  accumulator(hsgAccum) combiner(hsgCombine)

#define BUCKETS 256
typedef uint32_t Histogram[BUCKETS];

static void hsgAccum(Histogram *h, uchar in) { ++(*h)[in]; }

static void hsgCombine(Histogram *accum, const Histogram *addend) {
  for (int i = 0; i < BUCKETS; ++i)
    (*accum)[i] += (*addend)[i];
}

#pragma rs reduce(mode) \
  accumulator(hsgAccum) combiner(hsgCombine) \
  outconverter(modeOutConvert)

static void modeOutConvert(int2 *result, const Histogram *h) {
  uint32_t mode = 0;
  for (int i = 1; i < BUCKETS; ++i)
    if ((*h)[i] > (*h)[mode]) mode = i;
  result->x = mode;
  result->y = (*h)[mode];
}

/////////////////////////////////////////////////////////////////////////

#pragma rs reduce(sumgcd) accumulator(sgAccum) combiner(sgCombine)

static int gcd(int a, int b) {
  while (b != 0) {
    const int aNew = b;
    const int bNew = a % b;

    a = aNew;
    b = bNew;
  }
  return a;
}

static void sgAccum(long *accum, int a, int b) {
  *accum += gcd(a, b);
}

static void sgCombine(long *accum, const long *other) { *accum += *other; }

/////////////////////////////////////////////////////////////////////////

// These two kernels have anonymous result types that are equivalent.
// slang doesn't common them (i.e., each gets its own RSExportType);
// so Java reflection must guard against this to avoid creating two
// copies of the text that defines the reflected class resultArray4_int.

#pragma rs reduce(sillySumIntoDecArray) accumulator(aiAccum) outconverter(outSillySumIntoDecArray)
static void outSillySumIntoDecArray(int (*out)[4], const int *accumDatum) {
  for (int i = 0; i < 4; ++i)
    (*out)[i] = (*accumDatum)/(i+1);
}

#pragma rs reduce(sillySumIntoIncArray) accumulator(aiAccum) outconverter(outSillySumIntoIncArray)
static void outSillySumIntoIncArray(int (*out)[4], const int *accumDatum) {
  for (int i = 0; i < 4; ++i)
    (*out)[i] = (*accumDatum)/(4-i);
}

/////////////////////////////////////////////////////////////////////////

// finds min values (not their locations) from matrix input

// tests matrix input and matrix accumulator

// also tests calling conventions for two different composite types
// rs_matrix2x2: 32-bit coerces this to an int array
//               64-bit coerces this to float array
// rs_matrix4x4: 64-bit passes this by reference

//.......................................................................

#pragma rs reduce(findMinMat2) \
  initializer(fMinMat2Init) accumulator(fMinMat2Accumulator) \
  outconverter(fMinMat2OutConverter)

static void fMinMat2Init(rs_matrix2x2 *accum) {
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      rsMatrixSet(accum, i, j, posInf);
}

static void fMinMat2Accumulator(rs_matrix2x2 *accum, rs_matrix2x2 val) {
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      const float accumElt = rsMatrixGet(accum, i, j);
      const float valElt = rsMatrixGet(&val, i, j);
      if (valElt < accumElt)
        rsMatrixSet(accum, i, j, valElt);
    }
  }
}

// reduction does not support matrix result, so use array instead
static void fMinMat2OutConverter(float (*result)[4],  const rs_matrix2x2 *accum) {
  for (int i = 0; i < 4; ++i)
    (*result)[i] = accum->m[i];
}

//.......................................................................

#pragma rs reduce(findMinMat4) \
  initializer(fMinMat4Init) accumulator(fMinMat4Accumulator) \
  outconverter(fMinMat4OutConverter)

static void fMinMat4Init(rs_matrix4x4 *accum) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      rsMatrixSet(accum, i, j, posInf);
}

static void fMinMat4Accumulator(rs_matrix4x4 *accum, rs_matrix4x4 val) {
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      const float accumElt = rsMatrixGet(accum, i, j);
      const float valElt = rsMatrixGet(&val, i, j);
      if (valElt < accumElt)
        rsMatrixSet(accum, i, j, valElt);
    }
  }
}

// reduction does not support matrix result, so use array instead
static void fMinMat4OutConverter(float (*result)[16],  const rs_matrix4x4 *accum) {
  for (int i = 0; i < 16; ++i)
    (*result)[i] = accum->m[i];
}
