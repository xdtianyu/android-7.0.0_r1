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

import android.content.res.Resources;
import android.content.Context;
import android.app.Activity;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import java.text.DecimalFormatSymbols;
import java.util.HashMap;
import java.util.Locale;

/**
 * Collection of mapping functions between key ids, characters, internationalized
 * and non-internationalized characters, etc.
 * <p>
 * KeyMap instances are not meaningful; everything here is static.
 * All functions are either pure, or are assumed to be called only from a single UI thread.
 */
public class KeyMaps {
    /**
     * Map key id to corresponding (internationalized) display string.
     * Pure function.
     */
    public static String toString(Context context, int id) {
        switch(id) {
            case R.id.const_pi:
                return context.getString(R.string.const_pi);
            case R.id.const_e:
                return context.getString(R.string.const_e);
            case R.id.op_sqrt:
                return context.getString(R.string.op_sqrt);
            case R.id.op_fact:
                return context.getString(R.string.op_fact);
            case R.id.op_pct:
                return context.getString(R.string.op_pct);
            case R.id.fun_sin:
                return context.getString(R.string.fun_sin) + context.getString(R.string.lparen);
            case R.id.fun_cos:
                return context.getString(R.string.fun_cos) + context.getString(R.string.lparen);
            case R.id.fun_tan:
                return context.getString(R.string.fun_tan) + context.getString(R.string.lparen);
            case R.id.fun_arcsin:
                return context.getString(R.string.fun_arcsin) + context.getString(R.string.lparen);
            case R.id.fun_arccos:
                return context.getString(R.string.fun_arccos) + context.getString(R.string.lparen);
            case R.id.fun_arctan:
                return context.getString(R.string.fun_arctan) + context.getString(R.string.lparen);
            case R.id.fun_ln:
                return context.getString(R.string.fun_ln) + context.getString(R.string.lparen);
            case R.id.fun_log:
                return context.getString(R.string.fun_log) + context.getString(R.string.lparen);
            case R.id.fun_exp:
                // Button label doesn't work.
                return context.getString(R.string.exponential) + context.getString(R.string.lparen);
            case R.id.lparen:
                return context.getString(R.string.lparen);
            case R.id.rparen:
                return context.getString(R.string.rparen);
            case R.id.op_pow:
                return context.getString(R.string.op_pow);
            case R.id.op_mul:
                return context.getString(R.string.op_mul);
            case R.id.op_div:
                return context.getString(R.string.op_div);
            case R.id.op_add:
                return context.getString(R.string.op_add);
            case R.id.op_sqr:
                // Button label doesn't work.
                return context.getString(R.string.squared);
            case R.id.op_sub:
                return context.getString(R.string.op_sub);
            case R.id.dec_point:
                return context.getString(R.string.dec_point);
            case R.id.digit_0:
                return context.getString(R.string.digit_0);
            case R.id.digit_1:
                return context.getString(R.string.digit_1);
            case R.id.digit_2:
                return context.getString(R.string.digit_2);
            case R.id.digit_3:
                return context.getString(R.string.digit_3);
            case R.id.digit_4:
                return context.getString(R.string.digit_4);
            case R.id.digit_5:
                return context.getString(R.string.digit_5);
            case R.id.digit_6:
                return context.getString(R.string.digit_6);
            case R.id.digit_7:
                return context.getString(R.string.digit_7);
            case R.id.digit_8:
                return context.getString(R.string.digit_8);
            case R.id.digit_9:
                return context.getString(R.string.digit_9);
            default:
                return "";
        }
    }

    /**
     * Map key id to corresponding (internationalized) descriptive string that can be used
     * to correctly read back a formula.
     * Only used for operators and individual characters; not used inside constants.
     * Returns null when we don't need a descriptive string.
     * Pure function.
     */
    public static String toDescriptiveString(Context context, int id) {
        switch(id) {
            case R.id.op_fact:
                return context.getString(R.string.desc_op_fact);
            case R.id.fun_sin:
                return context.getString(R.string.desc_fun_sin)
                        + " " + context.getString(R.string.desc_lparen);
            case R.id.fun_cos:
                return context.getString(R.string.desc_fun_cos)
                        + " " + context.getString(R.string.desc_lparen);
            case R.id.fun_tan:
                return context.getString(R.string.desc_fun_tan)
                        + " " + context.getString(R.string.desc_lparen);
            case R.id.fun_arcsin:
                return context.getString(R.string.desc_fun_arcsin)
                        + " " + context.getString(R.string.desc_lparen);
            case R.id.fun_arccos:
                return context.getString(R.string.desc_fun_arccos)
                        + " " + context.getString(R.string.desc_lparen);
            case R.id.fun_arctan:
                return context.getString(R.string.desc_fun_arctan)
                        + " " + context.getString(R.string.desc_lparen);
            case R.id.fun_ln:
                return context.getString(R.string.desc_fun_ln)
                        + " " + context.getString(R.string.desc_lparen);
            case R.id.fun_log:
                return context.getString(R.string.desc_fun_log)
                        + " " + context.getString(R.string.desc_lparen);
            case R.id.fun_exp:
                return context.getString(R.string.desc_fun_exp)
                        + " " + context.getString(R.string.desc_lparen);
            case R.id.lparen:
                return context.getString(R.string.desc_lparen);
            case R.id.rparen:
                return context.getString(R.string.desc_rparen);
            case R.id.op_pow:
                return context.getString(R.string.desc_op_pow);
            case R.id.dec_point:
                return context.getString(R.string.desc_dec_point);
            default:
                return null;
        }
    }

    /**
     * Does a button id correspond to a binary operator?
     * Pure function.
     */
    public static boolean isBinary(int id) {
        switch(id) {
            case R.id.op_pow:
            case R.id.op_mul:
            case R.id.op_div:
            case R.id.op_add:
            case R.id.op_sub:
                return true;
            default:
                return false;
        }
    }

    /**
     * Does a button id correspond to a function that introduces an implicit lparen?
     * Pure function.
     */
    public static boolean isFunc(int id) {
        switch(id) {
            case R.id.fun_sin:
            case R.id.fun_cos:
            case R.id.fun_tan:
            case R.id.fun_arcsin:
            case R.id.fun_arccos:
            case R.id.fun_arctan:
            case R.id.fun_ln:
            case R.id.fun_log:
            case R.id.fun_exp:
                return true;
            default:
                return false;
        }
    }

    /**
     * Does a button id correspond to a prefix operator?
     * Pure function.
     */
    public static boolean isPrefix(int id) {
        switch(id) {
            case R.id.op_sqrt:
            case R.id.op_sub:
                return true;
            default:
                return false;
        }
    }

    /**
     * Does a button id correspond to a suffix operator?
     */
    public static boolean isSuffix(int id) {
        switch (id) {
            case R.id.op_fact:
            case R.id.op_pct:
            case R.id.op_sqr:
                return true;
            default:
                return false;
        }
    }

    public static final int NOT_DIGIT = 10;

    public static final String ELLIPSIS = "\u2026";

    public static final char MINUS_SIGN = '\u2212';

    /**
     * Map key id to digit or NOT_DIGIT
     * Pure function.
     */
    public static int digVal(int id) {
        switch (id) {
        case R.id.digit_0:
            return 0;
        case R.id.digit_1:
            return 1;
        case R.id.digit_2:
            return 2;
        case R.id.digit_3:
            return 3;
        case R.id.digit_4:
            return 4;
        case R.id.digit_5:
            return 5;
        case R.id.digit_6:
            return 6;
        case R.id.digit_7:
            return 7;
        case R.id.digit_8:
            return 8;
        case R.id.digit_9:
            return 9;
        default:
            return NOT_DIGIT;
        }
    }

    /**
     * Map digit to corresponding key.  Inverse of above.
     * Pure function.
     */
    public static int keyForDigVal(int v) {
        switch(v) {
        case 0:
            return R.id.digit_0;
        case 1:
            return R.id.digit_1;
        case 2:
            return R.id.digit_2;
        case 3:
            return R.id.digit_3;
        case 4:
            return R.id.digit_4;
        case 5:
            return R.id.digit_5;
        case 6:
            return R.id.digit_6;
        case 7:
            return R.id.digit_7;
        case 8:
            return R.id.digit_8;
        case 9:
            return R.id.digit_9;
        default:
            return View.NO_ID;
        }
    }

    // The following two are only used for recognizing additional
    // input characters from a physical keyboard.  They are not used
    // for output internationalization.
    private static char mDecimalPt;

    private static char mPiChar;

    /**
     * Character used as a placeholder for digits that are currently unknown in a result that
     * is being computed.  We initially generate blanks, and then use this as a replacement
     * during final translation.
     * <p/>
     * Note: the character must correspond closely to the width of a digit,
     * otherwise the UI will visibly shift once the computation is finished.
     */
    private static final char CHAR_DIGIT_UNKNOWN = '\u2007';

    /**
     * Map typed function name strings to corresponding button ids.
     * We (now redundantly?) include both localized and English names.
     */
    private static HashMap<String, Integer> sKeyValForFun;

    /**
     * Result string corresponding to a character in the calculator result.
     * The string values in the map are expected to be one character long.
     */
    private static HashMap<Character, String> sOutputForResultChar;

    /**
     * Locale string corresponding to preceding map and character constants.
     * We recompute the map if this is not the current locale.
     */
    private static String sLocaleForMaps = "none";

    /**
     * Activity to use for looking up buttons.
     */
    private static Activity mActivity;

    /**
     * Set acttivity used for looking up button labels.
     * Call only from UI thread.
     */
    public static void setActivity(Activity a) {
        mActivity = a;
    }

    /**
     * Return the button id corresponding to the supplied character or return NO_ID.
     * Called only by UI thread.
     */
    public static int keyForChar(char c) {
        validateMaps();
        if (Character.isDigit(c)) {
            int i = Character.digit(c, 10);
            return KeyMaps.keyForDigVal(i);
        }
        switch (c) {
            case '.':
            case ',':
                return R.id.dec_point;
            case '-':
            case MINUS_SIGN:
                return R.id.op_sub;
            case '+':
                return R.id.op_add;
            case '*':
            case '\u00D7': // MULTIPLICATION SIGN
                return R.id.op_mul;
            case '/':
            case '\u00F7': // DIVISION SIGN
                return R.id.op_div;
            // We no longer localize function names, so they can't start with an 'e' or 'p'.
            case 'e':
            case 'E':
                return R.id.const_e;
            case 'p':
            case 'P':
                return R.id.const_pi;
            case '^':
                return R.id.op_pow;
            case '!':
                return R.id.op_fact;
            case '%':
                return R.id.op_pct;
            case '(':
                return R.id.lparen;
            case ')':
                return R.id.rparen;
            default:
                if (c == mDecimalPt) return R.id.dec_point;
                if (c == mPiChar) return R.id.const_pi;
                    // pi is not translated, but it might be typable on a Greek keyboard,
                    // or pasted in, so we check ...
                return View.NO_ID;
        }
    }

    /**
     * Add information corresponding to the given button id to sKeyValForFun, to be used
     * when mapping keyboard input to button ids.
     */
    static void addButtonToFunMap(int button_id) {
        Button button = (Button)mActivity.findViewById(button_id);
        sKeyValForFun.put(button.getText().toString(), button_id);
    }

    /**
     * Add information corresponding to the given button to sOutputForResultChar, to be used
     * when translating numbers on output.
     */
    static void addButtonToOutputMap(char c, int button_id) {
        Button button = (Button)mActivity.findViewById(button_id);
        sOutputForResultChar.put(c, button.getText().toString());
    }

    // Ensure that the preceding map and character constants are
    // initialized and correspond to the current locale.
    // Called only by a single thread, namely the UI thread.
    static void validateMaps() {
        Locale locale = Locale.getDefault();
        String lname = locale.toString();
        if (lname != sLocaleForMaps) {
            Log.v ("Calculator", "Setting local to: " + lname);
            sKeyValForFun = new HashMap<String, Integer>();
            sKeyValForFun.put("sin", R.id.fun_sin);
            sKeyValForFun.put("cos", R.id.fun_cos);
            sKeyValForFun.put("tan", R.id.fun_tan);
            sKeyValForFun.put("arcsin", R.id.fun_arcsin);
            sKeyValForFun.put("arccos", R.id.fun_arccos);
            sKeyValForFun.put("arctan", R.id.fun_arctan);
            sKeyValForFun.put("asin", R.id.fun_arcsin);
            sKeyValForFun.put("acos", R.id.fun_arccos);
            sKeyValForFun.put("atan", R.id.fun_arctan);
            sKeyValForFun.put("ln", R.id.fun_ln);
            sKeyValForFun.put("log", R.id.fun_log);
            sKeyValForFun.put("sqrt", R.id.op_sqrt); // special treatment
            addButtonToFunMap(R.id.fun_sin);
            addButtonToFunMap(R.id.fun_cos);
            addButtonToFunMap(R.id.fun_tan);
            addButtonToFunMap(R.id.fun_arcsin);
            addButtonToFunMap(R.id.fun_arccos);
            addButtonToFunMap(R.id.fun_arctan);
            addButtonToFunMap(R.id.fun_ln);
            addButtonToFunMap(R.id.fun_log);

            // Set locale-dependent character "constants"
            mDecimalPt =
                DecimalFormatSymbols.getInstance().getDecimalSeparator();
                // We recognize this in keyboard input, even if we use
                // a different character.
            Resources res = mActivity.getResources();
            mPiChar = 0;
            String piString = res.getString(R.string.const_pi);
            if (piString.length() == 1) {
                mPiChar = piString.charAt(0);
            }

            sOutputForResultChar = new HashMap<Character, String>();
            sOutputForResultChar.put('e', "E");
            sOutputForResultChar.put('E', "E");
            sOutputForResultChar.put(' ', String.valueOf(CHAR_DIGIT_UNKNOWN));
            sOutputForResultChar.put(ELLIPSIS.charAt(0), ELLIPSIS);
            sOutputForResultChar.put('/', "/");
                        // Translate numbers for fraction display, but not
                        // the separating slash, which appears to be
                        // universal.
            addButtonToOutputMap('-', R.id.op_sub);
            addButtonToOutputMap('.', R.id.dec_point);
            for (int i = 0; i <= 9; ++i) {
                addButtonToOutputMap((char)('0' + i), keyForDigVal(i));
            }

            sLocaleForMaps = lname;

        }
    }

    /**
     * Return function button id for the substring of s starting at pos and ending with
     * the next "(".  Return NO_ID if there is none.
     * We currently check for both (possibly localized) button labels, and standard
     * English names.  (They should currently be the same, and hence this is currently redundant.)
     * Callable only from UI thread.
     */
    public static int funForString(String s, int pos) {
        validateMaps();
        int parenPos = s.indexOf('(', pos);
        if (parenPos != -1) {
            String funString = s.substring(pos, parenPos);
            Integer keyValue = sKeyValForFun.get(funString);
            if (keyValue == null) return View.NO_ID;
            return keyValue;
        }
        return View.NO_ID;
    }

    /**
     * Return the localization of the string s representing a numeric answer.
     * Callable only from UI thread.
     */
    public static String translateResult(String s) {
        StringBuilder result = new StringBuilder();
        int len = s.length();
        validateMaps();
        for (int i = 0; i < len; ++i) {
            char c = s.charAt(i);
            String translation = sOutputForResultChar.get(c);
            if (translation == null) {
                // Should not get here.  Report if we do.
                Log.v("Calculator", "Bad character:" + c);
                result.append(String.valueOf(c));
            } else {
                result.append(translation);
            }
        }
        return result.toString();
    }

}
