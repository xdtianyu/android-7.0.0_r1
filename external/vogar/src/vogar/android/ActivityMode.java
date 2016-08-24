/*
 * Copyright (C) 2009 The Android Open Source Project
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

import java.io.File;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.Set;
import vogar.Action;
import vogar.Classpath;
import vogar.Mode;
import vogar.Run;
import vogar.commands.VmCommandBuilder;
import vogar.tasks.ExtractJarResourceTask;
import vogar.tasks.Task;

/**
 * Runs an action in the context of an android.app.Activity on a device
 */
public final class ActivityMode implements Mode {
    private final Run run;

    public ActivityMode(Run run) {
        this.run = run;
    }

    @Override public Set<Task> installTasks() {
        return Collections.<Task>singleton(
                new ExtractJarResourceTask("/vogar/vogar.keystore", run.keystore));
    }

    @Override public Set<Task> installActionTasks(Action action, File jar) {
        return Collections.<Task>singleton(new InstallApkTask(run, action, jar));
    }

    @Override public Task executeActionTask(Action action, boolean useLargeTimeout) {
        return new RunActivityTask(run, action, useLargeTimeout);
    }

    @Override public Set<Task> cleanupTasks(Action action) {
        Set<Task> result = new LinkedHashSet<Task>();
        result.add(run.target.rmTask(action.getUserDir()));
        result.add(new UninstallApkTask(run.androidSdk, action.getName()));
        return result;
    }

    @Override public VmCommandBuilder newVmCommandBuilder(Action action, File workingDirectory) {
        throw new UnsupportedOperationException();
    }

    @Override public Classpath getRuntimeClasspath(Action action) {
        throw new UnsupportedOperationException();
    }
}
