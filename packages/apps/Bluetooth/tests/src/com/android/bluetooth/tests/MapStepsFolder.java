package com.android.bluetooth.tests;

import java.io.IOException;

import javax.obex.HeaderSet;
import javax.obex.Operation;
import javax.obex.ResponseCodes;

import junit.framework.Assert;
import android.util.Log;

import com.android.bluetooth.map.BluetoothMapAppParams;
import com.android.bluetooth.map.BluetoothMapFolderElement;
import com.android.bluetooth.tests.TestSequencer.OPTYPE;

public class MapStepsFolder {
    private final static String TAG = "MapStepsFolder";
    /**
     * Request and expect the following folder structure:
     * root
     *   telecom
     *     msg
     *       inbox
     *       outbox
     *       draft
     *       sent
     *       deleted
     *
     * The order in which they occur in the listing will not matter.
     * @param sequencer
     */
    protected static void addGoToMsgFolderSteps(TestSequencer sequencer) {
        SeqStep step;
        //BluetoothMapFolderElement rootDir = new BluetoothMapFolderElement("root", null);

        // MAP Get Folder Listing Steps
        // The telecom folder
        step = sequencer.addStep(OPTYPE.GET, null);
        HeaderSet hs = new HeaderSet();
        hs.setHeader(HeaderSet.TYPE, MapObexLevelTest.TYPE_GET_FOLDER_LISTING);
        step.mReqHeaders = hs;
        step.mValidator = new MapBuildFolderStructurValidator(1, null);

        step = sequencer.addStep(OPTYPE.SET_PATH, ObexTest.getResponsecodevalidator());
        hs = new HeaderSet();
        hs.setHeader(HeaderSet.NAME, "telecom");
        step.mReqHeaders = hs;
        step.mClientPostAction = new MapSetClientFolder("telecom");


        // The msg folder
        step = sequencer.addStep(OPTYPE.GET, null);
        hs = new HeaderSet();
        hs.setHeader(HeaderSet.TYPE, MapObexLevelTest.TYPE_GET_FOLDER_LISTING);
        step.mReqHeaders = hs;
        step.mValidator = new MapBuildFolderStructurValidator(1, null);

        step = sequencer.addStep(OPTYPE.SET_PATH, ObexTest.getResponsecodevalidator());
        hs = new HeaderSet();
        hs.setHeader(HeaderSet.NAME, "msg");
        step.mReqHeaders = hs;
        step.mClientPostAction = new MapSetClientFolder("msg");

        // The msg folder
        step = sequencer.addStep(OPTYPE.GET, null);
        hs = new HeaderSet();
        hs.setHeader(HeaderSet.TYPE, MapObexLevelTest.TYPE_GET_FOLDER_LISTING);
        step.mReqHeaders = hs;
        step.mValidator = new MapBuildFolderStructurValidator(5, buildDefaultFolderStructure());
    }

    /**
     * Sets the current folder on the client, to the folder name specified in the constructor.
     * TODO: Could be extended to be able to navigate back and forth in the folder structure.
     */
    private static class MapSetClientFolder implements ISeqStepAction {
        final String mFolderName;
        public MapSetClientFolder(String folderName) {
            super();
            this.mFolderName = folderName;
        }
        @Override
        public void execute(SeqStep step, HeaderSet request, Operation op)
                throws IOException {
            MapBuildFolderStructurValidator.sCurrentFolder =
                    MapBuildFolderStructurValidator.sCurrentFolder.getSubFolder(mFolderName);
            Assert.assertNotNull(MapBuildFolderStructurValidator.sCurrentFolder);
            Log.i(TAG, "MapSetClientFolder(): Current path: " +
                    MapBuildFolderStructurValidator.sCurrentFolder.getFullPath());
        }
    }

    /* Functions to validate results */
    private static class MapBuildFolderStructurValidator implements ISeqStepValidator {

        final int mExpectedListingSize;
        static BluetoothMapFolderElement sCurrentFolder = null;
        final BluetoothMapFolderElement mExpectedFolderElement;

        public MapBuildFolderStructurValidator(int mExpectedListingSize,
                BluetoothMapFolderElement folderElement) {
            super();
            if(sCurrentFolder == null) {
                sCurrentFolder = new BluetoothMapFolderElement("root", null);
            }
            this.mExpectedListingSize = mExpectedListingSize;
            this.mExpectedFolderElement = folderElement;
        }


        @Override
        public boolean validate(SeqStep step, HeaderSet response, Operation op)
                throws IOException {
            Assert.assertNotNull(op);
            op.noBodyHeader();
            try {
                // For some odd reason, the request will not be send before we start to read the
                // reply data, hence we need to do this first?
                sCurrentFolder.appendSubfolders(op.openInputStream());
                response = op.getReceivedHeader();
                byte[] appParamsRaw = (byte[])response.getHeader(HeaderSet.APPLICATION_PARAMETER);
                Assert.assertNotNull(appParamsRaw);
                BluetoothMapAppParams appParams;
                appParams = new BluetoothMapAppParams(appParamsRaw);
                Assert.assertNotNull(appParams);
                if(mExpectedFolderElement != null) {
                    // Recursively compare
                    Assert.assertTrue(mExpectedFolderElement.compareTo(sCurrentFolder.getRoot())
                            == 0);
                }
                int responseCode = op.getResponseCode();
                Assert.assertEquals(ResponseCodes.OBEX_HTTP_OK, responseCode);
                op.close();
            } catch (Exception e) {
                Log.e(TAG,"",e);
                Assert.fail();
            }
            return true;
        }

    }


    private static BluetoothMapFolderElement buildDefaultFolderStructure(){
        BluetoothMapFolderElement root =
                new BluetoothMapFolderElement("root", null); // This will be the root element
        BluetoothMapFolderElement tmpFolder;
        tmpFolder = root.addFolder("telecom"); // root/telecom
        tmpFolder = tmpFolder.addFolder("msg");         // root/telecom/msg
        tmpFolder.addFolder("inbox");                   // root/telecom/msg/inbox
        tmpFolder.addFolder("outbox");                  // root/telecom/msg/outbox
        tmpFolder.addFolder("sent");                    // root/telecom/msg/sent
        tmpFolder.addFolder("deleted");                 // root/telecom/msg/deleted
        tmpFolder.addFolder("draft");                   // root/telecom/msg/draft
        return root;
    }


}
