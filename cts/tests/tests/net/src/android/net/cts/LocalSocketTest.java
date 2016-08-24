/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.net.cts;

import junit.framework.TestCase;

import android.net.Credentials;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;

import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class LocalSocketTest extends TestCase {
    private final static String ADDRESS_PREFIX = "com.android.net.LocalSocketTest";

    public void testLocalConnections() throws IOException {
        String address = ADDRESS_PREFIX + "_testLocalConnections";
        // create client and server socket
        LocalServerSocket localServerSocket = new LocalServerSocket(address);
        LocalSocket clientSocket = new LocalSocket();

        // establish connection between client and server
        LocalSocketAddress locSockAddr = new LocalSocketAddress(address);
        assertFalse(clientSocket.isConnected());
        clientSocket.connect(locSockAddr);
        assertTrue(clientSocket.isConnected());
        LocalSocket serverSocket = localServerSocket.accept();

        Credentials credent = clientSocket.getPeerCredentials();
        assertTrue(0 != credent.getPid());

        // send data from client to server
        OutputStream clientOutStream = clientSocket.getOutputStream();
        clientOutStream.write(12);
        InputStream serverInStream = serverSocket.getInputStream();
        assertEquals(12, serverInStream.read());

        //send data from server to client
        OutputStream serverOutStream = serverSocket.getOutputStream();
        serverOutStream.write(3);
        InputStream clientInStream = clientSocket.getInputStream();
        assertEquals(3, clientInStream.read());

        // Test sending and receiving file descriptors
        clientSocket.setFileDescriptorsForSend(new FileDescriptor[]{FileDescriptor.in});
        clientOutStream.write(32);
        assertEquals(32, serverInStream.read());

        FileDescriptor[] out = serverSocket.getAncillaryFileDescriptors();
        assertEquals(1, out.length);
        FileDescriptor fd = clientSocket.getFileDescriptor();
        assertTrue(fd.valid());

        //shutdown input stream of client
        clientSocket.shutdownInput();
        assertEquals(-1, clientInStream.read());

        //shutdown output stream of client
        clientSocket.shutdownOutput();
        try {
            clientOutStream.write(10);
            fail("testLocalSocket shouldn't come to here");
        } catch (IOException e) {
            // expected
        }

        //shutdown input stream of server
        serverSocket.shutdownInput();
        assertEquals(-1, serverInStream.read());

        //shutdown output stream of server
        serverSocket.shutdownOutput();
        try {
            serverOutStream.write(10);
            fail("testLocalSocket shouldn't come to here");
        } catch (IOException e) {
            // expected
        }

        //close client socket
        clientSocket.close();
        try {
            clientInStream.read();
            fail("testLocalSocket shouldn't come to here");
        } catch (IOException e) {
            // expected
        }

        //close server socket
        serverSocket.close();
        try {
            serverInStream.read();
            fail("testLocalSocket shouldn't come to here");
        } catch (IOException e) {
            // expected
        }
    }

    public void testAccessors() throws IOException {
        String address = ADDRESS_PREFIX + "_testAccessors";
        LocalSocket socket = new LocalSocket();
        LocalSocketAddress addr = new LocalSocketAddress(address);

        assertFalse(socket.isBound());
        socket.bind(addr);
        assertTrue(socket.isBound());
        assertEquals(addr, socket.getLocalSocketAddress());

        String str = socket.toString();
        assertTrue(str.contains("impl:android.net.LocalSocketImpl"));

        socket.setReceiveBufferSize(1999);
        assertEquals(1999 << 1, socket.getReceiveBufferSize());

        socket.setSendBufferSize(3998);
        assertEquals(3998 << 1, socket.getSendBufferSize());

        assertEquals(0, socket.getSoTimeout());
        socket.setSoTimeout(1996);
        assertTrue(socket.getSoTimeout() > 0);

        try {
            socket.getRemoteSocketAddress();
            fail("testLocalSocketSecondary shouldn't come to here");
        } catch (UnsupportedOperationException e) {
            // expected
        }

        try {
            socket.isClosed();
            fail("testLocalSocketSecondary shouldn't come to here");
        } catch (UnsupportedOperationException e) {
            // expected
        }

        try {
            socket.isInputShutdown();
            fail("testLocalSocketSecondary shouldn't come to here");
        } catch (UnsupportedOperationException e) {
            // expected
        }

        try {
            socket.isOutputShutdown();
            fail("testLocalSocketSecondary shouldn't come to here");
        } catch (UnsupportedOperationException e) {
            // expected
        }

        try {
            socket.connect(addr, 2005);
            fail("testLocalSocketSecondary shouldn't come to here");
        } catch (UnsupportedOperationException e) {
            // expected
        }

        socket.close();
    }

    public void testAvailable() throws Exception {
        String address = ADDRESS_PREFIX + "_testAvailable";
        LocalServerSocket localServerSocket = new LocalServerSocket(address);
        LocalSocket clientSocket = new LocalSocket();

        // establish connection between client and server
        LocalSocketAddress locSockAddr = new LocalSocketAddress(address);
        clientSocket.connect(locSockAddr);
        assertTrue(clientSocket.isConnected());
        LocalSocket serverSocket = localServerSocket.accept();

        OutputStream clientOutputStream = clientSocket.getOutputStream();
        InputStream serverInputStream = serverSocket.getInputStream();
        assertEquals(0, serverInputStream.available());

        byte[] buffer = new byte[50];
        clientOutputStream.write(buffer);
        assertEquals(50, serverInputStream.available());

        InputStream clientInputStream = clientSocket.getInputStream();
        OutputStream serverOutputStream = serverSocket.getOutputStream();
        assertEquals(0, clientInputStream.available());
        serverOutputStream.write(buffer);
        assertEquals(50, serverInputStream.available());

        clientSocket.close();
        serverSocket.close();
        localServerSocket.close();
    }

    public void testFlush() throws Exception {
        String address = ADDRESS_PREFIX + "_testFlush";
        LocalServerSocket localServerSocket = new LocalServerSocket(address);
        LocalSocket clientSocket = new LocalSocket();

        // establish connection between client and server
        LocalSocketAddress locSockAddr = new LocalSocketAddress(address);
        clientSocket.connect(locSockAddr);
        assertTrue(clientSocket.isConnected());
        LocalSocket serverSocket = localServerSocket.accept();

        OutputStream clientOutputStream = clientSocket.getOutputStream();
        InputStream serverInputStream = serverSocket.getInputStream();
        testFlushWorks(clientOutputStream, serverInputStream);

        OutputStream serverOutputStream = serverSocket.getOutputStream();
        InputStream clientInputStream = clientSocket.getInputStream();
        testFlushWorks(serverOutputStream, clientInputStream);

        clientSocket.close();
        serverSocket.close();
        localServerSocket.close();
    }

    private void testFlushWorks(OutputStream outputStream, InputStream inputStream)
            throws Exception {
        final int bytesToTransfer = 50;
        StreamReader inputStreamReader = new StreamReader(inputStream, bytesToTransfer);

        byte[] buffer = new byte[bytesToTransfer];
        outputStream.write(buffer);
        assertEquals(bytesToTransfer, inputStream.available());

        // Start consuming the data.
        inputStreamReader.start();

        // This doesn't actually flush any buffers, it just polls until the reader has read all the
        // bytes.
        outputStream.flush();

        inputStreamReader.waitForCompletion(5000);
        inputStreamReader.assertBytesRead(bytesToTransfer);
        assertEquals(0, inputStream.available());
    }

    private static class StreamReader extends Thread {
        private final InputStream is;
        private final int expectedByteCount;
        private final CountDownLatch completeLatch = new CountDownLatch(1);

        private volatile Exception exception;
        private int bytesRead;

        private StreamReader(InputStream is, int expectedByteCount) {
            this.is = is;
            this.expectedByteCount = expectedByteCount;
        }

        @Override
        public void run() {
            try {
                byte[] buffer = new byte[10];
                int readCount;
                while ((readCount = is.read(buffer)) >= 0) {
                    bytesRead += readCount;
                    if (bytesRead >= expectedByteCount) {
                        break;
                    }
                }
            } catch (IOException e) {
                exception = e;
            } finally {
                completeLatch.countDown();
            }
        }

        public void waitForCompletion(long waitMillis) throws Exception {
            if (!completeLatch.await(waitMillis, TimeUnit.MILLISECONDS)) {
                fail("Timeout waiting for completion");
            }
            if (exception != null) {
                throw new Exception("Read failed", exception);
            }
        }

        public void assertBytesRead(int expected) {
            assertEquals(expected, bytesRead);
        }
    }
}
