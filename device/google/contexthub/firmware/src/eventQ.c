/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <platform.h>
#include <eventQ.h>
#include <stddef.h>
#include <timer.h>
#include <stdio.h>
#include <heap.h>
#include <slab.h>
#include <cpu.h>
#include <util.h>
#include <plat/inc/plat.h>


struct EvtRecord {
    struct EvtRecord *next;
    struct EvtRecord *prev;
    uint32_t evtType;
    void* evtData;
    uintptr_t evtFreeData;
};

struct EvtQueue {
    struct EvtRecord *head;
    struct EvtRecord *tail;
    struct SlabAllocator *evtsSlab;
    EvtQueueForciblyDiscardEvtCbkF forceDiscardCbk;
};



struct EvtQueue* evtQueueAlloc(uint32_t size, EvtQueueForciblyDiscardEvtCbkF forceDiscardCbk)
{
    struct EvtQueue *q = heapAlloc(sizeof(struct EvtQueue));
    struct SlabAllocator *slab = slabAllocatorNew(sizeof(struct EvtRecord), alignof(struct EvtRecord), size);

    if (q && slab) {
        q->forceDiscardCbk = forceDiscardCbk;
        q->evtsSlab = slab;
        q->head = NULL;
        q->tail = NULL;
        return q;
    }

    if (q)
        heapFree(q);
    if (slab)
        slabAllocatorDestroy(slab);

    return NULL;
}

void evtQueueFree(struct EvtQueue* q)
{
    struct EvtRecord *t;

    while (q->head) {
        t = q->head;
        q->head = q->head->next;
        q->forceDiscardCbk(t->evtType, t->evtData, t->evtFreeData);
        slabAllocatorFree(q->evtsSlab, t);
    }

    slabAllocatorDestroy(q->evtsSlab);
    heapFree(q);
}

bool evtQueueEnqueue(struct EvtQueue* q, uint32_t evtType, void *evtData, uintptr_t evtFreeData, bool atFront)
{
    struct EvtRecord *rec;
    uint64_t intSta;

    if (!q)
        return false;

    rec = slabAllocatorAlloc(q->evtsSlab);
    if (!rec) {
        intSta = cpuIntsOff();

        //find a victim for discarding
        rec = q->head;
        while (rec && !(rec->evtType & EVENT_TYPE_BIT_DISCARDABLE))
            rec = rec->next;

        if (rec) {
            q->forceDiscardCbk(rec->evtType, rec->evtData, rec->evtFreeData);
            if (rec->prev)
                rec->prev->next = rec->next;
            else
                q->head = rec->next;
            if (rec->next)
                rec->next->prev = rec->prev;
            else
                q->tail = rec->prev;
        }

        cpuIntsRestore (intSta);
        if (!rec)
           return false;
    }

    rec->next = NULL;
    rec->evtType = evtType;
    rec->evtData = evtData;
    rec->evtFreeData = evtFreeData;

    intSta = cpuIntsOff();

    if (atFront) { /* this is almost always not the case */
        rec->prev = NULL;
        rec->next = q->head;
        q->head = rec;
        if (q->tail)
            rec->next->prev = rec;
        else
            q->tail = rec;
    }
    else { /* the common case */
        rec->prev = q->tail;
        q->tail = rec;
        if (q->head)
            rec->prev->next = rec;
        else
            q->head = rec;
    }

    cpuIntsRestore(intSta);
    platWake();
    return true;
}

bool evtQueueDequeue(struct EvtQueue* q, uint32_t *evtTypeP, void **evtDataP, uintptr_t *evtFreeDataP, bool sleepIfNone)
{
    struct EvtRecord *rec = NULL;
    uint64_t intSta;

    while(1) {
        intSta = cpuIntsOff();

        rec = q->head;
        if (rec) {
            q->head = rec->next;
            if (q->head)
                q->head->prev = NULL;
            else
                q->tail = NULL;
            break;
        }
        else if (!sleepIfNone)
            break;
        else if (!timIntHandler()) { // check for timers. if any fire, do not sleep (since by the time callbacks run, moremight be due)
            platSleep();     //sleep
            timIntHandler(); //first thing when awake: check timers
        }
        cpuIntsRestore(intSta);
    }

    cpuIntsRestore(intSta);

    if (!rec)
        return false;

    *evtTypeP = rec->evtType;
    *evtDataP = rec->evtData;
    *evtFreeDataP = rec->evtFreeData;
    slabAllocatorFree(q->evtsSlab, rec);

    return true;
}

