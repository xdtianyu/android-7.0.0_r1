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
#include "utils.h"

// Set to true to get some extra debug information
bool gIsDebug = false;
// Set to true to output errors to stderr (for a Console app)
// or to false to output using msg box (for a Windows UI app)
bool gIsConsole = false;

// Application name used in error dialog. Defined using initUtils()
static CString gAppName("Find Java 2");

// Called by the application to initialize the app name used in error dialog boxes.
void initUtils(const TCHAR *appName) {
    if (appName != NULL) {
        gAppName = CString(appName);
        return;
    }

    // Try to get the VERSIONINFO.FileDescription and use as app name
    // Errors are ignored, in which case the default app name is used.

    // First get the module (aka app instance) filename.
    TCHAR moduleName[MAX_PATH + 1];
    DWORD sz = ::GetModuleFileName(NULL /*AfxGetInstanceHandle()*/, moduleName, MAX_PATH);
    if (sz == 0) {
        // GetModuleFileName failed. Do nothing.
        return;
    }
    moduleName[sz] = '\0';  // make sure string is properly terminated.

    // Get the size of the FileVersionInfo buffer
    DWORD obsoleteHandle; // see http://blogs.msdn.com/b/oldnewthing/archive/2007/07/31/4138786.aspx
    DWORD fviSize = ::GetFileVersionInfoSize(moduleName, &obsoleteHandle);
    if (fviSize == 0) {
        return; // do nothing on error
    }

    char *fviBuffer = new char[fviSize];
    if (::GetFileVersionInfo(moduleName, 0, fviSize, fviBuffer) != 0) {
        VOID *vBuffer;
        UINT vLen;

        struct LANGUAGE_CODEPAGE {
            WORD mLanguage;
            WORD mCodePage;
        } *lgcpBuffer;

        UINT lgcpSize;

        // Read the list of languages and code pages (c.f. MSDN for VerQueryValue)
        if (::VerQueryValue(fviBuffer, _T("\\VarFileInfo\\Translation"), (LPVOID*)&lgcpBuffer, &lgcpSize) != 0 &&
                lgcpSize >= sizeof(LANGUAGE_CODEPAGE)) {
            // Use the first available language and code page
            CString subBlock;
            subBlock.Format(_T("\\StringFileInfo\\%04x%04x\\FileDescription"),
                            lgcpBuffer[0].mLanguage,
                            lgcpBuffer[0].mCodePage);
            if (::VerQueryValue(fviBuffer, subBlock, &vBuffer, &vLen) != 0) {
                gAppName.SetString((LPCTSTR)vBuffer, vLen);
            }
        }
    }
    delete fviBuffer;
}

CString getAppName() {
    return gAppName;
}


// Displays a message in an ok+info dialog box.
void msgBox(const TCHAR* text, ...) {
    CString formatted;
    va_list ap;
    va_start(ap, text);
    formatted.FormatV(text, ap);
    va_end(ap);

    // TODO global CString to get app name
    MessageBox(NULL, formatted, gAppName, MB_OK | MB_ICONINFORMATION);
}

// Sets the string to the message matching Win32 GetLastError.
// If message is non-null, it is prepended to the last error string.
CString getLastWin32Error(const TCHAR* message) {
    DWORD err = GetLastError();
    CString result;
    LPTSTR errStr;
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | /* dwFlags */
                      FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,                             /* lpSource */
                      err,                              /* dwMessageId */
                      0,                                /* dwLanguageId */
                      (LPTSTR) &errStr,                 /* out lpBuffer */
                      0,                                /* nSize */
                      NULL) != 0) {                     /* va_list args */
        if (message == NULL) {
            result.Format(_T("[%d] %s"), err, errStr);
        } else {
            result.Format(_T("%s[%d] %s"), message, err, errStr);
        }
        LocalFree(errStr);
    }
    return result;
}

// Displays GetLastError prefixed with a description in an error dialog box
void displayLastError(const TCHAR *description, ...) {
    CString formatted;
    va_list ap;
    va_start(ap, description);
    formatted.FormatV(description, ap);
    va_end(ap);

    CString error = getLastWin32Error(NULL);
    formatted.Append(_T("\r\n"));
    formatted.Append(error);

    if (gIsConsole) {
        _ftprintf(stderr, _T("%s\n"), (LPCTSTR) formatted);
    } else {
        CString name(gAppName);
        name.Append(_T(" - Error"));
        MessageBox(NULL, formatted, name, MB_OK | MB_ICONERROR);
    }
}

// Executes the command line. Does not wait for the program to finish.
// The return code is from CreateProcess (0 means failure), not the running app.
int execNoWait(const TCHAR *app, const TCHAR *params, const TCHAR *workDir) {
    STARTUPINFO           startup;
    PROCESS_INFORMATION   pinfo;

    ZeroMemory(&pinfo, sizeof(pinfo));

    ZeroMemory(&startup, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWDEFAULT;

    int ret = CreateProcess(
        app,                                        /* program path */
        (TCHAR *)params,                            /* command-line */
        NULL,                  /* process handle is not inheritable */
        NULL,                   /* thread handle is not inheritable */
        TRUE,                          /* yes, inherit some handles */
        0,                                          /* create flags */
        NULL,                     /* use parent's environment block */
        workDir,                 /* use parent's starting directory */
        &startup,                 /* startup info, i.e. std handles */
        &pinfo);

    if (ret) {
        CloseHandle(pinfo.hProcess);
        CloseHandle(pinfo.hThread);
    }

    return ret;
}

// Executes command, waits for completion and returns exit code.
// As indicated in MSDN for CreateProcess, callers should double-quote the program name
// e.g. cmd="\"c:\program files\myapp.exe\" arg1 arg2";
int execWait(const TCHAR *cmd) {
    STARTUPINFO           startup;
    PROCESS_INFORMATION   pinfo;

    ZeroMemory(&pinfo, sizeof(pinfo));

    ZeroMemory(&startup, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE | SW_MINIMIZE;

    int ret = CreateProcess(
        NULL,                                       /* program path */
        (LPTSTR)cmd,                                /* command-line */
        NULL,                  /* process handle is not inheritable */
        NULL,                   /* thread handle is not inheritable */
        TRUE,                          /* yes, inherit some handles */
        CREATE_NO_WINDOW,                /* we don't want a console */
        NULL,                     /* use parent's environment block */
        NULL,                    /* use parent's starting directory */
        &startup,                 /* startup info, i.e. std handles */
        &pinfo);

    int result = -1;
    if (ret) {
        WaitForSingleObject(pinfo.hProcess, INFINITE);

        DWORD exitCode;
        if (GetExitCodeProcess(pinfo.hProcess, &exitCode)) {
            // this should not return STILL_ACTIVE (259)
            result = exitCode;
        }
        CloseHandle(pinfo.hProcess);
        CloseHandle(pinfo.hThread);
    }

    return result;
}

bool getModuleDir(CPath *outDir) {
    TCHAR programDir[MAX_PATH];
    int ret = GetModuleFileName(NULL, programDir, sizeof(programDir) * sizeof(TCHAR));
    if (ret != 0) {
        CPath dir(programDir);
        dir.RemoveFileSpec();
        *outDir = dir;
        return true;
    }
    return false;
}

// Disables the FS redirection done by WOW64.
// Because this runs as a 32-bit app, Windows automagically remaps some
// folder under the hood (e.g. "Programs Files(x86)" is mapped as "Program Files").
// This prevents the app from correctly searching for java.exe in these folders.
// The registry is also remapped. This method disables this redirection.
// Caller should restore the redirection later by using revertWow64FsRedirection().
PVOID disableWow64FsRedirection() {

    // The call we want to make is the following:
    //    PVOID oldWow64Value;
    //    Wow64DisableWow64FsRedirection(&oldWow64Value);
    // However that method may not exist (e.g. on XP non-64 systems) so
    // we must not call it directly.

    PVOID oldWow64Value = 0;

    HMODULE hmod = LoadLibrary(_T("kernel32.dll"));
    if (hmod != NULL) {
        FARPROC proc = GetProcAddress(hmod, "Wow64DisableWow64FsRedirection");
        if (proc != NULL) {
            typedef BOOL(WINAPI *disableWow64FuncType)(PVOID *);
            disableWow64FuncType funcPtr = (disableWow64FuncType)proc;
            funcPtr(&oldWow64Value);
        }

        FreeLibrary(hmod);
    }

    return oldWow64Value;
}

// Reverts the redirection disabled in disableWow64FsRedirection.
void revertWow64FsRedirection(PVOID oldWow64Value) {

    // The call we want to make is the following:
    //    Wow64RevertWow64FsRedirection(oldWow64Value);
    // However that method may not exist (e.g. on XP non-64 systems) so
    // we must not call it directly.

    HMODULE hmod = LoadLibrary(_T("kernel32.dll"));
    if (hmod != NULL) {
        FARPROC proc = GetProcAddress(hmod, "Wow64RevertWow64FsRedirection");
        if (proc != NULL) {
            typedef BOOL(WINAPI *revertWow64FuncType)(PVOID);
            revertWow64FuncType funcPtr = (revertWow64FuncType)proc;
            funcPtr(oldWow64Value);
        }

        FreeLibrary(hmod);
    }
}
