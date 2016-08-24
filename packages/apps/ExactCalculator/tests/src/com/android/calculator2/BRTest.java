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

// A test for BoundedRationals package.

package com.android.calculator2;

import com.hp.creals.CR;
import com.hp.creals.UnaryCRFunction;

import junit.framework.AssertionFailedError;
import junit.framework.TestCase;

import java.math.BigInteger;

public class BRTest extends TestCase {
    private static void check(boolean x, String s) {
        if (!x) throw new AssertionFailedError(s);
    }
    final static int TEST_PREC = -100; // 100 bits to the right of
                                       // binary point.
    private static void checkEq(BoundedRational x, CR y, String s) {
        check(x.CRValue().compareTo(y, TEST_PREC) == 0, s);
    }
    private static void checkWeakEq(BoundedRational x, CR y, String s) {
        if (x != null) checkEq(x, y, s);
    }

    private final static UnaryCRFunction ASIN = UnaryCRFunction.asinFunction;
    private final static UnaryCRFunction ACOS = UnaryCRFunction.acosFunction;
    private final static UnaryCRFunction ATAN = UnaryCRFunction.atanFunction;
    private final static UnaryCRFunction TAN = UnaryCRFunction.tanFunction;
    private final static BoundedRational BR_0 = new BoundedRational(0);
    private final static BoundedRational BR_M1 = new BoundedRational(-1);
    private final static BoundedRational BR_2 = new BoundedRational(2);
    private final static BoundedRational BR_M2 = new BoundedRational(-2);
    private final static BoundedRational BR_15 = new BoundedRational(15);
    private final static BoundedRational BR_390 = new BoundedRational(390);
    private final static BoundedRational BR_M390 = new BoundedRational(-390);
    private final static CR CR_1 = CR.valueOf(1);

    private final static CR RADIANS_PER_DEGREE = CR.PI.divide(CR.valueOf(180));
    private final static CR DEGREES_PER_RADIAN = CR.valueOf(180).divide(CR.PI);
    private final static CR LN10 = CR.valueOf(10).ln();

    private static CR toRadians(CR x) {
        return x.multiply(RADIANS_PER_DEGREE);
    }

    private static CR fromRadians(CR x) {
        return x.multiply(DEGREES_PER_RADIAN);
    }

    // We assume that x is simple enough that we don't overflow bounds.
    private static void checkBR(BoundedRational x) {
        check(x != null, "test data should not be null");
        CR xAsCR = x.CRValue();
        checkEq(BoundedRational.add(x, BoundedRational.ONE), xAsCR.add(CR_1),
                "add 1:" + x);
        checkEq(BoundedRational.subtract(x, BoundedRational.MINUS_THIRTY),
                xAsCR.subtract(CR.valueOf(-30)), "sub -30:" + x);
        checkEq(BoundedRational.multiply(x, BR_15),
                xAsCR.multiply(CR.valueOf(15)), "multiply 15:" + x);
        checkEq(BoundedRational.divide(x, BR_15),
                xAsCR.divide(CR.valueOf(15)), "divide 15:" + x);
        checkWeakEq(BoundedRational.sin(x), xAsCR.sin(), "sin:" + x);
        checkWeakEq(BoundedRational.cos(x), xAsCR.cos(), "cos:" + x);
        checkWeakEq(BoundedRational.tan(x), TAN.execute(xAsCR), "tan:" + x);
        checkWeakEq(BoundedRational.degreeSin(x), toRadians(xAsCR).sin(),
                "degree sin:" + x);
        checkWeakEq(BoundedRational.degreeCos(x), toRadians(xAsCR).cos(),
                "degree cos:" + x);
        BigInteger big_x = BoundedRational.asBigInteger(x);
        long long_x = (big_x == null? 0 : big_x.longValue());
        try {
            checkWeakEq(BoundedRational.degreeTan(x),
                        TAN.execute(toRadians(xAsCR)), "degree tan:" + x);
            check((long_x - 90) % 180 != 0, "missed undefined tan: " + x);
        } catch (ArithmeticException ignored) {
            check((long_x - 90) % 180 == 0, "exception on defined tan: " + x);
        }
        if (x.compareTo(BoundedRational.THIRTY) <= 0
                && x.compareTo(BoundedRational.MINUS_THIRTY) >= 0) {
            checkWeakEq(BoundedRational.exp(x), xAsCR.exp(), "exp:" + x);
            checkWeakEq(BoundedRational.pow(BR_15, x),
                    CR.valueOf(15).ln().multiply(xAsCR).exp(),
                    "pow(15,x):" + x);
        }
        if (x.compareTo(BoundedRational.ONE) <= 0
                && x.compareTo(BoundedRational.MINUS_ONE) >= 0) {
            checkWeakEq(BoundedRational.asin(x), ASIN.execute(xAsCR),
                        "asin:" + x);
            checkWeakEq(BoundedRational.acos(x), ACOS.execute(xAsCR),
                        "acos:" + x);
            checkWeakEq(BoundedRational.degreeAsin(x),
                        fromRadians(ASIN.execute(xAsCR)), "degree asin:" + x);
            checkWeakEq(BoundedRational.degreeAcos(x),
                        fromRadians(ACOS.execute(xAsCR)), "degree acos:" + x);
        }
        checkWeakEq(BoundedRational.atan(x), fromRadians(ATAN.execute(xAsCR)),
                    "atan:" + x);
        checkWeakEq(BoundedRational.degreeAtan(x),
                    fromRadians(ATAN.execute(xAsCR)), "degree atan:" + x);
        if (x.signum() > 0) {
            checkWeakEq(BoundedRational.ln(x), xAsCR.ln(), "ln:" + x);
            checkWeakEq(BoundedRational.log(x), xAsCR.ln().divide(LN10),
                        "log:" + x);
            checkWeakEq(BoundedRational.sqrt(x), xAsCR.sqrt(), "sqrt:" + x);
            checkEq(BoundedRational.pow(x, BR_15),
                    xAsCR.ln().multiply(CR.valueOf(15)).exp(),
                    "pow(x,15):" + x);
        }
    }

    public void testBR() {
        BoundedRational b = new BoundedRational(4,-6);
        check(b.toString().equals("4/-6"), "toString(4/-6)");
        check(b.toNiceString().equals("-2/3"),"toNiceString(4/-6)");
        checkEq(BR_0, CR.valueOf(0), "0");
        checkEq(BR_390, CR.valueOf(390), "390");
        checkEq(BR_15, CR.valueOf(15), "15");
        checkEq(BR_M390, CR.valueOf(-390), "-390");
        checkEq(BR_M1, CR.valueOf(-1), "-1");
        checkEq(BR_2, CR.valueOf(2), "2");
        checkEq(BR_M2, CR.valueOf(-2), "-2");
        check(BR_0.signum() == 0, "signum(0)");
        check(BR_M1.signum() == -1, "signum(-1)");
        check(BR_2.signum() == 1, "signum(2)");
        check(BoundedRational.asBigInteger(BR_390).intValue() == 390, "390.asBigInteger()");
        check(BoundedRational.asBigInteger(BoundedRational.HALF) == null, "1/2.asBigInteger()");
        check(BoundedRational.asBigInteger(BoundedRational.MINUS_HALF) == null,
                "-1/2.asBigInteger()");
        check(BoundedRational.asBigInteger(new BoundedRational(15, -5)).intValue() == -3,
                "-15/5.asBigInteger()");
        check(BoundedRational.digitsRequired(BoundedRational.ZERO) == 0, "digitsRequired(0)");
        check(BoundedRational.digitsRequired(BoundedRational.HALF) == 1, "digitsRequired(1/2)");
        check(BoundedRational.digitsRequired(BoundedRational.MINUS_HALF) == 1,
                "digitsRequired(-1/2)");
        check(BoundedRational.digitsRequired(new BoundedRational(1,-2)) == 1,
                "digitsRequired(1/-2)");
        check(BoundedRational.fact(BoundedRational.ZERO).equals(BoundedRational.ONE), "0!");
        check(BoundedRational.fact(BoundedRational.ONE).equals(BoundedRational.ONE), "1!");
        check(BoundedRational.fact(BoundedRational.TWO).equals(BoundedRational.TWO), "2!");
        check(BoundedRational.fact(BR_15).equals(new BoundedRational(1307674368000L)), "15!");
        // We check values that include all interesting degree values.
        BoundedRational r = BR_M390;
        while (!r.equals(BR_390)) {
            check(r != null, "loop counter overflowed!");
            checkBR(r);
            r = BoundedRational.add(r, BR_15);
        }
        checkBR(BoundedRational.HALF);
        checkBR(BoundedRational.MINUS_HALF);
        checkBR(BoundedRational.ONE);
        checkBR(BoundedRational.MINUS_ONE);
        checkBR(new BoundedRational(1000));
        checkBR(new BoundedRational(100));
        checkBR(new BoundedRational(4,9));
        check(BoundedRational.sqrt(new BoundedRational(4,9)) != null,
              "sqrt(4/9) is null");
        checkBR(BoundedRational.negate(new BoundedRational(4,9)));
        checkBR(new BoundedRational(5,9));
        checkBR(new BoundedRational(5,10));
        checkBR(new BoundedRational(5,10));
        checkBR(new BoundedRational(4,13));
        checkBR(new BoundedRational(36));
        checkBR(BoundedRational.negate(new BoundedRational(36)));
        check(BoundedRational.pow(null, BR_15) == null, "pow(null, 15)");
    }

    public void testBRexceptions() {
        try {
            BoundedRational.ln(BR_M1);
            check(false, "ln(-1)");
        } catch (ArithmeticException ignored) {}
        try {
            BoundedRational.log(BR_M2);
            check(false, "log(-2)");
        } catch (ArithmeticException ignored) {}
        try {
            BoundedRational.sqrt(BR_M1);
            check(false, "sqrt(-1)");
        } catch (ArithmeticException ignored) {}
        try {
            BoundedRational.asin(BR_M2);
            check(false, "asin(-2)");
        } catch (ArithmeticException ignored) {}
        try {
            BoundedRational.degreeAcos(BR_2);
            check(false, "degree acos(2)");
        } catch (ArithmeticException ignored) {}
    }

    public void testBROverflow() {
        BoundedRational sum = new BoundedRational(0);
        long i;
        for (i = 1; i < 1000; ++i) {
             sum = BoundedRational.add(sum,
                        BoundedRational.inverse(new BoundedRational(i)));
             if (sum == null) break;
        }
        // Experimentally, this overflows at 139, which seems
        // plausible based on the Wolfram Alpha result.
        // This test is robust against minor changes in MAX_SIZE.
        check(i > 100, "Harmonic series overflowed at " + i);
        check(i < 1000, "Harmonic series didn't overflow");
    }
}
