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
import android.platform.test.helpers.exceptions.UnknownUiException;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.Until;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.util.Log;

import java.util.List;
import java.util.regex.Pattern;

import junit.framework.Assert;


import android.os.Environment;
import java.io.File;
import java.io.IOException;

public class PhotosHelperImpl extends AbstractPhotosHelper {
    private static final String LOG_TAG = PhotosHelperImpl.class.getSimpleName();

    private static final long APP_LOAD_WAIT = 7500;
    private static final long HACKY_WAIT = 2500;
    private static final long PICTURE_LOAD_WAIT = 20000;
    private static final long UI_NAVIGATION_WAIT = 5000;

    private static final Pattern UI_PHOTO_DESC = Pattern.compile("^Photo.*");

    private static final String UI_DONE_BUTTON_ID = "done_button";
    private static final String UI_GET_STARTED_CONTAINER = "get_started_container";
    private static final String UI_GET_STARTED_ID = "get_started";
    private static final String UI_LOADING_ICON_ID = "list_empty_progress_bar";
    private static final String UI_NEXT_BUTTON_ID = "next_button";
    private static final String UI_PACKAGE_NAME = "com.google.android.apps.photos";
    private static final String UI_PHOTO_TAB_ID = "tab_photos";
    private static final String UI_DEVICE_FOLDER_TEXT = "Device folders";
    private static final String UI_PHOTO_VIEW_PAGER_ID = "photo_view_pager";
    private static final String UI_PHOTO_SCROLL_VIEW_ID = "recycler_view";
    private static final String UI_NAVIGATION_LIST_ID = "navigation_list";
    private static final int MAX_UI_SCROLL_COUNT = 20;
    private static final int MAX_DISMISS_INIT_DIALOG_RETRY = 20;

    public PhotosHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        return "com.google.android.apps.photos";
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        return "Photos";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        // Target Photos version 1.18.0.119671374
        SystemClock.sleep(APP_LOAD_WAIT);

        if (isOnInitialDialogScreen()) {
            UiObject2 getStartedButton = mDevice.wait(
                    Until.findObject(By.res(UI_PACKAGE_NAME, UI_GET_STARTED_ID)), APP_LOAD_WAIT);
            int retryCount = 0;
            while ((retryCount < MAX_DISMISS_INIT_DIALOG_RETRY) &&
                   (getStartedButton == null)) {
                /*
                  The UiAutomator sometimes cannot find GET STARTED button even though
                  it is seen on the screen.
                  The reason is because the initial "spinner" animation screen updates
                  views too quickly for UiAutomator to catch the change.

                  The following hack is used to reload the init dialog for UiAutomator to
                  retry catching the GET STARTED button.
                */

                mDevice.pressBack();
                mDevice.waitForIdle();
                mDevice.pressHome();
                mDevice.waitForIdle();
                open();

                getStartedButton = mDevice.wait(
                        Until.findObject(By.res(UI_PACKAGE_NAME, UI_GET_STARTED_ID)),
                        APP_LOAD_WAIT);
                retryCount += 1;

                if (!isOnInitialDialogScreen()) {
                    break;
                }
            }

            if (isOnInitialDialogScreen() && (getStartedButton == null)) {
                throw new IllegalStateException("UiAutomator cannot catch GET STARTED button");
            }
            else {
                if (getStartedButton != null) {
                    getStartedButton.click();
                }
            }
        }
        else {
            Log.e(LOG_TAG, "Didn't find GET STARTED button.");
        }

        // Address dialogs with an account vs. without an account
        Pattern signInWords = Pattern.compile("Sign in", Pattern.CASE_INSENSITIVE);
        boolean hasAccount = !mDevice.hasObject(By.text(signInWords));
        if (!hasAccount) {
            // Select 'NO THANKS' if no account exists
            Pattern noThanksWords = Pattern.compile("No thanks", Pattern.CASE_INSENSITIVE);
            UiObject2 noThanksButton = mDevice.findObject(By.text(noThanksWords));
            if (noThanksButton != null) {
                noThanksButton.click();
                mDevice.waitForIdle();
            } else {
                Log.e(LOG_TAG, "Unable to find NO THANKS button.");
            }
        } else {
            UiObject2 doneButton = mDevice.wait(Until.findObject(
                    By.res(UI_PACKAGE_NAME, UI_DONE_BUTTON_ID)), 5000);
            if (doneButton != null) {
                doneButton.click();
                mDevice.waitForIdle();
            }
            else {
                Log.e(LOG_TAG, "Didn't find DONE button.");
            }

            // Press the next button (arrow and check mark) four consecutive times
            for (int repeat = 0; repeat < 4; repeat++) {
                UiObject2 nextButton = mDevice.findObject(
                        By.res(UI_PACKAGE_NAME, UI_NEXT_BUTTON_ID));
                if (nextButton != null) {
                    nextButton.click();
                    mDevice.waitForIdle();
                } else {
                    Log.e(LOG_TAG, "Unable to find arrow or check mark buttons.");
                }
            }

            mDevice.wait(Until.gone(
                         By.res(UI_PACKAGE_NAME, UI_LOADING_ICON_ID)), PICTURE_LOAD_WAIT);
        }

        mDevice.waitForIdle();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void openFirstClip() {
        if (searchForVideoClip()) {
            UiObject2 clip = getFirstClip();
            if (clip != null) {
                clip.click();
                mDevice.wait(Until.findObject(
                        By.res(UI_PACKAGE_NAME, "photos_videoplayer_play_button_holder")), 2000);
            }
            else {
                throw new IllegalStateException("Cannot play a video after finding video clips");
            }
        }
        else {
            throw new UnsupportedOperationException("Cannot find a video clip");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void pauseClip() {
        UiObject2 holder = mDevice.findObject(
                By.res(UI_PACKAGE_NAME, "photos_videoplayer_play_button_holder"));
        if (holder != null) {
            holder.click();
        } else {
            throw new UnknownUiException("Unable to find pause button holder.");
        }

        UiObject2 pause = mDevice.wait(Until.findObject(
                By.res(UI_PACKAGE_NAME, "photos_videoplayer_pause_button")), 2500);
        if (pause != null) {
            pause.click();
            mDevice.wait(Until.findObject(By.desc("Play video")), 2500);
        } else {
            throw new UnknownUiException("Unable to find pause button.");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void playClip() {
        UiObject2 play = mDevice.findObject(By.desc("Play video"));
        if (play != null) {
            play.click();
            mDevice.wait(Until.findObject(
                    By.res(UI_PACKAGE_NAME, "photos_videoplayer_pause_button")), 2500);
        } else {
            throw new UnknownUiException("Unable to find play button");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToMainScreen() {
        for (int retriesRemaining = 5; retriesRemaining > 0 && !isOnMainScreen();
                --retriesRemaining) {
            // check if we see the Photos tab at the bottom of the screen
            // If we do, clicking on the tab should go to home screen.
            UiObject2 photosButton = mDevice.findObject(
                    By.res(UI_PACKAGE_NAME, UI_PHOTO_TAB_ID));
            if (photosButton != null) {
                photosButton.click();
            }
            else {
                mDevice.pressBack();
            }
            mDevice.waitForIdle();
        }

        if (!isOnMainScreen()) {
            throw new IllegalStateException("Cannot go to main screen");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void openPicture(int index) {

        mDevice.waitForIdle();
        List<UiObject2> photos = mDevice.findObjects(By.pkg(UI_PACKAGE_NAME).desc(UI_PHOTO_DESC));

        if (photos == null) {
            throw new IllegalStateException("Cannot find photos on current view screen");
        }

        if ((index < 0) || (index >= photos.size())) {
            String errMsg = String.format("Photo index (%d) out of bound (0..%d)",
                                          index, photos.size());
            throw new IllegalArgumentException(errMsg);
        }

        UiObject2 photo = photos.get(index);
        photo.click();
        if (!mDevice.wait(Until.hasObject(By.res(UI_PACKAGE_NAME, UI_PHOTO_VIEW_PAGER_ID)),
                UI_NAVIGATION_WAIT)) {
            throw new IllegalStateException("Cannot display photo on screen");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void scrollAlbum(Direction direction) {
        if (!(Direction.LEFT.equals(direction) || Direction.RIGHT.equals(direction))) {
            throw new IllegalArgumentException("Scroll direction must be LEFT or RIGHT");
        }

        UiObject2 scrollContainer = mDevice.findObject(
                By.res(UI_PACKAGE_NAME, UI_PHOTO_VIEW_PAGER_ID));

        if (scrollContainer == null) {
            throw new UnknownUiException("Cannot find scroll container");
        }

        scrollContainer.scroll(direction, 1.0f);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToDeviceFolderScreen() {
        if (!isOnDeviceFolderScreen()) {

            if (!isOnMainScreen()) {
                goToMainScreen();
            }

            openNavigationDrawer();

            UiObject2 deviceFolderButton = mDevice.wait(Until.findObject(
                                               By.text(UI_DEVICE_FOLDER_TEXT)), UI_NAVIGATION_WAIT);
            if (deviceFolderButton != null) {
                deviceFolderButton.click();
            }
            else {
                UiObject2 photosButton = mDevice.wait(Until.findObject(By.text("Photos")),
                                                      UI_NAVIGATION_WAIT);
                if (photosButton != null) {
                    photosButton.click();
                }
                else {
                    throw new IllegalStateException("No device folder in navigation drawer");
                }
            }
        }

        if (!isOnDeviceFolderScreen()) {
            throw new UnknownUiException("Can not go to device folder screen");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean searchForDeviceFolder(String folderName) {
        boolean foundFolder = false;
        int scrollCount = 0;
        while (!foundFolder && (scrollCount < MAX_UI_SCROLL_COUNT)) {
            foundFolder = mDevice.wait(Until.hasObject(By.text(folderName)), 2000);
            if (!foundFolder) {
                if (!scrollView(Direction.DOWN)) {
                    break;
                }
            }
            scrollCount += 1;
        }

        if (!foundFolder) {
            foundFolder = mDevice.wait(Until.hasObject(By.text(folderName)), 2000);
        }

        return foundFolder;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean searchForVideoClip() {
        boolean foundVideoClip = false;
        int scrollCount = 0;
        while (!foundVideoClip && (scrollCount < MAX_UI_SCROLL_COUNT)) {
            foundVideoClip = (getFirstClip() != null);
            if (!foundVideoClip) {
                if (!scrollView(Direction.DOWN)) {
                    break;
                }
            }
            scrollCount += 1;
        }
        return foundVideoClip;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean searchForPicture() {
        boolean foundPicture = false;
        int scrollCount = 0;
        while (!foundPicture && (scrollCount < MAX_UI_SCROLL_COUNT)) {
            foundPicture = mDevice.wait(Until.hasObject(By.descStartsWith("Photo")), 2000);
            if (!foundPicture) {
                if (!scrollView(Direction.DOWN)) {
                    break;
                }
            }
            scrollCount += 1;
        }
        return foundPicture;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void openDeviceFolder(String folderName) {
        UiObject2 deviceFolder = mDevice.wait(Until.findObject(By.text(folderName)),
                                              UI_NAVIGATION_WAIT);
        if (deviceFolder != null) {
            deviceFolder.click();
        }
        else {
            throw new IllegalArgumentException(String.format("Cannot open device folder %s",
                                                             folderName));
        }
    }

    private UiObject2 getFirstClip() {
        return mDevice.wait(Until.findObject(By.descStartsWith("Video")), 2000);
    }

    /**
     *  This function returns true if Photos is currently on the first-use
     *  initial dialog screen, with "Get Started" button displayed on screen
     *
     * @return Returns true if app is on the initial dialog screen, false otherwise
     */
    private boolean isOnInitialDialogScreen() {
        return mDevice.hasObject(By.res(UI_PACKAGE_NAME, UI_GET_STARTED_CONTAINER));
    }

    private boolean isOnMainScreen() {
        return mDevice.hasObject(By.descContains("Photos, selected"));
    }

    /**
     *  This function returns true if Photos is currently in the
     *  photo-viewing screen, displaying either one photo
     *  or video on the screen.
     *
     * @return Returns true if one photo or video is displayed on the screen,
     *         false otherwise.
     */
    private boolean isOnPhotoViewingScreen() {
        return mDevice.hasObject(By.res(UI_PACKAGE_NAME, UI_PHOTO_VIEW_PAGER_ID));
    }

    private boolean isOnDeviceFolderScreen() {

        if (mDevice.hasObject(By.pkg(UI_PACKAGE_NAME).text(UI_DEVICE_FOLDER_TEXT))) {
            return true;
        }

        // sometimes the "Device Folder" tab is hidden.
        // scroll down once to make sure the tab is visible
        UiObject2 scrollContainer = mDevice.findObject(
                                        By.res(UI_PACKAGE_NAME, UI_PHOTO_SCROLL_VIEW_ID));
        if (scrollContainer != null) {
            scrollContainer.scroll(Direction.DOWN, 1.0f);
            return mDevice.hasObject(By.pkg(UI_PACKAGE_NAME).text(UI_DEVICE_FOLDER_TEXT));
        }
        else {
            return false;
        }
    }

    /**
     * This function performs one scroll on the current screen, in the direction
     * specified by input argument.
     *
     * @param dir The direction of the scroll
     * @return Returns whether the object can still scroll in the given direction
     */
   private boolean scrollView(Direction dir) {
        UiObject2 scrollContainer = mDevice.findObject(By.res(UI_PACKAGE_NAME,
                                                              UI_PHOTO_SCROLL_VIEW_ID));
        if (scrollContainer == null) {
            return false;
        }

        return scrollContainer.scroll(dir, 1.0f);
    }

    private void openNavigationDrawer() {
        UiObject2 navigationDrawer = mDevice.findObject(By.desc("Show Navigation Drawer"));
        if (navigationDrawer == null) {
            mDevice.pressBack();
            navigationDrawer = mDevice.wait(Until.findObject(By.desc("Show Navigation Drawer")),
                                            UI_NAVIGATION_WAIT);
        }

        if (navigationDrawer == null) {
            throw new UnknownUiException("Cannot find navigation drawer");
        }

        navigationDrawer.click();

        if (!mDevice.hasObject(By.res(UI_PACKAGE_NAME, UI_NAVIGATION_LIST_ID))) {
            throw new UnknownUiException("Cannot open navigation drawer");
        }
    }
}
