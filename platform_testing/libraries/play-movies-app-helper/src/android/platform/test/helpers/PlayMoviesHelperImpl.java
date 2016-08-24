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
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.SystemClock;
import android.platform.test.helpers.exceptions.UnknownUiException;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Configurator;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.Until;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.util.Log;
import android.widget.EditText;

import java.util.regex.Pattern;

public class PlayMoviesHelperImpl extends AbstractPlayMoviesHelper {
    private static final String LOG_TAG = PlayMoviesHelperImpl.class.getSimpleName();

    private static final String UI_PACKAGE = "com.google.android.videos";
    private static final String UI_NAV_DRAWER_ID = "play_drawer_list";
    private static final String UI_MOVIE_LIST_ID = "play_header_listview";

    private static final int SEARCH_MOVIES_SCROLL_RETRY = 4;
    private static final long APP_INIT_WAIT = 5000;

    private boolean mIsVersion3p8 = false;

    public PlayMoviesHelperImpl(Instrumentation instr) {
        super(instr);

        try {
            mIsVersion3p8 = getVersion().startsWith("3.8");
        } catch (NameNotFoundException e) {
            Log.e(LOG_TAG, String.format("Unable to find package by name, %s", getPackage()));
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void open() {
        long original = Configurator.getInstance().getWaitForIdleTimeout();
        Configurator.getInstance().setWaitForIdleTimeout(1500);

        super.open();

        Configurator.getInstance().setWaitForIdleTimeout(original);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        return UI_PACKAGE;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        return "Play Movies & TV";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        if (mIsVersion3p8) {
            BySelector nextButton = By.res(UI_PACKAGE, "end_button");
            int count = 0;
            while (mDevice.hasObject(nextButton) && count < 10) {
                mDevice.findObject(nextButton).click();
                mDevice.wait(Until.gone(nextButton), 1000);
                count += 1;
            }
            BySelector gotIt = By.textContains("Got It");
            count = 0;
            while (mDevice.hasObject(gotIt) && count < 3) {
                UiObject2 gotItButton = mDevice.findObject(gotIt);
                if (gotItButton != null) {
                    gotItButton.click();
                    mDevice.wait(Until.gone(gotIt), 1000);
                }
                count += 1;
            }
        } else {
            long original = Configurator.getInstance().getWaitForIdleTimeout();
            Configurator.getInstance().setWaitForIdleTimeout(1500);

            for (int retry = 0; retry < 5; retry++) {
                Pattern words = Pattern.compile("GET STARTED", Pattern.CASE_INSENSITIVE);
                UiObject2 startedButton = mDevice.wait(Until.findObject(By.text(words)), 5000);
                if (startedButton != null) {
                    startedButton.click();
                }
            }

            Configurator.getInstance().setWaitForIdleTimeout(original);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void openMoviesTab() {
        // Navigate to the Movies tab through the Navigation drawer
        openNavigationDrawer();
        Pattern myLibraryPattern = Pattern.compile("My Library", Pattern.CASE_INSENSITIVE);
        UiObject2 libraryButton = mDevice.findObject(By.text(myLibraryPattern).clickable(true));
        libraryButton.click();
        waitForNavigationDrawerClose();
        // Select the Movies tab if necessary
        UiObject2 moviesTab = getMoviesTab();
        if (moviesTab == null) {
            throw new UnknownUiException("Unable to find the movies tab.");
        }
        if (!moviesTab.isSelected()) {
            moviesTab.click();
            mDevice.waitForIdle();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void playMovie(String name) {
        UiObject2 title = null;
        for (int retry = 0; retry < SEARCH_MOVIES_SCROLL_RETRY; retry++) {
            title = mDevice.findObject(By.textContains(name));
            if (title == null) {
                UiObject2 scroller = mDevice.findObject(By.res(UI_PACKAGE, UI_MOVIE_LIST_ID));
                if (scroller != null) {
                    scroller.scroll(Direction.DOWN, 1.0f);
                }
            }
        }
        if (title == null) {
            throw new IllegalArgumentException(
                    String.format("Failed to find movie by name %s", name));
        }
        title.click();
        UiObject2 play = mDevice.wait(Until.findObject(By.res(UI_PACKAGE, "play")), 5000);
        if (play == null) {
            throw new UnknownUiException("Failed to find the play button.");
        }
        play.click();
        mDevice.waitForIdle();
    }

    private boolean isNavigationDrawerOpen () {
        return mDevice.hasObject(By.res(UI_PACKAGE, UI_NAV_DRAWER_ID));
    }

    private void openNavigationDrawer() {
        if (isNavigationDrawerOpen()) {
            return;
        }

        UiObject2 backButton = mDevice.findObject(By.pkg(getPackage()).desc("Navigate up"));
        if (backButton != null) {
            backButton.click();
            mDevice.wait(Until.findObject(By.desc("Show navigation drawer")), 5000);
        }

        UiObject2 navButton = mDevice.findObject(By.desc("Show navigation drawer"));
        if (navButton == null) {
            throw new UnknownUiException("Unable to find the navigation drawer button.");
        }
        navButton.click();
        waitForNavigationDrawerOpen();
    }

    private void waitForNavigationDrawerOpen() {
        mDevice.wait(Until.hasObject(By.text("Settings").clickable(true)), 2500);
    }

    private void waitForNavigationDrawerClose() {
        mDevice.wait(Until.gone(By.text("Settings").clickable(true)), 2500);
    }

    private UiObject2 getMoviesTab() {
        Pattern moviesText = Pattern.compile("MY MOVIES", Pattern.CASE_INSENSITIVE);
        UiObject2 tab = mDevice.findObject(By.text(moviesText));
        if (tab == null) {
            moviesText = Pattern.compile("MOVIES", Pattern.CASE_INSENSITIVE);
            tab = mDevice.findObject(By.text(moviesText));
        }
        return tab;
    }
}
