package com.android.bluetooth.tests;

import java.io.IOException;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.util.ArrayList;
import java.util.Arrays;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Handler;
import android.os.Handler.Callback;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.test.AndroidTestCase;
import android.util.Log;

import com.android.bluetooth.sap.SapMessage;
import com.android.bluetooth.sap.SapServer;

/**
 * Test either using the reference ril without a modem, or using a RIL implementing the
 * BT SAP API, by providing the rild-bt socket as well as the extended API functions for SAP.
 *
 */
@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
public class SapServerTest extends AndroidTestCase {
    protected static String TAG = "SapServerTest";
    protected static final boolean D = true;
    // Set the RIL driver in test mode, where request stubs are used instead
    // of forwarding to the Modem/SIM.
    private static final boolean rilTestModeEnabled = false;
    private Context mContext = null;

    public SapServerTest() {
        super();
    }

    private void buildDefaultInitSeq(SapSequencer sequencer) throws IOException {
        SapMessage connectReq = new SapMessage(SapMessage.ID_CONNECT_REQ);
        connectReq.setMaxMsgSize(276);

        SapMessage connectResp = new SapMessage(SapMessage.ID_CONNECT_RESP);
        connectResp.setConnectionStatus(SapMessage.CON_STATUS_OK);
        connectResp.setMaxMsgSize(0); /* shall be connection status (0)  on success */

        SapMessage statusInd = new SapMessage(SapMessage.ID_STATUS_IND);
        statusInd.setStatusChange(SapMessage.STATUS_CARD_RESET);

        int index = sequencer.addStep(connectReq, connectResp);
        sequencer.addSubStep(index, null, statusInd);

    }
    /**
     * Test that the SapServer is capable of handling a connect request with no call ongoing.
     */
    public void testSapServerConnectSimple() {
        mContext = this.getContext();

        try {

            SapSequencer sequencer = new SapSequencer();
            if(rilTestModeEnabled) {
                sequencer.testModeEnable(true);
            }
            /* Build a default init sequence */
            buildDefaultInitSeq(sequencer);
            SapMessage disconnectReq = new SapMessage(SapMessage.ID_DISCONNECT_REQ);
            SapMessage disconnectResp = new SapMessage(SapMessage.ID_DISCONNECT_RESP);

            SapMessage resetResp = new SapMessage(SapMessage.ID_RESET_SIM_RESP);
            resetResp.setResultCode(SapMessage.RESULT_OK);

            int index = sequencer.addStep(disconnectReq, disconnectResp);

            assertTrue(sequencer.run());
        } catch (IOException e) {
            Log.e(TAG, "IOException", e);
        }
    }

    public void testSapServerApiComplete() {
        mContext = this.getContext();
        byte[] dummyBytes = {1, 2, 3, 4};

        /* Select file '2FE2' - observed selected in modem init sequence.
         * According to spec file '2FE2' is EF_ICCID (Elementary file
         * ICC identification).
         */

        byte[] selectFileApdu = {(byte)0xa0, (byte)0xa4, (byte)0x00, (byte)0x00,
            (byte)0x02, (byte)0x2f, (byte)0xe2};

        /* Command succesfull '9F', length '0F' of response data */
        byte[] selectFileApduResp = {(byte)0x9f, (byte)0x0f};

        try {

            SapSequencer sequencer = new SapSequencer();
            if(rilTestModeEnabled) {
                sequencer.testModeEnable(true);
            }

            /* Build a default init sequence */
            buildDefaultInitSeq(sequencer);

            SapMessage powerOffReq = new SapMessage(SapMessage.ID_POWER_SIM_OFF_REQ);

            SapMessage powerOffResp = new SapMessage(SapMessage.ID_POWER_SIM_OFF_RESP);
            powerOffResp.setResultCode(SapMessage.RESULT_OK);
            sequencer.addStep(powerOffReq, powerOffResp);


            SapMessage powerOnReq = new SapMessage(SapMessage.ID_POWER_SIM_ON_REQ);

            SapMessage powerOnResp = new SapMessage(SapMessage.ID_POWER_SIM_ON_RESP);
            powerOnResp.setResultCode(SapMessage.RESULT_OK);
            sequencer.addStep(powerOnReq, powerOnResp);

            SapMessage resetReq = new SapMessage(SapMessage.ID_RESET_SIM_REQ);

            SapMessage resetResp = new SapMessage(SapMessage.ID_RESET_SIM_RESP);
            resetResp.setResultCode(SapMessage.RESULT_OK);
            int index = sequencer.addStep(resetReq, resetResp);

            /* SapMessage statusInd = new SapMessage(SapMessage.ID_STATUS_IND); */
            /* statusInd.setStatusChange(SapMessage.STATUS_CARD_RESET); */
            /* sequencer.addSubStep(index, null, statusInd); */

            SapMessage atrReq = new SapMessage(SapMessage.ID_TRANSFER_ATR_REQ);

            SapMessage atrResp = new SapMessage(SapMessage.ID_TRANSFER_ATR_RESP);
            atrResp.setResultCode(SapMessage.RESULT_OK);
            if(rilTestModeEnabled) {
                /* Use the hard coded return array, must match the test array in RIL */
                byte[] atr = {1, 2, 3, 4};
                atrResp.setAtr(atr);
            } else {
                atrResp.setAtr(null);
            }
            sequencer.addStep(atrReq, atrResp);


            SapMessage apduReq = new SapMessage(SapMessage.ID_TRANSFER_APDU_REQ);
            if(rilTestModeEnabled) {
                apduReq.setApdu(dummyBytes);
            } else {
                apduReq.setApdu(selectFileApdu);
            }

            SapMessage apduResp = new SapMessage(SapMessage.ID_TRANSFER_APDU_RESP);
            apduResp.setResultCode(SapMessage.RESULT_OK);
            if(rilTestModeEnabled) {
                apduResp.setApduResp(dummyBytes);
            } else {
                apduResp.setApduResp(selectFileApduResp);
            }
            sequencer.addStep(apduReq, apduResp);


            SapMessage apdu7816Req = new SapMessage(SapMessage.ID_TRANSFER_APDU_REQ);
            if(rilTestModeEnabled) {
                apdu7816Req.setApdu7816(dummyBytes);
            } else {
                apdu7816Req.setApdu7816(selectFileApdu);
            }

            SapMessage apdu7816Resp = new SapMessage(SapMessage.ID_TRANSFER_APDU_RESP);
            apdu7816Resp.setResultCode(SapMessage.RESULT_OK);
            if(rilTestModeEnabled) {
                apdu7816Resp.setApduResp(dummyBytes);
            } else {
                apdu7816Resp.setApduResp(selectFileApduResp);
            }
            sequencer.addStep(apdu7816Req, apdu7816Resp);

            SapMessage transferCardReaderStatusReq =
                    new SapMessage(SapMessage.ID_TRANSFER_CARD_READER_STATUS_REQ);

            SapMessage transferCardReaderStatusResp =
                    new SapMessage(SapMessage.ID_TRANSFER_CARD_READER_STATUS_RESP);
            transferCardReaderStatusResp.setResultCode(SapMessage.RESULT_OK);
            sequencer.addStep(transferCardReaderStatusReq, transferCardReaderStatusResp);

            SapMessage setTransportProtocolReq =
                    new SapMessage(SapMessage.ID_SET_TRANSPORT_PROTOCOL_REQ);
            setTransportProtocolReq.setTransportProtocol(0x01); // T=1

            SapMessage setTransportProtocolResp =
                    new SapMessage(SapMessage.ID_SET_TRANSPORT_PROTOCOL_RESP);
            setTransportProtocolResp.setResultCode(SapMessage.RESULT_OK);
            sequencer.addStep(setTransportProtocolReq, setTransportProtocolResp);

            SapMessage disconnectReq = new SapMessage(SapMessage.ID_DISCONNECT_REQ);

            SapMessage disconnectResp = new SapMessage(SapMessage.ID_DISCONNECT_RESP);
            sequencer.addStep(disconnectReq, disconnectResp);

            assertTrue(sequencer.run());
        } catch (IOException e) {
            Log.e(TAG, "IOException", e);
        }
    }

    /**
     * This test fails if the apdu request generates a response before the reset request is handled.
     * This happens if the reference ril is used in test mode.
     */
    public void testSapServerResetWhileWritingApdu() {
        mContext = this.getContext();
        byte[] dummyBytes = {1, 2, 3, 4};
        int index;

        try {

            SapSequencer sequencer = new SapSequencer();
            if(rilTestModeEnabled) {
                sequencer.testModeEnable(true);
            }

            /* Build a default init sequence */
            buildDefaultInitSeq(sequencer);

            SapMessage apduReq = new SapMessage(SapMessage.ID_TRANSFER_APDU_REQ);
            apduReq.setApdu(dummyBytes);

            //
            // Expect no response as we send a SIM_RESET before the write APDU
            // completes.
            // TODO: Consider adding a real response, and add an optional flag.
            //
            SapMessage apduResp = null;
            index = sequencer.addStep(apduReq, apduResp);

            SapMessage resetReq = new SapMessage(SapMessage.ID_RESET_SIM_REQ);
            SapMessage resetResp = new SapMessage(SapMessage.ID_RESET_SIM_RESP);
            resetResp.setResultCode(SapMessage.RESULT_OK);
            sequencer.addSubStep(index, resetReq, resetResp);

            SapMessage statusInd = new SapMessage(SapMessage.ID_STATUS_IND);
            statusInd.setStatusChange(SapMessage.STATUS_CARD_RESET);
            sequencer.addSubStep(index, null, statusInd);

            assertTrue(sequencer.run());
        } catch (IOException e) {
            Log.e(TAG, "IOException", e);
        }
    }

    /**
     * Test that SapServer can disconnect based on a disconnect intent.
     * TODO: This test could validate the timeout values.
     * TODO: We need to add a IAction and add an action to a step, to be able to send
     *       the disconnect intent at the right time.
     */
    public void testSapServerTimeouts() {
        Intent sapDisconnectIntent = new Intent(SapServer.SAP_DISCONNECT_ACTION);
        sapDisconnectIntent.putExtra(
                SapServer.SAP_DISCONNECT_TYPE_EXTRA, SapMessage.DISC_IMMEDIATE);
        mContext = this.getContext();

        try {

            SapSequencer sequencer = new SapSequencer();
            if(rilTestModeEnabled) {
                sequencer.testModeEnable(true);
            }
            /* Build a default init sequence */
            buildDefaultInitSeq(sequencer);

            SapMessage disconnectReq = new SapMessage(SapMessage.ID_DISCONNECT_REQ);
            SapMessage disconnectResp = new SapMessage(SapMessage.ID_DISCONNECT_RESP);

            SapMessage resetResp = new SapMessage(SapMessage.ID_RESET_SIM_RESP);
            resetResp.setResultCode(SapMessage.RESULT_OK);

            int index = sequencer.addStep(disconnectReq, disconnectResp);

            assertTrue(sequencer.run());

            mContext.sendBroadcast(sapDisconnectIntent);

        } catch (IOException e) {
            Log.e(TAG, "IOException", e);
        }
    }
    public void testSapServerTimeoutsActionDiscIntent() {

    }

    public class SapSequencer implements Callback {

        private final static int MSG_ID_TIMEOUT = 0x01;
        private final static int TIMEOUT_VALUE = 100*2000; // ms
        private ArrayList<SeqStep> sequence = null;
        private HandlerThread handlerThread = null;
        private Handler messageHandler = null;

        private SapServer sapServer = null;

        private PipedInputStream inStream = null; // Used to write requests to SapServer
        private PipedOutputStream outStream = null; // Used to read commands from the SapServer


        public class SeqStep {
            public ArrayList<SapMessage> requests = null;
            public ArrayList<SapMessage> responses = null;
            public int index = 0; // Requests with same index are executed in
                                  // parallel without waiting for a response
            public SeqStep(SapMessage request, SapMessage response) {
                requests = new ArrayList<SapMessage>();
                responses = new ArrayList<SapMessage>();
                this.requests.add(request);
                this.responses.add(response);
            }

            public void add(SapMessage request, SapMessage response) {
                this.requests.add(request);
                this.responses.add(response);
            }

            /**
             * Examine if the step has any expected response.
             * @return true if one or more of the responses are != null. False otherwise.
             */
            public boolean hasResponse() {
                if(responses == null)
                    return false;
                for(SapMessage response : responses) {
                    if(response != null)
                        return true;
                }
                return false;
            }
        }

        public SapSequencer() throws IOException {
            /* Setup the looper thread to handle messages */
            handlerThread = new HandlerThread("SapTestTimeoutHandler",
                    android.os.Process.THREAD_PRIORITY_BACKGROUND);
            handlerThread.start();
            Looper testLooper = handlerThread.getLooper();
            messageHandler = new Handler(testLooper, this);

            /* Initialize members */
            sequence = new ArrayList<SeqStep>();

            /* Create a SapServer. Fake the BtSocket using piped input/output streams*/
            inStream = new PipedInputStream(8092);
            outStream = new PipedOutputStream();
            sapServer = new SapServer(null, mContext, new PipedInputStream(outStream, 8092),
                    new PipedOutputStream(inStream));
            sapServer.start();
        }

        /* TODO:
         *  - Add support for actions ?
         *  */

        /**
         * Enable/Disable test mode during the next connect request.
         * @param enable
         */
        public void testModeEnable(boolean enable) {
            if(enable)
                sapServer.setTestMode(SapMessage.TEST_MODE_ENABLE);
            else
                sapServer.setTestMode(SapMessage.TEST_MODE_DISABLE);
        }

        /**
         * Add a test step to the sequencer
         * @param request The request to send to the SAP server
         * @param response The response to EXPECT from the SAP server
         * @return The created step index, which can be used when adding events or actions.
         */
        public int addStep(SapMessage request, SapMessage response) {
            // TODO: should we add a step trigger? (in stead of just executing in sequence)
            SeqStep newStep = new SeqStep(request, response);
            sequence.add(newStep);
            return sequence.indexOf(newStep);
        }

        /**
         * Add a sub-step to a sequencer step. All requests added to the same index will be send to
         * the SapServer in the order added before listening for the response.
         * The response order is not validated - hence for each response received the entire list of
         * responses in the step will be searched for a match.
         * @param index the index returned from addStep() to which the sub-step is to be added.
         * @param request The request to send to the SAP server
         * @param response The response to EXPECT from the SAP server
         */
        public void addSubStep(int index, SapMessage request, SapMessage response) {
            SeqStep step = sequence.get(index);
            step.add(request, response);
        }


        /**
         * Run the sequence, by writing a request and wait for a response. Validate the response
         * is either the expected response or one of the expected events.
         * @return
         */
        public boolean run() throws IOException {
            SapMessage inMsg = null;
            boolean done;
            for(SeqStep step : sequence) {

                /* Write all requests - if any */
                if(step.requests != null) {
                    for(SapMessage request : step.requests) {
                        if(request != null) {
                            Log.i(TAG, "Writing request: " +
                                    SapMessage.getMsgTypeName(request.getMsgType()));
                            writeSapMessage(request, false); // write the message without flushing
                        }
                    }
                    writeSapMessage(null, true); /* flush the pipe */
                }

                /* Handle and validate all responses - if any */
                if(step.hasResponse() == true) {
                    done = false;
                    boolean foundMatch = false;
                    SapMessage responseMatch;
                    while(!done) {
                        for(SapMessage response : step.responses) {
                            if(response != null)
                                Log.i(TAG, "Waiting for the response: " +
                                        SapMessage.getMsgTypeName(response.getMsgType()));
                        }
                        inMsg = readSapMessage();
                        if(inMsg != null)
                            Log.i(TAG, "Read message: " +
                                    SapMessage.getMsgTypeName(inMsg.getMsgType()));
                        else
                            assertTrue("Failed to read message.", false);

                        responseMatch = null;
                        for(SapMessage response : step.responses) {
                            if(response != null
                                    && inMsg.getMsgType() == response.getMsgType()
                                    && compareSapMessages(inMsg, response) == true) {
                                foundMatch = true;
                                responseMatch = response;
                                break;
                            }
                        }

                        if(responseMatch != null)
                            step.responses.remove(responseMatch);

                        /* If we are expecting no more responses for this step, continue. */
                        if(step.hasResponse() != true) {
                            done = true;
                        }
                        /* Ensure what we received was expected */
                        assertTrue("wrong message received.", foundMatch);
                    }
                }
            }
            return true;
        }

        private void startTimer() {
            Message timeoutMessage = messageHandler.obtainMessage(MSG_ID_TIMEOUT);
            messageHandler.sendMessageDelayed(timeoutMessage, TIMEOUT_VALUE);
        }

        private void stopTimer() {
            messageHandler.removeMessages(MSG_ID_TIMEOUT);
        }

        /**
         * Compare two messages by comparing each member variable
         * @param received message
         * @param expected message
         * @return true if equal, else false
         */
        private boolean compareSapMessages(SapMessage received, SapMessage expected) {
            boolean retVal = true;
            if(expected.getCardReaderStatus() != -1 &&
                    received.getCardReaderStatus() != expected.getCardReaderStatus()) {
                Log.i(TAG, "received.getCardReaderStatus() != expected.getCardReaderStatus() "
                        + received.getCardReaderStatus() + " != " + expected.getCardReaderStatus());
                retVal = false;
            }
            if(received.getConnectionStatus() != expected.getConnectionStatus()) {
                Log.i(TAG, "received.getConnectionStatus() != expected.getConnectionStatus() "
                        + received.getConnectionStatus() + " != " + expected.getConnectionStatus());
                retVal = false;
            }
            if(received.getDisconnectionType() != expected.getDisconnectionType()) {
                Log.i(TAG, "received.getDisconnectionType() != expected.getDisconnectionType() "
                        + received.getDisconnectionType() + " != "
                        + expected.getDisconnectionType());
                retVal = false;
            }
            if(received.getMaxMsgSize() != expected.getMaxMsgSize()) {
                Log.i(TAG, "received.getMaxMsgSize() != expected.getMaxMsgSize() "
                        + received.getMaxMsgSize() +" != " + expected.getMaxMsgSize());
                retVal = false;
            }
            if(received.getMsgType() != expected.getMsgType()) {
                Log.i(TAG, "received.getMsgType() != expected.getMsgType() "
                        + received.getMsgType() +" != " + expected.getMsgType());
                retVal = false;
            }
            if(received.getResultCode() != expected.getResultCode()) {
                Log.i(TAG, "received.getResultCode() != expected.getResultCode() "
                        + received.getResultCode() + " != " + expected.getResultCode());
                retVal = false;
            }
            if(received.getStatusChange() != expected.getStatusChange()) {
                Log.i(TAG, "received.getStatusChange() != expected.getStatusChange() "
                        + received.getStatusChange() + " != " + expected.getStatusChange());
                retVal = false;
            }
            if(received.getTransportProtocol() != expected.getTransportProtocol()) {
                Log.i(TAG, "received.getTransportProtocol() != expected.getTransportProtocol() "
                        + received.getTransportProtocol() + " != "
                        + expected.getTransportProtocol());
                retVal = false;
            }
            if(!Arrays.equals(received.getApdu(), expected.getApdu())) {
                Log.i(TAG, "received.getApdu() != expected.getApdu() "
                        + Arrays.toString(received.getApdu()) + " != "
                        + Arrays.toString(expected.getApdu()));
                retVal = false;
            }
            if(!Arrays.equals(received.getApdu7816(), expected.getApdu7816())) {
                Log.i(TAG, "received.getApdu7816() != expected.getApdu7816() "
                        + Arrays.toString(received.getApdu7816()) + " != "
                        + Arrays.toString(expected.getApdu7816()));
                retVal = false;
            }
            if(expected.getApduResp() != null && !Arrays.equals(received.getApduResp(),
                    expected.getApduResp())) {
                Log.i(TAG, "received.getApduResp() != expected.getApduResp() "
                        + Arrays.toString(received.getApduResp()) + " != "
                        + Arrays.toString(expected.getApduResp()));
                retVal = false;
            }
            if(expected.getAtr() != null && !Arrays.equals(received.getAtr(), expected.getAtr())) {
                Log.i(TAG, "received.getAtr() != expected.getAtr() "
                        + Arrays.toString(received.getAtr()) + " != "
                        + Arrays.toString(expected.getAtr()));
                retVal = false;
            }
            return retVal;
        }

        private SapMessage readSapMessage() throws IOException {
            startTimer();
            int requestType = inStream.read();
            Log.i(TAG,"Received message with type: " + SapMessage.getMsgTypeName(requestType));
            SapMessage msg = SapMessage.readMessage(requestType, inStream);
            stopTimer();
            if(requestType != -1) {
                return msg;
            } else
            {
                assertTrue("Reached EOF too early...", false);
            }
            return msg;
        }

        private void writeSapMessage(SapMessage message, boolean flush) throws IOException {
            startTimer();
            if(message != null)
                message.write(outStream);
            if(flush == true)
                outStream.flush();
            stopTimer();
        }

        @Override
        public boolean handleMessage(Message msg) {

            Log.i(TAG,"Handling message ID: " + msg.what);

            switch(msg.what) {
            case MSG_ID_TIMEOUT:
                Log.w(TAG, "Timeout occured!");
                try {
                    inStream.close();
                } catch (IOException e) {
                    Log.e(TAG, "failed to close inStream", e);
                }
                try {
                    outStream.close();
                } catch (IOException e) {
                    Log.e(TAG, "failed to close outStream", e);
                }
                break;
            default:
                /* Message not handled */
                return false;
            }
            return true; // Message handles
        }

    }

}
