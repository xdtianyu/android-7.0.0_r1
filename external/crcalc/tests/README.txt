Run on Android with

1) Build the tests.
2) adb install <tree root>/out/target/product/generic/data/app/CRTests/CRTests.apk
3) adb shell am instrument -w com.hp.creals.tests/android.test.InstrumentationTestRunner

The last step takes around 10 minutes on a Nexus 5.
(CRTest is quick, SlowCRTest is not, especially not the final trig function
test.)

Note that Random seeds are not set.  Hence repreated runs should improve
coverage at the cost of reproducibility.  Failing arguments should however
be printed.

We expect that this test is much too nondeterministic to be usable for any kind
of performance evaluation.  Please don't try.
