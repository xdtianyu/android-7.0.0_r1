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
#include "FindJava2.h"
#include "utils.h"
#include "JavaFinder.h"
#include "FindJava2Dlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// The one and only MFC application object
class CFindJava2App : public CWinApp {
public:
    CFindJava2App() {
    }

    // Set CWinApp default registry key. Must be consistent with all apps using findjava2.
    void initRegistryKey() {
        SetRegistryKey(_T("Android-FindJava2"));
    }
};

CFindJava2App theApp;

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[]) {

    // Init utils; use default app name based on VERSIONINFO.FileDescription
    initUtils(NULL);

    // initialize MFC and print and error on failure
    HMODULE hModule = ::GetModuleHandle(NULL);
    if (hModule == NULL) {
        displayLastError(_T("Fatal Error: "));
        return -2;
    }
    if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0)) {
        displayLastError(_T("Fatal Error: "));
        return -3;
    }

    theApp.initRegistryKey();

    gIsConsole = true; // tell utils to to print errors to stderr
    gIsDebug = (getenv("ANDROID_SDKMAN_DEBUG") != NULL);

    // Parse command line
    bool doTests = false;
    bool doShortPath = false;
    bool doVersion = false;
    bool doJavaW = false;
    bool doForceUi = false;
    bool doJava1_7 = false;

    for (int i = 1; i < argc; i++) {
        if (_tcsnccmp(argv[i], _T("-t"), 2) == 0) {
            doTests = true;

        } else if (_tcsnccmp(argv[i], _T("-d"), 2) == 0) {
            gIsDebug = true;

        } else if (_tcsnccmp(argv[i], _T("-s"), 2) == 0) {
            doShortPath = true;

        } else if (_tcsnccmp(argv[i], _T("-v"), 2) == 0) {
            doVersion = true;

        } else if (_tcsnccmp(argv[i], _T("-f"), 2) == 0) {
            doForceUi = true;

        } else if (_tcsnccmp(argv[i], _T("-7"), 2) == 0) {
            doJava1_7 = true;

        } else if (_tcscmp(argv[i], _T("-w")) == 0 || _tcscmp(argv[i], _T("-javaw")) == 0) {
            doJavaW = true;

        } else {
            printf(
                "Outputs the path of the first Java.exe found on the local system.\n"
                "Returns code 0 when found, 1 when not found.\n"
                "Options:\n"
                "-h / -help   : This help.\n"
                "-t / -test   : Internal test.\n"
                "-f / -force  : Force UI selection.\n"
                "-7           : Java 1.7 minimum instead of 1.6.\n"
                "-s / -short  : Print path in short DOS form.\n"
                "-w / -javaw  : Search a matching javaw.exe; defaults to java.exe if not found.\n"
                "-v / -version: Only prints the Java version found.\n"
                );
            return 2;
        }
    }


    CJavaFinder javaFinder(JAVA_VERS_TO_INT(1, doJava1_7 ? 7 : 6));
    CJavaPath javaPath = javaFinder.getRegistryPath();

    if (doTests) {
        std::set<CJavaPath> paths;
        javaFinder.findJavaPaths(&paths);
        bool regPrinted = false;
        for (const CJavaPath &p : paths) {
            bool isReg = (p == javaPath);
            if (isReg) {
                regPrinted = true;
            }
            _tprintf(_T("%c [%s] %s\n"), isReg ? '*' : ' ', p.getVersion(), p.mPath);
        }

        if (!regPrinted && !javaPath.isEmpty()) {
            const CJavaPath &p = javaPath;
            _tprintf(_T("* [%s] %s\n"), p.getVersion(), p.mPath);
        }
        return 0;
    }

    if (doForceUi || javaPath.isEmpty()) {
        CFindJava2Dlg dlg;
        dlg.setJavaFinder(&javaFinder);
        INT_PTR nResponse = dlg.DoModal();

        if (nResponse == IDOK) {
            // Get java path selected by user and save into registry for later re-use
            javaPath = dlg.getSelectedPath();
            javaFinder.setRegistryPath(javaPath);
        } else if (nResponse == -1) {   // MFC boilerplate
            TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
            return 1;
        }
    }

    if (javaPath.isEmpty()) {
        fprintf(stderr, "No java.exe path found");
        return 1;
    }

    if (doShortPath) {
        PVOID oldWow64Value = disableWow64FsRedirection();
        if (!javaPath.toShortPath()) {
            revertWow64FsRedirection(&oldWow64Value);
            _ftprintf(stderr,
                    _T("Failed to convert path to a short DOS path: %s\n"),
                    javaPath.mPath);
            return 1;
        }
        revertWow64FsRedirection(&oldWow64Value);
    }

    if (doVersion) {
        // Print version found. We already have the version as an integer
        // so we don't need to run java -version a second time.
        _tprintf(_T("%s"), javaPath.getVersion());
        return 0;
    }

    if (doJavaW) {
        // Try to find a javaw.exe instead of java.exe at the same location.
        CPath javawPath = javaPath.mPath;
        javawPath.RemoveFileSpec();
        javawPath.Append(_T("javaw.exe"));
        javawPath.Canonicalize();

        // Only accept it if we can actually find the exec
        PVOID oldWow64Value = disableWow64FsRedirection();
        bool exists = javawPath.FileExists() == TRUE; // skip BOOL-to-bool warning
        revertWow64FsRedirection(&oldWow64Value);

        if (!exists) {
            _ftprintf(stderr,
                    _T("Failed to find javaw at: %s\n"),
                    javawPath);
            return 1;
        }

        javaPath.mPath = javawPath;
    }

    // Print java.exe path found
    _tprintf(_T("%s"), javaPath.mPath);
    return 0;

}
