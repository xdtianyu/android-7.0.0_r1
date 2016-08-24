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

import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;
import android.graphics.Rect;
import android.text.Layout;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.style.BackgroundColorSpan;
import android.text.style.ForegroundColorSpan;
import android.util.AttributeSet;
import android.view.ActionMode;
import android.view.GestureDetector;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.widget.OverScroller;
import android.widget.Toast;

// A text widget that is "infinitely" scrollable to the right,
// and obtains the text to display via a callback to Logic.
public class CalculatorResult extends AlignedTextView {
    static final int MAX_RIGHT_SCROLL = 10000000;
    static final int INVALID = MAX_RIGHT_SCROLL + 10000;
        // A larger value is unlikely to avoid running out of space
    final OverScroller mScroller;
    final GestureDetector mGestureDetector;
    class MyTouchListener implements View.OnTouchListener {
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            return mGestureDetector.onTouchEvent(event);
        }
    }
    final MyTouchListener mTouchListener = new MyTouchListener();
    private Evaluator mEvaluator;
    private boolean mScrollable = false;
                            // A scrollable result is currently displayed.
    private boolean mValid = false;
                            // The result holds something valid; either a a number or an error
                            // message.
    // A suffix of "Pos" denotes a pixel offset.  Zero represents a scroll position
    // in which the decimal point is just barely visible on the right of the display.
    private int mCurrentPos;// Position of right of display relative to decimal point, in pixels.
                            // Large positive values mean the decimal point is scrolled off the
                            // left of the display.  Zero means decimal point is barely displayed
                            // on the right.
    private int mLastPos;   // Position already reflected in display. Pixels.
    private int mMinPos;    // Minimum position before all digits disappear off the right. Pixels.
    private int mMaxPos;    // Maximum position before we start displaying the infinite
                            // sequence of trailing zeroes on the right. Pixels.
    // In the following, we use a suffix of Offset to denote a character position in a numeric
    // string relative to the decimal point.  Positive is to the right and negative is to
    // the left. 1 = tenths position, -1 = units.  Integer.MAX_VALUE is sometimes used
    // for the offset of the last digit in an a nonterminating decimal expansion.
    // We use the suffix "Index" to denote a zero-based index into a string representing a
    // result.
    // TODO: Apply the same convention to other classes.
    private int mMaxCharOffset;  // Character offset from decimal point of rightmost digit
                                 // that should be displayed.  Essentially the same as
    private int mLsdOffset;      // Position of least-significant digit in result
    private int mLastDisplayedOffset; // Offset of last digit actually displayed after adding
                                      // exponent.
    private final Object mWidthLock = new Object();
                            // Protects the next two fields.
    private int mWidthConstraint = -1;
                            // Our total width in pixels minus space for ellipsis.
    private float mCharWidth = 1;
                            // Maximum character width. For now we pretend that all characters
                            // have this width.
                            // TODO: We're not really using a fixed width font.  But it appears
                            // to be close enough for the characters we use that the difference
                            // is not noticeable.
    private static final int MAX_WIDTH = 100;
                            // Maximum number of digits displayed
    public static final int MAX_LEADING_ZEROES = 6;
                            // Maximum number of leading zeroes after decimal point before we
                            // switch to scientific notation with negative exponent.
    public static final int MAX_TRAILING_ZEROES = 6;
                            // Maximum number of trailing zeroes before the decimal point before
                            // we switch to scientific notation with positive exponent.
    private static final int SCI_NOTATION_EXTRA = 1;
                            // Extra digits for standard scientific notation.  In this case we
                            // have a decimal point and no ellipsis.
                            // We assume that we do not drop digits to make room for the decimal
                            // point in ordinary scientific notation. Thus >= 1.
    private ActionMode mActionMode;
    private final ForegroundColorSpan mExponentColorSpan;

    public CalculatorResult(Context context, AttributeSet attrs) {
        super(context, attrs);
        mScroller = new OverScroller(context);
        mGestureDetector = new GestureDetector(context,
            new GestureDetector.SimpleOnGestureListener() {
                @Override
                public boolean onDown(MotionEvent e) {
                    return true;
                }
                @Override
                public boolean onFling(MotionEvent e1, MotionEvent e2,
                                       float velocityX, float velocityY) {
                    if (!mScroller.isFinished()) {
                        mCurrentPos = mScroller.getFinalX();
                    }
                    mScroller.forceFinished(true);
                    stopActionMode();
                    CalculatorResult.this.cancelLongPress();
                    // Ignore scrolls of error string, etc.
                    if (!mScrollable) return true;
                    mScroller.fling(mCurrentPos, 0, - (int) velocityX, 0  /* horizontal only */,
                                    mMinPos, mMaxPos, 0, 0);
                    postInvalidateOnAnimation();
                    return true;
                }
                @Override
                public boolean onScroll(MotionEvent e1, MotionEvent e2,
                                        float distanceX, float distanceY) {
                    int distance = (int)distanceX;
                    if (!mScroller.isFinished()) {
                        mCurrentPos = mScroller.getFinalX();
                    }
                    mScroller.forceFinished(true);
                    stopActionMode();
                    CalculatorResult.this.cancelLongPress();
                    if (!mScrollable) return true;
                    if (mCurrentPos + distance < mMinPos) {
                        distance = mMinPos - mCurrentPos;
                    } else if (mCurrentPos + distance > mMaxPos) {
                        distance = mMaxPos - mCurrentPos;
                    }
                    int duration = (int)(e2.getEventTime() - e1.getEventTime());
                    if (duration < 1 || duration > 100) duration = 10;
                    mScroller.startScroll(mCurrentPos, 0, distance, 0, (int)duration);
                    postInvalidateOnAnimation();
                    return true;
                }
                @Override
                public void onLongPress(MotionEvent e) {
                    if (mValid) {
                        mActionMode = startActionMode(mCopyActionModeCallback,
                                ActionMode.TYPE_FLOATING);
                    }
                }
            });
        setOnTouchListener(mTouchListener);
        setHorizontallyScrolling(false);  // do it ourselves
        setCursorVisible(false);
        mExponentColorSpan = new ForegroundColorSpan(
                context.getColor(R.color.display_result_exponent_text_color));

        // Copy ActionMode is triggered explicitly, not through
        // setCustomSelectionActionModeCallback.
    }

    void setEvaluator(Evaluator evaluator) {
        mEvaluator = evaluator;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        final TextPaint paint = getPaint();
        final Context context = getContext();
        final float newCharWidth = Layout.getDesiredWidth("\u2007", paint);
        // Digits are presumed to have no more than newCharWidth.
        // We sometimes replace a character by an ellipsis or, due to SCI_NOTATION_EXTRA, add
        // an extra decimal separator beyond the maximum number of characters we normally allow.
        // Empirically, our minus sign is also slightly wider than a digit, so we have to
        // account for that.  We never have both an ellipsis and two minus signs, and
        // we assume an ellipsis is no narrower than a minus sign.
        final float decimalSeparatorWidth = Layout.getDesiredWidth(
                context.getString(R.string.dec_point), paint);
        final float minusExtraWidth = Layout.getDesiredWidth(
                context.getString(R.string.op_sub), paint) - newCharWidth;
        final float ellipsisExtraWidth = Layout.getDesiredWidth(KeyMaps.ELLIPSIS, paint)
                - newCharWidth;
        final int extraWidth = (int) (Math.ceil(Math.max(decimalSeparatorWidth + minusExtraWidth,
                ellipsisExtraWidth)) + Math.max(minusExtraWidth, 0.0f));
        final int newWidthConstraint = MeasureSpec.getSize(widthMeasureSpec)
                - (getPaddingLeft() + getPaddingRight()) - extraWidth;
        synchronized(mWidthLock) {
            mWidthConstraint = newWidthConstraint;
            mCharWidth = newCharWidth;
        }
    }

    // Return the length of the exponent representation for the given exponent, in
    // characters.
    private final int expLen(int exp) {
        if (exp == 0) return 0;
        final int abs_exp_digits = (int) Math.ceil(Math.log10(Math.abs((double)exp))
                + 0.0000000001d /* Round whole numbers to next integer */);
        return abs_exp_digits + (exp >= 0 ? 1 : 2);
    }

    /**
     * Initiate display of a new result.
     * The parameters specify various properties of the result.
     * @param initPrec Initial display precision computed by evaluator. (1 = tenths digit)
     * @param msd Position of most significant digit.  Offset from left of string.
                  Evaluator.INVALID_MSD if unknown.
     * @param leastDigPos Position of least significant digit (1 = tenths digit)
     *                    or Integer.MAX_VALUE.
     * @param truncatedWholePart Result up to but not including decimal point.
                                 Currently we only use the length.
     */
    void displayResult(int initPrec, int msd, int leastDigPos, String truncatedWholePart) {
        initPositions(initPrec, msd, leastDigPos, truncatedWholePart);
        redisplay();
    }

    /**
     * Set up scroll bounds (mMinPos, mMaxPos, etc.) and determine whether the result is
     * scrollable, based on the supplied information about the result.
     * This is unfortunately complicated because we need to predict whether trailing digits
     * will eventually be replaced by an exponent.
     * Just appending the exponent during formatting would be simpler, but would produce
     * jumpier results during transitions.
     */
    private void initPositions(int initPrecOffset, int msdIndex, int lsdOffset,
            String truncatedWholePart) {
        float charWidth;
        int maxChars = getMaxChars();
        mLastPos = INVALID;
        mLsdOffset = lsdOffset;
        synchronized(mWidthLock) {
            charWidth = mCharWidth;
        }
        mCurrentPos = mMinPos = (int) Math.round(initPrecOffset * charWidth);
        // Prevent scrolling past initial position, which is calculated to show leading digits.
        if (msdIndex == Evaluator.INVALID_MSD) {
            // Possible zero value
            if (lsdOffset == Integer.MIN_VALUE) {
                // Definite zero value.
                mMaxPos = mMinPos;
                mMaxCharOffset = (int) Math.round(mMaxPos/charWidth);
                mScrollable = false;
            } else {
                // May be very small nonzero value.  Allow user to find out.
                mMaxPos = mMaxCharOffset = MAX_RIGHT_SCROLL;
                mMinPos -= charWidth;  // Allow for future minus sign.
                mScrollable = true;
            }
            return;
        }
        int wholeLen =  truncatedWholePart.length();
        int negative = truncatedWholePart.charAt(0) == '-' ? 1 : 0;
        if (msdIndex > wholeLen && msdIndex <= wholeLen + 3) {
            // Avoid tiny negative exponent; pretend msdIndex is just to the right of decimal point.
            msdIndex = wholeLen - 1;
        }
        int minCharOffset = msdIndex - wholeLen;
                                // Position of leftmost significant digit relative to dec. point.
                                // Usually negative.
        mMaxCharOffset = MAX_RIGHT_SCROLL; // How far does it make sense to scroll right?
        // If msd is left of decimal point should logically be
        // mMinPos = - (int) Math.ceil(getPaint().measureText(truncatedWholePart)), but
        // we eventually translate to a character position by dividing by mCharWidth.
        // To avoid rounding issues, we use the analogous computation here.
        if (minCharOffset > -1 && minCharOffset < MAX_LEADING_ZEROES + 2) {
            // Small number of leading zeroes, avoid scientific notation.
            minCharOffset = -1;
        }
        if (lsdOffset < MAX_RIGHT_SCROLL) {
            mMaxCharOffset = lsdOffset;
            if (mMaxCharOffset < -1 && mMaxCharOffset > -(MAX_TRAILING_ZEROES + 2)) {
                mMaxCharOffset = -1;
            }
            // lsdOffset is positive or negative, never 0.
            int currentExpLen = 0;  // Length of required standard scientific notation exponent.
            if (mMaxCharOffset < -1) {
                currentExpLen = expLen(-minCharOffset - 1);
            } else if (minCharOffset > -1 || mMaxCharOffset >= maxChars) {
                // Number either entirely to the right of decimal point, or decimal point not
                // visible when scrolled to the right.
                currentExpLen = expLen(-minCharOffset);
            }
            mScrollable = (mMaxCharOffset + currentExpLen - minCharOffset + negative >= maxChars);
            int newMaxCharOffset;
            if (currentExpLen > 0) {
                if (mScrollable) {
                    // We'll use exponent corresponding to leastDigPos when scrolled to right.
                    newMaxCharOffset = mMaxCharOffset + expLen(-lsdOffset);
                } else {
                    newMaxCharOffset = mMaxCharOffset + currentExpLen;
                }
                if (mMaxCharOffset <= -1 && newMaxCharOffset > -1) {
                    // Very unlikely; just drop exponent.
                    mMaxCharOffset = -1;
                } else {
                    mMaxCharOffset = newMaxCharOffset;
                }
            }
            mMaxPos = Math.min((int) Math.round(mMaxCharOffset * charWidth), MAX_RIGHT_SCROLL);
            if (!mScrollable) {
                // Position the number consistently with our assumptions to make sure it
                // actually fits.
                mCurrentPos = mMaxPos;
            }
        } else {
            mMaxPos = mMaxCharOffset = MAX_RIGHT_SCROLL;
            mScrollable = true;
        }
    }

    void displayError(int resourceId) {
        mValid = true;
        mScrollable = false;
        setText(resourceId);
    }

    private final int MAX_COPY_SIZE = 1000000;

    /*
     * Return the most significant digit position in the given string or Evaluator.INVALID_MSD.
     * Unlike Evaluator.getMsdIndexOf, we treat a final 1 as significant.
     */
    public static int getNaiveMsdIndexOf(String s) {
        int len = s.length();
        for (int i = 0; i < len; ++i) {
            char c = s.charAt(i);
            if (c != '-' && c != '.' && c != '0') {
                return i;
            }
        }
        return Evaluator.INVALID_MSD;
    }

    // Format a result returned by Evaluator.getString() into a single line containing ellipses
    // (if appropriate) and an exponent (if appropriate).  precOffset is the value that was passed
    // to getString and thus identifies the significance of the rightmost digit.
    // A value of 1 means the rightmost digits corresponds to tenths.
    // maxDigs is the maximum number of characters in the result.
    // We set lastDisplayedOffset[0] to the offset of the last digit actually appearing in
    // the display.
    // If forcePrecision is true, we make sure that the last displayed digit corresponds to
    // precOffset, and allow maxDigs to be exceeded in assing the exponent.
    // We add two distinct kinds of exponents:
    // (1) If the final result contains the leading digit we use standard scientific notation.
    // (2) If not, we add an exponent corresponding to an interpretation of the final result as
    //     an integer.
    // We add an ellipsis on the left if the result was truncated.
    // We add ellipses and exponents in a way that leaves most digits in the position they
    // would have been in had we not done so.
    // This minimizes jumps as a result of scrolling.  Result is NOT internationalized,
    // uses "E" for exponent.
    public String formatResult(String in, int precOffset, int maxDigs, boolean truncated,
            boolean negative, int lastDisplayedOffset[], boolean forcePrecision) {
        final int minusSpace = negative ? 1 : 0;
        final int msdIndex = truncated ? -1 : getNaiveMsdIndexOf(in);  // INVALID_MSD is OK.
        String result = in;
        if (truncated || (negative && result.charAt(0) != '-')) {
            result = KeyMaps.ELLIPSIS + result.substring(1, result.length());
            // Ellipsis may be removed again in the type(1) scientific notation case.
        }
        final int decIndex = result.indexOf('.');
        lastDisplayedOffset[0] = precOffset;
        if ((decIndex == -1 || msdIndex != Evaluator.INVALID_MSD
                && msdIndex - decIndex > MAX_LEADING_ZEROES + 1) &&  precOffset != -1) {
            // No decimal point displayed, and it's not just to the right of the last digit,
            // or we should suppress leading zeroes.
            // Add an exponent to let the user track which digits are currently displayed.
            // Start with type (2) exponent if we dropped no digits. -1 accounts for decimal point.
            final int initExponent = precOffset > 0 ? -precOffset : -precOffset - 1;
            int exponent = initExponent;
            boolean hasPoint = false;
            if (!truncated && msdIndex < maxDigs - 1
                    && result.length() - msdIndex + 1 + minusSpace
                    <= maxDigs + SCI_NOTATION_EXTRA) {
                // Type (1) exponent computation and transformation:
                // Leading digit is in display window. Use standard calculator scientific notation
                // with one digit to the left of the decimal point. Insert decimal point and
                // delete leading zeroes.
                // We try to keep leading digits roughly in position, and never
                // lengthen the result by more than SCI_NOTATION_EXTRA.
                final int resLen = result.length();
                String fraction = result.substring(msdIndex + 1, resLen);
                result = (negative ? "-" : "") + result.substring(msdIndex, msdIndex + 1)
                        + "." + fraction;
                // Original exp was correct for decimal point at right of fraction.
                // Adjust by length of fraction.
                exponent = initExponent + resLen - msdIndex - 1;
                hasPoint = true;
            }
            // Exponent can't be zero.
            // Actually add the exponent of either type:
            if (!forcePrecision) {
                int dropDigits;  // Digits to drop to make room for exponent.
                if (hasPoint) {
                    // Type (1) exponent.
                    // Drop digits even if there is room. Otherwise the scrolling gets jumpy.
                    dropDigits = expLen(exponent);
                    if (dropDigits >= result.length() - 1) {
                        // Jumpy is better than no mantissa.  Probably impossible anyway.
                        dropDigits = Math.max(result.length() - 2, 0);
                    }
                } else {
                    // Type (2) exponent.
                    // Exponent depends on the number of digits we drop, which depends on
                    // exponent ...
                    for (dropDigits = 2; expLen(initExponent + dropDigits) > dropDigits;
                            ++dropDigits) {}
                    exponent = initExponent + dropDigits;
                    if (precOffset - dropDigits > mLsdOffset) {
                        // This can happen if e.g. result = 10^40 + 10^10
                        // It turns out we would otherwise display ...10e9 because it takes
                        // the same amount of space as ...1e10 but shows one more digit.
                        // But we don't want to display a trailing zero, even if it's free.
                        ++dropDigits;
                        ++exponent;
                    }
                }
                result = result.substring(0, result.length() - dropDigits);
                lastDisplayedOffset[0] -= dropDigits;
            }
            result = result + "E" + Integer.toString(exponent);
        }
        return result;
    }

    /**
     * Get formatted, but not internationalized, result from mEvaluator.
     * @param precOffset requested position (1 = tenths) of last included digit.
     * @param maxSize Maximum number of characters (more or less) in result.
     * @param lastDisplayedOffset Zeroth entry is set to actual offset of last included digit,
     *                            after adjusting for exponent, etc.
     * @param forcePrecision Ensure that last included digit is at pos, at the expense
     *                       of treating maxSize as a soft limit.
     */
    private String getFormattedResult(int precOffset, int maxSize, int lastDisplayedOffset[],
            boolean forcePrecision) {
        final boolean truncated[] = new boolean[1];
        final boolean negative[] = new boolean[1];
        final int requestedPrecOffset[] = {precOffset};
        final String rawResult = mEvaluator.getString(requestedPrecOffset, mMaxCharOffset,
                maxSize, truncated, negative);
        return formatResult(rawResult, requestedPrecOffset[0], maxSize, truncated[0], negative[0],
                lastDisplayedOffset, forcePrecision);
   }

    // Return entire result (within reason) up to current displayed precision.
    public String getFullText() {
        if (!mValid) return "";
        if (!mScrollable) return getText().toString();
        int currentCharOffset = getCurrentCharOffset();
        int unused[] = new int[1];
        return KeyMaps.translateResult(getFormattedResult(mLastDisplayedOffset, MAX_COPY_SIZE,
                unused, true));
    }

    public boolean fullTextIsExact() {
        return !mScrollable
                || mMaxCharOffset == getCurrentCharOffset() && mMaxCharOffset != MAX_RIGHT_SCROLL;
    }

    /**
     * Return the maximum number of characters that will fit in the result display.
     * May be called asynchronously from non-UI thread.
     */
    int getMaxChars() {
        int result;
        synchronized(mWidthLock) {
            result = (int) Math.floor(mWidthConstraint / mCharWidth);
            // We can apparently finish evaluating before onMeasure in CalculatorText has been
            // called, in which case we get 0 or -1 as the width constraint.
        }
        if (result <= 0) {
            // Return something conservatively big, to force sufficient evaluation.
            return MAX_WIDTH;
        } else {
            return result;
        }
    }

    /**
     * @return {@code true} if the currently displayed result is scrollable
     */
    public boolean isScrollable() {
        return mScrollable;
    }

    int getCurrentCharOffset() {
        synchronized(mWidthLock) {
            return (int) Math.round(mCurrentPos / mCharWidth);
        }
    }

    void clear() {
        mValid = false;
        mScrollable = false;
        setText("");
    }

    void redisplay() {
        int currentCharOffset = getCurrentCharOffset();
        int maxChars = getMaxChars();
        int lastDisplayedOffset[] = new int[1];
        String result = getFormattedResult(currentCharOffset, maxChars, lastDisplayedOffset, false);
        int expIndex = result.indexOf('E');
        result = KeyMaps.translateResult(result);
        if (expIndex > 0 && result.indexOf('.') == -1) {
          // Gray out exponent if used as position indicator
            SpannableString formattedResult = new SpannableString(result);
            formattedResult.setSpan(mExponentColorSpan, expIndex, result.length(),
                                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            setText(formattedResult);
        } else {
            setText(result);
        }
        mLastDisplayedOffset = lastDisplayedOffset[0];
        mValid = true;
    }

    @Override
    public void computeScroll() {
        if (!mScrollable) return;
        if (mScroller.computeScrollOffset()) {
            mCurrentPos = mScroller.getCurrX();
            if (mCurrentPos != mLastPos) {
                mLastPos = mCurrentPos;
                redisplay();
            }
            if (!mScroller.isFinished()) {
                postInvalidateOnAnimation();
            }
        }
    }

    // Copy support:

    private ActionMode.Callback2 mCopyActionModeCallback = new ActionMode.Callback2() {

        private BackgroundColorSpan mHighlightSpan;

        private void highlightResult() {
            final Spannable text = (Spannable) getText();
            mHighlightSpan = new BackgroundColorSpan(getHighlightColor());
            text.setSpan(mHighlightSpan, 0, text.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }

        private void unhighlightResult() {
            final Spannable text = (Spannable) getText();
            text.removeSpan(mHighlightSpan);
        }

        @Override
        public boolean onCreateActionMode(ActionMode mode, Menu menu) {
            MenuInflater inflater = mode.getMenuInflater();
            inflater.inflate(R.menu.copy, menu);
            highlightResult();
            return true;
        }

        @Override
        public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
            return false; // Return false if nothing is done
        }

        @Override
        public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
            switch (item.getItemId()) {
            case R.id.menu_copy:
                copyContent();
                mode.finish();
                return true;
            default:
                return false;
            }
        }

        @Override
        public void onDestroyActionMode(ActionMode mode) {
            unhighlightResult();
            mActionMode = null;
        }

        @Override
        public void onGetContentRect(ActionMode mode, View view, Rect outRect) {
            super.onGetContentRect(mode, view, outRect);
            outRect.left += getPaddingLeft();
            outRect.top += getPaddingTop();
            outRect.right -= getPaddingRight();
            outRect.bottom -= getPaddingBottom();
            final int width = (int) Layout.getDesiredWidth(getText(), getPaint());
            if (width < outRect.width()) {
                outRect.left = outRect.right - width;
            }
        }
    };

    public boolean stopActionMode() {
        if (mActionMode != null) {
            mActionMode.finish();
            return true;
        }
        return false;
    }

    private void setPrimaryClip(ClipData clip) {
        ClipboardManager clipboard = (ClipboardManager) getContext().
                                               getSystemService(Context.CLIPBOARD_SERVICE);
        clipboard.setPrimaryClip(clip);
    }

    private void copyContent() {
        final CharSequence text = getFullText();
        ClipboardManager clipboard =
                (ClipboardManager) getContext().getSystemService(Context.CLIPBOARD_SERVICE);
        // We include a tag URI, to allow us to recognize our own results and handle them
        // specially.
        ClipData.Item newItem = new ClipData.Item(text, null, mEvaluator.capture());
        String[] mimeTypes = new String[] {ClipDescription.MIMETYPE_TEXT_PLAIN};
        ClipData cd = new ClipData("calculator result", mimeTypes, newItem);
        clipboard.setPrimaryClip(cd);
        Toast.makeText(getContext(), R.string.text_copied_toast, Toast.LENGTH_SHORT).show();
    }

}
