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
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.Until;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.util.Log;

import junit.framework.Assert;

/**
 * UI test helper for Reddit: The Official App (package: com.reddit.frontpage)
 */

public class RedditHelperImpl extends AbstractRedditHelper {
    private static final String TAG = RedditHelperImpl.class.getSimpleName();

    private static final String UI_COMMENTS_PAGE_SCROLL_CONTAINER_ID = "detail_list";
    private static final String UI_FRONT_PAGE_SCROLL_CONTAINER_ID = "link_list";
    private static final String UI_LINK_TITLE_ID = "link_title";
    private static final String UI_PACKAGE_NAME = "com.reddit.frontpage";
    private static final String UI_REDDIT_WORDMARK_ID = "reddit_wordmark";
    private static final String UI_SAVE_BUTTON_ID = "action_save";

    private static final long UI_NAVIGATION_WAIT = 5000; // 5 secs

    public RedditHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        return UI_PACKAGE_NAME;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        return "Reddit";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {

    }

    private UiObject2 getRedditWordmark() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_REDDIT_WORDMARK_ID));
    }

    private UiObject2 getFrontPageScrollContainer() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_FRONT_PAGE_SCROLL_CONTAINER_ID));
    }

    private UiObject2 getFirstArticleTitle() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_LINK_TITLE_ID));
    }

    private UiObject2 getSaveButton() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_SAVE_BUTTON_ID));
    }

    private UiObject2 getCommentPageScrollContainer() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_COMMENTS_PAGE_SCROLL_CONTAINER_ID));
    }

    private boolean isOnFrontPage() {
        return (getRedditWordmark() != null);
    }

    private boolean isOnCommentsPage() {
        return (getSaveButton() != null);
    }

    public void goToFrontPage() {
        for (int retriesRemaining = 5; retriesRemaining > 0 && !isOnFrontPage();
                --retriesRemaining) {
            mDevice.pressBack();
            mDevice.waitForIdle();
        }
    }

    public void goToFirstArticleComments() {
        Assert.assertTrue("Not on front page", isOnFrontPage());

        UiObject2 articleTitle = getFirstArticleTitle();
        Assert.assertNotNull("Could not find first article", articleTitle);

        articleTitle.click();
        mDevice.wait(Until.hasObject(By.res(UI_PACKAGE_NAME, UI_SAVE_BUTTON_ID)),
                UI_NAVIGATION_WAIT);
    }

    public boolean scrollFrontPage(Direction direction, float percent) {
        Assert.assertTrue("Not on front page", isOnFrontPage());
        Assert.assertTrue("Scroll direction must be UP or DOWN",
                Direction.UP.equals(direction) || Direction.DOWN.equals(direction));

        UiObject2 scrollContainer = getFrontPageScrollContainer();
        Assert.assertNotNull("Could not find front page scroll container", scrollContainer);

        return scrollContainer.scroll(direction, percent);
    }

    public boolean scrollCommentPage(Direction direction, float percent) {
        Assert.assertTrue("Not on comment page", isOnCommentsPage());
        Assert.assertTrue("Scroll direction must be UP or DOWN",
                Direction.UP.equals(direction) || Direction.DOWN.equals(direction));

        UiObject2 scrollContainer = getCommentPageScrollContainer();
        Assert.assertNotNull("Could not find comment page scroll container", scrollContainer);

        return scrollContainer.scroll(direction, percent);
    }
}
