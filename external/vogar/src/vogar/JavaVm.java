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

package vogar;

import com.google.common.collect.Iterables;
import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Set;
import vogar.commands.VmCommandBuilder;
import vogar.tasks.RunActionTask;
import vogar.tasks.Task;

/**
 * A local Java virtual machine like Harmony or the RI.
 */
final class JavaVm implements Mode {
    private final Run run;

    JavaVm(Run run) {
        this.run = run;
    }

    @Override public Set<Task> installTasks() {
        return Collections.emptySet();
    }

    @Override public VmCommandBuilder newVmCommandBuilder(Action action, File workingDirectory) {
        List<String> vmCommand = new ArrayList<String>();
        Iterables.addAll(vmCommand, run.invokeWith());
        vmCommand.add(run.javaPath(run.vmCommand));
        if (run.profile) {
            vmCommand.add("-agentlib:hprof="
                          + "cpu=samples,"
                          + "format=" + (run.profileBinary ? 'b' : 'a') + ","
                          + "file=" + run.profileFile + ","
                          + "depth=" + run.profileDepth + ","
                          + "interval=" + run.profileInterval + ","
                          + "thread=y,"
                          + "verbose=n");
        }
        return new VmCommandBuilder(run.log)
                .userDir(workingDirectory)
                .vmCommand(vmCommand);
    }

    @Override public Task executeActionTask(Action action, boolean useLargeTimeout) {
        return new RunActionTask(run, action, useLargeTimeout);
    }

    @Override public Set<Task> installActionTasks(Action action, File jar) {
        return Collections.emptySet();
    }

    @Override public Classpath getRuntimeClasspath(Action action) {
        Classpath result = new Classpath();
        result.addAll(run.classpath);
        result.addAll(run.hostJar(action));

        /*
         * For javax.net.ssl tests dependency on Bouncy Castle for
         * creating a self-signed X509 certificate. Needs to be run
         * with an openjdk, not a sunjdk, which expects a signed jar
         * to authenticate security providers. For example:
         *
         * --java-home /usr/lib/jvm/java-6-openjdk
         */
        result.addAll(new File("/usr/share/java/bcprov.jar"));

        result.addAll(run.resourceClasspath);
        return result;
    }

    @Override public Set<Task> cleanupTasks(Action action) {
        return Collections.emptySet();
    }
}
