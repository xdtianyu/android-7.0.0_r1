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
#include "JavaFinder.h"
#include "utils.h"

#include <algorithm>        // std::sort and std::unique

#define  _CRT_SECURE_NO_WARNINGS

// --------------

#define JF_REGISTRY_KEY         _T("Software\\Android\\FindJava2")
#define JF_REGISTRY_VALUE_PATH  _T("JavaPath")
#define JF_REGISTRY_VALUE_VERS  _T("JavaVers")

// --------------


// Extract the first thing that looks like (digit.digit+).
// Note: this will break when java reports a version with major > 9.
// However it will reasonably cope with "1.10", if that ever happens.
static bool extractJavaVersion(const TCHAR *start,
                               int length,
                               CString *outVersionStr,
                               int *outVersionInt) {
    const TCHAR *end = start + length;
    for (const TCHAR *c = start; c < end - 2; c++) {
        if (isdigit(c[0]) &&
            c[1] == '.' &&
            isdigit(c[2])) {
            const TCHAR *e = c + 2;
            while (isdigit(e[1])) {
                e++;
            }
            outVersionStr->SetString(c, e - c + 1);

            // major is currently only 1 digit
            int major = (*c - '0');
            // add minor
            int minor = 0;
            for (int m = 1; *e != '.'; e--, m *= 10) {
                minor += (*e - '0') * m;
            }
            *outVersionInt = JAVA_VERS_TO_INT(major, minor);
            return true;
        }
    }
    return false;
}

// Tries to invoke the java.exe at the given path and extract it's
// version number.
// - outVersionStr: not null, will capture version as a string (e.g. "1.6")
// - outVersionInt: not null, will capture version as an int (see JavaPath.h).
bool getJavaVersion(CPath &javaPath, CString *outVersionStr, int *outVersionInt) {
    bool result = false;

    // Run "java -version", which outputs something to *STDERR* like this:
    //
    // java version "1.6.0_29"
    // Java(TM) SE Runtime Environment (build 1.6.0_29-b11)
    // Java HotSpot(TM) Client VM (build 20.4-b02, mixed mode, sharing)
    //
    // We want to capture the first line, and more exactly the "1.6" part.


    CString cmd;
    cmd.Format(_T("\"%s\" -version"), (LPCTSTR) javaPath);

    SECURITY_ATTRIBUTES   saAttr;
    STARTUPINFO           startup;
    PROCESS_INFORMATION   pinfo;

    // Want to inherit pipe handle
    ZeroMemory(&saAttr, sizeof(saAttr));
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create pipe for stdout
    HANDLE stdoutPipeRd, stdoutPipeWt;
    if (!CreatePipe(
            &stdoutPipeRd,      // hReadPipe,
            &stdoutPipeWt,      // hWritePipe,
            &saAttr,            // lpPipeAttributes,
            0)) {               // nSize (0=default buffer size)
        // In FindJava2, we do not report these errors. Leave commented for reference.
        // // if (gIsConsole || gIsDebug) displayLastError("CreatePipe failed: ");
        return false;
    }
    if (!SetHandleInformation(stdoutPipeRd, HANDLE_FLAG_INHERIT, 0)) {
        // In FindJava2, we do not report these errors. Leave commented for reference.
        // // if (gIsConsole || gIsDebug) displayLastError("SetHandleInformation failed: ");
        return false;
    }

    ZeroMemory(&pinfo, sizeof(pinfo));

    ZeroMemory(&startup, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    startup.wShowWindow = SW_HIDE | SW_MINIMIZE;
    // Capture both stderr and stdout
    startup.hStdError = stdoutPipeWt;
    startup.hStdOutput = stdoutPipeWt;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    BOOL ok = CreateProcess(
        NULL,                   // program path
        (LPTSTR)((LPCTSTR) cmd),// command-line
        NULL,                   // process handle is not inheritable
        NULL,                   // thread handle is not inheritable
        TRUE,                   // yes, inherit some handles
        0,                      // process creation flags
        NULL,                   // use parent's environment block
        NULL,                   // use parent's starting directory
        &startup,               // startup info, i.e. std handles
        &pinfo);

    // In FindJava2, we do not report these errors. Leave commented for reference.
    // // if ((gIsConsole || gIsDebug) && !ok) displayLastError("CreateProcess failed: ");

    // Close the write-end of the output pipe (we're only reading from it)
    CloseHandle(stdoutPipeWt);

    // Read from the output pipe. We don't need to read everything,
    // the first line should be 'Java version "1.2.3_45"\r\n'
    // so reading about 32 chars is all we need.
    TCHAR first32[32 + 1];
    int index = 0;
    first32[0] = 0;

    if (ok) {
        #define SIZE 1024
        char buffer[SIZE];
        DWORD sizeRead = 0;

        while (ok) {
            // Keep reading in the same buffer location
            // Note: ReadFile uses a char buffer, not a TCHAR one.
            ok = ReadFile(stdoutPipeRd,     // hFile
                          buffer,           // lpBuffer
                          SIZE,             // DWORD buffer size to read
                          &sizeRead,        // DWORD buffer size read
                          NULL);            // overlapped
            if (!ok || sizeRead == 0 || sizeRead > SIZE) break;

            // Copy up to the first 32 characters
            if (index < 32) {
                DWORD n = 32 - index;
                if (n > sizeRead) n = sizeRead;
                // copy as lowercase to simplify checks later
                for (char *b = buffer; n > 0; n--, b++, index++) {
                    char c = *b;
                    if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
                    first32[index] = c;
                }
                first32[index] = 0;
            }
        }

        WaitForSingleObject(pinfo.hProcess, INFINITE);

        DWORD exitCode;
        if (GetExitCodeProcess(pinfo.hProcess, &exitCode)) {
            // this should not return STILL_ACTIVE (259)
            result = exitCode == 0;
        }

        CloseHandle(pinfo.hProcess);
        CloseHandle(pinfo.hThread);
    }
    CloseHandle(stdoutPipeRd);

    if (result && index > 0) {
        // Look for a few keywords in the output however we don't
        // care about specific ordering or case-senstiviness.
        // We only capture roughtly the first line in lower case.
        TCHAR *j = _tcsstr(first32, _T("java"));
        TCHAR *v = _tcsstr(first32, _T("version"));
        // In FindJava2, we do not report these errors. Leave commented for reference.
        // // if ((gIsConsole || gIsDebug) && (!j || !v)) {
        // //     fprintf(stderr, "Error: keywords 'java version' not found in '%s'\n", first32);
        // // }
        if (j != NULL && v != NULL) {
            result = extractJavaVersion(first32, index, outVersionStr, outVersionInt);
        }
    }

    return result;
}

// --------------

// Checks whether we can find $PATH/java.exe.
// inOutPath should be the directory where we're looking at.
// In output, it will be the java path we tested.
// Returns the java version integer found (e.g. 1006 for 1.6).
// Return 0 in case of error.
static int checkPath(CPath *inOutPath) {

    // Append java.exe to path if not already present
    CString &p = (CString&)*inOutPath;
    int n = p.GetLength();
    if (n < 9 || p.Right(9).CompareNoCase(_T("\\java.exe")) != 0) {
        inOutPath->Append(_T("java.exe"));
    }

    int result = 0;
    PVOID oldWow64Value = disableWow64FsRedirection();
    if (inOutPath->FileExists()) {
        // Run java -version
        // Reject the version if it's not at least our current minimum.
        CString versionStr;
        if (!getJavaVersion(*inOutPath, &versionStr, &result)) {
            result = 0;
        }
    }

    revertWow64FsRedirection(oldWow64Value);
    return result;
}

// Check whether we can find $PATH/bin/java.exe
// Returns the Java version found (e.g. 1006 for 1.6) or 0 in case of error.
static int checkBinPath(CPath *inOutPath) {

    // Append bin to path if not already present
    CString &p = (CString&)*inOutPath;
    int n = p.GetLength();
    if (n < 4 || p.Right(4).CompareNoCase(_T("\\bin")) != 0) {
        inOutPath->Append(_T("bin"));
    }

    return checkPath(inOutPath);
}

// Search java.exe in the environment
static void findJavaInEnvPath(std::set<CJavaPath> *outPaths) {
    ::SetLastError(0);

    const TCHAR* envPath = _tgetenv(_T("JAVA_HOME"));
    if (envPath != NULL) {
        CPath p(envPath);
        int v = checkBinPath(&p);
        if (v > 0) {
            outPaths->insert(CJavaPath(v, p));
        }
    }

    envPath = _tgetenv(_T("PATH"));
    if (envPath != NULL) {
        // Otherwise look at the entries in the current path.
        // If we find more than one, keep the one with the highest version.
        CString pathTokens(envPath);
        int curPos = 0;
        CString tok;
        do {
            tok = pathTokens.Tokenize(_T(";"), curPos);
            if (!tok.IsEmpty()) {
                CPath p(tok);
                int v = checkPath(&p);
                if (v > 0) {
                    outPaths->insert(CJavaPath(v, p));
                }
            }
        } while (!tok.IsEmpty());
    }
}


// --------------

static bool getRegValue(const TCHAR *keyPath,
                        const TCHAR *keyName,
                        REGSAM access,
                        CString *outValue) {
    HKEY key;
    LSTATUS status = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,         // hKey
        keyPath,                    // lpSubKey
        0,                          // ulOptions
        KEY_READ | access,          // samDesired,
        &key);                      // phkResult
    if (status == ERROR_SUCCESS) {
        LSTATUS ret = ERROR_MORE_DATA;
        DWORD size = 4096; // MAX_PATH is 260, so 4 KB should be good enough
        TCHAR* buffer = (TCHAR*)malloc(size);

        while (ret == ERROR_MORE_DATA && size < (1 << 16) /*64 KB*/) {
            ret = RegQueryValueEx(
                key,                // hKey
                keyName,            // lpValueName
                NULL,               // lpReserved
                NULL,               // lpType
                (LPBYTE)buffer,     // lpData
                &size);             // lpcbData

            if (ret == ERROR_MORE_DATA) {
                size *= 2;
                buffer = (TCHAR*)realloc(buffer, size);
            } else {
                buffer[size] = 0;
            }
        }

        if (ret != ERROR_MORE_DATA) {
            outValue->SetString(buffer);
        }

        free(buffer);
        RegCloseKey(key);

        return (ret != ERROR_MORE_DATA);
    }

    return false;
}

// Explore the registry to find a suitable version of Java.
// Returns an int which is the version of Java found (e.g. 1006 for 1.6) and the
// matching path in outJavaPath.
// Returns 0 if nothing suitable was found.
static int exploreJavaRegistry(const TCHAR *entry, REGSAM access, std::set<CJavaPath> *outPaths) {

    // Let's visit HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Runtime Environment [CurrentVersion]
    CPath rootKey(_T("SOFTWARE\\JavaSoft\\"));
    rootKey.Append(entry);

    CString currentVersion;
    CPath subKey(rootKey);
    if (getRegValue(subKey, _T("CurrentVersion"), access, &currentVersion)) {
        // CurrentVersion should be something like "1.7".
        // We want to read HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Runtime Environment\1.7 [JavaHome]
        subKey.Append(currentVersion);
        CString value;
        if (getRegValue(subKey, _T("JavaHome"), access, &value)) {
            CPath javaHome(value);
            int v = checkBinPath(&javaHome);
            if (v > 0) {
                outPaths->insert(CJavaPath(v, javaHome));
            }
        }
    }

    // Try again, but this time look at all the versions available
    HKEY javaHomeKey;
    LSTATUS status = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,         // hKey
        _T("SOFTWARE\\JavaSoft"),   // lpSubKey
        0,                          // ulOptions
        KEY_READ | access,          // samDesired
        &javaHomeKey);              // phkResult
    if (status == ERROR_SUCCESS) {
        TCHAR name[MAX_PATH + 1];
        DWORD index = 0;
        CPath javaHome;
        for (LONG result = ERROR_SUCCESS; result == ERROR_SUCCESS; index++) {
            DWORD nameLen = MAX_PATH;
            name[nameLen] = 0;
            result = RegEnumKeyEx(
                javaHomeKey,  // hKey
                index,        // dwIndex
                name,         // lpName
                &nameLen,     // lpcName
                NULL,         // lpReserved
                NULL,         // lpClass
                NULL,         // lpcClass,
                NULL);        // lpftLastWriteTime
            if (result == ERROR_SUCCESS && nameLen < MAX_PATH) {
                name[nameLen] = 0;
                CPath subKey(rootKey);
                subKey.Append(name);

                CString value;
                if (getRegValue(subKey, _T("JavaHome"), access, &value)) {
                    CPath javaHome(value);
                    int v = checkBinPath(&javaHome);
                    if (v > 0) {
                        outPaths->insert(CJavaPath(v, javaHome));
                    }
                }
            }
        }

        RegCloseKey(javaHomeKey);
    }

    return 0;
}

static void findJavaInRegistry(std::set<CJavaPath> *outPaths) {
    // We'll do the registry test 3 times: first using the default mode,
    // then forcing the use of the 32-bit registry then forcing the use of
    // 64-bit registry. On Windows 2k, the 2 latter will fail since the
    // flags are not supported. On a 32-bit OS the 64-bit is obviously
    // useless and the 2 first tests should be equivalent so we just
    // need the first case.

    // Check the JRE first, then the JDK.
    exploreJavaRegistry(_T("Java Runtime Environment"), 0, outPaths);
    exploreJavaRegistry(_T("Java Development Kit"), 0, outPaths);

    // Get the app sysinfo state (the one hidden by WOW64)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    WORD programArch = sysInfo.wProcessorArchitecture;
    // Check the real sysinfo state (not the one hidden by WOW64) for x86
    GetNativeSystemInfo(&sysInfo);
    WORD actualArch = sysInfo.wProcessorArchitecture;

    // Only try to access the WOW64-32 redirected keys on a 64-bit system.
    // There's no point in doing this on a 32-bit system.
    if (actualArch == PROCESSOR_ARCHITECTURE_AMD64) {
        if (programArch != PROCESSOR_ARCHITECTURE_INTEL) {
            // If we did the 32-bit case earlier, don't do it twice.
            exploreJavaRegistry(_T("Java Runtime Environment"), KEY_WOW64_32KEY, outPaths);
            exploreJavaRegistry(_T("Java Development Kit"),     KEY_WOW64_32KEY, outPaths);

        } else if (programArch != PROCESSOR_ARCHITECTURE_AMD64) {
            // If we did the 64-bit case earlier, don't do it twice.
            exploreJavaRegistry(_T("Java Runtime Environment"), KEY_WOW64_64KEY, outPaths);
            exploreJavaRegistry(_T("Java Development Kit"),     KEY_WOW64_64KEY, outPaths);
        }
    }
}

// --------------

static void checkProgramFiles(std::set<CJavaPath> *outPaths) {

    TCHAR programFilesPath[MAX_PATH + 1];
    HRESULT result = SHGetFolderPath(
        NULL,                       // hwndOwner
        CSIDL_PROGRAM_FILES,        // nFolder
        NULL,                       // hToken
        SHGFP_TYPE_CURRENT,         // dwFlags
        programFilesPath);          // pszPath

    CPath path(programFilesPath);
    path.Append(_T("Java"));

    // Do we have a C:\\Program Files\\Java directory?
    if (!path.IsDirectory()) {
        return;
    }

    CPath glob(path);
    glob.Append(_T("j*"));

    WIN32_FIND_DATA findData;
    HANDLE findH = FindFirstFile(glob, &findData);
    if (findH == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            CPath temp(path);
            temp.Append(findData.cFileName);
            // Check C:\\Program Files[x86]\\Java\\j*\\bin\\java.exe
            int v = checkBinPath(&temp);
            if (v > 0) {
                outPaths->insert(CJavaPath(v, temp));
            }
        }
    } while (FindNextFile(findH, &findData) != 0);
    FindClose(findH);
}

static void findJavaInProgramFiles(std::set<CJavaPath> *outPaths) {
    // Check the C:\\Program Files (x86) directory
    // With WOW64 fs redirection in place by default, we should get the x86
    // version on a 64-bit OS since this app is a 32-bit itself.
    checkProgramFiles(outPaths);

    // Check the real sysinfo state (not the one hidden by WOW64) for x86
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);

    if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        // On a 64-bit OS, try again by disabling the fs redirection so
        // that we can try the real C:\\Program Files directory.
        PVOID oldWow64Value = disableWow64FsRedirection();
        checkProgramFiles(outPaths);
        revertWow64FsRedirection(oldWow64Value);
    }
}

//------


CJavaFinder::CJavaFinder(int minVersion) : mMinVersion(minVersion) {
}


CJavaFinder::~CJavaFinder() {
}

/*
 * Checks whether there's a recorded path in the registry and whether
 * this path still points to a valid Java executable.
 * Returns false if any of these do not match,
 * Returns true if both condition match,
 * outPath contains the result path when returning true.
*/
CJavaPath CJavaFinder::getRegistryPath() {
    CString existing;
    CRegKey rk;

    if (rk.Open(HKEY_CURRENT_USER, JF_REGISTRY_KEY, KEY_READ) == ERROR_SUCCESS) {
        ULONG sLen = MAX_PATH;
        TCHAR s[MAX_PATH + 1];
        if (rk.QueryStringValue(JF_REGISTRY_VALUE_PATH, s, &sLen) == ERROR_SUCCESS) {
            existing.SetString(s);
        }
        rk.Close();
    }

    if (!existing.IsEmpty()) {
        CJavaPath javaPath;
        if (checkJavaPath(existing, &javaPath)) {
            return javaPath;
        }
    }

    return CJavaPath::sEmpty;
}

bool CJavaFinder::setRegistryPath(const CJavaPath &javaPath) {
    CRegKey rk;

    if (rk.Create(HKEY_CURRENT_USER, JF_REGISTRY_KEY) == ERROR_SUCCESS) {
        bool ok = rk.SetStringValue(JF_REGISTRY_VALUE_PATH, javaPath.mPath, REG_SZ) == ERROR_SUCCESS &&
                  rk.SetStringValue(JF_REGISTRY_VALUE_VERS, javaPath.getVersion(), REG_SZ) == ERROR_SUCCESS;
        rk.Close();
        return ok;
    }

    return false;
}

void CJavaFinder::findJavaPaths(std::set<CJavaPath> *paths) {
    findJavaInEnvPath(paths);
    findJavaInProgramFiles(paths);
    findJavaInRegistry(paths);

    // Exclude any entries that do not match the minimum version.
    // The set is going to be fairly small so it's easier to do it here
    // than add the filter logic in all the static methods above.
    if (mMinVersion > 0) {
        for (auto it = paths->begin(); it != paths->end(); ) {
            if (it->mVersion < mMinVersion) {
                it = paths->erase(it);  // C++11 set.erase returns an iterator to the *next* element
            } else {
                ++it;
            }
        }
    }
}

bool CJavaFinder::checkJavaPath(const CString &path, CJavaPath *outPath) {
    CPath p(path);

    // try this path (if it ends with java.exe) or path\\java.exe
    int v = checkPath(&p);
    if (v == 0) {
        // reset path and try path\\bin\\java.exe
        p = CPath(path);
        v = checkBinPath(&p);
    }

    if (v > 0) {
        outPath->set(v, p);
        return v >= mMinVersion;
    }

    return false;
}

