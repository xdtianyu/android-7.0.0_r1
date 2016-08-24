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
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.util.Log;
import android.webkit.WebView;
import android.widget.ListView;

import java.io.IOException;

public class ChromeHelperImpl extends AbstractChromeHelper {
    private static final String LOG_TAG = ChromeHelperImpl.class.getSimpleName();

    private static final String UI_MENU_BUTTON_ID = "menu_button";
    private static final String UI_SEARCH_BOX_ID = "search_box_text";
    private static final String UI_URL_BAR_ID = "url_bar";
    private static final String UI_VIEW_HOLDER_ID = "compositor_view_holder";
    private static final String UI_POSITIVE_BUTTON_ID = "positive_button";
    private static final String UI_NEGATIVE_BUTTON_ID = "negative_button";

    private static final long APP_INIT_WAIT = 10000;
    private static final long MAX_DIALOG_TRANSITION = 5000;
    private static final long PAGE_LOAD_TIMEOUT = 30 * 1000;
    private static final long ANIMATION_TIMEOUT = 3000;

    private String mPackageName;
    private String mLauncherName;

    public ChromeHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        if (mPackageName == null) {
            String prop = null;
            try {
                mDevice.executeShellCommand("getprop dev.chrome.package");
            } catch (IOException ioe) {
                // log but ignore
                Log.e(LOG_TAG, "IOException while getprop", ioe);
            }
            if (prop == null || prop.isEmpty()) {
                prop = "com.android.chrome";
            }
            mPackageName = prop;
        }
        return mPackageName;
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        if (mLauncherName == null) {
            String prop = null;
            try {
                mDevice.executeShellCommand("getprop dev.chrome.name");
            } catch (IOException ioe) {
                // log but ignore
                Log.e(LOG_TAG, "IOException while getprop", ioe);
            }
            if (prop == null || prop.isEmpty()) {
                prop = "Chrome";
            }
            mLauncherName = prop;
        }
        return mLauncherName;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        // Terms of Service
        UiObject2 tos = mDevice.wait(Until.findObject(By.res(getPackage(), "terms_accept")),
                APP_INIT_WAIT);
        if (tos != null) {
            tos.click();
        }

        if (!hasAccountRegistered()) {
            // Device has no accounts registered that Chrome recognizes
            // Select negative button to skip setup wizard sign in
            UiObject2 negative = mDevice.wait(Until.findObject(
                    By.res(getPackage(), UI_NEGATIVE_BUTTON_ID)), MAX_DIALOG_TRANSITION);

            if (negative != null) {
                negative.click();
            }
        } else {
            // Device has an account registered that Chrome recognizes
            // Press positive buttons until through setup wizard
            for (int i = 0; i < 4; i++) {
                if (!isInSetupWizard()) {
                    break;
                }

                UiObject2 positive = mDevice.wait(Until.findObject(
                        By.res(getPackage(), UI_POSITIVE_BUTTON_ID)), MAX_DIALOG_TRANSITION);
                if (positive != null) {
                    positive.click();
                }
            }
        }

        mDevice.wait(Until.findObject(By.res(getPackage(), UI_SEARCH_BOX_ID)),
                MAX_DIALOG_TRANSITION);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void openUrl(String url) {
        UiObject2 urlBar = getUrlBar();
        if (urlBar == null) {
            throw new IllegalStateException("Failed to detect a URL bar");
        }

        mDevice.waitForIdle();
        urlBar.setText(url);
        mDevice.pressEnter();
        waitForPageLoad();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void flingPage(Direction dir) {
        UiObject2 page = getWebPage();
        if (page != null) {
            int minDim = Math.min(
                    page.getVisibleBounds().width(), page.getVisibleBounds().height());
            page.setGestureMargin((int)Math.floor(minDim * 0.25));
            page.fling(dir);
        } else {
            Log.e(LOG_TAG, String.format("Failed to fling page %s", dir.toString()));
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void openMenu() {
        UiObject2 menuButton = null;
        for (int retries = 2; retries > 0; retries--) {
            menuButton = mDevice.findObject(By.desc("More options"));
            if (menuButton == null) {
                flingPage(Direction.UP);
            } else {
                break;
            }
        }

        if (menuButton == null) {
            throw new IllegalStateException("Unable to find menu button.");
        }
        menuButton.clickAndWait(Until.newWindow(), 5000);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void mergeTabs() {
        openSettings();
        mDevice.findObject(By.text("Merge tabs and apps")).click();
        if (mDevice.findObject(By.text("On")) != null) {
            // Merge tabs is already on
            mDevice.pressBack();
            mDevice.pressBack();
        } else {
            mDevice.findObject(By.res(getPackage(), "switch_widget")).click();
            mDevice.findObject(By.text("OK")).click();
        }
        SystemClock.sleep(5000);
        waitForPageLoad();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void unmergeTabs() {
        openSettings();
        mDevice.findObject(By.text("Merge tabs and apps")).click();
        if (mDevice.findObject(By.text("Off")) != null) {
            // Merge tabs is already off
            mDevice.pressBack();
            mDevice.pressBack();
        } else {
            mDevice.findObject(By.res(getPackage(), "switch_widget")).click();
            mDevice.findObject(By.text("OK")).click();
        }
        SystemClock.sleep(5000);
        waitForPageLoad();
    }

    private void openSettings() {
        openMenu();
        UiObject2 menu = getMenu();
        // TODO: Change this to be non-constant
        menu.setGestureMargin(500);
        menu.scroll(Direction.DOWN, 1.0f);
        // Open the Settings menu
        mDevice.findObject(By.desc("Settings")).clickAndWait(Until.newWindow(), 3000);
    }

    private UiObject2 getWebPage() {
        mDevice.waitForIdle();

        UiObject2 webView = mDevice.findObject(By.clazz(WebView.class));
        if (webView != null) {
            return webView;
        }

        UiObject2 viewHolder = mDevice.findObject(
                By.res(getPackage(), UI_VIEW_HOLDER_ID));
        return viewHolder;
    }

    private UiObject2 getUrlBar() {
        // First time, URL bar is has id SEARCH_BOX_ID
        UiObject2 urlLoc = mDevice.findObject(By.res(getPackage(), UI_SEARCH_BOX_ID));
        if (urlLoc != null) {
            urlLoc.click();
            // Waits for the animation to complete.
            mDevice.wait(Until.findObject(By.res(getPackage(), UI_URL_BAR_ID)), ANIMATION_TIMEOUT);
        }

        // Afterwards, URL bar has id URL_BAR_ID; must re-select
        for (int retries = 2; retries > 0; retries--) {
            urlLoc = mDevice.findObject(By.res(getPackage(), UI_URL_BAR_ID));
            if (urlLoc == null) {
                flingPage(Direction.UP);
            } else {
                break;
            }
        }

        if (urlLoc != null) {
            urlLoc.click();
        } else {
            throw new IllegalStateException("Failed to find a URL bar.");
        }

        return urlLoc;
    }

    private UiObject2 getMenu() {
        return mDevice.findObject(By.clazz(ListView.class).pkg(getPackage()));
    }

    private void waitForPageLoad() {
        mDevice.waitForIdle();
        if (mDevice.hasObject(By.desc("Stop page loading"))) {
            mDevice.wait(Until.gone(By.desc("Stop page loading")), PAGE_LOAD_TIMEOUT);
        } else if (mDevice.hasObject(By.res(getPackage(), "progress"))) {
            mDevice.wait(Until.gone(By.res(getPackage(), "progress")), PAGE_LOAD_TIMEOUT);
        }
    }

    private boolean isInSetupWizard() {
        return mDevice.hasObject(By.res(getPackage(), "fre_pager"));
    }

    private boolean hasAccountRegistered() {
        boolean addAcountTextPresent = mDevice.wait(Until.hasObject(By.textStartsWith("Add an " +
                "account")), MAX_DIALOG_TRANSITION);

        UiObject2 next = mDevice.wait(Until.findObject(
                By.res(getPackage(), UI_POSITIVE_BUTTON_ID)), MAX_DIALOG_TRANSITION);
        boolean signInButtonPresent =  next != null && "SIGN IN".equals(next.getText());

        // If any of theese elements is present, then there is no account registered.
        return !addAcountTextPresent && !signInButtonPresent;
    }
}
