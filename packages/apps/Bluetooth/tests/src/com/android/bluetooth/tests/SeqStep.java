package com.android.bluetooth.tests;

import java.io.IOException;
import java.util.ArrayList;

import javax.obex.HeaderSet;
import javax.obex.ObexPacket;
import javax.obex.Operation;

import junit.framework.Assert;

import com.android.bluetooth.tests.TestSequencer.OPTYPE;

public class SeqStep {
    /**
     * Test step class to define the operations to be tested.
     * Some of the data in these test steps will be modified during
     * test - e.g. the HeaderSets will be modified to enable SRM
     * and/or carry test information
     */
    /* Operation type - Connect, Get, Put etc. */
    public OPTYPE mType;
    /* The headers to send in the request - and validate on server side */
    public HeaderSet mReqHeaders = null;
    /* The headers to send in the response - and validate on client side */
    public HeaderSet mResHeaders = null;
    /* Use SRM */
    public boolean mUseSrm = false;
    /* The amount of data to include in the body */
    public ObexTestParams mParams = null;
    /* The offset into the data where the un-pause signal is to be sent */
    public int mUnPauseOffset = -1;
    /* The offset into the data where the Abort request is to be sent */
    public int mAbortOffset = -1;
    /* The side to perform Abort */
    public boolean mServerSideAbout = false;
    /* The ID of the test step */
    private int mId;

    public boolean mSetPathBackup = false; /* bit 0 in flags */
    public boolean mSetPathCreate = false; /* Inverse of bit 1 in flags */


    public ISeqStepValidator mValidator = null;
    public ISeqStepAction mServerPreAction = null;
    public ISeqStepAction mClientPostAction = null;

    /* Arrays to hold expected sequence of request/response packets. */
    public ArrayList<ObexPacket> mRequestPackets = null;
    public ArrayList<ObexPacket> mResponsePackets = null;

    public int index = 0; /* requests with same index are executed in parallel
                             (without waiting for a response) */

    public SeqStep(OPTYPE type) {
        mRequestPackets = new ArrayList<ObexPacket>();
        mResponsePackets = new ArrayList<ObexPacket>();
        mType = type;
    }

    public boolean validate(HeaderSet response, Operation op) throws IOException {
        Assert.assertNotNull(mValidator);
        return mValidator.validate(this, response, op);
    }

    public void serverPreAction(HeaderSet request, Operation op) throws IOException {
        if(mServerPreAction != null) {
            mServerPreAction.execute(this, request, op);
        }
    }

    public void clientPostAction(HeaderSet response, Operation op) throws IOException {
        if(mClientPostAction != null) {
            mClientPostAction.execute(this, response, op);
        }
    }


    /* TODO: Consider to build these automatically based on the operations
     *       to be performed. Validate using utility functions - not strict
     *       binary compare.
     *
     *       OR simply remove!*/
    public void addObexPacketSet(ObexPacket request, ObexPacket response) {
        mRequestPackets.add(request);
        mResponsePackets.add(response);
    }
}
