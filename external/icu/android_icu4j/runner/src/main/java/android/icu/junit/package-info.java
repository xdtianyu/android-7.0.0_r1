/**
 * Contains classes used to make ICU tests runnable by JUnit.
 *
 * <p>Ideally, this will be integrated into ICU itself and cleaned up so that ICU tests can be
 * simply run using standard JUnit. e.g. ICU test classes will be annotated with
 * {@code RunWith(IcuTestGroupRunner.class)} or {@code RunWith(IcuTestFmwkRunner.class)} depending
 * on whether they extend {@link android.icu.dev.test.TestFmwk.TestGroup} or
 * {@link android.icu.dev.test.TestFmwk}.
 */
package android.icu.junit;
