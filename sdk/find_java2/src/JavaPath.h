/*
* Copyright (C) 2014 The Android Open Source Project
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

#pragma once

#include <atlpath.h>                            // ATL CPath


// Transforms a Java major.minor number (e.g. "1.7") to internal int value (1007)
#define JAVA_VERS_TO_INT(major, minor)  ((major) * 1000 + (minor))
// Extracts the major part from the internal int major.minor number
#define JAVA_MAJOR(majorMinor)          ((majorMinor) / 1000)
// Extracts the minor part from the internal int major.minor number
#define JAVA_MINOR(majorMinor)          ((majorMinor) % 1000)


struct CJavaPath {
    int mVersion;
    CPath mPath;

    // Static empty path that can be returned as a reference.
    static const CJavaPath sEmpty;

    CJavaPath() : mVersion(0) {}
    CJavaPath(int version, CPath path);
    void set(int version, CPath path);

    // Returns true if path/version is empty/0
    bool isEmpty() const;

    // Clears path and version to 0
    void clear();

    // Converts the internal path into a short DOS path.
    // Returns true if this was possible and false if the conversion failed.
    bool toShortPath();

    // Returns the version formatted as a string (e.g. "1.7" instead of 1007.)
    CString getVersion() const;

    // Operators < and == for this to be suitable in an ordered std::set
    bool operator<  (const CJavaPath& rhs) const;
    bool operator== (const CJavaPath& rhs) const;
};
