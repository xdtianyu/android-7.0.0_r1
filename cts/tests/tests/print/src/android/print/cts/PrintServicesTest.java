/*
 * Copyright (C) 2015 The Android Open Source Project
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

import android.app.Activity;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.Icon;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
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
import android.printservice.CustomPrinterIconCallback;
import android.printservice.PrintJob;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiSelector;

import junit.framework.AssertionFailedError;

import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.concurrent.TimeoutException;

/**
 * Test the interface from a print service to the print manager
 */
public class PrintServicesTest extends BasePrintTest {
    private static final String PRINTER_NAME = "Test printer";
    private static final int NUM_PAGES = 2;

    /** The print job processed in the test */
    private static PrintJob mPrintJob;

    /** The current progress of #mPrintJob once read from the system */
    private static float mPrintProgress;

    /** Printer under test */
    private static PrinterInfo mPrinter;

    /** The printer discovery session used in this test */
    private static StubbablePrinterDiscoverySession mDiscoverySession;

    /** The current status of #mPrintJob once read from the system */
    private static CharSequence mPrintStatus;

    /** The custom printer icon to use */
    private Icon mIcon;

    /**
     * Create a mock {@link PrintDocumentAdapter} that provides {@link #NUM_PAGES} empty pages.
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
                                .setPageCount(NUM_PAGES)
                                .build();

                        callback.onLayoutFinished(info, false);

                        // Mark layout was called.
                        onLayoutCalled();
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

                        // Mark write was called.
                        onWriteCalled();
                        return null;
                    }
                }, new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        // Mark finish was called.
                        onFinishCalled();
                        return null;
                    }
                });
    }

    /**
     * Create a mock {@link PrinterDiscoverySessionCallbacks} that discovers a single printer with
     * minimal capabilities.
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
                    List<PrinterInfo> printers = new ArrayList<PrinterInfo>();

                    // Add the printer.
                    PrinterId printerId = mDiscoverySession.getService()
                            .generatePrinterId(PRINTER_NAME);

                    PrinterCapabilitiesInfo capabilities = new PrinterCapabilitiesInfo.Builder(
                            printerId)
                                    .setMinMargins(new Margins(200, 200, 200, 200))
                                    .addMediaSize(MediaSize.ISO_A4, true)
                                    .addResolution(new Resolution("300x300", "300x300", 300, 300),
                                            true)
                                    .setColorModes(PrintAttributes.COLOR_MODE_COLOR,
                                            PrintAttributes.COLOR_MODE_COLOR)
                                    .build();

                    Intent infoIntent = new Intent(getActivity(), Activity.class);
                    PendingIntent infoPendingIntent = PendingIntent.getActivity(getActivity(), 0,
                            infoIntent, PendingIntent.FLAG_IMMUTABLE);

                    mPrinter = new PrinterInfo.Builder(printerId, PRINTER_NAME,
                            PrinterInfo.STATUS_IDLE)
                                    .setCapabilities(capabilities)
                                    .setDescription("Minimal capabilities")
                                    .setInfoIntent(infoPendingIntent)
                                    .build();
                    printers.add(mPrinter);

                    mDiscoverySession.addPrinters(printers);
                }
                return null;
            }
        }, null, null, new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                return null;
            }
        }, new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                CustomPrinterIconCallback callback = (CustomPrinterIconCallback) invocation
                        .getArguments()[2];

                if (mIcon != null) {
                    callback.onCustomPrinterIconLoaded(mIcon);
                }
                return null;
            }
        }, null, new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                // Take a note onDestroy was called.
                onPrinterDiscoverySessionDestroyCalled();
                return null;
            }
        });
    }

    /**
     * Get the current progress of #mPrintJob
     *
     * @return The current progress
     * @throws InterruptedException If the thread was interrupted while setting the progress
     */
    private float getProgress() throws InterruptedException {
        final PrintServicesTest synchronizer = PrintServicesTest.this;

        synchronized (synchronizer) {
            Runnable getter = new Runnable() {
                @Override
                public void run() {
                    synchronized (synchronizer) {
                        mPrintProgress = mPrintJob.getInfo().getProgress();

                        synchronizer.notify();
                    }
                }
            };

            (new Handler(Looper.getMainLooper())).post(getter);

            synchronizer.wait();
        }

        return mPrintProgress;
    }

    /**
     * Get the current status of #mPrintJob
     *
     * @return The current status
     * @throws InterruptedException If the thread was interrupted while getting the status
     */
    private CharSequence getStatus() throws InterruptedException {
        final PrintServicesTest synchronizer = PrintServicesTest.this;

        synchronized (synchronizer) {
            Runnable getter = new Runnable() {
                @Override
                public void run() {
                    synchronized (synchronizer) {
                        mPrintStatus = mPrintJob.getInfo()
                                .getStatus(getActivity().getPackageManager());

                        synchronizer.notify();
                    }
                }
            };

            (new Handler(Looper.getMainLooper())).post(getter);

            synchronizer.wait();
        }

        return mPrintStatus;
    }

    /**
     * Check if a print progress is correct.
     *
     * @param desiredProgress The expected @{link PrintProgresses}
     * @throws Exception If anything goes wrong or this takes more than 5 seconds
     */
    private void checkNotification(float desiredProgress,
            CharSequence desiredStatus) throws Exception {
        final long TIMEOUT = 5000;
        final Date start = new Date();

        while ((new Date()).getTime() - start.getTime() < TIMEOUT) {
            if (desiredProgress == getProgress()
                    && desiredStatus.toString().equals(getStatus().toString())) {
                return;
            }

            Thread.sleep(200);
        }

        throw new TimeoutException("Progress or status not updated in " + TIMEOUT + " ms");
    }

    /**
     * Set a new progress and status for #mPrintJob
     *
     * @param progress The new progress to set
     * @param status The new status to set
     * @throws InterruptedException If the thread was interrupted while setting
     */
    private void setProgressAndStatus(final float progress, final CharSequence status)
            throws InterruptedException {
        final PrintServicesTest synchronizer = PrintServicesTest.this;

        synchronized (synchronizer) {
            Runnable completer = new Runnable() {
                @Override
                public void run() {
                    synchronized (synchronizer) {
                        mPrintJob.setProgress(progress);
                        mPrintJob.setStatus(status);

                        synchronizer.notify();
                    }
                }
            };

            (new Handler(Looper.getMainLooper())).post(completer);

            synchronizer.wait();
        }
    }

    /**
     * Progress print job and check the print job state.
     *
     * @param progress How much to progress
     * @param status The status to set
     * @throws Exception If anything goes wrong.
     */
    private void progress(float progress, CharSequence status) throws Exception {
        setProgressAndStatus(progress, status);

        // Check that progress of job is correct
        checkNotification(progress, status);
    }

    /**
     * Create mock service callback for a session.
     *
     * @param sessionCallbacks The callbacks of the sessopm
     */
    private PrintServiceCallbacks createFirstMockPrinterServiceCallbacks(
            final PrinterDiscoverySessionCallbacks sessionCallbacks) {
        return createMockPrintServiceCallbacks(
                new Answer<PrinterDiscoverySessionCallbacks>() {
                    @Override
                    public PrinterDiscoverySessionCallbacks answer(InvocationOnMock invocation) {
                        return sessionCallbacks;
                    }
                },
                new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) {
                        mPrintJob = (PrintJob) invocation.getArguments()[0];
                        mPrintJob.start();
                        onPrintJobQueuedCalled();

                        return null;
                    }
                }, null);
    }

    /**
     * Test that the progress and status is propagated correctly.
     *
     * @throws Exception If anything is unexpected.
     */
    public void testProgress()
            throws Exception {
        if (!supportsPrinting()) {
            return;
        }
        // Create the session callbacks that we will be checking.
        PrinterDiscoverySessionCallbacks sessionCallbacks
                = createFirstMockPrinterDiscoverySessionCallbacks();

        // Create the service callbacks for the first print service.
        PrintServiceCallbacks serviceCallbacks = createFirstMockPrinterServiceCallbacks(
                sessionCallbacks);

        // Configure the print services.
        FirstPrintService.setCallbacks(serviceCallbacks);

        // We don't use the second service, but we have to still configure it
        SecondPrintService.setCallbacks(createMockPrintServiceCallbacks(null, null, null));

        // Create a print adapter that respects the print contract.
        PrintDocumentAdapter adapter = createMockPrintDocumentAdapter();

        // Start printing.
        print(adapter);

        // Wait for write of the first page.
        waitForWriteAdapterCallback(1);

        // Select the printer.
        selectPrinter(PRINTER_NAME);

        // Click the print button.
        clickPrintButton();

        // Answer the dialog for the print service cloud warning
        answerPrintServicesWarning(true);

        // Wait until the print job is queued and #mPrintJob is set
        waitForServiceOnPrintJobQueuedCallbackCalled(1);

        // Progress print job and check for appropriate notifications
        progress(0, "printed 0");
        progress(0.5f, "printed 50");
        progress(1, "printed 100");

        // Call complete from the main thread
        Handler handler = new Handler(Looper.getMainLooper());

        Runnable completer = new Runnable() {
            @Override
            public void run() {
                mPrintJob.complete();
            }
        };

        handler.post(completer);

        // Wait for all print jobs to be handled after which the session destroyed.
        waitForPrinterDiscoverySessionDestroyCallbackCalled(1);
    }

    /**
     * Render a {@link Drawable} into a {@link Bitmap}.
     *
     * @param d the drawable to be rendered
     * @return the rendered bitmap
     */
    private static Bitmap renderDrawable(Drawable d) {
        Bitmap bitmap = Bitmap.createBitmap(d.getIntrinsicWidth(), d.getIntrinsicHeight(),
                Bitmap.Config.ARGB_8888);

        Canvas canvas = new Canvas(bitmap);
        d.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
        d.draw(canvas);

        return bitmap;
    }

    /**
     * Update the printer
     *
     * @param printer the new printer to use
     * @throws InterruptedException If we were interrupted while the printer was updated.
     */
    private void updatePrinter(final PrinterInfo printer)
            throws InterruptedException {
        final PrintServicesTest synchronizer = PrintServicesTest.this;

        synchronized (synchronizer) {
            Runnable updated = new Runnable() {
                @Override
                public void run() {
                    synchronized (synchronizer) {
                        ArrayList<PrinterInfo> printers = new ArrayList<>(1);
                        printers.add(printer);
                        mDiscoverySession.addPrinters(printers);

                        synchronizer.notifyAll();
                    }
                }
            };

            (new Handler(Looper.getMainLooper())).post(updated);

            synchronizer.wait();
        }

        // Update local copy of printer
        mPrinter = printer;
    }

    /**
     * Assert is the printer icon does not match the bitmap. As the icon update might take some time
     * we try up to 5 seconds.
     *
     * @param bitmap The bitmap to match
     */
    private void assertThatIconIs(Bitmap bitmap) {
        final long TIMEOUT = 5000;

        final long startMillis = SystemClock.uptimeMillis();
        while (true) {
            try {
                if (bitmap.sameAs(renderDrawable(mPrinter.loadIcon(getActivity())))) {
                    return;
                }
                final long elapsedMillis = SystemClock.uptimeMillis() - startMillis;
                final long waitMillis = TIMEOUT - elapsedMillis;
                if (waitMillis <= 0) {
                    throw new AssertionFailedError("Icon does not match bitmap");
                }

                // We do not get notified about the icon update, hence wait and try again.
                Thread.sleep(1000);
            } catch (InterruptedException ie) {
               /* ignore */
            }
        }
    }

    /**
     * Test that the icon get be updated.
     *
     * @throws Exception If anything is unexpected.
     */
    public void testUpdateIcon()
            throws Exception {
        if (!supportsPrinting()) {
            return;
        }
        // Create the session callbacks that we will be checking.
        final PrinterDiscoverySessionCallbacks sessionCallbacks
                = createFirstMockPrinterDiscoverySessionCallbacks();

        // Create the service callbacks for the first print service.
        PrintServiceCallbacks serviceCallbacks = createFirstMockPrinterServiceCallbacks(
                sessionCallbacks);

        // Configure the print services.
        FirstPrintService.setCallbacks(serviceCallbacks);

        // We don't use the second service, but we have to still configure it
        SecondPrintService.setCallbacks(createMockPrintServiceCallbacks(null, null, null));

        // Create a print adapter that respects the print contract.
        PrintDocumentAdapter adapter = createMockPrintDocumentAdapter();

        // Start printing.
        print(adapter);

        // Open printer selection dropdown list to display icon on screen
        UiObject destinationSpinner = UiDevice.getInstance(getInstrumentation())
                .findObject(new UiSelector().resourceId(
                        "com.android.printspooler:id/destination_spinner"));
        destinationSpinner.click();

        // Get the print service's icon
        PackageManager packageManager = getActivity().getPackageManager();
        PackageInfo packageInfo = packageManager.getPackageInfo(
                new ComponentName(getActivity(), FirstPrintService.class).getPackageName(), 0);
        ApplicationInfo appInfo = packageInfo.applicationInfo;
        Drawable printServiceIcon = appInfo.loadIcon(packageManager);

        assertThatIconIs(renderDrawable(printServiceIcon));

        // Update icon to resource
        updatePrinter((new PrinterInfo.Builder(mPrinter)).setIconResourceId(R.drawable.red_printer)
                .build());

        assertThatIconIs(renderDrawable(getActivity().getDrawable(R.drawable.red_printer)));

        // Update icon to bitmap
        Bitmap bm = BitmapFactory.decodeResource(getActivity().getResources(),
                R.raw.yellow_printer);
        // Icon will be picked up from the discovery session once setHasCustomPrinterIcon is set
        mIcon = Icon.createWithBitmap(bm);
        updatePrinter((new PrinterInfo.Builder(mPrinter)).setHasCustomPrinterIcon(true).build());

        assertThatIconIs(renderDrawable(mIcon.loadDrawable(getActivity())));
    }
}
