package com.android.bluetooth.tests;

import java.io.IOException;
import java.util.ArrayList;
import java.util.concurrent.CountDownLatch;

import javax.obex.ClientSession;
import javax.obex.HeaderSet;
import javax.obex.ObexTransport;
import javax.obex.Operation;
import javax.obex.ServerSession;

import junit.framework.Assert;

import android.content.Context;
import android.hardware.camera2.impl.GetCommand;
import android.os.Handler;
import android.os.Handler.Callback;
import android.os.HandlerThread;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;

public class TestSequencer implements Callback {
    protected static String TAG = "TestSequencer";
    protected static final boolean D = true;

    private final static int MSG_ID_TIMEOUT = 0x01;
    private final static int TIMEOUT_VALUE = 100*2000; // ms
    private ArrayList<SeqStep> mSequence = null;
    private HandlerThread mHandlerThread = null;
    private Handler mMessageHandler = null;
    private ObexTransport mClientTransport;
    private ObexTransport mServerTransport;

    private ClientSession mClientSession = null;
    private ServerSession mServerSession = null;
    public static final int STEP_INDEX_HEADER = 0xF1; /*0xFE*/

    public enum OPTYPE {CONNECT, PUT, GET, SET_PATH, DISCONNECT};

    private ITestSequenceConfigurator mConfigurator = null;

    public TestSequencer(ObexTransport clientTransport, ObexTransport serverTransport,
            ITestSequenceConfigurator configurator)
            throws IOException {
        /* Setup the looper thread to handle timeout messages */
//            mHandlerThread = new HandlerThread("TestTimeoutHandler",
//                      android.os.Process.THREAD_PRIORITY_BACKGROUND);
//            mHandlerThread.start();
//            Looper testLooper = mHandlerThread.getLooper();
//            mMessageHandler = new Handler(testLooper, this);
        //TODO: fix looper cleanup on server - crash after 464 iterations - related to prepare?

        mClientTransport = clientTransport;
        mServerTransport = serverTransport;

        /* Initialize members */
        mSequence = new ArrayList<SeqStep>();
        mConfigurator = configurator;
        Assert.assertNotNull(configurator);
    }

    /**
     * Add a test step to the sequencer.
     * @param type the OBEX operation to perform.
     * @return the created step, which can be decorated before execution.
     */
    public SeqStep addStep(OPTYPE type, ISeqStepValidator validator) {
        SeqStep newStep = new SeqStep(type);
        newStep.mValidator = validator;
        mSequence.add(newStep);
        return newStep;
    }

    /**
     * Add a sub-step to a sequencer step. All requests added to the same index will be send to
     * the SapServer in the order added before listening for the response.
     * The response order is not validated - hence for each response received the entire list of
     * responses in the step will be searched for a match.
     * @param index the index returned from addStep() to which the sub-step is to be added.
     * @param request The request to send to the SAP server
     * @param response The response to EXPECT from the SAP server

    public void addSubStep(int index, SapMessage request, SapMessage response) {
        SeqStep step = sequence.get(index);
        step.add(request, response);
    }*/


    /**
     * Run the sequence.
     * Validate the response is either the expected response or one of the expected events.
     *
     * @return true when done - asserts at error/fail
     */
    public boolean run(Context context) throws IOException {
        CountDownLatch stopLatch = new CountDownLatch(1);
        PowerManager pm = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
        PowerManager.WakeLock wl = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);
        //wl.acquire();
        try {
            /* TODO:
             * First create sequencer to validate using BT-snoop
             * 1) Create the transports (this could include a validation sniffer on each side)
             * 2) Create a server thread with a link to the transport
             * 3) execute the client operation
             * 4) validate response
             *
             * On server:
             * 1) validate the request contains the expected content
             * 2) send response.
             * */

            /* Create the server */
            if(mServerTransport != null) {
                mServerSession = new ServerSession(mServerTransport,
                        mConfigurator.getObexServer(mSequence, stopLatch) , null);
            }

            /* Create the client */
            if(mClientTransport != null) {
                mClientSession = new ClientSession(mClientTransport);

                for(SeqStep step : mSequence) {
                    long stepIndex = mSequence.indexOf(step);

                    Log.i(TAG, "Executing step " + stepIndex + " of type: " + step.mType);

                    switch(step.mType) {
                    case CONNECT: {
                        HeaderSet reqHeaders = step.mReqHeaders;
                        if(reqHeaders == null) {
                            reqHeaders = new HeaderSet();
                        }
                        reqHeaders.setHeader(STEP_INDEX_HEADER, stepIndex);
                        HeaderSet response = mClientSession.connect(reqHeaders);
                        step.validate(response, null);
                        step.clientPostAction(response, null);
                        break;
                    }
                    case GET:{
                        HeaderSet reqHeaders = step.mReqHeaders;
                        if(reqHeaders == null) {
                            reqHeaders = new HeaderSet();
                        }
                        reqHeaders.setHeader(STEP_INDEX_HEADER, stepIndex);
                        Log.i(TAG, "  Starting operation...");
                        Operation op = mClientSession.get(reqHeaders);
                        Log.i(TAG, "  Operation done...");
                        step.validate(null, op);
                        step.clientPostAction(null, op);
                        break;
                    }
                    case PUT: {
                        HeaderSet reqHeaders = step.mReqHeaders;
                        if(reqHeaders == null) {
                            reqHeaders = new HeaderSet();
                        }
                        reqHeaders.setHeader(STEP_INDEX_HEADER, stepIndex);
                        Operation op = mClientSession.put(reqHeaders);
                        step.validate(null, op);
                        step.clientPostAction(null, op);
                        break;
                    }
                    case SET_PATH: {
                        HeaderSet reqHeaders = step.mReqHeaders;
                        if(reqHeaders == null) {
                            reqHeaders = new HeaderSet();
                        }
                        reqHeaders.setHeader(STEP_INDEX_HEADER, stepIndex);
                        try{
                            HeaderSet response = mClientSession.setPath(reqHeaders,
                                    step.mSetPathBackup, step.mSetPathCreate);;
                            Log.i(TAG,"Received setPath response...");
                            step.validate(response, null);
                            step.clientPostAction(response, null);
                        } catch (IOException e) {
                            Log.e(TAG, "Error getting response code", e);
                        }
                        break;
                    }
                    case DISCONNECT: {
                        Log.i(TAG,"Requesting disconnect...");
                        HeaderSet reqHeaders = step.mReqHeaders;
                        if(reqHeaders == null) {
                            reqHeaders = new HeaderSet();
                        }
                        reqHeaders.setHeader(STEP_INDEX_HEADER, stepIndex);
                        try{
                            HeaderSet response = mClientSession.disconnect(reqHeaders);
                            Log.i(TAG,"Received disconnect response...");
                            step.validate(response, null);
                            step.clientPostAction(response, null);
                        } catch (IOException e) {
                            Log.e(TAG, "Error getting response code", e);
                        }
                        break;
                    }
                    default:
                        Assert.assertTrue("Unknown type: " + step.mType, false);
                        break;

                    }
                }
                mClientSession.close();
            }
            /* All done, close down... */
            if(mServerSession != null) {
                boolean interrupted = false;
                do {
                    try {
                        interrupted = false;
                        Log.i(TAG,"Waiting for stopLatch signal...");
                        stopLatch.await();
                    } catch (InterruptedException e) {
                        Log.w(TAG,e);
                        interrupted = true;
                    }
                } while (interrupted == true);
                Log.i(TAG,"stopLatch signal received closing down...");
                try {
                    interrupted = false;
                    Log.i(TAG,"  Sleep 50ms to allow disconnect signal to be send before closing.");
                    Thread.sleep(50);
                } catch (InterruptedException e) {
                    Log.w(TAG,e);
                    interrupted = true;
                }
                mServerSession.close();
            }
            // this will close the I/O streams as well.
        } finally {
            //wl.release();
        }
        return true;
    }

    public void shutdown() {
//            mMessageHandler.removeCallbacksAndMessages(null);
//            mMessageHandler.quit();
//            mMessageHandler = null;
    }


//        private void startTimer() {
//            Message timeoutMessage = mMessageHandler.obtainMessage(MSG_ID_TIMEOUT);
//            mMessageHandler.sendMessageDelayed(timeoutMessage, TIMEOUT_VALUE);
//        }
//
//        private void stopTimer() {
//            mMessageHandler.removeMessages(MSG_ID_TIMEOUT);
//        }

    @Override
    public boolean handleMessage(Message msg) {

        Log.i(TAG,"Handling message ID: " + msg.what);

        switch(msg.what) {
        case MSG_ID_TIMEOUT:
            Log.w(TAG, "Timeout occured!");
/*                try {
                    //inStream.close();
                } catch (IOException e) {
                    Log.e(TAG, "failed to close inStream", e);
                }
                try {
                    //outStream.close();
                } catch (IOException e) {
                    Log.e(TAG, "failed to close outStream", e);
                }*/
            break;
        default:
            /* Message not handled */
            return false;
        }
        return true; // Message handles
    }



}

