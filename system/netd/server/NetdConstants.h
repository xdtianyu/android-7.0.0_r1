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

#ifndef _NETD_CONSTANTS_H
#define _NETD_CONSTANTS_H

#include <string>
#include <list>
#include <stdarg.h>

#include <chrono>

#include <private/android_filesystem_config.h>

#include "utils/RWLock.h"

const int PROTECT_MARK = 0x1;
const int MAX_SYSTEM_UID = AID_APP - 1;

extern const char * const IPTABLES_PATH;
extern const char * const IP6TABLES_PATH;
extern const char * const IP_PATH;
extern const char * const TC_PATH;
extern const char * const OEM_SCRIPT_PATH;
extern const char * const ADD;
extern const char * const DEL;

enum IptablesTarget { V4, V6, V4V6 };

int execIptables(IptablesTarget target, ...);
int execIptablesSilently(IptablesTarget target, ...);
int execIptablesRestore(IptablesTarget target, const std::string& commands);
bool isIfaceName(const char *name);
int parsePrefix(const char *prefix, uint8_t *family, void *address, int size, uint8_t *prefixlen);

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

#define __INT_STRLEN(i) sizeof(#i)
#define _INT_STRLEN(i) __INT_STRLEN(i)
#define INT32_STRLEN _INT_STRLEN(INT32_MIN)
#define UINT32_STRLEN _INT_STRLEN(UINT32_MAX)
#define UINT32_HEX_STRLEN sizeof("0x12345678")

#define WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))

const uid_t INVALID_UID = static_cast<uid_t>(-1);

class Stopwatch {
public:
    Stopwatch() : mStart(std::chrono::steady_clock::now()) {}
    virtual ~Stopwatch() {};

    float timeTaken() const {
        using ms = std::chrono::duration<float, std::ratio<1, 1000>>;
        return (std::chrono::duration_cast<ms>(
                std::chrono::steady_clock::now() - mStart)).count();
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> mStart;
};

namespace android {
namespace net {

/**
 * This lock exists to make NetdNativeService RPCs (which come in on multiple Binder threads)
 * coexist with the commands in CommandListener.cpp. These are presumed not thread-safe because
 * CommandListener has only one user (NetworkManagementService), which is connected through a
 * FrameworkListener that passes in commands one at a time.
 */
extern android::RWLock gBigNetdLock;

}  // namespace net
}  // namespace android

#endif  // _NETD_CONSTANTS_H
