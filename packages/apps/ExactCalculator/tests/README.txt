Run on Android with

1) Build the tests.
2) Install the calculator with
adb install <tree root>/out/target/product/generic/data/app/ExactCalculator/ExactCalculator.apk
3) adb install <tree root>/out/target/product/generic/data/app/ExactCalculatorTests/ExactCalculatorTests.apk
4) adb shell am instrument -w com.android.calculator2.tests/android.test.InstrumentationTestRunner

There are three kinds of tests:

1. A superficial test of calculator functionality through the UI.
This is a resurrected version of a test that appeared in KitKat.
This is currently only a placeholder for regression tests we shouldn't
forget; it doesn't yet actually do much of anything.

2. A test of the BoundedRationals library that mostly checks for agreement
with the constructive reals (CR) package.  (The BoundedRationals package
is used by the calculator mostly to identify exact results, i.e.
terminating decimal expansions.  But it's also used to optimize CR
computations, and bugs in BoundedRational could result in incorrect
outputs.)

3. A quick test of Evaluator.testUnflipZeroes(), which we do not know how to
test manually.

We currently have no automatic tests for display formatting corner cases.
The following numbers have exhibited problems in the past and would be good
to test.  Some of them are difficult to test automatically, because they
require scrolling to both ends of the result.  For those with finite
decimal expansions, it also worth confirming that the "display with leading
digits" display shows an exact value when scrolled all the way to the right.

Some interesting manual test cases:

10^10 + 10^30
10^30 + 10^-10
-10^30 + 20
10^30 + 10^-30
-10^30 - 10^10
-1.2x10^-9
-1.2x10^-8
-1.2x10^-10
-10^-12
1 - 10^-98
1 - 10^-100
1 - 10^-300
1/-56x10^18 (on a Nexus 7 sized portrait display)
-10^-500 (scroll to see the 1, then scroll back & verify minus sign appears)
