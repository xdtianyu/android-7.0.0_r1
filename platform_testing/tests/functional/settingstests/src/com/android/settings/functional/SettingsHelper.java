package android.settings.functional;

import android.app.Instrumentation;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.Until;

import java.util.regex.Pattern;

public class SettingsHelper {

    public static final String PKG = "com.android.settings";
    private static final int TIMEOUT = 2000;

    private UiDevice mDevice;
    private Instrumentation mInst;
    private ContentResolver mResolver;

    public SettingsHelper(UiDevice device, Instrumentation inst) {
        mDevice = device;
        mInst = inst;
        mResolver = inst.getContext().getContentResolver();
    }

    public static enum SettingsType {
        SYSTEM,
        SECURE,
        GLOBAL
    }

    public static void launchSettingsPage(Context ctx, String pageName) throws Exception {
        Intent intent = new Intent(pageName);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        ctx.startActivity(intent);
        Thread.sleep(TIMEOUT * 2);
    }

    public void clickSetting(String settingName) {
        mDevice.wait(Until.findObject(By.text(settingName)), TIMEOUT).click();
    }

    public void clickSetting(Pattern settingName) {
        mDevice.wait(Until.findObject(By.text(settingName)), TIMEOUT).click();
    }

    public void scrollVert(boolean isUp) {
        int w = mDevice.getDisplayWidth();
        int h = mDevice.getDisplayHeight();
        mDevice.swipe(w / 2, h / 2, w / 2, isUp ? h : 0, 2);
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
        int onSetting = Integer.parseInt(getStringSetting(type, internalName));
        if (doLaunch) {
            launchSettingsPage(mInst.getContext(), settingAction);
        }
        clickSetting(settingName);
        Thread.sleep(1000);
        String changedSetting = getStringSetting(type, internalName);
        return (1 - onSetting) == Integer.parseInt(changedSetting);
    }

    public boolean verifyRadioSetting(SettingsType type, String settingAction,
            String baseName, String settingName,
            String internalName, String testVal) throws Exception {
        clickSetting(baseName);
        clickSetting(settingName);
        Thread.sleep(500);
        return getStringSetting(type, internalName).equals(testVal);
    }

    private void putStringSetting(SettingsType type, String sName, String value) {
        switch (type) {
            case SYSTEM:
                Settings.System.putString(mResolver, sName, value); break;
            case GLOBAL:
                Settings.Global.putString(mResolver, sName, value); break;
            case SECURE:
                Settings.Secure.putString(mResolver, sName, value); break;
        }
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

    private void putIntSetting(SettingsType type, String sName, int value) {
        switch (type) {
            case SYSTEM:
                Settings.System.putInt(mResolver, sName, value); break;
            case GLOBAL:
                Settings.Global.putInt(mResolver, sName, value); break;
            case SECURE:
                Settings.Secure.putInt(mResolver, sName, value); break;
        }
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
