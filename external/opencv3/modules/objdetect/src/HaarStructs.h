#pragma once

typedef struct
{
    int first;
    int ntrees;
    float threshold;
} HaarStage;

typedef struct
{
    int featureIdx;
    float threshold;
    float left;
    float right;
} HaarStump;


typedef struct optFe
{
    int ofs0[4];
    int ofs1[4];
    int ofs2[4];
    float weight[4];
} HaarOptFeature;

typedef struct Fe
{
    int x[3];
    int y[3];
    int width[3];
    int height[3];
    float weight0;
    float weight1;
    float weight2;
} HaarFeature;

typedef struct hr
{
    int x;
    int y;
    int width;
    int height;
} HaarRect;

typedef struct
{
    int sqofs;
    int nofs[4];
    HaarRect nrect;
    double normRectArea;
    HaarStump* stumps;
    HaarStage* stages;
    HaarOptFeature* haarOptFeatures;
    HaarFeature* haarFeatures;
    int stagesSize;
    int nFeatures;
    int nStumps;
} HaarVars;