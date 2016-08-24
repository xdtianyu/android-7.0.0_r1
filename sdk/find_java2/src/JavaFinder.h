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


#include <set>                                  // STL std::set
#include "JavaPath.h"

class CJavaFinder {
public:
    // Creates a new JavaFinder.
    // minVersion to accept, using JAVA_VERS_TO_INT macro. 0 to accept everything.
    CJavaFinder(int minVersion = 0);
    ~CJavaFinder();

    int getMinVersion() const { return mMinVersion;  }

    // Returns the path recorded in the registry.
    // If there is no path or it is no longer valid, returns an empty string.
    CJavaPath getRegistryPath();

    // Sets the given path as the default to use in the registry.
    // Returns true on success.
    bool setRegistryPath(const CJavaPath &javaPath);

    // Scans the registry, the environment and program files for potential Java.exe locations.
    // Fills the given set with the tuples (version, path) found, guaranteed sorted and unique.
    void findJavaPaths(std::set<CJavaPath> *paths);

    // Checks the given path for a given java.exe.
    // Input path variation tried are: path as-is, path/java.exe or path/bin/java.exe.
    // Places the java path and version in outPath;
    // Returns true if a java path was found *and* its version is at least mMinVersion.
    bool checkJavaPath(const CString &path, CJavaPath *outPath);

private:
    int mMinVersion;
};
