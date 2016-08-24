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

import java.lang.IllegalStateException;

/**
 * UI test helper for Play Books: The Official App (package: com.google.android.apps.books).
 * Implementation based on app version: 3.8.15
 */

public class PlayBooksHelperImpl extends AbstractPlayBooksHelper {

    private static final String LOG_TAG = PlayBooksHelperImpl.class.getSimpleName();

    private static final String UI_PACKAGE_NAME = "com.google.android.apps.books";
    private static final String UI_NAVIGATE_UP_DESC = "Navigate up";
    private static final String UI_NAVIGATION_DRAWER_BUTTON_DESC = "Show navigation drawer";
    private static final String UI_EXIT_BOOK_DESC = "Exit book";
    private static final String UI_TAB_ALL_BOOKS_TEXT = "ALL BOOKS";
    private static final String UI_NAVIGATION_DRAWER_SETTING_TEXT = "Settings";
    private static final String UI_NAVIGATION_DRAWER_MYLIBRARY_TEXT = "My library";
    private static final String UI_OPTION_MENU_READ_ALOUD_TEXT = "Read aloud";
    private static final String UI_TURN_SYNC_ON_TEXT = "TURN SYNC ON";
    private static final String UI_SKIP_TEXT = "SKIP";
    private static final String UI_FULL_SCREEN_READER = "reader";
    private static final String UI_PLAY_DRAWER_ROOT = "play_drawer_root";
    private static final String UI_BOOK_THUMBNAIL = "li_thumbnail";

    private static final long SKIP_DELAY = 2000; // 2 secs
    private static final long UI_ANIMATION_TIMEOUT = 2500; // 2.5 secs
    private static final long OPEN_BOOK_TIMEOUT = 10000; // 10 secs
    private static final long SYNCING_BOOKS_TIMEOUT = 10000; //10 secs

    public PlayBooksHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        return "com.google.android.apps.books";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        return "Play Books";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        UiObject2 skipButton = getSkipButton();
        if (skipButton != null) {
            skipButton.click();
            SystemClock.sleep(SKIP_DELAY);
        }
        UiObject2 turnSyncOnButton = getTurnSyncOnButton();
        if (turnSyncOnButton != null) {
            turnSyncOnButton.click();
            SystemClock.sleep(SYNCING_BOOKS_TIMEOUT);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToAllBooksTab() {
        closeOptionMenu();
        exitReadingMode();
        closeSettingPanel();
        openNavigationDrawer();
        UiObject2 myLibraryButton = getMyLibraryButton();
        Assert.assertNotNull("Can't find \"My Library\" button", myLibraryButton);
        myLibraryButton.click();
        UiObject2 allBooksButton = mDevice.wait(Until.findObject(
                By.text(UI_TAB_ALL_BOOKS_TEXT).clickable(true)),
                UI_ANIMATION_TIMEOUT);
        Assert.assertNotNull("Can't find \"ALL BOOKS\" tab button", allBooksButton);
        allBooksButton.click();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void openBook() {
        UiObject2 bookThumbNail = getBookThumbnail();
        Assert.assertNotNull("No book in \"ALL BOOKS\" library", bookThumbNail);
        bookThumbNail.click();
        mDevice.wait(Until.hasObject(
                By.res(UI_PACKAGE_NAME, UI_FULL_SCREEN_READER)),
                OPEN_BOOK_TIMEOUT);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void exitReadingMode() {
        UiObject2 exitBookButton = null;
        UiObject2 fullScreenReader = getFullScreenReader();
        if (fullScreenReader != null) {
            fullScreenReader.click();
            exitBookButton = mDevice.wait(
                    Until.findObject(By.desc(UI_EXIT_BOOK_DESC)),
                    UI_ANIMATION_TIMEOUT);
            Assert.assertNotNull("Fail to exit full screen reader mode", exitBookButton);
        } else {
            exitBookButton = getExitBookButton();
        }
        if (exitBookButton != null) {
            exitBookButton.click();
            boolean hasNavButton = mDevice.wait(Until.hasObject(
                    By.desc(UI_NAVIGATION_DRAWER_BUTTON_DESC)),
                    UI_ANIMATION_TIMEOUT);
            Assert.assertTrue("Fail to exit reading mode", hasNavButton);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToNextPage() {
        UiObject2 fullScreenReader = getFullScreenReader();
        if (fullScreenReader == null) {
            throw new IllegalStateException("Not on a full-screen page of a book");
        }
        int displayHeight = mDevice.getDisplayHeight();
        int displayWidth = mDevice.getDisplayWidth();
        int nextPageX = displayWidth - 1;
        int nextPageY = displayHeight / 2;
        mDevice.click(nextPageX, nextPageY);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToPreviousPage() {
        UiObject2 fullScreenReader = getFullScreenReader();
        if (fullScreenReader == null) {
            throw new IllegalStateException("Not on a full-screen page of a book");
        }
        int displayHeight = mDevice.getDisplayHeight();
        int displayWidth = mDevice.getDisplayWidth();
        int previousPageX = 0;
        int previousPageY = displayHeight / 2;
        mDevice.click(previousPageX, previousPageY);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void scrollToNextPage() {
        UiObject2 fullScreenReader = getFullScreenReader();
        if (fullScreenReader == null) {
            throw new IllegalStateException("Not on a full-screen page of a book");
        }
        fullScreenReader.scroll(Direction.RIGHT, 1.0f);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void scrollToPreviousPage() {
        UiObject2 fullScreenReader = getFullScreenReader();
        if (fullScreenReader == null) {
            throw new IllegalStateException("Not on a full-screen page of a book");
        }
        fullScreenReader.scroll(Direction.LEFT, 1.0f);
    }

    private void closeOptionMenu() {
        if (isOptionMenuExpanded()) {
            mDevice.pressBack();
        }
    }

    private void closeSettingPanel() {
        UiObject2 backButton = getBackButton();
        if (backButton != null) {
            backButton.click();
            boolean hasNavButton = mDevice.wait(Until.hasObject(
                    By.desc(UI_NAVIGATION_DRAWER_BUTTON_DESC)),
                    UI_ANIMATION_TIMEOUT);
            Assert.assertNotNull("Fail to close setting panel", hasNavButton);
        }
    }

    private void openNavigationDrawer() {
        if (isDrawerOpen()) {
            return;
        }
        UiObject2 navButton = getNavButton();
        Assert.assertNotNull("Unable to find navigation drawer button", navButton);
        navButton.click();
        waitForNavigationDrawerOpen();
    }

    private boolean isOptionMenuExpanded() {
        return mDevice.hasObject(By.text(UI_OPTION_MENU_READ_ALOUD_TEXT));
    }

    private boolean isDrawerOpen() {
        return mDevice.hasObject(By.res(UI_PACKAGE_NAME, UI_PLAY_DRAWER_ROOT));
    }

    private UiObject2 getSkipButton() {
        return mDevice.findObject(By.text(UI_SKIP_TEXT));
    }

    private UiObject2 getTurnSyncOnButton() {
        return mDevice.findObject(By.text(UI_TURN_SYNC_ON_TEXT));
    }

    private UiObject2 getFullScreenReader() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_FULL_SCREEN_READER));
    }

    private UiObject2 getExitBookButton() {
        return mDevice.findObject(By.desc(UI_EXIT_BOOK_DESC));
    }

    private UiObject2 getBackButton() {
        return mDevice.findObject(By.desc(UI_NAVIGATE_UP_DESC));
    }

    private UiObject2 getNavButton() {
        return mDevice.findObject(By.desc(UI_NAVIGATION_DRAWER_BUTTON_DESC));
    }

    private UiObject2 getMyLibraryButton() {
        return mDevice.findObject(By.text(UI_NAVIGATION_DRAWER_MYLIBRARY_TEXT).clickable(true));
    }

    private UiObject2 getBookThumbnail() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_BOOK_THUMBNAIL));
    }

    private void waitForNavigationDrawerOpen() {
        mDevice.wait(Until.hasObject(
                By.text(UI_NAVIGATION_DRAWER_SETTING_TEXT).clickable(true)),
                UI_ANIMATION_TIMEOUT);
    }
}