#pragma once

extern void initInnerLoop(HaarVars hf, int origWidth, int origHeight);
extern void innerloops(const int height, const int width, const int* inArr, const int* inArrSq, const int yStep, bool* outData);
extern void cleanUpInnerLoops();