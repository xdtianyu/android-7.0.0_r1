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


#ifndef _MESSAGE_BUF_H_
#define _MESSAGE_BUF_H_

#include <endian.h>
#include <cstring>

namespace android {

namespace nanohub {

/*
 * Marshaling helper;
 * deals with alignment and endianness.
 * Assumption is:
 * read*()  parse buffer from device in LE format;
 *          return host endianness, aligned data
 * write*() primitives take host endinnness, aligned data,
 *          generate buffer to be passed to device in LE format
 *
 * Primitives do minimal error checking, only to ensure buffer read/write
 * safety. Caller is responsible for making sure correct amount of data
 * has been processed.
 */
class MessageBuf {
    char *data;
    size_t size;
    size_t pos;
    bool readOnly;
public:
    MessageBuf(char *buf, size_t bufSize) {
        size = bufSize;
        pos = 0;
        data = buf;
        readOnly = false;
    }
    MessageBuf(const char *buf, size_t bufSize) {
        size = bufSize;
        pos = 0;
        data = const_cast<char *>(buf);
        readOnly = true;
    }
    const char *getData() const { return data; }
    size_t getSize() const { return size; }
    size_t getPos() const { return pos; }
    size_t getRoom() const { return size - pos; }
    uint8_t readU8() {
        if (pos == size) {
            return 0;
        }
        return data[pos++];
    }
    void writeU8(uint8_t val) {
        if (pos == size || readOnly)
            return;
        data[pos++] = val;
    }
    uint16_t readU16() {
        if (pos > (size - sizeof(uint16_t))) {
            return 0;
        }
        uint16_t val;
        memcpy(&val, &data[pos], sizeof(val));
        pos += sizeof(val);
        return le16toh(val);
    }
    void writeU16(uint16_t val) {
        if (pos > (size - sizeof(uint16_t)) || readOnly) {
            return;
        }
        uint16_t tmp = htole16(val);
        memcpy(&data[pos], &tmp, sizeof(tmp));
        pos += sizeof(tmp);
    }
    uint32_t readU32() {
        if (pos > (size - sizeof(uint32_t))) {
            return 0;
        }
        uint32_t val;
        memcpy(&val, &data[pos], sizeof(val));
        pos += sizeof(val);
        return le32toh(val);
    }
    void writeU32(uint32_t val) {
        if (pos > (size - sizeof(uint32_t)) || readOnly) {
            return;
        }
        uint32_t tmp = htole32(val);
        memcpy(&data[pos], &tmp, sizeof(tmp));
        pos += sizeof(tmp);
    }
    uint64_t readU64() {
        if (pos > (size - sizeof(uint64_t))) {
            return 0;
        }
        uint64_t val;
        memcpy(&val, &data[pos], sizeof(val));
        pos += sizeof(val);
        return le32toh(val);
    }
    void writeU64(uint64_t val) {
        if (pos > (size - sizeof(uint64_t)) || readOnly) {
            return;
        }
        uint64_t tmp = htole64(val);
        memcpy(&data[pos], &tmp, sizeof(tmp));
        pos += sizeof(tmp);
    }
    const void *readRaw(size_t bufSize) {
        if (pos > (size - bufSize)) {
            return nullptr;
        }
        const void *buf = &data[pos];
        pos += bufSize;
        return buf;
    }
    void writeRaw(const void *buf, size_t bufSize) {
        if (pos > (size - bufSize) || readOnly) {
            return;
        }
        memcpy(&data[pos], buf, bufSize);
        pos += bufSize;
    }
};

}; // namespace nanohub

}; // namespace android

#endif

