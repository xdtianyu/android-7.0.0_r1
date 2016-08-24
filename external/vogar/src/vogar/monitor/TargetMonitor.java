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

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import java.io.IOException;
import java.io.PrintStream;
import java.net.ServerSocket;
import java.net.Socket;
import vogar.Result;
import vogar.target.Runner;

/**
 * Accepts a connection from the host process. Once connected, XML is sent over
 * raw sockets.
 */
public class TargetMonitor {

    private static final int ACCEPT_TIMEOUT_MILLIS = 10 * 1000;

    private final Gson gson = new Gson();
    private final String marker = "//00xx";

    private final PrintStream writer;

    private TargetMonitor(PrintStream writer) {
        this.writer = writer;
    }

    public static TargetMonitor forPrintStream(PrintStream printStream) {
        return new TargetMonitor(printStream);
    }

    public static TargetMonitor await(int port) {
        try {
            final ServerSocket serverSocket = new ServerSocket(port);
            serverSocket.setSoTimeout(ACCEPT_TIMEOUT_MILLIS);
            serverSocket.setReuseAddress(true);
            final Socket socket = serverSocket.accept();
            return new TargetMonitor(new PrintStream(socket.getOutputStream())) {
                @Override public void close() throws IOException {
                    socket.close();
                    serverSocket.close();
                }
            };

        } catch (IOException e) {
            throw new RuntimeException("Failed to accept a monitor on localhost:" + port, e);
        }
    }

    public void outcomeStarted(Class<? extends Runner> runnerClass, String outcomeName) {
        JsonObject jsonObject = new JsonObject();
        jsonObject.addProperty("outcome", outcomeName);
        if (runnerClass != null) {
            jsonObject.addProperty("runner", runnerClass.getName());
        }
        writer.print(marker + gson.toJson(jsonObject) + "\n");
    }

    public void output(String text) {
        writer.print(text);
    }

    public void outcomeFinished(Result result) {
        JsonObject jsonObject = new JsonObject();
        jsonObject.addProperty("result", result.name());
        writer.print(marker + gson.toJson(jsonObject) + "\n");
    }

    public synchronized void close() throws IOException {
        writer.close();
    }

    public void completedNormally(boolean completedNormally) {
        JsonObject jsonObject = new JsonObject();
        jsonObject.addProperty("completedNormally", completedNormally);
        writer.print(marker + gson.toJson(jsonObject) + "\n");
    }
}
