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
#include "WinLauncher2App.h"

#include "utils.h"
#include "JavaFinder.h"
#include "FindJava2Dlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CWinLauncher2App

BEGIN_MESSAGE_MAP(CWinLauncher2App, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// The one and only CWinLauncher2App object
CWinLauncher2App theApp;

class CLauncherCmdLineInfo : public CCommandLineInfo {
public:
    bool mDoHelp;
    bool mDoForceUi;
    bool mDoJava1_7;
    CString mFilename;

    CLauncherCmdLineInfo() : mDoHelp(false), mDoForceUi(false), mDoJava1_7(false) {}

    virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast) {
        // Expected command line:
        // /h | help  : msg box with command line arguments
        // /f | force : force UI selection
        // /7         : require java 1.7
        // path-to-launch

        if (!bFlag) {
            mFilename = pszParam;
        } else if (_tcsnccmp(pszParam, _T("h"), 2) == 0) {
            mDoHelp = true;
        } else if (_tcsnccmp(pszParam, _T("f"), 2) == 0) {
            mDoForceUi = true;
        } else if (_tcsnccmp(pszParam, _T("7"), 2) == 0) {
            mDoJava1_7 = true;
        }
    }
};


CWinLauncher2App::CWinLauncher2App() {
    // support Restart Manager
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

BOOL CWinLauncher2App::InitInstance() {
    // InitCommonControlsEx() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // Set this to include all the common control classes you want to use
    // in your application.
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();
    AfxEnableControlContainer();

    // Create the shell manager, in case the dialog contains
    // any shell tree view or shell list view controls.
    CShellManager *pShellManager = new CShellManager;

    // Activate "Windows Native" visual manager for enabling themes in MFC controls
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    // Set CWinApp default registry key. Must be consistent with all apps using findjava2.
    SetRegistryKey(_T("Android-FindJava2"));

    // Use VERSIONINFO.FileDescription as the canonical app name
    initUtils(NULL);

    CLauncherCmdLineInfo cmdLine;
    ParseCommandLine(cmdLine);

    if (cmdLine.mDoHelp) {
        const TCHAR *msg =
            _T("WinLauncher2 [/7|/f|/h]\r\n")
            _T("/7 : Requires Java 1.7 instead of 1.6\r\n")
            _T("/f : Force UI\r\n")
            _T("/h : Help\r\n");
            AfxMessageBox(msg);
        return FALSE; // quit without starting MFC app msg loop
    }

    CJavaFinder javaFinder(JAVA_VERS_TO_INT(1, cmdLine.mDoJava1_7 ? 7 : 6));
    CJavaPath javaPath = javaFinder.getRegistryPath();
    if (cmdLine.mDoForceUi || javaPath.isEmpty()) {
        javaPath.clear();

        CFindJava2Dlg dlg;
        dlg.setJavaFinder(&javaFinder);
        m_pMainWnd = &dlg;
        INT_PTR nResponse = dlg.DoModal();

        if (nResponse == IDOK) {
            // Use choice selected by user and save in registry.
            javaPath = dlg.getSelectedPath();
            javaFinder.setRegistryPath(javaPath);
        } else if (nResponse == IDCANCEL) {
            // Canceled by user, exit silently.
        } else if (nResponse == -1) {
            TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
        }
    }

    if (!javaPath.isEmpty()) {
        // TODO actually launch configured app instead of just printing path.
        CString msg(_T("PLACEHOLDER TODO run app using "));
        msg.Append(javaPath.mPath);
        AfxMessageBox(msg);
    }

    // Delete the shell manager created above.
    if (pShellManager != NULL) {
        delete pShellManager;
    }

    // Since the dialog has been closed, return FALSE so that we exit the
    // application, rather than start the application's message pump.
    return FALSE;
}

