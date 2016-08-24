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
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.Until;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.util.Log;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.TextView;

import java.util.regex.Pattern;

import junit.framework.Assert;

public class PlayMusicHelperImpl extends AbstractPlayMusicHelper {
    private static final String LOG_TAG = PlayMusicHelperImpl.class.getSimpleName();
    private static final String UI_PACKAGE = "com.google.android.music";

    private static final String UI_TAB_HEADER_ID = "play_header_list_tab_scroll";
    private static final String UI_PAUSE_PLAY_BUTTON_ID = "play_pause_header";

    private static final long APP_LOAD_WAIT = 10000;
    private static final long APP_INIT_WAIT = 10000;
    private static final long TAB_TRANSITION_WAIT = 5000;
    private static final long EXPAND_WAIT = 5000;
    private static final long NAV_BAR_WAIT = 5000;
    private static final long TOGGLE_PAUSE_PLAY_WAIT = 5000;

    public PlayMusicHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        return "com.google.android.music";
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        return "Play Music";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        // Dismiss "Add account" Dialog
        UiObject2 skipButton = mDevice.wait(Until.findObject(By.res(UI_PACKAGE, "skip_button")),
                APP_LOAD_WAIT);
        if (skipButton != null) {
            skipButton.clickAndWait(Until.newWindow(), APP_INIT_WAIT);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToTab(String tabTitle) {
        if (isLibraryTabSelected(tabTitle)) {
            return;
        } else {
            // Go to the "Library" page
            goToMyLibrary();

            for (int retries = 3; retries > 0; retries--) {
                UiObject2 title = getLibraryTab(tabTitle);
                if (title != null) {
                    title.click();
                    Assert.assertTrue(
                            String.format("Tab %s was not found selected", tabTitle.toUpperCase()),
                            mDevice.wait(
                                    Until.hasObject(getLibraryTabSelector(tabTitle).selected(true)),
                                    TAB_TRANSITION_WAIT));
                } else {
                    UiObject2 headerList = mDevice.findObject(By.res(UI_PACKAGE, UI_TAB_HEADER_ID));
                    Assert.assertNotNull("Could not find library header to scroll.", headerList);
                    headerList.scroll(Direction.RIGHT, 1.0f);
                }
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void selectSong(String album, String song) {
        UiObject2 albumItem = mDevice.wait(Until.findObject(By.res(UI_PACKAGE, "li_title")
                .textStartsWith(album)), EXPAND_WAIT);
        Assert.assertNotNull("Unable to find album item", albumItem);
        albumItem.click();

        mDevice.wait(Until.findObject(By.res(UI_PACKAGE, "title").textStartsWith(album)),
                EXPAND_WAIT);

        for (int retries = 5; retries > 0; retries--) {
            UiObject2 songItem = mDevice.findObject(By.res(UI_PACKAGE, "li_title").
                    textStartsWith(song));
            if (songItem != null) {
                songItem.click();
                mDevice.wait(Until.findObject(
                        By.res(UI_PACKAGE, "trackname").textStartsWith(song)), EXPAND_WAIT);

                // Waits for the animation to complete.
                mDevice.waitForIdle();
                return;
            } else {
                UiObject2 scroller = mDevice.findObject(
                        By.scrollable(true));
                scroller.setGestureMargin(500);
                scroller.scroll(Direction.DOWN, 1.0f);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void pauseSong() {
        BySelector selector1play = By.res(UI_PACKAGE, UI_PAUSE_PLAY_BUTTON_ID).desc("Play");
        BySelector selector1pause = By.res(UI_PACKAGE, UI_PAUSE_PLAY_BUTTON_ID).desc("Pause");
        BySelector selector2play = By.res(UI_PACKAGE, "pause").desc("Play");
        BySelector selector2pause = By.res(UI_PACKAGE, "pause").desc("Pause");

        UiObject2 button = null;
        if ((button = mDevice.findObject(selector1play)) != null) {
            return;
        } else if ((button = mDevice.findObject(selector1pause)) != null) {
            button.click();
            mDevice.wait(Until.findObject(selector1play), TOGGLE_PAUSE_PLAY_WAIT);
        } else if ((button = mDevice.findObject(selector2play)) != null) {
            return;
        } else if ((button = mDevice.findObject(selector2pause)) != null) {
            button.click();
            mDevice.wait(Until.findObject(selector2play), TOGGLE_PAUSE_PLAY_WAIT);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void playSong() {
        BySelector selector1play = By.res(UI_PACKAGE, UI_PAUSE_PLAY_BUTTON_ID).desc("Play");
        BySelector selector1pause = By.res(UI_PACKAGE, UI_PAUSE_PLAY_BUTTON_ID).desc("Pause");
        BySelector selector2play = By.res(UI_PACKAGE, "pause").desc("Play");
        BySelector selector2pause = By.res(UI_PACKAGE, "pause").desc("Pause");

        UiObject2 button = null;
        if ((button = mDevice.findObject(selector1pause)) != null) {
            return;
        } else if ((button = mDevice.findObject(selector1play)) != null) {
            button.click();
            mDevice.wait(Until.findObject(selector1pause), TOGGLE_PAUSE_PLAY_WAIT);
        } else if ((button = mDevice.findObject(selector2pause)) != null) {
            return;
        } else if ((button = mDevice.findObject(selector2play)) != null) {
            button.click();
            mDevice.wait(Until.findObject(selector2pause), TOGGLE_PAUSE_PLAY_WAIT);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void expandMediaControls() {
        UiObject2 header = mDevice.findObject(By.res(UI_PACKAGE, "trackname"));
        Assert.assertNotNull("Unable to find header to expand media controls.", header);
        header.click();
        mDevice.wait(Until.findObject(By.res(UI_PACKAGE, "lightsUpInterceptor")), EXPAND_WAIT);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void pressShuffleAll() {
        if (!isLibraryTabSelected("Songs")) {
            throw new IllegalStateException("The Songs tab was not selected");
        }

        UiObject2 shuffleAll = mDevice.findObject(By.text("SHUFFLE ALL"));
        Assert.assertNotNull("Could not find a 'SHUFFLE ALL' button.", shuffleAll);
        shuffleAll.click();
        Assert.assertTrue("Did not detect a song playing", mDevice.wait(Until.hasObject(
            By.res(UI_PACKAGE, UI_PAUSE_PLAY_BUTTON_ID)), TOGGLE_PAUSE_PLAY_WAIT));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void pressRepeat() {
        UiObject2 repeatButton = mDevice.findObject(By.res(UI_PACKAGE, "repeat"));
        Assert.assertNotNull("Unable to find repeat button to press.", repeatButton);
        repeatButton.click();
        mDevice.waitForIdle();
    }

    private void goToMyLibrary() {
        // Select for the title: "Library"
        if (mDevice.findObject(getLibraryTextSelector().clickable(false)) != null) {
            return;
        }

        openNavigationBar();
        // Select for the button: "Library"
        mDevice.findObject(getLibraryTextSelector().clickable(true)).click();
        mDevice.wait(Until.gone(By.res(UI_PACKAGE, "play_drawer_root")), NAV_BAR_WAIT);
    }

    private void openNavigationBar () {
        UiObject2 navBar = getNavigationBarButton();
        Assert.assertNotNull("Did not find navigation drawer button.", navBar);
        navBar.click();
        mDevice.wait(Until.findObject(By.res(UI_PACKAGE, "play_drawer_root")), NAV_BAR_WAIT);
    }

    private UiObject2 getNavigationBarButton() {
        return mDevice.findObject(By.desc("Show navigation drawer"));
    }

    private boolean isLibraryTabSelected(String tabTitle) {
        return mDevice.hasObject(getLibraryTabSelector(tabTitle).selected(true));
    }

    private UiObject2 getLibraryTab(String tabTitle) {
        return mDevice.findObject(getLibraryTabSelector(tabTitle));
    }

    private BySelector getLibraryTabSelector(String tabTitle) {
        return By.res(UI_PACKAGE, "title").text(tabTitle.toUpperCase());
    }

    private BySelector getLibraryTextSelector() {
        String libraryText = "Music library";
        Pattern libraryTextPattern = Pattern.compile(libraryText, Pattern.CASE_INSENSITIVE);
        return By.text(libraryTextPattern);
    }
}
