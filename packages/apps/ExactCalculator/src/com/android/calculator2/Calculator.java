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

// TODO: Copy & more general paste in formula?  Note that this requires
//       great care: Currently the text version of a displayed formula
//       is not directly useful for re-evaluating the formula later, since
//       it contains ellipses representing subexpressions evaluated with
//       a different degree mode.  Rather than supporting copy from the
//       formula window, we may eventually want to support generation of a
//       more useful text version in a separate window.  It's not clear
//       this is worth the added (code and user) complexity.

package com.android.calculator2;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ClipData;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.view.ViewPager;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.text.TextUtils;
import android.util.Property;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnKeyListener;
import android.view.View.OnLongClickListener;
import android.view.ViewAnimationUtils;
import android.view.ViewGroupOverlay;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.widget.TextView;
import android.widget.Toolbar;

import com.android.calculator2.CalculatorText.OnTextSizeChangeListener;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInput;
import java.io.ObjectInputStream;
import java.io.ObjectOutput;
import java.io.ObjectOutputStream;

public class Calculator extends Activity
        implements OnTextSizeChangeListener, OnLongClickListener, CalculatorText.OnPasteListener,
        AlertDialogFragment.OnClickListener {

    /**
     * Constant for an invalid resource id.
     */
    public static final int INVALID_RES_ID = -1;

    private enum CalculatorState {
        INPUT,          // Result and formula both visible, no evaluation requested,
                        // Though result may be visible on bottom line.
        EVALUATE,       // Both visible, evaluation requested, evaluation/animation incomplete.
                        // Not used for instant result evaluation.
        INIT,           // Very temporary state used as alternative to EVALUATE
                        // during reinitialization.  Do not animate on completion.
        ANIMATE,        // Result computed, animation to enlarge result window in progress.
        RESULT,         // Result displayed, formula invisible.
                        // If we are in RESULT state, the formula was evaluated without
                        // error to initial precision.
        ERROR           // Error displayed: Formula visible, result shows error message.
                        // Display similar to INPUT state.
    }
    // Normal transition sequence is
    // INPUT -> EVALUATE -> ANIMATE -> RESULT (or ERROR) -> INPUT
    // A RESULT -> ERROR transition is possible in rare corner cases, in which
    // a higher precision evaluation exposes an error.  This is possible, since we
    // initially evaluate assuming we were given a well-defined problem.  If we
    // were actually asked to compute sqrt(<extremely tiny negative number>) we produce 0
    // unless we are asked for enough precision that we can distinguish the argument from zero.
    // TODO: Consider further heuristics to reduce the chance of observing this?
    //       It already seems to be observable only in contrived cases.
    // ANIMATE, ERROR, and RESULT are translated to an INIT state if the application
    // is restarted in that state.  This leads us to recompute and redisplay the result
    // ASAP.
    // TODO: Possibly save a bit more information, e.g. its initial display string
    // or most significant digit position, to speed up restart.

    private final Property<TextView, Integer> TEXT_COLOR =
            new Property<TextView, Integer>(Integer.class, "textColor") {
        @Override
        public Integer get(TextView textView) {
            return textView.getCurrentTextColor();
        }

        @Override
        public void set(TextView textView, Integer textColor) {
            textView.setTextColor(textColor);
        }
    };

    // We currently assume that the formula does not change out from under us in
    // any way. We explicitly handle all input to the formula here.
    private final OnKeyListener mFormulaOnKeyListener = new OnKeyListener() {
        @Override
        public boolean onKey(View view, int keyCode, KeyEvent keyEvent) {
            stopActionMode();
            // Never consume DPAD key events.
            switch (keyCode) {
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                case KeyEvent.KEYCODE_DPAD_LEFT:
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    return false;
            }
            // Always cancel unrequested in-progress evaluation, so that we don't have
            // to worry about subsequent asynchronous completion.
            // Requested in-progress evaluations are handled below.
            if (mCurrentState != CalculatorState.EVALUATE) {
                mEvaluator.cancelAll(true);
            }
            // In other cases we go ahead and process the input normally after cancelling:
            if (keyEvent.getAction() != KeyEvent.ACTION_UP) {
                return true;
            }
            switch (keyCode) {
                case KeyEvent.KEYCODE_NUMPAD_ENTER:
                case KeyEvent.KEYCODE_ENTER:
                case KeyEvent.KEYCODE_DPAD_CENTER:
                    mCurrentButton = mEqualButton;
                    onEquals();
                    return true;
                case KeyEvent.KEYCODE_DEL:
                    mCurrentButton = mDeleteButton;
                    onDelete();
                    return true;
                default:
                    cancelIfEvaluating(false);
                    final int raw = keyEvent.getKeyCharacterMap()
                            .get(keyCode, keyEvent.getMetaState());
                    if ((raw & KeyCharacterMap.COMBINING_ACCENT) != 0) {
                        return true; // discard
                    }
                    // Try to discard non-printing characters and the like.
                    // The user will have to explicitly delete other junk that gets past us.
                    if (Character.isIdentifierIgnorable(raw)
                            || Character.isWhitespace(raw)) {
                        return true;
                    }
                    char c = (char) raw;
                    if (c == '=') {
                        mCurrentButton = mEqualButton;
                        onEquals();
                    } else {
                        addChars(String.valueOf(c), true);
                        redisplayAfterFormulaChange();
                    }
            }
            return false;
        }
    };

    private static final String NAME = Calculator.class.getName();
    private static final String KEY_DISPLAY_STATE = NAME + "_display_state";
    private static final String KEY_UNPROCESSED_CHARS = NAME + "_unprocessed_chars";
    private static final String KEY_EVAL_STATE = NAME + "_eval_state";
                // Associated value is a byte array holding both mCalculatorState
                // and the (much more complex) evaluator state.

    private CalculatorState mCurrentState;
    private Evaluator mEvaluator;

    private View mDisplayView;
    private TextView mModeView;
    private CalculatorText mFormulaText;
    private CalculatorResult mResultText;

    private ViewPager mPadViewPager;
    private View mDeleteButton;
    private View mClearButton;
    private View mEqualButton;

    private TextView mInverseToggle;
    private TextView mModeToggle;

    private View[] mInvertibleButtons;
    private View[] mInverseButtons;

    private View mCurrentButton;
    private Animator mCurrentAnimator;

    // Characters that were recently entered at the end of the display that have not yet
    // been added to the underlying expression.
    private String mUnprocessedChars = null;

    // Color to highlight unprocessed characters from physical keyboard.
    // TODO: should probably match this to the error color?
    private ForegroundColorSpan mUnprocessedColorSpan = new ForegroundColorSpan(Color.RED);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_calculator);
        setActionBar((Toolbar) findViewById(R.id.toolbar));

        // Hide all default options in the ActionBar.
        getActionBar().setDisplayOptions(0);

        mDisplayView = findViewById(R.id.display);
        mModeView = (TextView) findViewById(R.id.mode);
        mFormulaText = (CalculatorText) findViewById(R.id.formula);
        mResultText = (CalculatorResult) findViewById(R.id.result);

        mPadViewPager = (ViewPager) findViewById(R.id.pad_pager);
        mDeleteButton = findViewById(R.id.del);
        mClearButton = findViewById(R.id.clr);
        mEqualButton = findViewById(R.id.pad_numeric).findViewById(R.id.eq);
        if (mEqualButton == null || mEqualButton.getVisibility() != View.VISIBLE) {
            mEqualButton = findViewById(R.id.pad_operator).findViewById(R.id.eq);
        }

        mInverseToggle = (TextView) findViewById(R.id.toggle_inv);
        mModeToggle = (TextView) findViewById(R.id.toggle_mode);

        mInvertibleButtons = new View[] {
                findViewById(R.id.fun_sin),
                findViewById(R.id.fun_cos),
                findViewById(R.id.fun_tan),
                findViewById(R.id.fun_ln),
                findViewById(R.id.fun_log),
                findViewById(R.id.op_sqrt)
        };
        mInverseButtons = new View[] {
                findViewById(R.id.fun_arcsin),
                findViewById(R.id.fun_arccos),
                findViewById(R.id.fun_arctan),
                findViewById(R.id.fun_exp),
                findViewById(R.id.fun_10pow),
                findViewById(R.id.op_sqr)
        };

        mEvaluator = new Evaluator(this, mResultText);
        mResultText.setEvaluator(mEvaluator);
        KeyMaps.setActivity(this);

        if (savedInstanceState != null) {
            setState(CalculatorState.values()[
                savedInstanceState.getInt(KEY_DISPLAY_STATE,
                                          CalculatorState.INPUT.ordinal())]);
            CharSequence unprocessed = savedInstanceState.getCharSequence(KEY_UNPROCESSED_CHARS);
            if (unprocessed != null) {
                mUnprocessedChars = unprocessed.toString();
            }
            byte[] state = savedInstanceState.getByteArray(KEY_EVAL_STATE);
            if (state != null) {
                try (ObjectInput in = new ObjectInputStream(new ByteArrayInputStream(state))) {
                    mEvaluator.restoreInstanceState(in);
                } catch (Throwable ignored) {
                    // When in doubt, revert to clean state
                    mCurrentState = CalculatorState.INPUT;
                    mEvaluator.clear();
                }
            }
        } else {
            mCurrentState = CalculatorState.INPUT;
            mEvaluator.clear();
        }

        mFormulaText.setOnKeyListener(mFormulaOnKeyListener);
        mFormulaText.setOnTextSizeChangeListener(this);
        mFormulaText.setOnPasteListener(this);
        mDeleteButton.setOnLongClickListener(this);

        onInverseToggled(mInverseToggle.isSelected());
        onModeChanged(mEvaluator.getDegreeMode());

        if (mCurrentState != CalculatorState.INPUT) {
            // Just reevaluate.
            redisplayFormula();
            setState(CalculatorState.INIT);
            mEvaluator.requireResult();
        } else {
            redisplayAfterFormulaChange();
        }
        // TODO: We're currently not saving and restoring scroll position.
        //       We probably should.  Details may require care to deal with:
        //         - new display size
        //         - slow recomputation if we've scrolled far.
    }

    @Override
    protected void onSaveInstanceState(@NonNull Bundle outState) {
        mEvaluator.cancelAll(true);
        // If there's an animation in progress, cancel it first to ensure our state is up-to-date.
        if (mCurrentAnimator != null) {
            mCurrentAnimator.cancel();
        }

        super.onSaveInstanceState(outState);
        outState.putInt(KEY_DISPLAY_STATE, mCurrentState.ordinal());
        outState.putCharSequence(KEY_UNPROCESSED_CHARS, mUnprocessedChars);
        ByteArrayOutputStream byteArrayStream = new ByteArrayOutputStream();
        try (ObjectOutput out = new ObjectOutputStream(byteArrayStream)) {
            mEvaluator.saveInstanceState(out);
        } catch (IOException e) {
            // Impossible; No IO involved.
            throw new AssertionError("Impossible IO exception", e);
        }
        outState.putByteArray(KEY_EVAL_STATE, byteArrayStream.toByteArray());
    }

    // Set the state, updating delete label and display colors.
    // This restores display positions on moving to INPUT.
    // But movement/animation for moving to RESULT has already been done.
    private void setState(CalculatorState state) {
        if (mCurrentState != state) {
            if (state == CalculatorState.INPUT) {
                restoreDisplayPositions();
            }
            mCurrentState = state;

            if (mCurrentState == CalculatorState.RESULT) {
                // No longer do this for ERROR; allow mistakes to be corrected.
                mDeleteButton.setVisibility(View.GONE);
                mClearButton.setVisibility(View.VISIBLE);
            } else {
                mDeleteButton.setVisibility(View.VISIBLE);
                mClearButton.setVisibility(View.GONE);
            }

            if (mCurrentState == CalculatorState.ERROR) {
                final int errorColor = getColor(R.color.calculator_error_color);
                mFormulaText.setTextColor(errorColor);
                mResultText.setTextColor(errorColor);
                getWindow().setStatusBarColor(errorColor);
            } else if (mCurrentState != CalculatorState.RESULT) {
                mFormulaText.setTextColor(getColor(R.color.display_formula_text_color));
                mResultText.setTextColor(getColor(R.color.display_result_text_color));
                getWindow().setStatusBarColor(getColor(R.color.calculator_accent_color));
            }

            invalidateOptionsMenu();
        }
    }

    // Stop any active ActionMode.  Return true if there was one.
    private boolean stopActionMode() {
        if (mResultText.stopActionMode()) {
            return true;
        }
        if (mFormulaText.stopActionMode()) {
            return true;
        }
        return false;
    }

    @Override
    public void onBackPressed() {
        if (!stopActionMode()) {
            if (mPadViewPager != null && mPadViewPager.getCurrentItem() != 0) {
                // Select the previous pad.
                mPadViewPager.setCurrentItem(mPadViewPager.getCurrentItem() - 1);
            } else {
                // If the user is currently looking at the first pad (or the pad is not paged),
                // allow the system to handle the Back button.
                super.onBackPressed();
            }
        }
    }

    @Override
    public void onUserInteraction() {
        super.onUserInteraction();

        // If there's an animation in progress, end it immediately, so the user interaction can
        // be handled.
        if (mCurrentAnimator != null) {
            mCurrentAnimator.end();
        }
    }

    /**
     * Invoked whenever the inverse button is toggled to update the UI.
     *
     * @param showInverse {@code true} if inverse functions should be shown
     */
    private void onInverseToggled(boolean showInverse) {
        if (showInverse) {
            mInverseToggle.setContentDescription(getString(R.string.desc_inv_on));
            for (View invertibleButton : mInvertibleButtons) {
                invertibleButton.setVisibility(View.GONE);
            }
            for (View inverseButton : mInverseButtons) {
                inverseButton.setVisibility(View.VISIBLE);
            }
        } else {
            mInverseToggle.setContentDescription(getString(R.string.desc_inv_off));
            for (View invertibleButton : mInvertibleButtons) {
                invertibleButton.setVisibility(View.VISIBLE);
            }
            for (View inverseButton : mInverseButtons) {
                inverseButton.setVisibility(View.GONE);
            }
        }
    }

    /**
     * Invoked whenever the deg/rad mode may have changed to update the UI.
     *
     * @param degreeMode {@code true} if in degree mode
     */
    private void onModeChanged(boolean degreeMode) {
        if (degreeMode) {
            mModeView.setText(R.string.mode_deg);
            mModeView.setContentDescription(getString(R.string.desc_mode_deg));

            mModeToggle.setText(R.string.mode_rad);
            mModeToggle.setContentDescription(getString(R.string.desc_switch_rad));
        } else {
            mModeView.setText(R.string.mode_rad);
            mModeView.setContentDescription(getString(R.string.desc_mode_rad));

            mModeToggle.setText(R.string.mode_deg);
            mModeToggle.setContentDescription(getString(R.string.desc_switch_deg));
        }
    }

    /**
     * Switch to INPUT from RESULT state in response to input of the specified button_id.
     * View.NO_ID is treated as an incomplete function id.
     */
    private void switchToInput(int button_id) {
        if (KeyMaps.isBinary(button_id) || KeyMaps.isSuffix(button_id)) {
            mEvaluator.collapse();
        } else {
            announceClearedForAccessibility();
            mEvaluator.clear();
        }
        setState(CalculatorState.INPUT);
    }

    // Add the given button id to input expression.
    // If appropriate, clear the expression before doing so.
    private void addKeyToExpr(int id) {
        if (mCurrentState == CalculatorState.ERROR) {
            setState(CalculatorState.INPUT);
        } else if (mCurrentState == CalculatorState.RESULT) {
            switchToInput(id);
        }
        if (!mEvaluator.append(id)) {
            // TODO: Some user visible feedback?
        }
    }

    /**
     * Add the given button id to input expression, assuming it was explicitly
     * typed/touched.
     * We perform slightly more aggressive correction than in pasted expressions.
     */
    private void addExplicitKeyToExpr(int id) {
        if (mCurrentState == CalculatorState.INPUT && id == R.id.op_sub) {
            mEvaluator.getExpr().removeTrailingAdditiveOperators();
        }
        addKeyToExpr(id);
    }

    private void redisplayAfterFormulaChange() {
        // TODO: Could do this more incrementally.
        redisplayFormula();
        setState(CalculatorState.INPUT);
        if (mEvaluator.getExpr().hasInterestingOps()) {
            mEvaluator.evaluateAndShowResult();
        } else {
            mResultText.clear();
        }
    }

    public void onButtonClick(View view) {
        // Any animation is ended before we get here.
        mCurrentButton = view;
        stopActionMode();
        // See onKey above for the rationale behind some of the behavior below:
        if (mCurrentState != CalculatorState.EVALUATE) {
            // Cancel evaluations that were not specifically requested.
            mEvaluator.cancelAll(true);
        }
        final int id = view.getId();
        switch (id) {
            case R.id.eq:
                onEquals();
                break;
            case R.id.del:
                onDelete();
                break;
            case R.id.clr:
                onClear();
                break;
            case R.id.toggle_inv:
                final boolean selected = !mInverseToggle.isSelected();
                mInverseToggle.setSelected(selected);
                onInverseToggled(selected);
                if (mCurrentState == CalculatorState.RESULT) {
                    mResultText.redisplay();   // In case we cancelled reevaluation.
                }
                break;
            case R.id.toggle_mode:
                cancelIfEvaluating(false);
                final boolean mode = !mEvaluator.getDegreeMode();
                if (mCurrentState == CalculatorState.RESULT) {
                    mEvaluator.collapse();  // Capture result evaluated in old mode
                    redisplayFormula();
                }
                // In input mode, we reinterpret already entered trig functions.
                mEvaluator.setDegreeMode(mode);
                onModeChanged(mode);
                setState(CalculatorState.INPUT);
                mResultText.clear();
                if (mEvaluator.getExpr().hasInterestingOps()) {
                    mEvaluator.evaluateAndShowResult();
                }
                break;
            default:
                cancelIfEvaluating(false);
                addExplicitKeyToExpr(id);
                redisplayAfterFormulaChange();
                break;
        }
    }

    void redisplayFormula() {
        SpannableStringBuilder formula = mEvaluator.getExpr().toSpannableStringBuilder(this);
        if (mUnprocessedChars != null) {
            // Add and highlight characters we couldn't process.
            formula.append(mUnprocessedChars, mUnprocessedColorSpan,
                    Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        mFormulaText.changeTextTo(formula);
    }

    @Override
    public boolean onLongClick(View view) {
        mCurrentButton = view;

        if (view.getId() == R.id.del) {
            onClear();
            return true;
        }
        return false;
    }

    // Initial evaluation completed successfully.  Initiate display.
    public void onEvaluate(int initDisplayPrec, int msd, int leastDigPos,
            String truncatedWholeNumber) {
        // Invalidate any options that may depend on the current result.
        invalidateOptionsMenu();

        mResultText.displayResult(initDisplayPrec, msd, leastDigPos, truncatedWholeNumber);
        if (mCurrentState != CalculatorState.INPUT) { // in EVALUATE or INIT state
            onResult(mCurrentState != CalculatorState.INIT);
        }
    }

    // Reset state to reflect evaluator cancellation.  Invoked by evaluator.
    public void onCancelled() {
        // We should be in EVALUATE state.
        setState(CalculatorState.INPUT);
        mResultText.clear();
    }

    // Reevaluation completed; ask result to redisplay current value.
    public void onReevaluate()
    {
        mResultText.redisplay();
    }

    @Override
    public void onTextSizeChanged(final TextView textView, float oldSize) {
        if (mCurrentState != CalculatorState.INPUT) {
            // Only animate text changes that occur from user input.
            return;
        }

        // Calculate the values needed to perform the scale and translation animations,
        // maintaining the same apparent baseline for the displayed text.
        final float textScale = oldSize / textView.getTextSize();
        final float translationX = (1.0f - textScale) *
                (textView.getWidth() / 2.0f - textView.getPaddingEnd());
        final float translationY = (1.0f - textScale) *
                (textView.getHeight() / 2.0f - textView.getPaddingBottom());

        final AnimatorSet animatorSet = new AnimatorSet();
        animatorSet.playTogether(
                ObjectAnimator.ofFloat(textView, View.SCALE_X, textScale, 1.0f),
                ObjectAnimator.ofFloat(textView, View.SCALE_Y, textScale, 1.0f),
                ObjectAnimator.ofFloat(textView, View.TRANSLATION_X, translationX, 0.0f),
                ObjectAnimator.ofFloat(textView, View.TRANSLATION_Y, translationY, 0.0f));
        animatorSet.setDuration(getResources().getInteger(android.R.integer.config_mediumAnimTime));
        animatorSet.setInterpolator(new AccelerateDecelerateInterpolator());
        animatorSet.start();
    }

    /**
     * Cancel any in-progress explicitly requested evaluations.
     * @param quiet suppress pop-up message.  Explicit evaluation can change the expression
                    value, and certainly changes the display, so it seems reasonable to warn.
     * @return      true if there was such an evaluation
     */
    private boolean cancelIfEvaluating(boolean quiet) {
        if (mCurrentState == CalculatorState.EVALUATE) {
            mEvaluator.cancelAll(quiet);
            return true;
        } else {
            return false;
        }
    }

    private void onEquals() {
        // In non-INPUT state assume this was redundant and ignore it.
        if (mCurrentState == CalculatorState.INPUT && !mEvaluator.getExpr().isEmpty()) {
            setState(CalculatorState.EVALUATE);
            mEvaluator.requireResult();
        }
    }

    private void onDelete() {
        // Delete works like backspace; remove the last character or operator from the expression.
        // Note that we handle keyboard delete exactly like the delete button.  For
        // example the delete button can be used to delete a character from an incomplete
        // function name typed on a physical keyboard.
        // This should be impossible in RESULT state.
        // If there is an in-progress explicit evaluation, just cancel it and return.
        if (cancelIfEvaluating(false)) return;
        setState(CalculatorState.INPUT);
        if (mUnprocessedChars != null) {
            int len = mUnprocessedChars.length();
            if (len > 0) {
                mUnprocessedChars = mUnprocessedChars.substring(0, len-1);
            } else {
                mEvaluator.delete();
            }
        } else {
            mEvaluator.delete();
        }
        if (mEvaluator.getExpr().isEmpty()
                && (mUnprocessedChars == null || mUnprocessedChars.isEmpty())) {
            // Resulting formula won't be announced, since it's empty.
            announceClearedForAccessibility();
        }
        redisplayAfterFormulaChange();
    }

    private void reveal(View sourceView, int colorRes, AnimatorListener listener) {
        final ViewGroupOverlay groupOverlay =
                (ViewGroupOverlay) getWindow().getDecorView().getOverlay();

        final Rect displayRect = new Rect();
        mDisplayView.getGlobalVisibleRect(displayRect);

        // Make reveal cover the display and status bar.
        final View revealView = new View(this);
        revealView.setBottom(displayRect.bottom);
        revealView.setLeft(displayRect.left);
        revealView.setRight(displayRect.right);
        revealView.setBackgroundColor(getResources().getColor(colorRes));
        groupOverlay.add(revealView);

        final int[] clearLocation = new int[2];
        sourceView.getLocationInWindow(clearLocation);
        clearLocation[0] += sourceView.getWidth() / 2;
        clearLocation[1] += sourceView.getHeight() / 2;

        final int revealCenterX = clearLocation[0] - revealView.getLeft();
        final int revealCenterY = clearLocation[1] - revealView.getTop();

        final double x1_2 = Math.pow(revealView.getLeft() - revealCenterX, 2);
        final double x2_2 = Math.pow(revealView.getRight() - revealCenterX, 2);
        final double y_2 = Math.pow(revealView.getTop() - revealCenterY, 2);
        final float revealRadius = (float) Math.max(Math.sqrt(x1_2 + y_2), Math.sqrt(x2_2 + y_2));

        final Animator revealAnimator =
                ViewAnimationUtils.createCircularReveal(revealView,
                        revealCenterX, revealCenterY, 0.0f, revealRadius);
        revealAnimator.setDuration(
                getResources().getInteger(android.R.integer.config_longAnimTime));
        revealAnimator.addListener(listener);

        final Animator alphaAnimator = ObjectAnimator.ofFloat(revealView, View.ALPHA, 0.0f);
        alphaAnimator.setDuration(
                getResources().getInteger(android.R.integer.config_mediumAnimTime));

        final AnimatorSet animatorSet = new AnimatorSet();
        animatorSet.play(revealAnimator).before(alphaAnimator);
        animatorSet.setInterpolator(new AccelerateDecelerateInterpolator());
        animatorSet.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animator) {
                groupOverlay.remove(revealView);
                mCurrentAnimator = null;
            }
        });

        mCurrentAnimator = animatorSet;
        animatorSet.start();
    }

    private void announceClearedForAccessibility() {
        mResultText.announceForAccessibility(getResources().getString(R.string.cleared));
    }

    private void onClear() {
        if (mEvaluator.getExpr().isEmpty()) {
            return;
        }
        cancelIfEvaluating(true);
        announceClearedForAccessibility();
        reveal(mCurrentButton, R.color.calculator_accent_color, new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                mUnprocessedChars = null;
                mResultText.clear();
                mEvaluator.clear();
                setState(CalculatorState.INPUT);
                redisplayFormula();
            }
        });
    }

    // Evaluation encountered en error.  Display the error.
    void onError(final int errorResourceId) {
        if (mCurrentState == CalculatorState.EVALUATE) {
            setState(CalculatorState.ANIMATE);
            mResultText.announceForAccessibility(getResources().getString(errorResourceId));
            reveal(mCurrentButton, R.color.calculator_error_color,
                    new AnimatorListenerAdapter() {
                        @Override
                        public void onAnimationEnd(Animator animation) {
                           setState(CalculatorState.ERROR);
                           mResultText.displayError(errorResourceId);
                        }
                    });
        } else if (mCurrentState == CalculatorState.INIT) {
            setState(CalculatorState.ERROR);
            mResultText.displayError(errorResourceId);
        } else {
            mResultText.clear();
        }
    }


    // Animate movement of result into the top formula slot.
    // Result window now remains translated in the top slot while the result is displayed.
    // (We convert it back to formula use only when the user provides new input.)
    // Historical note: In the Lollipop version, this invisibly and instantaneously moved
    // formula and result displays back at the end of the animation.  We no longer do that,
    // so that we can continue to properly support scrolling of the result.
    // We assume the result already contains the text to be expanded.
    private void onResult(boolean animate) {
        // Calculate the textSize that would be used to display the result in the formula.
        // For scrollable results just use the minimum textSize to maximize the number of digits
        // that are visible on screen.
        float textSize = mFormulaText.getMinimumTextSize();
        if (!mResultText.isScrollable()) {
            textSize = mFormulaText.getVariableTextSize(mResultText.getText().toString());
        }

        // Scale the result to match the calculated textSize, minimizing the jump-cut transition
        // when a result is reused in a subsequent expression.
        final float resultScale = textSize / mResultText.getTextSize();

        // Set the result's pivot to match its gravity.
        mResultText.setPivotX(mResultText.getWidth() - mResultText.getPaddingRight());
        mResultText.setPivotY(mResultText.getHeight() - mResultText.getPaddingBottom());

        // Calculate the necessary translations so the result takes the place of the formula and
        // the formula moves off the top of the screen.
        final float resultTranslationY = (mFormulaText.getBottom() - mResultText.getBottom())
                - (mFormulaText.getPaddingBottom() - mResultText.getPaddingBottom());
        final float formulaTranslationY = -mFormulaText.getBottom();

        // Change the result's textColor to match the formula.
        final int formulaTextColor = mFormulaText.getCurrentTextColor();

        if (animate) {
            mResultText.announceForAccessibility(getResources().getString(R.string.desc_eq));
            mResultText.announceForAccessibility(mResultText.getText());
            setState(CalculatorState.ANIMATE);
            final AnimatorSet animatorSet = new AnimatorSet();
            animatorSet.playTogether(
                    ObjectAnimator.ofPropertyValuesHolder(mResultText,
                            PropertyValuesHolder.ofFloat(View.SCALE_X, resultScale),
                            PropertyValuesHolder.ofFloat(View.SCALE_Y, resultScale),
                            PropertyValuesHolder.ofFloat(View.TRANSLATION_Y, resultTranslationY)),
                    ObjectAnimator.ofArgb(mResultText, TEXT_COLOR, formulaTextColor),
                    ObjectAnimator.ofFloat(mFormulaText, View.TRANSLATION_Y, formulaTranslationY));
            animatorSet.setDuration(getResources().getInteger(
                    android.R.integer.config_longAnimTime));
            animatorSet.addListener(new AnimatorListenerAdapter() {
                @Override
                public void onAnimationEnd(Animator animation) {
                    setState(CalculatorState.RESULT);
                    mCurrentAnimator = null;
                }
            });

            mCurrentAnimator = animatorSet;
            animatorSet.start();
        } else /* No animation desired; get there fast, e.g. when restarting */ {
            mResultText.setScaleX(resultScale);
            mResultText.setScaleY(resultScale);
            mResultText.setTranslationY(resultTranslationY);
            mResultText.setTextColor(formulaTextColor);
            mFormulaText.setTranslationY(formulaTranslationY);
            setState(CalculatorState.RESULT);
        }
    }

    // Restore positions of the formula and result displays back to their original,
    // pre-animation state.
    private void restoreDisplayPositions() {
        // Clear result.
        mResultText.setText("");
        // Reset all of the values modified during the animation.
        mResultText.setScaleX(1.0f);
        mResultText.setScaleY(1.0f);
        mResultText.setTranslationX(0.0f);
        mResultText.setTranslationY(0.0f);
        mFormulaText.setTranslationY(0.0f);

        mFormulaText.requestFocus();
    }

    @Override
    public void onClick(AlertDialogFragment fragment, int which) {
        if (which == DialogInterface.BUTTON_POSITIVE) {
            // Timeout extension request.
            mEvaluator.setLongTimeOut();
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);

        getMenuInflater().inflate(R.menu.activity_calculator, menu);
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);

        // Show the leading option when displaying a result.
        menu.findItem(R.id.menu_leading).setVisible(mCurrentState == CalculatorState.RESULT);

        // Show the fraction option when displaying a rational result.
        menu.findItem(R.id.menu_fraction).setVisible(mCurrentState == CalculatorState.RESULT
                && mEvaluator.getRational() != null);

        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.menu_leading:
                displayFull();
                return true;
            case R.id.menu_fraction:
                displayFraction();
                return true;
            case R.id.menu_licenses:
                startActivity(new Intent(this, Licenses.class));
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    private void displayMessage(String s) {
        AlertDialogFragment.showMessageDialog(this, s, null);
    }

    private void displayFraction() {
        BoundedRational result = mEvaluator.getRational();
        displayMessage(KeyMaps.translateResult(result.toNiceString()));
    }

    // Display full result to currently evaluated precision
    private void displayFull() {
        Resources res = getResources();
        String msg = mResultText.getFullText() + " ";
        if (mResultText.fullTextIsExact()) {
            msg += res.getString(R.string.exact);
        } else {
            msg += res.getString(R.string.approximate);
        }
        displayMessage(msg);
    }

    /**
     * Add input characters to the end of the expression.
     * Map them to the appropriate button pushes when possible.  Leftover characters
     * are added to mUnprocessedChars, which is presumed to immediately precede the newly
     * added characters.
     * @param moreChars Characters to be added.
     * @param explicit These characters were explicitly typed by the user, not pasted.
     */
    private void addChars(String moreChars, boolean explicit) {
        if (mUnprocessedChars != null) {
            moreChars = mUnprocessedChars + moreChars;
        }
        int current = 0;
        int len = moreChars.length();
        boolean lastWasDigit = false;
        if (mCurrentState == CalculatorState.RESULT && len != 0) {
            // Clear display immediately for incomplete function name.
            switchToInput(KeyMaps.keyForChar(moreChars.charAt(current)));
        }
        while (current < len) {
            char c = moreChars.charAt(current);
            int k = KeyMaps.keyForChar(c);
            if (!explicit) {
                int expEnd;
                if (lastWasDigit && current !=
                        (expEnd = Evaluator.exponentEnd(moreChars, current))) {
                    // Process scientific notation with 'E' when pasting, in spite of ambiguity
                    // with base of natural log.
                    // Otherwise the 10^x key is the user's friend.
                    mEvaluator.addExponent(moreChars, current, expEnd);
                    current = expEnd;
                    lastWasDigit = false;
                    continue;
                } else {
                    boolean isDigit = KeyMaps.digVal(k) != KeyMaps.NOT_DIGIT;
                    if (current == 0 && (isDigit || k == R.id.dec_point)
                            && mEvaluator.getExpr().hasTrailingConstant()) {
                        // Refuse to concatenate pasted content to trailing constant.
                        // This makes pasting of calculator results more consistent, whether or
                        // not the old calculator instance is still around.
                        addKeyToExpr(R.id.op_mul);
                    }
                    lastWasDigit = (isDigit || lastWasDigit && k == R.id.dec_point);
                }
            }
            if (k != View.NO_ID) {
                mCurrentButton = findViewById(k);
                if (explicit) {
                    addExplicitKeyToExpr(k);
                } else {
                    addKeyToExpr(k);
                }
                if (Character.isSurrogate(c)) {
                    current += 2;
                } else {
                    ++current;
                }
                continue;
            }
            int f = KeyMaps.funForString(moreChars, current);
            if (f != View.NO_ID) {
                mCurrentButton = findViewById(f);
                if (explicit) {
                    addExplicitKeyToExpr(f);
                } else {
                    addKeyToExpr(f);
                }
                if (f == R.id.op_sqrt) {
                    // Square root entered as function; don't lose the parenthesis.
                    addKeyToExpr(R.id.lparen);
                }
                current = moreChars.indexOf('(', current) + 1;
                continue;
            }
            // There are characters left, but we can't convert them to button presses.
            mUnprocessedChars = moreChars.substring(current);
            redisplayAfterFormulaChange();
            return;
        }
        mUnprocessedChars = null;
        redisplayAfterFormulaChange();
    }

    @Override
    public boolean onPaste(ClipData clip) {
        final ClipData.Item item = clip.getItemCount() == 0 ? null : clip.getItemAt(0);
        if (item == null) {
            // nothing to paste, bail early...
            return false;
        }

        // Check if the item is a previously copied result, otherwise paste as raw text.
        final Uri uri = item.getUri();
        if (uri != null && mEvaluator.isLastSaved(uri)) {
            if (mCurrentState == CalculatorState.ERROR
                    || mCurrentState == CalculatorState.RESULT) {
                setState(CalculatorState.INPUT);
                mEvaluator.clear();
            }
            mEvaluator.appendSaved();
            redisplayAfterFormulaChange();
        } else {
            addChars(item.coerceToText(this).toString(), false);
        }
        return true;
    }
}
