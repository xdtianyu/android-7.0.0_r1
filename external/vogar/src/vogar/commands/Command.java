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

package vogar.commands;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.collect.ImmutableList;
import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import vogar.Log;
import vogar.util.Strings;

/**
 * An out of process executable.
 */
public final class Command {
    private static final ScheduledExecutorService timer
            = Executors.newSingleThreadScheduledExecutor();

    private final Log log;
    private final File workingDir;
    private final List<String> args;
    private final Map<String, String> env;
    private final boolean permitNonZeroExitStatus;
    private final PrintStream tee;

    private volatile Process process;
    private volatile boolean destroyed;
    private volatile long timeoutNanoTime;

    public Command(Log log, String... args) {
        this.log = log;
        this.workingDir = null;
        this.args = ImmutableList.copyOf(args);
        this.env = Collections.emptyMap();
        this.permitNonZeroExitStatus = false;
        this.tee = null;
    }

    private Command(Builder builder) {
        this.log = builder.log;
        this.workingDir = builder.workingDir;
        this.args = ImmutableList.copyOf(builder.args);
        this.env = builder.env;
        this.permitNonZeroExitStatus = builder.permitNonZeroExitStatus;
        this.tee = builder.tee;
        if (builder.maxLength != -1) {
            String string = toString();
            if (string.length() > builder.maxLength) {
                throw new IllegalStateException("Maximum command length " + builder.maxLength
                                                + " exceeded by: " + string);
            }
        }
    }

    public void start() throws IOException {
        if (isStarted()) {
            throw new IllegalStateException("Already started!");
        }

        log.verbose("executing " + args + (workingDir != null ? " in " + workingDir : ""));

        ProcessBuilder processBuilder = new ProcessBuilder()
                .directory(workingDir)
                .command(args)
                .redirectErrorStream(true);

        processBuilder.environment().putAll(env);

        process = processBuilder.start();
    }

    public boolean isStarted() {
        return process != null;
    }

    public InputStream getInputStream() {
        if (!isStarted()) {
            throw new IllegalStateException("Not started!");
        }

        return process.getInputStream();
    }

    public List<String> gatherOutput()
            throws IOException, InterruptedException {
        if (!isStarted()) {
            throw new IllegalStateException("Not started!");
        }

        BufferedReader in = new BufferedReader(
                new InputStreamReader(getInputStream(), "UTF-8"));
        List<String> outputLines = new ArrayList<String>();
        String outputLine;
        while ((outputLine = in.readLine()) != null) {
            if (tee != null) {
                tee.println(outputLine);
            }
            outputLines.add(outputLine);
        }

        int exitValue = process.waitFor();
        destroyed = true;
        if (exitValue != 0 && !permitNonZeroExitStatus) {
            throw new CommandFailedException(args, outputLines);
        }

        return outputLines;
    }

    public List<String> execute() {
        try {
            start();
            return gatherOutput();
        } catch (IOException e) {
            throw new RuntimeException("Failed to execute process: " + args, e);
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted while executing process: " + args, e);
        }
    }

    /**
     * Executes a command with a specified timeout. If the process does not
     * complete normally before the timeout has elapsed, it will be destroyed.
     *
     * @param timeoutSeconds how long to wait, or 0 to wait indefinitely
     * @return the command's output, or null if the command timed out
     */
    public List<String> executeWithTimeout(int timeoutSeconds) throws TimeoutException {
        if (timeoutSeconds == 0) {
            return execute();
        }

        scheduleTimeout(timeoutSeconds);
        return execute();
    }

    /**
     * Destroys the underlying process and closes its associated streams.
     */
    public void destroy() {
        Process process = this.process;
        if (process == null) {
            throw new IllegalStateException();
        }
        if (destroyed) {
            return;
        }

        destroyed = true;
        process.destroy();
        try {
            process.waitFor();
            int exitValue = process.exitValue();
            log.verbose("received exit value " + exitValue + " from destroyed command " + this);
        } catch (IllegalThreadStateException | InterruptedException destroyUnsuccessful) {
            log.warn("couldn't destroy " + this);
        }
    }

    @Override public String toString() {
        String envString = !env.isEmpty() ? (Strings.join(env.entrySet(), " ") + " ") : "";
        return envString + Strings.join(args, " ");
    }

    /**
     * Sets the time at which this process will be killed. If a timeout has
     * already been scheduled, it will be rescheduled.
     */
    public void scheduleTimeout(int timeoutSeconds) {
        timeoutNanoTime = System.nanoTime() + TimeUnit.SECONDS.toNanos(timeoutSeconds);

        new TimeoutTask() {
            @Override protected void onTimeout(Process process) {
                // send a quit signal immediately
                log.verbose("sending quit signal to command " + Command.this);
                sendQuitSignal(process);

                // hard kill in 2 seconds
                timeoutNanoTime = System.nanoTime() + TimeUnit.SECONDS.toNanos(2);
                new TimeoutTask() {
                    @Override protected void onTimeout(Process process) {
                        log.verbose("killing timed out command " + Command.this);
                        destroy();
                    }
                }.schedule();
            }
        }.schedule();
    }

    private void sendQuitSignal(Process process) {
        // TODO: 'adb shell kill' to kill on processes running on Androids
        new Command(log, "kill", "-3", Integer.toString(getPid(process))).execute();
    }

    /**
     * Return the PID of this command's process.
     */
    private int getPid(Process process) {
        try {
            // See org.openqa.selenium.ProcessUtils.getProcessId()
            Field field = process.getClass().getDeclaredField("pid");
            field.setAccessible(true);
            return (Integer) field.get(process);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    public boolean timedOut() {
        return System.nanoTime() >= timeoutNanoTime;
    }

    @VisibleForTesting
    public List<String> getArgs() {
        return args;
    }

    public static class Builder {
        private final Log log;
        private final List<String> args = new ArrayList<String>();
        private final Map<String, String> env = new LinkedHashMap<String, String>();
        private boolean permitNonZeroExitStatus = false;
        private PrintStream tee = null;
        private int maxLength = -1;
        private File workingDir;

        public Builder(Log log) {
            this.log = log;
        }

        public Builder(Builder other) {
            this.log = other.log;
            this.workingDir = other.workingDir;
            this.args.addAll(other.args);
            this.env.putAll(other.env);
            this.permitNonZeroExitStatus = other.permitNonZeroExitStatus;
            this.tee = other.tee;
            this.maxLength = other.maxLength;
        }

        public Builder args(Object... args) {
            return args(Arrays.asList(args));
        }

        public Builder args(Collection<?> args) {
            for (Object object : args) {
                this.args.add(object.toString());
            }
            return this;
        }

        public Builder env(String key, String value) {
            env.put(key, value);
            return this;
        }

        /**
         * Controls whether execute() throws if the invoked process returns a
         * nonzero exit code.
         */
        public Builder permitNonZeroExitStatus(boolean value) {
            this.permitNonZeroExitStatus = value;
            return this;
        }

        public Builder tee(PrintStream printStream) {
            tee = printStream;
            return this;
        }

        public Builder maxLength(int maxLength) {
            this.maxLength = maxLength;
            return this;
        }

        public Builder workingDir(File workingDir) {
            this.workingDir = workingDir;
            return this;
        }

        public Command build() {
            return new Command(this);
        }

        public List<String> execute() {
            return build().execute();
        }
    }

    /**
     * Runs some code when the command times out.
     */
    private abstract class TimeoutTask implements Runnable {
        public final void schedule() {
            timer.schedule(this, System.nanoTime() - timeoutNanoTime, TimeUnit.NANOSECONDS);
        }

        protected abstract void onTimeout(Process process);

        @Override public final void run() {
            // don't destroy commands that have already been destroyed
            Process process = Command.this.process;
            if (destroyed) {
                return;
            }

            if (timedOut()) {
                onTimeout(process);
            } else {
                // if the kill time has been pushed back, reschedule
                timer.schedule(this, System.nanoTime() - timeoutNanoTime, TimeUnit.NANOSECONDS);
            }
        }
    }
}
