/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.platform.test.helpers;

import android.app.Instrumentation;
import android.os.SystemClock;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.Until;
import android.support.test.uiautomator.UiObject2;

import junit.framework.Assert;

import java.lang.IllegalArgumentException;
import java.lang.IllegalStateException;

/**
 * UI test helper for Google Docs (package: com.google.android.apps.docs.editors.docs).
 * Implementation based on app version: 1.6.152
 */

public class GoogleDocsHelperImpl extends AbstractGoogleDocsHelper {

    private static final String LOG_TAG = GoogleDocsHelperImpl.class.getSimpleName();

    private static final String UI_PACKAGE_NAME = "com.google.android.apps.docs.editors.docs";
    private static final String UI_DOCS_LIST_TITLE = "title";
    private static final String UI_DOCS_LIST_VIEW = "doc_list_view";
    private static final String UI_EDITOR_VIEW = "kix_editor_view";
    private static final String UI_TOOLBAR = "toolbar";
    private static final String UI_TEXT_DOCS = "Docs";
    private static final String UI_TEXT_SKIP = "SKIP";

    private static final long HACKY_DELAY = 1000; // 1 sec
    private static final long LOAD_DOCUMENT_TIMEOUT = 60000; // 60 secs
    private static final int BACK_TO_RECENT_DOCS_MAX_RETRY = 5;
    private static final int SEARCHING_DOC_MAX_SCROLL_DOWN = 10;

    public GoogleDocsHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        return "com.google.android.apps.docs.editors.docs";
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        return "Docs";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        UiObject2 skipButton = getSkipButton();
        if (skipButton != null) {
            skipButton.click();
            mDevice.waitForIdle();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToRecentDocsTab() {
        for (int retryCnt = 0; retryCnt < BACK_TO_RECENT_DOCS_MAX_RETRY; retryCnt++) {
            if (isOnRecentDocsTab()) {
                return;
            }
            mDevice.pressBack();

            // TODO Hacky workaround
            // Bug: 28675538
            // mDevice.waitForIdle() is insufficient when a short (unscrollable)
            // document is scrolled by scrollDownDocument() before goToRecentDocsTab()
            // is called. isOnRecentDocsTab() fails to recognize the Recent Docs tab
            // even if the tab is indeed shown.
            SystemClock.sleep(HACKY_DELAY);
        }
        Assert.assertTrue("Failed to go to Recent Docs Tab", isOnRecentDocsTab());
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void openDoc(String title) {
        if (!isOnRecentDocsTab()) {
            throw new IllegalStateException("Not on the Recent Docs tab");
        }
        UiObject2 recentDocsList = getRecentDocsList();

        // TODO: Hacky workaround
        // Bug: 28675621
        // while (recentDocsList.scroll(Direction.UP, 1.0f));
        // The above while loop doesn't work as scroll doesn't return true
        // while there's more to scroll.
        recentDocsList.fling(Direction.UP);

        for (int cnt = 0; cnt < SEARCHING_DOC_MAX_SCROLL_DOWN; cnt++) {
            UiObject2 documentTitle = recentDocsList.findObject(
                    By.res(UI_PACKAGE_NAME, UI_DOCS_LIST_TITLE).text(title));
            if (documentTitle != null) {
                // document is found, click to download
                documentTitle.click();
                boolean editorLoaded = mDevice.wait(
                        Until.hasObject(By.res(UI_PACKAGE_NAME, UI_EDITOR_VIEW)),
                        LOAD_DOCUMENT_TIMEOUT);
                Assert.assertTrue(String.format("Failed to finish downloading %s", title),
                        editorLoaded);
                return;
            }
            recentDocsList.scroll(Direction.DOWN, 0.5f);
        }
        Assert.fail(String.format("Can't find the document: %s", title));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void scrollDownDocument() {
        UiObject2 docsEditorPage = getDocsEditorPage();
        if (docsEditorPage == null) {
            throw new IllegalStateException("Not on a document page");
        }
        docsEditorPage.scroll(Direction.DOWN, 1.0f);
    }

    private boolean isOnRecentDocsTab() {
        UiObject2 toolbar = getToolbar();
        if (toolbar == null) {
            return false;
        }
        return toolbar.hasObject(By.text(UI_TEXT_DOCS));
    }

    private UiObject2 getToolbar() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_TOOLBAR));
    }

    private UiObject2 getRecentDocsList() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_DOCS_LIST_VIEW));
    }

    private UiObject2 getDocsEditorPage() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_EDITOR_VIEW));
    }

    private UiObject2 getSkipButton() {
        return mDevice.findObject(By.text(UI_TEXT_SKIP));
    }
}
