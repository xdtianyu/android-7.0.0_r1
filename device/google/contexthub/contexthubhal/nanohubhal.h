/*
 * Copyright (c) 2016, Google. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _NANOHUB_HAL_H_
#define _NANOHUB_HAL_H_

#include <pthread.h>

#include <hardware/context_hub.h>
#include <utils/Mutex.h>

#define NANOAPP_VENDOR_GOOGLE NANOAPP_VENDOR("Googl")

//as per protocol
#define MAX_RX_PACKET           128
#define APP_FROM_HOST_EVENT_ID  0x000000F8

namespace android {

namespace nanohub {

void dumpBuffer(const char *pfx, const hub_app_name_t &appId, uint32_t evtId, const void *data, size_t len, int status = 0);

struct nano_message_hdr {
    uint32_t event_id;
    hub_app_name_t app_name;
    uint8_t len;
} __attribute__((packed));

struct nano_message {
    nano_message_hdr hdr;
    uint8_t data[MAX_RX_PACKET];
} __attribute__((packed));

class NanoHub {
    Mutex mLock;
    context_hub_callback *mMsgCbkFunc;
    int mThreadClosingPipe[2];
    int mFd; // [0] is read end
    void * mMsgCbkData;
    pthread_t mWorkerThread;

    NanoHub() {
        reset();
    }

    void reset() {
        mThreadClosingPipe[0] = -1;
        mThreadClosingPipe[1] = -1;
        mFd = -1;
        mMsgCbkData = nullptr;
        mMsgCbkFunc = nullptr;
        mWorkerThread = 0;
    }

    static void* run(void *);
    void* doRun();

    int openHub();
    int closeHub();

    static NanoHub *hubInstance() {
        static NanoHub theHub;
        return &theHub;
    }

    int doSubscribeMessages(uint32_t hub_id, context_hub_callback *cbk, void *cookie);
    int doSendToNanohub(uint32_t hub_id, const hub_message_t *msg);
    int doSendToDevice(const hub_app_name_t *name, const void *data, uint32_t len);
    void doSendToApp(const hub_app_name_t *name, uint32_t typ, const void *data, uint32_t len);

    static constexpr unsigned int FL_MESSAGE_TRACING = 1;

    unsigned int mFlags = 0;

public:

    // debugging interface

    static bool messageTracingEnabled() {
        return hubInstance()->mFlags & FL_MESSAGE_TRACING;
    }
    static unsigned int getDebugFlags() {
        return hubInstance()->mFlags;
    }
    static void setDebugFlags(unsigned int flags) {
        hubInstance()->mFlags = flags;
    }

    // messaging interface

    // define callback to invoke for APP messages
    static int subscribeMessages(uint32_t hub_id, context_hub_callback *cbk, void *cookie) {
        return hubInstance()->doSubscribeMessages(hub_id, cbk, cookie);
    }
    // all messages from APP go here
    static int sendToNanohub(uint32_t hub_id, const hub_message_t *msg) {
        return hubInstance()->doSendToNanohub(hub_id, msg);
    }
    // passes message to kernel driver directly
    static int sendToDevice(const hub_app_name_t *name, const void *data, uint32_t len) {
        return hubInstance()->doSendToDevice(name, data, len);
    }
    // passes message to APP via callback
    static void sendToApp(const hub_app_name_t *name, uint32_t typ, const void *data, uint32_t len) {
        hubInstance()->doSendToApp(name, typ, data, len);
    }
};

}; // namespace nanohub

}; // namespace android

#endif
