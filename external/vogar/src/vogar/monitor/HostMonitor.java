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

package vogar.monitor;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.ConnectException;
import java.net.Socket;
import java.net.SocketException;
import java.nio.charset.Charset;
import vogar.Log;
import vogar.Outcome;
import vogar.Result;
import vogar.util.IoUtils;

/**
 * Connects to a target process to monitor its action using XML over raw
 * sockets.
 */
public final class HostMonitor {
    private static final Charset UTF8 = Charset.forName("UTF-8");

    private Log log;
    private Handler handler;
    private final String marker = "//00xx";

    public HostMonitor(Log log, Handler handler) {
        this.log = log;
        this.handler = handler;
    }

    /**
     * Returns true if the target process completed normally.
     */
    public boolean attach(int port) throws IOException {
        for (int attempt = 0; true; attempt++) {
            Socket socket = null;
            try {
                socket = new Socket("localhost", port);
                InputStream in = new BufferedInputStream(socket.getInputStream());
                if (checkStream(in)) {
                    log.verbose("action monitor connected to " + socket.getRemoteSocketAddress());
                    return followStream(in);
                }
            } catch (ConnectException ignored) {
            } catch (SocketException ignored) {
            } finally {
                IoUtils.closeQuietly(socket);
            }

            log.verbose("connection " + attempt + " to localhost:"
                    + port + " failed; retrying in 1s");
            try {
                Thread.sleep(1000);
            } catch (InterruptedException ignored) {
            }
        }
    }

    /**
     * Somewhere between the host and client process, broken socket connections
     * are being accepted. Before we try to do any work on such a connection,
     * check it to make sure it's not dead!
     *
     * TODO: file a bug (against adb?) for this
     */
    private boolean checkStream(InputStream in) throws IOException {
        in.mark(1);
        if (in.read() == -1) {
            return false;
        } else {
            in.reset();
            return true;
        }
    }

    public boolean followStream(InputStream in) throws IOException {
        return followProcess(new InterleavedReader(marker, new InputStreamReader(in, UTF8)));
    }

    /**
     * Our wire format is a mix of strings and the JSON values like the following:
     *
     * {"outcome"="java.util.FormatterMain"}
     * {"result"="SUCCESS"}
     * {"outcome"="java.util.FormatterTest#testBar" runner="vogar.target.junit.JUnitRunner"}
     * {"result"="SUCCESS"}
     * {"completedNormally"=true}
     */
    private boolean followProcess(InterleavedReader reader) throws IOException {
        String currentOutcome = null;
        StringBuilder output = new StringBuilder();
        boolean completedNormally = false;

        Object o;
        while ((o = reader.read()) != null) {
            if (o instanceof String) {
                String text = (String) o;
                if (currentOutcome != null) {
                    output.append(text);
                    handler.output(currentOutcome, text);
                } else {
                    handler.print(text);
                }
            } else if (o instanceof JsonObject) {
                JsonObject jsonObject = (JsonObject) o;
                if (jsonObject.get("outcome") != null) {
                    currentOutcome = jsonObject.get("outcome").getAsString();
                    handler.output(currentOutcome, "");
                    JsonElement runner = jsonObject.get("runner");
                    String runnerClass = runner != null ? runner.getAsString() : null;
                    handler.start(currentOutcome, runnerClass);
                } else if (jsonObject.get("result") != null) {
                    Result currentResult = Result.valueOf(jsonObject.get("result").getAsString());
                    handler.finish(new Outcome(currentOutcome, currentResult, output.toString()));
                    output.delete(0, output.length());
                    currentOutcome = null;
                } else if (jsonObject.get("completedNormally") != null) {
                    completedNormally = jsonObject.get("completedNormally").getAsBoolean();
                }
            } else {
                throw new IllegalStateException("Unexpected object: " + o);
            }
        }

        return completedNormally;
    }


    /**
     * Handles updates on the outcomes of a target process.
     */
    public interface Handler {

        /**
         * @param runnerClass can be null, indicating nothing is actually being run. This will
         *        happen in the event of an impending error.
         */
        void start(String outcomeName, String runnerClass);

        /**
         * Receive a completed outcome.
         */
        void finish(Outcome outcome);

        /**
         * Receive partial output from an action being executed.
         */
        void output(String outcomeName, String output);

        /**
         * Receive a string to print immediately
         */
        void print(String string);
    }
}
