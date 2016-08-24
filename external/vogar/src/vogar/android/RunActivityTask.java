/*
 * Copyright (C) 2011 The Android Open Source Project
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

package vogar.android;

import vogar.Action;
import vogar.Run;
import vogar.commands.Command;
import vogar.tasks.RunActionTask;

public final class RunActivityTask extends RunActionTask {
    public RunActivityTask(Run run, Action action, boolean useLargeTimeout) {
        super(run, action, useLargeTimeout);
    }

    @Override public Command createActionCommand(Action action, String skipPast, int monitorPort) {
        if (monitorPort != -1) {
            throw new IllegalArgumentException("ActivityMode doesn't support runtime monitor ports!");
        }

        return new Command(run.log,
                "adb", "shell", "am", "start", "-W",
                "-a", "android.intent.action.MAIN",
                "-n", (InstallApkTask.packageName(action) + "/" + InstallApkTask.ACTIVITY_CLASS));
    }

    @Override public boolean useSocketMonitor() {
        return true;
    }
}
