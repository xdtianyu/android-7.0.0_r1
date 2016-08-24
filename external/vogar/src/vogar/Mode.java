/*
 * Copyright (C) 2010 The Android Open Source Project
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

import java.io.File;
import java.util.Set;
import vogar.commands.VmCommandBuilder;
import vogar.tasks.Task;

/**
 * A Mode for running actions. Examples including running in a virtual machine
 * either on the host or a device or within a specific context such as within an
 * Activity.
 */
public interface Mode {
    /**
     * Initializes the temporary directories and harness necessary to run
     * actions.
     */
    Set<Task> installTasks();

    Task executeActionTask(Action action, boolean useLargeTimeout);

    /**
     * Hook method called after action compilation.
     */
    Set<Task> installActionTasks(Action action, File jar);

    /**
     * Deletes files and releases any resources required for the execution of
     * the given action.
     */
    Set<Task> cleanupTasks(Action action);

    /**
     * Returns a VM for action execution.
     *
     * @param workingDirectory the working directory of the target process. If
     *     the process runs on another device, this is the working directory of
     *     the device.
     */
    VmCommandBuilder newVmCommandBuilder(Action action, File workingDirectory);

    /**
     * Returns the classpath containing JUnit and the dalvik annotations
     * required for action execution.
     */
    Classpath getRuntimeClasspath(Action action);
}
