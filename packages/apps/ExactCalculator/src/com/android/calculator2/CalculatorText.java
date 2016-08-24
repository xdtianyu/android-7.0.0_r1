/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.calculator2;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Rect;
import android.text.Layout;
import android.text.TextPaint;
import android.text.method.ScrollingMovementMethod;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.ActionMode;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;

/**
 * TextView adapted for Calculator display.
 */
public class CalculatorText extends AlignedTextView implements View.OnLongClickListener {

    private final ActionMode.Callback2 mPasteActionModeCallback = new ActionMode.Callback2() {

        @Override
        public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
            if (item.getItemId() == R.id.menu_paste) {
                paste();
                mode.finish();
                return true;
            }
            return false;
        }

        @Override
        public boolean onCreateActionMode(ActionMode mode, Menu menu) {
            final ClipboardManager clipboard = (ClipboardManager) getContext()
                    .getSystemService(Context.CLIPBOARD_SERVICE);
            if (clipboard.hasPrimaryClip()) {
                bringPointIntoView(length());
                MenuInflater inflater = mode.getMenuInflater();
                inflater.inflate(R.menu.paste, menu);
                return true;
            }
            // Prevents the selection action mode on double tap.
            return false;
        }

        @Override
        public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
            return false;
        }

        @Override
        public void onDestroyActionMode(ActionMode mode) {
            mActionMode = null;
        }

        @Override
        public void onGetContentRect(ActionMode mode, View view, Rect outRect) {
            super.onGetContentRect(mode, view, outRect);
            outRect.top += getTotalPaddingTop();
            outRect.right -= getTotalPaddingRight();
            outRect.bottom -= getTotalPaddingBottom();
            // Encourage menu positioning towards the right, possibly over formula.
            outRect.left = outRect.right;
        }
    };

    // Temporary paint for use in layout methods.
    private final TextPaint mTempPaint = new TextPaint();

    private final float mMaximumTextSize;
    private final float mMinimumTextSize;
    private final float mStepTextSize;

    private int mWidthConstraint = -1;

    private ActionMode mActionMode;

    private OnPasteListener mOnPasteListener;
    private OnTextSizeChangeListener mOnTextSizeChangeListener;

    public CalculatorText(Context context) {
        this(context, null /* attrs */);
    }

    public CalculatorText(Context context, AttributeSet attrs) {
        this(context, attrs, 0 /* defStyleAttr */);
    }

    public CalculatorText(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        final TypedArray a = context.obtainStyledAttributes(
                attrs, R.styleable.CalculatorText, defStyleAttr, 0);
        mMaximumTextSize = a.getDimension(
                R.styleable.CalculatorText_maxTextSize, getTextSize());
        mMinimumTextSize = a.getDimension(
                R.styleable.CalculatorText_minTextSize, getTextSize());
        mStepTextSize = a.getDimension(R.styleable.CalculatorText_stepTextSize,
                (mMaximumTextSize - mMinimumTextSize) / 3);
        a.recycle();

        // Allow scrolling by default.
        setMovementMethod(ScrollingMovementMethod.getInstance());

        // Reset the clickable flag, which is added when specifying a movement method.
        setClickable(false);

        // Add a long click to start the ActionMode manually.
        setOnLongClickListener(this);
    }

    @Override
    public boolean onLongClick(View v) {
        mActionMode = startActionMode(mPasteActionModeCallback, ActionMode.TYPE_FLOATING);
        return true;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        // Re-calculate our textSize based on new width.
        final int width = MeasureSpec.getSize(widthMeasureSpec)
                - getPaddingLeft() - getPaddingRight();
        if (mWidthConstraint != width) {
            mWidthConstraint = width;

            if (!isLaidOut()) {
                // Prevent shrinking/resizing with our variable textSize.
                setTextSizeInternal(TypedValue.COMPLEX_UNIT_PX, mMaximumTextSize,
                        false /* notifyListener */);
                setMinHeight(getLineHeight() + getCompoundPaddingBottom()
                        + getCompoundPaddingTop());
            }

            setTextSizeInternal(TypedValue.COMPLEX_UNIT_PX, getVariableTextSize(getText()),
                    false);
        }

        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    public int getWidthConstraint() { return mWidthConstraint; }

    @Override
    protected void onTextChanged(CharSequence text, int start, int lengthBefore, int lengthAfter) {
        super.onTextChanged(text, start, lengthBefore, lengthAfter);

        setTextSize(TypedValue.COMPLEX_UNIT_PX, getVariableTextSize(text.toString()));
    }

    private void setTextSizeInternal(int unit, float size, boolean notifyListener) {
        final float oldTextSize = getTextSize();
        super.setTextSize(unit, size);
        if (notifyListener && mOnTextSizeChangeListener != null && getTextSize() != oldTextSize) {
            mOnTextSizeChangeListener.onTextSizeChanged(this, oldTextSize);
        }
    }

    @Override
    public void setTextSize(int unit, float size) {
        setTextSizeInternal(unit, size, true);
    }

    public float getMinimumTextSize() {
        return mMinimumTextSize;
    }

    public float getMaximumTextSize() {
        return mMaximumTextSize;
    }

    public float getVariableTextSize(CharSequence text) {
        if (mWidthConstraint < 0 || mMaximumTextSize <= mMinimumTextSize) {
            // Not measured, bail early.
            return getTextSize();
        }

        // Capture current paint state.
        mTempPaint.set(getPaint());

        // Step through increasing text sizes until the text would no longer fit.
        float lastFitTextSize = mMinimumTextSize;
        while (lastFitTextSize < mMaximumTextSize) {
            mTempPaint.setTextSize(Math.min(lastFitTextSize + mStepTextSize, mMaximumTextSize));
            if (Layout.getDesiredWidth(text, mTempPaint) > mWidthConstraint) {
                break;
            }
            lastFitTextSize = mTempPaint.getTextSize();
        }

        return lastFitTextSize;
    }

    private static boolean startsWith(CharSequence whole, CharSequence prefix) {
        int wholeLen = whole.length();
        int prefixLen = prefix.length();
        if (prefixLen > wholeLen) {
            return false;
        }
        for (int i = 0; i < prefixLen; ++i) {
            if (prefix.charAt(i) != whole.charAt(i)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Functionally equivalent to setText(), but explicitly announce changes.
     * If the new text is an extension of the old one, announce the addition.
     * Otherwise, e.g. after deletion, announce the entire new text.
     */
    public void changeTextTo(CharSequence newText) {
        final CharSequence oldText = getText();
        if (startsWith(newText, oldText)) {
            final int newLen = newText.length();
            final int oldLen = oldText.length();
            if (newLen == oldLen + 1) {
                // The algorithm for pronouncing a single character doesn't seem
                // to respect our hints.  Don't give it the choice.
                final char c = newText.charAt(oldLen);
                final int id = KeyMaps.keyForChar(c);
                final String descr = KeyMaps.toDescriptiveString(getContext(), id);
                if (descr != null) {
                    announceForAccessibility(descr);
                } else {
                    announceForAccessibility(String.valueOf(c));
                }
            } else if (newLen > oldLen) {
                announceForAccessibility(newText.subSequence(oldLen, newLen));
            }
        } else {
            announceForAccessibility(newText);
        }
        setText(newText);
    }

    public boolean stopActionMode() {
        if (mActionMode != null) {
            mActionMode.finish();
            return true;
        }
        return false;
    }

    public void setOnTextSizeChangeListener(OnTextSizeChangeListener listener) {
        mOnTextSizeChangeListener = listener;
    }

    public void setOnPasteListener(OnPasteListener listener) {
        mOnPasteListener = listener;
    }

    private void paste() {
        final ClipboardManager clipboard = (ClipboardManager) getContext()
                .getSystemService(Context.CLIPBOARD_SERVICE);
        final ClipData primaryClip = clipboard.getPrimaryClip();
        if (primaryClip != null && mOnPasteListener != null) {
            mOnPasteListener.onPaste(primaryClip);
        }
    }

    public interface OnTextSizeChangeListener {
        void onTextSizeChanged(TextView textView, float oldSize);
    }

    public interface OnPasteListener {
        boolean onPaste(ClipData clip);
    }
}
