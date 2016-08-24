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

// Global flag indicating whether this is running in debug mode (more printfs)
extern bool gIsDebug;
// Global flag indicating whether this is running in console mode or GUI.
// In console mode, errors are written on the console; in GUI they use a MsgBox.
extern bool gIsConsole;

// Must be called by the application to initialize the app name used in error dialog boxes.
// If NULL is used, fetches VERSIONINFO.FileDescription from resources if available.
void initUtils(const TCHAR *appName);

// Returns the app name set in initUtils
CString getAppName();

// Displays a message in an ok+info dialog box. Useful in console mode.
void msgBox(const TCHAR* text, ...);

// Displays GetLastError prefixed with a description in an error dialog box. Useful in console mode.
void displayLastError(const TCHAR *description, ...);

// Executes the command line. Does not wait for the program to finish.
// The return code is from CreateProcess (0 means failure), not the running app.
int execNoWait(const TCHAR *app, const TCHAR *params, const TCHAR *workDir);

// Executes command, waits for completion and returns exit code.
// As indicated in MSDN for CreateProcess, callers should double-quote the program name
// e.g. cmd="\"c:\program files\myapp.exe\" arg1 arg2";
int execWait(const TCHAR *cmd);

bool getModuleDir(CPath *outDir);

// Disables the FS redirection done by WOW64.
// Because this runs as a 32-bit app, Windows automagically remaps some
// folder under the hood (e.g. "Programs Files(x86)" is mapped as "Program Files").
// This prevents the app from correctly searching for java.exe in these folders.
// The registry is also remapped.
PVOID disableWow64FsRedirection();

// Reverts the redirection disabled in disableWow64FsRedirection.
void revertWow64FsRedirection(PVOID oldWow64Value);
