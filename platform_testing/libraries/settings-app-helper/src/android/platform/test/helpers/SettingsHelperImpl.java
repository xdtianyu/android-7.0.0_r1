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
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.platform.test.helpers.AbstractSettingsHelper;
import android.util.Log;

import junit.framework.Assert;

import java.util.regex.Pattern;

public class SettingsHelperImpl extends AbstractSettingsHelper {

    private static final int SETTINGS_DASH_TIMEOUT = 3000;
    private static final String UI_PACKAGE_NAME = "com.android.settings";
    private static final BySelector SETTINGS_DASHBOARD = By.res(UI_PACKAGE_NAME,
            "dashboard_container");
    private static final int TIMEOUT = 2000;
    private static final String LOG_TAG = SettingsHelperImpl.class.getSimpleName();

    private ContentResolver mResolver;

    public static enum SettingsType {
        SYSTEM,
        SECURE,
        GLOBAL
    }

    public SettingsHelperImpl(Instrumentation instr) {
        super(instr);
        mResolver = instr.getContext().getContentResolver();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        return "com.android.settings";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        return "Settings";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
    }

     /**
     * {@inheritDoc}
     */
    @Override
    public void scrollThroughSettings(int numberOfFlings) throws Exception {
        UiObject2 settingsList = loadAllSettings();
        int count = 0;
        while (count <= numberOfFlings && settingsList.fling(Direction.DOWN)) {
            count++;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void flingSettingsToStart() throws Exception {
        UiObject2 settingsList = loadAllSettings();
        while (settingsList.fling(Direction.UP));
    }

    public static void launchSettingsPage(Context ctx, String pageName) throws Exception {
        Intent intent = new Intent(pageName);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        ctx.startActivity(intent);
        Thread.sleep(TIMEOUT * 2);
    }

    public void scrollVert(boolean isUp) {
        int w = mDevice.getDisplayWidth();
        int h = mDevice.getDisplayHeight();
        mDevice.swipe(w / 2, h / 2, w / 2, isUp ? h : 0, 2);
    }

    /**
     * On N, the settingsDashboard is initially collapsed, and the user can see the "See all"
     * element. On hitting "See all", the same settings dashboard element is now scrollable. For
     * pre-N, the settings Dashboard is always scrollable, hence the check in the while loop. All
     * this method does is expand the Settings list if needed, before returning the element.
     */
    private UiObject2 loadAllSettings() throws Exception {
        UiObject2 settingsDashboard = mDevice.wait(Until.findObject(SETTINGS_DASHBOARD),
                SETTINGS_DASH_TIMEOUT);
        Assert.assertNotNull("Could not find the settings dashboard object.", settingsDashboard);
        int count = 0;
        while (!settingsDashboard.isScrollable() && count <= 2) {
            mDevice.wait(Until.findObject(By.text("SEE ALL")), SETTINGS_DASH_TIMEOUT).click();
            settingsDashboard = mDevice.wait(Until.findObject(SETTINGS_DASHBOARD),
                    SETTINGS_DASH_TIMEOUT);
            count++;
        }
        return settingsDashboard;
    }

    public void clickSetting(String settingName) throws InterruptedException {
        mDevice.wait(Until.findObject(By.text(settingName)), TIMEOUT).click();
        Thread.sleep(400);
    }

    public void clickSetting(Pattern settingName) throws InterruptedException {
        mDevice.wait(Until.findObject(By.text(settingName)), TIMEOUT).click();
        Thread.sleep(400);
    }

    public boolean verifyToggleSetting(SettingsType type, String settingAction,
            String settingName, String internalName) throws Exception {
        return verifyToggleSetting(
                type, settingAction, Pattern.compile(settingName), internalName, true);
    }

    public boolean verifyToggleSetting(SettingsType type, String settingAction,
            Pattern settingName, String internalName) throws Exception {
        return verifyToggleSetting(type, settingAction, settingName, internalName, true);
    }

    public boolean verifyToggleSetting(SettingsType type, String settingAction,
            String settingName, String internalName, boolean doLaunch) throws Exception {
        return verifyToggleSetting(
                type, settingAction, Pattern.compile(settingName), internalName, doLaunch);
    }

    public boolean verifyToggleSetting(SettingsType type, String settingAction,
            Pattern settingName, String internalName, boolean doLaunch) throws Exception {
        String onSettingBaseVal = getStringSetting(type, internalName);
        if (onSettingBaseVal == null) {
            onSettingBaseVal = "0";
        }
        int onSetting = Integer.parseInt(onSettingBaseVal);
        Log.d(null, "On Setting value is : " + onSetting);
        if (doLaunch) {
            launchSettingsPage(mInstrumentation.getContext(), settingAction);
        }
        clickSetting(settingName);
        Log.d(null, "Clicked setting : " + settingName);
        Thread.sleep(1000);
        String changedSetting = getStringSetting(type, internalName);
        Log.d(null, "Changed Setting value is : " + changedSetting);
        if (changedSetting == null) {
            Log.d(null, "Changed Setting value is : NULL");
            changedSetting = "0";
        }
        return (1 - onSetting) == Integer.parseInt(changedSetting);
    }

    public boolean verifyRadioSetting(SettingsType type, String settingAction,
            String baseName, String settingName,
            String internalName, String testVal) throws Exception {
        if (baseName != null) clickSetting(baseName);
        clickSetting(settingName);
        Thread.sleep(500);
        return getStringSetting(type, internalName).equals(testVal);
    }

    private String getStringSetting(SettingsType type, String sName) {
        switch (type) {
            case SYSTEM:
                return Settings.System.getString(mResolver, sName);
            case GLOBAL:
                return Settings.Global.getString(mResolver, sName);
            case SECURE:
                return Settings.Secure.getString(mResolver, sName);
        }
        return "";
    }

    private int getIntSetting(SettingsType type, String sName) throws SettingNotFoundException {
        switch (type) {
            case SYSTEM:
                return Settings.System.getInt(mResolver, sName);
            case GLOBAL:
                return Settings.Global.getInt(mResolver, sName);
            case SECURE:
                return Settings.Secure.getInt(mResolver, sName);
        }
        return Integer.MIN_VALUE;
    }
}
