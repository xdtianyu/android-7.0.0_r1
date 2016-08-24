/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.print.cts;

import android.os.ParcelFileDescriptor;
import android.print.PageRange;
import android.print.PrintAttributes;
import android.print.PrintAttributes.Margins;
import android.print.PrintAttributes.MediaSize;
import android.print.PrintAttributes.Resolution;
import android.print.PrintDocumentAdapter;
import android.print.PrintDocumentAdapter.LayoutResultCallback;
import android.print.PrintDocumentAdapter.WriteResultCallback;
import android.print.PrintDocumentInfo;
import android.print.PrintJobInfo;
import android.print.PrinterCapabilitiesInfo;
import android.print.PrinterId;
import android.print.PrinterInfo;
import android.print.cts.services.CustomPrintOptionsActivity;
import android.print.cts.services.FirstPrintService;
import android.print.cts.services.PrintServiceCallbacks;
import android.print.cts.services.PrinterDiscoverySessionCallbacks;
import android.print.cts.services.SecondPrintService;
import android.print.cts.services.StubbablePrinterDiscoverySession;
import android.printservice.PrintJob;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiSelector;
import android.util.Log;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.util.ArrayList;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.assertNull;
import static junit.framework.Assert.assertTrue;
import static junit.framework.Assert.assertFalse;


/**
 * Tests all possible states of print jobs.
 */
public class PrintJobTest extends BasePrintTest {
    private static final String LOG_TAG = "PrintJobTest";

    private static final String PRINTER_NAME = "TestPrinter";

    private final static String VALID_NULL_KEY = "validNullKey";
    private final static String VALID_STRING_KEY = "validStringKey";
    private final static String STRING_VALUE = "string value";
    private final static String INVALID_STRING_KEY = "invalidStringKey";
    private final static String VALID_INT_KEY = "validIntKey";
    private final static int INT_VALUE = 23;
    private final static String INVALID_INT_KEY = "invalidIntKey";

    private final boolean testSuccess[] = new boolean[1];

    /** The printer discovery session used in this test */
    private static StubbablePrinterDiscoverySession mDiscoverySession;

    /**
     * Create a mock {@link PrintDocumentAdapter} that provides one empty page.
     *
     * @return The mock adapter
     */
    private PrintDocumentAdapter createMockPrintDocumentAdapter() {
        final PrintAttributes[] printAttributes = new PrintAttributes[1];

        return createMockPrintDocumentAdapter(
                new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        printAttributes[0] = (PrintAttributes) invocation.getArguments()[1];
                        LayoutResultCallback callback = (LayoutResultCallback) invocation
                                .getArguments()[3];

                        PrintDocumentInfo info = new PrintDocumentInfo.Builder(PRINT_JOB_NAME)
                                .setContentType(PrintDocumentInfo.CONTENT_TYPE_DOCUMENT)
                                .setPageCount(1)
                                .build();

                        callback.onLayoutFinished(info, false);
                        return null;
                    }
                }, new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        Object[] args = invocation.getArguments();
                        PageRange[] pages = (PageRange[]) args[0];
                        ParcelFileDescriptor fd = (ParcelFileDescriptor) args[1];
                        WriteResultCallback callback = (WriteResultCallback) args[3];

                        writeBlankPages(printAttributes[0], fd, pages[0].getStart(),
                                pages[0].getEnd());
                        fd.close();
                        callback.onWriteFinished(pages);
                        return null;
                    }
                }, new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        return null;
                    }
                });
    }

    /**
     * Create a mock {@link PrinterDiscoverySessionCallbacks} that discovers a simple test printer.
     *
     * @return The mock session callbacks
     */
    private PrinterDiscoverySessionCallbacks createFirstMockPrinterDiscoverySessionCallbacks() {
        return createMockPrinterDiscoverySessionCallbacks(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) {
                // Get the session.
                mDiscoverySession = ((PrinterDiscoverySessionCallbacks) invocation.getMock())
                        .getSession();

                if (mDiscoverySession.getPrinters().isEmpty()) {
                    PrinterId printerId =
                            mDiscoverySession.getService().generatePrinterId(PRINTER_NAME);
                    PrinterInfo.Builder printer = new PrinterInfo.Builder(
                            mDiscoverySession.getService().generatePrinterId(PRINTER_NAME),
                            PRINTER_NAME, PrinterInfo.STATUS_IDLE);

                    printer.setCapabilities(new PrinterCapabilitiesInfo.Builder(printerId)
                            .addMediaSize(MediaSize.ISO_A4, true)
                            .addResolution(new Resolution("300x300", "300dpi", 300, 300), true)
                            .setColorModes(PrintAttributes.COLOR_MODE_COLOR,
                                    PrintAttributes.COLOR_MODE_COLOR)
                            .setMinMargins(new Margins(0, 0, 0, 0)).build());

                    ArrayList<PrinterInfo> printers = new ArrayList<>(1);
                    printers.add(printer.build());

                    mDiscoverySession.addPrinters(printers);
                }
                return null;
            }
        }, null, null, new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                return null;
            }
        }, null, null, new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                // Take a note onDestroy was called.
                onPrinterDiscoverySessionDestroyCalled();
                return null;
            }
        });
    }

    private interface PrintJobTestFn {
        void onPrintJobQueued(PrintJob printJob) throws Exception;
    }

    /**
     * Create mock service callback for a session. Once the job is queued the test function is
     * called.
     *
     * @param sessionCallbacks The callbacks of the session
     * @param printJobTest test function to call
     */
    private PrintServiceCallbacks createFirstMockPrinterServiceCallbacks(
            final PrinterDiscoverySessionCallbacks sessionCallbacks,
            final PrintJobTestFn printJobTest) {
        return createMockPrintServiceCallbacks(
                new Answer<PrinterDiscoverySessionCallbacks>() {
                    @Override
                    public PrinterDiscoverySessionCallbacks answer(InvocationOnMock invocation) {
                        return sessionCallbacks;
                    }
                }, new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) {
                        PrintJob printJob = (PrintJob) invocation.getArguments()[0];

                        try {
                            printJobTest.onPrintJobQueued(printJob);
                            testSuccess[0] = true;
                        } catch (Exception e) {
                            Log.e(LOG_TAG, "Test function failed", e);
                        }

                        onPrintJobQueuedCalled();

                        return null;
                    }
                }, null);
    }

    /**
     * Make sure that a runnable eventually finishes without throwing a exception.
     *
     * @param r The runnable to run.
     */
    private static void eventually(Runnable r) {
        final long TIMEOUT_MILLS = 5000;
        long start = System.currentTimeMillis();

        while (true) {
            try {
                r.run();
                break;
            } catch (Exception e) {
                if (System.currentTimeMillis() - start < TIMEOUT_MILLS) {
                    Log.e(LOG_TAG, "Ignoring exception as we know that the print spooler does " +
                            "not guarantee to process commands in order", e);
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException e1) {
                        Log.e(LOG_TAG, "Interrupted", e);
                    }
                } else {
                    throw e;
                }
            }
        }
    }

    /**
     * Base test for the print job tests. Starts a print job and executes a testFn once the job is
     * queued.
     *
     * @throws Exception If anything is unexpected.
     */
    private void baseTest(PrintJobTestFn testFn, int testCaseNum)
            throws Exception {
        if (!supportsPrinting()) {
            return;
        }

        testSuccess[0] = false;

        // Create the session of the printers that we will be checking.
        PrinterDiscoverySessionCallbacks sessionCallbacks
                = createFirstMockPrinterDiscoverySessionCallbacks();

        // Create the service callbacks for the first print service.
        PrintServiceCallbacks serviceCallbacks = createFirstMockPrinterServiceCallbacks(
                sessionCallbacks, testFn);

        // Configure the print services.
        FirstPrintService.setCallbacks(serviceCallbacks);

        // We don't use the second service, but we have to still configure it
        SecondPrintService.setCallbacks(createMockPrintServiceCallbacks(null, null, null));

        // Create a print adapter that respects the print contract.
        PrintDocumentAdapter adapter = createMockPrintDocumentAdapter();

        // Start printing.
        print(adapter);

        if (testCaseNum == 0) {
            selectPrinter(PRINTER_NAME);
        }

        clickPrintButton();

        if (testCaseNum == 0) {
            answerPrintServicesWarning(true);
        }

        // Wait for print job to be queued
        waitForServiceOnPrintJobQueuedCallbackCalled(testCaseNum + 1);

        // Wait for discovery session to be destroyed to isolate tests from each other
        waitForPrinterDiscoverySessionDestroyCallbackCalled(1);

        if (!testSuccess[0]) {
            throw new Exception("Did not succeed");
        }
    }

    private static boolean setState(PrintJob job, int state) {
        switch (state) {
            case PrintJobInfo.STATE_QUEUED:
                // queue cannot be set, but is set at the beginning
                return job.isQueued();
            case PrintJobInfo.STATE_STARTED:
                return job.start();
            case PrintJobInfo.STATE_BLOCKED:
                return job.block(null);
            case PrintJobInfo.STATE_COMPLETED:
                return job.complete();
            case PrintJobInfo.STATE_FAILED:
                return job.fail(null);
            case PrintJobInfo.STATE_CANCELED:
                return job.cancel();
            default:
                // not reached
                throw new IllegalArgumentException("Cannot switch to " + state);
        }
    }

    private static boolean isStateTransitionAllowed(int before, int after) {
        switch (before) {
            case PrintJobInfo.STATE_QUEUED:
                switch (after) {
                    case PrintJobInfo.STATE_QUEUED:
                        // queued is not actually set, see setState
                    case PrintJobInfo.STATE_STARTED:
                    case PrintJobInfo.STATE_FAILED:
                    case PrintJobInfo.STATE_CANCELED:
                        return true;
                    default:
                        return false;
                }
            case PrintJobInfo.STATE_STARTED:
                switch (after) {
                    case PrintJobInfo.STATE_QUEUED:
                    case PrintJobInfo.STATE_STARTED:
                        return false;
                    default:
                        return true;
                }
            case PrintJobInfo.STATE_BLOCKED:
                switch (after) {
                    case PrintJobInfo.STATE_STARTED:
                        // blocked -> started == restart
                    case PrintJobInfo.STATE_FAILED:
                    case PrintJobInfo.STATE_CANCELED:
                        return true;
                    default:
                        return false;
                }
            case PrintJobInfo.STATE_COMPLETED:
                return false;
            case PrintJobInfo.STATE_FAILED:
                return false;
            case PrintJobInfo.STATE_CANCELED:
                return false;
            default:
                // not reached
                throw new IllegalArgumentException("Cannot switch from " + before);
        }
    }

    private static void checkState(PrintJob job, int state) {
        eventually(() -> assertEquals(state, job.getInfo().getState()));
        switch (state) {
            case PrintJobInfo.STATE_QUEUED:
                eventually(() -> assertTrue(job.isQueued()));
                break;
            case PrintJobInfo.STATE_STARTED:
                eventually(() -> assertTrue(job.isStarted()));
                break;
            case PrintJobInfo.STATE_BLOCKED:
                eventually(() -> assertTrue(job.isBlocked()));
                break;
            case PrintJobInfo.STATE_COMPLETED:
                eventually(() -> assertTrue(job.isCompleted()));
                break;
            case PrintJobInfo.STATE_FAILED:
                eventually(() -> assertTrue(job.isFailed()));
                break;
            case PrintJobInfo.STATE_CANCELED:
                eventually(() -> assertTrue(job.isCancelled()));
                break;
            default:
                // not reached
                throw new IllegalArgumentException("Cannot check " + state);
        }
    }

    public void testStateTransitions() throws Exception {
        int states[] = new int[] { PrintJobInfo.STATE_QUEUED,
                PrintJobInfo.STATE_STARTED,
                PrintJobInfo.STATE_BLOCKED,
                PrintJobInfo.STATE_COMPLETED,
                PrintJobInfo.STATE_FAILED,
                PrintJobInfo.STATE_CANCELED
        };

        final boolean knownFailures[][] = new boolean[8][8];

        int testCaseNum = 0;

        for (final int state1 : states) {
            for (final int state2 : states) {
                for (final int state3 : states) {
                    // No need to test the same non-transitions twice
                    if (state1 == state2 && state2 == state3) {
                        continue;
                    }

                    // No need to repeat what previously failed
                    if (knownFailures[state1][state2]
                            || knownFailures[state2][state3]) {
                        continue;
                    }

                    // QUEUED does not actually set a state, see setState
                    if (state1 == PrintJobInfo.STATE_QUEUED) {
                        continue;
                    }

                    Log.i(LOG_TAG, "Test " + state1 + " -> " + state2 + " -> " + state3);

                    baseTest(new PrintJobTestFn() {
                        @Override
                        public void onPrintJobQueued(PrintJob printJob) throws Exception {
                            knownFailures[PrintJobInfo.STATE_QUEUED][state1] = true;

                            boolean success = setState(printJob, state1);
                            assertEquals(isStateTransitionAllowed(PrintJobInfo.STATE_QUEUED,
                                    state1), success);
                            if (!success) {
                                return;
                            }
                            checkState(printJob, state1);

                            knownFailures[PrintJobInfo.STATE_QUEUED][state1] = false;

                            knownFailures[state1][state2] = true;

                            success = setState(printJob, state2);
                            assertEquals(isStateTransitionAllowed(state1, state2), success);
                            if (!success) {
                                return;
                            }
                            checkState(printJob, state2);

                            knownFailures[state1][state2] = false;

                            knownFailures[state2][state3] = true;

                            success = setState(printJob, state3);
                            assertEquals(isStateTransitionAllowed(state2, state3), success);
                            if (!success) {
                                return;
                            }
                            checkState(printJob, state3);

                            knownFailures[state2][state3] = false;
                        }
                    }, testCaseNum);

                    testCaseNum++;
                }
            }
        }
    }

    public void testBlockWithReason() throws Exception {
        baseTest(new PrintJobTestFn() {
            @Override
            public void onPrintJobQueued(PrintJob printJob) throws Exception {
                printJob.start();
                checkState(printJob, PrintJobInfo.STATE_STARTED);

                printJob.setStatus(R.string.testStr1);
                eventually(() -> assertEquals(getActivity().getString(R.string.testStr1),
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));

                boolean success = printJob.block("test reason");
                assertTrue(success);
                checkState(printJob, PrintJobInfo.STATE_BLOCKED);
                eventually(() -> assertEquals("test reason",
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));

                success = printJob.block("another reason");
                assertFalse(success);
                checkState(printJob, PrintJobInfo.STATE_BLOCKED);
                eventually(() -> assertEquals("test reason",
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));

                printJob.setStatus(R.string.testStr2);
                eventually(() -> assertEquals(getActivity().getString(R.string.testStr2),
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));
            }
        }, 0);
    }

    public void testFailWithReason() throws Exception {
        baseTest(new PrintJobTestFn() {
            @Override
            public void onPrintJobQueued(PrintJob printJob) throws Exception {
                printJob.start();
                checkState(printJob, PrintJobInfo.STATE_STARTED);

                boolean success = printJob.fail("test reason");
                assertTrue(success);
                checkState(printJob, PrintJobInfo.STATE_FAILED);
                eventually(() -> assertEquals("test reason",
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));

                success = printJob.fail("another reason");
                assertFalse(success);
                checkState(printJob, PrintJobInfo.STATE_FAILED);
                eventually(() -> assertEquals("test reason",
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));
            }
        }, 0);
    }

    public void testTag() throws Exception {
        baseTest(new PrintJobTestFn() {
            @Override
            public void onPrintJobQueued(PrintJob printJob) throws Exception {
                // Default value should be null
                assertNull(printJob.getTag());

                printJob.setTag("testTag");
                eventually(() -> assertEquals("testTag", printJob.getTag()));

                printJob.setTag(null);
                eventually(() -> assertNull(printJob.getTag()));
            }
        }, 0);
    }

    public void testAdvancedOption() throws Exception {
        if (!supportsPrinting()) {
            return;
        }

        testSuccess[0] = false;

        // Create the session of the printers that we will be checking.
        PrinterDiscoverySessionCallbacks sessionCallbacks
                = createFirstMockPrinterDiscoverySessionCallbacks();

        // Create the service callbacks for the first print service.
        PrintServiceCallbacks serviceCallbacks = createFirstMockPrinterServiceCallbacks(
                sessionCallbacks, new PrintJobTestFn() {
                    @Override
                    public void onPrintJobQueued(PrintJob printJob) throws Exception {
                        assertTrue(printJob.hasAdvancedOption(VALID_STRING_KEY));
                        assertEquals(STRING_VALUE, printJob.getAdvancedStringOption(VALID_STRING_KEY));

                        assertFalse(printJob.hasAdvancedOption(INVALID_STRING_KEY));
                        assertNull(printJob.getAdvancedStringOption(INVALID_STRING_KEY));

                        assertTrue(printJob.hasAdvancedOption(VALID_INT_KEY));
                        assertEquals(INT_VALUE, printJob.getAdvancedIntOption(VALID_INT_KEY));

                        assertTrue(printJob.hasAdvancedOption(VALID_NULL_KEY));
                        assertNull(printJob.getAdvancedStringOption(VALID_NULL_KEY));

                        assertFalse(printJob.hasAdvancedOption(INVALID_INT_KEY));
                        assertEquals(0, printJob.getAdvancedIntOption(INVALID_INT_KEY));

                        assertNull(printJob.getAdvancedStringOption(VALID_INT_KEY));
                        assertEquals(0, printJob.getAdvancedIntOption(VALID_STRING_KEY));
                    }
                });

        CustomPrintOptionsActivity.setCallBack(
                new CustomPrintOptionsActivity.CustomPrintOptionsCallback() {
                    @Override
                    public PrintJobInfo executeCustomPrintOptionsActivity(
                            PrintJobInfo printJob, PrinterInfo printer) {
                        PrintJobInfo.Builder printJobBuilder = new PrintJobInfo.Builder(printJob);

                        try {
                            printJobBuilder.putAdvancedOption(null, STRING_VALUE);
                            throw new RuntimeException("Should not be able to use a null key");
                        } catch (NullPointerException e) {
                            // expected
                        }

                        // Second put overrides the first
                        printJobBuilder.putAdvancedOption(VALID_STRING_KEY, "something");
                        printJobBuilder.putAdvancedOption(VALID_STRING_KEY, STRING_VALUE);

                        printJobBuilder.putAdvancedOption(VALID_INT_KEY, "something");
                        printJobBuilder.putAdvancedOption(VALID_INT_KEY, INT_VALUE);

                        printJobBuilder.putAdvancedOption(VALID_NULL_KEY, null);

                        return printJobBuilder.build();
                    }
                });

        // Configure the print services.
        FirstPrintService.setCallbacks(serviceCallbacks);

        // We don't use the second service, but we have to still configure it
        SecondPrintService.setCallbacks(createMockPrintServiceCallbacks(null, null, null));

        // Create a print adapter that respects the print contract.
        PrintDocumentAdapter adapter = createMockPrintDocumentAdapter();

        // Start printing.
        print(adapter);

        selectPrinter(PRINTER_NAME);
        openPrintOptions();
        openCustomPrintOptions();
        clickPrintButton();
        answerPrintServicesWarning(true);

        // Wait for print job to be queued
        waitForServiceOnPrintJobQueuedCallbackCalled(1);

        // Wait for discovery session to be destroyed to isolate tests from each other
        waitForPrinterDiscoverySessionDestroyCallbackCalled(1);

        if (!testSuccess[0]) {
            throw new Exception("Did not succeed");
        }
    }

    public void testOther() throws Exception {
        baseTest(new PrintJobTestFn() {
            @Override
            public void onPrintJobQueued(PrintJob printJob) throws Exception {
                assertNotNull(printJob.getDocument());
                assertNotNull(printJob.getId());
            }
        }, 0);
    }

    public void testSetStatus() throws Exception {
        baseTest(new PrintJobTestFn() {
            @Override
            public void onPrintJobQueued(PrintJob printJob) throws Exception {
                printJob.start();

                printJob.setStatus(R.string.testStr1);
                eventually(() -> assertEquals(getActivity().getString(R.string.testStr1),
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));

                printJob.setStatus("testStr3");
                eventually(() -> assertEquals("testStr3",
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));

                printJob.setStatus(R.string.testStr2);
                eventually(() -> assertEquals(getActivity().getString(R.string.testStr2),
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));

                printJob.setStatus(null);
                eventually(() -> assertNull(
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));

                printJob.block("testStr4");
                eventually(() -> assertEquals("testStr4",
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));

                printJob.setStatus(R.string.testStr2);
                eventually(() -> assertEquals(getActivity().getString(R.string.testStr2),
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));

                printJob.setStatus(0);
                eventually(() -> assertNull(
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));

                printJob.setStatus("testStr3");
                eventually(() -> assertEquals("testStr3",
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));

                printJob.setStatus(-1);
                eventually(() -> assertNull(
                        printJob.getInfo().getStatus(getActivity().getPackageManager())));
            }
        }, 0);
    }
}
