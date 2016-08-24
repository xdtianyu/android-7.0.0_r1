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

import java.awt.Rectangle;
import java.lang.Integer;
import java.lang.String;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;

import java.util.regex.Pattern;
import java.util.regex.Matcher;

import static android.server.cts.ActivityManagerTestBase.HOME_STACK_ID;
import static android.server.cts.StateLogger.log;
import static android.server.cts.StateLogger.logE;

class ActivityManagerState {
    private static final String DUMPSYS_ACTIVITY_ACTIVITIES = "dumpsys activity activities";

    // Copied from ActivityRecord.java
    private static final int APPLICATION_ACTIVITY_TYPE = 0;
    private static final int HOME_ACTIVITY_TYPE = 1;
    private static final int RECENTS_ACTIVITY_TYPE = 2;

    private final Pattern mDisplayIdPattern = Pattern.compile("Display #(\\d+)");
    private final Pattern mStackIdPattern = Pattern.compile("Stack #(\\d+)\\:");
    private final Pattern mFocusedActivityPattern =
            Pattern.compile("mFocusedActivity\\: ActivityRecord\\{(.+) u(\\d+) (\\S+) (\\S+)\\}");
    private final Pattern mFocusedStackPattern =
            Pattern.compile("mFocusedStack=ActivityStack\\{(.+) stackId=(\\d+), (.+)\\}(.+)");

    private final Pattern[] mExtractStackExitPatterns =
            { mStackIdPattern, mFocusedActivityPattern, mFocusedStackPattern};

    // Stacks in z-order with the top most at the front of the list.
    private final List<ActivityStack> mStacks = new ArrayList();
    private int mFocusedStackId = -1;
    private String mFocusedActivityRecord = null;
    private final List<String> mResumedActivities = new ArrayList();
    private final LinkedList<String> mSysDump = new LinkedList();

    void computeState(ITestDevice device) throws DeviceNotAvailableException {
        // It is possible the system is in the middle of transition to the right state when we get
        // the dump. We try a few times to get the information we need before giving up.
        int retriesLeft = 3;
        boolean retry = false;
        String dump = null;

        log("==============================");
        log("     ActivityManagerState     ");
        log("==============================");

        do {
            if (retry) {
                log("***Incomplete AM state. Retrying...");
                // Wait half a second between retries for activity manager to finish transitioning.
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    log(e.toString());
                    // Well I guess we are not waiting...
                }
            }

            final CollectingOutputReceiver outputReceiver = new CollectingOutputReceiver();
            device.executeShellCommand(DUMPSYS_ACTIVITY_ACTIVITIES, outputReceiver);
            dump = outputReceiver.getOutput();
            parseSysDump(dump);

            retry = mStacks.isEmpty() || mFocusedStackId == -1 || mFocusedActivityRecord == null
                    || mResumedActivities.isEmpty();
        } while (retry && retriesLeft-- > 0);

        if (retry) {
            log(dump);
        }

        if (mStacks.isEmpty()) {
            logE("No stacks found...");
        }
        if (mFocusedStackId == -1) {
            logE("No focused stack found...");
        }
        if (mFocusedActivityRecord == null) {
            logE("No focused activity found...");
        }
        if (mResumedActivities.isEmpty()) {
            logE("No resumed activities found...");
        }
    }

    private void parseSysDump(String sysDump) {
        reset();

        Collections.addAll(mSysDump, sysDump.split("\\n"));

        int currentDisplayId = 0;
        while (!mSysDump.isEmpty()) {
            final ActivityStack stack = ActivityStack.create(mSysDump, mStackIdPattern,
                    mExtractStackExitPatterns, currentDisplayId);

            if (stack != null) {
                mStacks.add(stack);
                if (stack.mResumedActivity != null) {
                    mResumedActivities.add(stack.mResumedActivity);
                }
                continue;
            }

            final String line = mSysDump.pop().trim();

            Matcher matcher = mFocusedStackPattern.matcher(line);
            if (matcher.matches()) {
                log(line);
                final String stackId = matcher.group(2);
                log(stackId);
                mFocusedStackId = Integer.parseInt(stackId);
                continue;
            }

            matcher = mFocusedActivityPattern.matcher(line);
            if (matcher.matches()) {
                log(line);
                mFocusedActivityRecord = matcher.group(3);
                log(mFocusedActivityRecord);
                continue;
            }

            matcher = mDisplayIdPattern.matcher(line);
            if (matcher.matches()) {
                log(line);
                final String displayId = matcher.group(2);
                log(displayId);
                currentDisplayId = Integer.parseInt(displayId);
            }
        }
    }

    private void reset() {
        mStacks.clear();
        mFocusedStackId = -1;
        mFocusedActivityRecord = null;
        mResumedActivities.clear();
        mSysDump.clear();
    }

    int getFrontStackId() {
        return mStacks.get(0).mStackId;
    }

    int getFocusedStackId() {
        return mFocusedStackId;
    }

    String getFocusedActivity() {
        return mFocusedActivityRecord;
    }

    String getResumedActivity() {
        return mResumedActivities.get(0);
    }

    int getResumedActivitiesCount() {
        return mResumedActivities.size();
    }

    boolean containsStack(int stackId) {
        return getStackById(stackId) != null;
    }

    ActivityStack getStackById(int stackId) {
        for (ActivityStack stack : mStacks) {
            if (stackId == stack.mStackId) {
                return stack;
            }
        }
        return null;
    }

    List<ActivityStack> getStacks() {
        return new ArrayList(mStacks);
    }

    int getStackCount() {
        return mStacks.size();
    }

    boolean isActivityVisible(String activityName) {
        for (ActivityStack stack : mStacks) {
            for (ActivityTask task : stack.mTasks) {
               for (Activity activity : task.mActivities) {
                   if (activity.name.equals(activityName)) {
                       return activity.visible;
                   }
               }
            }
        }
        return false;
    }

    boolean isHomeActivityVisible() {
        final Activity homeActivity = getHomeActivity();
        return homeActivity != null && homeActivity.visible;
    }

    private Activity getHomeActivity() {
        for (ActivityStack stack : mStacks) {
            if (stack.mStackId != HOME_STACK_ID) {
                continue;
            }

            for (ActivityTask task : stack.mTasks) {
                if (task.mTaskType != HOME_ACTIVITY_TYPE) {
                    continue;
                }
                return task.mActivities.get(task.mActivities.size() - 1);
            }

            return null;
        }
        return null;
    }

    ActivityTask getTaskByActivityName(String activityName) {
        return getTaskByActivityName(activityName, -1);
    }

    ActivityTask getTaskByActivityName(String activityName, int stackId) {
        String fullName = ActivityManagerTestBase.getActivityComponentName(activityName);
        for (ActivityStack stack : mStacks) {
            if (stackId == -1 || stackId == stack.mStackId) {
                for (ActivityTask task : stack.mTasks) {
                    for (Activity activity : task.mActivities) {
                        if (activity.name.equals(fullName)) {
                            return task;
                        }
                    }
                }
            }
        }
        return null;
    }

    static class ActivityStack extends ActivityContainer {

        private static final Pattern TASK_ID_PATTERN = Pattern.compile("Task id #(\\d+)");
        private static final Pattern RESUMED_ACTIVITY_PATTERN = Pattern.compile(
                "mResumedActivity\\: ActivityRecord\\{(.+) u(\\d+) (\\S+) (\\S+)\\}");

        int mDisplayId;
        int mStackId;
        String mResumedActivity;
        ArrayList<ActivityTask> mTasks = new ArrayList();

        private ActivityStack() {
        }

        static ActivityStack create(LinkedList<String> dump, Pattern stackIdPattern,
                                    Pattern[] exitPatterns, int displayId) {
            final String line = dump.peek().trim();

            final Matcher matcher = stackIdPattern.matcher(line);
            if (!matcher.matches()) {
                // Not a stack.
                return null;
            }
            // For the stack Id line we just read.
            dump.pop();

            final ActivityStack stack = new ActivityStack();
            stack.mDisplayId = displayId;
            log(line);
            final String stackId = matcher.group(1);
            log(stackId);
            stack.mStackId = Integer.parseInt(stackId);
            stack.extract(dump, exitPatterns);
            return stack;
        }

        private void extract(LinkedList<String> dump, Pattern[] exitPatterns) {

            final List<Pattern> taskExitPatterns = new ArrayList();
            Collections.addAll(taskExitPatterns, exitPatterns);
            taskExitPatterns.add(TASK_ID_PATTERN);
            taskExitPatterns.add(RESUMED_ACTIVITY_PATTERN);
            final Pattern[] taskExitPatternsArray =
                    taskExitPatterns.toArray(new Pattern[taskExitPatterns.size()]);

            while (!doneExtracting(dump, exitPatterns)) {
                final ActivityTask task =
                        ActivityTask.create(dump, TASK_ID_PATTERN, taskExitPatternsArray);

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

                Matcher matcher = RESUMED_ACTIVITY_PATTERN.matcher(line);
                if (matcher.matches()) {
                    log(line);
                    mResumedActivity = matcher.group(3);
                    log(mResumedActivity);
                    continue;
                }
            }
        }

        List<ActivityTask> getTasks() {
            return new ArrayList(mTasks);
        }

        ActivityTask getTask(int taskId) {
            for (ActivityTask task : mTasks) {
                if (taskId == task.mTaskId) {
                    return task;
                }
            }
            return null;
        }
    }

    static class ActivityTask extends ActivityContainer {
        private static final Pattern TASK_RECORD_PATTERN = Pattern.compile("\\* TaskRecord\\"
                + "{(\\S+) #(\\d+) (\\S+)=(\\S+) U=(\\d+) StackId=(\\d+) sz=(\\d+)\\}");

        private static final Pattern LAST_NON_FULLSCREEN_BOUNDS_PATTERN = Pattern.compile(
                "mLastNonFullscreenBounds=Rect\\((\\d+), (\\d+) - (\\d+), (\\d+)\\)");

        private static final Pattern ORIG_ACTIVITY_PATTERN = Pattern.compile("origActivity=(\\S+)");
        private static final Pattern REAL_ACTIVITY_PATTERN = Pattern.compile("realActivity=(\\S+)");

        private static final Pattern ACTIVITY_NAME_PATTERN = Pattern.compile(
                "\\* Hist #(\\d+)\\: ActivityRecord\\{(\\S+) u(\\d+) (\\S+) t(\\d+)\\}");

        private static final Pattern TASK_TYPE_PATTERN = Pattern.compile("autoRemoveRecents=(\\S+) "
                + "isPersistable=(\\S+) numFullscreen=(\\d+) taskType=(\\d+) "
                + "mTaskToReturnTo=(\\d+)");

        int mTaskId;
        int mStackId;
        Rectangle mLastNonFullscreenBounds;
        String mRealActivity;
        String mOrigActivity;
        ArrayList<Activity> mActivities = new ArrayList();
        int mTaskType = -1;
        int mReturnToType = -1;

        private ActivityTask() {
        }

        static ActivityTask create(
                LinkedList<String> dump, Pattern taskIdPattern, Pattern[] exitPatterns) {
            final String line = dump.peek().trim();

            final Matcher matcher = taskIdPattern.matcher(line);
            if (!matcher.matches()) {
                // Not a task.
                return null;
            }
            // For the task Id line we just read.
            dump.pop();

            final ActivityTask task = new ActivityTask();
            log(line);
            final String taskId = matcher.group(1);
            log(taskId);
            task.mTaskId = Integer.parseInt(taskId);
            task.extract(dump, exitPatterns);
            return task;
        }

        private void extract(LinkedList<String> dump, Pattern[] exitPatterns) {
            final List<Pattern> activityExitPatterns = new ArrayList();
            Collections.addAll(activityExitPatterns, exitPatterns);
            activityExitPatterns.add(ACTIVITY_NAME_PATTERN);
            final Pattern[] activityExitPatternsArray =
                    activityExitPatterns.toArray(new Pattern[activityExitPatterns.size()]);

            while (!doneExtracting(dump, exitPatterns)) {
                final Activity activity =
                        Activity.create(dump, ACTIVITY_NAME_PATTERN, activityExitPatternsArray);

                if (activity != null) {
                    mActivities.add(activity);
                    continue;
                }

                final String line = dump.pop().trim();

                if (extractFullscreen(line)) {
                    continue;
                }

                if (extractBounds(line)) {
                    continue;
                }

                if (extractMinimalSize(line)) {
                    continue;
                }

                Matcher matcher = TASK_RECORD_PATTERN.matcher(line);
                if (matcher.matches()) {
                    log(line);
                    final String stackId = matcher.group(6);
                    mStackId = Integer.valueOf(stackId);
                    log(stackId);
                    continue;
                }

                matcher = LAST_NON_FULLSCREEN_BOUNDS_PATTERN.matcher(line);
                if (matcher.matches()) {
                    log(line);
                    mLastNonFullscreenBounds = extractBounds(matcher);
                }

                matcher = REAL_ACTIVITY_PATTERN.matcher(line);
                if (matcher.matches()) {
                    if (mRealActivity == null) {
                        log(line);
                        mRealActivity = matcher.group(1);
                        log(mRealActivity);
                    }
                    continue;
                }

                matcher = ORIG_ACTIVITY_PATTERN.matcher(line);
                if (matcher.matches()) {
                    if (mOrigActivity == null) {
                        log(line);
                        mOrigActivity = matcher.group(1);
                        log(mOrigActivity);
                    }
                    continue;
                }

                matcher = TASK_TYPE_PATTERN.matcher(line);
                if (matcher.matches()) {
                    log(line);
                    mTaskType = Integer.valueOf(matcher.group(4));
                    mReturnToType = Integer.valueOf(matcher.group(5));
                    continue;
                }
            }
        }
    }

    static class Activity {
        private static final Pattern VISIBILITY_PATTERN = Pattern.compile("keysPaused=(\\S+) "
                + "inHistory=(\\S+) visible=(\\S+) sleeping=(\\S+) idle=(\\S+) "
                + "mStartingWindowState=(\\S+)");
        private static final Pattern FRONT_OF_TASK_PATTERN = Pattern.compile("frontOfTask=(\\S+) "
                + "task=TaskRecord\\{(\\S+) #(\\d+) A=(\\S+) U=(\\d+) StackId=(\\d+) sz=(\\d+)\\}");

        String name;
        boolean visible;
        boolean frontOfTask;

        private Activity() {
        }

        static Activity create(
                LinkedList<String> dump, Pattern activityNamePattern, Pattern[] exitPatterns) {
            final String line = dump.peek().trim();

            final Matcher matcher = activityNamePattern.matcher(line);
            if (!matcher.matches()) {
                // Not an activity.
                return null;
            }
            // For the activity name line we just read.
            dump.pop();

            final Activity activity = new Activity();
            log(line);
            activity.name = matcher.group(4);
            log(activity.name);
            activity.extract(dump, exitPatterns);
            return activity;
        }

        private void extract(LinkedList<String> dump, Pattern[] exitPatterns) {

            while (!doneExtracting(dump, exitPatterns)) {
                final String line = dump.pop().trim();

                Matcher matcher = VISIBILITY_PATTERN.matcher(line);
                if (matcher.matches()) {
                    log(line);
                    final String visibleString = matcher.group(3);
                    visible = Boolean.valueOf(visibleString);
                    log(visibleString);
                    continue;
                }

                matcher = FRONT_OF_TASK_PATTERN.matcher(line);
                if (matcher.matches()) {
                    log(line);
                    final String frontOfTaskString = matcher.group(1);
                    frontOfTask = Boolean.valueOf(frontOfTaskString);
                    log(frontOfTaskString);
                    continue;
                }
            }
        }
    }

    static abstract class ActivityContainer {
        protected static final Pattern FULLSCREEN_PATTERN = Pattern.compile("mFullscreen=(\\S+)");
        protected static final Pattern BOUNDS_PATTERN =
                Pattern.compile("mBounds=Rect\\((\\d+), (\\d+) - (\\d+), (\\d+)\\)");
        protected static final Pattern MIN_WIDTH_PATTERN =
                Pattern.compile("mMinWidth=(\\d+)");
        protected static final Pattern MIN_HEIGHT_PATTERN =
                Pattern.compile("mMinHeight=(\\d+)");

        protected boolean mFullscreen;
        protected Rectangle mBounds;
        protected int mMinWidth = -1;
        protected int mMinHeight = -1;

        boolean extractFullscreen(String line) {
            final Matcher matcher = FULLSCREEN_PATTERN.matcher(line);
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
            final Matcher matcher = BOUNDS_PATTERN.matcher(line);
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

        boolean extractMinimalSize(String line) {
            final Matcher minWidthMatcher = MIN_WIDTH_PATTERN.matcher(line);
            final Matcher minHeightMatcher = MIN_HEIGHT_PATTERN.matcher(line);

            if (minWidthMatcher.matches()) {
                log(line);
                mMinWidth = Integer.valueOf(minWidthMatcher.group(1));
            } else if (minHeightMatcher.matches()) {
                log(line);
                mMinHeight = Integer.valueOf(minHeightMatcher.group(1));
            } else {
                return false;
            }
            return true;
        }

        Rectangle getBounds() {
            return mBounds;
        }

        boolean isFullscreen() {
            return mFullscreen;
        }

        int getMinWidth() {
            return mMinWidth;
        }

        int getMinHeight() {
            return mMinHeight;
        }
    }

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
}
