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
import android.content.res.Configuration;
import android.graphics.Point;
import android.os.SystemClock;
import android.platform.test.helpers.exceptions.UiTimeoutException;
import android.platform.test.helpers.exceptions.UnknownUiException;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.Until;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.util.Log;

import java.util.regex.Pattern;

public class YouTubeHelperImpl extends AbstractYouTubeHelper {
    private static final String TAG = AbstractYouTubeHelper.class.getSimpleName();

    private static final String UI_ACCOUNT_BUTTON_DESC = "Account";
    private static final String UI_HOME_CONTAINER_ID = "results";
    private static final String UI_FULLSCREEN_BUTTON_DESC = "Enter fullscreen";
    private static final String UI_HELP_AND_FEEDBACK_TEXT = "Help & feedback";
    private static final String UI_HOME_BUTTON_DESC = "Home";
    private static final String UI_HOME_PAGE_VIDEO_ID = "event_item";
    private static final String UI_VIDEO_INFO_VIEW_ID = "video_info_view";
    private static final String UI_PACKAGE_NAME = "com.google.android.youtube";
    private static final String UI_PLAY_VIDEO_DESC = "Play video";
    private static final String UI_PROGRESS_ID = "load_progress";
    private static final String UI_RESULT_FILTER_ID = "menu_filter_results";
    private static final String UI_SEARCH_BUTTON_ID = "menu_search";
    private static final String UI_SEARCH_EDIT_TEXT_ID = "search_edit_text";
    private static final String UI_SELECT_DIALOG_LISTVIEW_ID = "select_dialog_listview";
    private static final String UI_TRENDING_BUTTON_DESC = "Trending";
    private static final String UI_VIDEO_PLAYER_ID = "watch_player";
    private static final String UI_VIDEO_PLAYER_OVERFLOW_BUTTON_ID = "player_overflow_button";
    private static final String UI_VIDEO_PLAYER_PLAY_PAUSE_REPLAY_BUTTON_ID =
            "player_control_play_pause_replay_button";
    private static final String UI_VIDEO_PLAYER_QUALITY_BUTTON_ID = "quality_button";

    private static final long MAX_HOME_LOAD_WAIT = 30 * 1000;
    private static final long MAX_VIDEO_LOAD_WAIT = 30 * 1000;

    private static final long APP_INIT_WAIT = 20000;
    private static final long STANDARD_DIALOG_WAIT = 5000;
    private static final long UI_NAVIGATION_WAIT = 5000;

    public YouTubeHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        return "com.google.android.youtube";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        return "YouTube";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        BySelector dialog1 = By.text("OK");
        // Dismiss the splash screen that might appear on first start.
        UiObject2 splash = mDevice.wait(Until.findObject(dialog1), APP_INIT_WAIT);
        if (splash != null) {
            splash.click();
            mDevice.wait(Until.gone(dialog1), STANDARD_DIALOG_WAIT);
        }

        UiObject2 laterButton = mDevice.wait(Until.findObject(
                By.res(UI_PACKAGE_NAME, "later_button")), STANDARD_DIALOG_WAIT);
        if (laterButton != null) {
            laterButton.clickAndWait(Until.newWindow(), STANDARD_DIALOG_WAIT);
        }

        UiObject2 helpAndFeedbackButton = mDevice.findObject(
            By.pkg(UI_PACKAGE_NAME).text(UI_HELP_AND_FEEDBACK_TEXT));
        if (helpAndFeedbackButton != null) {
            mDevice.pressBack();
            mDevice.wait(Until.gone(By.pkg(UI_PACKAGE_NAME).text(UI_HELP_AND_FEEDBACK_TEXT)),
                STANDARD_DIALOG_WAIT);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void playHomePageVideo() {
        if (!isOnHomePage()) {
            throw new IllegalStateException("YouTube is not on the home page.");
        }

        if (hasConnectionEstablishedMessage()) {
            pressGoOnline();
        }

        for (int i = 0; i < 3; i++) {
            UiObject2 video = getPlayableVideo();
            if (video != null) {
                video.click();
                waitForVideoToLoad(UI_NAVIGATION_WAIT);
                return;
            } else {
                scrollHomePage(Direction.DOWN);
            }
        }

        if (isLoading()) {
            throw new UiTimeoutException("Timed out waiting for video search results.");
        }

        throw new UnknownUiException("Unsuccessful attempt playing home page video.");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void playSearchResultPageVideo() {
        if (!isOnSearchResultsPage()) {
            throw new IllegalStateException("YouTube is not on the home page.");
        }

        for (int i = 0; i < 3; i++) {
            UiObject2 video = getPlayableVideo();
            if (video != null) {
                video.click();
                waitForVideoToLoad(UI_NAVIGATION_WAIT);
                return;
            } else {
                scrollSearchResultsPage(Direction.DOWN);
            }
        }

        throw new UnknownUiException("Unsuccessful attempt playing search result video.");
    }

    private UiObject2 getHomePageContainer() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_HOME_CONTAINER_ID));
    }

    private UiObject2 getSearchResultsPageContainer() {
        return getHomePageContainer();
    }

    private UiObject2 getHomeButton() {
        return mDevice.findObject(By.pkg(UI_PACKAGE_NAME).desc(UI_HOME_BUTTON_DESC));
    }

    private UiObject2 getTrendingButton() {
        return mDevice.findObject(By.pkg(UI_PACKAGE_NAME).desc(UI_TRENDING_BUTTON_DESC));
    }

    private UiObject2 getAccountButton() {
        return mDevice.findObject(By.pkg(UI_PACKAGE_NAME).desc(UI_ACCOUNT_BUTTON_DESC));
    }

    private UiObject2 getSearchButton() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_SEARCH_BUTTON_ID));
    }

    private void scrollHomePage(Direction dir) {
        if (dir == Direction.RIGHT || dir == Direction.LEFT) {
            throw new IllegalArgumentException("Can only scroll up and down.");
        }

        UiObject2 scrollContainer = getHomePageContainer();
        if (scrollContainer != null) {
            scrollContainer.scroll(dir, 1.0f);
            mDevice.waitForIdle();
        } else {
            throw new UnknownUiException("No scrolling mechanism found.");
        }
    }

    private void scrollSearchResultsPage(Direction dir) {
        if (dir == Direction.RIGHT || dir == Direction.LEFT) {
            throw new IllegalArgumentException("Can only scroll up and down.");
        }

        UiObject2 scrollContainer = getSearchResultsPageContainer();
        if (scrollContainer != null) {
            scrollContainer.scroll(dir, 1.0f);
            mDevice.waitForIdle();
        } else {
            throw new UnknownUiException("No scrolling mechanism found.");
        }
    }

    private boolean isLoading() {
        // TODO: Is loading what? Requires more documentation.
        return mDevice.hasObject(By.res(UI_PACKAGE_NAME, UI_PROGRESS_ID));
    }

    private boolean isOnHomePage() {
        UiObject2 homeButton = getHomeButton();
        return (homeButton != null && homeButton.isSelected());
    }

    private boolean isOnTrendingPage() {
        UiObject2 trendingButton = getTrendingButton();
        return (trendingButton != null && trendingButton.isSelected());
    }

    private boolean isOnAccountPage() {
        UiObject2 accountButton = getAccountButton();
        return (accountButton != null && accountButton.isSelected());
    }

    private boolean isOnSearchResultsPage() {
        // Simplest way to identify search result page is the result filter button.
        UiObject2 resultFilterButton =
                mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_RESULT_FILTER_ID));
        return (resultFilterButton != null);
    }

    private UiObject2 getPlayableVideo() {
        UiObject2 video = mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_HOME_PAGE_VIDEO_ID));
        if (video == null) {
            video = mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_VIDEO_INFO_VIEW_ID));
        }
        return video;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean waitForVideoToLoad(long timeout) {
        return mDevice.wait(Until.hasObject(
            By.res(UI_PACKAGE_NAME, UI_VIDEO_PLAYER_ID)), timeout);
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public void goToHomePage() {
        for (int retriesRemaining = 5; retriesRemaining > 0 && getHomeButton() == null &&
                getTrendingButton() == null && getAccountButton() == null; --retriesRemaining) {
            mDevice.pressBack();
            SystemClock.sleep(3000);
        }
        // Get and press the home button
        UiObject2 homeButton = getHomeButton();
        if (homeButton == null) {
            throw new UnknownUiException("Could not find home button.");
        } else if (!homeButton.isSelected()) {
            homeButton.click();
            // Validate the home button is selected
            if (!mDevice.wait(Until.hasObject(
                    By.pkg(UI_PACKAGE_NAME).desc(UI_HOME_BUTTON_DESC).selected(true)),
                    UI_NAVIGATION_WAIT)) {
                throw new UnknownUiException("Not on home page after pressing home button.");
            } else {
                // Make sure the transition is complete
                mDevice.waitForIdle();
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToSearchPage() {
        if (!isOnHomePage()) {
            throw new IllegalStateException("YouTube is not on the home page.");
        }

        UiObject2 searchButton = getSearchButton();
        if (searchButton == null) {
            throw new UnknownUiException("Could not find search button.");
        } else {
            searchButton.click();
            if (!mDevice.wait(Until.hasObject(
                    By.res(UI_PACKAGE_NAME, UI_SEARCH_EDIT_TEXT_ID)), UI_NAVIGATION_WAIT)) {
                throw new UnknownUiException("Not on search page after pressing search button.");
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToFullscreenMode() {
        if (!isOnVideo()) {
            throw new IllegalStateException("YouTube is not on a video page.");
        }

        if (getOrientation() == Configuration.ORIENTATION_LANDSCAPE) {
            return;
        }

        UiObject2 fullscreenButton = null;
        for (int retriesRemaining = 5; retriesRemaining > 0; --retriesRemaining) {
            UiObject2 miniVideoPlayer = getVideoPlayer();
            if (miniVideoPlayer == null) {
                throw new UnknownUiException("Could not find mini video player.");
            }

            miniVideoPlayer.click();
            SystemClock.sleep(1500);
            fullscreenButton = getFullscreenButton();
            if (fullscreenButton != null) {
                fullscreenButton.click();
                // TODO: Add a valid wait for fullscreen
                break;
            }
        }

        if (fullscreenButton == null) {
            throw new UnknownUiException("Did not find a fullscreen button.");
        }
    }

    private UiObject2 getVideoPlayer() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_VIDEO_PLAYER_ID));
    }

    private boolean isOnVideo() {
        return (getVideoPlayer() != null);
    }

    private UiObject2 getVideoPlayerOverflowButton() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_VIDEO_PLAYER_OVERFLOW_BUTTON_ID));
    }

    private UiObject2 getVideoPlayerQualityButton() {
        UiObject2 videoPlayer = getVideoPlayer();
        UiObject2 qualityButton = null;

        if (videoPlayer != null) {
            qualityButton = mDevice.findObject(
                    By.res(UI_PACKAGE_NAME, UI_VIDEO_PLAYER_QUALITY_BUTTON_ID));
        }
        return qualityButton;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean waitForSearchResults(long timeout) {
        return mDevice.wait(Until.hasObject(
                By.res(UI_PACKAGE_NAME, UI_VIDEO_INFO_VIEW_ID)), timeout);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setVideoQuality(VideoQuality quality) {
        if (!isOnVideo()) {
            throw new IllegalStateException("YouTube is not on a video page.");
        }

        UiObject2 overflowButton = getVideoPlayerOverflowButton();
        // Open the mini video player
        if (overflowButton == null) {
            UiObject2 miniVideoPlayer = getVideoPlayer();
            if (miniVideoPlayer == null) {
                throw new UnknownUiException("Could not find mini video player.");
            }

            miniVideoPlayer.click();
            mDevice.wait(Until.findObject(By.res(
                UI_PACKAGE_NAME, UI_VIDEO_PLAYER_OVERFLOW_BUTTON_ID)), UI_NAVIGATION_WAIT);
            overflowButton = getVideoPlayerOverflowButton();
        }

        if (overflowButton == null) {
            throw new UnknownUiException("Could not find overflow button.");
        }

        overflowButton.click();
        UiObject2 qualityButton = mDevice.wait(Until.findObject(
                By.res(UI_PACKAGE_NAME, UI_VIDEO_PLAYER_QUALITY_BUTTON_ID)), UI_NAVIGATION_WAIT);
        if (qualityButton == null) {
            throw new UnknownUiException("Could not find video quality button.");
        }

        qualityButton.click();
        UiObject2 quality360pLabel = mDevice.wait(Until.findObject(By.text(
                AbstractYouTubeHelper.VideoQuality.QUALITY_360p.getText())), UI_NAVIGATION_WAIT);
        if (quality360pLabel == null) {
            throw new UnknownUiException("Could not find 360p quality label.");
        }

        UiObject2 selectDialog = quality360pLabel.getParent();
        if (selectDialog == null) {
            throw new UnknownUiException("Could not find video quality dialog.");
        }

        UiObject2 qualityLabel = null;
        for (int retriesRemaining = 5; retriesRemaining > 0; --retriesRemaining) {
            qualityLabel = mDevice.findObject(By.text(quality.getText()));
            if (qualityLabel != null) {
                break;
            }
            selectDialog.scroll(Direction.DOWN, 1.0f);
            mDevice.waitForIdle();
        }
        if (qualityLabel == null) {
            throw new UnknownUiException(
                    String.format("Could not find quality %s label", quality.getText()));
        }

        Log.v(TAG, String.format("Found quality %s label", quality.getText()));
        qualityLabel.click();
        if (!mDevice.wait(Until.hasObject(By.res(UI_PACKAGE_NAME, UI_VIDEO_PLAYER_ID)),
                UI_NAVIGATION_WAIT)) {
            throw new UnknownUiException("Did not find video player after selecting quality.");
        }
    }

    private UiObject2 getFullscreenButton() {
        return mDevice.findObject(By.desc(UI_FULLSCREEN_BUTTON_DESC));
    }

    private UiObject2 getPlayPauseReplayButton() {
        return mDevice.findObject(
            By.res(UI_PACKAGE_NAME, UI_VIDEO_PLAYER_PLAY_PAUSE_REPLAY_BUTTON_ID));
    }

    public void resumeVideo() {
        UiObject2 videoPlayer = getVideoPlayer();
        if (videoPlayer == null) {
            throw new UnknownUiException("Could not find video player.");
        }

        videoPlayer.click();
        UiObject2 playPauseReplayButton = mDevice.wait(Until.findObject(By.res(UI_PACKAGE_NAME,
                UI_VIDEO_PLAYER_PLAY_PAUSE_REPLAY_BUTTON_ID)), UI_NAVIGATION_WAIT);
        if (playPauseReplayButton == null) {
            throw new UnknownUiException("Could not find the pause/play button.");
        }

        if (UI_PLAY_VIDEO_DESC.equals(playPauseReplayButton.getContentDescription())) {
            playPauseReplayButton.click();
        }
    }

    private boolean hasConnectionEstablishedMessage() {
        Pattern establishedMsg =
                Pattern.compile("Connection established", Pattern.CASE_INSENSITIVE);
        return mDevice.hasObject(By.res(UI_PACKAGE_NAME, "message").text(establishedMsg));
    }

    private void pressGoOnline() {
        Pattern goOnlineMsg = Pattern.compile("Go online", Pattern.CASE_INSENSITIVE);
        UiObject2 button = mDevice.findObject(By.res(UI_PACKAGE_NAME, "action").text(goOnlineMsg));
        if (button != null) {
            button.click();
            mDevice.waitForIdle();
        } else {
            throw new UnknownUiException("Unable to find GO ONLINE button.");
        }
    }
}
