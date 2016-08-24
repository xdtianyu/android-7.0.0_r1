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

import com.google.common.collect.Lists;
import java.util.Collection;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import vogar.util.MarkResetConsole;

/**
 * Controls, formats and emits output to the command line. This class emits
 * output in two modes:
 * <ul>
 *   <li><strong>Streaming</strong> output prints as it is received, but cannot
 *       support multiple concurrent output streams.
 *   <li><strong>Multiplexing</strong> buffers output until it is complete and
 *       then prints it completely.
 * </ul>
 */
public abstract class Console implements Log {
    static final long DAY_MILLIS = 1000 * 60 * 60 * 24;
    static final long HOUR_MILLIS = 1000 * 60 * 60;
    static final long WARNING_HOURS = 12;
    static final long FAILURE_HOURS = 48;

    private boolean useColor;
    private boolean ansi;
    private boolean verbose;
    protected String indent;
    protected CurrentLine currentLine = CurrentLine.NEW;
    protected final MarkResetConsole out = new MarkResetConsole(System.out);
    protected MarkResetConsole.Mark currentVerboseMark;
    protected MarkResetConsole.Mark currentStreamMark;

    private Console() {}

    public void setIndent(String indent) {
        this.indent = indent;
    }

    public void setUseColor(
      boolean useColor, int passColor, int skipColor, int failColor, int warnColor) {
        this.useColor = useColor;
        Color.PASS.setCode(passColor);
        Color.SKIP.setCode(skipColor);
        Color.FAIL.setCode(failColor);
        Color.WARN.setCode(warnColor);
        Color.COMMENT.setCode(34);
    }

    public void setAnsi(boolean ansi) {
        this.ansi = ansi;
    }

    public void setVerbose(boolean verbose) {
        this.verbose = verbose;
    }

    public boolean isVerbose() {
        return verbose;
    }

    public synchronized void verbose(String s) {
        /*
         * terminal does't support overwriting output, so don't print
         * verbose message unless requested.
         */
        if (!verbose && !ansi) {
            return;
        }
        /*
         * When writing verbose output in the middle of streamed output, keep
         * the streamed mark location. That way we can remove the verbose output
         * later without losing our position mid-line in the streamed output.
         */
        MarkResetConsole.Mark savedStreamMark = currentLine == CurrentLine.STREAMED_OUTPUT
                ? out.mark()
                : currentStreamMark;
        newLine();
        currentStreamMark = savedStreamMark;

        currentVerboseMark = out.mark();
        out.print(s);
        currentLine = CurrentLine.VERBOSE;
    }

    public synchronized void warn(String message) {
        warn(message, Collections.<String>emptyList());
    }

    /**
     * Warns, and also puts a list of strings afterwards.
     */
    public synchronized void warn(String message, List<String> list) {
        newLine();
        out.println(colorString("Warning: " + message, Color.WARN));
        for (String item : list) {
            out.println(colorString(indent + item, Color.WARN));
        }
    }

    public synchronized void info(String s) {
        newLine();
        out.println(s);
    }

    public synchronized void info(String message, Throwable throwable) {
        newLine();
        out.println(message);
        throwable.printStackTrace(System.out);
    }

    /**
     * Begins streaming output for the named action.
     */
    public void action(String name) {}

    /**
     * Begins streaming output for the named outcome.
     */
    public void outcome(String name) {}

    /**
     * Appends the action output immediately to the stream when streaming is on,
     * or to a buffer when streaming is off. Buffered output will be held and
     * printed only if the outcome is unsuccessful.
     */
    public abstract void streamOutput(String outcomeName, String output);

    /**
     * Hook to flush anything streamed via {@link #streamOutput}.
     */
    protected void flushBufferedOutput(String outcomeName) {}

    /**
     * Writes the action's outcome.
     */
    public synchronized void printResult(
            String outcomeName, Result result, ResultValue resultValue, Expectation expectation) {
        // when the result is interesting, include the description and bug number
        if (result != Result.SUCCESS || resultValue != ResultValue.OK) {
            if (!expectation.getDescription().isEmpty()) {
                streamOutput(outcomeName, "\n" + colorString(expectation.getDescription(), Color.COMMENT));
            }
            if (expectation.getBug() != -1) {
                streamOutput(outcomeName, "\n" + colorString("http://b/" + expectation.getBug(), Color.COMMENT));
            }
        }

        flushBufferedOutput(outcomeName);

        if (currentLine == CurrentLine.NAME) {
            out.print(" ");
        } else {
            newLine(); // TODO: backup the cursor up to the name if there's no streaming output
            out.print(indent + outcomeName + " ");
        }

        if (resultValue == ResultValue.OK) {
            out.println(colorString("OK (" + result + ")", Color.PASS));
        } else if (resultValue == ResultValue.FAIL) {
            out.println(colorString("FAIL (" + result + ")", Color.FAIL));
        } else if (resultValue == ResultValue.IGNORE) {
            out.println(colorString("SKIP (" + result + ")", Color.WARN));
        }

        currentLine = CurrentLine.NEW;
    }

    public synchronized void summarizeOutcomes(Collection<AnnotatedOutcome> annotatedOutcomes) {
        List<AnnotatedOutcome> annotatedOutcomesSorted =
                AnnotatedOutcome.ORDER_BY_NAME.sortedCopy(annotatedOutcomes);

        List<String> failures = Lists.newArrayList();
        List<String> skips = Lists.newArrayList();
        List<String> successes = Lists.newArrayList();
        List<String> warnings = Lists.newArrayList();

        // figure out whether each outcome is noteworthy, and add a message to the appropriate list
        for (AnnotatedOutcome annotatedOutcome : annotatedOutcomesSorted) {
            if (!annotatedOutcome.isNoteworthy()) {
                continue;
            }

            Color color;
            List<String> list;
            ResultValue resultValue = annotatedOutcome.getResultValue();
            if (resultValue == ResultValue.OK) {
                color = Color.PASS;
                list = successes;
            } else if (resultValue == ResultValue.FAIL) {
                color = Color.FAIL;
                list = failures;
            } else if (resultValue == ResultValue.WARNING) {
                color = Color.WARN;
                list = warnings;
            } else {
                color = Color.SKIP;
                list = skips;
            }

            Long lastRun = annotatedOutcome.lastRun(null);
            String timestamp;
            if (lastRun == null) {
                timestamp = colorString("unknown", Color.WARN);
            } else {
                timestamp = formatElapsedTime(new Date().getTime() - lastRun);
            }

            String brokeThisMessage = "";
            ResultValue mostRecentResultValue = annotatedOutcome.getMostRecentResultValue(null);
            if (mostRecentResultValue != null && resultValue != mostRecentResultValue) {
                if (resultValue == ResultValue.OK) {
                    brokeThisMessage = colorString(" (you might have fixed this)", Color.WARN);
                } else {
                    brokeThisMessage = colorString(" (you might have broken this)", Color.WARN);
                }
            } else if (mostRecentResultValue == null) {
                brokeThisMessage = colorString(" (no test history available)", Color.WARN);
            }

            List<ResultValue> previousResultValues = annotatedOutcome.getPreviousResultValues();
            int numPreviousResultValues = previousResultValues.size();
            int numResultValuesToShow = Math.min(10, numPreviousResultValues);
            List<ResultValue> previousResultValuesToShow = previousResultValues.subList(
                    numPreviousResultValues - numResultValuesToShow, numPreviousResultValues);

            StringBuilder sb = new StringBuilder();
            sb.append(indent);
            sb.append(colorString(annotatedOutcome.getOutcome().getName(), color));
            if (!previousResultValuesToShow.isEmpty()) {
                sb.append(String.format(" [last %d: %s] [last run: %s]",
                        previousResultValuesToShow.size(),
                        generateSparkLine(previousResultValuesToShow),
                        timestamp));
            }
            sb.append(brokeThisMessage);
            list.add(sb.toString());
        }

        newLine();
        if (!successes.isEmpty()) {
            out.println("Success summary:");
            for (String success : successes) {
                out.println(success);
            }
        }
        if (!failures.isEmpty()) {
            out.println("Failure summary:");
            for (String failure : failures) {
                out.println(failure);
            }
        }
        if (!skips.isEmpty()) {
            out.println("Skips summary:");
            for (String skip : skips) {
                out.println(skip);
            }
        }
        if (!warnings.isEmpty()) {
            out.println("Warnings summary:");
            for (String warning : warnings) {
                out.println(warning);
            }
        }
    }

    private String formatElapsedTime(long elapsedTime) {
        if (elapsedTime < 0) {
            throw new IllegalArgumentException("non-negative elapsed times only");
        }

        String formatted;
        if (elapsedTime >= DAY_MILLIS) {
            long days = elapsedTime / DAY_MILLIS;
            formatted = String.format("%d days ago", days);
        } else if (elapsedTime >= HOUR_MILLIS) {
            long hours = elapsedTime / HOUR_MILLIS;
            formatted = String.format("%d hours ago", hours);
        } else {
            formatted = "less than an hour ago";
        }

        Color color = elapsedTimeWarningColor(elapsedTime);
        return colorString(formatted, color);
    }

    private Color elapsedTimeWarningColor(long elapsedTime) {
        if (elapsedTime < WARNING_HOURS * HOUR_MILLIS) {
            return Color.PASS;
        } else if (elapsedTime < FAILURE_HOURS * HOUR_MILLIS) {
            return Color.WARN;
        } else {
            return Color.FAIL;
        }
    }

    private String generateSparkLine(List<ResultValue> resultValues) {
        StringBuilder sb = new StringBuilder();
        for (ResultValue resultValue : resultValues) {
            if (resultValue == ResultValue.OK) {
                sb.append(colorString("\u2713", Color.PASS));
            } else if (resultValue == ResultValue.FAIL) {
                sb.append(colorString("X", Color.FAIL));
            } else {
                sb.append(colorString("-", Color.WARN));
            }
        }
        return sb.toString();
    }

    /**
     * Prints the action output with appropriate indentation.
     */
    public synchronized void streamOutput(CharSequence streamedOutput) {
        if (streamedOutput.length() == 0) {
            return;
        }

        String[] lines = messageToLines(streamedOutput.toString());

        if (currentLine == CurrentLine.VERBOSE && currentStreamMark != null && ansi) {
            currentStreamMark.reset();
            currentStreamMark = null;
        } else if (currentLine != CurrentLine.STREAMED_OUTPUT) {
            newLine();
            out.print(indent);
            out.print(indent);
        }
        out.print(lines[0]);
        currentLine = CurrentLine.STREAMED_OUTPUT;

        for (int i = 1; i < lines.length; i++) {
            newLine();

            if (lines[i].length() > 0) {
                out.print(indent);
                out.print(indent);
                out.print(lines[i]);
                currentLine = CurrentLine.STREAMED_OUTPUT;
            }
        }
    }

    /**
     * Inserts a linebreak if necessary.
     */
    protected void newLine() {
        currentStreamMark = null;

        if (currentLine == CurrentLine.VERBOSE && !verbose && ansi) {
            /*
             * Verbose means we leave all verbose output on the screen.
             * Otherwise we overwrite verbose output when new output arrives.
             */
            currentVerboseMark.reset();
        } else if (currentLine != CurrentLine.NEW) {
            out.print("\n");
        }

        currentLine = CurrentLine.NEW;
    }

    /**
     * Status of a currently-in-progress line of output.
     */
    enum CurrentLine {

        /**
         * The line is blank.
         */
        NEW,

        /**
         * The line contains streamed application output. Additional streamed
         * output may be appended without additional line separators or
         * indentation.
         */
        STREAMED_OUTPUT,

        /**
         * The line contains the name of an action or outcome. The outcome's
         * result (such as "OK") can be appended without additional line
         * separators or indentation.
         */
        NAME,

        /**
         * The line contains verbose output, and may be overwritten.
         */
        VERBOSE,
    }

    /**
     * Returns an array containing the lines of the given text.
     */
    private String[] messageToLines(String message) {
        // pass Integer.MAX_VALUE so split doesn't trim trailing empty strings.
        return message.split("\r\n|\r|\n", Integer.MAX_VALUE);
    }

    private enum Color {
        PASS, FAIL, SKIP, WARN, COMMENT;

        int code = 0;

        public int getCode() {
            return code;
        }

        public void setCode(int code) {
            this.code = code;
        }
    }

    protected String colorString(String message, Color color) {
        return useColor ? ("\u001b[" + color.getCode() + ";1m" + message + "\u001b[0m") : message;
    }

    /**
     * This console prints output as it's emitted. It supports at most one
     * action at a time.
     */
    static class StreamingConsole extends Console {
        private String currentName;

        @Override public synchronized void action(String name) {
            newLine();
            out.print("Action " + name);
            currentName = name;
            currentLine = CurrentLine.NAME;
        }

        /**
         * Prints the beginning of the named outcome.
         */
        @Override public synchronized void outcome(String name) {
            // if the outcome and action names are the same, omit the outcome name
            if (name.equals(currentName)) {
                return;
            }

            currentName = name;
            newLine();
            out.print(indent + name);
            currentLine = CurrentLine.NAME;
        }

        @Override public synchronized void streamOutput(String outcomeName, String output) {
            streamOutput(output);
        }
    }

    /**
     * This console buffers output, only printing when a result is found. It
     * supports multiple concurrent actions.
     */
    static class MultiplexingConsole extends Console {
        private final Map<String, StringBuilder> bufferedOutputByOutcome = new HashMap<String, StringBuilder>();

        @Override public synchronized void streamOutput(String outcomeName, String output) {
            StringBuilder buffer = bufferedOutputByOutcome.get(outcomeName);
            if (buffer == null) {
                buffer = new StringBuilder();
                bufferedOutputByOutcome.put(outcomeName, buffer);
            }

            buffer.append(output);
        }

        @Override protected synchronized void flushBufferedOutput(String outcomeName) {
            newLine();
            out.print(indent + outcomeName);
            currentLine = CurrentLine.NAME;

            StringBuilder buffer = bufferedOutputByOutcome.remove(outcomeName);
            if (buffer != null) {
                streamOutput(buffer);
            }
        }
    }
}
