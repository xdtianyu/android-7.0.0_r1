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

package com.android.calculator2;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.support.annotation.VisibleForTesting;
import android.util.Log;

import com.hp.creals.CR;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import java.math.BigInteger;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;
import java.util.TimeZone;

/**
 * This implements the calculator evaluation logic.  The underlying expression is constructed and
 * edited with append(), delete(), and clear().  An evaluation an then be started with a call to
 * evaluateAndShowResult() or requireResult().  This starts an asynchronous computation, which
 * requests display of the initial result, when available.  When initial evaluation is complete,
 * it calls the calculator onEvaluate() method.  This occurs in a separate event, possibly quite a
 * bit later.  Once a result has been computed, and before the underlying expression is modified,
 * the getString() method may be used to produce Strings that represent approximations to various
 * precisions.
 *
 * Actual expressions being evaluated are represented as {@link CalculatorExpr}s.
 *
 * The Evaluator owns the expression being edited and all associated state needed for evaluating
 * it.  It provides functionality for saving and restoring this state.  However the current
 * CalculatorExpr is exposed to the client, and may be directly accessed after cancelling any
 * in-progress computations by invoking the cancelAll() method.
 *
 * When evaluation is requested, we invoke the eval() method on the CalculatorExpr from a
 * background AsyncTask.  A subsequent getString() callback returns immediately, though it may
 * return a result containing placeholder ' ' characters.  If we had to return palceholder
 * characters, we start a background task, which invokes the onReevaluate() callback when it
 * completes.  In either case, the background task computes the appropriate result digits by
 * evaluating the constructive real (CR) returned by CalculatorExpr.eval() to the required
 * precision.
 *
 * We cache the best decimal approximation we have already computed.  We compute generously to
 * allow for some scrolling without recomputation and to minimize the chance of digits flipping
 * from "0000" to "9999".  The best known result approximation is maintained as a string by
 * mResultString (and in a different format by the CR representation of the result).  When we are
 * in danger of not having digits to display in response to further scrolling, we also initiate a
 * background computation to higher precision, as if we had generated placeholder characters.
 *
 * The code is designed to ensure that the error in the displayed result (excluding any
 * placeholder characters) is always strictly less than 1 in the last displayed digit.  Typically
 * we actually display a prefix of a result that has this property and additionally is computed to
 * a significantly higher precision.  Thus we almost always round correctly towards zero.  (Fully
 * correct rounding towards zero is not computable, at least given our representation.)
 *
 * Initial expression evaluation may time out.  This may happen in the case of domain errors such
 * as division by zero, or for large computations.  We do not currently time out reevaluations to
 * higher precision, since the original evaluation precluded a domain error that could result in
 * non-termination.  (We may discover that a presumed zero result is actually slightly negative
 * when re-evaluated; but that results in an exception, which we can handle.)  The user can abort
 * either kind of computation.
 *
 * We ensure that only one evaluation of either kind (AsyncEvaluator or AsyncReevaluator) is
 * running at a time.
 */
class Evaluator {

    // When naming variables and fields, "Offset" denotes a character offset in a string
    // representing a decimal number, where the offset is relative to the decimal point.  1 =
    // tenths position, -1 = units position.  Integer.MAX_VALUE is sometimes used for the offset
    // of the last digit in an a nonterminating decimal expansion.  We use the suffix "Index" to
    // denote a zero-based absolute index into such a string.

    private static final String KEY_PREF_DEGREE_MODE = "degree_mode";

    // The minimum number of extra digits we always try to compute to improve the chance of
    // producing a correctly-rounded-towards-zero result.  The extra digits can be displayed to
    // avoid generating placeholder digits, but should only be displayed briefly while computing.
    private static final int EXTRA_DIGITS = 20;

    // We adjust EXTRA_DIGITS by adding the length of the previous result divided by
    // EXTRA_DIVISOR.  This helps hide recompute latency when long results are requested;
    // We start the recomputation substantially before the need is likely to be visible.
    private static final int EXTRA_DIVISOR = 5;

    // In addition to insisting on extra digits (see above), we minimize reevaluation
    // frequency by precomputing an extra PRECOMPUTE_DIGITS
    // + <current_precision_offset>/PRECOMPUTE_DIVISOR digits, whenever we are forced to
    // reevaluate.  The last term is dropped if prec < 0.
    private static final int PRECOMPUTE_DIGITS = 30;
    private static final int PRECOMPUTE_DIVISOR = 5;

    // Initial evaluation precision.  Enough to guarantee that we can compute the short
    // representation, and that we rarely have to evaluate nonzero results to MAX_MSD_PREC_OFFSET.
    // It also helps if this is at least EXTRA_DIGITS + display width, so that we don't
    // immediately need a second evaluation.
    private static final int INIT_PREC = 50;

    // The largest number of digits to the right of the decimal point to which we will evaluate to
    // compute proper scientific notation for values close to zero.  Chosen to ensure that we
    // always to better than IEEE double precision at identifying nonzeros.
    private static final int MAX_MSD_PREC_OFFSET = 320;

    // If we can replace an exponent by this many leading zeroes, we do so.  Also used in
    // estimating exponent size for truncating short representation.
    private static final int EXP_COST = 3;

    private final Calculator mCalculator;
    private final CalculatorResult mResult;

    // The current caluclator expression.
    private CalculatorExpr mExpr;

    // Last saved expression.  Either null or contains a single CalculatorExpr.PreEval node.
    private CalculatorExpr mSaved;

    //  A hopefully unique name associated with mSaved.
    private String mSavedName;

    // The expression may have changed since the last evaluation in ways that would affect its
    // value.
    private boolean mChangedValue;

    private SharedPreferences mSharedPrefs;

    private boolean mDegreeMode;       // Currently in degree (not radian) mode.

    private final Handler mTimeoutHandler;  // Used to schedule evaluation timeouts.

    // The following are valid only if an evaluation completed successfully.
        private CR mVal;               // Value of mExpr as constructive real.
        private BoundedRational mRatVal; // Value of mExpr as rational or null.

    // We cache the best known decimal result in mResultString.  Whenever that is
    // non-null, it is computed to exactly mResultStringOffset, which is always > 0.
    // The cache is filled in by the UI thread.
    // Valid only if mResultString is non-null and !mChangedValue.
    private String mResultString;
    private int mResultStringOffset = 0;

    // Number of digits to which (possibly incomplete) evaluation has been requested.
    // Only accessed by UI thread.
    private int mResultStringOffsetReq;  // Number of digits that have been

    public static final int INVALID_MSD = Integer.MAX_VALUE;

    // Position of most significant digit in current cached result, if determined.  This is just
    // the index in mResultString holding the msd.
    private int mMsdIndex = INVALID_MSD;

    // Currently running expression evaluator, if any.
    private AsyncEvaluator mEvaluator;

    // The one and only un-cancelled and currently running reevaluator. Touched only by UI thread.
    private AsyncReevaluator mCurrentReevaluator;

    Evaluator(Calculator calculator,
              CalculatorResult resultDisplay) {
        mCalculator = calculator;
        mResult = resultDisplay;
        mExpr = new CalculatorExpr();
        mSaved = new CalculatorExpr();
        mSavedName = "none";
        mTimeoutHandler = new Handler();

        mSharedPrefs = PreferenceManager.getDefaultSharedPreferences(calculator);
        mDegreeMode = mSharedPrefs.getBoolean(KEY_PREF_DEGREE_MODE, false);
    }

    /**
     * Result of initial asynchronous result computation.
     * Represents either an error or a result computed to an initial evaluation precision.
     */
    private static class InitialResult {
        public final int errorResourceId;    // Error string or INVALID_RES_ID.
        public final CR val;                 // Constructive real value.
        public final BoundedRational ratVal; // Rational value or null.
        public final String newResultString;       // Null iff it can't be computed.
        public final int newResultStringOffset;
        public final int initDisplayOffset;
        InitialResult(CR v, BoundedRational rv, String s, int p, int idp) {
            errorResourceId = Calculator.INVALID_RES_ID;
            val = v;
            ratVal = rv;
            newResultString = s;
            newResultStringOffset = p;
            initDisplayOffset = idp;
        }
        InitialResult(int errorId) {
            errorResourceId = errorId;
            val = CR.valueOf(0);
            ratVal = BoundedRational.ZERO;
            newResultString = "BAD";
            newResultStringOffset = 0;
            initDisplayOffset = 0;
        }
        boolean isError() {
            return errorResourceId != Calculator.INVALID_RES_ID;
        }
    }

    private void displayCancelledMessage() {
        new AlertDialog.Builder(mCalculator)
            .setMessage(R.string.cancelled)
            .setPositiveButton(R.string.dismiss,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface d, int which) { }
                })
            .create()
            .show();
    }

    // Timeout handling.
    // Expressions are evaluated with a sort timeout or a long timeout.
    // Each implies different maxima on both computation time and bit length.
    // We recheck bit length separetly to avoid wasting time on decimal conversions that are
    // destined to fail.

    /**
     * Is a long timeout in effect for the main expression?
     */
    private boolean mLongTimeout = false;

    /**
     * Is a long timeout in effect for the saved expression?
     */
    private boolean mLongSavedTimeout = false;

    /**
     * Return the timeout in milliseconds.
     * @param longTimeout a long timeout is in effect
     */
    private long getTimeout(boolean longTimeout) {
        return longTimeout ? 15000 : 2000;
        // Exceeding a few tens of seconds increases the risk of running out of memory
        // and impacting the rest of the system.
    }

    /**
     * Return the maximum number of bits in the result.  Longer results are assumed to time out.
     * @param longTimeout a long timeout is in effect
     */
    private int getMaxResultBits(boolean longTimeout) {
        return longTimeout ? 350000 : 120000;
    }

    /**
     * Timeout for unrequested, speculative evaluations, in milliseconds.
     */
    private final long QUICK_TIMEOUT = 1000;

    /**
     * Maximum result bit length for unrequested, speculative evaluations.
     */
    private final int QUICK_MAX_RESULT_BITS = 50000;

    private void displayTimeoutMessage() {
        AlertDialogFragment.showMessageDialog(mCalculator, mCalculator.getString(R.string.timeout),
                (mLongTimeout ? null : mCalculator.getString(R.string.ok_remove_timeout)));
    }

    public void setLongTimeOut() {
        mLongTimeout = true;
    }

    /**
     * Compute initial cache contents and result when we're good and ready.
     * We leave the expression display up, with scrolling disabled, until this computation
     * completes.  Can result in an error display if something goes wrong.  By default we set a
     * timeout to catch runaway computations.
     */
    class AsyncEvaluator extends AsyncTask<Void, Void, InitialResult> {
        private boolean mDm;  // degrees
        private boolean mRequired; // Result was requested by user.
        private boolean mQuiet;  // Suppress cancellation message.
        private Runnable mTimeoutRunnable = null;
        AsyncEvaluator(boolean dm, boolean required) {
            mDm = dm;
            mRequired = required;
            mQuiet = !required;
        }
        private void handleTimeOut() {
            boolean running = (getStatus() != AsyncTask.Status.FINISHED);
            if (running && cancel(true)) {
                mEvaluator = null;
                // Replace mExpr with clone to avoid races if task
                // still runs for a while.
                mExpr = (CalculatorExpr)mExpr.clone();
                if (mRequired) {
                    suppressCancelMessage();
                    displayTimeoutMessage();
                }
            }
        }
        private void suppressCancelMessage() {
            mQuiet = true;
        }
        @Override
        protected void onPreExecute() {
            long timeout = mRequired ? getTimeout(mLongTimeout) : QUICK_TIMEOUT;
            mTimeoutRunnable = new Runnable() {
                @Override
                public void run() {
                    handleTimeOut();
                }
            };
            mTimeoutHandler.postDelayed(mTimeoutRunnable, timeout);
        }
        /**
         * Is a computed result too big for decimal conversion?
         */
        private boolean isTooBig(CalculatorExpr.EvalResult res) {
            int maxBits = mRequired ? getMaxResultBits(mLongTimeout) : QUICK_MAX_RESULT_BITS;
            if (res.ratVal != null) {
                return res.ratVal.wholeNumberBits() > maxBits;
            } else {
                return res.val.get_appr(maxBits).bitLength() > 2;
            }
        }
        @Override
        protected InitialResult doInBackground(Void... nothing) {
            try {
                CalculatorExpr.EvalResult res = mExpr.eval(mDm);
                if (isTooBig(res)) {
                    // Avoid starting a long uninterruptible decimal conversion.
                    return new InitialResult(R.string.timeout);
                }
                int precOffset = INIT_PREC;
                String initResult = res.val.toString(precOffset);
                int msd = getMsdIndexOf(initResult);
                if (BoundedRational.asBigInteger(res.ratVal) == null
                        && msd == INVALID_MSD) {
                    precOffset = MAX_MSD_PREC_OFFSET;
                    initResult = res.val.toString(precOffset);
                    msd = getMsdIndexOf(initResult);
                }
                final int lsdOffset = getLsdOffset(res.ratVal, initResult,
                        initResult.indexOf('.'));
                final int initDisplayOffset = getPreferredPrec(initResult, msd, lsdOffset);
                final int newPrecOffset = initDisplayOffset + EXTRA_DIGITS;
                if (newPrecOffset > precOffset) {
                    precOffset = newPrecOffset;
                    initResult = res.val.toString(precOffset);
                }
                return new InitialResult(res.val, res.ratVal,
                        initResult, precOffset, initDisplayOffset);
            } catch (CalculatorExpr.SyntaxException e) {
                return new InitialResult(R.string.error_syntax);
            } catch (BoundedRational.ZeroDivisionException e) {
                return new InitialResult(R.string.error_zero_divide);
            } catch(ArithmeticException e) {
                return new InitialResult(R.string.error_nan);
            } catch(CR.PrecisionOverflowException e) {
                // Extremely unlikely unless we're actually dividing by zero or the like.
                return new InitialResult(R.string.error_overflow);
            } catch(CR.AbortedException e) {
                return new InitialResult(R.string.error_aborted);
            }
        }
        @Override
        protected void onPostExecute(InitialResult result) {
            mEvaluator = null;
            mTimeoutHandler.removeCallbacks(mTimeoutRunnable);
            if (result.isError()) {
                if (result.errorResourceId == R.string.timeout) {
                    if (mRequired) {
                        displayTimeoutMessage();
                    }
                    mCalculator.onCancelled();
                } else {
                    mCalculator.onError(result.errorResourceId);
                }
                return;
            }
            mVal = result.val;
            mRatVal = result.ratVal;
            // TODO: If the new result ends in lots of zeroes, and we have a rational result which
            // is greater than (in absolute value) the result string, we should subtract 1 ulp
            // from the result string.  That will prevent a later change from zeroes to nines.  We
            // know that the correct, rounded-toward-zero result has nines.
            mResultString = result.newResultString;
            mResultStringOffset = result.newResultStringOffset;
            final int dotIndex = mResultString.indexOf('.');
            String truncatedWholePart = mResultString.substring(0, dotIndex);
            // Recheck display precision; it may change, since display dimensions may have been
            // unknow the first time.  In that case the initial evaluation precision should have
            // been conservative.
            // TODO: Could optimize by remembering display size and checking for change.
            int initPrecOffset = result.initDisplayOffset;
            final int msdIndex = getMsdIndexOf(mResultString);
            final int leastDigOffset = getLsdOffset(mRatVal, mResultString, dotIndex);
            final int newInitPrecOffset = getPreferredPrec(mResultString, msdIndex, leastDigOffset);
            if (newInitPrecOffset < initPrecOffset) {
                initPrecOffset = newInitPrecOffset;
            } else {
                // They should be equal.  But nothing horrible should happen if they're not. e.g.
                // because CalculatorResult.MAX_WIDTH was too small.
            }
            mCalculator.onEvaluate(initPrecOffset, msdIndex, leastDigOffset, truncatedWholePart);
        }
        @Override
        protected void onCancelled(InitialResult result) {
            // Invoker resets mEvaluator.
            mTimeoutHandler.removeCallbacks(mTimeoutRunnable);
            if (mRequired && !mQuiet) {
                displayCancelledMessage();
            } // Otherwise, if mRequired, timeout processing displayed message.
            mCalculator.onCancelled();
            // Just drop the evaluation; Leave expression displayed.
            return;
        }
    }

    /**
     * Check whether a new higher precision result flips previously computed trailing 9s
     * to zeroes.  If so, flip them back.  Return the adjusted result.
     * Assumes newPrecOffset >= oldPrecOffset > 0.
     * Since our results are accurate to < 1 ulp, this can only happen if the true result
     * is less than the new result with trailing zeroes, and thus appending 9s to the
     * old result must also be correct.  Such flips are impossible if the newly computed
     * digits consist of anything other than zeroes.
     * It is unclear that there are real cases in which this is necessary,
     * but we have failed to prove there aren't such cases.
     */
    @VisibleForTesting
    static String unflipZeroes(String oldDigs, int oldPrecOffset, String newDigs,
            int newPrecOffset) {
        final int oldLen = oldDigs.length();
        if (oldDigs.charAt(oldLen - 1) != '9') {
            return newDigs;
        }
        final int newLen = newDigs.length();
        final int precDiff = newPrecOffset - oldPrecOffset;
        final int oldLastInNew = newLen - 1 - precDiff;
        if (newDigs.charAt(oldLastInNew) != '0') {
            return newDigs;
        }
        // Earlier digits could not have changed without a 0 to 9 or 9 to 0 flip at end.
        // The former is OK.
        if (!newDigs.substring(newLen - precDiff).equals(repeat('0', precDiff))) {
            throw new AssertionError("New approximation invalidates old one!");
        }
        return oldDigs + repeat('9', precDiff);
    }

    /**
     * Result of asynchronous reevaluation.
     */
    private static class ReevalResult {
        public final String newResultString;
        public final int newResultStringOffset;
        ReevalResult(String s, int p) {
            newResultString = s;
            newResultStringOffset = p;
        }
    }

    /**
     * Compute new mResultString contents to prec digits to the right of the decimal point.
     * Ensure that onReevaluate() is called after doing so.  If the evaluation fails for reasons
     * other than a timeout, ensure that onError() is called.
     */
    private class AsyncReevaluator extends AsyncTask<Integer, Void, ReevalResult> {
        @Override
        protected ReevalResult doInBackground(Integer... prec) {
            try {
                final int precOffset = prec[0].intValue();
                return new ReevalResult(mVal.toString(precOffset), precOffset);
            } catch(ArithmeticException e) {
                return null;
            } catch(CR.PrecisionOverflowException e) {
                return null;
            } catch(CR.AbortedException e) {
                // Should only happen if the task was cancelled, in which case we don't look at
                // the result.
                return null;
            }
        }

        @Override
        protected void onPostExecute(ReevalResult result) {
            if (result == null) {
                // This should only be possible in the extremely rare case of encountering a
                // domain error while reevaluating or in case of a precision overflow.  We don't
                // know of a way to get the latter with a plausible amount of user input.
                mCalculator.onError(R.string.error_nan);
            } else {
                if (result.newResultStringOffset < mResultStringOffset) {
                    throw new AssertionError("Unexpected onPostExecute timing");
                }
                mResultString = unflipZeroes(mResultString, mResultStringOffset,
                        result.newResultString, result.newResultStringOffset);
                mResultStringOffset = result.newResultStringOffset;
                mCalculator.onReevaluate();
            }
            mCurrentReevaluator = null;
        }
        // On cancellation we do nothing; invoker should have left no trace of us.
    }

    /**
     * If necessary, start an evaluation to precOffset.
     * Ensure that the display is redrawn when it completes.
     */
    private void ensureCachePrec(int precOffset) {
        if (mResultString != null && mResultStringOffset >= precOffset
                || mResultStringOffsetReq >= precOffset) return;
        if (mCurrentReevaluator != null) {
            // Ensure we only have one evaluation running at a time.
            mCurrentReevaluator.cancel(true);
            mCurrentReevaluator = null;
        }
        mCurrentReevaluator = new AsyncReevaluator();
        mResultStringOffsetReq = precOffset + PRECOMPUTE_DIGITS;
        if (mResultString != null) {
            mResultStringOffsetReq += mResultStringOffsetReq / PRECOMPUTE_DIVISOR;
        }
        mCurrentReevaluator.execute(mResultStringOffsetReq);
    }

    /**
     * Return the rightmost nonzero digit position, if any.
     * @param ratVal Rational value of result or null.
     * @param cache Current cached decimal string representation of result.
     * @param decIndex Index of decimal point in cache.
     * @result Position of rightmost nonzero digit relative to decimal point.
     *         Integer.MIN_VALUE if ratVal is zero.  Integer.MAX_VALUE if there is no lsd,
     *         or we cannot determine it.
     */
    int getLsdOffset(BoundedRational ratVal, String cache, int decIndex) {
        if (ratVal != null && ratVal.signum() == 0) return Integer.MIN_VALUE;
        int result = BoundedRational.digitsRequired(ratVal);
        if (result == 0) {
            int i;
            for (i = -1; decIndex + i > 0 && cache.charAt(decIndex + i) == '0'; --i) { }
            result = i;
        }
        return result;
    }

    // TODO: We may want to consistently specify the position of the current result
    // window using the left-most visible digit index instead of the offset for the rightmost one.
    // It seems likely that would simplify the logic.

    /**
     * Retrieve the preferred precision "offset" for the currently displayed result.
     * May be called from non-UI thread.
     * @param cache Current approximation as string.
     * @param msd Position of most significant digit in result.  Index in cache.
     *            Can be INVALID_MSD if we haven't found it yet.
     * @param lastDigitOffset Position of least significant digit (1 = tenths digit)
     *                  or Integer.MAX_VALUE.
     */
    private int getPreferredPrec(String cache, int msd, int lastDigitOffset) {
        final int lineLength = mResult.getMaxChars();
        final int wholeSize = cache.indexOf('.');
        final int negative = cache.charAt(0) == '-' ? 1 : 0;
        // Don't display decimal point if result is an integer.
        if (lastDigitOffset == 0) {
            lastDigitOffset = -1;
        }
        if (lastDigitOffset != Integer.MAX_VALUE) {
            if (wholeSize <= lineLength && lastDigitOffset <= 0) {
                // Exact integer.  Prefer to display as integer, without decimal point.
                return -1;
            }
            if (lastDigitOffset >= 0
                    && wholeSize + lastDigitOffset + 1 /* decimal pt. */ <= lineLength) {
                // Display full exact number wo scientific notation.
                return lastDigitOffset;
            }
        }
        if (msd > wholeSize && msd <= wholeSize + EXP_COST + 1) {
            // Display number without scientific notation.  Treat leading zero as msd.
            msd = wholeSize - 1;
        }
        if (msd > wholeSize + MAX_MSD_PREC_OFFSET) {
            // Display a probable but uncertain 0 as "0.000000000",
            // without exponent.  That's a judgment call, but less likely
            // to confuse naive users.  A more informative and confusing
            // option would be to use a large negative exponent.
            return lineLength - 2;
        }
        // Return position corresponding to having msd at left, effectively
        // presuming scientific notation that preserves the left part of the
        // result.
        return msd - wholeSize + lineLength - negative - 1;
    }

    private static final int SHORT_TARGET_LENGTH  = 8;
    private static final String SHORT_UNCERTAIN_ZERO = "0.00000" + KeyMaps.ELLIPSIS;

    /**
     * Get a short representation of the value represented by the string cache.
     * We try to match the CalculatorResult code when the result is finite
     * and small enough to suit our needs.
     * The result is not internationalized.
     * @param cache String approximation of value.  Assumed to be long enough
     *              that if it doesn't contain enough significant digits, we can
     *              reasonably abbreviate as SHORT_UNCERTAIN_ZERO.
     * @param msdIndex Index of most significant digit in cache, or INVALID_MSD.
     * @param lsdOffset Position of least significant digit in finite representation,
     *            relative to decimal point, or MAX_VALUE.
     */
    private String getShortString(String cache, int msdIndex, int lsdOffset) {
        // This somewhat mirrors the display formatting code, but
        // - The constants are different, since we don't want to use the whole display.
        // - This is an easier problem, since we don't support scrolling and the length
        //   is a bit flexible.
        // TODO: Think about refactoring this to remove partial redundancy with CalculatorResult.
        final int dotIndex = cache.indexOf('.');
        final int negative = cache.charAt(0) == '-' ? 1 : 0;
        final String negativeSign = negative == 1 ? "-" : "";

        // Ensure we don't have to worry about running off the end of cache.
        if (msdIndex >= cache.length() - SHORT_TARGET_LENGTH) {
            msdIndex = INVALID_MSD;
        }
        if (msdIndex == INVALID_MSD) {
            if (lsdOffset < INIT_PREC) {
                return "0";
            } else {
                return SHORT_UNCERTAIN_ZERO;
            }
        }
        // Avoid scientific notation for small numbers of zeros.
        // Instead stretch significant digits to include decimal point.
        if (lsdOffset < -1 && dotIndex - msdIndex + negative <= SHORT_TARGET_LENGTH
            && lsdOffset >= -CalculatorResult.MAX_TRAILING_ZEROES - 1) {
            // Whole number that fits in allotted space.
            // CalculatorResult would not use scientific notation either.
            lsdOffset = -1;
        }
        if (msdIndex > dotIndex) {
            if (msdIndex <= dotIndex + EXP_COST + 1) {
                // Preferred display format inthis cases is with leading zeroes, even if
                // it doesn't fit entirely.  Replicate that here.
                msdIndex = dotIndex - 1;
            } else if (lsdOffset <= SHORT_TARGET_LENGTH - negative - 2
                    && lsdOffset <= CalculatorResult.MAX_LEADING_ZEROES + 1) {
                // Fraction that fits entirely in allotted space.
                // CalculatorResult would not use scientific notation either.
                msdIndex = dotIndex -1;
            }
        }
        int exponent = dotIndex - msdIndex;
        if (exponent > 0) {
            // Adjust for the fact that the decimal point itself takes space.
            exponent--;
        }
        if (lsdOffset != Integer.MAX_VALUE) {
            final int lsdIndex = dotIndex + lsdOffset;
            final int totalDigits = lsdIndex - msdIndex + negative + 1;
            if (totalDigits <= SHORT_TARGET_LENGTH && dotIndex > msdIndex && lsdOffset >= -1) {
                // Fits, no exponent needed.
                return negativeSign + cache.substring(msdIndex, lsdIndex + 1);
            }
            if (totalDigits <= SHORT_TARGET_LENGTH - 3) {
                return negativeSign + cache.charAt(msdIndex) + "."
                        + cache.substring(msdIndex + 1, lsdIndex + 1) + "E" + exponent;
            }
        }
        // We need to abbreviate.
        if (dotIndex > msdIndex && dotIndex < msdIndex + SHORT_TARGET_LENGTH - negative - 1) {
            return negativeSign + cache.substring(msdIndex,
                    msdIndex + SHORT_TARGET_LENGTH - negative - 1) + KeyMaps.ELLIPSIS;
        }
        // Need abbreviation + exponent
        return negativeSign + cache.charAt(msdIndex) + "."
                + cache.substring(msdIndex + 1, msdIndex + SHORT_TARGET_LENGTH - negative - 4)
                + KeyMaps.ELLIPSIS + "E" + exponent;
    }

    /**
     * Return the most significant digit index in the given numeric string.
     * Return INVALID_MSD if there are not enough digits to prove the numeric value is
     * different from zero.  As usual, we assume an error of strictly less than 1 ulp.
     */
    public static int getMsdIndexOf(String s) {
        final int len = s.length();
        int nonzeroIndex = -1;
        for (int i = 0; i < len; ++i) {
            char c = s.charAt(i);
            if (c != '-' && c != '.' && c != '0') {
                nonzeroIndex = i;
                break;
            }
        }
        if (nonzeroIndex >= 0 && (nonzeroIndex < len - 1 || s.charAt(nonzeroIndex) != '1')) {
            return nonzeroIndex;
        } else {
            return INVALID_MSD;
        }
    }

    /**
     * Return most significant digit index in the currently computed result.
     * Returns an index in the result character array.  Return INVALID_MSD if the current result
     * is too close to zero to determine the result.
     * Result is almost consistent through reevaluations: It may increase by one, once.
     */
    private int getMsdIndex() {
        if (mMsdIndex != INVALID_MSD) {
            // 0.100000... can change to 0.0999999...  We may have to correct once by one digit.
            if (mResultString.charAt(mMsdIndex) == '0') {
                mMsdIndex++;
            }
            return mMsdIndex;
        }
        if (mRatVal != null && mRatVal.signum() == 0) {
            return INVALID_MSD;  // None exists
        }
        int result = INVALID_MSD;
        if (mResultString != null) {
            result = getMsdIndexOf(mResultString);
        }
        return result;
    }

    /**
     * Return a string with n copies of c.
     */
    private static String repeat(char c, int n) {
        StringBuilder result = new StringBuilder();
        for (int i = 0; i < n; ++i) {
            result.append(c);
        }
        return result.toString();
    }

    // Refuse to scroll past the point at which this many digits from the whole number
    // part of the result are still displayed.  Avoids sily displays like 1E1.
    private static final int MIN_DISPLAYED_DIGS = 5;

    /**
     * Return result to precOffset[0] digits to the right of the decimal point.
     * PrecOffset[0] is updated if the original value is out of range.  No exponent or other
     * indication of precision is added.  The result is returned immediately, based on the current
     * cache contents, but it may contain question marks for unknown digits.  It may also use
     * uncertain digits within EXTRA_DIGITS.  If either of those occurred, schedule a reevaluation
     * and redisplay operation.  Uncertain digits never appear to the left of the decimal point.
     * PrecOffset[0] may be negative to only retrieve digits to the left of the decimal point.
     * (precOffset[0] = 0 means we include the decimal point, but nothing to the right.
     * precOffset[0] = -1 means we drop the decimal point and start at the ones position.  Should
     * not be invoked before the onEvaluate() callback is received.  This essentially just returns
     * a substring of the full result; a leading minus sign or leading digits can be dropped.
     * Result uses US conventions; is NOT internationalized.  Use getRational() to determine
     * whether the result is exact, or whether we dropped trailing digits.
     *
     * @param precOffset Zeroth element indicates desired and actual precision
     * @param maxPrecOffset Maximum adjusted precOffset[0]
     * @param maxDigs Maximum length of result
     * @param truncated Zeroth element is set if leading nonzero digits were dropped
     * @param negative Zeroth element is set of the result is negative.
     */
    public String getString(int[] precOffset, int maxPrecOffset, int maxDigs, boolean[] truncated,
            boolean[] negative) {
        int currentPrecOffset = precOffset[0];
        // Make sure we eventually get a complete answer
        if (mResultString == null) {
            ensureCachePrec(currentPrecOffset + EXTRA_DIGITS);
            // Nothing else to do now; seems to happen on rare occasion with weird user input
            // timing; Will repair itself in a jiffy.
            return " ";
        } else {
            ensureCachePrec(currentPrecOffset + EXTRA_DIGITS + mResultString.length()
                    / EXTRA_DIVISOR);
        }
        // Compute an appropriate substring of mResultString.  Pad if necessary.
        final int len = mResultString.length();
        final boolean myNegative = mResultString.charAt(0) == '-';
        negative[0] = myNegative;
        // Don't scroll left past leftmost digits in mResultString unless that still leaves an
        // integer.
            int integralDigits = len - mResultStringOffset;
                            // includes 1 for dec. pt
            if (myNegative) {
                --integralDigits;
            }
            int minPrecOffset = Math.min(MIN_DISPLAYED_DIGS - integralDigits, -1);
            currentPrecOffset = Math.min(Math.max(currentPrecOffset, minPrecOffset),
                    maxPrecOffset);
            precOffset[0] = currentPrecOffset;
        int extraDigs = mResultStringOffset - currentPrecOffset; // trailing digits to drop
        int deficit = 0;  // The number of digits we're short
        if (extraDigs < 0) {
            extraDigs = 0;
            deficit = Math.min(currentPrecOffset - mResultStringOffset, maxDigs);
        }
        int endIndex = len - extraDigs;
        if (endIndex < 1) {
            return " ";
        }
        int startIndex = Math.max(endIndex + deficit - maxDigs, 0);
        truncated[0] = (startIndex > getMsdIndex());
        String result = mResultString.substring(startIndex, endIndex);
        if (deficit > 0) {
            result += repeat(' ', deficit);
            // Blank character is replaced during translation.
            // Since we always compute past the decimal point, this never fills in the spot
            // where the decimal point should go, and we can otherwise treat placeholders
            // as though they were digits.
        }
        return result;
    }

    /**
     * Return rational representation of current result, if any.
     * Return null if the result is irrational, or we couldn't track the rational value,
     * e.g. because the denominator got too big.
     */
    public BoundedRational getRational() {
        return mRatVal;
    }

    private void clearCache() {
        mResultString = null;
        mResultStringOffset = mResultStringOffsetReq = 0;
        mMsdIndex = INVALID_MSD;
    }


    private void clearPreservingTimeout() {
        mExpr.clear();
        clearCache();
    }

    public void clear() {
        clearPreservingTimeout();
        mLongTimeout = false;
    }

    /**
     * Start asynchronous result evaluation of formula.
     * Will result in display on completion.
     * @param required result was explicitly requested by user.
     */
    private void evaluateResult(boolean required) {
        clearCache();
        mEvaluator = new AsyncEvaluator(mDegreeMode, required);
        mEvaluator.execute();
        mChangedValue = false;
    }

    /**
     * Start optional evaluation of result and display when ready.
     * Can quietly time out without a user-visible display.
     */
    public void evaluateAndShowResult() {
        if (!mChangedValue) {
            // Already done or in progress.
            return;
        }
        // In very odd cases, there can be significant latency to evaluate.
        // Don't show obsolete result.
        mResult.clear();
        evaluateResult(false);
    }

    /**
     * Start required evaluation of result and display when ready.
     * Will eventually call back mCalculator to display result or error, or display
     * a timeout message.  Uses longer timeouts than optional evaluation.
     */
    public void requireResult() {
        if (mResultString == null || mChangedValue) {
            // Restart evaluator in requested mode, i.e. with longer timeout.
            cancelAll(true);
            evaluateResult(true);
        } else {
            // Notify immediately, reusing existing result.
            final int dotIndex = mResultString.indexOf('.');
            final String truncatedWholePart = mResultString.substring(0, dotIndex);
            final int leastDigOffset = getLsdOffset(mRatVal, mResultString, dotIndex);
            final int msdIndex = getMsdIndex();
            final int preferredPrecOffset = getPreferredPrec(mResultString, msdIndex,
                    leastDigOffset);
            mCalculator.onEvaluate(preferredPrecOffset, msdIndex, leastDigOffset,
                    truncatedWholePart);
        }
    }

    /**
     * Cancel all current background tasks.
     * @param quiet suppress cancellation message
     * @return      true if we cancelled an initial evaluation
     */
    public boolean cancelAll(boolean quiet) {
        if (mCurrentReevaluator != null) {
            mCurrentReevaluator.cancel(true);
            mResultStringOffsetReq = mResultStringOffset;
            // Backgound computation touches only constructive reals.
            // OK not to wait.
            mCurrentReevaluator = null;
        }
        if (mEvaluator != null) {
            if (quiet) {
                mEvaluator.suppressCancelMessage();
            }
            mEvaluator.cancel(true);
            // There seems to be no good way to wait for cancellation
            // to complete, and the evaluation continues to look at
            // mExpr, which we will again modify.
            // Give ourselves a new copy to work on instead.
            mExpr = (CalculatorExpr)mExpr.clone();
            // Approximation of constructive reals should be thread-safe,
            // so we can let that continue until it notices the cancellation.
            mEvaluator = null;
            mChangedValue = true;    // Didn't do the expected evaluation.
            return true;
        }
        return false;
    }

    /**
     * Restore the evaluator state, including the expression and any saved value.
     */
    public void restoreInstanceState(DataInput in) {
        mChangedValue = true;
        try {
            CalculatorExpr.initExprInput();
            mDegreeMode = in.readBoolean();
            mLongTimeout = in.readBoolean();
            mLongSavedTimeout = in.readBoolean();
            mExpr = new CalculatorExpr(in);
            mSavedName = in.readUTF();
            mSaved = new CalculatorExpr(in);
        } catch (IOException e) {
            Log.v("Calculator", "Exception while restoring:\n" + e);
        }
    }

    /**
     * Save the evaluator state, including the expression and any saved value.
     */
    public void saveInstanceState(DataOutput out) {
        try {
            CalculatorExpr.initExprOutput();
            out.writeBoolean(mDegreeMode);
            out.writeBoolean(mLongTimeout);
            out.writeBoolean(mLongSavedTimeout);
            mExpr.write(out);
            out.writeUTF(mSavedName);
            mSaved.write(out);
        } catch (IOException e) {
            Log.v("Calculator", "Exception while saving state:\n" + e);
        }
    }


    /**
     * Append a button press to the current expression.
     * @param id Button identifier for the character or operator to be added.
     * @return false if we rejected the insertion due to obvious syntax issues, and the expression
     * is unchanged; true otherwise
     */
    public boolean append(int id) {
        if (id == R.id.fun_10pow) {
            add10pow();  // Handled as macro expansion.
            return true;
        } else {
            mChangedValue = mChangedValue || !KeyMaps.isBinary(id);
            return mExpr.add(id);
        }
    }

    public void delete() {
        mChangedValue = true;
        mExpr.delete();
        if (mExpr.isEmpty()) {
            mLongTimeout = false;
        }
    }

    void setDegreeMode(boolean degreeMode) {
        mChangedValue = true;
        mDegreeMode = degreeMode;

        mSharedPrefs.edit()
                .putBoolean(KEY_PREF_DEGREE_MODE, degreeMode)
                .apply();
    }

    boolean getDegreeMode() {
        return mDegreeMode;
    }

    /**
     * @return the {@link CalculatorExpr} representation of the current result.
     */
    private CalculatorExpr getResultExpr() {
        final int dotIndex = mResultString.indexOf('.');
        final int leastDigOffset = getLsdOffset(mRatVal, mResultString, dotIndex);
        return mExpr.abbreviate(mVal, mRatVal, mDegreeMode,
                getShortString(mResultString, getMsdIndexOf(mResultString), leastDigOffset));
    }

    /**
     * Abbreviate the current expression to a pre-evaluated expression node.
     * This should not be called unless the expression was previously evaluated and produced a
     * non-error result.  Pre-evaluated expressions can never represent an expression for which
     * evaluation to a constructive real diverges.  Subsequent re-evaluation will also not
     * diverge, though it may generate errors of various kinds.  E.g.  sqrt(-10^-1000) .
     */
    public void collapse() {
        final CalculatorExpr abbrvExpr = getResultExpr();
        clearPreservingTimeout();
        mExpr.append(abbrvExpr);
        mChangedValue = true;
    }

    /**
     * Abbreviate current expression, and put result in mSaved.
     * mExpr is left alone.  Return false if result is unavailable.
     */
    public boolean collapseToSaved() {
        if (mResultString == null) {
            return false;
        }
        final CalculatorExpr abbrvExpr = getResultExpr();
        mSaved.clear();
        mSaved.append(abbrvExpr);
        mLongSavedTimeout = mLongTimeout;
        return true;
    }

    private Uri uriForSaved() {
        return new Uri.Builder().scheme("tag")
                                .encodedOpaquePart(mSavedName)
                                .build();
    }

    /**
     * Collapse the current expression to mSaved and return a URI describing it.
     * describing this particular result, so that we can refer to it
     * later.
     */
    public Uri capture() {
        if (!collapseToSaved()) return null;
        // Generate a new (entirely private) URI for this result.
        // Attempt to conform to RFC4151, though it's unclear it matters.
        final TimeZone tz = TimeZone.getDefault();
        DateFormat df = new SimpleDateFormat("yyyy-MM-dd");
        df.setTimeZone(tz);
        final String isoDate = df.format(new Date());
        mSavedName = "calculator2.android.com," + isoDate + ":"
                + (new Random().nextInt() & 0x3fffffff);
        return uriForSaved();
    }

    public boolean isLastSaved(Uri uri) {
        return uri.equals(uriForSaved());
    }

    public void appendSaved() {
        mChangedValue = true;
        mLongTimeout |= mLongSavedTimeout;
        mExpr.append(mSaved);
    }

    /**
     * Add the power of 10 operator to the expression.
     * This is treated essentially as a macro expansion.
     */
    private void add10pow() {
        CalculatorExpr ten = new CalculatorExpr();
        ten.add(R.id.digit_1);
        ten.add(R.id.digit_0);
        mChangedValue = true;  // For consistency.  Reevaluation is probably not useful.
        mExpr.append(ten);
        mExpr.add(R.id.op_pow);
    }

    /**
     * Retrieve the main expression being edited.
     * It is the callee's reponsibility to call cancelAll to cancel ongoing concurrent
     * computations before modifying the result.  The resulting expression should only
     * be modified by the caller if either the expression value doesn't change, or in
     * combination with another add() or delete() call that makes the value change apparent
     * to us.
     * TODO: Perhaps add functionality so we can keep this private?
     */
    public CalculatorExpr getExpr() {
        return mExpr;
    }

    /**
     * Maximum number of characters in a scientific notation exponent.
     */
    private static final int MAX_EXP_CHARS = 8;

    /**
     * Return the index of the character after the exponent starting at s[offset].
     * Return offset if there is no exponent at that position.
     * Exponents have syntax E[-]digit* .  "E2" and "E-2" are valid.  "E+2" and "e2" are not.
     * We allow any Unicode digits, and either of the commonly used minus characters.
     */
    public static int exponentEnd(String s, int offset) {
        int i = offset;
        int len = s.length();
        if (i >= len - 1 || s.charAt(i) != 'E') {
            return offset;
        }
        ++i;
        if (KeyMaps.keyForChar(s.charAt(i)) == R.id.op_sub) {
            ++i;
        }
        if (i == len || !Character.isDigit(s.charAt(i))) {
            return offset;
        }
        ++i;
        while (i < len && Character.isDigit(s.charAt(i))) {
            ++i;
            if (i > offset + MAX_EXP_CHARS) {
                return offset;
            }
        }
        return i;
    }

    /**
     * Add the exponent represented by s[begin..end) to the constant at the end of current
     * expression.
     * The end of the current expression must be a constant.  Exponents have the same syntax as
     * for exponentEnd().
     */
    public void addExponent(String s, int begin, int end) {
        int sign = 1;
        int exp = 0;
        int i = begin + 1;
        // We do the decimal conversion ourselves to exactly match exponentEnd() conventions
        // and handle various kinds of digits on input.  Also avoids allocation.
        if (KeyMaps.keyForChar(s.charAt(i)) == R.id.op_sub) {
            sign = -1;
            ++i;
        }
        for (; i < end; ++i) {
            exp = 10 * exp + Character.digit(s.charAt(i), 10);
        }
        mExpr.addExponent(sign * exp);
        mChangedValue = true;
    }
}
