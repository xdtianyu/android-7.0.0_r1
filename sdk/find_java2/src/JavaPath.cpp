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

#include "stdafx.h"
#include "JavaPath.h"
#include "utils.h"

#define  _CRT_SECURE_NO_WARNINGS

// --------------

const CJavaPath CJavaPath::sEmpty = CJavaPath();

CJavaPath::CJavaPath(int version, CPath path) : mVersion(version), mPath(path) {
    mPath.Canonicalize();
}

bool CJavaPath::isEmpty() const {
    return mVersion <= 0;
}

void CJavaPath::clear() {
    mVersion = 0;
    mPath = CPath();
}

void CJavaPath::set(int version, CPath path) {
    mVersion = version;
    mPath = path;
    mPath.Canonicalize();
}

CString CJavaPath::getVersion() const {
    CString s;
    s.Format(_T("%d.%d"), JAVA_MAJOR(mVersion), JAVA_MINOR(mVersion));
    return s;
}


bool CJavaPath::toShortPath() {
    const TCHAR *longPath = mPath;
    if (longPath == nullptr) {
        return false;
    }

    DWORD lenShort = _tcslen(longPath) + 1;
    TCHAR *shortPath = (TCHAR *)malloc(lenShort * sizeof(TCHAR));

    DWORD length = GetShortPathName(longPath, shortPath, lenShort);
    if (length > lenShort) {
        // The buffer wasn't big enough, this is the size to use.
        free(shortPath);
        lenShort = length;
        shortPath = (TCHAR *)malloc(length);
        length = GetShortPathName(longPath, shortPath, lenShort);
    }

    if (length != 0) {
        mPath = CPath(shortPath);
    }

    free(shortPath);
    return length != 0;
}

bool CJavaPath::operator< (const CJavaPath& rhs) const {
    if (mVersion != rhs.mVersion) {
        // sort in reverse order on the version
        return rhs.mVersion > mVersion;
    }
    // sort in normal order on the path
    const CString &pl = mPath;
    const CString &pr = rhs.mPath;
    return pl.Compare(pr) < 0;
}

bool CJavaPath::operator== (const CJavaPath& rhs) const {
    if (mVersion == rhs.mVersion) {
        const CString &pl = mPath;
        const CString &pr = rhs.mPath;
        return pl.Compare(pr) == 0;
    }
    return false;
}
