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
 * limitations under the License
 */

package android.support.test.launcherhelper;

import android.graphics.Point;
import android.os.RemoteException;
import android.os.SystemClock;
import android.support.test.uiautomator.*;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

public class LeanbackLauncherStrategy implements ILeanbackLauncherStrategy {

    private static final String LOG_TAG = LeanbackLauncherStrategy.class.getSimpleName();
    private static final String PACKAGE_LAUNCHER = "com.google.android.leanbacklauncher";
    private static final String PACKAGE_SEARCH = "com.google.android.katniss";

    private static final int MAX_SCROLL_ATTEMPTS = 20;
    private static final int APP_LAUNCH_TIMEOUT = 10000;
    private static final int SHORT_WAIT_TIME = 5000;    // 5 sec

    protected UiDevice mDevice;


    /**
     * {@inheritDoc}
     */
    @Override
    public String getSupportedLauncherPackage() {
        return PACKAGE_LAUNCHER;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUiDevice(UiDevice uiDevice) {
        mDevice = uiDevice;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void open() {
        // if we see main list view, assume at home screen already
        if (!mDevice.hasObject(getWorkspaceSelector())) {
            mDevice.pressHome();
            // ensure launcher is shown
            if (!mDevice.wait(Until.hasObject(getWorkspaceSelector()), SHORT_WAIT_TIME)) {
                // HACK: dump hierarchy to logcat
                ByteArrayOutputStream baos = new ByteArrayOutputStream();
                try {
                    mDevice.dumpWindowHierarchy(baos);
                    baos.flush();
                    baos.close();
                    String[] lines = baos.toString().split("\\r?\\n");
                    for (String line : lines) {
                        Log.d(LOG_TAG, line.trim());
                    }
                } catch (IOException ioe) {
                    Log.e(LOG_TAG, "error dumping XML to logcat", ioe);
                }
                throw new RuntimeException("Failed to open leanback launcher");
            }
            mDevice.waitForIdle();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public UiObject2 openAllApps(boolean reset) {
        UiObject2 appsRow = selectAppsRow();
        if (appsRow == null) {
            throw new RuntimeException("Could not find all apps row");
        }
        if (reset) {
            Log.w(LOG_TAG, "The reset will be ignored on leanback launcher");
        }
        return appsRow;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public BySelector getWorkspaceSelector() {
        return By.res(getSupportedLauncherPackage(), "main_list_view");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public BySelector getSearchRowSelector() {
        return By.res(getSupportedLauncherPackage(), "search_view");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public BySelector getNotificationRowSelector() {
        return By.res(getSupportedLauncherPackage(), "notification_view");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public BySelector getAppsRowSelector() {
        return By.res(getSupportedLauncherPackage(), "list").desc("Apps");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public BySelector getGamesRowSelector() {
        return By.res(getSupportedLauncherPackage(), "list").desc("Games");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public BySelector getSettingsRowSelector() {
        return By.res(getSupportedLauncherPackage(), "list").desc("")
                .hasDescendant(By.res("icon"));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Direction getAllAppsScrollDirection() {
        return Direction.RIGHT;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public BySelector getAllAppsSelector() {
        // On Leanback launcher the Apps row corresponds to the All Apps on phone UI
        return getAppsRowSelector();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public long launch(String appName, String packageName) {
        BySelector app = By.res(getSupportedLauncherPackage(), "app_banner").desc(appName);
        return launchApp(this, app, packageName);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void search(String query) {
        if (selectSearchRow() == null) {
            throw new RuntimeException("Could not find search row.");
        }

        BySelector keyboardOrb = By.res(getSupportedLauncherPackage(), "keyboard_orb");
        UiObject2 orbButton = mDevice.wait(Until.findObject(keyboardOrb), SHORT_WAIT_TIME);
        if (orbButton == null) {
            throw new RuntimeException("Could not find keyboard orb.");
        }
        if (orbButton.isFocused()) {
            mDevice.pressDPadCenter();
        } else {
            // Move the focus to keyboard orb by DPad button.
            mDevice.pressDPadRight();
            if (orbButton.isFocused()) {
                mDevice.pressDPadCenter();
            }
        }
        mDevice.wait(Until.gone(keyboardOrb), SHORT_WAIT_TIME);

        BySelector searchEditor = By.res(PACKAGE_SEARCH, "search_text_editor");
        UiObject2 editText = mDevice.wait(Until.findObject(searchEditor), SHORT_WAIT_TIME);
        if (editText == null) {
            throw new RuntimeException("Could not find search text input.");
        }

        editText.setText(query);
        SystemClock.sleep(SHORT_WAIT_TIME);

        // Note that Enter key is pressed instead of DPad keys to dismiss leanback IME
        mDevice.pressEnter();
        mDevice.wait(Until.gone(searchEditor), SHORT_WAIT_TIME);
    }

    /**
     * {@inheritDoc}
     *
     * Assume that the rows are sorted in the following order from the top:
     *  Search, Notification(, Partner), Apps, Games, Settings(, and Inputs)
     */
    @Override
    public UiObject2 selectNotificationRow() {
        if (!isNotificationRowSelected()) {
            open();
            mDevice.pressHome();    // Home key to move to the first card in the Notification row
        }
        return mDevice.wait(Until.findObject(
                getNotificationRowSelector().hasChild(By.focused(true))), SHORT_WAIT_TIME);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public UiObject2 selectSearchRow() {
        if (!isSearchRowSelected()) {
            selectNotificationRow();
            mDevice.pressDPadUp();
        }
        return mDevice.wait(Until.findObject(
                getSearchRowSelector().hasDescendant(By.focused(true))), SHORT_WAIT_TIME);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public UiObject2 selectAppsRow() {
        // Start finding Apps row from Notification row
        if (!isAppsRowSelected()) {
            selectNotificationRow();
            mDevice.pressDPadDown();
        }
        return mDevice.wait(Until.findObject(
                getAllAppsSelector().hasChild(By.focused(true))), SHORT_WAIT_TIME);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public UiObject2 selectGamesRow() {
        if (!isGamesRowSelected()) {
            selectAppsRow();
            mDevice.pressDPadDown();
            // If more than or equal to 16 apps are installed, the app banner could be cut off
            // into two rows at maximum. It needs to scroll down once more.
            if (!isGamesRowSelected()) {
                mDevice.pressDPadDown();
            }
        }
        return mDevice.wait(Until.findObject(
                getGamesRowSelector().hasChild(By.focused(true))), SHORT_WAIT_TIME);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public UiObject2 selectSettingsRow() {
        if (!isSettingsRowSelected()) {
            open();
            mDevice.pressHome();    // Home key to move to the first card in the Notification row
            // The Settings row is at the last position
            final int MAX_ROW_NUMS = 8;
            for (int i = 0; i < MAX_ROW_NUMS; ++i) {
                mDevice.pressDPadDown();
            }
        }
        return null;
    }

    @SuppressWarnings("unused")
    @Override
    public UiObject2 openAllWidgets(boolean reset) {
        throw new UnsupportedOperationException(
                "All Widgets is not available on Leanback Launcher.");
    }

    @SuppressWarnings("unused")
    @Override
    public BySelector getAllWidgetsSelector() {
        throw new UnsupportedOperationException(
                "All Widgets is not available on Leanback Launcher.");
    }

    @SuppressWarnings("unused")
    @Override
    public Direction getAllWidgetsScrollDirection() {
        throw new UnsupportedOperationException(
                "All Widgets is not available on Leanback Launcher.");
    }

    @SuppressWarnings("unused")
    @Override
    public BySelector getHotSeatSelector() {
        throw new UnsupportedOperationException(
                "Hot Seat is not available on Leanback Launcher.");
    }

    @SuppressWarnings("unused")
    @Override
    public Direction getWorkspaceScrollDirection() {
        throw new UnsupportedOperationException(
                "Workspace is not available on Leanback Launcher.");
    }

    protected long launchApp(ILauncherStrategy launcherStrategy, BySelector app,
            String packageName) {
        return launchApp(launcherStrategy, app, packageName, MAX_SCROLL_ATTEMPTS);
    }

    protected long launchApp(ILauncherStrategy launcherStrategy, BySelector app,
            String packageName, int maxScrollAttempts) {
        unlockDeviceIfAsleep();

        if (isAppOpen(packageName)) {
            // Application is already open
            return 0;
        }

        // Go to the home page
        launcherStrategy.open();
        // attempt to find the app icon if it's not already on the screen
        UiObject2 container = launcherStrategy.openAllApps(false);
        UiObject2 appIcon = container.findObject(app);
        int attempts = 0;
        while (attempts++ < maxScrollAttempts) {
            // Compare the focused icon and the app icon to search for.
            UiObject2 focusedIcon = container.findObject(By.focused(true))
                    .findObject(By.res(getSupportedLauncherPackage(), "app_banner"));

            if (appIcon == null) {
                appIcon = findApp(container, focusedIcon, app);
                if (appIcon == null) {
                    throw new RuntimeException("Failed to find the app icon on screen: "
                            + packageName);
                }
                continue;
            } else if (focusedIcon.equals(appIcon)) {
                // The app icon is on the screen, and selected.
                break;
            } else {
                // The app icon is on the screen, but not selected yet
                // Move one step closer to the app icon
                Point currentPosition = focusedIcon.getVisibleCenter();
                Point targetPosition = appIcon.getVisibleCenter();
                int dx = targetPosition.x - currentPosition.x;
                int dy = targetPosition.y - currentPosition.y;
                final int MARGIN = 10;
                // The sequence of moving should be kept in the following order so as not to
                // be stuck in case that the apps row are not even.
                if (dx < -MARGIN) {
                    mDevice.pressDPadLeft();
                    continue;
                }
                if (dy < -MARGIN) {
                    mDevice.pressDPadUp();
                    continue;
                }
                if (dx > MARGIN) {
                    mDevice.pressDPadRight();
                    continue;
                }
                if (dy > MARGIN) {
                    mDevice.pressDPadDown();
                    continue;
                }
                throw new RuntimeException(
                        "Failed to navigate to the app icon on screen: " + packageName);
            }
        }

        if (attempts == maxScrollAttempts) {
            throw new RuntimeException(
                    "scrollBackToBeginning: exceeded max attempts: " + maxScrollAttempts);
        }

        // The app icon is already found and focused.
        long ready = SystemClock.uptimeMillis();
        mDevice.pressDPadCenter();
        mDevice.waitForIdle();
        if (packageName != null) {
            Log.w(LOG_TAG, String.format(
                    "No UI element with package name %s detected.", packageName));
            boolean success = mDevice.wait(Until.hasObject(
                    By.pkg(packageName).depth(0)), APP_LAUNCH_TIMEOUT);
            if (success) {
                return ready;
            } else {
                return ILauncherStrategy.LAUNCH_FAILED_TIMESTAMP;
            }
        } else {
            return ready;
        }
    }

    protected boolean isSearchRowSelected() {
        UiObject2 row = mDevice.findObject(getSearchRowSelector());
        if (row == null) {
            return false;
        }
        return row.hasObject(By.focused(true));
    }

    protected boolean isAppsRowSelected() {
        UiObject2 row = mDevice.findObject(getAppsRowSelector());
        if (row == null) {
            return false;
        }
        return row.hasObject(By.focused(true));
    }

    protected boolean isGamesRowSelected() {
        UiObject2 row = mDevice.findObject(getGamesRowSelector());
        if (row == null) {
            return false;
        }
        return row.hasObject(By.focused(true));
    }

    protected boolean isNotificationRowSelected() {
        UiObject2 row = mDevice.findObject(getNotificationRowSelector());
        if (row == null) {
            return false;
        }
        return row.hasObject(By.focused(true));
    }

    protected boolean isSettingsRowSelected() {
        // Settings label is only visible if the settings row is selected
        return mDevice.hasObject(By.res(getSupportedLauncherPackage(), "label").text("Settings"));
    }

    protected boolean isAppOpen (String appPackage) {
        return mDevice.hasObject(By.pkg(appPackage).depth(0));
    }

    protected void unlockDeviceIfAsleep () {
        // Turn screen on if necessary
        try {
            if (!mDevice.isScreenOn()) {
                mDevice.wakeUp();
            }
        } catch (RemoteException e) {
            Log.e(LOG_TAG, "Failed to unlock the screen-off device.", e);
        }
    }

    protected UiObject2 findApp(UiObject2 container, UiObject2 focusedIcon, BySelector app) {
        UiObject2 appIcon;
        // The app icon is not on the screen.
        // Search by going left first until it finds the app icon on the screen
        String prevText = focusedIcon.getContentDescription();
        String nextText;
        do {
            mDevice.pressDPadLeft();
            appIcon = container.findObject(app);
            if (appIcon != null) {
                return appIcon;
            }
            nextText = container.findObject(By.focused(true)).findObject(
                    By.res(getSupportedLauncherPackage(),
                            "app_banner")).getContentDescription();
        } while (nextText != null && !nextText.equals(prevText));

        // If we haven't found it yet, search by going right
        do {
            mDevice.pressDPadRight();
            appIcon = container.findObject(app);
            if (appIcon != null) {
                return appIcon;
            }
            nextText = container.findObject(By.focused(true)).findObject(
                    By.res(getSupportedLauncherPackage(),
                            "app_banner")).getContentDescription();
        } while (nextText != null && !nextText.equals(prevText));
        return null;
    }
}
