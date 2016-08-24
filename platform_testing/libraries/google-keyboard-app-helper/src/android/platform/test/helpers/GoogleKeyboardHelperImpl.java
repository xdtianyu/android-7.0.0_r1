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
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Until;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject2;
import android.util.Log;

import junit.framework.Assert;

import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.regex.Pattern;
import java.util.Set;

public class GoogleKeyboardHelperImpl extends AbstractGoogleKeyboardHelper {
    private static final String TAG = GoogleKeyboardHelperImpl.class.getCanonicalName();

    private static final Map<Character, String> SPECIAL_KEY_CONTENT_DESCRIPTIONS;

    private static final Set<Character> ALWAYS_VISIBLE_CHARACTERS;
    private static final Set<Character> KEYBOARD_NUMBER_SCREEN_SYMBOLS;
    private static final Set<Character> KEYBOARD_OTHER_SYMBOLS;

    private static final String UI_ANDROID_VIEW_CLASS = "android.view.View";
    private static final String UI_DECLINE_BUTTON_ID = "decline_button";
    private static final String UI_KEYBOARD_KEY_CLASS = "com.android.inputmethod.keyboard.Key";
    private static final String UI_KEYBOARD_LETTER_KEY_DESC = "Letters";
    private static final String UI_KEYBOARD_NUMBER_KEY_DESC = "Symbols";
    private static final String UI_KEYBOARD_SHIFT_KEY_DESC = "Shift";
    private static final String UI_KEYBOARD_SYMBOL_KEY_DESC = "More symbols";
    private static final String UI_KEYBOARD_VIEW_ID = "keyboard_view";
    private static final String UI_RESOURCE_NAME = "com.android.inputmethod.latin";
    private static final String UI_PACKAGE_NAME = "com.google.android.inputmethod.latin";
    private static final String UI_QUICK_SEARCH_BOX_PACKAGE_NAME =
            "com.google.android.googlequicksearchbox";

    private static final char KEYBOARD_TEST_LOWER_CASE_LETTER = 'a';
    private static final char KEYBOARD_TEST_NUMBER = '1';
    private static final char KEYBOARD_TEST_SYMBOL = '~';
    private static final char KEYBOARD_TEST_UPPER_CASE_LETTER = 'A';

    private static final long KEYBOARD_MODE_CHANGE_TIMEOUT = 5000; // 5 secs

    static {
        Map<Character, String> specialKeyContentDescriptions = new HashMap<>();
        specialKeyContentDescriptions.put(' ', "Space");
        specialKeyContentDescriptions.put('I', "Capital I");
        specialKeyContentDescriptions.put('Δ', "Increment");
        specialKeyContentDescriptions.put('©', "Copyright sign");
        specialKeyContentDescriptions.put('®', "Registered sign");
        specialKeyContentDescriptions.put('™', "Trade mark sign");
        specialKeyContentDescriptions.put('℅', "Care of");
        SPECIAL_KEY_CONTENT_DESCRIPTIONS =
                Collections.unmodifiableMap(specialKeyContentDescriptions);

        String alwaysVisibleCharacters = ".,";
        ALWAYS_VISIBLE_CHARACTERS = createImmutableSet(alwaysVisibleCharacters);

        String keyboardNumberScreenSymbols = "@#$%&-+()*\"':;!?_/";
        KEYBOARD_NUMBER_SCREEN_SYMBOLS = createImmutableSet(keyboardNumberScreenSymbols);

        String keyboardOtherSymbols = "~`|•√π÷×¶Δ£¢€¥^°={}\\©®™℅[]<>";
        KEYBOARD_OTHER_SYMBOLS = createImmutableSet(keyboardOtherSymbols);
    }

    public GoogleKeyboardHelperImpl(Instrumentation instr) {
        super(instr);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void open() {
        Log.w(TAG, "No method defined to open Google Keyboard. (no-op)");
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
        return "Google Keyboard";
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void dismissInitialDialogs() {
        mDevice.pressHome();
        UiObject2 searchPlate = mDevice.findObject(
                By.res(UI_QUICK_SEARCH_BOX_PACKAGE_NAME, "search_plate"));
        if (searchPlate != null) {
            searchPlate.click();
            UiObject2 skipGoogleNowButton = mDevice.wait(Until.findObject(
                    By.res(UI_QUICK_SEARCH_BOX_PACKAGE_NAME, UI_DECLINE_BUTTON_ID)), 20000);
            if (skipGoogleNowButton != null) {
                skipGoogleNowButton.click();
            }
            BySelector closeSelector = By.text(Pattern.compile("CLOSE", Pattern.CASE_INSENSITIVE));
            Assert.assertTrue("Could not find close button to dismiss Google Keyboard dialog",
                    mDevice.wait(Until.hasObject(closeSelector), 5000));
            mDevice.findObject(closeSelector).click();
            mDevice.wait(Until.gone(closeSelector), 5000);
        }

    }

    private static Set<Character> createImmutableSet(String setCharacters) {
        Assert.assertNotNull("setCharacters cannot be null", setCharacters);

        Set<Character> tempSet = new HashSet<>();
        for (int i = 0; i < setCharacters.length(); ++i) {
            tempSet.add(setCharacters.charAt(i));
        }
        return Collections.unmodifiableSet(tempSet);
    }

    private UiObject2 getKeyboardView() {
        return mDevice.findObject(By.clazz(UI_ANDROID_VIEW_CLASS).res(
                UI_RESOURCE_NAME, UI_KEYBOARD_VIEW_ID));
    }

    private UiObject2 getShiftKey() {
        return mDevice.findObject(
                By.clazz(UI_KEYBOARD_KEY_CLASS).desc(UI_KEYBOARD_SHIFT_KEY_DESC));
    }

    private UiObject2 getNumberKey() {
        return mDevice.findObject(
                By.clazz(UI_KEYBOARD_KEY_CLASS).desc(UI_KEYBOARD_NUMBER_KEY_DESC));
    }

    private UiObject2 getLetterKey() {
        return mDevice.findObject(
                By.clazz(UI_KEYBOARD_KEY_CLASS).desc(UI_KEYBOARD_LETTER_KEY_DESC));
    }

    private UiObject2 getSymbolKey() {
        return mDevice.findObject(
                By.clazz(UI_KEYBOARD_KEY_CLASS).desc(UI_KEYBOARD_SYMBOL_KEY_DESC));
    }

    private String getKeyDesc(char key) {
        String specialKeyDesc = SPECIAL_KEY_CONTENT_DESCRIPTIONS.get(key);
        if (specialKeyDesc != null) {
            return specialKeyDesc;
        } else {
            return String.valueOf(key);
        }
    }

    private UiObject2 getKeyboardKey(char key) {
        String keyDesc = getKeyDesc(key);

        return mDevice.findObject(By.clazz(UI_KEYBOARD_KEY_CLASS).desc(keyDesc));
    }

    private boolean isLowerCaseLetter(char c) {
        return (c >= 'a' && c <= 'z');
    }

    private boolean isUpperCaseLetter(char c) {
        return (c >= 'A' && c <= 'Z');
    }

    private boolean isDigit(char c) {
        return (c >= '0' && c <= '9');
    }

    private boolean isKeyboardOpen() {
        return (getKeyboardView() != null);
    }

    private boolean isOnLowerCaseMode() {
        return (getKeyboardKey(KEYBOARD_TEST_LOWER_CASE_LETTER) != null);
    }

    private boolean isOnUpperCaseMode() {
        return (getKeyboardKey(KEYBOARD_TEST_UPPER_CASE_LETTER) != null);
    }

    private boolean isOnNumberMode() {
        return (getKeyboardKey(KEYBOARD_TEST_NUMBER) != null);
    }

    private boolean isOnSymbolMode() {
        return (getKeyboardKey(KEYBOARD_TEST_SYMBOL) != null);
    }

    private void toggleShiftMode() {
        UiObject2 shiftKey = getShiftKey();
        Assert.assertNotNull("Could not find Shift key", shiftKey);

        shiftKey.click();
    }

    private void switchToLetterMode() {
        UiObject2 letterKey = getLetterKey();
        Assert.assertNotNull("Could not find Letter key", letterKey);

        letterKey.click();
    }

    private void switchToLowerCaseMode() {
        if (isOnNumberMode() || isOnSymbolMode()) {
            switchToLetterMode();
        }

        if (isOnUpperCaseMode()) {
            toggleShiftMode();
        }

        Assert.assertTrue("Could not switch to lower case letters mode on Google Keyboard",
                mDevice.wait(Until.hasObject(By.clazz(UI_KEYBOARD_KEY_CLASS).desc(
                String.valueOf(KEYBOARD_TEST_LOWER_CASE_LETTER))), KEYBOARD_MODE_CHANGE_TIMEOUT));
    }

    private void switchToUpperCaseMode() {
        if (isOnNumberMode() || isOnSymbolMode()) {
            switchToLetterMode();
        }

        if (isOnLowerCaseMode()) {
            toggleShiftMode();
        }

        Assert.assertTrue("Could not switch to upper case letters mode on Google Keyboard",
                mDevice.wait(Until.hasObject(By.clazz(UI_KEYBOARD_KEY_CLASS).desc(
                String.valueOf(KEYBOARD_TEST_UPPER_CASE_LETTER))), KEYBOARD_MODE_CHANGE_TIMEOUT));
    }

    private void switchToNumberMode() {
        if (!isOnNumberMode()) {
            UiObject2 numberKey = getNumberKey();
            Assert.assertNotNull("Could not find Number key", numberKey);

            numberKey.click();
        }

        Assert.assertTrue("Could not switch to number mode on Google Keyboard",
                mDevice.wait(Until.hasObject(By.clazz(UI_KEYBOARD_KEY_CLASS).desc(
                String.valueOf(KEYBOARD_TEST_NUMBER))), KEYBOARD_MODE_CHANGE_TIMEOUT));
    }

    private void switchToSymbolMode() {
        if (isOnLowerCaseMode() || isOnUpperCaseMode()) {
            switchToNumberMode();
        }

        if (isOnNumberMode()) {
            UiObject2 symbolKey = getSymbolKey();
            Assert.assertNotNull("Could not find Symbol key", symbolKey);

            symbolKey.click();
        }

        Assert.assertTrue("Could not switch to symbol mode on Google Keyboard",
                mDevice.wait(Until.hasObject(By.clazz(UI_KEYBOARD_KEY_CLASS).desc(
                String.valueOf(KEYBOARD_TEST_SYMBOL))), KEYBOARD_MODE_CHANGE_TIMEOUT));
    }

    /*
     * {@inheritDoc}
     */
    @Override
    public boolean waitForKeyboard(long timeout) {
        return mDevice.wait(Until.hasObject(By.clazz(UI_ANDROID_VIEW_CLASS).res(
                UI_RESOURCE_NAME, UI_KEYBOARD_VIEW_ID)), timeout);
    }

    /*
     * {@inheritDoc}
     */
    @Override
    public void typeText(String text, long delayBetweenKeyPresses) {
        Assert.assertTrue("Google Keyboard is not open", isKeyboardOpen());
        for (int i = 0; i < text.length(); ++i) {
            char c = text.charAt(i);

            if (ALWAYS_VISIBLE_CHARACTERS.contains(c)) {
                // Period and comma are visible on all keyboard modes so no need to switch modes
            } else if (isLowerCaseLetter(c)) {
                switchToLowerCaseMode();
            } else if (isUpperCaseLetter(c)) {
                switchToUpperCaseMode();
            } else if (isDigit(c) ||
                    KEYBOARD_NUMBER_SCREEN_SYMBOLS.contains(c)) {
                switchToNumberMode();
            } else if (KEYBOARD_OTHER_SYMBOLS.contains(c)) {
                switchToSymbolMode();
            } else {
                Assert.fail(String.format("Unrecognized character '%c'", c));
            }
            UiObject2 keyboardKey = getKeyboardKey(c);
            Assert.assertNotNull(String.format("Could not find key '%c' on Google Keyboard", c),
                    keyboardKey);

            keyboardKey.click();
            SystemClock.sleep(delayBetweenKeyPresses);
        }
    }
}
