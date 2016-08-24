package com.android.bluetooth.tests;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.concurrent.CountDownLatch;

import javax.obex.HeaderSet;
import javax.obex.Operation;
import javax.obex.ServerRequestHandler;

import junit.framework.Assert;
import android.annotation.TargetApi;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.os.Build;
import android.os.RemoteException;
import android.test.AndroidTestCase;
import android.util.Log;

import com.android.bluetooth.BluetoothObexTransport;
import com.android.bluetooth.tests.TestSequencer.OPTYPE;

@TargetApi(Build.VERSION_CODES.KITKAT)
public class MapObexLevelTest extends AndroidTestCase implements ITestSequenceConfigurator {
    protected static String TAG = "MapObexLevelTest";
    protected static final boolean D = true;
    protected static final boolean TRACE = false;
    protected static final boolean DELAY_PASS_30_SEC = true;

    // 128 bit UUID for MAP MAS
    static final byte[] MAS_TARGET = new byte[] {
             (byte)0xBB, (byte)0x58, (byte)0x2B, (byte)0x40,
             (byte)0x42, (byte)0x0C, (byte)0x11, (byte)0xDB,
             (byte)0xB0, (byte)0xDE, (byte)0x08, (byte)0x00,
             (byte)0x20, (byte)0x0C, (byte)0x9A, (byte)0x66
             };

    // 128 bit UUID for MAP MNS
    static final byte[] MNS_TARGET = new byte[] {
             (byte)0xBB, (byte)0x58, (byte)0x2B, (byte)0x41,
             (byte)0x42, (byte)0x0C, (byte)0x11, (byte)0xDB,
             (byte)0xB0, (byte)0xDE, (byte)0x08, (byte)0x00,
             (byte)0x20, (byte)0x0C, (byte)0x9A, (byte)0x66
             };

    /* Message types */
    static final String TYPE_GET_FOLDER_LISTING              = "x-obex/folder-listing";
    static final String TYPE_GET_MESSAGE_LISTING             = "x-bt/MAP-msg-listing";
    static final String TYPE_GET_CONVO_LISTING               = "x-bt/MAP-convo-listing";
    static final String TYPE_MESSAGE                         = "x-bt/message";
    static final String TYPE_SET_MESSAGE_STATUS              = "x-bt/messageStatus";
    static final String TYPE_SET_NOTIFICATION_REGISTRATION   = "x-bt/MAP-NotificationRegistration";
    static final String TYPE_MESSAGE_UPDATE                  = "x-bt/MAP-messageUpdate";
    static final String TYPE_GET_MAS_INSTANCE_INFORMATION    = "x-bt/MASInstanceInformation";

    public void testFolder() {
        testLocalSockets(new buildFolderTestSeq());
    }

    public void testFolderServer() {
        testServer(new buildFolderTestSeq());
    }

    public void testFolderClient() {
        testClient(new buildFolderTestSeq());
    }

    protected class buildFolderTestSeq implements ITestSequenceBuilder {
        @Override
        public void build(TestSequencer sequencer) {
            addConnectStep(sequencer);

            MapStepsFolder.addGoToMsgFolderSteps(sequencer);

            // MAP DISCONNECT Step
            addDisconnectStep(sequencer);
        }
    }


    public void testConvo() {
        testLocalSockets(new buildConvoTestSeq());
    }

    public void testConvoServer() {
        testServer(new buildConvoTestSeq());
    }

    public void testConvoClient() {
        testClient(new buildConvoTestSeq());
    }

    class buildConvoTestSeq implements ITestSequenceBuilder {
        @Override
        public void build(TestSequencer sequencer) {
            addConnectStep(sequencer);

            MapStepsFolder.addGoToMsgFolderSteps(sequencer);

            MapStepsConvo.addConvoListingSteps(sequencer);

            // MAP DISCONNECT Step
            addDisconnectStep(sequencer);
        }
    }

    /**
     * Run the test sequence using a local socket on a single device.
     * Throughput around 4000 kbyte/s - with a larger OBEX package size.
     *
     * Downside: Unable to get a BT-snoop file...
     */
    protected void testLocalSockets(ITestSequenceBuilder builder) {
        mContext = this.getContext();
        MapTestData.init(mContext);
        Log.i(TAG,"Setting up sockets...");

        try {
            /* Create and interconnect local pipes for transport */
            LocalServerSocket serverSock = new LocalServerSocket("com.android.bluetooth.tests.sock");
            LocalSocket clientSock = new LocalSocket();
            LocalSocket acceptSock;

            clientSock.connect(serverSock.getLocalSocketAddress());

            acceptSock = serverSock.accept();

            /* Create the OBEX transport objects to wrap the pipes - enable SRM */
            ObexPipeTransport clientTransport = new ObexPipeTransport(clientSock.getInputStream(),
                    clientSock.getOutputStream(), true);
            ObexPipeTransport serverTransport = new ObexPipeTransport(acceptSock.getInputStream(),
                    acceptSock.getOutputStream(), true);

            TestSequencer sequencer  = new TestSequencer(clientTransport, serverTransport, this);

            builder.build(sequencer);

            //Debug.startMethodTracing("ObexTrace");
            assertTrue(sequencer.run(mContext));
            //Debug.stopMethodTracing();

            clientSock.close();
            acceptSock.close();
            serverSock.close();
        } catch (IOException e) {
            Log.e(TAG, "IOException", e);
        }
    }

    /**
     * Server side of a dual device test using a Bluetooth Socket.
     * Enables the possibility to get a BT-snoop file.
     * If you need the btsnoop from the device which completes the test with success
     * you need to add a delay after the test ends, and fetch the file before this delay
     * expires. When the test completes, the Bluetooth subsystem will be restarted, causing
     * a new bt-snoop to overwrite the one used in test.
     */
    public void testServer(ITestSequenceBuilder builder) {
        mContext = this.getContext();
        MapTestData.init(mContext);
        Log.i(TAG,"Setting up sockets...");

        try {
            /* This will turn on BT and create a server socket on which accept can be called. */
            BluetoothServerSocket serverSocket=ObexTest.createServerSocket(BluetoothSocket.TYPE_L2CAP, true);

            Log.i(TAG, "Waiting for client to connect...");
            BluetoothSocket socket = serverSocket.accept();
            Log.i(TAG, "Client connected...");

            BluetoothObexTransport serverTransport = new BluetoothObexTransport(socket);

            TestSequencer sequencer  = new TestSequencer(null, serverTransport, this);

            builder.build(sequencer);

            //Debug.startMethodTracing("ObexTrace");
            assertTrue(sequencer.run(mContext));
            //Debug.stopMethodTracing();

            serverSocket.close();
            socket.close();
        } catch (IOException e) {
            Log.e(TAG, "IOException", e);
        }
        if(DELAY_PASS_30_SEC) {
            Log.i(TAG, "\n\n\nTest done - please fetch logs within 30 seconds...\n\n\n");
            try {
                Thread.sleep(30000);
            } catch (InterruptedException e) {}
        }
    }

    /**
     * Server side of a dual device test using a Bluetooth Socket.
     * Enables the possibility to get a BT-snoop file.
     * If you need the btsnoop from the device which completes the test with success
     * you need to add a delay after the test ends, and fetch the file before this delay
     * expires. When the test completes, the Bluetooth subsystem will be restarted, causing
     * a new bt-snoop to overwrite the one used in test.
     */
    public void testClient(ITestSequenceBuilder builder) {
        mContext = this.getContext();
        MapTestData.init(mContext);
        Log.i(TAG, "Setting up sockets...");

        try {
            /* This will turn on BT and connect */
            BluetoothSocket clientSock =
                    ObexTest.connectClientSocket(BluetoothSocket.TYPE_L2CAP, true, mContext);

            BluetoothObexTransport clientTransport = new BluetoothObexTransport(clientSock);

            TestSequencer sequencer  = new TestSequencer(clientTransport, null, this);

            builder.build(sequencer);

            //Debug.startMethodTracing("ObexTrace");
            assertTrue(sequencer.run(mContext));
            //Debug.stopMethodTracing();

            clientSock.close();
        } catch (IOException e) {
            Log.e(TAG, "IOException", e);
        }
        if(DELAY_PASS_30_SEC) {
            Log.i(TAG, "\n\n\nTest done - please fetch logs within 30 seconds...\n\n\n");
            try {
                Thread.sleep(30000);
            } catch (InterruptedException e) {}
        }
    }

    protected void addConnectStep(TestSequencer sequencer) {
        SeqStep step;

        // MAP CONNECT Step
        step = sequencer.addStep(OPTYPE.CONNECT, null);
        HeaderSet hs = new HeaderSet();
        hs.setHeader(HeaderSet.TARGET, MAS_TARGET);
        step.mReqHeaders = hs;
        step.mValidator = new MapConnectValidator();
        //step.mServerPreAction = new MapAddSmsMessages(); // could take in parameters
    }

    protected void addDisconnectStep(TestSequencer sequencer) {
        sequencer.addStep(OPTYPE.DISCONNECT, ObexTest.getResponsecodevalidator());
    }

    /* Functions to validate results */

    private class MapConnectValidator implements ISeqStepValidator {
        @Override
        public boolean validate(SeqStep step, HeaderSet response, Operation notUsed)
                throws IOException {
            Assert.assertNotNull(response);
            byte[] who = (byte[])response.getHeader(HeaderSet.WHO);
            Assert.assertNotNull(who);
            Assert.assertTrue(Arrays.equals(who, MAS_TARGET));
            Assert.assertNotNull(response.getHeader(HeaderSet.CONNECTION_ID));
            return true;
        }
    }

    /**
     * This is the function creating the Obex Server to be used in this class.
     * Here we use a mocked version of the MapObexServer class
     */
    @Override
    public ServerRequestHandler getObexServer(ArrayList<SeqStep> sequence,
            CountDownLatch stopLatch) {
        try {
            return new MapObexTestServer(mContext, sequence, stopLatch);
        } catch (RemoteException e) {
            Log.e(TAG, "exception", e);
            fail("Unable to create MapObexTestServer");
        }
        return null;
    }


}

