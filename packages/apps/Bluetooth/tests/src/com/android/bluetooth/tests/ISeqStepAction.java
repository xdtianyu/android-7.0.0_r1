package com.android.bluetooth.tests;

import java.io.IOException;

import javax.obex.HeaderSet;
import javax.obex.Operation;

public interface ISeqStepAction {

    void execute(SeqStep step, HeaderSet request, Operation op)
            throws IOException;

}
