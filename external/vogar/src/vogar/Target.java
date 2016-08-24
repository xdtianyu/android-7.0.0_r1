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

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Joiner;
import com.google.common.collect.ImmutableList;
import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import vogar.tasks.Task;

/**
 * A target runtime environment such as a remote device or the local host
 */
public abstract class Target {
    protected abstract ImmutableList<String> targetProcessPrefix();
    public abstract String getDeviceUserName();

    public abstract List<File> ls(File directory) throws FileNotFoundException;
    public abstract void await(File nonEmptyDirectory);
    public abstract void rm(File file);
    public abstract void mkdirs(File file);
    public abstract void forwardTcp(int port);
    public abstract void push(File local, File remote);
    public abstract void pull(File remote, File local);

    public final Task pushTask(final File local, final File remote) {
        return new Task("push " + remote) {
            @Override protected Result execute() throws Exception {
                push(local, remote);
                return Result.SUCCESS;
            }
        };
    }

    public final Task rmTask(final File remote) {
        return new Task("rm " + remote) {
            @Override protected Result execute() throws Exception {
                rm(remote);
                return Result.SUCCESS;
            }
        };
    }

    /**
     * Create a {@link ScriptBuilder} appropriate for this target.
     */
    public ScriptBuilder newScriptBuilder() {
        return new ScriptBuilder(targetProcessPrefix());
    }

    /**
     * Responsible for constructing a one line script appropriate for a specific target.
     */
    public static class ScriptBuilder {

        private static final Joiner SCRIPT_JOINER = Joiner.on(" ");

        /**
         * Escape any special shell characters so that the target shell treats them as literal
         * characters.
         *
         * <p>e.g. an escaped space will not be treated as a token separator, an escaped dollar will
         * not cause shell substitution.
         */
        @VisibleForTesting
        static String escape(String token) {
            int length = token.length();
            StringBuilder builder = new StringBuilder(length);
            for (int i = 0; i < length; ++i) {
                char c = token.charAt(i);
                if (Character.isWhitespace(c)
                        || c == '\'' || c == '\"'
                        || c == '|' || c == '&'
                        || c == '<' || c == '>'
                        || c == '$' || c == '!'
                        || c == '(' || c == ')') {
                    builder.append('\\');
                }

                builder.append(c);
            }

            return builder.toString();
        }

        /**
         * The prefix to insert before the script to produce a command line that will execute the
         * script within a shell.
         */
        private final ImmutableList<String> commandLinePrefix;

        /**
         * The list of tokens making up the script, they were escaped where necessary before they
         * were added to the list.
         */
        private final List<String> escapedTokens;

        private ScriptBuilder(ImmutableList<String> commandLinePrefix) {
            this.commandLinePrefix = commandLinePrefix;
            escapedTokens = new ArrayList<>();
        }

        /**
         * Set the working directory in the target shell before running the command.
         */
        public ScriptBuilder workingDirectory(File workingDirectory) {
            escapedTokens.add("cd");
            escapedTokens.add(escape(workingDirectory.getPath()));
            escapedTokens.add("&&");
            return this;
        }

        /**
         * Set inline environment variables on the target shell that will affect the command.
         */
        public void env(Map<String, String> env) {
            for (Map.Entry<String, String> entry : env.entrySet()) {
                String name = entry.getKey();
                String value = entry.getValue();
                escapedTokens.add(name + "=" + escape(value));
            }
        }

        /**
         * Add tokens to the script.
         *
         * <p>Each token is escaped before adding it to the list. This method is suitable for adding
         * the command line name and arguments.
         */
        public ScriptBuilder tokens(List<String> tokens) {
            for (String token : tokens) {
                escapedTokens.add(escape(token));
            }
            return this;
        }

        /**
         * Add tokens to the script.
         *
         * <p>Each token is escaped before adding it to the list. This method is suitable for adding
         * the command line name and arguments.
         */
        public ScriptBuilder tokens(String... tokens) {
            for (String token : tokens) {
                escapedTokens.add(escape(token));
            }
            return this;
        }

        /**
         * Construct a command line to execute the script in the target shell.
         */
        public List<String> commandLine() {
            // Group the tokens into a single String argument. This is necessary for running in
            // a local shell where the first argument after the -c option is the script and the
            // remainder are treated as arguments to the script. This has no effect on either
            // adb or ssh shells as they both concatenate all their arguments into one single
            // string before parsing.
            String grouped = SCRIPT_JOINER.join(escapedTokens);
            return new ImmutableList.Builder<String>()
                    .addAll(commandLinePrefix)
                    .add(grouped)
                    .build();
        }
    }
}
