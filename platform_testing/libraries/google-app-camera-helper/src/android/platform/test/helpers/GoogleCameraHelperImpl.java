/*
 * Copyright (C) 2015 The Android Open Source Project
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
import android.platform.test.helpers.exceptions.UnknownUiException;
import android.support.test.launcherhelper.ILauncherStrategy;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Configurator;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.Until;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.UiWatcher;
import android.util.Log;

import java.text.SimpleDateFormat;
import java.text.DateFormat;
import java.util.Date;
import java.util.regex.Pattern;

public class GoogleCameraHelperImpl extends AbstractGoogleCameraHelper {
    private static final String LOG_TAG = GoogleCameraHelperImpl.class.getSimpleName();
    private static final String UI_ACTIVITY_VIEW_ID = "activity_root_view";
    private static final String UI_ALBUM_FILMSTRIP_VIEW_ID = "filmstrip_view";
    private static final String UI_PACKAGE_NAME = "com.android.camera2";
    private static final String UI_RECORDING_TIME_ID = "recording_time";
    private static final String UI_SHUTTER_DESC_CAM_3X = "Capture photo";
    private static final String UI_SHUTTER_DESC_CAM_2X = "Shutter";
    private static final String UI_SHUTTER_DESC_VID_3X = "Capture video";
    private static final String UI_SHUTTER_DESC_VID_2X = "Shutter";
    private static final String UI_THUMBNAIL_ALBUM_BUTTON_ID = "rounded_thumbnail_view";
    private static final String UI_TOGGLE_BUTTON_ID = "photo_video_paginator";
    private static final String UI_BACK_FRONT_TOGGLE_BUTTON_ID = "camera_toggle_button";
    private static final String UI_MODE_OPTION_TOGGLE_BUTTON_ID = "mode_options_toggle";
    private static final String UI_SHUTTER_BUTTON_ID_3X = "photo_video_button";
    private static final String UI_SHUTTER_BUTTON_ID_2X = "shutter_button";
    private static final String UI_SETTINGS_BUTTON_ID = "settings_button";
    private static final String UI_MENU_BUTTON_ID_3X = "menuButton";
    private static final String UI_MENU_BUTTON_ID_4X = "toybox_menu_button";
    private static final String UI_SPECIAL_MODE_CLOSE = "closeButton";
    private static final String UI_HDR_BUTTON_ID_2X = "hdr_plus_toggle_button";
    private static final String UI_HDR_BUTTON_ID_3X = "hdr_plus_toggle_button";
    private static final String UI_HDR_BUTTON_ID_4X = "hdr_button";
    private static final String UI_HDR_AUTO_ID_4X = "hdr_auto";
    private static final String UI_HDR_ON_ID_4X = "hdr_on";
    private static final String UI_HDR_OFF_ID_4X = "hdr_off";
    private static final String UI_SELECTED_OPTION_ID = "selected_option_label";
    private static final String UI_HFR_TOGGLE_ID_J = "hfr_button";
    private static final String UI_HFR_TOGGLE_ID_I = "hfr_mode_toggle_button";

    private static final String DESC_HDR_AUTO = "HDR Plus auto";
    private static final String DESC_HDR_OFF_3X = "HDR Plus off";
    private static final String DESC_HDR_ON_3X = "HDR Plus on";

    private static final String DESC_HDR_OFF_2X = "HDR off";
    private static final String DESC_HDR_ON_2X = "HDR on";

    private static final String DESC_HFR_OFF = "Slow motion is off";
    private static final String DESC_HFR_120_FPS = "Slow motion is set to 120 fps";
    private static final String DESC_HFR_240_FPS = "Slow motion is set to 240 fps";

    private static final String TEXT_4K_ON = "UHD 4K";
    private static final String TEXT_HD_1080 = "HD 1080p";
    private static final String TEXT_HD_720 = "HD 720p";
    private static final String TEXT_HDR_AUTO = "HDR off";
    private static final String TEXT_HDR_ON = "HDR+ Auto";
    private static final String TEXT_HDR_OFF = "HDR on";
    private static final String TEXT_BACK_VIDEO_RESOLUTION_4X = "Back camera video resolution";
    private static final String TEXT_BACK_VIDEO_RESOLUTION_3X = "Back camera video";

    public static final int HDR_MODE_AUTO = -1;
    public static final int HDR_MODE_OFF = 0;
    public static final int HDR_MODE_ON = 1;

    public static final int VIDEO_4K_MODE_ON = 1;
    public static final int VIDEO_HD_1080 = 0;
    public static final int VIDEO_HD_720 = -1;

    public static final int HFR_MODE_OFF = 0;
    public static final int HFR_MODE_120_FPS = 1;
    public static final int HFR_MODE_240_FPS = 2;

    private static final long APP_INIT_WAIT = 20000;
    private static final long DIALOG_TRANSITION_WAIT = 5000;
    private static final long SHUTTER_WAIT_TIME = 20000;
    private static final long SWITCH_WAIT_TIME = 5000;
    private static final long MENU_WAIT_TIME = 5000;

    private boolean mIsVersionH = false;
    private boolean mIsVersionI = false;
    private boolean mIsVersionJ = false;
    private boolean mIsVersionK = false;

    public GoogleCameraHelperImpl(Instrumentation instr) {
        super(instr);

        try {
            mIsVersionH = getVersion().startsWith("2.");
            mIsVersionI = getVersion().startsWith("3.0") || getVersion().startsWith("3.1");
            mIsVersionJ = getVersion().startsWith("3.2");
            mIsVersionK = getVersion().startsWith("4");
        } catch (NameNotFoundException e) {
            Log.e(LOG_TAG, String.format("Unable to find package by name, %s", getPackage()));
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        return "com.google.android.GoogleCamera";
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        return "Camera";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        if (mIsVersionK) {
            // Dismiss dogfood confidentiality dialog
            Pattern okText = Pattern.compile("OK, GOT IT", Pattern.CASE_INSENSITIVE);
            UiObject2 dogfoodMessage = mDevice.wait(
                    Until.findObject(By.text(okText)), APP_INIT_WAIT);
            if (dogfoodMessage != null) {
                dogfoodMessage.click();
            }
        } else if (mIsVersionI || mIsVersionJ) {
            // Dismiss dogfood confidentiality dialog
            Pattern okText = Pattern.compile("OK, GOT IT", Pattern.CASE_INSENSITIVE);
            UiObject2 dogfoodMessage = mDevice.wait(
                    Until.findObject(By.text(okText)), APP_INIT_WAIT);
            if (dogfoodMessage != null) {
                dogfoodMessage.click();
            }
            // Swipe left to dismiss 'how to open video message'
            UiObject2 activityView = mDevice.wait(Until.findObject(
                    By.res(UI_PACKAGE_NAME, "activity_root_view")), DIALOG_TRANSITION_WAIT);
            if (activityView != null) {
                activityView.swipe(Direction.LEFT, 1.0f);
            }
            // Confirm 'GOT IT' for action above
            UiObject2 thanks = mDevice.wait(Until.findObject(By.text("GOT IT")),
                    DIALOG_TRANSITION_WAIT);
            if (thanks != null) {
                thanks.click();
            }
        } else {
            BySelector confirm = By.res(UI_PACKAGE_NAME, "confirm_button");
            UiObject2 location = mDevice.wait(Until.findObject(
                    By.copy(confirm).text("NEXT")), APP_INIT_WAIT);
            if (location != null) {
                location.click();
            }
            // Choose sensor size. It's okay to timeout. These dialog screens might not exist..
            UiObject2 sensor = mDevice.wait(Until.findObject(
                    By.copy(confirm).text("OK, GOT IT")), DIALOG_TRANSITION_WAIT);
            if (sensor != null) {
                sensor.click();
            }
            // Dismiss dogfood dialog
            if (mDevice.wait(Until.hasObject(
                    By.res(UI_PACKAGE_NAME, "internal_release_dialog_title")), 5000)) {
                mDevice.findObject(By.res(UI_PACKAGE_NAME, "ok_button")).click();
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void capturePhoto() {
        if (!isCameraMode()) {
            throw new IllegalStateException(
                    "GoogleCamera must be in Camera mode to capture photos.");
        }

        getCameraShutter().click();
        waitForCameraShutterEnabled();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void captureVideo(long timeInMs) {
        if (!isVideoMode()) {
            throw new IllegalStateException("GoogleCamera must be in Video mode to record videos.");
        }

        if (isRecording()) {
            return;
        }

        // Temporary hack #1: Make UI code responsive by shortening the UiAutomator idle timeout.
        // The pulsing record button broadcasts unnecessary events of TYPE_WINDOW_CONTENT_CHANGED,
        // but we intend to have a fix and remove this hack with Kenai (GC 3.0).
        long original = Configurator.getInstance().getWaitForIdleTimeout();
        Configurator.getInstance().setWaitForIdleTimeout(1000);

        try {
            getVideoShutter().click();
            SystemClock.sleep(timeInMs);
            getVideoShutter().click();
            waitForVideoShutterEnabled();
        } finally {
            Configurator.getInstance().setWaitForIdleTimeout(original);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void snapshotVideo(long videoTimeInMs, long snapshotStartTimeInMs) {
        if (!isVideoMode()) {
            throw new IllegalStateException("GoogleCamera must be in Video mode to record videos.");
        } else if (videoTimeInMs <= snapshotStartTimeInMs) {
            throw new IllegalArgumentException(
                    "video recording time length must be larger than snapshot start time");
        }

        // Temporary hack #2: Make UI code responsive by shortening the UiAutomator idle timeout.
        // The pulsing record button broadcasts unnecessary events of TYPE_WINDOW_CONTENT_CHANGED,
        // but we intend to have a fix and remove this hack with Kenai (GC 3.0).
        long original = Configurator.getInstance().getWaitForIdleTimeout();
        Configurator.getInstance().setWaitForIdleTimeout(1000);

        if (isRecording()) {
            return;
        }

        try {
            getVideoShutter().click();
            SystemClock.sleep(snapshotStartTimeInMs);

            boolean snapshot_success = false;

            // Take a snapshot
            if (mIsVersionJ || mIsVersionK) {
                UiObject2 snapshotButton = mDevice.findObject(By.res(UI_PACKAGE_NAME, "snapshot_button"));
                if (snapshotButton != null) {
                    snapshotButton.click();
                    snapshot_success = true;
                }
            } else if (mIsVersionI) {
                // Ivvavik Version of GCA doesn't support snapshot
                snapshot_success = false;
            } else {
                UiObject2 snapshotButton = mDevice.findObject(By.res(UI_PACKAGE_NAME, "recording_time"));
                if (snapshotButton != null) {
                    snapshotButton.click();
                    snapshot_success = true;
                }
            }

            if (!snapshot_success) {
                getVideoShutter().click();
                waitForVideoShutterEnabled();
                throw new UnknownUiException("snapshot button not found!");
            }

            SystemClock.sleep(videoTimeInMs - snapshotStartTimeInMs);
            getVideoShutter().click();
            waitForVideoShutterEnabled();
        } finally {
            Configurator.getInstance().setWaitForIdleTimeout(original);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToCameraMode() {
        if (isCameraMode()) {
            return;
        }

        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            UiObject2 toggle = getCameraVideoToggleButton();
            if (toggle != null) {
                toggle.click();
            }
        } else {
            openMenu();
            selectMenuItem("Camera");
        }

        mDevice.waitForIdle();
        waitForCameraShutterEnabled();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToVideoMode() {
        if (isVideoMode()) {
            return;
        }

        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            UiObject2 toggle = getCameraVideoToggleButton();
            if (toggle != null) {
                toggle.click();
            }
        } else {
            openMenu();
            selectMenuItem("Video");
        }

        mDevice.waitForIdle();
        waitForVideoShutterEnabled();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToBackCamera() {
        if (isBackCamera()) {
            return;
        }

        // Close menu if open
        closeMenu();

        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            pressBackFrontToggleButton();
        } else {
            // Open mode options if not open.
            // Note: the mode option button only appear if mode option menu not open
            UiObject2 modeoptions = getModeOptionsMenuButton();
            if (modeoptions != null) {
                modeoptions.click();
            }
            pressBackFrontToggleButton();
        }

        // Wait for ensuring back camera button enabled
        waitForBackEnabled();

        // Wait for ensuring shutter button enabled
        waitForCurrentShutterEnabled();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToFrontCamera() {
        if (isFrontCamera()) {
            return;
        }

        // Close menu if open
        closeMenu();

        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            pressBackFrontToggleButton();
        } else {
            // Open mode options if not open.
            // Note: the mode option button only appear if mode option menu not open
            UiObject2 modeoptions = getModeOptionsMenuButton();
            if (modeoptions != null) {
                modeoptions.click();
            }
            pressBackFrontToggleButton();
        }

        // Wait for ensuring front camera button enabled
        waitForFrontEnabled();

        // Wait for ensuring shutter button enabled
        waitForCurrentShutterEnabled();
    }

    /**
     * {@inheritDoc}
     */
    public void setHdrMode(int mode) {
        if (!isCameraMode()) {
            throw new IllegalStateException("Cannot set HDR unless in camera mode.");
        }

        if (mIsVersionK) {
            if (getHdrToggleButton() == null) {
                if (mode == HDR_MODE_OFF) {
                    return;
                } else {
                    throw new UnsupportedOperationException(
                            "Cannot set HDR on this device as requested.");
                }
            }

            getHdrToggleButton().click();
            // After clicking the HDR auto button should be visible.
            mDevice.wait(Until.findObject(By.res(UI_PACKAGE_NAME, UI_HDR_AUTO_ID_4X)),
                    DIALOG_TRANSITION_WAIT);

            switch (mode) {
                case HDR_MODE_AUTO:
                    mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_HDR_AUTO_ID_4X)).click();
                    break;
                case HDR_MODE_ON:
                    mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_HDR_ON_ID_4X)).click();
                    break;
                case HDR_MODE_OFF:
                    mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_HDR_OFF_ID_4X)).click();
                    break;
                default:
                    throw new UnknownUiException("Failing setting HDR+ mode!");
            }
            mDevice.waitForIdle();
        } else if (mIsVersionI || mIsVersionJ) {
            if (getHdrToggleButton() == null) {
                if (mode == HDR_MODE_OFF) {
                    return;
                } else {
                    throw new UnsupportedOperationException(
                            "Cannot set HDR on this device as requested.");
                }
            }

            for (int retries = 0; retries < 3; retries++) {
                if (!isHdrMode(mode)) {
                    getHdrToggleButton().click();
                    mDevice.waitForIdle();
                } else {
                    Log.e(LOG_TAG, "Successfully set HDR mode!");
                    mDevice.waitForIdle();
                    return;
                }
            }
        } else {
            // Open mode options before checking Hdr status
            openModeOptions2X();
            if (getHdrToggleButton() == null) {
                if (mode == HDR_MODE_OFF) {
                    return;
                } else {
                    throw new UnsupportedOperationException(
                            "Cannot set HDR on this device as requested.");
                }
            }

            for (int retries = 0; retries < 3; retries++) {
                if (!isHdrMode(mode)) {
                    getHdrToggleButton().click();
                    mDevice.waitForIdle();
                } else {
                    Log.e(LOG_TAG, "Successfully set HDR mode!");
                    mDevice.waitForIdle();
                    return;
                }
            }
        }
    }

    private boolean isHdrMode(int mode) {
        if (mIsVersionK) {
            getHdrToggleButton().click();
            mDevice.waitForIdle();
            UiObject2 selectedOption = mDevice.wait(Until.findObject(
                    By.res(UI_PACKAGE_NAME, UI_SELECTED_OPTION_ID)), MENU_WAIT_TIME);
            String currentHdrModeText = selectedOption.getText();
            int currentMode = 0;
            switch (currentHdrModeText) {
                case TEXT_HDR_AUTO:
                    currentMode = HDR_MODE_AUTO;
                    break;
                case TEXT_HDR_ON:
                    currentMode = HDR_MODE_ON;
                    break;
                case TEXT_HDR_OFF:
                    currentMode = HDR_MODE_OFF;
                    break;
                default:
                    throw new UnknownUiException("Failed to identify the HDR+ settings!");
            }
            selectedOption.click();
            mDevice.wait(Until.findObject(
                    By.res(UI_PACKAGE_NAME, UI_HDR_BUTTON_ID_4X)), MENU_WAIT_TIME);
            return mode == currentMode;
        } else if (mIsVersionI || mIsVersionJ) {
            String modeDesc = getHdrToggleButton().getContentDescription();
            if (DESC_HDR_AUTO.equals(modeDesc)) {
                return HDR_MODE_AUTO == mode;
            } else if (DESC_HDR_OFF_3X.equals(modeDesc)) {
                return HDR_MODE_OFF == mode;
            } else if (DESC_HDR_ON_3X.equals(modeDesc)) {
                return HDR_MODE_ON == mode;
            } else {
                throw new UnknownUiException("Unexpected failure.");
            }
        } else {
            // Open mode options before checking Hdr status
            openModeOptions2X();
            // Check the HDR mode
            String modeDesc = getHdrToggleButton().getContentDescription();
            if (DESC_HDR_OFF_2X.equals(modeDesc)) {
                return HDR_MODE_OFF == mode;
            } else if (DESC_HDR_ON_2X.equals(modeDesc)) {
                return HDR_MODE_ON == mode;
            } else {
                throw new UnknownUiException("Unexpected failure.");
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    public void set4KMode(int mode) {
        // If the menu is not open, open it
        if (!isMenuOpen()) {
            openMenu();
        }

        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            // Select Menu Item "Settings"
            selectMenuItem("Settings");
        } else {
            // Select Menu Item "Settings"
            selectSetting2X();
        }

        if (mIsVersionI || mIsVersionJ) {
            // Select Item "Resolution & Quality"
            selectSettingItem("Resolution & quality");
        }

        // Select Item "Back camera video", which is the only mode supports 4k
        selectVideoResolution(mode);

        if (mIsVersionI || mIsVersionJ) {
            // Quit Menu "Resolution & Quality"
            closeSettingItem();
        }

        // Close Main Menu
        closeMenuItem();
    }

    /**
     * {@inheritDoc}
     */
    public void setHFRMode(int mode) {
        if (!isVideoMode()) {
            throw new IllegalStateException("Must be in video mode to set HFR mode.");
        }

        // Haleakala doesn't support slow motion, so throw exception
        if (mIsVersionH) {
            throw new UnsupportedOperationException(
                    "HFR not supported on this version of Google Camera.");
        } else if (mIsVersionI) {
            waitForHFRToggleEnabled();
            for (int retries = 0; retries < 3; retries++) {
                if (!isHfrMode(mode)) {
                    getHfrToggleButton().click();
                    mDevice.waitForIdle();
                } else {
                    Log.e(LOG_TAG, "Successfully set HFR mode!");
                    mDevice.waitForIdle();
                    waitForVideoShutterEnabled();
                    return;
                }
            }
            //If none of the 3 options match expected option, throw an exception
            if (mode == HFR_MODE_OFF) {
                throw new UnknownUiException("Failed to turn off the HFR mode");
            } else {
                throw new UnknownUiException(String.format("Failed to select HFR mode to FPS %d",
                        (int) Math.floor(mode * 120)));
            }
        } else if (mIsVersionJ || mIsVersionK) {
            String uiMenuButton = (mIsVersionK)? UI_MENU_BUTTON_ID_4X:UI_MENU_BUTTON_ID_3X;
            if (mode == HFR_MODE_OFF) {
                // This close button ui only appeared in hfr mode
                UiObject2 hfrmodeclose = mDevice.findObject(By.res(UI_PACKAGE_NAME,
                        UI_SPECIAL_MODE_CLOSE));
                if (hfrmodeclose != null) {
                    hfrmodeclose.click();
                    mDevice.wait(Until.hasObject(By.res(UI_PACKAGE_NAME, uiMenuButton)),
                            MENU_WAIT_TIME);
                } else {
                    throw new UnknownUiException(
                            "Fail to find hfr mode close button when trying to turn off HFR mode");
                }
                return;
            }

            // When not in HFR interface, select menu to open HFR interface
            if (mDevice.hasObject(By.res(UI_PACKAGE_NAME, UI_SPECIAL_MODE_CLOSE))
                    && !isVideoMode()) {
                UiObject2 specialmodeclose = mDevice.findObject(By.res(UI_PACKAGE_NAME,
                        UI_SPECIAL_MODE_CLOSE));
                if (specialmodeclose != null) {
                    specialmodeclose.click();
                    mDevice.wait(Until.hasObject(By.res(UI_PACKAGE_NAME, uiMenuButton)),
                            MENU_WAIT_TIME);
                } else {
                    throw new UnknownUiException(
                            "Fail to close other special mode before setting hfr mode");
                }
            }

            if (!mDevice.hasObject(By.res(UI_PACKAGE_NAME, UI_SPECIAL_MODE_CLOSE))) {
                // If the menu is not open, open it
                if (!isMenuOpen()) {
                    openMenu();
                }
                // Select Item "Slow Motion"
                selectSettingItem("Slow Motion");
                // Change Slow Motion mode to 120FPS or 240FPS
            }

            mDevice.waitForIdle();
            // Detect if hfr toggle exists in the interface
            if (!mDevice.hasObject(By.res(UI_PACKAGE_NAME, UI_HFR_TOGGLE_ID_J))) {
                if (mode == HFR_MODE_240_FPS) {
                    throw new UnknownUiException(
                            "The 240 fps HFR mode is not supported on the device.");
                }
                return;
            }

            for (int retries = 0; retries < 2; retries++) {
                if (!isHfrMode(mode)) {
                    getHfrToggleButton().click();
                    mDevice.waitForIdle();
                } else {
                    Log.e(LOG_TAG, "Successfully set HFR mode!");
                    mDevice.waitForIdle();
                    waitForVideoShutterEnabled();
                    return;
                }
            }
            //If neither of the 2 options match expected option, throw an exception
            throw new UnknownUiException(String.format("Failed to select HFR mode to FPS %d",
                    (int) Math.floor(mode * 120)));
        } else {
            throw new UnknownUiException("The Google Camera version is not supported.");
        }
    }

    private boolean isHfrMode(int mode) {
        if (mIsVersionI) {
            String modeDesc = getHfrToggleButton().getContentDescription();
            if (DESC_HFR_120_FPS.equals(modeDesc)) {
                return HFR_MODE_120_FPS == mode;
            } else if (DESC_HFR_240_FPS.equals(modeDesc)) {
                return HFR_MODE_240_FPS == mode;
            } else if (DESC_HFR_OFF.equals(modeDesc)) {
                return HFR_MODE_OFF == mode;
            } else {
                throw new UnknownUiException("Fail to identify HFR toggle description.");
            }
        } else if (mIsVersionJ || mIsVersionK) {
            if (getHfrToggleButton() == null) {
                return HFR_MODE_OFF == mode;
            }
            String modeDesc = getHfrToggleButton().getContentDescription();
            if (DESC_HFR_120_FPS.equals(modeDesc)) {
                return HFR_MODE_120_FPS == mode;
            } else if (DESC_HFR_240_FPS.equals(modeDesc)) {
                return HFR_MODE_240_FPS == mode;
            } else {
                throw new UnknownUiException("Fail to identify HFR toggle description.");
            }
        }
        return HFR_MODE_OFF == mode;
    }

    private void openModeOptions2X() {
        // If the mode option is already open, return as it is
        if (mDevice.hasObject(By.res(UI_PACKAGE_NAME, "mode_options_buttons"))) {
            return;
        }
        // Before openning the mode option, close the menu if the menu is open
        closeMenu();
        waitForVideoShutterEnabled();
        // Open the mode options to check HDR mode
        UiObject2 modeoptions = getModeOptionsMenuButton();
        if (modeoptions != null) {
            modeoptions.click();
            // If succeeded, the hdr toggle button should be visible.
            mDevice.wait(Until.hasObject(By.res(UI_PACKAGE_NAME, "hdr_plus_toggle_button")),
                    DIALOG_TRANSITION_WAIT);
        } else {
            throw new UnknownUiException(
                    "Fail to find modeoption button when trying to check HDR mode");
        }
    }

    private void openMenu() {
        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            String uiMenuButton = (mIsVersionK)? UI_MENU_BUTTON_ID_4X:UI_MENU_BUTTON_ID_3X;
            UiObject2 menu = mDevice.findObject(By.res(UI_PACKAGE_NAME, uiMenuButton));
            menu.click();
        } else {
            UiObject2 activityView = mDevice.wait(Until.findObject(
                    By.res(UI_PACKAGE_NAME, UI_ACTIVITY_VIEW_ID)), MENU_WAIT_TIME);
            activityView.swipe(Direction.RIGHT, 1.0f);
        }

        mDevice.wait(Until.hasObject(By.text("Photo Sphere")), MENU_WAIT_TIME);

        mDevice.waitForIdle();
    }

    private void selectMenuItem(String mode) {
        UiObject2 menuItem = mDevice.findObject(By.text(mode));
        if (menuItem != null) {
            menuItem.click();
        } else {
            throw new UnknownUiException(
                    String.format("Menu item button was not enabled with %d seconds",
                    (int)Math.floor(MENU_WAIT_TIME / 1000)));
        }
        mDevice.wait(Until.gone(By.text("Photo Sphere")), MENU_WAIT_TIME);

        mDevice.waitForIdle();
    }

    private void closeMenuItem() {
        UiObject2 navUp = mDevice.findObject(By.desc("Navigate up"));
        if (navUp != null) {
            navUp.click();
        } else {
            throw new UnknownUiException(String.format(
                    "Navigation up button was not enabled with %d seconds",
                    (int)Math.floor(MENU_WAIT_TIME / 1000)));
        }
        mDevice.wait(Until.gone(By.text("Help & feedback")), MENU_WAIT_TIME);

        mDevice.waitForIdle();
    }

    private void selectSettingItem(String mode) {
        UiObject2 settingItem = mDevice.findObject(By.text(mode));
        if (settingItem != null) {
            settingItem.click();
        } else {
            throw new UnknownUiException(
                    String.format("Setting item button was not enabled with %d seconds",
                    (int)Math.floor(MENU_WAIT_TIME / 1000)));
        }
        mDevice.wait(Until.gone(By.text("Help & feedback")), MENU_WAIT_TIME);

        mDevice.waitForIdle();
    }

    private void selectSetting2X() {
        UiObject2 settingItem = mDevice.findObject(By.desc("Settings"));
        if (settingItem != null) {
            settingItem.click();
        } else {
            throw new UnknownUiException(
                    String.format("Setting item button was not enabled with %d seconds",
                    (int)Math.floor(MENU_WAIT_TIME / 1000)));
        }
        mDevice.wait(Until.gone(By.text("Help & feedback")), MENU_WAIT_TIME);

        mDevice.waitForIdle();
    }

    private void closeSettingItem() {
        UiObject2 navUp = mDevice.findObject(By.desc("Navigate up"));
        if (navUp != null) {
            navUp.click();
        } else {
            throw new UnknownUiException(
                    String.format("Navigation up button was not enabled with %d seconds",
                    (int)Math.floor(MENU_WAIT_TIME / 1000)));
        }
        mDevice.wait(Until.findObject(By.text("Help & feedback")), MENU_WAIT_TIME);

        mDevice.waitForIdle();
    }

    private void selectVideoResolution(int mode) {
        String textBackVideoResolution =
                (mIsVersionK)? TEXT_BACK_VIDEO_RESOLUTION_4X:TEXT_BACK_VIDEO_RESOLUTION_3X;
        UiObject2 backCamera = mDevice.findObject(By.text(textBackVideoResolution));
        if (backCamera != null) {
            backCamera.click();
        } else {
            throw new UnknownUiException(
                    String.format("Back camera button was not enabled with %d seconds",
                    (int)Math.floor(MENU_WAIT_TIME / 1000)));
        }
        mDevice.wait(Until.findObject(By.text("CANCEL")), MENU_WAIT_TIME);
        mDevice.waitForIdle();

        if (mode == VIDEO_4K_MODE_ON) {
            mDevice.wait(Until.findObject(By.text(TEXT_4K_ON)), MENU_WAIT_TIME).click();
        } else if (mode == VIDEO_HD_1080) {
            mDevice.wait(Until.findObject(By.text(TEXT_HD_1080)), MENU_WAIT_TIME).click();
        } else if (mode == VIDEO_HD_720){
            mDevice.wait(Until.findObject(By.text(TEXT_HD_720)), MENU_WAIT_TIME).click();
        } else {
            throw new UnknownUiException("Failed to set video resolution");
        }

        mDevice.wait(Until.gone(By.text("CANCEL")), MENU_WAIT_TIME);

        mDevice.waitForIdle();
    }

    private void closeMenu() {
        // Should only call this function when menu is open, do nothing if menu is not open
        if (!isMenuOpen()) {
            return;
        }

        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            // Click menu button to close menu (this is NOT for taking pictures)
            String uiMenuButton = (mIsVersionK)? UI_MENU_BUTTON_ID_4X:UI_MENU_BUTTON_ID_3X;
            UiObject2 backButton = mDevice.findObject(By.res(UI_PACKAGE_NAME, uiMenuButton));
            if (backButton != null) {
                backButton.click();
            }
        } else {
            // Click shutter button to close menu (this is NOT for taking pictures)
            UiObject2 shutter = mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_SHUTTER_BUTTON_ID_2X));
            if (shutter != null) {
                shutter.click();
            }
        }
    }

    private boolean isCameraMode() {
        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            return (mDevice.hasObject(By.desc(UI_SHUTTER_DESC_CAM_3X)));
        } else {
            // TODO: identify a Haleakala UiObject2 unique Camera mode
            return !isVideoMode();
        }
    }

    private boolean isVideoMode() {
        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            return (mDevice.hasObject(By.desc(UI_SHUTTER_DESC_VID_3X)));
        } else {
            return (mDevice.hasObject(By.res(UI_PACKAGE_NAME, "recording_time_rect")));
        }
    }

    private boolean isRecording() {
        return mDevice.hasObject(By.res(UI_PACKAGE_NAME, UI_RECORDING_TIME_ID));
    }

    private boolean isFrontCamera() {
        // Close menu if open
        closeMenu();

        if (mIsVersionJ || mIsVersionK) {
            return (mDevice.hasObject(By.desc("Switch to back camera")));
        } else if (mIsVersionI) {
            return (mDevice.hasObject(By.desc("Front camera")));
        } else {
            // Open mode options if not open
            UiObject2 modeoptions = getModeOptionsMenuButton();
            if (modeoptions != null) {
                modeoptions.click();
            }
            return (mDevice.hasObject(By.desc("Front camera")));
        }
    }

    private boolean isBackCamera() {
        // Close menu if open
        closeMenu();

        if (mIsVersionJ || mIsVersionK) {
            return (mDevice.hasObject(By.desc("Switch to front camera")));
        } else if (mIsVersionI) {
            return (mDevice.hasObject(By.desc("Back camera")));
        } else {
            // Open mode options if not open
            UiObject2 modeoptions = getModeOptionsMenuButton();
            if (modeoptions != null) {
                modeoptions.click();
            }
            return (mDevice.hasObject(By.desc("Back camera")));
        }
    }

    private boolean isMenuOpen() {
        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            if (mDevice.hasObject(By.desc("Open settings"))) {
                return true;
            }
        } else {
            if (mDevice.hasObject(By.res(UI_PACKAGE_NAME, UI_SETTINGS_BUTTON_ID))) {
                return true;
            }
        }
        return false;
    }

    private void pressBackFrontToggleButton() {
        UiObject2 toggle = getBackFrontToggleButton();
        if (toggle != null) {
            toggle.click();
        } else {
            throw new UnknownUiException("Failed to detect a back-front toggle button");
        }
    }

    private UiObject2 getCameraVideoToggleButton() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_TOGGLE_BUTTON_ID));
    }

    private UiObject2 getBackFrontToggleButton() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_BACK_FRONT_TOGGLE_BUTTON_ID));
    }

    private UiObject2 getHdrToggleButton() {
        if (mIsVersionK) {
            return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_HDR_BUTTON_ID_4X));
        } else if (mIsVersionI || mIsVersionJ) {
            return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_HDR_BUTTON_ID_3X));
        } else {
            return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_HDR_BUTTON_ID_2X));
        }
    }

    private UiObject2 getHfrToggleButton() {
        if (mIsVersionI) {
            return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_HFR_TOGGLE_ID_I));
        } else if (mIsVersionJ || mIsVersionK) {
            return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_HFR_TOGGLE_ID_J));
        } else {
            throw new UnsupportedOperationException(
                    "HFR not supported on this version of Google Camera.");
        }
    }

    private UiObject2 getModeOptionsMenuButton() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_MODE_OPTION_TOGGLE_BUTTON_ID));
    }

    private UiObject2 getCameraShutter() {
        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            return mDevice.findObject(By.desc(UI_SHUTTER_DESC_CAM_3X).enabled(true));
        } else {
            return mDevice.findObject(By.desc(UI_SHUTTER_DESC_CAM_2X).enabled(true));
        }
    }

    private UiObject2 getVideoShutter() {
        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            return mDevice.findObject(By.desc(UI_SHUTTER_DESC_VID_3X).enabled(true));
        } else {
            return mDevice.findObject(By.desc(UI_SHUTTER_DESC_VID_2X).enabled(true));
        }
    }

    private UiObject2 getThumbnailAlbumButton() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_THUMBNAIL_ALBUM_BUTTON_ID));
    }

    private UiObject2 getAlbumFilmstripView() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_ALBUM_FILMSTRIP_VIEW_ID));
    }

    /**
     * {@inheritDoc}
     */
    public void waitForCameraShutterEnabled() {
        boolean uiSuccess = false;

        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            uiSuccess = mDevice.wait(Until.hasObject(
                    By.desc(UI_SHUTTER_DESC_CAM_3X).enabled(true)), SHUTTER_WAIT_TIME);
        } else {
            uiSuccess = mDevice.wait(Until.hasObject(
                    By.desc(UI_SHUTTER_DESC_CAM_2X).enabled(true)), SHUTTER_WAIT_TIME);
        }

        if (!uiSuccess) {
            throw new UnknownUiException(
                    String.format("Camera shutter was not enabled with %d seconds",
                    (int)Math.floor(SHUTTER_WAIT_TIME / 1000)));
        }
    }

    /**
     * {@inheritDoc}
     */
    public void waitForVideoShutterEnabled() {
        boolean uiSuccess = false;

        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            uiSuccess = mDevice.wait(Until.hasObject(
                    By.desc(UI_SHUTTER_DESC_VID_3X).enabled(true)), SHUTTER_WAIT_TIME);
        } else {
            uiSuccess = mDevice.wait(Until.hasObject(
                    By.desc(UI_SHUTTER_DESC_VID_2X).enabled(true)), SHUTTER_WAIT_TIME);
        }

        if (!uiSuccess) {
            throw new UnknownUiException(
                    String.format("Video shutter was not enabled with %d seconds",
                    (int)Math.floor(SHUTTER_WAIT_TIME / 1000)));
        }
    }

    private void waitForCurrentShutterEnabled() {
        // This function is called to wait for shutter button enabled in either camera or video mode
        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            mDevice.wait(Until.hasObject(By.res(UI_PACKAGE_NAME, UI_SHUTTER_BUTTON_ID_3X).enabled(true)),
                    SHUTTER_WAIT_TIME);
        } else {
            mDevice.wait(Until.hasObject(By.res(UI_PACKAGE_NAME, UI_SHUTTER_BUTTON_ID_2X).enabled(true)),
                    SHUTTER_WAIT_TIME);
        }
    }

    private void waitForBackEnabled() {
        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            mDevice.wait(Until.hasObject(By.desc("Switch to front camera").enabled(true)),
                    SWITCH_WAIT_TIME);
        } else {
            mDevice.wait(Until.hasObject(By.desc("Back camera").enabled(true)),
                    SWITCH_WAIT_TIME);
        }
    }

    private void waitForFrontEnabled() {
        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            mDevice.wait(Until.hasObject(By.desc("Switch to back camera").enabled(true)),
                    SWITCH_WAIT_TIME);
        } else {
            mDevice.wait(Until.hasObject(By.desc("Front camera").enabled(true)),
                    SWITCH_WAIT_TIME);
        }
    }

    private void waitForHFRToggleEnabled() {
        if (mIsVersionJ || mIsVersionK) {
            mDevice.wait(Until.hasObject(By.res(UI_PACKAGE_NAME, UI_HFR_TOGGLE_ID_J).enabled(true)),
                    SWITCH_WAIT_TIME);
        } else if (mIsVersionI) {
            mDevice.wait(Until.hasObject(By.res(UI_PACKAGE_NAME, UI_HFR_TOGGLE_ID_I).enabled(true)),
                    SWITCH_WAIT_TIME);
        } else {
            throw new UnknownUiException("HFR is not supported on this version of Google Camera");
        }
    }

    private void waitForAppInit() {
        boolean initalized = false;
        if (mIsVersionI || mIsVersionJ || mIsVersionK) {
            String uiMenuButton = (mIsVersionK)? UI_MENU_BUTTON_ID_4X:UI_MENU_BUTTON_ID_3X;
            initalized = mDevice.wait(Until.hasObject(By.res(UI_PACKAGE_NAME, uiMenuButton)),
                    APP_INIT_WAIT);
        } else {
            initalized = mDevice.wait(Until.hasObject(By.res(UI_PACKAGE_NAME, UI_MODE_OPTION_TOGGLE_BUTTON_ID)),
                    APP_INIT_WAIT);
        }

        waitForCurrentShutterEnabled();

        mDevice.waitForIdle();

        if (initalized) {
            Log.e(LOG_TAG, "Successfully initialized.");
        } else {
            Log.e(LOG_TAG, "Failed to find initialization indicator.");
        }
    }

    /**
     * TODO: Temporary. Create long-term solution for registering watchers.
     */
    public void registerCrashWatcher() {
        final UiDevice fDevice = mDevice;

        mDevice.registerWatcher("GoogleCamera-crash-watcher", new UiWatcher() {
            @Override
            public boolean checkForCondition() {
                Pattern dismissWords =
                        Pattern.compile("DISMISS", Pattern.CASE_INSENSITIVE);
                UiObject2 buttonDismiss = fDevice.findObject(By.text(dismissWords).enabled(true));
                if (buttonDismiss != null) {
                    buttonDismiss.click();
                    throw new UnknownUiException("Camera crash dialog encountered. Failing test.");
                }

                return false;
            }
        });
    }

    /**
     * TODO: Temporary. Create long-term solution for registering watchers.
     */
    public void unregisterCrashWatcher() {
        mDevice.removeWatcher("GoogleCamera-crash-watcher");
    }

    /**
     * TODO: Should only be temporary
     * {@inheritDoc}
     */
    public String openWithShutterTimeString() {
        String pkg = getPackage();
        String id = getLauncherName();

        long launchStart = ILauncherStrategy.LAUNCH_FAILED_TIMESTAMP;
        if (!mDevice.hasObject(By.pkg(pkg).depth(0))) {
            launchStart = mLauncherStrategy.launch(id, pkg);
        }

        if (launchStart == ILauncherStrategy.LAUNCH_FAILED_TIMESTAMP) {
            throw new UnknownUiException("Failed to launch GoogleCamera.");
        }

        waitForAppInit();
        waitForCurrentShutterEnabled();
        long launchDuration = SystemClock.uptimeMillis() - launchStart;

        Date dateNow = new Date();
        DateFormat dateFormat = new SimpleDateFormat("MM-dd HH:mm:ss.SSS");
        String dateString = dateFormat.format(dateNow);

        if (isCameraMode()) {
            return String.format("%s %s %d\n", dateString, "camera", launchDuration);
        } else if (isVideoMode()) {
            return String.format("%s %s %d\n", dateString, "video", launchDuration);
        } else {
            return String.format("%s %s %d\n", dateString, "wtf", launchDuration);
        }
    }

    /**
     * {@inheritDoc}
     */
    public void goToAlbum() {
        UiObject2 thumbnailAlbumButton = getThumbnailAlbumButton();
        if (thumbnailAlbumButton == null) {
            throw new UnknownUiException("Could not find thumbnail album button");
        }

        thumbnailAlbumButton.click();
        if (!mDevice.wait(Until.hasObject(
                By.res(UI_PACKAGE_NAME, UI_ALBUM_FILMSTRIP_VIEW_ID)), DIALOG_TRANSITION_WAIT)) {
            throw new UnknownUiException("Could not find album filmstrip");
        }
    }

    /**
     * {@inheritDoc}
     */
    public void scrollAlbum(Direction direction) {
        if (!(Direction.LEFT.equals(direction) || Direction.RIGHT.equals(direction))) {
            throw new IllegalArgumentException("direction must be LEFT or RIGHT");
        }

        UiObject2 albumFilmstripView = getAlbumFilmstripView();
        if (albumFilmstripView == null) {
            throw new UnknownUiException("Could not find album filmstrip view");
        }

        albumFilmstripView.scroll(direction, 5.0f);
        mDevice.waitForIdle();
    }
}
