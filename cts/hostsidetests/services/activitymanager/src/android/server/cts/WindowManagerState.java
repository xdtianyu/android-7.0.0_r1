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

package android.server.cts;

import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;

import java.awt.Rectangle;
import java.lang.String;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;

import java.util.regex.Pattern;
import java.util.regex.Matcher;

import static android.server.cts.StateLogger.log;
import static android.server.cts.StateLogger.logE;

class WindowManagerState {
    private static final String DUMPSYS_WINDOWS_APPS = "dumpsys window apps";
    private static final String DUMPSYS_WINDOWS_VISIBLE_APPS = "dumpsys window visible-apps";

    private static final Pattern sWindowPattern =
            Pattern.compile("Window #(\\d+) Window\\{([0-9a-fA-F]+) u(\\d+) (.+)\\}\\:");
    private static final Pattern sStartingWindowPattern =
            Pattern.compile("Window #(\\d+) Window\\{([0-9a-fA-F]+) u(\\d+) Starting (.+)\\}\\:");
    private static final Pattern sExitingWindowPattern =
            Pattern.compile("Window #(\\d+) Window\\{([0-9a-fA-F]+) u(\\d+) (.+) EXITING\\}\\:");

    private static final Pattern sFocusedWindowPattern = Pattern.compile(
            "mCurrentFocus=Window\\{([0-9a-fA-F]+) u(\\d+) (\\S+)\\}");
    private static final Pattern sAppErrorFocusedWindowPattern = Pattern.compile(
            "mCurrentFocus=Window\\{([0-9a-fA-F]+) u(\\d+) Application Error\\: (\\S+)\\}");

    private static final Pattern sFocusedAppPattern =
            Pattern.compile("mFocusedApp=AppWindowToken\\{(.+) token=Token\\{(.+) "
                    + "ActivityRecord\\{(.+) u(\\d+) (\\S+) (\\S+)");

    private static final Pattern sStackIdPattern = Pattern.compile("mStackId=(\\d+)");

    private static final Pattern[] sExtractStackExitPatterns = {
            sStackIdPattern, sWindowPattern, sStartingWindowPattern, sExitingWindowPattern,
            sFocusedWindowPattern, sAppErrorFocusedWindowPattern, sFocusedAppPattern };

    // Windows in z-order with the top most at the front of the list.
    private List<String> mWindows = new ArrayList();
    private List<WindowState> mWindowStates = new ArrayList();
    private List<WindowStack> mStacks = new ArrayList();
    private List<Display> mDisplays = new ArrayList();
    private String mFocusedWindow = null;
    private String mFocusedApp = null;
    private final LinkedList<String> mSysDump = new LinkedList();

    void computeState(ITestDevice device, boolean visibleOnly) throws DeviceNotAvailableException {
        // It is possible the system is in the middle of transition to the right state when we get
        // the dump. We try a few times to get the information we need before giving up.
        int retriesLeft = 3;
        boolean retry = false;
        String dump = null;

        log("==============================");
        log("      WindowManagerState      ");
        log("==============================");
        do {
            if (retry) {
                log("***Incomplete WM state. Retrying...");
                // Wait half a second between retries for window manager to finish transitioning...
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    log(e.toString());
                    // Well I guess we are not waiting...
                }
            }

            final CollectingOutputReceiver outputReceiver = new CollectingOutputReceiver();
            final String dumpsysCmd = visibleOnly ?
                    DUMPSYS_WINDOWS_VISIBLE_APPS : DUMPSYS_WINDOWS_APPS;
            device.executeShellCommand(dumpsysCmd, outputReceiver);
            dump = outputReceiver.getOutput();
            parseSysDump(dump, visibleOnly);

            retry = mWindows.isEmpty() || mFocusedWindow == null || mFocusedApp == null;
        } while (retry && retriesLeft-- > 0);

        if (retry) {
            log(dump);
        }

        if (mWindows.isEmpty()) {
            logE("No Windows found...");
        }
        if (mFocusedWindow == null) {
            logE("No Focused Window...");
        }
        if (mFocusedApp == null) {
            logE("No Focused App...");
        }
    }

    private void parseSysDump(String sysDump, boolean visibleOnly) {
        reset();

        Collections.addAll(mSysDump, sysDump.split("\\n"));

        while (!mSysDump.isEmpty()) {
            final Display display =
                    Display.create(mSysDump, sExtractStackExitPatterns);
            if (display != null) {
                log(display.toString());
                mDisplays.add(display);
                continue;
            }

            final WindowStack stack =
                    WindowStack.create(mSysDump, sStackIdPattern, sExtractStackExitPatterns);

            if (stack != null) {
                mStacks.add(stack);
                continue;
            }


            final WindowState ws = WindowState.create(mSysDump, sExtractStackExitPatterns);
            if (ws != null) {
                log(ws.toString());

                if (visibleOnly) {
                    // Check to see if we are in the middle of transitioning. If we are, we want to
                    // skip dumping until window manager is done transitioning windows.
                    if (ws.isStartingWindow()) {
                        log("Skipping dump due to starting window transition...");
                        return;
                    }

                    if (ws.isExitingWindow()) {
                        log("Skipping dump due to exiting window transition...");
                        return;
                    }
                }

                mWindows.add(ws.getName());
                mWindowStates.add(ws);
                continue;
            }

            final String line = mSysDump.pop().trim();

            Matcher matcher = sFocusedWindowPattern.matcher(line);
            if (matcher.matches()) {
                log(line);
                final String focusedWindow = matcher.group(3);
                log(focusedWindow);
                mFocusedWindow = focusedWindow;
                continue;
            }

            matcher = sAppErrorFocusedWindowPattern.matcher(line);
            if (matcher.matches()) {
                log(line);
                final String focusedWindow = matcher.group(3);
                log(focusedWindow);
                mFocusedWindow = focusedWindow;
                continue;
            }

            matcher = sFocusedAppPattern.matcher(line);
            if (matcher.matches()) {
                log(line);
                final String focusedApp = matcher.group(5);
                log(focusedApp);
                mFocusedApp = focusedApp;
                continue;
            }
        }
    }

    void getMatchingWindowTokens(final String windowName, List<String> tokenList) {
        tokenList.clear();

        for (WindowState ws : mWindowStates) {
            if (windowName.equals(ws.getName())) {
                tokenList.add(ws.getToken());
            }
        }
    }

    void getMatchingWindowState(final String windowName, List<WindowState> windowList) {
        windowList.clear();
        for (WindowState ws : mWindowStates) {
            if (windowName.equals(ws.getName())) {
                windowList.add(ws);
            }
        }
    }

    Display getDisplay(int displayId) {
        for (Display display : mDisplays) {
            if (displayId == display.getDisplayId()) {
                return display;
            }
        }
        return null;
    }

    String getFrontWindow() {
        if (mWindows == null || mWindows.isEmpty()) {
            return null;
        }
        return mWindows.get(0);
    }

    String getFocusedWindow() {
        return mFocusedWindow;
    }

    String getFocusedApp() {
        return mFocusedApp;
    }

    int getFrontStackId() {
        return mStacks.get(0).mStackId;
    }

    boolean containsStack(int stackId) {
        for (WindowStack stack : mStacks) {
            if (stackId == stack.mStackId) {
                return true;
            }
        }
        return false;
    }

    boolean isWindowVisible(String windowName) {
        for (String window : mWindows) {
            if (window.equals(windowName)) {
                return true;
            }
        }
        return false;
    }

    WindowStack getStack(int stackId) {
        for (WindowStack stack : mStacks) {
            if (stackId == stack.mStackId) {
                return stack;
            }
        }
        return null;
    }

    private void reset() {
        mSysDump.clear();
        mStacks.clear();
        mDisplays.clear();
        mWindows.clear();
        mWindowStates.clear();
        mFocusedWindow = null;
        mFocusedApp = null;
    }

    static class WindowStack extends WindowContainer {

        private static final Pattern sTaskIdPattern = Pattern.compile("taskId=(\\d+)");

        int mStackId;
        ArrayList<WindowTask> mTasks = new ArrayList();

        private WindowStack() {

        }

        static WindowStack create(
                LinkedList<String> dump, Pattern stackIdPattern, Pattern[] exitPatterns) {
            final String line = dump.peek().trim();

            final Matcher matcher = stackIdPattern.matcher(line);
            if (!matcher.matches()) {
                // Not a stack.
                return null;
            }
            // For the stack Id line we just read.
            dump.pop();

            final WindowStack stack = new WindowStack();
            log(line);
            final String stackId = matcher.group(1);
            log(stackId);
            stack.mStackId = Integer.parseInt(stackId);
            stack.extract(dump, exitPatterns);
            return stack;
        }

        void extract(LinkedList<String> dump, Pattern[] exitPatterns) {

            final List<Pattern> taskExitPatterns = new ArrayList();
            Collections.addAll(taskExitPatterns, exitPatterns);
            taskExitPatterns.add(sTaskIdPattern);
            final Pattern[] taskExitPatternsArray =
                    taskExitPatterns.toArray(new Pattern[taskExitPatterns.size()]);

            while (!doneExtracting(dump, exitPatterns)) {
                final WindowTask task =
                        WindowTask.create(dump, sTaskIdPattern, taskExitPatternsArray);

                if (task != null) {
                    mTasks.add(task);
                    continue;
                }

                final String line = dump.pop().trim();

                if (extractFullscreen(line)) {
                    continue;
                }

                if (extractBounds(line)) {
                    continue;
                }
            }
        }

        WindowTask getTask(int taskId) {
            for (WindowTask task : mTasks) {
                if (taskId == task.mTaskId) {
                    return task;
                }
            }
            return null;
        }
    }

    static class WindowTask extends WindowContainer {
        private static final Pattern sTempInsetBoundsPattern =
                Pattern.compile("mTempInsetBounds=\\[(\\d+),(\\d+)\\]\\[(\\d+),(\\d+)\\]");

        private static final Pattern sAppTokenPattern = Pattern.compile(
                "Activity #(\\d+) AppWindowToken\\{(\\S+) token=Token\\{(\\S+) "
                + "ActivityRecord\\{(\\S+) u(\\d+) (\\S+) t(\\d+)\\}\\}\\}");


        int mTaskId;
        Rectangle mTempInsetBounds;
        List<String> mAppTokens = new ArrayList();

        private WindowTask() {
        }

        static WindowTask create(
                LinkedList<String> dump, Pattern taskIdPattern, Pattern[] exitPatterns) {
            final String line = dump.peek().trim();

            final Matcher matcher = taskIdPattern.matcher(line);
            if (!matcher.matches()) {
                // Not a task.
                return null;
            }
            // For the task Id line we just read.
            dump.pop();

            final WindowTask task = new WindowTask();
            log(line);
            final String taskId = matcher.group(1);
            log(taskId);
            task.mTaskId = Integer.parseInt(taskId);
            task.extract(dump, exitPatterns);
            return task;
        }

        private void extract(LinkedList<String> dump, Pattern[] exitPatterns) {
            while (!doneExtracting(dump, exitPatterns)) {
                final String line = dump.pop().trim();

                if (extractFullscreen(line)) {
                    continue;
                }

                if (extractBounds(line)) {
                    continue;
                }

                Matcher matcher = sTempInsetBoundsPattern.matcher(line);
                if (matcher.matches()) {
                    log(line);
                    mTempInsetBounds = extractBounds(matcher);
                }

                matcher = sAppTokenPattern.matcher(line);
                if (matcher.matches()) {
                    log(line);
                    final String appToken = matcher.group(6);
                    log(appToken);
                    mAppTokens.add(appToken);
                    continue;
                }
            }
        }
    }

    static abstract class WindowContainer {
        protected static final Pattern sFullscreenPattern = Pattern.compile("mFullscreen=(\\S+)");
        protected static final Pattern sBoundsPattern =
                Pattern.compile("mBounds=\\[(-?\\d+),(-?\\d+)\\]\\[(-?\\d+),(-?\\d+)\\]");

        protected boolean mFullscreen;
        protected Rectangle mBounds;

        static boolean doneExtracting(LinkedList<String> dump, Pattern[] exitPatterns) {
            if (dump.isEmpty()) {
                return true;
            }
            final String line = dump.peek().trim();

            for (Pattern pattern : exitPatterns) {
                if (pattern.matcher(line).matches()) {
                    return true;
                }
            }
            return false;
        }

        boolean extractFullscreen(String line) {
            final Matcher matcher = sFullscreenPattern.matcher(line);
            if (!matcher.matches()) {
                return false;
            }
            log(line);
            final String fullscreen = matcher.group(1);
            log(fullscreen);
            mFullscreen = Boolean.valueOf(fullscreen);
            return true;
        }

        boolean extractBounds(String line) {
            final Matcher matcher = sBoundsPattern.matcher(line);
            if (!matcher.matches()) {
                return false;
            }
            log(line);
            mBounds = extractBounds(matcher);
            return true;
        }

        static Rectangle extractBounds(Matcher matcher) {
            final int left = Integer.valueOf(matcher.group(1));
            final int top = Integer.valueOf(matcher.group(2));
            final int right = Integer.valueOf(matcher.group(3));
            final int bottom = Integer.valueOf(matcher.group(4));
            final Rectangle rect = new Rectangle(left, top, right - left, bottom - top);

            log(rect.toString());
            return rect;
        }

        static void extractMultipleBounds(Matcher matcher, int groupIndex, Rectangle... rectList) {
            for (Rectangle rect : rectList) {
                if (rect == null) {
                    return;
                }
                final int left = Integer.valueOf(matcher.group(groupIndex++));
                final int top = Integer.valueOf(matcher.group(groupIndex++));
                final int right = Integer.valueOf(matcher.group(groupIndex++));
                final int bottom = Integer.valueOf(matcher.group(groupIndex++));
                rect.setBounds(left, top, right - left, bottom - top);
            }
        }

        Rectangle getBounds() {
            return mBounds;
        }

        boolean isFullscreen() {
            return mFullscreen;
        }
    }

    static class Display extends WindowContainer {
        private static final String TAG = "[Display] ";

        private static final Pattern sDisplayIdPattern =
                Pattern.compile("Display: mDisplayId=(\\d+)");
        private static final Pattern sDisplayInfoPattern =
                Pattern.compile("(.+) (\\d+)dpi cur=(\\d+)x(\\d+) app=(\\d+)x(\\d+) (.+)");

        private final int mDisplayId;
        private Rectangle mDisplayRect = new Rectangle();
        private Rectangle mAppRect = new Rectangle();
        private int mDpi;

        private Display(int displayId) {
            mDisplayId = displayId;
        }

        int getDisplayId() {
            return mDisplayId;
        }

        int getDpi() {
            return mDpi;
        }

        Rectangle getDisplayRect() {
            return mDisplayRect;
        }

        Rectangle getAppRect() {
            return mAppRect;
        }

        static Display create(LinkedList<String> dump, Pattern[] exitPatterns) {
            // TODO: exit pattern for displays?
            final String line = dump.peek().trim();

            Matcher matcher = sDisplayIdPattern.matcher(line);
            if (!matcher.matches()) {
                return null;
            }

            log(TAG + "DISPLAY_ID: " + line);
            dump.pop();

            final int displayId = Integer.valueOf(matcher.group(1));
            final Display display = new Display(displayId);
            display.extract(dump, exitPatterns);
            return display;
        }

        private void extract(LinkedList<String> dump, Pattern[] exitPatterns) {
            while (!doneExtracting(dump, exitPatterns)) {
                final String line = dump.pop().trim();

                final Matcher matcher = sDisplayInfoPattern.matcher(line);
                if (matcher.matches()) {
                    log(TAG + "DISPLAY_INFO: " + line);
                    mDpi = Integer.valueOf(matcher.group(2));

                    final int displayWidth = Integer.valueOf(matcher.group(3));
                    final int displayHeight = Integer.valueOf(matcher.group(4));
                    mDisplayRect.setBounds(0, 0, displayWidth, displayHeight);

                    final int appWidth = Integer.valueOf(matcher.group(5));
                    final int appHeight = Integer.valueOf(matcher.group(6));
                    mAppRect.setBounds(0, 0, appWidth, appHeight);

                    // break as we don't need other info for now
                    break;
                }
                // Extract other info here if needed
            }
        }

        @Override
        public String toString() {
            return "Display #" + mDisplayId + ": mDisplayRect=" + mDisplayRect
                    + " mAppRect=" + mAppRect;
        }
    }

    static class WindowState extends WindowContainer {
        private static final String TAG = "[WindowState] ";

        private static final String RECT_STR = "\\[(\\d+),(\\d+)\\]\\[(\\d+),(\\d+)\\]";
        private static final Pattern sFramePattern =
                Pattern.compile("Frames: containing=" + RECT_STR + " parent=" + RECT_STR);
        private static final Pattern sWindowAssociationPattern =
                Pattern.compile("mDisplayId=(\\d+) stackId=(\\d+) (.+)");

        private final String mName;
        private final String mAppToken;
        private final boolean mStarting;
        private final boolean mExiting;
        private int mDisplayId;
        private int mStackId;
        private Rectangle mContainingFrame = new Rectangle();
        private Rectangle mParentFrame = new Rectangle();

        private WindowState(Matcher matcher, boolean starting, boolean exiting) {
            mName = matcher.group(4);
            mAppToken = matcher.group(2);
            mStarting = starting;
            mExiting = exiting;
        }

        String getName() {
            return mName;
        }

        String getToken() {
            return mAppToken;
        }

        boolean isStartingWindow() {
            return mStarting;
        }

        boolean isExitingWindow() {
            return mExiting;
        }

        int getDisplayId() {
            return mDisplayId;
        }

        int getStackId() {
            return mStackId;
        }

        Rectangle getContainingFrame() {
            return mContainingFrame;
        }

        Rectangle getParentFrame() {
            return mParentFrame;
        }

        static WindowState create(LinkedList<String> dump, Pattern[] exitPatterns) {
            final String line = dump.peek().trim();

            Matcher matcher = sWindowPattern.matcher(line);
            if (!matcher.matches()) {
                return null;
            }

            log(TAG + "WINDOW: " + line);
            dump.pop();

            final WindowState window;
            Matcher specialMatcher = sStartingWindowPattern.matcher(line);
            if (specialMatcher.matches()) {
                log(TAG + "STARTING: " + line);
                window = new WindowState(specialMatcher, true, false);
            } else {
                specialMatcher = sExitingWindowPattern.matcher(line);
                if (specialMatcher.matches()) {
                    log(TAG + "EXITING: " + line);
                    window = new WindowState(specialMatcher, false, true);
                } else {
                    window = new WindowState(matcher, false, false);
                }
            }

            window.extract(dump, exitPatterns);
            return window;
        }

        private void extract(LinkedList<String> dump, Pattern[] exitPatterns) {
            while (!doneExtracting(dump, exitPatterns)) {
                final String line = dump.pop().trim();

                Matcher matcher = sWindowAssociationPattern.matcher(line);
                if (matcher.matches()) {
                    log(TAG + "WINDOW_ASSOCIATION: " + line);
                    mDisplayId = Integer.valueOf(matcher.group(1));
                    mStackId = Integer.valueOf(matcher.group(2));
                    continue;
                }

                matcher = sFramePattern.matcher(line);
                if (matcher.matches()) {
                    log(TAG + "FRAME: " + line);
                    extractMultipleBounds(matcher, 1, mContainingFrame, mParentFrame);
                    continue;
                }

                // Extract other info here if needed
            }
        }

        @Override
        public String toString() {
            return "WindowState: {" + mAppToken + " " + mName
                    + (mStarting ? " STARTING" : "") + (mExiting ? " EXITING" : "") + "}"
                    + " cf=" + mContainingFrame + " pf=" + mParentFrame;
        }
    }
}
