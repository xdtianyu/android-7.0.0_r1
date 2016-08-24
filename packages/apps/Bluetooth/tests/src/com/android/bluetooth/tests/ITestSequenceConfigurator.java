package com.android.bluetooth.tests;

import java.util.ArrayList;
import java.util.concurrent.CountDownLatch;

import javax.obex.ServerRequestHandler;

public interface ITestSequenceConfigurator {

    /** Use this to customize a serverRequestHandler
     * @param sequence A reference to the sequence to handle
     * @param stopLatch a reference to a latch that must be count down, when test completes.
     * @return Reference to the ServerRequestHandler.
     */
    public ServerRequestHandler getObexServer(ArrayList<SeqStep> sequence,
            CountDownLatch stopLatch);
}
