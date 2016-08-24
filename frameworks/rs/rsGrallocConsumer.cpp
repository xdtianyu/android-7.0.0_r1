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

#define ATRACE_TAG ATRACE_TAG_RS

#include "rsContext.h"
#include "rsAllocation.h"
#include "rs_hal.h"

#include <cutils/compiler.h>
#include <utils/Log.h>
#include "rsGrallocConsumer.h"
#include <gui/BufferItem.h>
#include <ui/GraphicBuffer.h>


namespace android {
namespace renderscript {

GrallocConsumer::GrallocConsumer(Allocation *a, const sp<IGraphicBufferConsumer>& bq, int flags, uint32_t numAlloc) :
    ConsumerBase(bq, true)
{
    mAlloc = new Allocation *[numAlloc];
    mAcquiredBuffer = new AcquiredBuffer[numAlloc];
    isIdxUsed = new bool[numAlloc];

    mAlloc[0] = a;
    isIdxUsed[0] = true;
    mNumAlloc = numAlloc;
    if (flags == 0) {
        flags = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_RENDERSCRIPT;
    } else {
        flags |= GRALLOC_USAGE_RENDERSCRIPT;
    }
    mConsumer->setConsumerUsageBits(flags);
    mConsumer->setMaxAcquiredBufferCount(numAlloc + 1);

    uint32_t y = a->mHal.drvState.lod[0].dimY;
    if (y < 1) y = 1;
    mConsumer->setDefaultBufferSize(a->mHal.drvState.lod[0].dimX, y);

    if (a->mHal.state.yuv) {
        bq->setDefaultBufferFormat(a->mHal.state.yuv);
    }
    for (uint32_t i = 1; i < numAlloc; i++) {
        isIdxUsed[i] = false;
    }
    //mBufferQueue->setConsumerName(name);
}

GrallocConsumer::~GrallocConsumer() {
    delete[] mAlloc;
    delete[] mAcquiredBuffer;
    delete[] isIdxUsed;
}



status_t GrallocConsumer::lockNextBuffer(uint32_t idx) {
    Mutex::Autolock _l(mMutex);
    status_t err;

    if (idx >= mNumAlloc) {
        ALOGE("Invalid buffer index: %d", idx);
        return BAD_VALUE;
    }

    if (mAcquiredBuffer[idx].mSlot != BufferQueue::INVALID_BUFFER_SLOT) {
        err = releaseAcquiredBufferLocked(idx);
        if (err) {
            return err;
        }
    }

    BufferItem b;

    err = acquireBufferLocked(&b, 0);
    if (err != OK) {
        if (err == BufferQueue::NO_BUFFER_AVAILABLE) {
            return BAD_VALUE;
        } else {
            ALOGE("Error acquiring buffer: %s (%d)", strerror(err), err);
            return err;
        }
    }

    int slot = b.mSlot;

    if (b.mFence.get()) {
        err = b.mFence->waitForever("GrallocConsumer::lockNextBuffer");
        if (err != OK) {
            ALOGE("Failed to wait for fence of acquired buffer: %s (%d)",
                    strerror(-err), err);
            return err;
        }
    }

    void *bufferPointer = nullptr;
    android_ycbcr ycbcr = android_ycbcr();

    if (mSlots[slot].mGraphicBuffer->getPixelFormat() ==
            HAL_PIXEL_FORMAT_YCbCr_420_888) {
        err = mSlots[slot].mGraphicBuffer->lockYCbCr(
            GraphicBuffer::USAGE_SW_READ_OFTEN,
            b.mCrop,
            &ycbcr);

        if (err != OK) {
            ALOGE("Unable to lock YCbCr buffer for CPU reading: %s (%d)",
                    strerror(-err), err);
            return err;
        }
        bufferPointer = ycbcr.y;
    } else {
        err = mSlots[slot].mGraphicBuffer->lock(
            GraphicBuffer::USAGE_SW_READ_OFTEN,
            b.mCrop,
            &bufferPointer);

        if (err != OK) {
            ALOGE("Unable to lock buffer for CPU reading: %s (%d)",
                    strerror(-err), err);
            return err;
        }
    }

    size_t lockedIdx = 0;
    rsAssert(mAcquiredBuffer[idx].mSlot == BufferQueue::INVALID_BUFFER_SLOT);

    mAcquiredBuffer[idx].mSlot = slot;
    mAcquiredBuffer[idx].mBufferPointer = bufferPointer;
    mAcquiredBuffer[idx].mGraphicBuffer = mSlots[slot].mGraphicBuffer;

    mAlloc[idx]->mHal.drvState.lod[0].mallocPtr = reinterpret_cast<uint8_t*>(bufferPointer);
    mAlloc[idx]->mHal.drvState.lod[0].stride = mSlots[slot].mGraphicBuffer->getStride() *
            mAlloc[idx]->mHal.state.type->getElementSizeBytes();
    mAlloc[idx]->mHal.state.nativeBuffer = mAcquiredBuffer[idx].mGraphicBuffer->getNativeBuffer();
    mAlloc[idx]->mHal.state.timestamp = b.mTimestamp;

    rsAssert(mAlloc[idx]->mHal.drvState.lod[0].dimX ==
             mSlots[slot].mGraphicBuffer->getWidth());
    rsAssert(mAlloc[idx]->mHal.drvState.lod[0].dimY ==
             mSlots[slot].mGraphicBuffer->getHeight());

    //mAlloc->format = mSlots[buf].mGraphicBuffer->getPixelFormat();

    //mAlloc->crop        = b.mCrop;
    //mAlloc->transform   = b.mTransform;
    //mAlloc->scalingMode = b.mScalingMode;
    //mAlloc->frameNumber = b.mFrameNumber;

    // For YUV Allocations, we need to populate the drvState with details of how
    // the data is layed out.
    // RenderScript requests a buffer in the YCbCr_420_888 format.
    // The Camera HAL can return a buffer of YCbCr_420_888 or YV12, regardless
    // of the requested format.
    // mHal.state.yuv contains the requested format,
    // mGraphicBuffer->getPixelFormat() is the returned format.
    if (mAlloc[idx]->mHal.state.yuv == HAL_PIXEL_FORMAT_YCbCr_420_888) {
        const int yWidth = mAlloc[idx]->mHal.drvState.lod[0].dimX;
        const int yHeight = mAlloc[idx]->mHal.drvState.lod[0].dimY;

        if (mSlots[slot].mGraphicBuffer->getPixelFormat() ==
                HAL_PIXEL_FORMAT_YCbCr_420_888) {
            const int cWidth = yWidth / 2;
            const int cHeight = yHeight / 2;

            mAlloc[idx]->mHal.drvState.lod[1].dimX = cWidth;
            mAlloc[idx]->mHal.drvState.lod[1].dimY = cHeight;
            mAlloc[idx]->mHal.drvState.lod[2].dimX = cWidth;
            mAlloc[idx]->mHal.drvState.lod[2].dimY = cHeight;

            mAlloc[idx]->mHal.drvState.lod[0].mallocPtr = ycbcr.y;
            mAlloc[idx]->mHal.drvState.lod[1].mallocPtr = ycbcr.cb;
            mAlloc[idx]->mHal.drvState.lod[2].mallocPtr = ycbcr.cr;

            mAlloc[idx]->mHal.drvState.lod[0].stride = ycbcr.ystride;
            mAlloc[idx]->mHal.drvState.lod[1].stride = ycbcr.cstride;
            mAlloc[idx]->mHal.drvState.lod[2].stride = ycbcr.cstride;

            mAlloc[idx]->mHal.drvState.yuv.shift = 1;
            mAlloc[idx]->mHal.drvState.yuv.step = ycbcr.chroma_step;
            mAlloc[idx]->mHal.drvState.lodCount = 3;
        } else if (mSlots[slot].mGraphicBuffer->getPixelFormat() ==
                       HAL_PIXEL_FORMAT_YV12) {
            // For YV12, the data layout is Y, followed by Cr, followed by Cb;
            // for YCbCr_420_888, it's Y, followed by Cb, followed by Cr.
            // RenderScript assumes lod[0] is Y, lod[1] is Cb, and lod[2] is Cr.
            const int cWidth = yWidth / 2;
            const int cHeight = yHeight / 2;

            mAlloc[idx]->mHal.drvState.lod[1].dimX = cWidth;
            mAlloc[idx]->mHal.drvState.lod[1].dimY = cHeight;
            mAlloc[idx]->mHal.drvState.lod[2].dimX = cWidth;
            mAlloc[idx]->mHal.drvState.lod[2].dimY = cHeight;

            size_t yStride = rsRound(yWidth *
                 mAlloc[idx]->mHal.state.type->getElementSizeBytes(), 16);
            size_t cStride = rsRound(yStride >> 1, 16);

            uint8_t *yPtr = (uint8_t *)mAlloc[idx]->mHal.drvState.lod[0].mallocPtr;
            uint8_t *crPtr = yPtr + yStride * yHeight;
            uint8_t *cbPtr = crPtr + cStride * cHeight;

            mAlloc[idx]->mHal.drvState.lod[1].mallocPtr = cbPtr;
            mAlloc[idx]->mHal.drvState.lod[2].mallocPtr = crPtr;

            mAlloc[idx]->mHal.drvState.lod[0].stride = yStride;
            mAlloc[idx]->mHal.drvState.lod[1].stride = cStride;
            mAlloc[idx]->mHal.drvState.lod[2].stride = cStride;

            mAlloc[idx]->mHal.drvState.yuv.shift = 1;
            mAlloc[idx]->mHal.drvState.yuv.step = 1;
            mAlloc[idx]->mHal.drvState.lodCount = 3;
        } else {
            ALOGD("Unrecognized format: %d",
               mSlots[slot].mGraphicBuffer->getPixelFormat());
        }
    }

    return OK;
}

status_t GrallocConsumer::unlockBuffer(uint32_t idx) {
    Mutex::Autolock _l(mMutex);
    return releaseAcquiredBufferLocked(idx);
}

status_t GrallocConsumer::releaseAcquiredBufferLocked(uint32_t idx) {
    status_t err;

    if (idx >= mNumAlloc) {
        ALOGE("Invalid buffer index: %d", idx);
        return BAD_VALUE;
    }
    if (mAcquiredBuffer[idx].mGraphicBuffer == nullptr) {
       return OK;
    }

    err = mAcquiredBuffer[idx].mGraphicBuffer->unlock();
    if (err != OK) {
        ALOGE("%s: Unable to unlock graphic buffer", __FUNCTION__);
        return err;
    }
    int buf = mAcquiredBuffer[idx].mSlot;

    // release the buffer if it hasn't already been freed by the BufferQueue.
    // This can happen, for example, when the producer of this buffer
    // disconnected after this buffer was acquired.
    if (CC_LIKELY(mAcquiredBuffer[idx].mGraphicBuffer ==
            mSlots[buf].mGraphicBuffer)) {
        releaseBufferLocked(
                buf, mAcquiredBuffer[idx].mGraphicBuffer,
                EGL_NO_DISPLAY, EGL_NO_SYNC_KHR);
    }

    mAcquiredBuffer[idx].mSlot = BufferQueue::INVALID_BUFFER_SLOT;
    mAcquiredBuffer[idx].mBufferPointer = nullptr;
    mAcquiredBuffer[idx].mGraphicBuffer.clear();
    return OK;
}

uint32_t GrallocConsumer::getNextAvailableIdx(Allocation *a) {
    for (uint32_t i = 0; i < mNumAlloc; i++) {
        if (isIdxUsed[i] == false) {
            mAlloc[i] = a;
            isIdxUsed[i] = true;
            return i;
        }
    }
    return mNumAlloc;
}

bool GrallocConsumer::releaseIdx(uint32_t idx) {
    if (idx >= mNumAlloc) {
        ALOGE("Invalid buffer index: %d", idx);
        return false;
    }
    if (isIdxUsed[idx] == false) {
        ALOGV("Buffer index already released: %d", idx);
        return true;
    }
    status_t err;
    err = unlockBuffer(idx);
    if (err != OK) {
        ALOGE("Unable to unlock graphic buffer");
        return false;
    }
    mAlloc[idx] = nullptr;
    isIdxUsed[idx] = false;
    return true;
}

} // namespace renderscript
} // namespace android
