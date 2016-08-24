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

package android.platform.test.helpers;

import android.app.Instrumentation;
import android.platform.test.helpers.exceptions.UnknownUiException;
import android.support.test.launcherhelper.ILeanbackLauncherStrategy;
import android.support.test.launcherhelper.LauncherStrategyFactory;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.BySelector;
import android.support.test.uiautomator.Direction;
import android.support.test.uiautomator.UiObject2;
import android.support.test.uiautomator.Until;
import android.util.Log;

/**
 *  This app helper handles the following important widgets for TV apps:
 *  BrowseFragment, DetailsFragment, SearchFragment and PlaybackOverlayFragment
 */
public abstract class AbstractLeanbackAppHelper extends AbstractStandardAppHelper {

    private static final String TAG = AbstractLeanbackAppHelper.class.getSimpleName();
    private static final long OPEN_SECTION_WAIT_TIME_MS = 5000;
    private static final long OPEN_SIDE_PANEL_WAIT_TIME_MS = 5000;
    private static final int OPEN_SIDE_PANEL_MAX_ATTEMPTS = 5;
    private static final long MAIN_ACTIVITY_WAIT_TIME_MS = 250;

    protected DPadHelper mDPadHelper;
    public ILeanbackLauncherStrategy mLauncherStrategy;


    public AbstractLeanbackAppHelper(Instrumentation instr) {
        super(instr);
        mDPadHelper = DPadHelper.getInstance(instr);
        mLauncherStrategy = LauncherStrategyFactory.getInstance(
                mDevice).getLeanbackLauncherStrategy();
    }

    protected abstract BySelector getAppSelector();

    protected abstract BySelector getSidePanelSelector();

    protected abstract BySelector getSidePanelResultSelector(String sectionName);

    /**
     * Selector to identify main activity for getMainActivitySelector().
     * Not every application has its main activity, so the override is optional.
     */
    protected BySelector getMainActivitySelector() {
        return null;
    }

    /**
     * Setup expectation: Side panel is selected on browse fragment
     *
     * Best effort attempt to go to the side panel, and open the selected section.
     */
    public void openSection(String sectionName) {
        openSidePanel();
        // Section header is focused; it should not be after pressing the DPad
        selectSection(sectionName);
        mDevice.pressDPadCenter();

        // Test for focus change and selection result
        BySelector sectionResult = getSidePanelResultSelector(sectionName);
        if (!mDevice.wait(Until.hasObject(sectionResult), OPEN_SECTION_WAIT_TIME_MS)) {
            throw new UnknownUiException(
                    String.format("Failed to find result opening section %s", sectionName));
        }
        Log.v(TAG, "Successfully opened section");
    }

    /**
     * Setup expectation: On navigation screen on browse fragment
     *
     * Best effort attempt to open the side panel.
     * @param onMainActivity True if it opens the side panel on app's main activity.
     */
    public void openSidePanel(boolean onMainActivity) {
        if (onMainActivity) {
            returnToMainActivity();
        }
        int attempts = 0;
        while (!isSidePanelSelected(OPEN_SIDE_PANEL_WAIT_TIME_MS)
                && attempts++ < OPEN_SIDE_PANEL_MAX_ATTEMPTS) {
            mDevice.pressDPadLeft();
        }
        if (attempts == OPEN_SIDE_PANEL_MAX_ATTEMPTS) {
            throw new UnknownUiException("Failed to open side panel");
        }
    }

    public void openSidePanel() {
        openSidePanel(false);
    }

    /**
     * Select target item through the container in the given direction.
     * @param container
     * @param target
     * @param direction
     * @return the focused object
     */
    public UiObject2 select(UiObject2 container, BySelector target, Direction direction) {
        if (container == null) {
            throw new IllegalArgumentException("The container should not be null.");
        }
        UiObject2 focus = container.findObject(By.focused(true));
        if (focus == null) {
            throw new UnknownUiException("The container should have a focus.");
        }
        while (!focus.hasObject(target)) {
            UiObject2 prev = focus;
            mDPadHelper.pressDPad(direction);
            focus = container.findObject(By.focused(true));
            if (focus == null) {
                mDPadHelper.pressDPad(Direction.reverse(direction));
                focus = container.findObject(By.focused(true));
            }
            if (focus.equals(prev)) {
                // It reached at the end, but no target is found.
                return null;
            }
        }
        return focus;
    }

    /**
     * Attempts to return to main activity with getMainActivitySelector()
     * by pressing the back button repeatedly and sleeping briefly to allow for UI slowness.
     */
    public void returnToMainActivity() {
        int maxBackAttempts = 10;
        BySelector selector = getMainActivitySelector();
        if (selector == null) {
            throw new IllegalStateException("getMainActivitySelector() should be overridden.");
        }
        while (!mDevice.wait(Until.hasObject(selector), MAIN_ACTIVITY_WAIT_TIME_MS)
                && maxBackAttempts-- > 0) {
            mDevice.pressBack();
        }
    }

    @Override
    public void dismissInitialDialogs() {
        return;
    }

    protected boolean isSidePanelSelected(long timeout) {
        UiObject2 sidePanel = mDevice.wait(Until.findObject(getSidePanelSelector()), timeout);
        if (sidePanel == null) {
            return false;
        }
        return sidePanel.hasObject(By.focused(true).minDepth(1));
    }

    protected UiObject2 selectSection(String sectionName) {
        UiObject2 container = mDevice.wait(
                Until.findObject(getSidePanelSelector()), OPEN_SIDE_PANEL_WAIT_TIME_MS);
        BySelector section = By.clazz(".TextView").text(sectionName);

        // Wait until the section text appears at runtime. This needs to be long enough to run under
        // low bandwidth environments in the test lab.
        mDevice.wait(Until.findObject(section), 60 * 1000);

        // Search up, then down
        UiObject2 focused = select(container, section, Direction.UP);
        if (focused != null) {
            return focused;
        }
        focused = select(container, section, Direction.DOWN);
        if (focused != null) {
            return focused;
        }
        throw new UnknownUiException("Failed to select section");
    }

}
