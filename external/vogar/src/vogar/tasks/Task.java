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

package vogar.tasks;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import vogar.Console;
import vogar.Result;

/**
 * A task necessary to accomplish the user's requested actions. Tasks have
 * prerequisites; a task must not be run until it reports that it is runnable.
 * Tasks may be run at most once; running a task produces a result.
 */
public abstract class Task {
    private final String name;
    final List<Task> tasksThatMustFinishFirst = new ArrayList<Task>();
    final List<Task> tasksThatMustFinishSuccessfullyFirst = new ArrayList<Task>();
    volatile Result result;
    Exception thrown;

    protected Task(String name) {
        this.name = name;
    }

    /**
     * Returns true if this task is an action task. The queue imposes limits
     * on how many actions may be run concurrently.
     */
    public boolean isAction() {
        return false;
    }

    public Task after(Task prerequisite) {
        tasksThatMustFinishFirst.add(prerequisite);
        return this;
    }

    public Task after(Collection<Task> prerequisites) {
        tasksThatMustFinishFirst.addAll(prerequisites);
        return this;
    }

    public Task afterSuccess(Task prerequisite) {
        tasksThatMustFinishSuccessfullyFirst.add(prerequisite);
        return this;
    }

    public Task afterSuccess(Collection<Task> prerequisitess) {
        tasksThatMustFinishSuccessfullyFirst.addAll(prerequisitess);
        return this;
    }

    public final boolean isRunnable() {
        for (Task task : tasksThatMustFinishFirst) {
            if (task.result == null) {
                return false;
            }
        }
        for (Task task : tasksThatMustFinishSuccessfullyFirst) {
            if (task.result != Result.SUCCESS) {
                return false;
            }
        }
        return true;
    }

    protected abstract Result execute() throws Exception;

    final void run(Console console) {
        if (result != null) {
            throw new IllegalStateException();
        }
        try {
            console.verbose("running " + this);
            result = execute();
        } catch (Exception e) {
            thrown = e;
            result = Result.ERROR;
        }

        if (result != Result.SUCCESS) {
            console.verbose("warning " + this + " " + result);
        }
    }

    @Override public final String toString() {
        return name;
    }
}
