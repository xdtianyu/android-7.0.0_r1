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

import java.io.File;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import vogar.tasks.BuildActionTask;
import vogar.tasks.PrepareTarget;
import vogar.tasks.PrepareUserDirTask;
import vogar.tasks.RetrieveFilesTask;
import vogar.tasks.RmTask;
import vogar.tasks.Task;
import vogar.util.TimeUtilities;

/**
 * Compiles, installs, runs and reports on actions.
 */
public final class Driver {
    private final Run run;

    public Driver(Run run) {
        this.run = run;
    }

    private int successes = 0;
    private int failures = 0;
    private int skipped = 0;
    private int warnings = 0;

    private Task prepareTargetTask;
    private Set<Task> installVogarTasks;

    private final Map<String, Action> actions = Collections.synchronizedMap(
            new LinkedHashMap<String, Action>());
    private final Map<String, Outcome> outcomes = Collections.synchronizedMap(
            new LinkedHashMap<String, Outcome>());
    public boolean recordResults = true;

    /**
     * Builds and executes the actions in the given files.
     */
    public boolean buildAndRun(Collection<File> files, Collection<String> classes) {
        if (!actions.isEmpty()) {
            throw new IllegalStateException("Drivers are not reusable");
        }

        run.mkdir.mkdirs(run.localTemp);

        filesToActions(files);
        classesToActions(classes);

        if (actions.isEmpty()) {
            run.console.info("Nothing to do.");
            return false;
        }

        run.console.info("Actions: " + actions.size());
        final long t0 = System.currentTimeMillis();

        prepareTargetTask = new PrepareTarget(run, run.target);
        run.taskQueue.enqueue(prepareTargetTask);

        installVogarTasks = run.mode.installTasks();
        run.taskQueue.enqueueAll(installVogarTasks);
        registerPrerequisites(Collections.singleton(prepareTargetTask), installVogarTasks);

        for (Action action : actions.values()) {
            action.setUserDir(new File(run.runnerDir, action.getName()));
            Outcome outcome = outcomes.get(action.getName());
            if (outcome != null) {
                addEarlyResult(outcome);
            } else if (run.expectationStore.get(action.getName()).getResult() == Result.UNSUPPORTED) {
                addEarlyResult(new Outcome(action.getName(), Result.UNSUPPORTED,
                    "Unsupported according to expectations file"));
            } else {
                enqueueActionTasks(action);
            }
        }

        if (run.cleanAfter) {
            Set<Task> shutdownTasks = new HashSet<Task>();
            shutdownTasks.add(new RmTask(run.rm, run.localTemp));
            shutdownTasks.add(run.target.rmTask(run.runnerDir));
            for (Task task : shutdownTasks) {
                task.after(run.taskQueue.getTasks());
            }
            run.taskQueue.enqueueAll(shutdownTasks);
        }

        run.taskQueue.printTasks();
        run.taskQueue.runTasks();
        if (run.taskQueue.hasFailedTasks()) {
            run.taskQueue.printProblemTasks();
            return false;
        }

        if (run.reportPrinter.isReady()) {
            run.console.info("Printing XML Reports... ");
            int numFiles = run.reportPrinter.generateReports(outcomes.values());
            run.console.info(numFiles + " XML files written.");
        }

        long t1 = System.currentTimeMillis();

        Map<String, AnnotatedOutcome> annotatedOutcomes = run.outcomeStore.read(this.outcomes);
        if (recordResults) {
            run.outcomeStore.write(outcomes);
        }

        run.console.summarizeOutcomes(annotatedOutcomes.values());

        List<String> jarStringList = run.jarSuggestions.getStringList();
        if (!jarStringList.isEmpty()) {
            run.console.warn(
                    "consider adding the following to the classpath:",
                    jarStringList);
        }

        if (failures > 0 || skipped > 0 || warnings > 0) {
            run.console.info(String.format(
                    "Outcomes: %s. Passed: %d, Failed: %d, Skipped: %d, Warnings: %d. Took %s.",
                    (successes + failures + warnings + skipped), successes, failures, skipped, warnings,
                    TimeUtilities.msToString(t1 - t0)));
        } else {
            run.console.info(String.format("Outcomes: %s. All successful. Took %s.",
                    successes, TimeUtilities.msToString(t1 - t0)));
        }
        return failures == 0;
    }

    private void enqueueActionTasks(Action action) {
        Expectation expectation = run.expectationStore.get(action.getName());
        boolean useLargeTimeout = expectation.getTags().contains("large");
        File jar;
        if (run.useJack) {
            jar = run.hostJack(action);
        } else {
            jar = run.hostJar(action);
        }
        Task build = new BuildActionTask(run, action, this, jar);
        run.taskQueue.enqueue(build);

        Task prepareUserDir = new PrepareUserDirTask(run.target, action);
        prepareUserDir.after(installVogarTasks);
        run.taskQueue.enqueue(prepareUserDir);

        Set<Task> install = run.mode.installActionTasks(action, jar);
        registerPrerequisites(Collections.singleton(build), install);
        registerPrerequisites(installVogarTasks, install);
        registerPrerequisites(Collections.singleton(prepareTargetTask), install);
        run.taskQueue.enqueueAll(install);

        Task execute = run.mode.executeActionTask(action, useLargeTimeout)
                .afterSuccess(installVogarTasks)
                .afterSuccess(build)
                .afterSuccess(prepareUserDir)
                .afterSuccess(install);
        run.taskQueue.enqueue(execute);

        Task retrieveFiles = new RetrieveFilesTask(run, action.getUserDir()).after(execute);
        run.taskQueue.enqueue(retrieveFiles);

        if (run.cleanAfter) {
            run.taskQueue.enqueue(new RmTask(run.rm, run.localFile(action))
                    .after(execute).after(retrieveFiles));
            Set<Task> cleanupTasks = run.mode.cleanupTasks(action);
            for (Task task : cleanupTasks) {
                task.after(execute).after(retrieveFiles);
            }
            run.taskQueue.enqueueAll(cleanupTasks);
        }
    }

    private void registerPrerequisites(Set<Task> allBefore, Set<Task> allAfter) {
        for (Task task : allAfter) {
            task.afterSuccess(allBefore);
        }
    }

    private void classesToActions(Collection<String> classNames) {
        for (String className : classNames) {
            Action action = new Action(className, className, null, null, null);
            actions.put(action.getName(), action);
        }
    }

    private void filesToActions(Collection<File> files) {
        for (File file : files) {
            new ActionFinder(run.console, actions, outcomes).findActions(file);
        }
    }

    public synchronized void addEarlyResult(Outcome earlyFailure) {
        if (earlyFailure.getResult() == Result.UNSUPPORTED) {
            run.console.verbose("skipped " + earlyFailure.getName());
            skipped++;

        } else {
            for (String line : earlyFailure.getOutputLines()) {
                run.console.streamOutput(earlyFailure.getName(), line + "\n");
            }
            recordOutcome(earlyFailure);
        }
    }

    public synchronized void recordOutcome(Outcome outcome) {
        outcomes.put(outcome.getName(), outcome);
        Expectation expectation = run.expectationStore.get(outcome);
        ResultValue resultValue = outcome.getResultValue(expectation);

        if (resultValue == ResultValue.OK) {
            successes++;
        } else if (resultValue == ResultValue.FAIL) {
            failures++;
        } else if (resultValue == ResultValue.WARNING) {
            warnings++;
        } else { // ResultValue.IGNORE
            skipped++;
        }

        Result result = outcome.getResult();
        run.console.outcome(outcome.getName());
        run.console.printResult(outcome.getName(), result, resultValue, expectation);

        JarSuggestions singleOutcomeJarSuggestions = new JarSuggestions();
        singleOutcomeJarSuggestions.addSuggestionsFromOutcome(outcome, run.classFileIndex,
                run.classpath);
        List<String> jarStringList = singleOutcomeJarSuggestions.getStringList();
        if (!jarStringList.isEmpty()) {
            run.console.warn(
                    "may have failed because some of these jars are missing from the classpath:",
                    jarStringList);
        }
        run.jarSuggestions.addSuggestions(singleOutcomeJarSuggestions);
    }
}
