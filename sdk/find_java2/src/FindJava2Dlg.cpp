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
#include "FindJava2Dlg.h"
#include "afxdialogex.h"
#include <atlpath.h>                            // ATL CPath

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define COL_PATH 1


CFindJava2Dlg::CFindJava2Dlg(CWnd* pParent /*=NULL*/)
    : CDialog(CFindJava2Dlg::IDD, pParent), mSelectedIndex(-1) {
    m_hIcon = AfxGetApp()->LoadIcon(IDI_ANDROID_ICON);
}

void CFindJava2Dlg::DoDataExchange(CDataExchange* pDX) {
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PATH_LIST, mPathsListCtrl);
    DDX_Control(pDX, IDOK, mOkButton);
}

BEGIN_MESSAGE_MAP(CFindJava2Dlg, CDialog)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_ADD, &CFindJava2Dlg::OnBnClickedButtonAdd)
    ON_NOTIFY(NM_CLICK, IDC_PATH_LIST, &CFindJava2Dlg::OnNMClickPathList)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_PATH_LIST, &CFindJava2Dlg::OnLvnItemchangedPathList)
END_MESSAGE_MAP()


// -----
// CFindJava2Dlg message handlers

BOOL CFindJava2Dlg::OnInitDialog() {
    CDialog::OnInitDialog();

    SetWindowText(getAppName());

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);			// Set big icon
    SetIcon(m_hIcon, FALSE);		// Set small icon

    // Initialize list controls
    mPathsListCtrl.SetExtendedStyle(
        mPathsListCtrl.GetExtendedStyle() |
        LVS_EX_CHECKBOXES | 
        LVS_EX_FULLROWSELECT | 
        LVS_EX_GRIDLINES);

    // We want 2 columns: Java version and path
    mPathsListCtrl.InsertColumn(0, _T("Version"), LVCFMT_RIGHT, 60,  0);
    mPathsListCtrl.InsertColumn(1, _T("Path"),     LVCFMT_LEFT, 386, 0);

    mJavaFinder->findJavaPaths(&mPaths);
    fillPathsList();
    adjustButtons();

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
// to draw the icon.  For MFC applications using the document/view model,
// this is automatically done for you by the framework.
// [Note: MFC boilerplate, keep as-is]
void CFindJava2Dlg::OnPaint() {
    if (IsIconic()) {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    } else {
        CDialog::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
// the minimized window. [Note: MFC boilerplate, keep as-is]
HCURSOR CFindJava2Dlg::OnQueryDragIcon() {
    return static_cast<HCURSOR>(m_hIcon);
}

// Button add has been pressed; use file dialog and add path if it's a valid java.exe
void CFindJava2Dlg::OnBnClickedButtonAdd() {
    CFileDialog fileDlg(
        TRUE,           // true=open dialog,  false=save-as dialog
        _T("exe"),      // lpszDefExt 
        _T("java.exe"), // lpszFileName 
        OFN_FILEMUSTEXIST || OFN_PATHMUSTEXIST,
        NULL,           // lpszFilter
        this);          // pParentWnd

    if (fileDlg.DoModal() == IDOK) {
        CString path = fileDlg.GetPathName();

        CJavaPath javaPath;
        if (!mJavaFinder->checkJavaPath(path, &javaPath)) {
            CString msg;
            if (javaPath.mVersion > 0) {
                msg.Format(_T("Insufficient Java Version found: expected %s, got %s"), 
                           CJavaPath(mJavaFinder->getMinVersion(), CPath()).getVersion(),
                           javaPath.getVersion());
            } else {
                msg.Format(_T("No valid Java Version found for %s"), path);
            }
            AfxMessageBox(msg, MB_OK);

        } else {
            if (mPaths.find(javaPath) == mPaths.end()) {
                // Path isn't known yet so add it and refresh the list.
                mPaths.insert(javaPath);
                fillPathsList();
            }

            // Select item in list and set mSelectedIndex
            selectPath(-1 /*index*/, &javaPath);
        }
    }
}

// An item in the list has been selected, select checkmark and set mSelectedIndex.
void CFindJava2Dlg::OnNMClickPathList(NMHDR *pNMHDR, LRESULT *pResult) {
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    int index = pNMItemActivate->iItem;
    selectPath(index, nullptr);
    *pResult = TRUE;
}

// An item in the list has changed, toggle checkmark as needed.
void CFindJava2Dlg::OnLvnItemchangedPathList(NMHDR *pNMHDR, LRESULT *pResult) {
    *pResult = FALSE;
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    if ((pNMLV->uChanged & LVIF_STATE) != 0) {
        // Item's state has changed. Check the selection to see if it needs to be adjusted.
        int index = pNMLV->iItem;

        UINT oldState = pNMLV->uOldState;
        UINT newState = pNMLV->uNewState;

        if ((oldState & LVIS_STATEIMAGEMASK) != 0 || (newState & LVIS_STATEIMAGEMASK) != 0) {
            // Checkbox uses the STATEIMAGE: 1 for unchecked, 2 for checked.
            // Checkbox is checked when (old/new-state & state-image-mask) == INDEXTOSTATEIMAGEMASK(2).

            bool oldChecked = (oldState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2);
            bool newChecked = (newState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2);

            if (oldChecked && !newChecked && index == mSelectedIndex) {
                mSelectedIndex = -1;
                adjustButtons();
            } else if (!oldChecked && newChecked && index != mSelectedIndex) {
                // Uncheck any checked rows if any
                for (int n = mPathsListCtrl.GetItemCount() - 1; n >= 0; --n) {
                    if (n != index && mPathsListCtrl.GetCheck(n)) {
                        mPathsListCtrl.SetCheck(n, FALSE);
                    }
                }

                mSelectedIndex = index;
                adjustButtons();
            }
            // We handled this case, don't dispatch it further
            *pResult = TRUE;
        }
    }
}

// -----

const CJavaPath& CFindJava2Dlg::getSelectedPath() {
    int i = 0;
    for (const CJavaPath &p : mPaths) {
        if (i == mSelectedIndex) {
            return p;
        }
        ++i;
    }

    return CJavaPath::sEmpty;
}


void CFindJava2Dlg::fillPathsList() {
    mPathsListCtrl.DeleteAllItems();
    int index = 0;

    for (const CJavaPath& pv : mPaths) {
        mPathsListCtrl.InsertItem(index, pv.getVersion());        // column 0 = version
        mPathsListCtrl.SetItemText(index, COL_PATH, pv.mPath);    // column 1 = path
        mPathsListCtrl.SetCheck(index, mSelectedIndex == index);
        ++index;
    }
}

// Checks the given index if valid. Unchecks all other items.
//
// If index >= 0, it is used to select that item from the ListControl.
// Otherwise if path != nullptr, it is used to find the item and select it.
//
// Side effect: in both cases, mSelectedIndex is set to the matching index or -1.
//
// If index is invalid and path isn't in the mPaths list, all items are unselected
// so calling this with (0, nullptr) will clear the current selection.
void CFindJava2Dlg::selectPath(int index, const CJavaPath *path) {

    const CJavaPath *foundPath;
    // If index is not defined, find the given path in the internal list.
    // If path is not defined, find its index in the internal list.
    int i = 0;
    int n = mPathsListCtrl.GetItemCount();
    for (const CJavaPath &p : mPaths) {
        if (index < 0 && path != nullptr && p == *path) {
            index = i;
            foundPath = path;
        } else if (index == i) {
            foundPath = &p;
        }

        // uncheck any marked path
        if (i != index && i < n && mPathsListCtrl.GetCheck(i)) {
            mPathsListCtrl.SetCheck(i, FALSE);
        }

        ++i;
    }

    mSelectedIndex = index;
    if (index >= 0 && index <= n) {
        mPathsListCtrl.SetCheck(index, TRUE);
    }

    adjustButtons();
}

void CFindJava2Dlg::adjustButtons() {
    int n = mPathsListCtrl.GetItemCount();
    mOkButton.EnableWindow(mSelectedIndex >= 0 && mSelectedIndex < n);
}
