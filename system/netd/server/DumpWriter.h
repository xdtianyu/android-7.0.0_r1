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

#ifndef NETD_SERVER_DUMPWRITER_H_
#define NETD_SERVER_DUMPWRITER_H_

#include <string>
#include <utils/String16.h>
#include <utils/Vector.h>


class DumpWriter {
public:
    DumpWriter(int fd);

    void incIndent();
    void decIndent();

    void println(const std::string& line);
    void println(const char* fmt, ...);
    void blankline() { println(""); }

private:
    uint8_t mIndentLevel;
    int mFd;
};

#endif  // NETD_SERVER_DUMPWRITER_H_
