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
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.TimeUnit;
import vogar.Console;
import vogar.Result;
import vogar.util.Threads;

/**
 * A set of tasks to execute.
 */
public final class TaskQueue {
    private static final int FOREVER = 60 * 60 * 24 * 28; // four weeks
    private final Console console;
    private int runningTasks;
    private int runningActions;
    private int maxConcurrentActions;
    private final LinkedList<Task> tasks = new LinkedList<Task>();
    private final LinkedList<Task> runnableActions = new LinkedList<Task>();
    private final LinkedList<Task> runnableTasks = new LinkedList<Task>();
    private final List<Task> failedTasks = new ArrayList<Task>();

    public TaskQueue(Console console, int maxConcurrentActions) {
        this.console = console;
        this.maxConcurrentActions = maxConcurrentActions;
    }

    /**
     * Adds a task to the queue.
     */
    public synchronized void enqueue(Task task) {
        tasks.add(task);
    }

    public void enqueueAll(Collection<Task> tasks) {
        this.tasks.addAll(tasks);
    }

    public synchronized List<Task> getTasks() {
        return new ArrayList<Task>(tasks);
    }

    public void runTasks() {
        promoteBlockedTasks();

        ExecutorService runners = Threads.threadPerCpuExecutor(console, "TaskQueue");
        for (int i = 0; i < Runtime.getRuntime().availableProcessors(); i++) {
            runners.execute(new Runnable() {
                @Override public void run() {
                    while (runOneTask()) {
                    }
                }
            });
        }

        runners.shutdown();
        try {
            runners.awaitTermination(FOREVER, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            throw new AssertionError();
        }
    }

    public void printTasks() {
        if (!console.isVerbose()) {
            return;
        }

        int i = 0;
        for (Task task : tasks) {
            StringBuilder message = new StringBuilder()
                    .append("Task ").append(i++).append(": ").append(task);
            for (Task blocker : task.tasksThatMustFinishFirst) {
                message.append("\n  depends on completed task: ").append(blocker);
            }
            for (Task blocker : task.tasksThatMustFinishSuccessfullyFirst) {
                message.append("\n  depends on successful task: ").append(blocker);
            }
            console.verbose(message.toString());
        }
    }

    public boolean hasFailedTasks() {
        return !failedTasks.isEmpty();
    }

    public void printProblemTasks() {
        for (Task task : failedTasks) {
            String message = "Failed task: " + task + " " + task.result;
            if (task.thrown != null) {
                console.info(message, task.thrown);
            } else {
                console.info(message);
            }
        }
        if (!console.isVerbose()) {
            return;
        }
        for (Task task : tasks) {
            StringBuilder message = new StringBuilder()
                    .append("Failed to execute task: ").append(task);
            for (Task blocker : task.tasksThatMustFinishFirst) {
                if (blocker.result == null) {
                    message.append("\n  blocked by unexecuted task: ").append(blocker);
                }
            }
            for (Task blocker : task.tasksThatMustFinishSuccessfullyFirst) {
                if (blocker.result == null) {
                    message.append("\n  blocked by unexecuted task: ").append(blocker);
                } else if (blocker.result != Result.SUCCESS) {
                    message.append("\n  blocked by unsuccessful task: ").append(blocker);
                }
            }
            console.verbose(message.toString());
        }
    }

    private boolean runOneTask() {
        Task task = takeTask();
        if (task == null) {
            return false;
        }
        String threadName = Thread.currentThread().getName();
        Thread.currentThread().setName(task.toString());
        try {
            task.run(console);
        } finally {
            doneTask(task);
            Thread.currentThread().setName(threadName);
        }
        return true;
    }

    private synchronized Task takeTask() {
        while (true) {
            Task task = null;
            if (runningActions < maxConcurrentActions) {
                task = runnableActions.poll();
            }
            if (task == null) {
                task = runnableTasks.poll();
            }

            if (task != null) {
                runningTasks++;
                if (task.isAction()) {
                    runningActions++;
                }
                return task;
            }

            if (isExhausted()) {
                return null;
            }

            try {
                wait();
            } catch (InterruptedException e) {
                throw new AssertionError();
            }
        }
    }

    private synchronized void doneTask(Task task) {
        if (task.result != Result.SUCCESS) {
            failedTasks.add(task);
        }
        runningTasks--;
        if (task.isAction()) {
            runningActions--;
        }
        promoteBlockedTasks();
        if (isExhausted()) {
            notifyAll();
        }
    }

    private synchronized void promoteBlockedTasks() {
        for (Iterator<Task> it = tasks.iterator(); it.hasNext(); ) {
            Task potentiallyUnblocked = it.next();
            if (potentiallyUnblocked.isRunnable()) {
                it.remove();
                if (potentiallyUnblocked.isAction()) {
                    runnableActions.add(potentiallyUnblocked);
                } else {
                    runnableTasks.add(potentiallyUnblocked);
                }
                notifyAll();
            }
        }
    }

    /**
     * Returns true if there are no tasks to run and no tasks currently running.
     */
    private boolean isExhausted() {
        return runnableTasks.isEmpty() && runnableActions.isEmpty() && runningTasks == 0;
    }
}
