package com.android.bluetooth.sap;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import org.android.btsap.SapApi.MsgHeader;

import com.google.protobuf.micro.CodedInputStreamMicro;
import com.google.protobuf.micro.CodedOutputStreamMicro;

import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class SapRilReceiver implements Runnable {

    private static final String TAG = "SapRilReceiver";
    public static final boolean DEBUG = true;
    public static final boolean VERBOSE = true;

    private static final String SOCKET_NAME_RIL_BT = "sap_uim_socket1";
    // match with constant in ril.cpp - as in RIL.java
    private static final int SOCKET_OPEN_RETRY_MILLIS = 4 * 1000;

    LocalSocket mSocket = null;
    CodedOutputStreamMicro mRilBtOutStream = null;
    InputStream mRilBtInStream = null;
    private Handler mSapServerMsgHandler = null;
    private Handler mSapServiceHandler = null;

    public static final int RIL_MAX_COMMAND_BYTES = (8 * 1024);
    byte[] buffer = new byte[RIL_MAX_COMMAND_BYTES];

    public SapRilReceiver(Handler SapServerMsgHandler, Handler sapServiceHandler) {
        mSapServerMsgHandler = SapServerMsgHandler;
        mSapServiceHandler = sapServiceHandler;
    }

    /**
     * Open the RIL-BT socket in rild. Will continuously try to open the BT socket until
     * success. (Based on the approach used to open the rild socket in telephony)
     * @return The socket handle
     */
    public static LocalSocket openRilBtSocket() {
        int retryCount = 0;
        LocalSocket rilSocket = null;

        for (;;) {
            LocalSocketAddress address;

            try {
                rilSocket = new LocalSocket();
                address = new LocalSocketAddress(SOCKET_NAME_RIL_BT,
                        LocalSocketAddress.Namespace.RESERVED);
                rilSocket.connect(address);
                break; // Socket opened
            } catch (IOException ex){
                try {
                    if (rilSocket != null) {
                        rilSocket.close();
                    }
                } catch (IOException ex2) {
                    //ignore failure to close after failure to connect
                }

                // don't print an error message after the the first time
                // or after the 8th time
                if (retryCount == 8) {
                    Log.e (TAG,
                        "Couldn't find '" + SOCKET_NAME_RIL_BT
                        + "' socket after " + retryCount
                        + " times, continuing to retry silently");
                } else if (retryCount > 0 && retryCount < 8) {
                    Log.i (TAG,
                        "Couldn't find '" + SOCKET_NAME_RIL_BT
                        + "' socket; retrying after timeout");
                    if (VERBOSE) Log.w(TAG, ex);
                }

                try {
                    Thread.sleep(SOCKET_OPEN_RETRY_MILLIS);
                } catch (InterruptedException er) {
                }

                retryCount++;
                continue;
            }
        }
        return rilSocket;
    }


    public CodedOutputStreamMicro getRilBtOutStream() {
        return mRilBtOutStream;
    }

    /**
     * Notify SapServer that this class is ready for shutdown.
     */
    private void notifyShutdown() {
        if (DEBUG) Log.i(TAG, "notifyShutdown()");
        // If we are already shutdown, don't bother sending a notification.
        synchronized (this) {
            if (mSocket != null) sendShutdownMessage();
        }
    }

    /**
     * This will terminate the SapRilReceiver thread, by closing the RIL-BT in-/output
     * streams.
     */
    public void shutdown() {
        if (DEBUG) Log.i(TAG, "shutdown()");

        /* On Android you need to close the IOstreams using Socket.shutdown*
         * The IOstream close must not be used, as it some how decouples the
         * stream from the socket, and when the socket is closed, the pending
         * reads never return nor throw and exception.
         * Hence here we use the shutdown method: */
        synchronized (this) {
            if (mSocket != null) {
                try {
                    mSocket.shutdownOutput();
                } catch (IOException e) {}
                try {
                    mSocket.shutdownInput();
                } catch (IOException e) {}
                try {
                    mSocket.close();
                } catch (IOException ex) {
                    if (VERBOSE) Log.e(TAG,"Uncaught exception", ex);
                } finally {
                    mSocket = null;
                }
            }
        }
    }

    /**
     * Read the message into buffer
     * @param is
     * @param buffer
     * @return the length of the message
     * @throws IOException
     */
    private static int readMessage(InputStream is, byte[] buffer) throws IOException {
        int countRead;
        int offset;
        int remaining;
        int messageLength;

        // Read in the length of the message
        offset = 0;
        remaining = 4;
        do {
            countRead = is.read(buffer, offset, remaining);

            if (countRead < 0 ) {
                Log.e(TAG, "Hit EOS reading message length");
                return -1;
            }

            offset += countRead;
            remaining -= countRead;
        } while (remaining > 0);

        messageLength = ((buffer[0] & 0xff) << 24)
                | ((buffer[1] & 0xff) << 16)
                | ((buffer[2] & 0xff) << 8)
                | (buffer[3] & 0xff);
        if (VERBOSE) Log.e(TAG,"Message length found to be: "+messageLength);
        // Read the message
        offset = 0;
        remaining = messageLength;
        do {
            countRead = is.read(buffer, offset, remaining);

            if (countRead < 0 ) {
                Log.e(TAG, "Hit EOS reading message.  messageLength=" + messageLength
                        + " remaining=" + remaining);
                return -1;
            }

            offset += countRead;
            remaining -= countRead;
        } while (remaining > 0);

        return messageLength;
    }

    /**
     * The RIL reader thread. Will handle open of the RIL-BT socket, and notify
     * SapServer when done.
     */
    @Override
    public void run() {

        try {
            if (VERBOSE) Log.i(TAG, "Starting RilBtReceiverThread...");

            mSocket = openRilBtSocket();
            mRilBtInStream = mSocket.getInputStream();
            mRilBtOutStream = CodedOutputStreamMicro.newInstance(mSocket.getOutputStream());

            // Notify the SapServer that we have connected to the RilBtSocket
            sendRilConnectMessage();

            // The main loop - read messages and forward to SAP server
            for (;;) {
                SapMessage sapMsg = null;
                MsgHeader rilMsg;

                if (VERBOSE) Log.i(TAG, "Waiting for incoming message...");
                int length = readMessage(mRilBtInStream, buffer);

                SapService.notifyUpdateWakeLock(mSapServiceHandler);

                if (length == -1) {
                    if (DEBUG) Log.i(TAG, "EOF reached - closing down.");
                    break;
                }

                CodedInputStreamMicro msgStream =
                        CodedInputStreamMicro.newInstance(buffer, 0, length);

                rilMsg = MsgHeader.parseFrom(msgStream);

                if (VERBOSE) Log.i(TAG, "Message received.");

                sapMsg = SapMessage.newInstance(rilMsg);

                if (sapMsg != null && sapMsg.getMsgType() != SapMessage.INVALID_VALUE)
                {
                    if (sapMsg.getMsgType() < SapMessage.ID_RIL_BASE) {
                        sendClientMessage(sapMsg);
                    } else {
                        sendRilIndMessage(sapMsg);
                    }
                } // else simply ignore it
            }

        } catch (IOException e) {
            notifyShutdown(); /* Only needed in case of a connection error */
            Log.i(TAG, "'" + SOCKET_NAME_RIL_BT + "' socket inputStream closed", e);

        } finally {
            Log.i(TAG, "Disconnected from '" + SOCKET_NAME_RIL_BT + "' socket");
        }
    }

    /**
     * Notify SapServer that the RIL socket is connected
     */
    private void sendRilConnectMessage() {
        if (mSapServerMsgHandler != null) {
            mSapServerMsgHandler.sendEmptyMessage(SapServer.SAP_MSG_RIL_CONNECT);
        }
    }

    /**
     * Send reply (solicited) message from the RIL to the Sap Server Handler Thread
     * @param sapMsg The message to send
     */
    private void sendClientMessage(SapMessage sapMsg) {
        Message newMsg = mSapServerMsgHandler.obtainMessage(SapServer.SAP_MSG_RFC_REPLY, sapMsg);
        mSapServerMsgHandler.sendMessage(newMsg);
    }

    /**
     * Send a shutdown signal to SapServer to indicate the
     */
    private void sendShutdownMessage() {
        if (mSapServerMsgHandler != null) {
            mSapServerMsgHandler.sendEmptyMessage(SapServer.SAP_RIL_SOCK_CLOSED);
        }
    }

    /**
     * Send indication (unsolicited) message from RIL to the Sap Server Handler Thread
     * @param sapMsg The message to send
     */
    private void sendRilIndMessage(SapMessage sapMsg) {
        Message newMsg = mSapServerMsgHandler.obtainMessage(SapServer.SAP_MSG_RIL_IND, sapMsg);
        mSapServerMsgHandler.sendMessage(newMsg);
    }

}
