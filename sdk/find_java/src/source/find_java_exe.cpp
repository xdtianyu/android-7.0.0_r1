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

/*
 * "find_java.exe", for Windows only.
 * Tries to find a Java binary in a variety of places and prints the
 * first one found on STDOUT and returns 0.
 *
 * If not found, returns error 1 with no message
 * (unless ANDROID_SDKMAN_DEBUG or -d if set, in which case there's a message on STDERR).
 *
 * Implementation details:
 * - We don't have access to ATL or MFC.
 * - We don't want to pull in things like STL.
 * - No Unicode/MBCS support for now.
 *
 * TODO for later version:
 * - provide an env variable to let users override which version is being used.
 * - if there's more than one java.exe found, enumerate them all.
 * - and in that case take the one with the highest Java version number.
 * - since that operation is expensive, do it only once and cache the result
 *   in a temp file. If the temp file is not found or the java binary no
 *   longer exists, re-run the enumaration.
 */

#ifdef _WIN32

#include "utils.h"
#include "find_java.h"
#include <io.h>
#include <fcntl.h>

static int showHelpMessage() {
    printf(
        "Outputs the path of the first Java.exe found on the local system.\n"
        "Returns code 0 when found, 1 when not found.\n"
        "Options:\n"
        "-h / -help       : This help.\n"
        "-t / -test       : Internal test.\n"
        "-e / -error      : Print an error message to the console if Java.exe isn't found.\n"
        "-j / -jdk        : Only returns java.exe found in a JDK.\n"
        "-s / -short      : Print path in short DOS form.\n"
        "-p / -path `dir` : A custom path to search first. Pass in JDK base dir if -j is set.\n"
        "-w / -javaw      : Search a matching javaw.exe; defaults to java.exe if not found.\n"
        "-m / -minv #     : Pass in a minimum version to use (default: 1.6).\n"
        "-v / -version: Only prints the Java version found.\n"
    );
    return 2;
}

static void printError(const char *message) {

    CString error;
    error.setLastWin32Error(message);
    printf(error.cstr());
}

static void testFindJava(bool isJdk, int minVersion) {

    CPath javaPath("<not found>");
    int v = findJavaInEnvPath(&javaPath, isJdk, minVersion);
    printf("  findJavaInEnvPath: [%d] %s\n", v, javaPath.cstr());

    javaPath.set("<not found>");
    v = findJavaInRegistry(&javaPath, isJdk, minVersion);
    printf("  findJavaInRegistry [%d] %s\n", v, javaPath.cstr());

    javaPath.set("<not found>");
    v = findJavaInProgramFiles(&javaPath, isJdk, minVersion);
    printf("  findJavaInProgramFiles [%d] %s\n", v, javaPath.cstr());
}

static void testFindJava(int minVersion) {

    printf("Searching for version %d.%d or newer...\n", JAVA_MAJOR(minVersion),
        JAVA_MINOR(minVersion));

    printf("\n");
    printf("Searching for any java.exe:\n");
    testFindJava(false, minVersion);

    printf("\n");
    printf("Searching for java.exe within a JDK:\n");
    testFindJava(true, minVersion);
}

// Returns 0 on failure or a java version on success.
int parseMinVersionArg(char* arg) {

    int versionMajor = -1;
    int versionMinor = -1;
    if (sscanf(arg, "%d.%d", &versionMajor, &versionMinor) != 2) {
        // -m arg is malformatted
        return 0;
    }
    return TO_JAVA_VERSION(versionMajor, versionMinor);
}

int main(int argc, char* argv[]) {

    gIsConsole = true; // tell utils to to print errors to stderr
    gIsDebug = (getenv("ANDROID_SDKMAN_DEBUG") != NULL);
    bool doShortPath = false;
    bool doVersion = false;
    bool doJavaW = false;
    bool isJdk = false;
    bool shouldPrintError = false;
    int minVersion = MIN_JAVA_VERSION;
    const char *customPathStr = NULL;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-t", 2) == 0) {
            testFindJava(minVersion);
            return 0;

        } else if (strncmp(argv[i], "-j", 2) == 0) {
            isJdk = true;

        } else if (strncmp(argv[i], "-e", 2) == 0) {
            shouldPrintError = true;

        } else if (strncmp(argv[i], "-p", 2) == 0) {
            i++;
            if (i == argc) {
                return showHelpMessage();
            }
            customPathStr = argv[i];
        } else if (strncmp(argv[i], "-d", 2) == 0) {
            gIsDebug = true;

        } else if (strncmp(argv[i], "-s", 2) == 0) {
            doShortPath = true;

        } else if (strncmp(argv[i], "-v", 2) == 0) {
            doVersion = true;

        } else if (strncmp(argv[i], "-m", 2) == 0) {
            i++;
            if (i == argc ||
                ((minVersion = parseMinVersionArg(argv[i])) == 0)) {
                return showHelpMessage();
            }
        }
        else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "-javaw") == 0) {
            doJavaW = true;

        }
        else {
            return showHelpMessage();
        }
    }

    // Find the first suitable version of Java we can use.
    CPath javaPath;

    int version = 0;
    if (customPathStr != NULL) {
        CPath customPath(customPathStr);
        version = findJavaInPath(customPath, &javaPath, isJdk, minVersion);
    }
    if (version == 0) {
        version = findJavaInEnvPath(&javaPath, isJdk, minVersion);
    }
    if (version == 0) {
        version = findJavaInRegistry(&javaPath, isJdk, minVersion);
    }
    if (version == 0) {
        version = findJavaInProgramFiles(&javaPath, isJdk, minVersion);
    }

    if (version == 0) {
        CString s;
        s.setf("Failed to find Java %d.%d (or newer) on your system. ", JAVA_MAJOR(minVersion),
            JAVA_MINOR(minVersion));

        if (gIsDebug) {
            fprintf(stderr, s.cstr());
        }

        if (shouldPrintError) {
            printError(s.cstr());
        }

        return 1;
    }
    _ASSERT(!javaPath.isEmpty());

    if (doShortPath) {
        if (!javaPath.toShortPath(&javaPath)) {
            CString s;
            s.setf("Failed to convert path (%s) to a short DOS path. ", javaPath.cstr());
            fprintf(stderr, s.cstr());

            if (shouldPrintError) {
                printError(s.cstr());
            }

            return 1;
        }
    }

    if (doVersion) {
        // Print version found. We already have the version as an integer
        // so we don't need to run java -version a second time.
        printf("%d.%d", JAVA_MAJOR(version), JAVA_MINOR(version));
        return 0;
    }

    if (doJavaW) {
        // Try to find a javaw.exe instead of java.exe at the same location.
        CPath javawPath(javaPath);
        javawPath.replaceName("java.exe", "javaw.exe");
        // Only accept it if we can actually find the exec
        if (javawPath.fileExists()) {
            javaPath.set(javawPath.cstr());
        }
    }

    // Print java.exe path found
    printf(javaPath.cstr());
    return 0;
}

#endif /* _WIN32 */
