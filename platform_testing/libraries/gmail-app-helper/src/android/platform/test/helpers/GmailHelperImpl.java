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
import android.content.res.Configuration;
import android.os.SystemClock;
import android.platform.test.helpers.exceptions.UnknownUiException;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.Until;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.util.Log;
import android.webkit.WebView;
import android.widget.ListView;
import android.widget.ImageButton;

import java.io.IOException;
import java.util.List;
import java.util.regex.Pattern;

public class GmailHelperImpl extends AbstractGmailHelper {
    private static final String LOG_TAG = GmailHelperImpl.class.getSimpleName();

    private static final long APP_INIT_WAIT = 10000;
    private static final long DIALOG_TIMEOUT = 5000;
    private static final long POPUP_TIMEOUT = 7500;
    private static final long COMPOSE_TIMEOUT = 10000;
    private static final long SEND_TIMEOUT = 10000;
    private static final long LOADING_TIMEOUT = 25000;
    private static final long LOAD_EMAIL_TIMEOUT = 20000;
    private static final long WIFI_TIMEOUT = 60 * 1000;
    private static final long RELOAD_INBOX_TIMEOUT = 10 * 1000;
    private static final long COMPOSE_EMAIL_TIMEOUT = 10 * 1000;

    private static final String UI_ATTACHMENT_TILE_SAVE_ID = "attachment_tile_save";
    private static final String UI_NAME_ID = "name";
    private static final String UI_PACKAGE_NAME = "com.google.android.gm";
    private static final String UI_PROMO_ACTION_NEG_RES = "promo_action_negative_single_line";
    private static final String UI_CONVERSATIONS_LIST_ID = "conversation_list_view";
    private static final String UI_CONVERSATION_LIST_LOADING_VIEW_ID =
            "conversation_list_loading_view";
    private static final String UI_CONVERSATION_PAGER = "conversation_pager";
    private static final String UI_MULTI_PANE_CONTAINER_ID = "two_pane_activity";
    private static final BySelector PRIMARY_SELECTOR =
            By.res(UI_PACKAGE_NAME, "name").text("Primary");
    private static final BySelector INBOX_SELECTOR =
            By.res(UI_PACKAGE_NAME, "name").text("Inbox");
    private static final BySelector NAV_DRAWER_SELECTOR = By.res("android", "list").focused(true);

    public GmailHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getPackage() {
        return "com.google.android.gm";
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public String getLauncherName() {
        return "Gmail";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        // Check for the first, option dialog dismissal screen
        if (mDevice.wait(Until.hasObject(By.res(UI_PACKAGE_NAME, "welcome_tour_pager")),
                APP_INIT_WAIT)) {
            // Dismiss "New in Gmail" with GOT IT button or "Wecome to Gmail" with > button
            BySelector gotItSelector = By.res(UI_PACKAGE_NAME, "welcome_tour_got_it");
            BySelector skipSelector = By.res(UI_PACKAGE_NAME, "welcome_tour_skip");
            if (mDevice.hasObject(gotItSelector)) {
                mDevice.findObject(gotItSelector).clickAndWait(Until.newWindow(), DIALOG_TIMEOUT);
            } else if (mDevice.hasObject(skipSelector)) {
                mDevice.findObject(skipSelector).clickAndWait(Until.newWindow(), DIALOG_TIMEOUT);
            }
        } else {
            Log.e(LOG_TAG, "Unable to find initial screen. Continuing anyway.");
        }
        // Dismiss "Add another email address" with TAKE ME TO GMAIL button
        UiObject2 tutorialDone = mDevice.wait(Until.findObject(
                By.res(UI_PACKAGE_NAME, "action_done")), DIALOG_TIMEOUT);
        if (tutorialDone != null) {
            tutorialDone.clickAndWait(Until.newWindow(), DIALOG_TIMEOUT);
        }
        // Dismiss dogfood confidentiality dialog with OK, GOT IT button
        Pattern gotItWord = Pattern.compile("OK, GOT IT", Pattern.CASE_INSENSITIVE);
        UiObject2 splash = mDevice.wait(Until.findObject(By.text(gotItWord)), DIALOG_TIMEOUT);
        if (splash != null) {
            splash.clickAndWait(Until.newWindow(), DIALOG_TIMEOUT);
        }
        // Wait for "Getting your messages" to disappear
        if (mDevice.findObject(By.textContains("Getting your messages")) != null) {
            if (!mDevice.wait(Until.gone(By.text("Getting your messages")), WIFI_TIMEOUT)) {
                throw new UnknownUiException(
                        "Timed out waiting for 'Getting your messages' to disappear");
            }
        }
        if (!mDevice.wait(Until.hasObject(
                By.res(UI_PACKAGE_NAME, UI_CONVERSATIONS_LIST_ID)), WIFI_TIMEOUT)) {
            throw new UnknownUiException("Timed out waiting for conversation list to appear");
        }
        // Dismiss "Tap a sender image" dialog
        UiObject2 senderImageDismissButton =
                mDevice.findObject(By.res(UI_PACKAGE_NAME, "dismiss_icon"));
        if (senderImageDismissButton != null) {
            senderImageDismissButton.click();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToInbox() {
        // Check if already in Inbox or Primary
        if (isInPrimaryOrInbox()) {
            return;
        }

        if (isMultiPaneActivity()) {
            // Select for the closed Primary icon
            UiObject2 primaryClosed = mDevice.findObject(
                    By.res(UI_PACKAGE_NAME, "image_view").text("Primary"));
            if (primaryClosed != null) {
                primaryClosed.click();
                mDevice.waitForIdle();
                return;
            }

            // Select for the closed Inbox icon
            UiObject2 inboxClosed = mDevice.findObject(
                    By.res(UI_PACKAGE_NAME, "image_view").text("Inbox"));
            if (inboxClosed != null) {
                inboxClosed.click();
                mDevice.waitForIdle();
                return;
            }

            scrollNavigationDrawer(Direction.UP);

            // Select for the open Primary icon
            UiObject2 primaryOpen = mDevice.findObject(
                    By.res(UI_PACKAGE_NAME, "name").text("Primary"));
            if (primaryOpen != null) {
                primaryOpen.click();
                mDevice.waitForIdle();
                return;
            }

            // Select for the open Inbox icon
            UiObject2 inboxOpen = mDevice.findObject(
                    By.res(UI_PACKAGE_NAME, "name").text("Inbox"));
            if (inboxOpen != null) {
                inboxOpen.click();
                mDevice.waitForIdle();
                return;
            }

            // Currently unhandled case; throw Exception.
            throw new RuntimeException("Unable to find method to get to Primary/Inbox");
        } else {
            // Simply press back if in a conversation
            if (isInConversation()) {
                mDevice.pressBack();
                waitForConversationsList();
            }

            // If in another e-mail sub-folder, go to Primary or Inbox
            if (!isInPrimaryOrInbox()) {
                // Search with the navigation drawer
                openNavigationDrawer();

                // Select for "Primary" and for "Inbox"
                UiObject2 primaryInboxSelector = mDevice.findObject(PRIMARY_SELECTOR);
                if (primaryInboxSelector == null) {
                    primaryInboxSelector = mDevice.findObject(INBOX_SELECTOR);
                }

                primaryInboxSelector.click();
                waitForConversationsList();
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void goToComposeEmail() {
        if (!isInPrimaryOrInbox()) {
            throw new IllegalStateException("Gmail is not on the Inbox or Primary page");
        }
        UiObject2 compose = mDevice.findObject(By.desc("Compose"));
        if (compose == null) {
            throw new UnknownUiException("Compose button not found");
        }
        compose.clickAndWait(Until.newWindow(), COMPOSE_TIMEOUT);
        waitForCompose();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void openEmailByIndex(int index) {
        if (!isInMailbox()) {
            throw new IllegalStateException("Must be in a mailbox to open an email by index");
        }

        if (index >= getVisibleEmailCount()) {
            throw new IllegalArgumentException(String.format("Cannot select %s'th message of %s",
                    (index + 1), getVisibleEmailCount()));
        }

        // Select an e-mail by index
        UiObject2 conversationList = getConversationList();
        List<UiObject2> emails = conversationList.findObjects(
                By.clazz(android.widget.FrameLayout.class));
        if (conversationList == null) {
            throw new UnknownUiException("No e-mails found.");
        }
        emails.get(index).click();

        // Wait until the e-mail is open
        UiObject2 loadMsg = mDevice.findObject(By.res(UI_PACKAGE_NAME, "loading_progress"));
        if (loadMsg != null) {
            if (!mDevice.wait(Until.gone(
                    By.res(UI_PACKAGE_NAME, "loading_progress")), LOADING_TIMEOUT)) {
                throw new RuntimeException("Loading message timed out after 20s");
            }
        }

        waitForConversation();
    }
    /**
     * {@inheritDoc}
     */
    @Override
    public int getVisibleEmailCount() {
        if (!isInMailbox()) {
            throw new IllegalStateException("Must be in a mailbox to open an email by index");
        }

        return getConversationList().getChildCount();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void sendReplyEmail(String address, String body) {
        if (!isInConversation()) {
            throw new IllegalStateException("Must have an e-mail open to send a reply.");
        }

        UiObject2 convScroll = getConversationPager();
        while(convScroll.scroll(Direction.DOWN, 1.0f));

        UiObject2 replyButton = mDevice.findObject(By.text("Reply"));
        if (replyButton != null) {
            replyButton.clickAndWait(Until.newWindow(), COMPOSE_TIMEOUT);
            waitForCompose();
        } else {
            throw new UnknownUiException("Failed to find a 'Reply' button.");
        }

        // Set the necessary fields (address and body)
        setEmailToAddress(address);
        setEmailBody(body);

        // Send the reply e-mail and wait for original e-mail
        clickSendButton();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setEmailToAddress(String address) {
        UiObject2 convScroll = getComposeScrollContainer();
        while (convScroll.scroll(Direction.UP, 1.0f));

        UiObject2 toField = getToField();
        for (int retries = 5; retries > 0 && toField == null; retries--) {
            convScroll.scroll(Direction.DOWN, 1.0f);
            toField = getToField();
        }

        if (toField != null) {
            toField.setText(address);
        } else {
            throw new UnknownUiException("Failed to find a 'To' field.");
        }
    }


    /**
     * {@inheritDoc}
     */
    @Override
    public void setEmailSubject(String subject) {
        UiObject2 convScroll = getComposeScrollContainer();
        while (convScroll.scroll(Direction.UP, 1.0f));

        UiObject2 subjectField = getSubjectField();
        for (int retries = 5; retries > 0 && subjectField == null; retries--) {
            convScroll.scroll(Direction.DOWN, 1.0f);
            subjectField = getSubjectField();
        }

        if (subjectField != null) {
            subjectField.setText(subject);
        } else {
            throw new UnknownUiException("Failed to find a 'Subject' field.");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setEmailBody(String body) {
        UiObject2 convScroll = getComposeScrollContainer();
        while (convScroll.scroll(Direction.UP, 1.0f));

        UiObject2 bodyField = getBodyField();
        for (int retries = 5; retries > 0 && bodyField == null; retries--) {
            convScroll.scroll(Direction.DOWN, 1.0f);
            bodyField = getBodyField();
        }

        if (bodyField != null) {
            // Ensure the focus is left in the body field.
            bodyField.click();
            bodyField.setText(body);
        } else {
            throw new UnknownUiException("Failed to find a 'Body' field.");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void clickSendButton() {
        UiObject2 convScroll = getComposeScrollContainer();
        while (convScroll.scroll(Direction.UP, 1.0f));

        UiObject2 sendButton = getSendButton();
        if (sendButton != null) {
            sendButton.clickAndWait(Until.newWindow(), SEND_TIMEOUT);
            waitForConversation();
        } else {
            throw new UnknownUiException("Failed to find a 'Send' button.");
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getComposeEmailBody(){
        UiObject2 bodyField = getBodyField();
        return bodyField.getText();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void openNavigationDrawer() {
        for (int retries = 3; retries > 0; retries--) {
            if (isNavDrawerOpen()) {
                return;
            }

            UiObject2 nav = mDevice.findObject(By.desc(Pattern.compile(
                    "(Open navigation drawer)|(Navigate up)")));

            if (nav == null) {
                throw new IllegalStateException("Could not find navigation drawer");
            }
            nav.click();
            mDevice.waitForIdle();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void scrollNavigationDrawer(Direction dir) {
        if (dir == Direction.RIGHT || dir == Direction.LEFT) {
            throw new IllegalArgumentException("Can only scroll navigation drawer up and down.");
        }

        UiObject2 scroll = getNavDrawerContainer();
        if (scroll == null) {
            throw new UnknownUiException("No navigation drawer found to scroll");
        }
        scroll.scroll(dir, 1.0f);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean closeNavigationDrawer() {
        UiObject2 navDrawer = mDevice.wait(Until.findObject(
                By.clazz(ImageButton.class).desc("Close navigation drawer")), 1000);
        if (navDrawer != null) {
            navDrawer.click();
            return true;
        }
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isInComposeEmail(){
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, "compose")) != null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isInPrimaryOrInbox() {
        if (isMultiPaneActivity()) {
            return (mDevice.hasObject(By.res(UI_PACKAGE_NAME, "actionbar_title").text("Primary")) ||
                    mDevice.hasObject(By.res(UI_PACKAGE_NAME, "actionbar_title").text("Inbox")));
        } else {
            return getConversationList() != null &&
                    (mDevice.hasObject(By.text("Primary")) ||
                            mDevice.hasObject(By.text("Inbox")));
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void scrollMailbox(Direction direction, float amount, boolean scrollToEnd) {
        if (!isInMailbox()) {
            throw new IllegalStateException("Not in mailbox");
        }

        if (!(Direction.UP.equals(direction) || Direction.DOWN.equals(direction))) {
            throw new IllegalArgumentException("Scroll direction must be UP or DOWN");
        }

        UiObject2 scrollContainer = getConversationList();
        if (scrollContainer == null) {
            throw new IllegalStateException("Could not find scroll container");
        }

        scroll(scrollContainer, direction, amount, scrollToEnd);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void scrollEmail(Direction direction, float amount, boolean scrollToEnd) {
        if (!(Direction.UP.equals(direction) || Direction.DOWN.equals(direction))) {
            throw new IllegalArgumentException("Scroll direction must be UP or DOWN");
        }

        UiObject2 scrollContainer = getConversationPager();
        if (scrollContainer == null) {
            throw new IllegalStateException("Could not find email scroll container");
        }

        scroll(scrollContainer, direction, amount, scrollToEnd);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void openMailbox(String mailboxName) {
        if (!isNavDrawerOpen()) {
            throw new IllegalStateException("Navigation drawer is not open");
        }

        UiObject2 mailbox = null;
        for (int scrollsRemaining = 5; scrollsRemaining > 0; --scrollsRemaining) {
            mailbox = mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_NAME_ID).text(
                    Pattern.compile(mailboxName, Pattern.CASE_INSENSITIVE)));
            if (mailbox != null) {
                break;
            } else {
                scrollNavigationDrawer(Direction.DOWN);
            }
        }
        if (mailbox == null) {
            throw new IllegalArgumentException(
                    String.format("Could not find mailbox '%s'", mailboxName));
        }
        mailbox.click();
        mDevice.waitForIdle();
        mDevice.wait(Until.gone(
                By.res(UI_PACKAGE_NAME, UI_CONVERSATION_LIST_LOADING_VIEW_ID)), WIFI_TIMEOUT);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void returnToMailbox() {
        for (int retriesRemaining = 5; retriesRemaining > 0; --retriesRemaining) {
            if (isInMailbox()) {
                break;
            } else {
                mDevice.pressBack();
                mDevice.waitForIdle();
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void downloadAttachment(int index) {
        if (!isInConversation()) {
            throw new IllegalStateException("Email is not open");
        }

        List<UiObject2> downloadButtons =
                mDevice.findObjects(By.res(UI_PACKAGE_NAME, UI_ATTACHMENT_TILE_SAVE_ID));
        if (downloadButtons != null && index >= 0 && index < downloadButtons.size()) {
            downloadButtons.get(index).click();
        } else {
            throw new IndexOutOfBoundsException("attachment index out of bounds");
        }
    }

    private UiObject2 getToField() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, "to"));
    }

    private UiObject2 getSubjectField() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, "subject"));
    }

    private UiObject2 getBodyField() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, "body"));
    }

    private UiObject2 getSendButton() {
        return mDevice.findObject(By.desc("Send"));
    }

    private UiObject2 getComposeScrollContainer() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, "compose"));
    }

    private UiObject2 getNavDrawerContainer() {
        return mDevice.findObject(NAV_DRAWER_SELECTOR);
    }

    private UiObject2 getConversationList() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_CONVERSATIONS_LIST_ID));
    }

    private UiObject2 getConversationPager() {
        return mDevice.findObject(By.res(UI_PACKAGE_NAME, UI_CONVERSATION_PAGER));
    }

    private boolean isInConversation() {
        return mDevice.hasObject(By.res(UI_PACKAGE_NAME, UI_CONVERSATION_PAGER));
    }

    private boolean isNavDrawerOpen() {
        if (isMultiPaneActivity()) {
            return mDevice.hasObject(By.res("android", "list"));
        } else {
            return mDevice.hasObject(NAV_DRAWER_SELECTOR);
        }
    }

    private void waitForConversationsList() {
        mDevice.wait(Until.hasObject(
                By.res(UI_PACKAGE_NAME, UI_CONVERSATIONS_LIST_ID)), RELOAD_INBOX_TIMEOUT);
    }

    private void waitForConversation() {
        mDevice.wait(Until.hasObject(
                By.res(UI_PACKAGE_NAME, UI_CONVERSATION_PAGER)), LOAD_EMAIL_TIMEOUT);
    }

    private void waitForCompose() {
        mDevice.wait(Until.findObject(By.res(UI_PACKAGE_NAME, "compose")), COMPOSE_EMAIL_TIMEOUT);
    }

    private boolean isMultiPaneActivity() {
        return mDevice.hasObject(By.res(UI_PACKAGE_NAME, UI_MULTI_PANE_CONTAINER_ID));
    }

    private boolean isInMailbox() {
        if (isMultiPaneActivity()) {
            return mDevice.hasObject(By.desc("Search"));
        } else {
            return mDevice.hasObject(By.desc("Search")) && getNavDrawerContainer() == null;
        }
    }

    private void scroll(UiObject2 scrollContainer, Direction direction,
            float amount, boolean scrollToEnd) {
        if (amount < 0.0f) {
            throw new IllegalArgumentException("Scroll amount cannot be negative");
        }

        if (scrollToEnd) {
            while (scrollContainer.scroll(direction, 1.0f)) {
                // empty
            }
        } else {
            scrollContainer.scroll(direction, (float) amount);
        }
    }
}
