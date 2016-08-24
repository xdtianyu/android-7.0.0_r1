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
#include "afxwin.h"
#include "JavaFinder.h"

#include "resource.h"		// main symbols


// CFindJava2Dlg dialog
class CFindJava2Dlg : public CDialog {
    // Construction
public:
    CFindJava2Dlg(CWnd* pParent = NULL);	// standard constructor

    void setJavaFinder(CJavaFinder *javaFinder) { mJavaFinder = javaFinder;  }
    const CJavaPath& getSelectedPath();

    // Dialog Data
    enum { IDD = IDD_FINDJAVA2_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


    // Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()

    afx_msg void OnBnClickedButtonAdd();
    afx_msg void OnNMClickPathList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnItemchangedPathList(NMHDR *pNMHDR, LRESULT *pResult);

private:
    std::set<CJavaPath> mPaths;
    int mSelectedIndex;
    CJavaFinder *mJavaFinder;
    CListCtrl mPathsListCtrl;
    CButton mOkButton;

    void fillPathsList();
    void adjustButtons();
    void selectPath(int index = -1, const CJavaPath *path = nullptr);
};
