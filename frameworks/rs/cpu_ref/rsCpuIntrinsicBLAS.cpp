/*
 * Copyright (C) 2012 The Android Open Source Project
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


#include "rsCpuIntrinsic.h"
#include "rsCpuIntrinsicInlines.h"
#include "rsCpuBLASDispatch.h"
#include "eight_bit_int_gemm.h"

using namespace android;
using namespace android::renderscript;

namespace android {
namespace renderscript {


class RsdCpuScriptIntrinsicBLAS : public RsdCpuScriptIntrinsic {
public:
    void invokeForEach(uint32_t slot,
                       const Allocation ** ain,
                       uint32_t inLen,
                       Allocation * aout,
                       const void * usr,
                       uint32_t usrLen,
                       const RsScriptCall *sc) override;

    void populateScript(Script *) override;
    ~RsdCpuScriptIntrinsicBLAS() override;
    RsdCpuScriptIntrinsicBLAS(RsdCpuReferenceImpl *ctx, const Script *s);

protected:

    uint8_t a_offset = 0;
    uint8_t b_offset = 0;
    uint8_t c_offset = 0;

#ifdef RS_COMPATIBILITY_LIB
    bool isBlasLibInitialized = false;
#endif
    static void kernelBNNM(size_t m, size_t n, size_t k,
                           const uint8_t* a, uint8_t a_offset, size_t lda,
                           const uint8_t* b, uint8_t b_offset, size_t ldb,
                           uint8_t* c, int32_t c_offset, size_t ldc,
                           int32_t c_mult_int);



};

}
}

void RsdCpuScriptIntrinsicBLAS::populateScript(Script *s) {
    s->mHal.info.exportedVariableCount = 0;
}

static void initABC(const Allocation ** ain,
                    size_t size,
                    void** A,
                    void** B,
                    void** C,
                    int* lda,
                    int* ldb,
                    int* ldc)
{
    if (ain[0]) {
        *A = ain[0]->mHal.drvState.lod[0].mallocPtr;
        *lda = (int)(ain[0]->mHal.drvState.lod[0].stride/size);
    }
    if (ain[1]) {
        *B = ain[1]->mHal.drvState.lod[0].mallocPtr;
        *ldb = (int)(ain[1]->mHal.drvState.lod[0].stride/size);
    }
    if (ain[2]) {
        *C = ain[2]->mHal.drvState.lod[0].mallocPtr;
        *ldc = (int)(ain[2]->mHal.drvState.lod[0].stride/size);
    }


}

void RsdCpuScriptIntrinsicBLAS::invokeForEach(uint32_t slot,
                                              const Allocation ** ain,
                                              uint32_t inLen,
                                              Allocation * aout,
                                              const void * usr,
                                              uint32_t usrLen,
                                              const RsScriptCall *sc) {
    RsBlasCall* call = (RsBlasCall*) usr;
    // setup BLAS enum args
    enum CBLAS_TRANSPOSE TransA = (enum CBLAS_TRANSPOSE)call->transA;
    enum CBLAS_TRANSPOSE TransB = (enum CBLAS_TRANSPOSE)call->transB;
    enum CBLAS_UPLO Uplo = (enum CBLAS_UPLO)call->uplo;
    enum CBLAS_DIAG Diag = (enum CBLAS_DIAG)call->diag;
    enum CBLAS_SIDE Side = (enum CBLAS_SIDE)call->side;

    void *A = nullptr;
    void *B = nullptr;
    void *C = nullptr;
    void *X = nullptr;
    void *Y = nullptr;

    int lda = 0, ldb = 0, ldc = 0;

#ifdef RS_COMPATIBILITY_LIB
    // Allow BNNM even without libblas
    if (call->func != RsBlas_bnnm && !isBlasLibInitialized) {
        if (!loadBLASLib()) {
            ALOGE("Failed to load the BLAS lib, IntrinsicBLAS NOT supported!\n");
            return;
        }
        isBlasLibInitialized = true;
    }
#endif

    switch (call->func) {

    // Level 1 BLAS: returns into a 1D Allocation


    // Level 2 BLAS
    case (RsBlas_sgemv):
        initABC(ain, sizeof(float), &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_sgemv(CblasRowMajor, TransA, call->M, call->N, call->alpha.f, (float*)A,
                    lda, (float*)X, call->incX, call->beta.f, (float*)Y, call->incY);
        break;
    case (RsBlas_sgbmv):
        initABC(ain, sizeof(float), &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_sgbmv(CblasRowMajor, TransA, call->M, call->N, call->KL, call->KU,
                    call->alpha.f, (float*)A, lda, (float*)X, call->incX,
                    call->beta.f, (float*)Y, call->incY);
        break;
    case (RsBlas_strmv):
        initABC(ain, sizeof(float), &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_strmv(CblasRowMajor, Uplo, TransA, Diag, call->N, (float*)A,
                    lda, (float*)X, call->incX);
        break;
    case (RsBlas_stbmv):
        initABC(ain, sizeof(float), &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_stbmv(CblasRowMajor, Uplo, TransA, Diag, call->N, call->K, (float*)A,
                    lda, (float*)X, call->incX);
        break;
    // stpmv takes a packed 1D Allocation only
    case (RsBlas_stpmv):
        initABC(ain, sizeof(float), &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_stpmv(CblasRowMajor, Uplo, TransA, Diag, call->N, (float*)A,
                    (float*)X, call->incX);
        break;
    case (RsBlas_strsv):
        initABC(ain, sizeof(float), &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_strsv(CblasRowMajor, Uplo, TransA, Diag, call->N, (float*)A, lda,
                    (float*)X, call->incX);
        break;
    case (RsBlas_stbsv):
        initABC(ain, sizeof(float), &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_stbsv(CblasRowMajor, Uplo, TransA, Diag, call->N, call->K, (float*)A,
                    lda, (float*)X, call->incX);
        break;
    case (RsBlas_stpsv):
        initABC(ain, sizeof(float), &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_stpsv(CblasRowMajor, Uplo, TransA, Diag, call->N, (float*)A,
                    (float*)X, call->incX);
        break;
    case (RsBlas_dgemv):
        initABC(ain, sizeof(double), &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_dgemv(CblasRowMajor, TransA, call->M, call->N, call->alpha.d, (double*)A,
                    lda, (double*)X, call->incX, call->beta.d, (double*)Y, call->incY);
        break;
    case (RsBlas_dgbmv):
        initABC(ain, sizeof(double), &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_dgbmv(CblasRowMajor, TransA, call->M, call->N, call->KL, call->KU,
                    call->alpha.d, (double*)A, lda, (double*)X, call->incX,
                    call->beta.d, (double*)Y, call->incY);
        break;
    case (RsBlas_dtrmv):
        initABC(ain, sizeof(double), &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_dtrmv(CblasRowMajor, Uplo, TransA, Diag, call->N, (double*)A,
                    lda, (double*)X, call->incX);
        break;
    case (RsBlas_dtbmv):
        initABC(ain, sizeof(double), &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_dtbmv(CblasRowMajor, Uplo, TransA, Diag, call->N, call->K, (double*)A,
                    lda, (double*)X, call->incX);
        break;
    // stpmv takes a packed 1D Allocation only
    case (RsBlas_dtpmv):
        initABC(ain, sizeof(double), &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_dtpmv(CblasRowMajor, Uplo, TransA, Diag, call->N, (double*)A,
                    (double*)X, call->incX);
        break;
    case (RsBlas_dtrsv):
        initABC(ain, sizeof(double), &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_dtrsv(CblasRowMajor, Uplo, TransA, Diag, call->N, (double*)A, lda,
                    (double*)X, call->incX);
        break;
    case (RsBlas_dtbsv):
        initABC(ain, sizeof(double), &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_dtbsv(CblasRowMajor, Uplo, TransA, Diag, call->N, call->K, (double*)A,
                    lda, (double*)X, call->incX);
        break;
    case (RsBlas_dtpsv):
        initABC(ain, sizeof(double), &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_dtpsv(CblasRowMajor, Uplo, TransA, Diag, call->N, (double*)A,
                    (double*)X, call->incX);
        break;
    case (RsBlas_cgemv):
        initABC(ain, sizeof(float)*2, &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_cgemv(CblasRowMajor, TransA, call->M, call->N, (void*)&call->alpha.c, (void*)A,
                    lda, (void*)X, call->incX, (void*)&call->beta.c, (void*)Y, call->incY);
        break;
    case (RsBlas_cgbmv):
        initABC(ain, sizeof(float)*2, &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_cgbmv(CblasRowMajor, TransA, call->M, call->N, call->KL, call->KU,
                    (void*)&call->alpha.c, (void*)A, lda, (void*)X, call->incX,
                    (void*)&call->beta.c, (void*)Y, call->incY);
        break;
    case (RsBlas_ctrmv):
        initABC(ain, sizeof(float)*2, &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_ctrmv(CblasRowMajor, Uplo, TransA, Diag, call->N, (void*)A,
                    lda, (void*)X, call->incX);
        break;
    case (RsBlas_ctbmv):
        initABC(ain, sizeof(float)*2, &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_ctbmv(CblasRowMajor, Uplo, TransA, Diag, call->N, call->K, (void*)A,
                    lda, (void*)X, call->incX);
        break;
    // stpmv takes a packed 1D Allocation only
    case (RsBlas_ctpmv):
        initABC(ain, sizeof(float)*2, &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_ctpmv(CblasRowMajor, Uplo, TransA, Diag, call->N, (void*)A,
                    (void*)X, call->incX);
        break;
    case (RsBlas_ctrsv):
        initABC(ain, sizeof(float)*2, &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_ctrsv(CblasRowMajor, Uplo, TransA, Diag, call->N, (void*)A, lda,
                    (void*)X, call->incX);
        break;
    case (RsBlas_ctbsv):
        initABC(ain, sizeof(float)*2, &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_ctbsv(CblasRowMajor, Uplo, TransA, Diag, call->N, call->K, (void*)A,
                    lda, (void*)X, call->incX);
        break;
    case (RsBlas_ctpsv):
        initABC(ain, sizeof(float)*2, &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_ctpsv(CblasRowMajor, Uplo, TransA, Diag, call->N, (void*)A,
                    (void*)X, call->incX);
        break;
    case (RsBlas_zgemv):
        initABC(ain, sizeof(double)*2, &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_zgemv(CblasRowMajor, TransA, call->M, call->N, (void*)&call->alpha.z, (void*)A,
                    lda, (void*)X, call->incX, (void*)&call->beta.z, (void*)Y, call->incY);
        break;
    case (RsBlas_zgbmv):
        initABC(ain, sizeof(double)*2, &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_zgbmv(CblasRowMajor, TransA, call->M, call->N, call->KL, call->KU,
                    (void*)&call->alpha.z, (void*)A, lda, (void*)X, call->incX,
                    (void*)&call->beta.z, (void*)Y, call->incY);
        break;
    case (RsBlas_ztrmv):
        initABC(ain, sizeof(double)*2, &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_ztrmv(CblasRowMajor, Uplo, TransA, Diag, call->N, (void*)A,
                    lda, (void*)X, call->incX);
        break;
    case (RsBlas_ztbmv):
        initABC(ain, sizeof(double)*2, &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_ztbmv(CblasRowMajor, Uplo, TransA, Diag, call->N, call->K, (void*)A,
                    lda, (void*)X, call->incX);
        break;
    // stpmv takes a packed 1D Allocation only
    case (RsBlas_ztpmv):
        initABC(ain, sizeof(double)*2, &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_ztpmv(CblasRowMajor, Uplo, TransA, Diag, call->N, (void*)A,
                    (void*)X, call->incX);
        break;
    case (RsBlas_ztrsv):
        initABC(ain, sizeof(double)*2, &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_ztrsv(CblasRowMajor, Uplo, TransA, Diag, call->N, (void*)A, lda,
                    (void*)X, call->incX);
        break;
    case (RsBlas_ztbsv):
        initABC(ain, sizeof(double)*2, &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_ztbsv(CblasRowMajor, Uplo, TransA, Diag, call->N, call->K, (void*)A,
                    lda, (void*)X, call->incX);
        break;
    case (RsBlas_ztpsv):
        initABC(ain, sizeof(double)*2, &A, &X, nullptr, &lda, &ldb, nullptr);
        cblas_ztpsv(CblasRowMajor, Uplo, TransA, Diag, call->N, (void*)A,
                    (void*)X, call->incX);
        break;


    // S and D only
    case (RsBlas_ssymv):
        initABC(ain, sizeof(float), &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_ssymv(CblasRowMajor, Uplo, call->N, call->alpha.f, (float*)A, lda,
                    (float*)X, call->incX, call->beta.f, (float*)Y, call->incY);
        break;
    case (RsBlas_ssbmv):
        initABC(ain, sizeof(float), &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_ssbmv(CblasRowMajor, Uplo, call->N, call->K, call->alpha.f,
                    (float*)A, lda, (float*)X, call->incX, call->beta.f,
                    (float*)Y, call->incY);
        break;
    //sspmv requires a packed 1D Allocation
    case (RsBlas_sspmv):
        initABC(ain, sizeof(float), &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_sspmv(CblasRowMajor, Uplo, call->N, call->alpha.f, (float*)A,
                    (float*)X, call->incX, call->beta.f, (float*)Y, call->incY);
        break;
    // following calls have init reordered because A is output matrix
    case (RsBlas_sger):
        initABC(ain, sizeof(float), &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_sger(CblasRowMajor, call->M, call->N, call->alpha.f, (float*)X,
                   call->incX, (float*)Y, call->incY, (float*)A, lda);
        break;
    case (RsBlas_ssyr):
        initABC(ain, sizeof(float), &X, &A, nullptr, &ldb, &lda, nullptr);
        cblas_ssyr(CblasRowMajor, Uplo, call->N, call->alpha.f, (float*)X, call->incX,
                   (float*)A, lda);
        break;
    // sspr is packed 1D Allocation A only
    case (RsBlas_sspr):
        initABC(ain, sizeof(float), &X, &A, nullptr, &ldb, &lda, nullptr);
        cblas_sspr(CblasRowMajor, Uplo, call->N, call->alpha.f, (float*)X, call->incX,
                   (float*)A);
        break;
    case (RsBlas_ssyr2):
        initABC(ain, sizeof(float), &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_ssyr2(CblasRowMajor, Uplo, call->N, call->alpha.f, (float*)X, call->incX,
                    (float*)Y, call->incY, (float*)A, lda);
        break;
    // sspr2 is packed 1D Allocation A only
    case (RsBlas_sspr2):
        initABC(ain, sizeof(float), &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_sspr2(CblasRowMajor, Uplo, call->N, call->alpha.f, (float*)X, call->incX,
                    (float*)Y, call->incY, (float*)A);
        break;
    case (RsBlas_dsymv):
        initABC(ain, sizeof(double), &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_dsymv(CblasRowMajor, Uplo, call->N, call->alpha.d, (double*)A, lda,
                    (double*)X, call->incX, call->beta.d, (double*)Y, call->incY);
        break;
    case (RsBlas_dsbmv):
        initABC(ain, sizeof(double), &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_dsbmv(CblasRowMajor, Uplo, call->N, call->K, call->alpha.d,
                    (double*)A, lda, (double*)X, call->incX, call->beta.d,
                    (double*)Y, call->incY);
        break;
    // dspmv requires a packed 1D Allocation
    case (RsBlas_dspmv):
        initABC(ain, sizeof(double), &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_dspmv(CblasRowMajor, Uplo, call->N, call->alpha.d, (double*)A,
                    (double*)X, call->incX, call->beta.d, (double*)Y, call->incY);
        break;
    // following calls have init reordered because A is output matrix
    case (RsBlas_dger):
        initABC(ain, sizeof(double), &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_dger(CblasRowMajor, call->M, call->N, call->alpha.d, (double*)X,
                   call->incX, (double*)Y, call->incY, (double*)A, lda);
        break;
    case (RsBlas_dsyr):
        initABC(ain, sizeof(double), &X, &A, nullptr, &ldb, &lda, nullptr);
        cblas_dsyr(CblasRowMajor, Uplo, call->N, call->alpha.d, (double*)X, call->incX,
                   (double*)A, lda);
        break;
    // dspr is packed 1D Allocation A only
    case (RsBlas_dspr):
        initABC(ain, sizeof(double), &X, &A, nullptr, &ldb, &lda, nullptr);
        cblas_dspr(CblasRowMajor, Uplo, call->N, call->alpha.d, (double*)X, call->incX,
                   (double*)A);
        break;
    case (RsBlas_dsyr2):
        initABC(ain, sizeof(double), &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_dsyr2(CblasRowMajor, Uplo, call->N, call->alpha.d, (double*)X, call->incX,
                    (double*)Y, call->incY, (double*)A, lda);
        break;
    // dspr2 is packed 1D Allocation A only
    case (RsBlas_dspr2):
        initABC(ain, sizeof(double), &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_dspr2(CblasRowMajor, Uplo, call->N, call->alpha.d, (double*)X, call->incX,
                    (double*)Y, call->incY, (double*)A);
        break;

    // C and Z only
    case (RsBlas_chemv):
        initABC(ain, sizeof(float)*2, &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_chemv(CblasRowMajor, Uplo, call->N, (void*)&call->alpha.c, A, lda,
                    X, call->incX, (void*)&call->beta.c, Y, call->incY);
        break;
    case (RsBlas_chbmv):
        initABC(ain, sizeof(float)*2, &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_chbmv(CblasRowMajor, Uplo, call->N, call->K, (void*)&call->alpha.c,
                    A, lda, X, call->incX, (void*)&call->beta.c, Y, call->incY);
        break;
    case (RsBlas_chpmv):
        initABC(ain, sizeof(float)*2, &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_chpmv(CblasRowMajor, Uplo, call->N, (void*)&call->alpha.c, A,
                    X, call->incX, (void*)&call->beta.c, Y, call->incY);
        break;
    case (RsBlas_cgeru):
        initABC(ain, sizeof(float)*2, &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_cgeru(CblasRowMajor, call->M, call->N, (void*)&call->alpha.c,
                    X, call->incX, Y, call->incY, A, lda);
        break;
    case (RsBlas_cgerc):
        initABC(ain, sizeof(float)*2, &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_cgerc(CblasRowMajor, call->M, call->N, (void*)&call->alpha.c,
                    X, call->incX, Y, call->incY, A, lda);
        break;
    case (RsBlas_cher):
        initABC(ain, sizeof(float)*2, &X, nullptr, &A, &ldb, nullptr, &lda);
        cblas_cher(CblasRowMajor, Uplo, call->N, call->alpha.f,
                   X, call->incX, A, lda);
        break;
    // packed 1D Allocations only
    case (RsBlas_chpr):
        initABC(ain, sizeof(float)*2, &X, nullptr, &A, &ldb, nullptr, &lda);
        cblas_chpr(CblasRowMajor, Uplo, call->N, call->alpha.f, X,
                   call->incX, A);
        break;
    case (RsBlas_cher2):
        initABC(ain, sizeof(float)*2, &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_cher2(CblasRowMajor, Uplo, call->N, (void*)&call->alpha.c,
                   X, call->incX, Y, call->incY, A, lda);
        break;
    // packed 1D Allocations only
    case (RsBlas_chpr2):
        initABC(ain, sizeof(float)*2, &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_chpr2(CblasRowMajor, Uplo, call->N, (void*)&call->alpha.c, X,
                   call->incX, Y, call->incY, A);
        break;
    case (RsBlas_zhemv):
        initABC(ain, sizeof(double)*2, &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_zhemv(CblasRowMajor, Uplo, call->N, (void*)&call->alpha.z, A, lda,
                    X, call->incX, (void*)&call->beta.z, Y, call->incY);
        break;
    case (RsBlas_zhbmv):
        initABC(ain, sizeof(double)*2, &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_zhbmv(CblasRowMajor, Uplo, call->N, call->K, (void*)&call->alpha.z,
                    A, lda, X, call->incX, (void*)&call->beta.z, Y, call->incY);
        break;
    case (RsBlas_zhpmv):
        initABC(ain, sizeof(double)*2, &A, &X, &Y, &lda, &ldb, &ldc);
        cblas_zhpmv(CblasRowMajor, Uplo, call->N, (void*)&call->alpha.z, A,
                    X, call->incX, (void*)&call->beta.z, Y, call->incY);
        break;
    case (RsBlas_zgeru):
        initABC(ain, sizeof(double)*2, &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_zgeru(CblasRowMajor, call->M, call->N, (void*)&call->alpha.z,
                    X, call->incX, Y, call->incY, A, lda);
        break;
    case (RsBlas_zgerc):
        initABC(ain, sizeof(double)*2, &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_zgerc(CblasRowMajor, call->M, call->N, (void*)&call->alpha.z,
                    X, call->incX, Y, call->incY, A, lda);
        break;
    case (RsBlas_zher):
        initABC(ain, sizeof(double)*2, &X, nullptr, &A, &ldb, nullptr, &lda);
        cblas_zher(CblasRowMajor, Uplo, call->N, call->alpha.d,
                   X, call->incX, A, lda);
        break;
    // packed 1D Allocations only
    case (RsBlas_zhpr):
        initABC(ain, sizeof(double)*2, &X, nullptr, &A, &ldb, nullptr, &lda);
        cblas_zhpr(CblasRowMajor, Uplo, call->N, call->alpha.d, X,
                   call->incX, A);
        break;
    case (RsBlas_zher2):
        initABC(ain, sizeof(double)*2, &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_zher2(CblasRowMajor, Uplo, call->N, (void*)&call->alpha.z,
                   X, call->incX, Y, call->incY, A, lda);
        break;
    // packed 1D Allocations only
    case (RsBlas_zhpr2):
        initABC(ain, sizeof(double)*2, &X, &Y, &A, &ldb, &ldc, &lda);
        cblas_zhpr2(CblasRowMajor, Uplo, call->N, (void*)&call->alpha.z, X,
                   call->incX, Y, call->incY, A);
        break;

    // Level 3 BLAS
    case (RsBlas_sgemm):
        initABC(ain, sizeof(float), &A, &B, &C, &lda, &ldb, &ldc);
        cblas_sgemm(CblasRowMajor, TransA, TransB, call->M, call->N, call->K, call->alpha.f,
                    (float*)A, lda, (float*)B, ldb, call->beta.f, (float*)C, ldc);
        break;
    case (RsBlas_ssymm):
        initABC(ain, sizeof(float), &A, &B, &C, &lda, &ldb, &ldc);
        cblas_ssymm(CblasRowMajor, Side, Uplo, call->M, call->N, call->alpha.f, (float*)A,
                    lda, (float*)B, ldb, call->beta.f, (float*)C, ldc);
        break;
    case (RsBlas_ssyrk):
        initABC(ain, sizeof(float), &A, nullptr, &C, &lda, nullptr, &ldc);
        cblas_ssyrk(CblasRowMajor, Uplo, TransA, call->N, call->K, call->alpha.f, (float*)A,
                    lda, call->beta.f, (float*)C, ldc);
        break;
    case (RsBlas_ssyr2k):
        initABC(ain, sizeof(float), &A, &B, &C, &lda, &ldb, &ldc);
        cblas_ssyr2k(CblasRowMajor, Uplo, TransA, call->N, call->K, call->alpha.f, (float*)A,
                     lda, (float*)B, ldb, call->beta.f, (float*)C, ldc);
        break;
    case (RsBlas_strmm):
        initABC(ain, sizeof(float), &A, &B, nullptr, &lda, &ldb, nullptr);
        cblas_strmm(CblasRowMajor, Side, Uplo, TransA, Diag, call->M, call->N, call->alpha.f,
                    (float*)A, lda, (float*)B, ldb);
        break;
    case (RsBlas_strsm):
        initABC(ain, sizeof(float), &A, &B, nullptr, &lda, &ldb, nullptr);
        cblas_strsm(CblasRowMajor, Side, Uplo, TransA, Diag, call->M, call->N, call->alpha.f,
                    (float*)A, lda, (float*)B, ldb);
        break;


    case (RsBlas_dgemm):
        initABC(ain, sizeof(double), &A, &B, &C, &lda, &ldb, &ldc);
        cblas_dgemm(CblasRowMajor, TransA, TransB, call->M, call->N, call->K, call->alpha.d,
                    (double*)A, lda, (double*)B, ldb, call->beta.d, (double*)C, ldc);
        break;
    case (RsBlas_dsymm):
        initABC(ain, sizeof(double), &A, &B, &C, &lda, &ldb, &ldc);
        cblas_dsymm(CblasRowMajor, Side, Uplo, call->M, call->N, call->alpha.d, (double*)A,
                    lda, (double*)B, ldb, call->beta.d, (double*)C, ldc);
        break;
    case (RsBlas_dsyrk):
        initABC(ain, sizeof(double), &A, nullptr, &C, &lda, nullptr, &ldc);
        cblas_dsyrk(CblasRowMajor, Uplo, TransA, call->N, call->K, call->alpha.d, (double*)A,
                    lda, call->beta.d, (double*)C, ldc);
        break;
    case (RsBlas_dsyr2k):
        initABC(ain, sizeof(double), &A, &B, &C, &lda, &ldb, &ldc);
        cblas_dsyr2k(CblasRowMajor, Uplo, TransA, call->N, call->K, call->alpha.d, (double*)A,
                     lda, (double*)B, ldb, call->beta.d, (double*)C, ldc);
        break;
    case (RsBlas_dtrmm):
        initABC(ain, sizeof(double), &A, &B, nullptr, &lda, &ldb, nullptr);
        cblas_dtrmm(CblasRowMajor, Side, Uplo, TransA, Diag, call->M, call->N, call->alpha.d,
                    (double*)A, lda, (double*)B, ldb);
        break;
    case (RsBlas_dtrsm):
        initABC(ain, sizeof(double), &A, &B, nullptr, &lda, &ldb, nullptr);
        cblas_dtrsm(CblasRowMajor, Side, Uplo, TransA, Diag, call->M, call->N, call->alpha.d,
                    (double*)A, lda, (double*)B, ldb);
        break;

    case (RsBlas_cgemm):
        initABC(ain, sizeof(float)*2, &A, &B, &C, &lda, &ldb, &ldc);
        cblas_cgemm(CblasRowMajor, TransA, TransB, call->M, call->N, call->K, (void*)&call->alpha.c,
                    A, lda, B, ldb, (void*)&call->beta.c, C, ldc);
        break;
    case (RsBlas_csymm):
        initABC(ain, sizeof(float)*2, &A, &B, &C, &lda, &ldb, &ldc);
        cblas_csymm(CblasRowMajor, Side, Uplo, call->M, call->N, (void*)&call->alpha.c, A,
                    lda, B, ldb, (void*)&call->beta.c, C, ldc);
        break;
    case (RsBlas_csyrk):
        initABC(ain, sizeof(float)*2, &A, nullptr, &C, &lda, nullptr, &ldc);
        cblas_csyrk(CblasRowMajor, Uplo, TransA, call->N, call->K, (void*)&call->alpha.c, A,
                    lda, (void*)&call->beta.c, C, ldc);
        break;
    case (RsBlas_csyr2k):
        initABC(ain, sizeof(float)*2, &A, &B, &C, &lda, &ldb, &ldc);
        cblas_csyr2k(CblasRowMajor, Uplo, TransA, call->N, call->K, (void*)&call->alpha.c, A,
                     lda, B, ldb, (void*)&call->beta.c, C, ldc);
        break;
    case (RsBlas_ctrmm):
        initABC(ain, sizeof(float)*2, &A, &B, nullptr, &lda, &ldb, nullptr);
        cblas_ctrmm(CblasRowMajor, Side, Uplo, TransA, Diag, call->M, call->N, (void*)&call->alpha.c,
                    A, lda, B, ldb);
        break;
    case (RsBlas_ctrsm):
        initABC(ain, sizeof(float)*2, &A, &B, nullptr, &lda, &ldb, nullptr);
        cblas_ctrsm(CblasRowMajor, Side, Uplo, TransA, Diag, call->M, call->N, (void*)&call->alpha.c,
                    A, lda, B, ldb);
        break;

    case (RsBlas_zgemm):
        initABC(ain, sizeof(double)*2, &A, &B, &C, &lda, &ldb, &ldc);
        cblas_zgemm(CblasRowMajor, TransA, TransB, call->M, call->N, call->K, (void*)&call->alpha.z,
                    A, lda, B, ldb, (void*)&call->beta.z, C, ldc);
        break;
    case (RsBlas_zsymm):
        initABC(ain, sizeof(double)*2, &A, &B, &C, &lda, &ldb, &ldc);
        cblas_zsymm(CblasRowMajor, Side, Uplo, call->M, call->N, (void*)&call->alpha.z, A,
                    lda, B, ldb, (void*)&call->beta.z, C, ldc);
        break;
    case (RsBlas_zsyrk):
        initABC(ain, sizeof(double)*2, &A, nullptr, &C, &lda, nullptr, &ldc);
        cblas_zsyrk(CblasRowMajor, Uplo, TransA, call->N, call->K, (void*)&call->alpha.z, A,
                    lda, (void*)&call->beta.z, C, ldc);
        break;
    case (RsBlas_zsyr2k):
        initABC(ain, sizeof(double)*2, &A, &B, &C, &lda, &ldb, &ldc);
        cblas_zsyr2k(CblasRowMajor, Uplo, TransA, call->N, call->K, (void*)&call->alpha.z, A,
                     lda, B, ldb, (void*)&call->beta.z, C, ldc);
        break;
    case (RsBlas_ztrmm):
        initABC(ain, sizeof(double)*2, &A, &B, nullptr, &lda, &ldb, nullptr);
        cblas_ztrmm(CblasRowMajor, Side, Uplo, TransA, Diag, call->M, call->N, (void*)&call->alpha.z,
                    A, lda, B, ldb);
        break;
    case (RsBlas_ztrsm):
        initABC(ain, sizeof(double)*2, &A, &B, nullptr, &lda, &ldb, nullptr);
        cblas_ztrsm(CblasRowMajor, Side, Uplo, TransA, Diag, call->M, call->N, (void*)&call->alpha.z,
                    A, lda, B, ldb);
        break;

    // Level 3 C and Z only
    case (RsBlas_chemm):
        initABC(ain, sizeof(float)*2, &A, &B, &C, &lda, &ldb, &ldc);
        cblas_chemm(CblasRowMajor, Side, Uplo, call->M, call->N, (void*)&call->alpha.c, A, lda,
                    B, ldb, (void*)&call->beta.c, C, ldc);
        break;
    case (RsBlas_cherk):
        initABC(ain, sizeof(float)*2, &A, nullptr, &C, &lda, nullptr, &ldc);
        cblas_cherk(CblasRowMajor, Uplo, TransA, call->N, call->K, call->alpha.f, A, lda,
                    call->beta.f, C, ldc);
        break;
    case (RsBlas_cher2k):
        initABC(ain, sizeof(float)*2, &A, &B, &C, &lda, &ldb, &ldc);
        cblas_cher2k(CblasRowMajor, Uplo, TransA, call->N, call->K, (void*)&call->alpha.c, A, lda,
                     B, ldb, call->beta.f, C, ldc);
        break;

    case (RsBlas_zhemm):
        initABC(ain, sizeof(double)*2, &A, &B, &C, &lda, &ldb, &ldc);
        cblas_zhemm(CblasRowMajor, Side, Uplo, call->M, call->N, (void*)&call->alpha.z, A, lda,
                    B, ldb, (void*)&call->beta.z, C, ldc);
        break;
    case (RsBlas_zherk):
        initABC(ain, sizeof(double)*2, &A, nullptr, &C, &lda, nullptr, &ldc);
        cblas_zherk(CblasRowMajor, Uplo, TransA, call->N, call->K, call->alpha.d, A, lda,
                    call->beta.d, C, ldc);
        break;
    case (RsBlas_zher2k):
        initABC(ain, sizeof(double)*2, &A, &B, &C, &lda, &ldb, &ldc);
        cblas_zher2k(CblasRowMajor, Uplo, TransA, call->N, call->K, (void*)&call->alpha.z, A, lda,
                     B, ldb, call->beta.d, C, ldc);
        break;


    case (RsBlas_bnnm):
        initABC(ain, sizeof(uint8_t), &A, &B, &C, &lda, &ldb, &ldc);
        kernelBNNM(call->M, call->N, call->K,
                    (const uint8_t*)A, call->a_offset, lda,
                    (const uint8_t*)B, call->b_offset, ldb,
                    (uint8_t*)C, call->c_offset, ldc,
                    call->c_mult_int);

        break;

    default:
        ALOGE("unimplemented\n");
    }


}

void RsdCpuScriptIntrinsicBLAS::kernelBNNM(size_t m, size_t n, size_t k,
                                           const uint8_t* a, uint8_t a_offset, size_t lda,
                                           const uint8_t* b, uint8_t b_offset, size_t ldb,
                                           uint8_t* c, int32_t c_offset, size_t ldc,
                                           int32_t c_mult_int) {
    const int c_shift = 21;
#if defined(ARCH_ARM_HAVE_VFP) || defined(ARCH_ARM_USE_INTRINSICS)
    // Non-optimized path for ARMv7 devices without SIMD instructions.
    if (!gArchUseSIMD) {
        /*
         * Calculations are done in 1.10.21 fixed-point format for the final output,
         * just before there's a shift down to drop the fractional parts. The output
         * values are gated to 0 to 255 to fit in a byte, but the 10-bit format
         * gives some headroom to avoid wrapping around on small overflows.
         */
        size_t i = 0, j = 0, l = 0;
        for (j = 0; j < n; j++) {
            for (i = 0; i < m; i++) {
                int32_t total = 0;
                for (l = 0; l < k; l++) {
                    const int a_index = ((i * lda) + l);
                    const uint8_t a_as_byte = a[a_index];
                    const int32_t a_as_int = (((int32_t)(a_as_byte)) - a_offset);
                    const int b_index = ((j * ldb) + l);
                    const uint8_t b_as_byte = b[b_index];
                    const int32_t b_as_int = (((int32_t)(b_as_byte)) - b_offset);
                    const int32_t mult_as_int = (a_as_int * b_as_int);
                    total += mult_as_int;
                }
                const int c_index = ((ldc * i) + j);
                int32_t output =
                    ((((total + c_offset) * c_mult_int) + (1 << (c_shift - 1)))
                     >> c_shift);
                if (output > 255) {
                    output = 255;
                }
                if (output < 0) {
                    output = 0;
                }
                c[c_index] = (uint8_t)(output);
            }
        }
        return;
    }
#endif

    // Using gemmlowp to calculate the low precision 8 bit GEMM.
    bool transpose_a = true;
    bool transpose_b = false;
    bool transpose_c = true;
    gemmlowp::eight_bit_int_gemm::EightBitIntGemm(transpose_a, transpose_b, transpose_c,
                                                  m, n, k, a, -a_offset, lda,
                                                  b, -b_offset, ldb, c, c_offset,
                                                  c_mult_int, c_shift, ldc,
                                                  gemmlowp::eight_bit_int_gemm::BitDepthSetting::A8B8);

}





RsdCpuScriptIntrinsicBLAS::RsdCpuScriptIntrinsicBLAS(RsdCpuReferenceImpl *ctx,
                                                   const Script *s)
            : RsdCpuScriptIntrinsic(ctx, s, nullptr, RS_SCRIPT_INTRINSIC_ID_BLAS) {


}

RsdCpuScriptIntrinsicBLAS::~RsdCpuScriptIntrinsicBLAS() {
}





RsdCpuScriptImpl * rsdIntrinsic_BLAS(RsdCpuReferenceImpl *ctx,
                                    const Script *s, const Element *e) {

    return new RsdCpuScriptIntrinsicBLAS(ctx, s);
}
