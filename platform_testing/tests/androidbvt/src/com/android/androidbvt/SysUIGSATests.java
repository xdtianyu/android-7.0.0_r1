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

package com.android.androidbvt;

import android.app.UiAutomation;
import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.test.suitebuilder.annotation.LargeTest;
import android.view.KeyEvent;

import junit.framework.TestCase;

import java.util.List;

/**
 * Contains tests for features that are loosely coupled with Android system for sanity
 */
public class SysUIGSATests extends TestCase {
    private final String QSB_PKG = "com.google.android.googlequicksearchbox";
    private UiAutomation mUiAutomation = null;
    private UiDevice mDevice;
    private Context mContext = null;
    private AndroidBvtHelper mABvtHelper = null;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mDevice.setOrientationNatural();
        mContext = InstrumentationRegistry.getTargetContext();
        mUiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
        mABvtHelper = AndroidBvtHelper.getInstance(mDevice, mContext, mUiAutomation);
        mDevice.pressMenu();
        mDevice.pressHome();
    }

    @Override
    public void tearDown() throws Exception {
        mDevice.pressHome();
        mDevice.unfreezeRotation();
        super.tearDown();
    }

    /**
     * Ensures search via QSB searches both web and device apps Suuggested texts starts with
     * searched text Remembers searched item, suggests as top suggestion next time
     */
    @LargeTest
    public void testGoogleQuickSearchBar() throws InterruptedException {
        final String TextToSearch = "co";
        UiObject2 searchBox = null;
        int counter = 5;
        while (--counter > 0
                && ((searchBox = mDevice.wait(Until.findObject(By.res(QSB_PKG, "search_box")),
                        mABvtHelper.SHORT_TIMEOUT)) == null)) {
            Thread.sleep(mABvtHelper.SHORT_TIMEOUT);
            mDevice.pressHome();
            mDevice.pressSearch();
        }
        mDevice.wait(Until.findObject(By.res(QSB_PKG, "search_box")),
                mABvtHelper.LONG_TIMEOUT).setText(TextToSearch);
        Thread.sleep(mABvtHelper.LONG_TIMEOUT);
        // make the IME down
        mDevice.pressKeyCode(KeyEvent.KEYCODE_BACK);
        // searching for 'co' will result from web, as well as 'Contacts' app. So there should be
        // more than 1 container
        UiObject2 searchSuggestionsContainer = mDevice.wait(Until.findObject(By.res(
                QSB_PKG, "search_suggestions_container")), mABvtHelper.LONG_TIMEOUT);
        assertTrue("QS suggestion should have more than 1 container",
                searchSuggestionsContainer.getChildCount() > 1);
        UiObject2 searchSuggestions = mDevice.wait(Until.findObject(By.res(
                QSB_PKG, "search_suggestions_web")), mABvtHelper.LONG_TIMEOUT);
        assertNotNull(
                "Web Search suggestions shouldn't be null & should have more than 1 suggestions",
                searchSuggestions != null && searchSuggestions.getChildCount() > 1);
        List<UiObject2> suggestions = mDevice.wait(Until.findObjects(By.res(QSB_PKG, "text_1")),
                mABvtHelper.LONG_TIMEOUT);
        assertNotNull("Contacts app should be found", mDevice.wait(Until.findObject(
                By.res(QSB_PKG, "text_1").text("Contacts")), mABvtHelper.LONG_TIMEOUT));
        String topSuggestedText = suggestions.get(0).getText();
        suggestions.get(0).clickAndWait(Until.newWindow(), mABvtHelper.LONG_TIMEOUT);
        Thread.sleep(mABvtHelper.LONG_TIMEOUT);
        // Search again and ensure last searched item showed as top suggestion
        mDevice.pressHome();
        Thread.sleep(mABvtHelper.SHORT_TIMEOUT);
        mDevice.pressSearch();
        String currentTopSuggestion = mDevice.wait(Until.findObjects(By.res(QSB_PKG, "text_1")),
                mABvtHelper.LONG_TIMEOUT).get(0).getText();
        assertTrue("Previous searched item isn't top suggested word",
                topSuggestedText.toLowerCase().equals(topSuggestedText.toLowerCase()));
    }

    /**
     * Ensures if any account is opted in GoogleNow, Google-assist offers card on long home press
     */
    @LargeTest
    public void testGoogleAssist() throws InterruptedException {
        mDevice.wait(Until.findObject(By.res(QSB_PKG, "search_plate")),
                mABvtHelper.SHORT_TIMEOUT).click();
        Thread.sleep(mABvtHelper.SHORT_TIMEOUT);
        UiObject2 getStarted = mDevice.wait(Until.findObject(By.text("GET STARTED")),
                mABvtHelper.SHORT_TIMEOUT);
        if (getStarted != null) {
            getStarted.clickAndWait(Until.newWindow(), mABvtHelper.SHORT_TIMEOUT);
            mDevice.wait(Until.findObject(By.res(QSB_PKG, "text_container")),
                    mABvtHelper.SHORT_TIMEOUT).swipe(Direction.UP, 1.0f);
            mDevice.wait(Until.findObject(By.text("YES, I’M IN")),
                    mABvtHelper.SHORT_TIMEOUT)
                    .clickAndWait(Until.newWindow(), mABvtHelper.SHORT_TIMEOUT);
        }
        // Search for Paris and click on first suggested text
        mDevice.wait(Until.findObject(By.res(QSB_PKG, "text_container")),
                mABvtHelper.LONG_TIMEOUT).setText("Paris");
        Thread.sleep(mABvtHelper.LONG_TIMEOUT);
        List<UiObject2> suggestedTexts = null;
        int counter = 5;
        while (--counter > 0
                && ((suggestedTexts = mDevice.wait(Until.findObjects(By.res(QSB_PKG, "text_1")),
                        mABvtHelper.LONG_TIMEOUT)) == null)) {
            Thread.sleep(mABvtHelper.SHORT_TIMEOUT);
        }
        assertNotNull("Suggested text shouldn't be null", suggestedTexts);
        UiObject2 itemToClick = suggestedTexts.get(0);
        for (UiObject2 item : suggestedTexts) {
            if (item.getText().toLowerCase().equals("paris")) {
                itemToClick = item;
            }
        }
        itemToClick.clickAndWait(Until.newWindow(), mABvtHelper.SHORT_TIMEOUT);
        Thread.sleep(mABvtHelper.LONG_TIMEOUT);
        // Now long press home to load assist layer
        mDevice.pressKeyCode(KeyEvent.KEYCODE_ASSIST);
        UiObject2 optInYes = mDevice.wait(
                Until.findObject(By.res(QSB_PKG, "screen_assist_opt_in_yes")),
                mABvtHelper.SHORT_TIMEOUT);
        if (optInYes != null) {
            optInYes.clickAndWait(Until.newWindow(), mABvtHelper.SHORT_TIMEOUT);
        }
        // Ensure some cards are loaded
        // Note card's content isn't verified
        counter = 5;
        UiObject2 cardContainer = null;
        while (--counter > 0 && ((cardContainer = mDevice.wait(
                Until.findObject(By.res(QSB_PKG, "card_container")),
                mABvtHelper.SHORT_TIMEOUT)) != null)) {
            Thread.sleep(mABvtHelper.SHORT_TIMEOUT);
        }
        assertNotNull("Some cards should be loaded", cardContainer);
    }
}
