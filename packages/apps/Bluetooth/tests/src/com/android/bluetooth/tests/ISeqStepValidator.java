package com.android.bluetooth.tests;

import java.io.IOException;

import javax.obex.HeaderSet;
import javax.obex.Operation;

/**
 * Interface to validate test step result
 */
public interface ISeqStepValidator {
    boolean validate(SeqStep step, HeaderSet response, Operation op)
            throws IOException;

}
