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
import android.print.PrinterCapabilitiesInfo;
import android.print.PrinterId;
import android.print.PrinterInfo;
import android.print.cts.services.FirstPrintService;
import android.print.cts.services.PrintServiceCallbacks;
import android.print.cts.services.PrinterDiscoverySessionCallbacks;
import android.print.cts.services.SecondPrintService;
import android.print.cts.services.StubbablePrinterDiscoverySession;
import android.util.Log;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiSelector;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeoutException;
import java.util.function.Consumer;
import java.util.function.Function;

/**
 * This test verifies changes to the printer capabilities are applied correctly.
 */
public class PrinterCapabilitiesTest extends BasePrintTest {
    private static final String LOG_TAG = "PrinterCapabilitiesTest";
    private static final String PRINTER_NAME = "Test printer";

    private static final Margins DEFAULT_MARGINS = new Margins(0, 0, 0, 0);
    private static final PrintAttributes.Resolution RESOLUTION_300 =
            new PrintAttributes.Resolution("300", "300", 300, 300);
    private static final PrintAttributes.Resolution RESOLUTION_600 =
            new PrintAttributes.Resolution("600", "600", 600, 600);

    /**
     * Generate a new list of printers containing a singer printer with the given media size and
     * status. The other capabilities are default values.
     *
     * @param printerId The id of the printer
     * @param mediaSize The media size to use
     * @param status    The status of th printer
     *
     * @return The list of printers
     */
    private List<PrinterInfo> generatePrinters(PrinterId printerId, MediaSize mediaSize,
            int status) {
        List<PrinterInfo> printers = new ArrayList<>(1);

        PrinterCapabilitiesInfo cap;

        if (mediaSize != null) {
            PrinterCapabilitiesInfo.Builder builder = new PrinterCapabilitiesInfo.Builder(
                    printerId);
            builder.setMinMargins(DEFAULT_MARGINS)
                    .setColorModes(PrintAttributes.COLOR_MODE_COLOR,
                            PrintAttributes.COLOR_MODE_COLOR)
                    .setDuplexModes(PrintAttributes.DUPLEX_MODE_NONE,
                            PrintAttributes.DUPLEX_MODE_NONE)
                    .addMediaSize(mediaSize, true)
                    .addResolution(RESOLUTION_300, true);
            cap = builder.build();
        } else {
            cap = null;
        }

        printers.add(new PrinterInfo.Builder(printerId, PRINTER_NAME, status).setCapabilities(cap)
                .build());

        return printers;
    }

    /**
     * Wait until the print activity requested an update with print attributes matching the media
     * size.
     *
     * @param printAttributes The print attributes container
     * @param mediaSize       The media size to match
     *
     * @throws Exception If anything unexpected happened, e.g. the attributes did not change.
     */
    private void waitForMediaSizeChange(PrintAttributes[] printAttributes, MediaSize mediaSize)
            throws Exception {
        synchronized (PrinterCapabilitiesTest.this) {
            long endTime = System.currentTimeMillis() + OPERATION_TIMEOUT_MILLIS;
            while (printAttributes[0] == null ||
                    !printAttributes[0].getMediaSize().equals(mediaSize)) {
                wait(Math.max(0, endTime - System.currentTimeMillis()));

                if (endTime < System.currentTimeMillis()) {
                    throw new TimeoutException(
                            "Print attributes did not change to " + mediaSize + " in " +
                                    OPERATION_TIMEOUT_MILLIS + " ms. Current attributes"
                                    + printAttributes[0]);
                }
            }
        }
    }

    /**
     * Change the media size of the capabilities of the printer
     *
     * @param session     The session used in the test
     * @param printerId   The printer to change
     * @param mediaSize   The new mediaSize to apply
     * @param isAvailable If the printer should be available or not
     */
    private void changeCapabilities(final StubbablePrinterDiscoverySession session,
            final PrinterId printerId, final MediaSize mediaSize, final boolean isAvailable) {
        getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                session.addPrinters(generatePrinters(printerId, mediaSize, isAvailable ?
                        PrinterInfo.STATUS_IDLE :
                        PrinterInfo.STATUS_UNAVAILABLE));
            }
        });
    }

    /**
     * Wait until the message is shown that indicates that a printer is unavilable.
     *
     * @throws Exception If anything was unexpected.
     */
    private void waitForPrinterUnavailable() throws Exception {
        final String PRINTER_UNAVAILABLE_MSG = "This printer isn't available right now.";

        UiObject message = getUiDevice().findObject(new UiSelector().resourceId(
                "com.android.printspooler:id/message"));
        if (!message.getText().equals(PRINTER_UNAVAILABLE_MSG)) {
            throw new Exception("Wrong message: " + message.getText() + " instead of " +
                    PRINTER_UNAVAILABLE_MSG);
        }
    }

    /**
     * A single test case from {@link #testPrinterCapabilityChange}. Tests a single capability
     * change.
     *
     * @param session     The session used in  the test
     * @param printerId   The ID of the test printer
     * @param availBefore If the printer should be available before the change
     * @param msBefore    The media size of the printer before the change
     * @param availAfter  If the printer should be available after the change
     * @param msAfter     The media size of the printer after the change
     *
     * @throws Exception If anything is unexpected
     */
    private void testCase(final StubbablePrinterDiscoverySession[] session,
            final PrinterId[] printerId, final boolean availBefore, final MediaSize msBefore,
            final boolean availAfter, final MediaSize msAfter) throws Exception {
        if (!supportsPrinting()) {
            return;
        }

        Log.i(LOG_TAG, "Test case: " + availBefore + ", " + msBefore + " -> " + availAfter + ", "
                + msAfter);

        final PrintAttributes[] layoutAttributes = new PrintAttributes[1];
        final PrintAttributes[] writeAttributes = new PrintAttributes[1];

        PrintDocumentAdapter adapter = createMockPrintDocumentAdapter(
                new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        LayoutResultCallback callback = (LayoutResultCallback) invocation
                                .getArguments()[3];
                        PrintDocumentInfo info = new PrintDocumentInfo.Builder(PRINT_JOB_NAME)
                                .setContentType(PrintDocumentInfo.CONTENT_TYPE_DOCUMENT)
                                .setPageCount(1)
                                .build();

                        synchronized (PrinterCapabilitiesTest.this) {
                            layoutAttributes[0] = (PrintAttributes) invocation.getArguments()[1];

                            PrinterCapabilitiesTest.this.notify();
                        }

                        callback.onLayoutFinished(info, true);
                        return null;
                    }
                },
                new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        Object[] args = invocation.getArguments();
                        PageRange[] pages = (PageRange[]) args[0];
                        ParcelFileDescriptor fd = (ParcelFileDescriptor) args[1];
                        WriteResultCallback callback = (WriteResultCallback) args[3];

                        writeBlankPages(layoutAttributes[0], fd, pages[0].getStart(),
                                pages[0].getEnd());
                        fd.close();

                        synchronized (PrinterCapabilitiesTest.this) {
                            writeAttributes[0] = layoutAttributes[0];

                            PrinterCapabilitiesTest.this.notify();
                        }

                        callback.onWriteFinished(pages);
                        return null;
                    }
                }, null);

        // Start printing.
        print(adapter);

        // Select the printer.
        selectPrinter(PRINTER_NAME);

        changeCapabilities(session[0], printerId[0], msBefore, availBefore);
        if (availBefore && msBefore != null) {
            waitForMediaSizeChange(layoutAttributes, msBefore);
            waitForMediaSizeChange(writeAttributes, msBefore);
        } else {
            waitForPrinterUnavailable();
        }

        changeCapabilities(session[0], printerId[0], msAfter, availAfter);
        if (availAfter && msAfter != null) {
            waitForMediaSizeChange(layoutAttributes, msAfter);
            waitForMediaSizeChange(writeAttributes, msAfter);
        } else {
            waitForPrinterUnavailable();
        }

        // Reset printer to default in case discovery session is reused
        changeCapabilities(session[0], printerId[0], MediaSize.NA_LETTER, true);
        waitForMediaSizeChange(layoutAttributes, MediaSize.NA_LETTER);
        waitForMediaSizeChange(writeAttributes, MediaSize.NA_LETTER);

        getUiDevice().pressBack();

        // Wait until PrintActivity is gone
        Thread.sleep(500);
    }

    /**
     * Tests that the printActivity propertly requests (layout and write) updates when the printer
     * capabilities change. This tests all combination of changes.
     *
     * @throws Exception If something is unexpected.
     */
    public void testPrinterCapabilityChange() throws Exception {
        // The session might be reused between test cases, hence carry them from test case to case
        final StubbablePrinterDiscoverySession[] session = new StubbablePrinterDiscoverySession[1];
        final PrinterId[] printerId = new PrinterId[1];

        // Create the session[0] callbacks that we will be checking.
        final PrinterDiscoverySessionCallbacks firstSessionCallbacks =
                createMockPrinterDiscoverySessionCallbacks(new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) {
                        session[0] = ((PrinterDiscoverySessionCallbacks) invocation.getMock())
                                .getSession();

                        printerId[0] = session[0].getService().generatePrinterId(PRINTER_NAME);

                        session[0].addPrinters(generatePrinters(printerId[0], MediaSize.NA_LETTER,
                                PrinterInfo.STATUS_IDLE));
                        return null;
                    }
                }, null, null, null, null, null, new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) {
                        onPrinterDiscoverySessionDestroyCalled();
                        return null;
                    }
                });

        // Create the service callbacks for the first print service.
        PrintServiceCallbacks firstServiceCallbacks = createMockPrintServiceCallbacks(
                new Answer<PrinterDiscoverySessionCallbacks>() {
                    @Override
                    public PrinterDiscoverySessionCallbacks answer(InvocationOnMock invocation) {
                        return firstSessionCallbacks;
                    }
                }, null, null);

        // Configure the print services.
        FirstPrintService.setCallbacks(firstServiceCallbacks);
        SecondPrintService.setCallbacks(createMockPrintServiceCallbacks(null, null, null));

        testCase(session, printerId, false, null, false, null);
        testCase(session, printerId, false, null, false, MediaSize.ISO_A0);
        testCase(session, printerId, false, null, false, MediaSize.ISO_B0);
        testCase(session, printerId, false, null, true, null);
        testCase(session, printerId, false, null, true, MediaSize.ISO_A0);
        testCase(session, printerId, false, null, true, MediaSize.ISO_B0);
        testCase(session, printerId, false, MediaSize.ISO_A0, false, null);
        testCase(session, printerId, false, MediaSize.ISO_A0, false, MediaSize.ISO_A0);
        testCase(session, printerId, false, MediaSize.ISO_A0, false, MediaSize.ISO_B0);
        testCase(session, printerId, false, MediaSize.ISO_A0, true, null);
        testCase(session, printerId, false, MediaSize.ISO_A0, true, MediaSize.ISO_A0);
        testCase(session, printerId, false, MediaSize.ISO_A0, true, MediaSize.ISO_B0);
        testCase(session, printerId, true, null, false, null);
        testCase(session, printerId, true, null, false, MediaSize.ISO_B0);
        testCase(session, printerId, true, null, true, null);
        testCase(session, printerId, true, null, true, MediaSize.ISO_B0);
        testCase(session, printerId, true, MediaSize.ISO_A0, false, null);
        testCase(session, printerId, true, MediaSize.ISO_A0, false, MediaSize.ISO_A0);
        testCase(session, printerId, true, MediaSize.ISO_A0, false, MediaSize.ISO_B0);
        testCase(session, printerId, true, MediaSize.ISO_A0, true, null);
        testCase(session, printerId, true, MediaSize.ISO_A0, true, MediaSize.ISO_A0);
        testCase(session, printerId, true, MediaSize.ISO_A0, true, MediaSize.ISO_B0);

        waitForPrinterDiscoverySessionDestroyCallbackCalled(1);
    }

    /**
     * Run a runnable and expect and exception of a certain type.
     *
     * @param r The runnable to run
     * @param expectedClass The expected exception type
     */
    private void assertException(Runnable r, Class<? extends RuntimeException> expectedClass) {
        try {
            r.run();
        } catch (Exception e) {
            if (e.getClass().isAssignableFrom(expectedClass)) {
                return;
            } else {
                throw new AssertionError("Expected: " + expectedClass.getName() + ", got: "
                        + e.getClass().getName());
            }
        }

        throw new AssertionError("No exception thrown");
    }

    /**
     * That that you cannot create illegal PrinterCapabilityInfos.
     *
     * @throws Exception If anything is unexpected
     */
    public void testIllegalPrinterCapabilityInfos() throws Exception {
        if (!supportsPrinting()) {
            return;
        }

        final PrinterDiscoverySessionCallbacks firstSessionCallbacks =
                createMockPrinterDiscoverySessionCallbacks(new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) {
                        StubbablePrinterDiscoverySession session =
                                ((PrinterDiscoverySessionCallbacks)
                                        invocation.getMock()).getSession();

                        PrinterId printerId = session.getService().generatePrinterId(PRINTER_NAME);

                        // printerId need to be set
                        assertException(() -> new PrinterCapabilitiesInfo.Builder(null),
                                IllegalArgumentException.class);

                        // All capability fields (beside duplex) need to be initialized:
                        // Test no color
                        assertException(() ->
                                        (new PrinterCapabilitiesInfo.Builder(printerId))
                                                .setMinMargins(DEFAULT_MARGINS)
                                                .addMediaSize(MediaSize.ISO_A4, true)
                                                .addResolution(RESOLUTION_300, true).build(),
                                IllegalStateException.class);
                        // Test bad colors
                        assertException(() ->
                                        (new PrinterCapabilitiesInfo.Builder(printerId))
                                                .setColorModes(0xffff,
                                                        PrintAttributes.COLOR_MODE_MONOCHROME),
                                IllegalArgumentException.class);
                        // Test bad duplex mode
                        assertException(() ->
                                        (new PrinterCapabilitiesInfo.Builder(printerId))
                                                .setDuplexModes(0xffff,
                                                        PrintAttributes.DUPLEX_MODE_NONE),
                                IllegalArgumentException.class);
                        // Test no mediasize
                        assertException(() ->
                                        (new PrinterCapabilitiesInfo.Builder(printerId))
                                                .setColorModes(PrintAttributes.COLOR_MODE_COLOR,
                                                        PrintAttributes.COLOR_MODE_COLOR)
                                                .setMinMargins(DEFAULT_MARGINS)
                                                .addResolution(RESOLUTION_300, true).build(),
                                IllegalStateException.class);
                        // Test no default mediasize
                        assertException(() ->
                                        (new PrinterCapabilitiesInfo.Builder(printerId))
                                                .setColorModes(PrintAttributes.COLOR_MODE_COLOR,
                                                        PrintAttributes.COLOR_MODE_COLOR)
                                                .setMinMargins(DEFAULT_MARGINS)
                                                .addMediaSize(MediaSize.ISO_A4, false)
                                                .addResolution(RESOLUTION_300, true).build(),
                                IllegalStateException.class);
                        // Test two default mediasizes
                        assertException(() ->
                                        (new PrinterCapabilitiesInfo.Builder(printerId))
                                                .addMediaSize(MediaSize.ISO_A4, true)
                                                .addMediaSize(MediaSize.ISO_A5, true),
                                IllegalArgumentException.class);
                        // Test no resolution
                        assertException(() ->
                                        (new PrinterCapabilitiesInfo.Builder(printerId))
                                                .setColorModes(PrintAttributes.COLOR_MODE_COLOR,
                                                        PrintAttributes.COLOR_MODE_COLOR)
                                                .setMinMargins(DEFAULT_MARGINS)
                                                .addMediaSize(MediaSize.ISO_A4, true).build(),
                                IllegalStateException.class);
                        // Test no default resolution
                        assertException(() ->
                                        (new PrinterCapabilitiesInfo.Builder(printerId))
                                                .setColorModes(PrintAttributes.COLOR_MODE_COLOR,
                                                        PrintAttributes.COLOR_MODE_COLOR)
                                                .setMinMargins(DEFAULT_MARGINS)
                                                .addMediaSize(MediaSize.ISO_A4, true)
                                                .addResolution(RESOLUTION_300, false).build(),
                                IllegalStateException.class);
                        // Test two default resolutions
                        assertException(() ->
                                        (new PrinterCapabilitiesInfo.Builder(printerId))
                                                .addResolution(RESOLUTION_300, true)
                                                .addResolution(RESOLUTION_600, true),
                                IllegalArgumentException.class);

                        onPrinterDiscoverySessionCreateCalled();
                        return null;
                    }
                }, null, null, null, null, null, new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) {
                        onPrinterDiscoverySessionDestroyCalled();
                        return null;
                    }
                });

        // Create the service callbacks for the first print service.
        PrintServiceCallbacks firstServiceCallbacks = createMockPrintServiceCallbacks(
                new Answer<PrinterDiscoverySessionCallbacks>() {
                    @Override
                    public PrinterDiscoverySessionCallbacks answer(InvocationOnMock invocation) {
                        return firstSessionCallbacks;
                    }
                }, null, null);

        // Configure the print services.
        FirstPrintService.setCallbacks(firstServiceCallbacks);
        SecondPrintService.setCallbacks(createMockPrintServiceCallbacks(null, null, null));

        PrintDocumentAdapter adapter = createMockPrintDocumentAdapter(null, null, null);

        // Start printing.
        print(adapter);

        waitForPrinterDiscoverySessionCreateCallbackCalled();

        getActivity().finish();

        waitForPrinterDiscoverySessionDestroyCallbackCalled(1);
    }

    /**
     * That that you can use all sane legal PrinterCapabilityInfos.
     *
     * @throws Exception If anything is unexpected
     */
    public void testSanePrinterCapabilityInfos() throws Exception {
        if (!supportsPrinting()) {
            return;
        }

        final PrinterDiscoverySessionCallbacks firstSessionCallbacks =
                createMockPrinterDiscoverySessionCallbacks(new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) {
                        StubbablePrinterDiscoverySession session =
                                ((PrinterDiscoverySessionCallbacks)
                                        invocation.getMock()).getSession();

                        MediaSize[] mediaSizes = {MediaSize.ISO_A0, MediaSize.ISO_A0,
                                MediaSize.ISO_A1};
                        Resolution[] resolutions = {RESOLUTION_300, RESOLUTION_300,
                                RESOLUTION_600};
                        int[] colorModes = {PrintAttributes.COLOR_MODE_MONOCHROME,
                                PrintAttributes.COLOR_MODE_COLOR};
                        int[] duplexModes = {PrintAttributes.DUPLEX_MODE_NONE,
                                PrintAttributes.DUPLEX_MODE_LONG_EDGE,
                                PrintAttributes.DUPLEX_MODE_SHORT_EDGE};

                        ArrayList<PrinterInfo> printers = new ArrayList<>();
                        for (int mediaSizeIndex = 1; mediaSizeIndex < mediaSizes.length;
                             mediaSizeIndex++) {
                            for (int resolutionIndex = 1; resolutionIndex < mediaSizes.length;
                                 resolutionIndex++) {
                                for (int colorIndex = 1; colorIndex < colorModes.length;
                                     colorIndex++) {
                                    for (int duplexIndex = 1; duplexIndex < duplexModes.length;
                                         duplexIndex++) {
                                        PrinterId printerId = session.getService()
                                                .generatePrinterId(Integer.valueOf(printers.size())
                                                        .toString());

                                        PrinterCapabilitiesInfo.Builder b =
                                                new PrinterCapabilitiesInfo.Builder(printerId);

                                        for (int i = 0; i < mediaSizeIndex; i++) {
                                            b.addMediaSize(mediaSizes[i], i == mediaSizeIndex - 1);
                                        }

                                        for (int i = 0; i < resolutionIndex; i++) {
                                            b.addResolution(resolutions[i],
                                                    i == resolutionIndex - 1);
                                        }

                                        int allColors = 0;
                                        for (int i = 0; i < colorIndex; i++) {
                                            allColors |= colorModes[i];
                                        }
                                        b.setColorModes(allColors, colorModes[colorIndex - 1]);

                                        int allDuplexModes = 0;
                                        for (int i = 0; i < duplexIndex; i++) {
                                            allDuplexModes |= duplexModes[i];
                                        }
                                        b.setDuplexModes(allDuplexModes,
                                                duplexModes[duplexIndex - 1]);

                                        printers.add((new PrinterInfo.Builder(printerId,
                                                Integer.valueOf(printers.size()).toString(),
                                                PrinterInfo.STATUS_IDLE)).setCapabilities(b.build())
                                                .build());
                                    }
                                }
                            }
                        }

                        session.addPrinters(printers);

                        onPrinterDiscoverySessionCreateCalled();
                        return null;
                    }
                }, null, null, null, null, null, new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) {
                        onPrinterDiscoverySessionDestroyCalled();
                        return null;
                    }
                });

        // Create the service callbacks for the first print service.
        PrintServiceCallbacks firstServiceCallbacks = createMockPrintServiceCallbacks(
                new Answer<PrinterDiscoverySessionCallbacks>() {
                    @Override
                    public PrinterDiscoverySessionCallbacks answer(InvocationOnMock invocation) {
                        return firstSessionCallbacks;
                    }
                }, null, null);

        // Configure the print services.
        FirstPrintService.setCallbacks(firstServiceCallbacks);
        SecondPrintService.setCallbacks(createMockPrintServiceCallbacks(null, null, null));

        PrintDocumentAdapter adapter = createMockPrintDocumentAdapter(null, null, null);

        // Start printing.
        print(adapter);

        waitForPrinterDiscoverySessionCreateCallbackCalled();

        getUiDevice().pressBack();

        waitForPrinterDiscoverySessionDestroyCallbackCalled(1);
    }

    /**
     * Base test that performs a print operation with a give PrinterCapabilityInfo and run a test
     * function before finishing.
     *
     * @throws Exception
     */
    private void testPrinterCapabilityInfo(final Function<PrinterId, PrinterCapabilitiesInfo>
            capBuilder, Consumer<PrintAttributes> test) throws Exception {
        if (!supportsPrinting()) {
            return;
        }

        final PrinterDiscoverySessionCallbacks firstSessionCallbacks =
                createMockPrinterDiscoverySessionCallbacks(new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) {
                        StubbablePrinterDiscoverySession session =
                                ((PrinterDiscoverySessionCallbacks)
                                        invocation.getMock()).getSession();

                        PrinterId printerId = session.getService()
                                .generatePrinterId(PRINTER_NAME);

                        ArrayList<PrinterInfo> printers = new ArrayList<>();
                        printers.add((new PrinterInfo.Builder(printerId, PRINTER_NAME,
                                PrinterInfo.STATUS_IDLE))
                                .setCapabilities(capBuilder.apply(printerId)).build());

                        session.addPrinters(printers);

                        onPrinterDiscoverySessionCreateCalled();
                        return null;
                    }
                }, null, null, null, null, null, new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) {
                        onPrinterDiscoverySessionDestroyCalled();
                        return null;
                    }
                });

        // Create the service callbacks for the first print service.
        PrintServiceCallbacks firstServiceCallbacks = createMockPrintServiceCallbacks(
                new Answer<PrinterDiscoverySessionCallbacks>() {
                    @Override
                    public PrinterDiscoverySessionCallbacks answer(InvocationOnMock invocation) {
                        return firstSessionCallbacks;
                    }
                }, null, null);

        // Configure the print services.
        FirstPrintService.setCallbacks(firstServiceCallbacks);
        SecondPrintService.setCallbacks(createMockPrintServiceCallbacks(null, null, null));

        final PrintAttributes[] layoutAttributes = new PrintAttributes[1];

        PrintDocumentAdapter adapter = createMockPrintDocumentAdapter(
                new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        LayoutResultCallback callback = (LayoutResultCallback) invocation
                                .getArguments()[3];
                        PrintDocumentInfo info = new PrintDocumentInfo.Builder(PRINT_JOB_NAME)
                                .setPageCount(1)
                                .build();
                        layoutAttributes[0] = (PrintAttributes) invocation.getArguments()[1];

                        callback.onLayoutFinished(info, true);
                        return null;
                    }
                },
                new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        Object[] args = invocation.getArguments();
                        PageRange[] pages = (PageRange[]) args[0];
                        ParcelFileDescriptor fd = (ParcelFileDescriptor) args[1];
                        WriteResultCallback callback = (WriteResultCallback) args[3];

                        writeBlankPages(layoutAttributes[0], fd, pages[0].getStart(),
                                pages[0].getEnd());
                        fd.close();

                        callback.onWriteFinished(pages);
                        return null;
                    }
                }, null);

        // Start printing.
        print(adapter);

        // make sure that options does not crash
        openPrintOptions();

        // Select printer under test
        selectPrinter(PRINTER_NAME);

        clickPrintButton();

        answerPrintServicesWarning(true);

        test.accept(layoutAttributes[0]);

        waitForPrinterDiscoverySessionDestroyCallbackCalled(1);
    }

    /**
     * That that you use a default color that is not in the allowed colors. This is allowed because
     * of historical reasons.
     *
     * @throws Exception If anything is unexpected
     */
    public void testInvalidDefaultColor() throws Exception {
        testPrinterCapabilityInfo(
                (printerId) -> (new PrinterCapabilitiesInfo.Builder(printerId))
                        .addMediaSize(MediaSize.ISO_A4, true)
                        .addResolution(RESOLUTION_300, true)
                        .setColorModes(PrintAttributes.COLOR_MODE_MONOCHROME,
                                PrintAttributes.COLOR_MODE_COLOR).build(),
                (layoutAttributes) -> assertEquals(layoutAttributes.getColorMode(),
                        PrintAttributes.COLOR_MODE_MONOCHROME));
    }

    /**
     * That that you use a default duplex mode that is not in the allowed duplex modes. This is
     * allowed because of historical reasons.
     *
     * @throws Exception If anything is unexpected
     */
    public void testInvalidDefaultDuplexMode() throws Exception {
        testPrinterCapabilityInfo(
                (printerId) -> (new PrinterCapabilitiesInfo.Builder(printerId))
                        .addMediaSize(MediaSize.ISO_A4, true)
                        .addResolution(RESOLUTION_300, true)
                        .setColorModes(PrintAttributes.COLOR_MODE_MONOCHROME,
                                PrintAttributes.COLOR_MODE_MONOCHROME)
                        .setDuplexModes(PrintAttributes.DUPLEX_MODE_LONG_EDGE
                                | PrintAttributes.DUPLEX_MODE_NONE,
                                PrintAttributes.DUPLEX_MODE_SHORT_EDGE).build(),
                (layoutAttributes) -> assertTrue(layoutAttributes.getDuplexMode() ==
                        PrintAttributes.DUPLEX_MODE_LONG_EDGE || layoutAttributes.getDuplexMode() ==
                        PrintAttributes.DUPLEX_MODE_NONE));
    }
}
