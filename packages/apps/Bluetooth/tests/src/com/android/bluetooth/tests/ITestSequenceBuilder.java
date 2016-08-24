package com.android.bluetooth.tests;

public interface ITestSequenceBuilder {

    /**
     * Add steps to a sequencer
     * @param sequencer The sequencer the steps will be added to.
     */
    public void build(TestSequencer sequencer);

}
