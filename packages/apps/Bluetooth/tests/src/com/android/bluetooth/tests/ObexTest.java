/*
* Copyright (C) 2014 Samsung System LSI
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

package com.android.bluetooth.tests;

import java.io.IOException;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.util.ArrayList;
import java.util.concurrent.CountDownLatch;

import javax.obex.HeaderSet;
import javax.obex.ObexTransport;
import javax.obex.Operation;
import javax.obex.ResponseCodes;
import javax.obex.ServerRequestHandler;

import junit.framework.Assert;
import android.annotation.TargetApi;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.bluetooth.BluetoothUuid;
import android.bluetooth.SdpMasRecord;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.os.Build;
import android.os.Debug;
import android.os.ParcelUuid;
import android.test.AndroidTestCase;
import android.util.Log;

import com.android.bluetooth.BluetoothObexTransport;
import com.android.bluetooth.sdp.SdpManager;
import com.android.bluetooth.tests.TestSequencer.OPTYPE;

/**
 * Test either using the reference ril without a modem, or using a RIL implementing the
 * BT SAP API, by providing the rild-bt socket as well as the extended API functions for SAP.
 *
 */
@TargetApi(Build.VERSION_CODES.KITKAT)
public class ObexTest extends AndroidTestCase implements ITestSequenceConfigurator {
    protected static String TAG = "ObexTest";
    protected static final boolean D = true;
    protected static final boolean TRACE = false;
    protected static final boolean DELAY_PASS_30_SEC = false;
    public static final long PROGRESS_INTERVAL_MS = 1000;
    private static final ObexTestParams defaultParams =
            new ObexTestParams(2*8092, 0, 2*1024*1024);

    private static final ObexTestParams throttle100Params =
            new ObexTestParams(2*8092, 100000, 1024*1024);

    private static final ObexTestParams smallParams =
            new ObexTestParams(2*8092, 0, 2*1024);

    private static final ObexTestParams hugeParams =
            new ObexTestParams(2*8092, 0, 100*1024*1024);

    private static final int SMALL_OPERATION_COUNT = 1000;
    private static final int CONNECT_OPERATION_COUNT = 4500;

    private static final int L2CAP_PSM = 29; /* If SDP is not used */
    private static final int RFCOMM_CHANNEL = 29; /* If SDP is not used */

    //public static final String SERVER_ADDRESS = "10:68:3F:5E:F9:2E";
    public static final String SERVER_ADDRESS = "F8:CF:C5:A8:70:7E";

    private static final String SDP_SERVER_NAME = "Samsung Server";
    private static final String SDP_CLIENT_NAME = "Samsung Client";

    private static final long SDP_FEATURES  = 0x87654321L; /* 32 bit */
    private static final int SDP_MSG_TYPES  = 0xf1;       /*  8 bit */
    private static final int SDP_MAS_ID     = 0xCA;       /*  8 bit */
    private static final int SDP_VERSION    = 0xF0C0;     /* 16 bit */
    public static final ParcelUuid SDP_UUID_OBEX_MAS = BluetoothUuid.MAS;

    private static int sSdpHandle = -1;
    private static final ObexTestDataHandler sDataHandler = new ObexTestDataHandler("(Client)");
    private static final ISeqStepValidator sResponseCodeValidator = new ResponseCodeValidator();
    private static final ISeqStepValidator sDataValidator = new DataValidator();


    private enum SequencerType {
        SEQ_TYPE_PAYLOAD,
        SEQ_TYPE_CONNECT_DISCONNECT
    }

    private Context mContext = null;
    private int mChannelType = 0;

    public ObexTest() {
        super();
    }

    /**
     * Test that a connection can be established.
     * WARNING: The performance of the pipe implementation is not good. I'm only able to get a
     * throughput of around 220 kbyte/sec - less that when using Bluetooth :-)
     * UPDATE: Did a local socket implementation below to replace this...
     *         This has a throughput of more than 4000 kbyte/s
     */
    public void testLocalPipes() {
        mContext = this.getContext();
        System.out.println("Setting up pipes...");

        PipedInputStream clientInStream = null;
        PipedOutputStream clientOutStream = null;
        PipedInputStream serverInStream = null;
        PipedOutputStream serverOutStream = null;
        ObexPipeTransport clientTransport = null;
        ObexPipeTransport serverTransport = null;

        try {
            /* Create and interconnect local pipes for transport */
            clientInStream = new PipedInputStream(5*8092);
            clientOutStream = new PipedOutputStream();
            serverInStream = new PipedInputStream(clientOutStream, 5*8092);
            serverOutStream = new PipedOutputStream(clientInStream);

            /* Create the OBEX transport objects to wrap the pipes - enable SRM */
            clientTransport = new ObexPipeTransport(clientInStream, clientOutStream, true);
            serverTransport = new ObexPipeTransport(serverInStream, serverOutStream, true);

            TestSequencer sequencer = createBtPayloadTestSequence(clientTransport, serverTransport);

            //Debug.startMethodTracing("ObexTrace");
            assertTrue(sequencer.run(mContext));
            //Debug.stopMethodTracing();
        } catch (IOException e) {
            Log.e(TAG, "IOException", e);
        }
    }

    /**
     * Run the test sequence using a local socket.
     * Throughput around 4000 kbyte/s - with a larger OBEX package size.
     */
    public void testLocalSockets() {
        mContext = this.getContext();
        System.out.println("Setting up sockets...");

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

            TestSequencer sequencer = createBtPayloadTestSequence(clientTransport, serverTransport);

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

    /* Create a sequence of put/get operations with different payload sizes */
    private TestSequencer createBtPayloadTestSequence(ObexTransport clientTransport,
            ObexTransport serverTransport)
            throws IOException {
        TestSequencer sequencer = new TestSequencer(clientTransport, serverTransport, this);
        SeqStep step;

        step = sequencer.addStep(OPTYPE.CONNECT, sResponseCodeValidator);
        if(false){

        step = sequencer.addStep(OPTYPE.PUT, sDataValidator);
        step.mParams = defaultParams;
        step.mUseSrm = true;

        step = sequencer.addStep(OPTYPE.GET, sDataValidator);
        step.mParams = defaultParams;
        step.mUseSrm = true;

        step = sequencer.addStep(OPTYPE.PUT, sDataValidator);
        step.mParams = throttle100Params;
        step.mUseSrm = true;

        step = sequencer.addStep(OPTYPE.GET, sDataValidator);
        step.mParams = throttle100Params;
        step.mUseSrm = true;

        for(int i=0; i<SMALL_OPERATION_COUNT; i++){
            step = sequencer.addStep(OPTYPE.PUT, sDataValidator);
            step.mParams = smallParams;
            step.mUseSrm = true;

            step = sequencer.addStep(OPTYPE.GET, sDataValidator);
            step.mParams = smallParams;
            step.mUseSrm = true;

        }
}

        step = sequencer.addStep(OPTYPE.PUT, sDataValidator);
        step.mParams = hugeParams;
        step.mUseSrm = true;

        step = sequencer.addStep(OPTYPE.GET, sDataValidator);
        step.mParams = hugeParams;
        step.mUseSrm = true;
        step = sequencer.addStep(OPTYPE.DISCONNECT, sResponseCodeValidator);

        return sequencer;
    }

    private TestSequencer createBtConnectTestSequence(ObexTransport clientTransport,
            ObexTransport serverTransport)
            throws IOException {
        TestSequencer sequencer = new TestSequencer(clientTransport, serverTransport, this);
        SeqStep step;

            step = sequencer.addStep(OPTYPE.CONNECT, sResponseCodeValidator);

            step = sequencer.addStep(OPTYPE.PUT, sDataValidator);
            step.mParams = smallParams;
            step.mUseSrm = true;

            step = sequencer.addStep(OPTYPE.GET, sDataValidator);
            step.mParams = smallParams;
            step.mUseSrm = true;

            step = sequencer.addStep(OPTYPE.DISCONNECT, sResponseCodeValidator);

        return sequencer;
    }


    /**
     * Use this validator to validate operation response codes. E.g. for OBEX CONNECT and
     * DISCONNECT operations.
     * Expects HeaderSet to be valid, and Operation to be null.
     */
    public static ISeqStepValidator getResponsecodevalidator() {
        return sResponseCodeValidator;
    }

    /**
     * Use this validator to validate (and read/write data) for OBEX PUT and GET operations.
     * Expects Operation to be valid, and HeaderSet to be null.
     */
    public static ISeqStepValidator getDatavalidator() {
        return sDataValidator;
    }

    /**
     * Use this validator to validate operation response codes. E.g. for OBEX CONNECT and
     * DISCONNECT operations.
     * Expects HeaderSet to be valid, and Operation to be null.
     */
    private static class ResponseCodeValidator implements ISeqStepValidator {

        protected static boolean validateHeaderSet(HeaderSet headers, HeaderSet expected)
                throws IOException {
            if(headers.getResponseCode() != ResponseCodes.OBEX_HTTP_OK) {
                Log.e(TAG,"Wrong ResponseCode: " + headers.getResponseCode());
                Assert.assertTrue(false);
                return false;
            }
            return true;
        }

        @Override
        public boolean validate(SeqStep step, HeaderSet response, Operation op)
                throws IOException {
            if(response == null) {
                if(op.getResponseCode() != ResponseCodes.OBEX_HTTP_OK) {
                    Log.e(TAG,"Wrong ResponseCode: " + op.getResponseCode());
                    Assert.assertTrue(false);
                    return false;
                }
                return true;
            }
            return validateHeaderSet(response, step.mResHeaders);
        }
    }

    /**
     * Use this validator to validate (and read/write data) for OBEX PUT and GET operations.
     * Expects Operation to ve valid, and HeaderSet to be null.
     */
    private static class DataValidator implements ISeqStepValidator {
        @Override
        public boolean validate(SeqStep step, HeaderSet notUsed, Operation op)
        throws IOException {
            Assert.assertNotNull(op);
            if(step.mType == OPTYPE.GET) {
                op.noBodyHeader();
                sDataHandler.readData(op.openDataInputStream(), step.mParams);
            } else if (step.mType == OPTYPE.PUT) {
                sDataHandler.writeData(op.openDataOutputStream(), step.mParams);
            }
            int responseCode = op.getResponseCode();
            Log.i(TAG, "response code: " + responseCode);
            HeaderSet response = op.getReceivedHeader();
            ResponseCodeValidator.validateHeaderSet(response, step.mResHeaders);
            op.close();
            return true;
        }
    }

    public void testBtServerL2cap() {
        testBtServer(BluetoothSocket.TYPE_L2CAP, false, SequencerType.SEQ_TYPE_PAYLOAD);
    }

    public void testBtServerRfcomm() {
        testBtServer(BluetoothSocket.TYPE_RFCOMM, false, SequencerType.SEQ_TYPE_PAYLOAD);
    }

    public void testBtClientL2cap() {
        testBtClient(BluetoothSocket.TYPE_L2CAP, false, SequencerType.SEQ_TYPE_PAYLOAD);
    }

    public void testBtClientRfcomm() {
        testBtClient(BluetoothSocket.TYPE_RFCOMM, false, SequencerType.SEQ_TYPE_PAYLOAD);
    }

    public void testBtServerSdpL2cap() {
        testBtServer(BluetoothSocket.TYPE_L2CAP, true, SequencerType.SEQ_TYPE_PAYLOAD);
    }

    public void testBtServerSdpRfcomm() {
        testBtServer(BluetoothSocket.TYPE_RFCOMM, true, SequencerType.SEQ_TYPE_PAYLOAD);
    }

    public void testBtClientSdpL2cap() {
        testBtClient(BluetoothSocket.TYPE_L2CAP, true, SequencerType.SEQ_TYPE_PAYLOAD);
    }

    public void testBtClientSdpRfcomm() {
        testBtClient(BluetoothSocket.TYPE_RFCOMM, true, SequencerType.SEQ_TYPE_PAYLOAD);
    }

    public void testBtServerConnectL2cap() {
        for(int i=0; i<CONNECT_OPERATION_COUNT; i++){
            Log.i(TAG, "Starting iteration " + i);
            testBtServer(BluetoothSocket.TYPE_L2CAP, true,
                    SequencerType.SEQ_TYPE_CONNECT_DISCONNECT);
            try {
                Thread.sleep(50);
            } catch (InterruptedException e) {
                Log.e(TAG,"Exception while waiting...",e);
            }
        }
    }

    public void testBtClientConnectL2cap() {
        for(int i=0; i<CONNECT_OPERATION_COUNT; i++){
            Log.i(TAG, "Starting iteration " + i);
            testBtClient(BluetoothSocket.TYPE_L2CAP, true,
                    SequencerType.SEQ_TYPE_CONNECT_DISCONNECT);
            try {
                // We give the server 100ms to allow adding SDP record
                Thread.sleep(150);
            } catch (InterruptedException e) {
                Log.e(TAG,"Exception while waiting...",e);
            }
        }
    }

    public void testBtServerConnectRfcomm() {
        for(int i=0; i<CONNECT_OPERATION_COUNT; i++){
            Log.i(TAG, "Starting iteration " + i);
            testBtServer(BluetoothSocket.TYPE_RFCOMM, true,
                    SequencerType.SEQ_TYPE_CONNECT_DISCONNECT);
            try {
                Thread.sleep(50);
            } catch (InterruptedException e) {
                Log.e(TAG,"Exception while waiting...",e);
            }
        }
    }

    public void testBtClientConnectRfcomm() {
        for(int i=0; i<CONNECT_OPERATION_COUNT; i++){
            Log.i(TAG, "Starting iteration " + i);
            testBtClient(BluetoothSocket.TYPE_RFCOMM, true,
                    SequencerType.SEQ_TYPE_CONNECT_DISCONNECT);
            try {
                // We give the server 100ms to allow adding SDP record
                Thread.sleep(250);
            } catch (InterruptedException e) {
                Log.e(TAG,"Exception while waiting...",e);
            }
        }
    }

    /**
     * Create a serverSocket
     * @param type
     * @param useSdp
     * @return
     * @throws IOException
     */
    public static BluetoothServerSocket createServerSocket(int type, boolean useSdp)
            throws IOException {
        int rfcommChannel = -1;
        int l2capPsm = -1;

        BluetoothAdapter bt = BluetoothAdapter.getDefaultAdapter();
        if(bt == null) {
            Log.e(TAG,"No Bluetooth Device!");
            assertTrue(false);
        }
        BluetoothTestUtils.enableBt(bt);
        BluetoothServerSocket serverSocket=null;
        if(type == BluetoothSocket.TYPE_L2CAP) {
            if(useSdp == true) {
                serverSocket = bt.listenUsingL2capOn(
                        BluetoothAdapter.SOCKET_CHANNEL_AUTO_STATIC_NO_SDP);
            } else {
                serverSocket = bt.listenUsingL2capOn(L2CAP_PSM);
            }
            l2capPsm = serverSocket.getChannel();
            Log.d(TAG, "L2CAP createde, PSM: " + l2capPsm);
        } else if(type == BluetoothSocket.TYPE_RFCOMM) {
            if(useSdp == true) {
                serverSocket = bt.listenUsingInsecureRfcommOn(
                        BluetoothAdapter.SOCKET_CHANNEL_AUTO_STATIC_NO_SDP);
            } else {
                serverSocket = bt.listenUsingInsecureRfcommOn(RFCOMM_CHANNEL);
            }
            rfcommChannel = serverSocket.getChannel();
            Log.d(TAG, "RFCOMM createde, Channel: " + rfcommChannel);
        } else {
            fail("Invalid transport type!");
        }
        if(useSdp == true) {
            /* We use the MAP service record to be able to set rfcomm and l2cap channels */
            // TODO: We need to free this
            if(sSdpHandle >= 0) {
                SdpManager.getDefaultManager().removeSdpRecord(sSdpHandle);
            }
            Log.d(TAG, "Creating record with rfcomm channel: " + rfcommChannel +
                    " and l2cap channel: " + l2capPsm);
            sSdpHandle = SdpManager.getDefaultManager().createMapMasRecord(SDP_SERVER_NAME,
                    SDP_MAS_ID, rfcommChannel, l2capPsm,
                    SDP_VERSION, SDP_MSG_TYPES, (int)(SDP_FEATURES & 0xffffffff));
        } else {
            Log.d(TAG, "SKIP creation of record with rfcomm channel: " + rfcommChannel +
                    " and l2cap channel: " + l2capPsm);
        }
        return serverSocket;
    }

    public static void removeSdp() {
        if(sSdpHandle > 0) {
            SdpManager.getDefaultManager().removeSdpRecord(sSdpHandle);
            sSdpHandle = -1;
        }
    }

    /**
     * Server side of a two device Bluetooth test of OBEX
     */
    private void testBtServer(int type, boolean useSdp, SequencerType sequencerType) {
        mContext = this.getContext();
        Log.d(TAG,"Starting BT Server...");

        if(TRACE) Debug.startMethodTracing("ServerSide");
        try {
            BluetoothServerSocket serverSocket=createServerSocket(type, useSdp);

            Log.i(TAG, "Waiting for client to connect");
            BluetoothSocket socket = serverSocket.accept();
            Log.i(TAG, "Client connected");

            BluetoothObexTransport serverTransport = new BluetoothObexTransport(socket);
            TestSequencer sequencer = null;
            switch(sequencerType) {
            case SEQ_TYPE_CONNECT_DISCONNECT:
                sequencer = createBtConnectTestSequence(null, serverTransport);
                break;
            case SEQ_TYPE_PAYLOAD:
                sequencer = createBtPayloadTestSequence(null, serverTransport);
                break;
            default:
                fail("Invalid sequencer type");
                break;

            }
            //Debug.startMethodTracing("ObexTrace");
            assertTrue(sequencer.run(mContext));
            //Debug.stopMethodTracing();
            // Same as below... serverTransport.close();
            // This is done by the obex server socket.close();
            serverSocket.close();
            removeSdp();
            sequencer.shutdown();
        } catch (IOException e) {
            Log.e(TAG, "IOException", e);
        }
        if(TRACE) Debug.stopMethodTracing();
        if(DELAY_PASS_30_SEC) {
            Log.i(TAG, "\n\n\nTest done - please fetch logs within 30 seconds...\n\n\n");
            try {
                Thread.sleep(30000);
            } catch (InterruptedException e) {}
        }
        Log.i(TAG, "Test done.");
    }

    /**
     * Enable Bluetooth and connect to a server socket
     * @param type
     * @param useSdp
     * @param context
     * @return
     * @throws IOException
     */
    static public BluetoothSocket connectClientSocket(int type, boolean useSdp, Context context)
            throws IOException {
        int rfcommChannel = RFCOMM_CHANNEL;
        int l2capPsm = L2CAP_PSM;

        BluetoothAdapter bt = BluetoothAdapter.getDefaultAdapter();
        if(bt == null) {
            Log.e(TAG,"No Bluetooth Device!");
            assertTrue(false);
        }
        BluetoothTestUtils.enableBt(bt);
        BluetoothDevice serverDevice = bt.getRemoteDevice(SERVER_ADDRESS);

        if(useSdp == true) {
            SdpMasRecord record = clientAwaitSdp(serverDevice, context);
            rfcommChannel = record.getRfcommCannelNumber();
            l2capPsm = record.getL2capPsm();
        }

        BluetoothSocket socket = null;
        if(type == BluetoothSocket.TYPE_L2CAP) {
            socket = serverDevice.createL2capSocket(l2capPsm);
        } else if(type == BluetoothSocket.TYPE_RFCOMM) {
            socket = serverDevice.createRfcommSocket(rfcommChannel);
        } else {
            fail("Invalid transport type!");
        }

        socket.connect();

        return socket;
    }

    /**
     * Test that a connection can be established.
     */
    private void testBtClient(int type, boolean useSdp, SequencerType sequencerType) {
        mContext = this.getContext();
        mChannelType = type;
        BluetoothSocket socket = null;
        System.out.println("Starting BT Client...");
        if(TRACE) Debug.startMethodTracing("ClientSide");
        try {
            socket = connectClientSocket(type, useSdp, mContext);

            BluetoothObexTransport clientTransport = new BluetoothObexTransport(socket);

            TestSequencer sequencer = null;
            switch(sequencerType) {
            case SEQ_TYPE_CONNECT_DISCONNECT:
                sequencer = createBtConnectTestSequence(clientTransport, null);
                break;
            case SEQ_TYPE_PAYLOAD:
                sequencer = createBtPayloadTestSequence(clientTransport, null);
                break;
            default:
                fail("Invalid test type");
                break;

            }
            //Debug.startMethodTracing("ObexTrace");
            assertTrue(sequencer.run(mContext));
            //Debug.stopMethodTracing();
            socket.close(); // Only the streams are closed by the obex client
            sequencer.shutdown();

        } catch (IOException e) {
            Log.e(TAG, "IOException", e);
        }
        if(TRACE) Debug.stopMethodTracing();
        if(DELAY_PASS_30_SEC) {
            Log.i(TAG, "\n\n\nTest done - please fetch logs within 30 seconds...\n\n\n");
            try {
                Thread.sleep(30000);
            } catch (InterruptedException e) {}
        }
        Log.i(TAG, "Test done.");
    }

    /* Using an anonymous class is not efficient, but keeps a tight code structure. */
    static class SdpBroadcastReceiver extends BroadcastReceiver {
        private SdpMasRecord mMasRecord; /* A non-optimal way of setting an object reference from
                                            a anonymous class. */
        final CountDownLatch mLatch;
        public SdpBroadcastReceiver(CountDownLatch latch) {
            mLatch = latch;
        }

        SdpMasRecord getMasRecord() {
            return mMasRecord;
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "onReceive");
            String action = intent.getAction();
            if (action.equals(BluetoothDevice.ACTION_SDP_RECORD)){
                Log.v(TAG, "Received ACTION_SDP_RECORD.");
                ParcelUuid uuid = intent.getParcelableExtra(BluetoothDevice.EXTRA_UUID);
                Log.v(TAG, "Received UUID: " + uuid.toString());
                Log.v(TAG, "existing UUID: " + SDP_UUID_OBEX_MAS.toString());
                if(uuid.toString().equals(SDP_UUID_OBEX_MAS.toString())) {
                    assertEquals(SDP_UUID_OBEX_MAS.toString(), uuid.toString());
                    Log.v(TAG, " -> MAS UUID in result.");
                    SdpMasRecord record = intent.getParcelableExtra(
                            BluetoothDevice.EXTRA_SDP_RECORD);
                    assertNotNull(record);
                    Log.v(TAG, " -> record: "+record);
                    if(record.getServiceName().equals(SDP_SERVER_NAME)) {

                        assertEquals(((long)record.getSupportedFeatures())
                                &0xffffffffL, SDP_FEATURES);

                        assertEquals(record.getSupportedMessageTypes(), SDP_MSG_TYPES);

                        assertEquals(record.getProfileVersion(), SDP_VERSION);

                        assertEquals(record.getServiceName(), SDP_SERVER_NAME);

                        assertEquals(record.getMasInstanceId(), SDP_MAS_ID);

                        int status = intent.getIntExtra(BluetoothDevice.EXTRA_SDP_SEARCH_STATUS,
                                -1);
                        Log.v(TAG, " -> status: "+status);
                        mMasRecord = record;
                        mLatch.countDown();
                    } else {
                        Log.i(TAG, "Wrong service name (" + record.getServiceName()
                                + ") received, still waiting...");
                    }
                } else {
                    Log.i(TAG, "Wrong UUID received, still waiting...");
                }
            } else {
                fail("Unexpected intent received???");
            }
        }
    };


    private static SdpMasRecord clientAwaitSdp(BluetoothDevice serverDevice, Context context) {
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_SDP_RECORD);
        final CountDownLatch latch = new CountDownLatch(1);
        SdpBroadcastReceiver broadcastReceiver = new SdpBroadcastReceiver(latch);

        context.registerReceiver(broadcastReceiver, filter);

        serverDevice.sdpSearch(SDP_UUID_OBEX_MAS);
        boolean waiting = true;
        while(waiting == true) {
            try {
                Log.i(TAG, "SDP Search requested - awaiting result...");
                latch.await();
                Log.i(TAG, "SDP Search reresult received - continueing.");
                waiting = false;
            } catch (InterruptedException e) {
                Log.w(TAG, "Interrupted witle waiting - keep waiting.", e);
                waiting = true;
            }
        }
        context.unregisterReceiver(broadcastReceiver);
        return broadcastReceiver.getMasRecord();
    }

    @Override
    public ServerRequestHandler getObexServer(ArrayList<SeqStep> sequence,
            CountDownLatch stopLatch) {
        return new ObexTestServer(sequence, stopLatch);
    }



}

