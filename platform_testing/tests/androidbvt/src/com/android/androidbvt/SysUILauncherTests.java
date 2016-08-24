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

import android.app.WallpaperManager;
import android.content.Context;
import android.graphics.Point;
import android.graphics.drawable.Drawable;
import android.os.RemoteException;
import android.provider.Settings;
import android.support.test.InstrumentationRegistry;
import android.support.test.launcherhelper.ILauncherStrategy;
import android.support.test.launcherhelper.LauncherStrategyFactory;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.test.suitebuilder.annotation.LargeTest;

import junit.framework.TestCase;

import java.io.IOException;
import java.util.List;

public class SysUILauncherTests extends TestCase {
    private static final int LONG_TIMEOUT = 5000;
    private static final String APP_NAME = "Clock";
    private static final String PKG_NAME = "com.google.android.deskclock";
    private static final String WIDGET_PREVIEW = "widget_preview";
    private static final String APP_WIDGET_VIEW = "android.appwidget.AppWidgetHostView";
    private static final String WIDGET_TEXT_VIEW = "android.widget.TextView";
    private UiDevice mDevice = null;
    private Context mContext;
    private ILauncherStrategy mLauncherStrategy = null;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(InstrumentationRegistry.getInstrumentation());
        mContext = InstrumentationRegistry.getTargetContext();
        mDevice.setOrientationNatural();
        mLauncherStrategy = LauncherStrategyFactory.getInstance(mDevice).getLauncherStrategy();
    }

    @Override
    public void tearDown() throws Exception {
        mDevice.pressHome();
        mDevice.unfreezeRotation();
        mDevice.waitForIdle();
        super.tearDown();
    }

    /**
     * Add and remove a widget from home screen
     */
    @LargeTest
    public void testAddAndRemoveWidget() throws InterruptedException, IOException {
        // press menu key
        mDevice.pressMenu();
        Thread.sleep(LONG_TIMEOUT);
        mDevice.wait(Until.findObject(By.clazz(WIDGET_TEXT_VIEW)
                .text("WIDGETS")), LONG_TIMEOUT).click();
        Thread.sleep(LONG_TIMEOUT);
        // Long click to add widget
        mDevice.wait(
                Until.findObject(
                        By.res(mDevice.getLauncherPackageName(), WIDGET_PREVIEW)),
                LONG_TIMEOUT).click(1000);
        mDevice.pressHome();
        UiObject2 appWidget = mDevice.wait(
                Until.findObject(By.clazz(APP_WIDGET_VIEW)), LONG_TIMEOUT);
        assertNotNull("Widget has not been added", appWidget);
        removeObject(appWidget);
        appWidget = mDevice.wait(Until.findObject(By.clazz(APP_WIDGET_VIEW)),
                LONG_TIMEOUT);
        assertNull("Widget is still there", appWidget);
    }

    /**
     * Change Wall Paper
     */
    @LargeTest
    public void testChangeWallPaper() throws InterruptedException, IOException {
        try {
            WallpaperManager wallpaperManagerPre = WallpaperManager.getInstance(mContext);
            wallpaperManagerPre.clear();
            Thread.sleep(LONG_TIMEOUT);
            Drawable wallPaperPre = wallpaperManagerPre.getDrawable().getCurrent();
            // press menu key
            mDevice.pressMenu();
            Thread.sleep(LONG_TIMEOUT);
            mDevice.wait(Until.findObject(By.clazz(WIDGET_TEXT_VIEW)
                    .text("WALLPAPERS")), LONG_TIMEOUT).click();
            Thread.sleep(LONG_TIMEOUT);
            // set second wall paper as current wallpaper for home screen and lockscreen
            mDevice.wait(Until.findObject(By.descContains("Wallpaper 2")), LONG_TIMEOUT).click();
            mDevice.wait(Until.findObject(By.text("Set wallpaper")), LONG_TIMEOUT).click();
            UiObject2 homeScreen = mDevice
                    .wait(Until.findObject(By.text("Home screen and lock screen")), LONG_TIMEOUT);
            if (homeScreen != null) {
                homeScreen.click();
            }
            Thread.sleep(LONG_TIMEOUT);
            WallpaperManager wallpaperManagerPost = WallpaperManager.getInstance(mContext);
            Drawable wallPaperPost = wallpaperManagerPost.getDrawable().getCurrent();
            assertFalse("Wallpaper has not been changed", wallPaperPre.equals(wallPaperPost));
        } finally {
            WallpaperManager wallpaperManagerCurrrent = WallpaperManager.getInstance(mContext);
            wallpaperManagerCurrrent.clear();
            Thread.sleep(LONG_TIMEOUT);
        }
    }

    /**
     * Add and remove short cut from home screen
     */
    @LargeTest
    public void testAddAndRemoveShortCut() throws InterruptedException {
        mLauncherStrategy.openAllApps(true);
        Thread.sleep(LONG_TIMEOUT);
        // This is a long press and should add the shortcut to the Home screen
        mDevice.wait(Until.findObject(By.clazz("android.widget.TextView")
                .desc(APP_NAME)), LONG_TIMEOUT).click(1000);
        // Searching for the object on the Home screen
        UiObject2 app = mDevice.wait(Until.findObject(By.text(APP_NAME)), LONG_TIMEOUT);
        assertNotNull("Apps has been added", app);
        removeObject(app);
        app = mDevice.wait(Until.findObject(By.text(APP_NAME)), LONG_TIMEOUT);
        assertNull(APP_NAME + " is still there", app);
    }

    /**
     * Remove object from home screen
     */
    private void removeObject(UiObject2 app) throws InterruptedException {
        // Drag shortcut/widget icon to Remove button which behinds Google Search bar
        UiObject2 removeButton = mDevice.wait(Until.findObject(By.desc("Google Search")),
                LONG_TIMEOUT);
        app.drag(new Point(removeButton.getVisibleCenter().x, removeButton.getVisibleCenter().y),
                1000);
    }
}
