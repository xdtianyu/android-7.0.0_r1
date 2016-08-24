package com.android.bluetooth.tests;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.text.ParseException;
import java.util.Arrays;

import javax.obex.HeaderSet;
import javax.obex.Operation;
import javax.obex.ResponseCodes;

import junit.framework.Assert;
import android.util.Log;

import com.android.bluetooth.map.BluetoothMapAppParams;
import com.android.bluetooth.map.BluetoothMapConvoListing;
import com.android.bluetooth.map.BluetoothMapConvoListingElement;
import com.android.bluetooth.map.BluetoothMapFolderElement;
import com.android.bluetooth.tests.TestSequencer.OPTYPE;

public class MapStepsConvo {
    private static final String TAG = "MapStepsConvo";


    protected static void addConvoListingSteps(TestSequencer sequencer) {
        SeqStep step;
        final int count = 5;

        // TODO: As we use the default message database, these tests will fail if the
        //       database has any content.
        //       To cope with this for now, the validation is disabled.

        /* Request the number of messages */
        step = addConvoListingStep(sequencer,
                0 /*maxListCount*/,
                -1 /*listStartOffset*/,
                null /*activityBegin*/,
                null /*activityEnd*/,
                -1 /*readStatus*/,
                null /*recipient*/,
                new MapConvoListValidator()
                /*new MapConvoListValidator(MapTestData.TEST_NUM_CONTACTS)validator*/);
        /* Add messages and contacts for the entire sequence of tests */
        step.mServerPreAction = new MapTestData.MapAddSmsMessages(count);

        /* Request the XML for all conversations */
        step = addConvoListingStep(sequencer,
                -1 /*maxListCount*/,
                -1 /*listStartOffset*/,
                null /*activityBegin*/,
                null /*activityEnd*/,
                -1 /*readStatus*/,
                null /*recipient*/,
                /*nearly impossible to validate due to auto assigned ID values*/
                new MapConvoListValidator()
                /*new MapConvoListValidator(MapTestData.TEST_NUM_CONTACTS)validator*/);

        step = addConvoListingStep(sequencer,
                2 /*maxListCount*/,
                -1 /*listStartOffset*/,
                null /*activityBegin*/,
                null /*activityEnd*/,
                -1 /*readStatus*/,
                null /*recipient*/,
                /*nearly impossible to validate due to auto assigned ID values*/
                new MapConvoListValidator()
                /*new MapConvoListValidator(MapTestData.TEST_NUM_CONTACTS)validator*/);

        step = addConvoListingStep(sequencer,
                2 /*maxListCount*/,
                1 /*listStartOffset*/,
                null /*activityBegin*/,
                null /*activityEnd*/,
                -1 /*readStatus*/,
                null /*recipient*/,
                /*nearly impossible to validate due to auto assigned ID values*/
                new MapConvoListValidator()
                /*new MapConvoListValidator(MapTestData.TEST_NUM_CONTACTS)validator*/);

        step = addConvoListingStep(sequencer,
                3 /*maxListCount*/,
                2 /*listStartOffset*/,
                null /*activityBegin*/,
                null /*activityEnd*/,
                -1 /*readStatus*/,
                null /*recipient*/,
                /*nearly impossible to validate due to auto assigned ID values*/
                new MapConvoListValidator()
                /*new MapConvoListValidator(MapTestData.TEST_NUM_CONTACTS)validator*/);

        step = addConvoListingStep(sequencer,
                5 /*maxListCount*/,
                1 /*listStartOffset*/,
                MapTestData.TEST_ACTIVITY_BEGIN_STRING /*activityBegin*/,
                null /*activityEnd*/,
                -1 /*readStatus*/,
                null /*recipient*/,
                /*nearly impossible to validate due to auto assigned ID values*/
                new MapConvoListValidator()
                /*new MapConvoListValidator(MapTestData.TEST_NUM_CONTACTS)validator*/);

        step = addConvoListingStep(sequencer,
                5 /*maxListCount*/,
                0 /*listStartOffset*/,
                MapTestData.TEST_ACTIVITY_BEGIN_STRING /*activityBegin*/,
                MapTestData.TEST_ACTIVITY_END_STRING /*activityEnd*/,
                -1 /*readStatus*/,
                null /*recipient*/,
                /*nearly impossible to validate due to auto assigned ID values*/
                new MapConvoListValidator()
                /*new MapConvoListValidator(MapTestData.TEST_NUM_CONTACTS)validator*/);

        step = addConvoListingStep(sequencer,
                5 /*maxListCount*/,
                1 /*listStartOffset*/,
                MapTestData.TEST_ACTIVITY_BEGIN_STRING /*activityBegin*/,
                null /*activityEnd*/,
                2/* read only */ /*readStatus*/,
                null /*recipient*/,
                /*nearly impossible to validate due to auto assigned ID values*/
                new MapConvoListValidator()
                /*new MapConvoListValidator(MapTestData.TEST_NUM_CONTACTS)validator*/);

        /* TODO: Test the different combinations of filtering */
    }

    /**
     * Use -1 or null to omit value in request
     * @param sequencer
     * @param maxListCount
     * @param listStartOffset
     * @param activityBegin
     * @param activityEnd
     * @param readStatus -1 omit value, 0 = no filtering, 1 = get unread only, 2 = get read only,
     *                   3 = 1+2 - hence get none...
     * @param recipient substring of the recipient name
     * @param validator
     * @return a reference to the step added, for further decoration
     */
    private static SeqStep addConvoListingStep(TestSequencer sequencer, int maxListCount,
            int listStartOffset, String activityBegin, String activityEnd,
            int readStatus, String recipient, ISeqStepValidator validator) {
        SeqStep step;
        BluetoothMapAppParams appParams = new BluetoothMapAppParams();
        try {
            if(activityBegin != null) {
                appParams.setFilterLastActivityBegin(activityBegin);
            }
            if(activityEnd != null) {
                appParams.setFilterLastActivityEnd(activityEnd);
            }
            if(readStatus != -1) {
                appParams.setFilterReadStatus(readStatus);
            }
            if(recipient != null) {
                appParams.setFilterRecipient(recipient);
            }
            if(maxListCount != -1) {
                appParams.setMaxListCount(maxListCount);
            }
            if(listStartOffset != -1) {
                appParams.setStartOffset(listStartOffset);
            }
        } catch (ParseException e) {
            Log.e(TAG, "unable to build appParams", e);
        }
        step = sequencer.addStep(OPTYPE.GET, null);
        HeaderSet hs = new HeaderSet();
        hs.setHeader(HeaderSet.TYPE, MapObexLevelTest.TYPE_GET_CONVO_LISTING);
        try {
            hs.setHeader(HeaderSet.APPLICATION_PARAMETER, appParams.EncodeParams());
        } catch (UnsupportedEncodingException e) {
            Log.e(TAG, "ERROR", e);
            Assert.fail();
        }
        step.mReqHeaders = hs;
        step.mValidator = validator;
        return step;
    }

    /* Functions to validate results */
    private static class MapConvoListValidator implements ISeqStepValidator {

        final BluetoothMapConvoListing mExpectedListing;
        final int mExpectedSize;

        public MapConvoListValidator(BluetoothMapConvoListing listing) {
            this.mExpectedListing = listing;
            this.mExpectedSize = -1;
        }

        public MapConvoListValidator(int convoListingSize) {
            this.mExpectedListing = null;
            this.mExpectedSize = convoListingSize;
        }

        public MapConvoListValidator() {
            this.mExpectedListing = null;
            this.mExpectedSize = -1;
        }

        @Override
        public boolean validate(SeqStep step, HeaderSet response, Operation op)
                throws IOException {
            Assert.assertNotNull(op);
            op.noBodyHeader();
            try {
                // For some odd reason, the request will not be send before we start to read the
                // reply data, hence we need to do this first?
                BluetoothMapConvoListing receivedListing = new BluetoothMapConvoListing();
                receivedListing.appendFromXml(op.openInputStream());
                response = op.getReceivedHeader();
                byte[] appParamsRaw = (byte[])response.getHeader(HeaderSet.APPLICATION_PARAMETER);
                Assert.assertNotNull(appParamsRaw);
                BluetoothMapAppParams appParams;
                appParams = new BluetoothMapAppParams(appParamsRaw);
                Assert.assertNotNull(appParams);
                Assert.assertNotNull(appParams.getDatabaseIdentifier());
                Assert.assertNotSame(BluetoothMapAppParams.INVALID_VALUE_PARAMETER,
                        appParams.getConvoListingSize());
                if(mExpectedSize >= 0) {
                    Assert.assertSame(mExpectedSize, appParams.getConvoListingSize());
                }
                if(mExpectedListing != null) {
                    // Recursively compare
                    Assert.assertTrue(mExpectedListing.equals(receivedListing));
                    Assert.assertSame(mExpectedListing.getList().size(),
                            appParams.getConvoListingSize());
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
}
