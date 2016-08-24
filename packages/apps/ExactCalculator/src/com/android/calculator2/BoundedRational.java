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


import java.math.BigInteger;
import com.hp.creals.CR;

/**
 * Rational numbers that may turn to null if they get too big.
 * For many operations, if the length of the nuumerator plus the length of the denominator exceeds
 * a maximum size, we simply return null, and rely on our caller do something else.
 * We currently never return null for a pure integer or for a BoundedRational that has just been
 * constructed.
 *
 * We also implement a number of irrational functions.  These return a non-null result only when
 * the result is known to be rational.
 */
public class BoundedRational {
    // TODO: Consider returning null for integers.  With some care, large factorials might become
    // much faster.
    // TODO: Maybe eventually make this extend Number?

    private static final int MAX_SIZE = 800; // total, in bits

    private final BigInteger mNum;
    private final BigInteger mDen;

    public BoundedRational(BigInteger n, BigInteger d) {
        mNum = n;
        mDen = d;
    }

    public BoundedRational(BigInteger n) {
        mNum = n;
        mDen = BigInteger.ONE;
    }

    public BoundedRational(long n, long d) {
        mNum = BigInteger.valueOf(n);
        mDen = BigInteger.valueOf(d);
    }

    public BoundedRational(long n) {
        mNum = BigInteger.valueOf(n);
        mDen = BigInteger.valueOf(1);
    }

    /**
     * Convert to String reflecting raw representation.
     * Debug or log messages only, not pretty.
     */
    public String toString() {
        return mNum.toString() + "/" + mDen.toString();
    }

    /**
     * Convert to readable String.
     * Intended for output output to user.  More expensive, less useful for debugging than
     * toString().  Not internationalized.
     */
    public String toNiceString() {
        BoundedRational nicer = reduce().positiveDen();
        String result = nicer.mNum.toString();
        if (!nicer.mDen.equals(BigInteger.ONE)) {
            result += "/" + nicer.mDen;
        }
        return result;
    }

    public static String toString(BoundedRational r) {
        if (r == null) {
            return "not a small rational";
        }
        return r.toString();
    }

    /**
     * Return a double approximation.
     * Primarily for debugging.
     */
    public double doubleValue() {
        return mNum.doubleValue() / mDen.doubleValue();
    }

    public CR CRValue() {
        return CR.valueOf(mNum).divide(CR.valueOf(mDen));
    }

    // Approximate number of bits to left of binary point.
    public int wholeNumberBits() {
        return mNum.bitLength() - mDen.bitLength();
    }

    private boolean tooBig() {
        if (mDen.equals(BigInteger.ONE)) {
            return false;
        }
        return (mNum.bitLength() + mDen.bitLength() > MAX_SIZE);
    }

    /**
     * Return an equivalent fraction with a positive denominator.
     */
    private BoundedRational positiveDen() {
        if (mDen.signum() > 0) {
            return this;
        }
        return new BoundedRational(mNum.negate(), mDen.negate());
    }

    /**
     * Return an equivalent fraction in lowest terms.
     * Denominator sign may remain negative.
     */
    private BoundedRational reduce() {
        if (mDen.equals(BigInteger.ONE)) {
            return this;  // Optimization only
        }
        final BigInteger divisor = mNum.gcd(mDen);
        return new BoundedRational(mNum.divide(divisor), mDen.divide(divisor));
    }

    /**
     * Return a possibly reduced version of this that's not tooBig().
     * Return null if none exists.
     */
    private BoundedRational maybeReduce() {
        if (!tooBig()) {
            return this;
        }
        BoundedRational result = positiveDen();
        result = result.reduce();
        if (!result.tooBig()) {
            return this;
        }
        return null;
    }

    public int compareTo(BoundedRational r) {
        // Compare by multiplying both sides by denominators, invert result if denominator product
        // was negative.
        return mNum.multiply(r.mDen).compareTo(r.mNum.multiply(mDen)) * mDen.signum()
                * r.mDen.signum();
    }

    public int signum() {
        return mNum.signum() * mDen.signum();
    }

    public boolean equals(BoundedRational r) {
        return compareTo(r) == 0;
    }

    // We use static methods for arithmetic, so that we can easily handle the null case.  We try
    // to catch domain errors whenever possible, sometimes even when one of the arguments is null,
    // but not relevant.

    /**
     * Returns equivalent BigInteger result if it exists, null if not.
     */
    public static BigInteger asBigInteger(BoundedRational r) {
        if (r == null) {
            return null;
        }
        final BigInteger[] quotAndRem = r.mNum.divideAndRemainder(r.mDen);
        if (quotAndRem[1].signum() == 0) {
            return quotAndRem[0];
        } else {
            return null;
        }
    }
    public static BoundedRational add(BoundedRational r1, BoundedRational r2) {
        if (r1 == null || r2 == null) {
            return null;
        }
        final BigInteger den = r1.mDen.multiply(r2.mDen);
        final BigInteger num = r1.mNum.multiply(r2.mDen).add(r2.mNum.multiply(r1.mDen));
        return new BoundedRational(num,den).maybeReduce();
    }

    public static BoundedRational negate(BoundedRational r) {
        if (r == null) {
            return null;
        }
        return new BoundedRational(r.mNum.negate(), r.mDen);
    }

    static BoundedRational subtract(BoundedRational r1, BoundedRational r2) {
        return add(r1, negate(r2));
    }

    static BoundedRational multiply(BoundedRational r1, BoundedRational r2) {
        // It's tempting but marginally unsound to reduce 0 * null to 0.  The null could represent
        // an infinite value, for which we failed to throw an exception because it was too big.
        if (r1 == null || r2 == null) {
            return null;
        }
        final BigInteger num = r1.mNum.multiply(r2.mNum);
        final BigInteger den = r1.mDen.multiply(r2.mDen);
        return new BoundedRational(num,den).maybeReduce();
    }

    public static class ZeroDivisionException extends ArithmeticException {
        public ZeroDivisionException() {
            super("Division by zero");
        }
    }

    /**
     * Return the reciprocal of r (or null).
     */
    static BoundedRational inverse(BoundedRational r) {
        if (r == null) {
            return null;
        }
        if (r.mNum.signum() == 0) {
            throw new ZeroDivisionException();
        }
        return new BoundedRational(r.mDen, r.mNum);
    }

    static BoundedRational divide(BoundedRational r1, BoundedRational r2) {
        return multiply(r1, inverse(r2));
    }

    static BoundedRational sqrt(BoundedRational r) {
        // Return non-null if numerator and denominator are small perfect squares.
        if (r == null) {
            return null;
        }
        r = r.positiveDen().reduce();
        if (r.mNum.signum() < 0) {
            throw new ArithmeticException("sqrt(negative)");
        }
        final BigInteger num_sqrt = BigInteger.valueOf(Math.round(Math.sqrt(r.mNum.doubleValue())));
        if (!num_sqrt.multiply(num_sqrt).equals(r.mNum)) {
            return null;
        }
        final BigInteger den_sqrt = BigInteger.valueOf(Math.round(Math.sqrt(r.mDen.doubleValue())));
        if (!den_sqrt.multiply(den_sqrt).equals(r.mDen)) {
            return null;
        }
        return new BoundedRational(num_sqrt, den_sqrt);
    }

    public final static BoundedRational ZERO = new BoundedRational(0);
    public final static BoundedRational HALF = new BoundedRational(1,2);
    public final static BoundedRational MINUS_HALF = new BoundedRational(-1,2);
    public final static BoundedRational ONE = new BoundedRational(1);
    public final static BoundedRational MINUS_ONE = new BoundedRational(-1);
    public final static BoundedRational TWO = new BoundedRational(2);
    public final static BoundedRational MINUS_TWO = new BoundedRational(-2);
    public final static BoundedRational THIRTY = new BoundedRational(30);
    public final static BoundedRational MINUS_THIRTY = new BoundedRational(-30);
    public final static BoundedRational FORTY_FIVE = new BoundedRational(45);
    public final static BoundedRational MINUS_FORTY_FIVE = new BoundedRational(-45);
    public final static BoundedRational NINETY = new BoundedRational(90);
    public final static BoundedRational MINUS_NINETY = new BoundedRational(-90);

    private static BoundedRational map0to0(BoundedRational r) {
        if (r == null) {
            return null;
        }
        if (r.mNum.signum() == 0) {
            return ZERO;
        }
        return null;
    }

    private static BoundedRational map0to1(BoundedRational r) {
        if (r == null) {
            return null;
        }
        if (r.mNum.signum() == 0) {
            return ONE;
        }
        return null;
    }

    private static BoundedRational map1to0(BoundedRational r) {
        if (r == null) {
            return null;
        }
        if (r.mNum.equals(r.mDen)) {
            return ZERO;
        }
        return null;
    }

    // Throw an exception if the argument is definitely out of bounds for asin or acos.
    private static void checkAsinDomain(BoundedRational r) {
        if (r == null) {
            return;
        }
        if (r.mNum.abs().compareTo(r.mDen.abs()) > 0) {
            throw new ArithmeticException("inverse trig argument out of range");
        }
    }

    public static BoundedRational sin(BoundedRational r) {
        return map0to0(r);
    }

    private final static BigInteger BIG360 = BigInteger.valueOf(360);

    public static BoundedRational degreeSin(BoundedRational r) {
        final BigInteger r_BI = asBigInteger(r);
        if (r_BI == null) {
            return null;
        }
        final int r_int = r_BI.mod(BIG360).intValue();
        if (r_int % 30 != 0) {
            return null;
        }
        switch (r_int / 10) {
        case 0:
            return ZERO;
        case 3: // 30 degrees
            return HALF;
        case 9:
            return ONE;
        case 15:
            return HALF;
        case 18: // 180 degrees
            return ZERO;
        case 21:
            return MINUS_HALF;
        case 27:
            return MINUS_ONE;
        case 33:
            return MINUS_HALF;
        default:
            return null;
        }
    }

    public static BoundedRational asin(BoundedRational r) {
        checkAsinDomain(r);
        return map0to0(r);
    }

    public static BoundedRational degreeAsin(BoundedRational r) {
        checkAsinDomain(r);
        final BigInteger r2_BI = asBigInteger(multiply(r, TWO));
        if (r2_BI == null) {
            return null;
        }
        final int r2_int = r2_BI.intValue();
        // Somewhat surprisingly, it seems to be the case that the following covers all rational
        // cases:
        switch (r2_int) {
        case -2: // Corresponding to -1 argument
            return MINUS_NINETY;
        case -1: // Corresponding to -1/2 argument
            return MINUS_THIRTY;
        case 0:
            return ZERO;
        case 1:
            return THIRTY;
        case 2:
            return NINETY;
        default:
            throw new AssertionError("Impossible asin arg");
        }
    }

    public static BoundedRational tan(BoundedRational r) {
        // Unlike the degree case, we cannot check for the singularity, since it occurs at an
        // irrational argument.
        return map0to0(r);
    }

    public static BoundedRational degreeTan(BoundedRational r) {
        final BoundedRational degSin = degreeSin(r);
        final BoundedRational degCos = degreeCos(r);
        if (degCos != null && degCos.mNum.signum() == 0) {
            throw new ArithmeticException("Tangent undefined");
        }
        return divide(degSin, degCos);
    }

    public static BoundedRational atan(BoundedRational r) {
        return map0to0(r);
    }

    public static BoundedRational degreeAtan(BoundedRational r) {
        final BigInteger r_BI = asBigInteger(r);
        if (r_BI == null) {
            return null;
        }
        if (r_BI.abs().compareTo(BigInteger.ONE) > 0) {
            return null;
        }
        final int r_int = r_BI.intValue();
        // Again, these seem to be all rational cases:
        switch (r_int) {
        case -1:
            return MINUS_FORTY_FIVE;
        case 0:
            return ZERO;
        case 1:
            return FORTY_FIVE;
        default:
            throw new AssertionError("Impossible atan arg");
        }
    }

    public static BoundedRational cos(BoundedRational r) {
        return map0to1(r);
    }

    public static BoundedRational degreeCos(BoundedRational r) {
        return degreeSin(add(r, NINETY));
    }

    public static BoundedRational acos(BoundedRational r) {
        checkAsinDomain(r);
        return map1to0(r);
    }

    public static BoundedRational degreeAcos(BoundedRational r) {
        final BoundedRational asin_r = degreeAsin(r);
        return subtract(NINETY, asin_r);
    }

    private static final BigInteger BIG_TWO = BigInteger.valueOf(2);

    /**
     * Compute an integral power of this.
     */
    private BoundedRational pow(BigInteger exp) {
        if (exp.signum() < 0) {
            return inverse(pow(exp.negate()));
        }
        if (exp.equals(BigInteger.ONE)) {
            return this;
        }
        if (exp.and(BigInteger.ONE).intValue() == 1) {
            return multiply(pow(exp.subtract(BigInteger.ONE)), this);
        }
        if (exp.signum() == 0) {
            return ONE;
        }
        BoundedRational tmp = pow(exp.shiftRight(1));
        if (Thread.interrupted()) {
            throw new CR.AbortedException();
        }
        return multiply(tmp, tmp);
    }

    public static BoundedRational pow(BoundedRational base, BoundedRational exp) {
        if (exp == null) {
            return null;
        }
        if (exp.mNum.signum() == 0) {
            // Questionable if base has undefined value.  Java.lang.Math.pow() returns 1 anyway,
            // so we do the same.
            return new BoundedRational(1);
        }
        if (base == null) {
            return null;
        }
        exp = exp.reduce().positiveDen();
        if (!exp.mDen.equals(BigInteger.ONE)) {
            return null;
        }
        return base.pow(exp.mNum);
    }

    public static BoundedRational ln(BoundedRational r) {
        if (r != null && r.signum() <= 0) {
            throw new ArithmeticException("log(non-positive)");
        }
        return map1to0(r);
    }

    public static BoundedRational exp(BoundedRational r) {
        return map0to1(r);
    }

    /**
     * Return the base 10 log of n, if n is a power of 10, -1 otherwise.
     * n must be positive.
     */
    private static long b10Log(BigInteger n) {
        // This algorithm is very naive, but we doubt it matters.
        long count = 0;
        while (n.mod(BigInteger.TEN).signum() == 0) {
            if (Thread.interrupted()) {
                throw new CR.AbortedException();
            }
            n = n.divide(BigInteger.TEN);
            ++count;
        }
        if (n.equals(BigInteger.ONE)) {
            return count;
        }
        return -1;
    }

    public static BoundedRational log(BoundedRational r) {
        if (r == null) {
            return null;
        }
        if (r.signum() <= 0) {
            throw new ArithmeticException("log(non-positive)");
        }
        r = r.reduce().positiveDen();
        if (r == null) {
            return null;
        }
        if (r.mDen.equals(BigInteger.ONE)) {
            long log = b10Log(r.mNum);
            if (log != -1) {
                return new BoundedRational(log);
            }
        } else if (r.mNum.equals(BigInteger.ONE)) {
            long log = b10Log(r.mDen);
            if (log != -1) {
                return new BoundedRational(-log);
            }
        }
        return null;
    }

    /**
     * Generalized factorial.
     * Compute n * (n - step) * (n - 2 * step) * etc.  This can be used to compute factorial a bit
     * faster, especially if BigInteger uses sub-quadratic multiplication.
     */
    private static BigInteger genFactorial(long n, long step) {
        if (n > 4 * step) {
            BigInteger prod1 = genFactorial(n, 2 * step);
            if (Thread.interrupted()) {
                throw new CR.AbortedException();
            }
            BigInteger prod2 = genFactorial(n - step, 2 * step);
            if (Thread.interrupted()) {
                throw new CR.AbortedException();
            }
            return prod1.multiply(prod2);
        } else {
            if (n == 0) {
                return BigInteger.ONE;
            }
            BigInteger res = BigInteger.valueOf(n);
            for (long i = n - step; i > 1; i -= step) {
                res = res.multiply(BigInteger.valueOf(i));
            }
            return res;
        }
    }

    /**
     * Factorial function.
     * Always produces non-null (or exception) when called on non-null r.
     */
    public static BoundedRational fact(BoundedRational r) {
        if (r == null) {
            return null;
        }
        final BigInteger rAsInt = asBigInteger(r);
        if (rAsInt == null) {
            throw new ArithmeticException("Non-integral factorial argument");
        }
        if (rAsInt.signum() < 0) {
            throw new ArithmeticException("Negative factorial argument");
        }
        if (rAsInt.bitLength() > 30) {
            // Will fail.  LongValue() may not work. Punt now.
            throw new ArithmeticException("Factorial argument too big");
        }
        return new BoundedRational(genFactorial(rAsInt.longValue(), 1));
    }

    private static final BigInteger BIG_FIVE = BigInteger.valueOf(5);
    private static final BigInteger BIG_MINUS_ONE = BigInteger.valueOf(-1);

    /**
     * Return the number of decimal digits to the right of the decimal point required to represent
     * the argument exactly.
     * Return Integer.MAX_VALUE if that's not possible.  Never returns a value less than zero, even
     * if r is a power of ten.
     */
    static int digitsRequired(BoundedRational r) {
        if (r == null) {
            return Integer.MAX_VALUE;
        }
        int powersOfTwo = 0;  // Max power of 2 that divides denominator
        int powersOfFive = 0;  // Max power of 5 that divides denominator
        // Try the easy case first to speed things up.
        if (r.mDen.equals(BigInteger.ONE)) {
            return 0;
        }
        r = r.reduce();
        BigInteger den = r.mDen;
        if (den.bitLength() > MAX_SIZE) {
            return Integer.MAX_VALUE;
        }
        while (!den.testBit(0)) {
            ++powersOfTwo;
            den = den.shiftRight(1);
        }
        while (den.mod(BIG_FIVE).signum() == 0) {
            ++powersOfFive;
            den = den.divide(BIG_FIVE);
        }
        // If the denominator has a factor of other than 2 or 5 (the divisors of 10), the decimal
        // expansion does not terminate.  Multiplying the fraction by any number of powers of 10
        // will not cancel the demoniator.  (Recall the fraction was in lowest terms to start
        // with.) Otherwise the powers of 10 we need to cancel the denominator is the larger of
        // powersOfTwo and powersOfFive.
        if (!den.equals(BigInteger.ONE) && !den.equals(BIG_MINUS_ONE)) {
            return Integer.MAX_VALUE;
        }
        return Math.max(powersOfTwo, powersOfFive);
    }
}
