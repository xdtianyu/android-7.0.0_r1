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

package android.launcher.functional;

import java.io.File;
import java.io.IOException;

import android.app.UiAutomation;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.Context;
import android.graphics.Point;
import android.os.PowerManager;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.launcherhelper.ILauncherStrategy;
import android.support.test.launcherhelper.LauncherStrategyFactory;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.Log;
import android.view.KeyEvent;

public class HomeScreenTests extends InstrumentationTestCase {

    private static final int TIMEOUT = 3000;
    private static final String HOTSEAT = "hotseat";
    private UiDevice mDevice;
    private PackageManager mPackageManager;
    private ILauncherStrategy mLauncherStrategy = null;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mPackageManager = getInstrumentation().getContext().getPackageManager();
        mDevice.setOrientationNatural();
        mLauncherStrategy = LauncherStrategyFactory.getInstance(mDevice).getLauncherStrategy();
    }

    @Override
    protected void tearDown() throws Exception {
        mDevice.pressHome();
        mDevice.unfreezeRotation();
        mDevice.waitForIdle();
        super.tearDown();
    }

    public String getLauncherPackage() {
        return mDevice.getLauncherPackageName();
    }

    public void launchAppWithIntent(String appPackageName) {
        Intent appIntent = mPackageManager.getLaunchIntentForPackage(appPackageName);
        appIntent.addCategory(Intent.CATEGORY_LAUNCHER);
        appIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        getInstrumentation().getContext().startActivity(appIntent);
        SystemClock.sleep(TIMEOUT);
    }

    @MediumTest
    public void testGoHome() {
        launchAppWithIntent("com.android.chrome");
        mDevice.pressHome();
        UiObject2 hotseat = mDevice.findObject(By.res(getLauncherPackage(), HOTSEAT));
        assertNotNull("Hotseat could not be found", hotseat);
    }

    @MediumTest
    public void testHomeToRecentsNavigation() throws Exception {
        mDevice.pressRecentApps();
        assertNotNull("Recents not found when navigating from hotseat",
                mDevice.wait(Until.hasObject(By.res("com.android.systemui:id/recents_view")),
                TIMEOUT));
    }

    @MediumTest
    public void testCreateAndDeleteShortcutOnHome() throws Exception {
        createShortcutOnHome("Calculator");
        // Verify presence of shortcut on Home screen
        UiObject2 hotseat = mDevice.findObject(By.res(getLauncherPackage(), HOTSEAT));
        assertNotNull("Not on Home page; hotseat could not be found", hotseat);
        UiObject2 calculatorIcon = mDevice.wait(Until.findObject(By.text("Calculator")), TIMEOUT);
        assertNotNull("Calculator shortcut not found on Home screen", calculatorIcon);
        removeObjectFromHomeScreen(calculatorIcon, "text", "Calculator");
    }

    // Screen on with power button press
    @MediumTest
    public void testScreenOffOnUsingPowerButton() throws Exception {
        Context currentContext = getInstrumentation().getContext();
        PowerManager pm = (PowerManager) currentContext
                .getSystemService(currentContext.POWER_SERVICE);
        mDevice.pressKeyCode(KeyEvent.KEYCODE_POWER);
        Thread.sleep(TIMEOUT);
        assertFalse("Screen wasn't turned off", pm.isInteractive());
        mDevice.pressKeyCode(KeyEvent.KEYCODE_POWER);
        Thread.sleep(TIMEOUT);
        assertTrue("Screen wasn't turned on by pressing Power Key", pm.isInteractive());
        // Unlock screen since this ends up putting the device in Swpe lock mode.
        mDevice.wakeUp();
        mDevice.pressMenu();
    }

    // Wallpaper menu from home page
    @MediumTest
    public void testLongPressFromHomeToWallpaperMenu() {
        String wallpaperResourceId = getLauncherPackage() + ":id/wallpaper_image";
        verifyHomeLongPressMenu("Wallpaper", "wallpaper_button", wallpaperResourceId);
    }

    // Widget menu from home page
    @MediumTest
    public void testLongPressFromHomeToWidgetMenu() {
        String widgetResourceId = getLauncherPackage() + ":id/widgets_list_view";
        verifyHomeLongPressMenu("Widgets", "widget_button", widgetResourceId);
    }

    // Settings menu from home page
    @MediumTest
    public void testLongPressFromHomeToGoogleSettingsMenu() {
         verifyHomeLongPressMenu("Google settings", "settings_button",
                 "android:id/action_bar");
     }

    // Home screen long press display menu
    private void verifyHomeLongPressMenu(String longPressElementName,
            String longPressElementResourceId, String pageLoadResourceId) {
        mDevice.pressHome();
        UiObject2 workspace = mDevice.findObject(By.res(getLauncherPackage(), "workspace"));
        workspace.longClick();
        UiObject2 longPressElementButton = mDevice.findObject(By.res(getLauncherPackage(),
                longPressElementResourceId));
        assertNotNull(longPressElementName +
                " element is not visible on long press", longPressElementButton);
        longPressElementButton.click();
        mDevice.waitForIdle();
        assertNotNull(longPressElementName + " page hasn't loaded correctly on clicking",
                mDevice.wait(Until.hasObject(By.res(pageLoadResourceId)), TIMEOUT));
    }

    // Home screen add widget
    @MediumTest
    public void testAddRemoveWidgetOnHome() throws Exception {
        mDevice.pressHome();
        mDevice.waitForIdle();
        UiObject2 workspace = mDevice.findObject(By.res(getLauncherPackage(), "workspace"));
        workspace.longClick();
        UiObject2 widgetButton = mDevice.findObject(By.res(getLauncherPackage(),
                "widget_button"));
        widgetButton.click();
        mDevice.waitForIdle();
        UiObject2 analogClock = mDevice.wait(Until.findObject(By.text("Analog clock")), TIMEOUT);
        analogClock.click(2000L);

        // Verify presence of shortcut on Home screen
        UiObject2 hotseat = mDevice.findObject(By.res(getLauncherPackage(), HOTSEAT));
        assertNotNull("Not on Home page; hotseat could not be found", hotseat);
        UiObject2 analogClockWidget = mDevice.findObject
                (By.res("com.google.android.deskclock:id/analog_appwidget"));
        assertNotNull("Clock widget not found on Home screen", analogClockWidget);
        removeObjectFromHomeScreen(analogClockWidget, "res",
                "com.google.android.deskclock:id/analog_appwidget");
    }

    @MediumTest
    public void testCreateRenameRemoveFolderOnHome() throws Exception {
        // Create two shortcuts on the home screen
        createShortcutOnHome("Calculator");
        createShortcutOnHome("Clock");
        mDevice.pressHome();
        mDevice.waitForIdle();

        // Drag and drop the calculator shortcut onto
        // the clock shortcut to create a folder.
        UiObject2 calculatorIcon = mDevice.wait
                (Until.findObject(By.text("Calculator")), TIMEOUT);
        UiObject2 clockIcon = mDevice.wait
                (Until.findObject(By.text("Clock")), TIMEOUT);
        calculatorIcon.drag(clockIcon.getVisibleCenter(), 1000);

        // Verify that there is a new unnamed folder at this point
        UiObject2 customFolder = mDevice.wait
                (Until.findObject(By.desc("Folder: ")), TIMEOUT);
        customFolder.click();
        UiObject2 unnamedFolder = mDevice.wait
                (Until.findObject(By.text("Unnamed Folder")), TIMEOUT);
        assertNotNull("Custom folder not created", unnamedFolder);

        // Rename the unnamed folder to 'Snowflake'
        unnamedFolder.click();
        unnamedFolder.setText("Snowflake");

        // Dismiss the IME and then collapse the folder.
        mDevice.pressBack();
        mDevice.pressBack();
        mDevice.waitForIdle();
        UiObject2 workspace = mDevice.findObject(By.res(getLauncherPackage(), "workspace"));
        workspace.click();

        // Verify the newly renamed Snowflake folder
        UiObject2 snowflakeFolder = mDevice.wait
                (Until.findObject(By.text("Snowflake")), TIMEOUT);
        assertNotNull("Custom folder not created", snowflakeFolder);

        // Verify that the Snowflake folder can be removed
        removeObjectFromHomeScreen(snowflakeFolder, "text", "Snowflake");
    }

    // Folders - opening an app from folder
    @MediumTest
    public void testOpenAppFromFolderOnHome() {
        mDevice.pressHome();
        mDevice.waitForIdle();
        UiObject2 googleFolder = mDevice.wait
                (Until.findObject(By.desc("Folder: Google")), TIMEOUT);
        googleFolder.click();
        UiObject2 youTubeButton = mDevice.wait
                (Until.findObject(By.text("YouTube")), TIMEOUT);
        youTubeButton.click();
        assertTrue("Youtube wasn't opened from the Google folder",
                mDevice.wait(Until.hasObject
                (By.pkg("com.google.android.youtube")), TIMEOUT));
        mDevice.pressHome();
        mDevice.waitForIdle();
    }

    /* This method takes in an object to be drag/dropped onto the
     * Remove button hiding behind the search bar
     *
     * @param objectToRemove The UI object to be removed from the
     * Home screen
     * @param searchCategory String of value text, res or desc, based on which
     * the By selector is chosen to find the object
     * @param searchContent String to be searched in the searchCategory
     */
    private void removeObjectFromHomeScreen(UiObject2 objectToRemove,
            String searchCategory, String searchContent) {
        // Find the center of the Google Search Bar that the Remove button
        // is hidden behind.
        // Note: We're using this hacky way of locating the Remove button
        // because today, UIAutomator doesn't allow us to search for an element
        // while a touchdown has been executed, but before the touch up.
        // FYI: A click is a combination of a touch down and a touch up motion.
        UiObject2 removeButton = mDevice.wait(Until.findObject(By.desc("Google Search")),
                TIMEOUT);
        // Drag the calculator icon to the 'Remove' button to remove it
        objectToRemove.drag(new Point(mDevice.getDisplayWidth() / 2,
                 removeButton.getVisibleCenter().y), 1000);

        UiObject2 checkForObject = null;
        // Refetch the calculator icon
        if (searchCategory.equals("text")) {
            checkForObject = mDevice.findObject(By.text(searchContent));
        }
        else if (searchCategory.equals("res")) {
            checkForObject = mDevice.findObject(By.res(searchContent));
        }
        else if (searchCategory.equals("desc")) {
            checkForObject = mDevice.findObject(By.desc(searchContent));
        }
        else {
            Log.d(null, "Your search category doesn't match common use cases.");
        }
        assertNull(searchContent + " is present on the Home screen after removal attempt",
                checkForObject);
    }

    /* Creates a shortcut for the given app name on the Home screen
     *
     * @param appName text of the app name as seen in 'All Apps'
     */
    private void createShortcutOnHome(String appName) throws Exception {
        // Navigate to All Apps
        mDevice.pressHome();
        UiObject2 allApps = mDevice.findObject(By.desc("Apps"));
        allApps.click();
        mDevice.waitForIdle();

        // Long press on the Calculator app for two seconds and release on home screen
        // to create a shortcut
        UiObject2 appIcon =  mDevice.wait(Until.findObject
                (By.res(getLauncherPackage(), "icon").text(appName)), TIMEOUT);
        appIcon.click(2000L);
    }
}
