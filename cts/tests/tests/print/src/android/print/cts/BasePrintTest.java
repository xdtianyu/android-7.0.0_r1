/*
 * Copyright (C) 2014 The Android Open Source Project
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

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.argThat;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.cts.util.SystemUtil;
import android.graphics.pdf.PdfDocument;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.os.LocaleList;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
import android.print.PageRange;
import android.print.PrintAttributes;
import android.print.PrintDocumentAdapter;
import android.print.PrintDocumentAdapter.LayoutResultCallback;
import android.print.PrintDocumentAdapter.WriteResultCallback;
import android.print.PrintManager;
import android.print.PrinterId;
import android.print.cts.services.PrintServiceCallbacks;
import android.print.cts.services.PrinterDiscoverySessionCallbacks;
import android.print.cts.services.StubbablePrinterDiscoverySession;
import android.print.pdf.PrintedPdfDocument;
import android.printservice.CustomPrinterIconCallback;
import android.printservice.PrintJob;
import android.printservice.PrintService;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiObject;
import android.support.test.uiautomator.UiObjectNotFoundException;
import android.support.test.uiautomator.UiSelector;
import android.test.InstrumentationTestCase;
import android.util.DisplayMetrics;
import android.util.Log;

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.mockito.InOrder;
import org.mockito.stubbing.Answer;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.TimeoutException;
/**
 * This is the base class for print tests.
 */
public abstract class BasePrintTest extends InstrumentationTestCase {
    private final static String LOG_TAG = "BasePrintTest";

    protected static final long OPERATION_TIMEOUT_MILLIS = 60000;
    private static final String PRINT_SPOOLER_PACKAGE_NAME = "com.android.printspooler";
    protected static final String PRINT_JOB_NAME = "Test";
    private static final String PM_CLEAR_SUCCESS_OUTPUT = "Success";
    private static final String COMMAND_LIST_ENABLED_IME_COMPONENTS = "ime list -s";
    private static final String COMMAND_PREFIX_ENABLE_IME = "ime enable ";
    private static final String COMMAND_PREFIX_DISABLE_IME = "ime disable ";
    private static final int CURRENT_USER_ID = -2; // Mirrors UserHandle.USER_CURRENT

    private static PrintDocumentActivity sActivity;
    private UiDevice mUiDevice;

    /**
     * Return the UI device
     *
     * @return the UI device
     */
    public UiDevice getUiDevice() {
        return mUiDevice;
    }

    private LocaleList mOldLocale;

    private CallCounter mCancelOperationCounter;
    private CallCounter mLayoutCallCounter;
    private CallCounter mWriteCallCounter;
    private CallCounter mFinishCallCounter;
    private CallCounter mPrintJobQueuedCallCounter;
    private CallCounter mCreateSessionCallCounter;
    private CallCounter mDestroySessionCallCounter;
    private static CallCounter sDestroyActivityCallCounter = new CallCounter();
    private static CallCounter sCreateActivityCallCounter = new CallCounter();

    private String[] mEnabledImes;

    private String[] getEnabledImes() throws IOException {
        List<String> imeList = new ArrayList<>();

        ParcelFileDescriptor pfd = getInstrumentation().getUiAutomation()
                .executeShellCommand(COMMAND_LIST_ENABLED_IME_COMPONENTS);
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(new FileInputStream(pfd.getFileDescriptor())))) {

            String line;
            while ((line = reader.readLine()) != null) {
                imeList.add(line);
            }
        }

        String[] imeArray = new String[imeList.size()];
        imeList.toArray(imeArray);

        return imeArray;
    }

    private void disableImes() throws Exception {
        mEnabledImes = getEnabledImes();
        for (String ime : mEnabledImes) {
            String disableImeCommand = COMMAND_PREFIX_DISABLE_IME + ime;
            SystemUtil.runShellCommand(getInstrumentation(), disableImeCommand);
        }
    }

    private void enableImes() throws Exception {
        for (String ime : mEnabledImes) {
            String enableImeCommand = COMMAND_PREFIX_ENABLE_IME + ime;
            SystemUtil.runShellCommand(getInstrumentation(), enableImeCommand);
        }
        mEnabledImes = null;
    }

    @Override
    protected void runTest() throws Throwable {
        // Do nothing if the device does not support printing.
        if (supportsPrinting()) {
            super.runTest();
        }
    }

    @Override
    public void setUp() throws Exception {
        Log.d(LOG_TAG, "setUp()");

        super.setUp();
        if (!supportsPrinting()) {
            return;
        }

        mUiDevice = UiDevice.getInstance(getInstrumentation());

        // Make sure we start with a clean slate.
        Log.d(LOG_TAG, "clearPrintSpoolerData()");
        clearPrintSpoolerData();
        Log.d(LOG_TAG, "disableImes()");
        disableImes();

        // Workaround for dexmaker bug: https://code.google.com/p/dexmaker/issues/detail?id=2
        // Dexmaker is used by mockito.
        System.setProperty("dexmaker.dexcache", getInstrumentation()
                .getTargetContext().getCacheDir().getPath());

        // Set to US locale.
        Log.d(LOG_TAG, "set locale");
        Resources resources = getInstrumentation().getTargetContext().getResources();
        Configuration oldConfiguration = resources.getConfiguration();
        if (!oldConfiguration.getLocales().get(0).equals(Locale.US)) {
            mOldLocale = oldConfiguration.getLocales();
            DisplayMetrics displayMetrics = resources.getDisplayMetrics();
            Configuration newConfiguration = new Configuration(oldConfiguration);
            newConfiguration.setLocale(Locale.US);
            resources.updateConfiguration(newConfiguration, displayMetrics);
        }

        // Initialize the latches.
        Log.d(LOG_TAG, "init counters");
        mCancelOperationCounter = new CallCounter();
        mLayoutCallCounter = new CallCounter();
        mFinishCallCounter = new CallCounter();
        mWriteCallCounter = new CallCounter();
        mFinishCallCounter = new CallCounter();
        mPrintJobQueuedCallCounter = new CallCounter();
        mCreateSessionCallCounter = new CallCounter();
        mDestroySessionCallCounter = new CallCounter();

        // Create the activity for the right locale.
        Log.d(LOG_TAG, "createActivity()");
        createActivity();
        Log.d(LOG_TAG, "setUp() done");
    }

    @Override
    public void tearDown() throws Exception {
        Log.d(LOG_TAG, "tearDown()");

        if (!supportsPrinting()) {
            return;
        }

        // Done with the activity.
        Log.d(LOG_TAG, "finish activity");
        if (!getActivity().isFinishing()) {
            getActivity().finish();
        }

        Log.d(LOG_TAG, "enableImes()");
        enableImes();

        // Restore the locale if needed.
        Log.d(LOG_TAG, "restore locale");
        if (mOldLocale != null) {
            Resources resources = getInstrumentation().getTargetContext().getResources();
            DisplayMetrics displayMetrics = resources.getDisplayMetrics();
            Configuration newConfiguration = new Configuration(resources.getConfiguration());
            newConfiguration.setLocales(mOldLocale);
            mOldLocale = null;
            resources.updateConfiguration(newConfiguration, displayMetrics);
        }

        // Make sure the spooler is cleaned, this also un-approves all services
        Log.d(LOG_TAG, "clearPrintSpoolerData()");
        clearPrintSpoolerData();

        super.tearDown();
        Log.d(LOG_TAG, "tearDown() done");
    }

    protected void print(final PrintDocumentAdapter adapter, final PrintAttributes attributes) {
        // Initiate printing as if coming from the app.
        getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                PrintManager printManager = (PrintManager) getActivity()
                        .getSystemService(Context.PRINT_SERVICE);
                printManager.print("Print job", adapter, attributes);
            }
        });
    }

    protected void print(PrintDocumentAdapter adapter) {
        print(adapter, null);
    }

    protected void onCancelOperationCalled() {
        mCancelOperationCounter.call();
    }

    protected void onLayoutCalled() {
        mLayoutCallCounter.call();
    }

    protected int getWriteCallCount() {
        return mWriteCallCounter.getCallCount();
    }

    protected void onWriteCalled() {
        mWriteCallCounter.call();
    }

    protected void onFinishCalled() {
        mFinishCallCounter.call();
    }

    protected void onPrintJobQueuedCalled() {
        mPrintJobQueuedCallCounter.call();
    }

    protected void onPrinterDiscoverySessionCreateCalled() {
        mCreateSessionCallCounter.call();
    }

    protected void onPrinterDiscoverySessionDestroyCalled() {
        mDestroySessionCallCounter.call();
    }

    protected void waitForCancelOperationCallbackCalled() {
        waitForCallbackCallCount(mCancelOperationCounter, 1,
                "Did not get expected call to onCancel for the current operation.");
    }

    protected void waitForPrinterDiscoverySessionCreateCallbackCalled() {
        waitForCallbackCallCount(mCreateSessionCallCounter, 1,
                "Did not get expected call to onCreatePrinterDiscoverySession.");
    }

    protected void waitForPrinterDiscoverySessionDestroyCallbackCalled(int count) {
        waitForCallbackCallCount(mDestroySessionCallCounter, count,
                "Did not get expected call to onDestroyPrinterDiscoverySession.");
    }

    protected void waitForServiceOnPrintJobQueuedCallbackCalled(int count) {
        waitForCallbackCallCount(mPrintJobQueuedCallCounter, count,
                "Did not get expected call to onPrintJobQueued.");
    }

    protected void waitForAdapterFinishCallbackCalled() {
        waitForCallbackCallCount(mFinishCallCounter, 1,
                "Did not get expected call to finish.");
    }

    protected void waitForLayoutAdapterCallbackCount(int count) {
        waitForCallbackCallCount(mLayoutCallCounter, count,
                "Did not get expected call to layout.");
    }

    protected void waitForWriteAdapterCallback(int count) {
        waitForCallbackCallCount(mWriteCallCounter, count, "Did not get expected call to write.");
    }

    private static void waitForCallbackCallCount(CallCounter counter, int count, String message) {
        try {
            counter.waitForCount(count, OPERATION_TIMEOUT_MILLIS);
        } catch (TimeoutException te) {
            fail(message);
        }
    }

    /**
     * Indicate the print activity was created.
     */
    static void onActivityCreateCalled(PrintDocumentActivity activity) {
        sActivity = activity;
        sCreateActivityCallCounter.call();
    }

    /**
     * Indicate the print activity was destroyed.
     */
    static void onActivityDestroyCalled() {
        sDestroyActivityCallCounter.call();
    }

    /**
     * Get the number of ties the print activity was destroyed.
     *
     * @return The number of onDestroy calls on the print activity.
     */
    protected static int getActivityDestroyCallbackCallCount() {
        return sDestroyActivityCallCounter.getCallCount();
    }

    /**
     * Get the number of ties the print activity was created.
     *
     * @return The number of onCreate calls on the print activity.
     */
    protected static int getActivityCreateCallbackCallCount() {
        return sCreateActivityCallCounter.getCallCount();
    }

    /**
     * Wait until create was called {@code count} times.
     *
     * @param count The number of create calls to expect.
     */
    private static void waitForActivityCreateCallbackCalled(int count) {
        waitForCallbackCallCount(sCreateActivityCallCounter, count,
                "Did not get expected call to create.");
    }

    /**
     * Reset all counters.
     */
    protected void resetCounters() {
        mCancelOperationCounter.reset();
        mLayoutCallCounter.reset();
        mWriteCallCounter.reset();
        mFinishCallCounter.reset();
        mPrintJobQueuedCallCounter.reset();
        mCreateSessionCallCounter.reset();
        mDestroySessionCallCounter.reset();
        sDestroyActivityCallCounter.reset();
        sCreateActivityCallCounter.reset();
    }

    protected void selectPrinter(String printerName) throws UiObjectNotFoundException, IOException {
        try {
            long delay = 100;
            while (true) {
                try {
                    UiObject destinationSpinner = mUiDevice.findObject(new UiSelector().resourceId(
                            "com.android.printspooler:id/destination_spinner"));

                    destinationSpinner.click();

                    // Give spinner some time to expand
                    try {
                        Thread.sleep(delay);
                    } catch (InterruptedException e) {
                        // ignore
                    }

                    // try to select printer
                    UiObject printerOption = mUiDevice
                            .findObject(new UiSelector().text(printerName));
                    printerOption.click();
                } catch (UiObjectNotFoundException e) {
                    Log.e(LOG_TAG, "Could not select printer " + printerName, e);
                }

                // Make sure printer is selected
                if (getUiDevice().hasObject(By.text(printerName))) {
                    break;
                } else {
                    if (delay <= OPERATION_TIMEOUT_MILLIS) {
                        Log.w(LOG_TAG, "Cannot find printer " + printerName + ", retrying.");
                        delay *= 2;
                        continue;
                    } else {
                        throw new UiObjectNotFoundException("Could find printer " + printerName +
                                " even though we retried");
                    }
                }
            }
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected void answerPrintServicesWarning(boolean confirm) throws UiObjectNotFoundException {
        UiDevice uiDevice = UiDevice.getInstance(getInstrumentation());
        UiObject button;
        if (confirm) {
            button = uiDevice.findObject(new UiSelector().resourceId("android:id/button1"));
        } else {
            button = uiDevice.findObject(new UiSelector().resourceId("android:id/button2"));
        }
        button.click();
    }

    protected void changeOrientation(String orientation)
            throws UiObjectNotFoundException, IOException {
        try {
            UiObject orientationSpinner = mUiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/orientation_spinner"));
            orientationSpinner.click();
            UiObject orientationOption = mUiDevice.findObject(new UiSelector().text(orientation));
            orientationOption.click();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected String getOrientation() throws UiObjectNotFoundException, IOException {
        try {
            UiObject orientationSpinner = mUiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/orientation_spinner"));
            return orientationSpinner.getText();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected void changeMediaSize(String mediaSize) throws UiObjectNotFoundException, IOException {
        try {
            UiObject mediaSizeSpinner = mUiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/paper_size_spinner"));
            mediaSizeSpinner.click();
            UiObject mediaSizeOption = mUiDevice.findObject(new UiSelector().text(mediaSize));
            mediaSizeOption.click();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected String getMediaSize() throws UiObjectNotFoundException, IOException {
        try {
            UiObject mediaSizeSpinner = mUiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/paper_size_spinner"));
            return mediaSizeSpinner.getText();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected void changeColor(String color) throws UiObjectNotFoundException, IOException {
        try {
            UiObject colorSpinner = mUiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/color_spinner"));
            colorSpinner.click();
            UiObject colorOption = mUiDevice.findObject(new UiSelector().text(color));
            colorOption.click();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected String getColor() throws UiObjectNotFoundException, IOException {
        try {
            UiObject colorSpinner = mUiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/color_spinner"));
            return colorSpinner.getText();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected void changeDuplex(String duplex) throws UiObjectNotFoundException, IOException {
        try {
            UiObject duplexSpinner = mUiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/duplex_spinner"));
            duplexSpinner.click();
            UiObject duplexOption = mUiDevice.findObject(new UiSelector().text(duplex));
            duplexOption.click();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected String getDuplex() throws UiObjectNotFoundException, IOException {
        try {
            UiObject duplexSpinner = mUiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/duplex_spinner"));
            return duplexSpinner.getText();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected String getCopies() throws UiObjectNotFoundException, IOException {
        try {
            UiObject copies = mUiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/copies_edittext"));
            return copies.getText();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected void clickPrintButton() throws UiObjectNotFoundException, IOException {
        try {
            UiObject printButton = mUiDevice.findObject(new UiSelector().resourceId(
                    "com.android.printspooler:id/print_button"));
            printButton.click();
        } catch (UiObjectNotFoundException e) {
            dumpWindowHierarchy();
            throw e;
        }
    }

    protected void dumpWindowHierarchy() throws IOException {
        ByteArrayOutputStream os = new ByteArrayOutputStream();
        mUiDevice.dumpWindowHierarchy(os);

        Log.w(LOG_TAG, "Window hierarchy:");
        for (String line : os.toString("UTF-8").split("\n")) {
            Log.w(LOG_TAG, line);
        }
    }

    protected PrintDocumentActivity getActivity() {
        return sActivity;
    }

    protected void createActivity() {
        int createBefore = getActivityCreateCallbackCallCount();

        launchActivity(getInstrumentation().getTargetContext().getPackageName(),
                PrintDocumentActivity.class, null);

        waitForActivityCreateCallbackCalled(createBefore + 1);
    }

    protected void openPrintOptions() throws UiObjectNotFoundException {
        UiObject expandHandle = mUiDevice.findObject(new UiSelector().resourceId(
                "com.android.printspooler:id/expand_collapse_handle"));
        expandHandle.click();
    }

    protected void openCustomPrintOptions() throws UiObjectNotFoundException {
        UiObject expandHandle = mUiDevice.findObject(new UiSelector().resourceId(
                "com.android.printspooler:id/more_options_button"));
        expandHandle.click();
    }

    protected void clearPrintSpoolerData() throws Exception {
        assertTrue("failed to clear print spooler data",
                SystemUtil.runShellCommand(getInstrumentation(), String.format(
                        "pm clear --user %d %s", CURRENT_USER_ID, PRINT_SPOOLER_PACKAGE_NAME))
                        .contains(PM_CLEAR_SUCCESS_OUTPUT));
    }

    protected void verifyLayoutCall(InOrder inOrder, PrintDocumentAdapter mock,
            PrintAttributes oldAttributes, PrintAttributes newAttributes,
            final boolean forPreview) {
        inOrder.verify(mock).onLayout(eq(oldAttributes), eq(newAttributes),
                any(CancellationSignal.class), any(LayoutResultCallback.class), argThat(
                        new BaseMatcher<Bundle>() {
                            @Override
                            public boolean matches(Object item) {
                                Bundle bundle = (Bundle) item;
                                return forPreview == bundle.getBoolean(
                                        PrintDocumentAdapter.EXTRA_PRINT_PREVIEW);
                            }

                            @Override
                            public void describeTo(Description description) {
                                /* do nothing */
                            }
                        }));
    }

    protected PrintDocumentAdapter createMockPrintDocumentAdapter(Answer<Void> layoutAnswer,
            Answer<Void> writeAnswer, Answer<Void> finishAnswer) {
        // Create a mock print adapter.
        PrintDocumentAdapter adapter = mock(PrintDocumentAdapter.class);
        if (layoutAnswer != null) {
            doAnswer(layoutAnswer).when(adapter).onLayout(any(PrintAttributes.class),
                    any(PrintAttributes.class), any(CancellationSignal.class),
                    any(LayoutResultCallback.class), any(Bundle.class));
        }
        if (writeAnswer != null) {
            doAnswer(writeAnswer).when(adapter).onWrite(any(PageRange[].class),
                    any(ParcelFileDescriptor.class), any(CancellationSignal.class),
                    any(WriteResultCallback.class));
        }
        if (finishAnswer != null) {
            doAnswer(finishAnswer).when(adapter).onFinish();
        }
        return adapter;
    }

    @SuppressWarnings("unchecked")
    protected PrinterDiscoverySessionCallbacks createMockPrinterDiscoverySessionCallbacks(
            Answer<Void> onStartPrinterDiscovery, Answer<Void> onStopPrinterDiscovery,
            Answer<Void> onValidatePrinters, Answer<Void> onStartPrinterStateTracking,
            Answer<Void> onRequestCustomPrinterIcon, Answer<Void> onStopPrinterStateTracking,
            Answer<Void> onDestroy) {
        PrinterDiscoverySessionCallbacks callbacks = mock(PrinterDiscoverySessionCallbacks.class);

        doCallRealMethod().when(callbacks).setSession(any(StubbablePrinterDiscoverySession.class));
        when(callbacks.getSession()).thenCallRealMethod();

        if (onStartPrinterDiscovery != null) {
            doAnswer(onStartPrinterDiscovery).when(callbacks).onStartPrinterDiscovery(
                    any(List.class));
        }
        if (onStopPrinterDiscovery != null) {
            doAnswer(onStopPrinterDiscovery).when(callbacks).onStopPrinterDiscovery();
        }
        if (onValidatePrinters != null) {
            doAnswer(onValidatePrinters).when(callbacks).onValidatePrinters(
                    any(List.class));
        }
        if (onStartPrinterStateTracking != null) {
            doAnswer(onStartPrinterStateTracking).when(callbacks).onStartPrinterStateTracking(
                    any(PrinterId.class));
        }
        if (onRequestCustomPrinterIcon != null) {
            doAnswer(onRequestCustomPrinterIcon).when(callbacks).onRequestCustomPrinterIcon(
                    any(PrinterId.class), any(CancellationSignal.class),
                    any(CustomPrinterIconCallback.class));
        }
        if (onStopPrinterStateTracking != null) {
            doAnswer(onStopPrinterStateTracking).when(callbacks).onStopPrinterStateTracking(
                    any(PrinterId.class));
        }
        if (onDestroy != null) {
            doAnswer(onDestroy).when(callbacks).onDestroy();
        }

        return callbacks;
    }

    protected PrintServiceCallbacks createMockPrintServiceCallbacks(
            Answer<PrinterDiscoverySessionCallbacks> onCreatePrinterDiscoverySessionCallbacks,
            Answer<Void> onPrintJobQueued, Answer<Void> onRequestCancelPrintJob) {
        final PrintServiceCallbacks service = mock(PrintServiceCallbacks.class);

        doCallRealMethod().when(service).setService(any(PrintService.class));
        when(service.getService()).thenCallRealMethod();

        if (onCreatePrinterDiscoverySessionCallbacks != null) {
            doAnswer(onCreatePrinterDiscoverySessionCallbacks).when(service)
                    .onCreatePrinterDiscoverySessionCallbacks();
        }
        if (onPrintJobQueued != null) {
            doAnswer(onPrintJobQueued).when(service).onPrintJobQueued(any(PrintJob.class));
        }
        if (onRequestCancelPrintJob != null) {
            doAnswer(onRequestCancelPrintJob).when(service).onRequestCancelPrintJob(
                    any(PrintJob.class));
        }

        return service;
    }

    protected void writeBlankPages(PrintAttributes constraints, ParcelFileDescriptor output,
            int fromIndex, int toIndex) throws IOException {
        PrintedPdfDocument document = new PrintedPdfDocument(getActivity(), constraints);
        final int pageCount = toIndex - fromIndex + 1;
        for (int i = 0; i < pageCount; i++) {
            PdfDocument.Page page = document.startPage(i);
            document.finishPage(page);
        }
        FileOutputStream fos = new FileOutputStream(output.getFileDescriptor());
        document.writeTo(fos);
        document.close();
    }

    protected static final class CallCounter {
        private final Object mLock = new Object();

        private int mCallCount;

        public void call() {
            synchronized (mLock) {
                mCallCount++;
                mLock.notifyAll();
            }
        }

        public int getCallCount() {
            synchronized (mLock) {
                return mCallCount;
            }
        }

        public void reset() {
            synchronized (mLock) {
                mCallCount = 0;
            }
        }

        public void waitForCount(int count, long timeoutMillis) throws TimeoutException {
            synchronized (mLock) {
                final long startTimeMillis = SystemClock.uptimeMillis();
                while (mCallCount < count) {
                    try {
                        final long elapsedTimeMillis = SystemClock.uptimeMillis() - startTimeMillis;
                        final long remainingTimeMillis = timeoutMillis - elapsedTimeMillis;
                        if (remainingTimeMillis <= 0) {
                            throw new TimeoutException();
                        }
                        mLock.wait(timeoutMillis);
                    } catch (InterruptedException ie) {
                        /* ignore */
                    }
                }
            }
        }
    }


    /**
     * Make {@code printerName} the default printer by adding it to the history of printers by
     * printing once.
     *
     * @param adapter The {@link PrintDocumentAdapter} used
     * @throws Exception If the printer could not be made default
     */
    protected void makeDefaultPrinter(PrintDocumentAdapter adapter, String printerName)
            throws Exception {
        // Perform a full print operation on the printer
        Log.d(LOG_TAG, "print");
        print(adapter);
        Log.d(LOG_TAG, "waitForWriteAdapterCallback");
        waitForWriteAdapterCallback(1);
        Log.d(LOG_TAG, "selectPrinter");
        selectPrinter(printerName);
        Log.d(LOG_TAG, "clickPrintButton");
        clickPrintButton();
        Log.d(LOG_TAG, "answerPrintServicesWarning");
        answerPrintServicesWarning(true);
        Log.d(LOG_TAG, "waitForPrinterDiscoverySessionDestroyCallbackCalled");
        waitForPrinterDiscoverySessionDestroyCallbackCalled(1);

        // Switch to new activity, which should now use the default printer
        Log.d(LOG_TAG, "getActivity().finish()");
        getActivity().finish();

        Log.d(LOG_TAG, "createActivity");
        createActivity();
    }

    protected boolean supportsPrinting() {
        return getInstrumentation().getContext().getPackageManager()
                .hasSystemFeature(PackageManager.FEATURE_PRINTING);
    }
}
