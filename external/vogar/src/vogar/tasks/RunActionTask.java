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

import java.io.File;
import java.io.IOException;
import vogar.Action;
import vogar.Classpath;
import vogar.Outcome;
import vogar.Result;
import vogar.Run;
import vogar.commands.Command;
import vogar.commands.VmCommandBuilder;
import vogar.monitor.HostMonitor;
import vogar.target.CaliperRunner;
import vogar.target.TestRunner;

/**
 * Executes a single action and then prints the result.
 */
public class RunActionTask extends Task implements HostMonitor.Handler {
    /**
     * Assign each runner thread a unique ID. Necessary so threads don't
     * conflict when selecting a monitor port.
     */
    private final ThreadLocal<Integer> runnerThreadId = new ThreadLocal<Integer>() {
        private int next = 0;
        @Override protected synchronized Integer initialValue() {
            return next++;
        }
    };

    protected final Run run;
    private final boolean useLargeTimeout;
    private final Action action;
    private final String actionName;
    private Command currentCommand;
    private String lastStartedOutcome;
    private String lastFinishedOutcome;

    public RunActionTask(Run run, Action action, boolean useLargeTimeout) {
        super("run " + action.getName());
        this.run = run;
        this.action = action;
        this.actionName = action.getName();
        this.useLargeTimeout = useLargeTimeout;
    }

    @Override public boolean isAction() {
        return true;
    }

    @Override protected Result execute() throws Exception {
        run.console.action(actionName);

        while (true) {
            /*
             * If the target process failed midway through a set of
             * outcomes, that's okay. We pickup right after the first
             * outcome that wasn't completed.
             */
            String skipPast = lastStartedOutcome;
            lastStartedOutcome = null;

            currentCommand = createActionCommand(action, skipPast, monitorPort(-1));
            try {
                currentCommand.start();

                int timeoutSeconds = useLargeTimeout
                        ? run.largeTimeoutSeconds
                        : run.smallTimeoutSeconds;
                if (timeoutSeconds != 0) {
                    currentCommand.scheduleTimeout(timeoutSeconds);
                }

                HostMonitor hostMonitor = new HostMonitor(run.console, this);
                boolean completedNormally = useSocketMonitor()
                        ? hostMonitor.attach(monitorPort(run.firstMonitorPort))
                        : hostMonitor.followStream(currentCommand.getInputStream());

                if (completedNormally) {
                    return Result.SUCCESS;
                }

                String earlyResultOutcome;
                boolean giveUp;

                if (lastStartedOutcome == null || lastStartedOutcome.equals(actionName)) {
                    earlyResultOutcome = actionName;
                    giveUp = true;
                } else if (!lastStartedOutcome.equals(lastFinishedOutcome)) {
                    earlyResultOutcome = lastStartedOutcome;
                    giveUp = false;
                } else {
                    continue;
                }

                run.driver.addEarlyResult(new Outcome(earlyResultOutcome, Result.ERROR,
                        "Action " + action + " did not complete normally.\n"
                                + "timedOut=" + currentCommand.timedOut() + "\n"
                                + "lastStartedOutcome=" + lastStartedOutcome + "\n"
                                + "lastFinishedOutcome=" + lastFinishedOutcome + "\n"
                                + "command=" + currentCommand));

                if (giveUp) {
                    return Result.ERROR;
                }
            } catch (IOException e) {
                // if the monitor breaks, assume the worst and don't retry
                run.driver.addEarlyResult(new Outcome(actionName, Result.ERROR, e));
                return Result.ERROR;
            } finally {
                currentCommand.destroy();
                currentCommand = null;
            }
        }
    }

    /**
     * Create the command that executes the action.
     *
     * @param skipPast the last outcome to skip, or null to run all outcomes.
     * @param monitorPort the port to accept connections on, or -1 for the
     */
    public Command createActionCommand(Action action, String skipPast, int monitorPort) {
        File workingDirectory = action.getUserDir();
        VmCommandBuilder vmCommandBuilder = run.mode.newVmCommandBuilder(action, workingDirectory);
        Classpath runtimeClasspath = run.mode.getRuntimeClasspath(action);
        if (run.useBootClasspath) {
            vmCommandBuilder.bootClasspath(runtimeClasspath);
        } else {
            vmCommandBuilder.classpath(runtimeClasspath);
        }
        if (monitorPort != -1) {
            vmCommandBuilder.args("--monitorPort", Integer.toString(monitorPort));
        }
        if (skipPast != null) {
            vmCommandBuilder.args("--skipPast", skipPast);
        }
        return vmCommandBuilder
                .temp(workingDirectory)
                .debugPort(run.debugPort)
                .vmArgs(run.additionalVmArgs)
                .mainClass(TestRunner.class.getName())
                .args(run.targetArgs)
                .build(run.target);
    }

    /**
     * Returns true if this mode requires a socket connection for reading test
     * results. Otherwise all communication happens over the output stream of
     * the forked process.
     */
    protected boolean useSocketMonitor() {
        return false;
    }

    private int monitorPort(int defaultValue) {
        return run.maxConcurrentActions == 1
                ? defaultValue
                : run.firstMonitorPort + runnerThreadId.get();
    }

    @Override public void start(String outcomeName, String runnerClass) {
        outcomeName = toQualifiedOutcomeName(outcomeName);
        lastStartedOutcome = outcomeName;
        // TODO add to Outcome knowledge about what class was used to run it
        if (CaliperRunner.class.getName().equals(runnerClass)) {
            if (!run.benchmark) {
                throw new RuntimeException("you must use --benchmark when running Caliper "
                        + "benchmarks.");
            }
            run.console.verbose("running " + outcomeName + " with unlimited timeout");
            Command command = currentCommand;
            if (command != null && run.smallTimeoutSeconds != 0) {
                command.scheduleTimeout(run.smallTimeoutSeconds);
            }
            run.driver.recordResults = false;
        } else {
            run.driver.recordResults = true;
        }
    }

    @Override public void output(String outcomeName, String output) {
        outcomeName = toQualifiedOutcomeName(outcomeName);
        run.console.outcome(outcomeName);
        run.console.streamOutput(outcomeName, output);
    }

    @Override public void finish(Outcome outcome) {
        Command command = currentCommand;
        if (command != null && run.smallTimeoutSeconds != 0) {
            command.scheduleTimeout(run.smallTimeoutSeconds);
        }
        lastFinishedOutcome = toQualifiedOutcomeName(outcome.getName());
        // TODO: support flexible timeouts for JUnit tests
        run.driver.recordOutcome(new Outcome(lastFinishedOutcome, outcome.getResult(),
                outcome.getOutputLines()));
    }

    /**
     * Test suites that use main classes in the default package have lame
     * outcome names like "Clear" rather than "com.foo.Bar.Clear". In that
     * case, just replace the outcome name with the action name.
     */
    private String toQualifiedOutcomeName(String outcomeName) {
        if (actionName.endsWith("." + outcomeName)
                && !outcomeName.contains(".") && !outcomeName.contains("#")) {
            return actionName;
        } else {
            return outcomeName;
        }
    }

    @Override public void print(String string) {
        run.console.streamOutput(string);
    }
}
