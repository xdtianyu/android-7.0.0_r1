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

import java.util.regex.Pattern;


public class MapsHelperImpl extends AbstractMapsHelper {
    private static final String LOG_TAG = MapsHelperImpl.class.getSimpleName();

    private static final String UI_CLOSE_NAVIGATION_DESC = "Close navigation";
    private static final String UI_DIRECTIONS_BUTTON_ID = "placepage_directions_button";
    private static String UI_PACKAGE;
    private static final String UI_START_NAVIGATION_BUTTON_ID = "start_button";
    private static final String UI_TEXTVIEW_CLASS = "android.widget.TextView";
    private static final String UI_PROGRESSBAR_CLASS = "android.widget.ProgressBar";

    private static final int UI_RESPONSE_WAIT = 5000;
    private static final int SEARCH_RESPONSE_WAIT = 25000;
    private static final int MAP_SERVER_CONNECT_WAIT = 120000;

    private boolean mIsVersion9p30;

    private static final int MAX_CONNECT_TO_SERVER_RETRY = 5;
    private static final int MAX_START_NAV_RETRY = 5;
    private static final int MAX_DISMISS_INITIAL_DIALOG_RETRY = 2;

    public MapsHelperImpl(Instrumentation instr) {
        super(instr);

        try {
            mIsVersion9p30 = getVersion().startsWith("9.30.");
            if (mIsVersion9p30) {
                UI_PACKAGE = "com.google.android.apps.maps";
            } else {
                UI_PACKAGE = "com.google.android.apps.gmm";
            }
        } catch (NameNotFoundException e) {
            Log.e(LOG_TAG, String.format("Unable to find package by name, %s", getPackage()));
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        return "com.google.android.apps.maps";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        return "Maps";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        Log.v(LOG_TAG, "Maps dismissing initial welcome screen.");

        // ToS welcome dialog
        boolean successTosDismiss = hasSearchBar(0);

        String text = "ACCEPT & CONTINUE";
        Pattern pattern = Pattern.compile(text, Pattern.CASE_INSENSITIVE);

        UiObject2 terms = mDevice.wait(Until.findObject(By.text(pattern)), 10000);
        int tryCounter = 0;

        while ((terms != null) && (tryCounter < MAX_DISMISS_INITIAL_DIALOG_RETRY)) {
            terms.click();

            mDevice.wait(Until.gone(By.pkg(UI_PACKAGE).clazz(UI_PROGRESSBAR_CLASS)),
                         MAP_SERVER_CONNECT_WAIT);

            if (!checkServerConnectivity()) {
                throw new IllegalStateException("Unable to connect to Google Maps server");
            }

            terms = mDevice.wait(Until.findObject(By.text(pattern)), UI_RESPONSE_WAIT);
            tryCounter += 1;
        }

        if (terms != null) {
            throw new IllegalStateException("Unable to dismiss initial dialogs");
        }

        if (mIsVersion9p30) {
            exit();
            open();
        }

        // Location services dialog
        text = "YES, I'M IN";
        pattern = Pattern.compile(text, Pattern.CASE_INSENSITIVE);
        UiObject2 location = mDevice.wait(Until.findObject(By.text(pattern)),
                                          UI_RESPONSE_WAIT);
        if (location != null) {
            location.click();
            mDevice.waitForIdle();
        } else {
            Log.e(LOG_TAG, "Did not find a location services dialog.");
        }

        if (!mIsVersion9p30) {
            // Tap here dialog
            UiObject2 cling = mDevice.wait(
                                Until.findObject(By.res(UI_PACKAGE, "tapherehint_textbox")),
                                UI_RESPONSE_WAIT);
            if (cling != null) {
                cling.click();
                mDevice.waitForIdle();
            } else {
                Log.e(LOG_TAG, "Did not find 'tap here' dialog");
            }

            // Reset map dialog
            UiObject2 resetView = mDevice.wait(
                                     Until.findObject(By.res(UI_PACKAGE, "mylocation_button")),
                                     UI_RESPONSE_WAIT);
            if (resetView != null) {
                resetView.click();
                mDevice.waitForIdle();
            } else {
                Log.e(LOG_TAG, "Did not find 'reset map' dialog.");
            }
        }

        // 'Side menu' dialog
        text = "GOT IT";
        pattern = Pattern.compile(text, Pattern.CASE_INSENSITIVE);
        BySelector gotIt = By.text(Pattern.compile("GOT IT", Pattern.CASE_INSENSITIVE));
        UiObject2 sideMenuTut = mDevice.wait(Until.findObject(gotIt), 5000);
        if (sideMenuTut != null) {
            sideMenuTut.click();
        } else {
            Log.e(LOG_TAG, "Did not find any 'side menu' dialog.");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void doSearch(String query) {
        Log.v(LOG_TAG, "Maps doing an address search");

        if (!checkServerConnectivity()) {
            throw new IllegalStateException("Cannot connect to Google Maps servers");
        }

        // Navigate if necessary
        goToQueryScreen();
        // Select search bar
        UiObject2 searchSelect = getSelectableSearchBar(0);
        if (searchSelect == null) {
            throw new IllegalStateException("No selectable search bar found.");
        }
        searchSelect.click();

        // Edit search query
        UiObject2 searchEdit = getEditableSearchBar(UI_RESPONSE_WAIT);
        if (searchEdit == null) {
            throw new IllegalStateException("Not editable search bar found.");
        }
        searchEdit.clear();
        searchEdit.setText(query);

        // Search and wait for the directions option
        UiObject2 firstAddressResult = mDevice.wait(Until.findObject(By.pkg(UI_PACKAGE).clazz(
            UI_TEXTVIEW_CLASS)), SEARCH_RESPONSE_WAIT);
        if (firstAddressResult == null) {
            String err_msg = String.format("Did not detect address result after %d seconds",
                                           (int) Math.floor(SEARCH_RESPONSE_WAIT / 1000));
            throw new IllegalStateException(err_msg);
        }
        firstAddressResult.click();

        if (getDirectionsButton(SEARCH_RESPONSE_WAIT) == null) {
            throw new IllegalStateException("Could not find directions button");
        }
    }

    /*
     * {@inheritDoc}
     */
    @Override
    public void getDirections() {
        Log.v(LOG_TAG, "Maps getting direction.");

        dismissPullUpDialog();

        UiObject2 directionsButton = getDirectionsButton(UI_RESPONSE_WAIT);
        if (directionsButton == null) {
            throw new IllegalStateException("Unable to find start direction button");
        }
        directionsButton.click();

        dismissGetARidePopUp();
        if (getStartNavigationButton(UI_RESPONSE_WAIT) == null) {
            throw new IllegalStateException("Unable to find start navigation button");
        }
    }

    /*
     * {@inheritDoc}
     */
    @Override
    public void startNavigation() {
        Log.v(LOG_TAG, "starting navigation.");

        UiObject2 startNavigationButton = getStartNavigationButton(UI_RESPONSE_WAIT);

        if (startNavigationButton == null) {
            dismissGetARidePopUp();
            startNavigationButton = getStartNavigationButton(UI_RESPONSE_WAIT);

            if (startNavigationButton == null) {
                throw new IllegalStateException("Unable to find start navigation button");
            }
        }
        startNavigationButton.click();

        boolean hasCloseNavigationDesc = (getCloseNavigationButton(UI_RESPONSE_WAIT) != null);
        int tryCounter = 0;
        while ((tryCounter < MAX_START_NAV_RETRY) && (!hasCloseNavigationDesc)) {
            dismissBetaUseDialog();
            dismissSearchAlongRoutePopUp();
            hasCloseNavigationDesc = (getCloseNavigationButton(UI_RESPONSE_WAIT) != null);
            tryCounter += 1;
        }

        if (!hasCloseNavigationDesc) {
            throw new IllegalStateException("Unable to find close navigation button");
        }
    }

    /*
     * {@inheritDoc}
     */
    @Override
    public void stopNavigation() {
        Log.v(LOG_TAG, "stopping navigation.");

        dismissSearchAlongRoutePopUp();

        UiObject2 closeNavigationButton = getCloseNavigationButton(0);

        if (closeNavigationButton != null) {
            closeNavigationButton.click();
        }

        if (hasNavigationButton(UI_RESPONSE_WAIT)) {
            mDevice.pressBack();
        }
    }

    private void goToQueryScreen() {
        for (int backup = 5; backup > 0; backup--) {
            if (hasSearchBar(0)) {
                return;
            } else {
                mDevice.pressBack();
                mDevice.waitForIdle();
            }
        }
    }

    private UiObject2 getSelectableSearchBar(int wait_time) {
        return mDevice.wait(Until.findObject(By.res(UI_PACKAGE, "search_omnibox_text_box")),
                            wait_time);
    }

    private UiObject2 getEditableSearchBar(int wait_time) {
        return mDevice.wait(Until.findObject(By.res(UI_PACKAGE, "search_omnibox_edit_text")),
                            wait_time);
    }

    private UiObject2 getStartNavigationButton(int wait_time) {
        return mDevice.wait(Until.findObject(By.res(UI_PACKAGE, UI_START_NAVIGATION_BUTTON_ID)),
                            wait_time);
    }

    private UiObject2 getCloseNavigationButton(int wait_time) {
        return mDevice.wait(Until.findObject(By.pkg(UI_PACKAGE).desc(UI_CLOSE_NAVIGATION_DESC)),
                            wait_time);
    }

    private UiObject2 getDirectionsButton(int wait_time) {
        return mDevice.wait(Until.findObject(By.res(UI_PACKAGE, UI_DIRECTIONS_BUTTON_ID)),
                            wait_time);
    }

    private boolean hasSearchBar(int wait_time) {
        return ((getSelectableSearchBar(wait_time) != null) ||
                (getEditableSearchBar(wait_time) != null));
    }

    private boolean hasNavigationButton(int wait_time) {
        return ((getStartNavigationButton(wait_time) != null) ||
                (getDirectionsButton(wait_time) != null));
    }

    // check connectivity issues by looking for "TRY AGAIN" pop-up dialog
    private boolean checkServerConnectivity() {
        int tryCounter = 0;

        UiObject2 tryAgainButton = mDevice.wait(Until.findObject(By.text("TRY AGAIN")),
                                                UI_RESPONSE_WAIT);
        while ((tryCounter < MAX_CONNECT_TO_SERVER_RETRY) && (tryAgainButton != null)) {
            tryAgainButton.click();

            tryAgainButton = mDevice.wait(Until.findObject(By.text("TRY AGAIN")),
                                          MAP_SERVER_CONNECT_WAIT);
            tryCounter += 1;
        }

        if (tryAgainButton != null) {
            return false;
        }
        else {
            return true;
        }
    }

    // Dismiss pop up dialog with title "Google Maps Navigation is in beta.  Use caution"
    private void dismissBetaUseDialog() {
        UiObject2 acceptButton = mDevice.wait(
                                   Until.findObject(By.text("ACCEPT")),
                                   UI_RESPONSE_WAIT);
        if (acceptButton != null) {
            acceptButton.click();
            mDevice.wait(Until.gone(By.text("ACCEPT")), UI_RESPONSE_WAIT);
        }
    }

    // Dismiss pop-up dialog with title "Search along route"
    private void dismissSearchAlongRoutePopUp() {
        UiObject2 searchAlongRoute = mDevice.wait(
                                       Until.findObject(By.textContains("Search along route")),
                                       UI_RESPONSE_WAIT);
        if (searchAlongRoute != null) {
            mDevice.pressBack();
        }
    }

    // Dismiss pop-up dialog with title "Pull up"
    private void dismissPullUpDialog() {
        UiObject2 gotItButton = mDevice.wait(
                                  Until.findObject(By.text("GOT IT")),
                                  UI_RESPONSE_WAIT);
        if (gotItButton != null) {
            gotItButton.click();
            mDevice.wait(Until.gone(By.text("GOT IT")), UI_RESPONSE_WAIT);
        }
    }

    // Dismiss pop-up advertising for taxi-ride with title "Get a ride in minutes"
    private void dismissGetARidePopUp() {
        UiObject2 getARide = mDevice.wait(
                               Until.findObject(By.textContains("Get a ride in minutes")),
                               UI_RESPONSE_WAIT);
        if (getARide != null) {
            mDevice.pressBack();
        }
    }
}
