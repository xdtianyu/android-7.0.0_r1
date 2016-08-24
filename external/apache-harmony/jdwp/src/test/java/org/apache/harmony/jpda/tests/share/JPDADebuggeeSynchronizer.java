/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/**
 * @author Vitaly A. Provodin
 */

/**
 * Created on 29.01.2005
 */
package org.apache.harmony.jpda.tests.share;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.ServerSocket;
import java.net.UnknownHostException;

import org.apache.harmony.jpda.tests.framework.DebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.framework.LogWriter;
import org.apache.harmony.jpda.tests.framework.TestErrorException;
import org.apache.harmony.jpda.tests.framework.TestOptions;

/**
 * This class implements <code>DebuggeeSynchronizer</code> interface using
 * TCP/IP sockets. All operations can be timed out according to default timeout.
 */
public class JPDADebuggeeSynchronizer implements DebuggeeSynchronizer {

    public final static String SGNL_READY = "ready";

    public final static String SGNL_CONTINUE = "continue";

    protected Socket clientSocket = null;

    protected ServerSocket serverSocket = null;

    protected DataOutputStream out;

    protected DataInputStream in;

    protected TestOptions settings;

    protected LogWriter logWriter;

    /**
     * A constructor that initializes an instance of the class with specified
     * <code>LogWriter</code> and <code>TestOptions</code>.
     * 
     * @param logWriter
     *            The instance of implementation of LogWriter.
     * @param settings
     *            Instance of test options.
     */
    public JPDADebuggeeSynchronizer(LogWriter logWriter, TestOptions settings) {
        super();
        this.logWriter = logWriter;
        this.settings = settings;
    }

    /**
     * Sends specified message to synchronization channel.
     * 
     * @param message
     *            a message to be sent.
     */
    public synchronized void sendMessage(String message) {
        try {
            out.writeUTF(message);
            out.flush();
            logWriter.println("[SYNC] Message sent: " + message);
        } catch (IOException e) {
            throw new TestErrorException(e);
        }
    }

    /**
     * Receives message from synchronization channel and compares it with the
     * expected <code>message</code>.
     * 
     * @param message
     *            expected message.
     * @return <code>true</code> if received string is equals to
     *         <code>message</code> otherwise - <code>false</code>.
     * 
     */
    public synchronized boolean receiveMessage(String message) {
        String msg;
        try {
            logWriter.println("[SYNC] Waiting for message: " + message);
            msg = in.readUTF();
            logWriter.println("[SYNC] Received message: " + msg);
        } catch (EOFException e) {
            return false;
        } catch (IOException e) {
            logWriter.printError(e);
            return false;
        }
        return message.equalsIgnoreCase(msg);
    }

    /**
     * Receives message from synchronization channel.
     * 
     * @return received string or null if connection was closed.
     */
    public synchronized String receiveMessage() {
        String msg;
        try {
            logWriter.println("[SYNC] Waiting for any messsage");
            msg = in.readUTF();
            logWriter.println("[SYNC] Received message: " + msg);
        } catch (EOFException e) {
            return null;
        } catch (IOException e) {
            throw new TestErrorException(e);
        }
        return msg;
    }

    /**
     * Receives message from synchronization channel without Exception.
     * 
     * @return received string
     */
    public synchronized String receiveMessageWithoutException(String invoker) {
        String msg;
        try {
            logWriter.println("[SYNC] Waiting for any message");
            msg = in.readUTF();
            logWriter.println("[SYNC] Received message: " + msg);
        } catch (Throwable thrown) {
            if (invoker != null) {
                logWriter.println("#### receiveMessageWithoutException: Exception occurred:");
                logWriter.println("#### " + thrown);
                logWriter.println("#### Invoker = " + invoker);
            }
            msg = "" + thrown;
        }
        return msg;
    }

    /**
     * Returns socket address for connecting to the server.
     * 
     * If <code>serverAddress.getPort()</code> returns 0 (i.e.,
     * <code>org.apache.harmony.jpda.tests.framework.TestOptions.DEFAULT_SYNC_PORT</code>),
     * a port will automatically be chosen by the OS when the server is bound to a socket.
     * 
     * @return socket address
     */
    public InetSocketAddress getSyncServerAddress() {
        // Use the LOOPBACK directly instead of doing a DNS lookup.
        int port = settings.getSyncPortNumber();
        try {
            // Use IPv4 to ensure we do not depend on IPv6 to run these tests.
            // TODO(25178637): Use InetAddress.getLoopbackAddress() instead.
            return new InetSocketAddress(
                InetAddress.getByAddress(new byte[] { 127, 0, 0, 1 }), port);
        } catch (UnknownHostException e) {
            throw new TestErrorException(
                    "[SYNC] Exception in binding for socket sync connection", e);
        }
    }

    /**
     * Binds server to listen socket port.
     * 
     * If <code>serverAddress.getPort()</code> returns 0 (i.e.,
     * <code>org.apache.harmony.jpda.tests.framework.TestOptions.DEFAULT_SYNC_PORT</code>),
     * the OS will choose a port automatically for this server socket.
     * 
     * @return port number
     */
    public synchronized int bindServer() {
        InetSocketAddress serverAddress = getSyncServerAddress();
        try {
            logWriter.println("[SYNC] Binding socket on: " + serverAddress);
            serverSocket = new ServerSocket(serverAddress.getPort(), 0, serverAddress.getAddress());
            int localPort = serverSocket.getLocalPort();
            logWriter.println("[SYNC] Bound socket on: " + serverAddress
                    + " (local port: " + localPort + ")" );
            return localPort;
        } catch (IOException e) {
            throw new TestErrorException(
                    "[SYNC] Exception in binding for socket sync connection", e);
        }
    }

    /**
     * Accepts sync connection form server side.
     */
    public synchronized void startServer() {
        long timeout = settings.getTimeout();
        try {
            serverSocket.setSoTimeout((int) timeout);
            logWriter.println("[SYNC] Accepting socket connection");
            clientSocket = serverSocket.accept();
            logWriter.println("[SYNC] Accepted socket connection");

            clientSocket.setSoTimeout((int) timeout);
            out = new DataOutputStream(clientSocket.getOutputStream());
            in = new DataInputStream(clientSocket.getInputStream());
        } catch (IOException e) {
            throw new TestErrorException(
                    "[SYNC] Exception in accepting socket sync connection", e);
        }
    }

    /**
     * Attaches for sync connection from client side..
     */
    public synchronized void startClient() {
        long timeout = settings.getTimeout();
        InetSocketAddress serverAddress = getSyncServerAddress();
        try {
            logWriter.println("[SYNC] Attaching socket to: " + serverAddress);
            clientSocket = new Socket(serverAddress.getAddress(), serverAddress.getPort());
            logWriter.println("[SYNC] Attached socket");

            clientSocket.setSoTimeout((int) timeout);
            out = new DataOutputStream(clientSocket.getOutputStream());
            in = new DataInputStream(clientSocket.getInputStream());
        } catch (IOException e) {
            throw new TestErrorException(
                    "[SYNC] Exception in attaching for socket sync connection", e);
        }
    }

    /**
     * Stops synchronization work. It ignores <code>IOException</code> but
     * prints a message.
     */
    public void stop() {
        try {
            if (out != null)
                out.close();
            if (in != null)
                in.close();
            if (clientSocket != null)
                clientSocket.close();
            if (serverSocket != null)
                serverSocket.close();
        } catch (IOException e) {
            logWriter.println
                    ("[SYNC] Ignoring exception in closing socket sync connection: " + e);
        }
        logWriter.println("[SYNC] Closed socket");
    }
}
