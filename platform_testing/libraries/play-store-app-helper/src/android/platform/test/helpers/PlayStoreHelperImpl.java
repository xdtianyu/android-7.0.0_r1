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
import android.widget.EditText;

import junit.framework.Assert;

public class PlayStoreHelperImpl extends AbstractPlayStoreHelper {
    private static final String LOG_TAG = PlayStoreHelperImpl.class.getSimpleName();
    private static final String UI_PACKAGE = "com.android.vending";

    public PlayStoreHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        return "com.android.vending";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        return "Play Store";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        UiObject2 tos = mDevice.findObject(By.res(UI_PACKAGE, "positive_button"));
        if (tos != null) {
            tos.clickAndWait(Until.newWindow(), 5000);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void doSearch(String query) {
        // Back up and scroll up until search is visible
        for (int retries = 3; retries > 0; retries--) {
            if (getSearchBox() != null) {
                break;
            } else {
                UiObject2 scroller = getScrollContainer();
                if (scroller != null) {
                    scroller.scroll(Direction.UP, 100.0f);
                } else {
                    mDevice.pressBack();
                }
            }
        }

        //Interact with the search box
        UiObject2 searchBox = getSearchBox();
        if (searchBox != null) {
            searchBox.click();
        } else {
            Assert.fail("Unable to select search box.");
        }
        UiObject2 edit = mDevice.wait(
                Until.findObject(By.clazz(EditText.class)), 5000);
        Assert.assertNotNull("Could not find edit box", edit);
        edit.setText(query);
        mDevice.pressEnter();

        // Wait until the search results container is open
        Assert.assertTrue("Could not find search results",
                mDevice.wait(Until.hasObject(By.res(UI_PACKAGE, "search_results_list")), 5000));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void selectFirstResult() {
        try {
            if (getVersion().startsWith("5.")) {
                expandSection("Apps");
            }
        } catch (NameNotFoundException e) {
            Log.e(LOG_TAG, "Unable to find version for package: " + UI_PACKAGE);
        }
        UiObject2 result = mDevice.findObject(By.res(UI_PACKAGE, "play_card"));
        Assert.assertNotNull("Failed to find a result card", result);
        result.click();
    }

    private void expandSection(String header) {
        for (int retries = 3; retries > 0; retries--) {
            BySelector section = By.res(UI_PACKAGE, "header_title_main").text(header);
            UiObject2 title = mDevice.findObject(section);
            if (title != null) {
                title.click();
                mDevice.wait(Until.gone(section), 5000);
                return;
            } else {
                UiObject2 container = mDevice.findObject(By.res(UI_PACKAGE, "search_results_list"));
                container.scroll(Direction.DOWN, 1.0f);
            }
        }
        Assert.fail("Failed to find section header.");
    }

    private UiObject2 getSearchBox() {
        UiObject2 searchBox = mDevice.findObject(By.res(UI_PACKAGE, "search_box_idle_text"));
        if (searchBox == null) {
            searchBox = mDevice.findObject(By.res(UI_PACKAGE, "search_button"));
        }
        return searchBox;
    }

    private UiObject2 getScrollContainer() {
        UiObject2 scroller = mDevice.findObject(By.res(UI_PACKAGE, "recycler_view"));
        if (scroller == null) {
            scroller = mDevice.findObject(By.res(UI_PACKAGE, "viewpager"));
        }
        return scroller;
    }
}

