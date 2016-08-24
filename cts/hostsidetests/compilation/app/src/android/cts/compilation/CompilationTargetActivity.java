/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.cts.compilation;

import android.app.Activity;
import android.os.AsyncTask;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Random;

/**
 * A simple activity which can be subjected to (dex to native) compilation.
 *
 * If you change this code, you need to regenerate APK and profile using the
 * the new code - see instructions in {@code assets/README.txt}.
 */
public class CompilationTargetActivity extends Activity {

    private AsyncTask<Integer, String, Void> mTask;

    @Override
    protected void onResume() {
        super.onResume();
        setTitle("Starting...");
        mTask = new AsyncTask<Integer, String, Void>() {
            @Override
            protected Void doInBackground(Integer... params) {
                int numValues = params[0];
                int numIter = params[1];
                for (int i = 0; i < numIter; i++) {
                    if (Thread.interrupted()) {
                        break;
                    }
                    publishProgress("Step " + (i+1) + " of " + numIter);
                    List<Integer> values = makeValues(numValues);
                    Collections.shuffle(values);
                    Collections.sort(values);
                }
                return null;
            }

            @Override
            protected void onProgressUpdate(String... values) {
                setTitle(values[0]);
            }

            @Override
            protected void onPostExecute(Void aVoid) {
                setTitle("Done");
            }
        };
        mTask.execute(1024, 100 * 1000);
    }

    @Override
    protected void onPause() {
        mTask.cancel(/* mayInterruptIfRunning */ true);
        mTask = null;
        super.onPause();
    }

    private List<Integer> makeValues(int numValues) {
        List<Integer> result = new ArrayList<>();
        Random random = new Random();
        for (int i = 0; i < numValues; i++) {
            int v = dispatch(random.nextInt());
            result.add(v);
        }
        return result;
    }

    /**
     * Dispatches to a bunch of simple methods because JIT profiles are only generated for
     * apps with enough methods (10, as of May 2016).
     */
    private int dispatch(int i) {
        int v = Math.abs(i % 100);
        switch (v) {
            case 0: return m0();
            case 1: return m1();
            case 2: return m2();
            case 3: return m3();
            case 4: return m4();
            case 5: return m5();
            case 6: return m6();
            case 7: return m7();
            case 8: return m8();
            case 9: return m9();
            case 10: return m10();
            case 11: return m11();
            case 12: return m12();
            case 13: return m13();
            case 14: return m14();
            case 15: return m15();
            case 16: return m16();
            case 17: return m17();
            case 18: return m18();
            case 19: return m19();
            case 20: return m20();
            case 21: return m21();
            case 22: return m22();
            case 23: return m23();
            case 24: return m24();
            case 25: return m25();
            case 26: return m26();
            case 27: return m27();
            case 28: return m28();
            case 29: return m29();
            case 30: return m30();
            case 31: return m31();
            case 32: return m32();
            case 33: return m33();
            case 34: return m34();
            case 35: return m35();
            case 36: return m36();
            case 37: return m37();
            case 38: return m38();
            case 39: return m39();
            case 40: return m40();
            case 41: return m41();
            case 42: return m42();
            case 43: return m43();
            case 44: return m44();
            case 45: return m45();
            case 46: return m46();
            case 47: return m47();
            case 48: return m48();
            case 49: return m49();
            case 50: return m50();
            case 51: return m51();
            case 52: return m52();
            case 53: return m53();
            case 54: return m54();
            case 55: return m55();
            case 56: return m56();
            case 57: return m57();
            case 58: return m58();
            case 59: return m59();
            case 60: return m60();
            case 61: return m61();
            case 62: return m62();
            case 63: return m63();
            case 64: return m64();
            case 65: return m65();
            case 66: return m66();
            case 67: return m67();
            case 68: return m68();
            case 69: return m69();
            case 70: return m70();
            case 71: return m71();
            case 72: return m72();
            case 73: return m73();
            case 74: return m74();
            case 75: return m75();
            case 76: return m76();
            case 77: return m77();
            case 78: return m78();
            case 79: return m79();
            case 80: return m80();
            case 81: return m81();
            case 82: return m82();
            case 83: return m83();
            case 84: return m84();
            case 85: return m85();
            case 86: return m86();
            case 87: return m87();
            case 88: return m88();
            case 89: return m89();
            case 90: return m90();
            case 91: return m91();
            case 92: return m92();
            case 93: return m93();
            case 94: return m94();
            case 95: return m95();
            case 96: return m96();
            case 97: return m97();
            case 98: return m98();
            case 99: return m99();
            default: throw new AssertionError(v + " out of bounds");
        }
    }

    public int m0() { return new Random(0).nextInt(); }
    public int m1() { return new Random(1).nextInt(); }
    public int m2() { return new Random(2).nextInt(); }
    public int m3() { return new Random(3).nextInt(); }
    public int m4() { return new Random(4).nextInt(); }
    public int m5() { return new Random(5).nextInt(); }
    public int m6() { return new Random(6).nextInt(); }
    public int m7() { return new Random(7).nextInt(); }
    public int m8() { return new Random(8).nextInt(); }
    public int m9() { return new Random(9).nextInt(); }
    public int m10() { return new Random(10).nextInt(); }
    public int m11() { return new Random(11).nextInt(); }
    public int m12() { return new Random(12).nextInt(); }
    public int m13() { return new Random(13).nextInt(); }
    public int m14() { return new Random(14).nextInt(); }
    public int m15() { return new Random(15).nextInt(); }
    public int m16() { return new Random(16).nextInt(); }
    public int m17() { return new Random(17).nextInt(); }
    public int m18() { return new Random(18).nextInt(); }
    public int m19() { return new Random(19).nextInt(); }
    public int m20() { return new Random(20).nextInt(); }
    public int m21() { return new Random(21).nextInt(); }
    public int m22() { return new Random(22).nextInt(); }
    public int m23() { return new Random(23).nextInt(); }
    public int m24() { return new Random(24).nextInt(); }
    public int m25() { return new Random(25).nextInt(); }
    public int m26() { return new Random(26).nextInt(); }
    public int m27() { return new Random(27).nextInt(); }
    public int m28() { return new Random(28).nextInt(); }
    public int m29() { return new Random(29).nextInt(); }
    public int m30() { return new Random(30).nextInt(); }
    public int m31() { return new Random(31).nextInt(); }
    public int m32() { return new Random(32).nextInt(); }
    public int m33() { return new Random(33).nextInt(); }
    public int m34() { return new Random(34).nextInt(); }
    public int m35() { return new Random(35).nextInt(); }
    public int m36() { return new Random(36).nextInt(); }
    public int m37() { return new Random(37).nextInt(); }
    public int m38() { return new Random(38).nextInt(); }
    public int m39() { return new Random(39).nextInt(); }
    public int m40() { return new Random(40).nextInt(); }
    public int m41() { return new Random(41).nextInt(); }
    public int m42() { return new Random(42).nextInt(); }
    public int m43() { return new Random(43).nextInt(); }
    public int m44() { return new Random(44).nextInt(); }
    public int m45() { return new Random(45).nextInt(); }
    public int m46() { return new Random(46).nextInt(); }
    public int m47() { return new Random(47).nextInt(); }
    public int m48() { return new Random(48).nextInt(); }
    public int m49() { return new Random(49).nextInt(); }
    public int m50() { return new Random(50).nextInt(); }
    public int m51() { return new Random(51).nextInt(); }
    public int m52() { return new Random(52).nextInt(); }
    public int m53() { return new Random(53).nextInt(); }
    public int m54() { return new Random(54).nextInt(); }
    public int m55() { return new Random(55).nextInt(); }
    public int m56() { return new Random(56).nextInt(); }
    public int m57() { return new Random(57).nextInt(); }
    public int m58() { return new Random(58).nextInt(); }
    public int m59() { return new Random(59).nextInt(); }
    public int m60() { return new Random(60).nextInt(); }
    public int m61() { return new Random(61).nextInt(); }
    public int m62() { return new Random(62).nextInt(); }
    public int m63() { return new Random(63).nextInt(); }
    public int m64() { return new Random(64).nextInt(); }
    public int m65() { return new Random(65).nextInt(); }
    public int m66() { return new Random(66).nextInt(); }
    public int m67() { return new Random(67).nextInt(); }
    public int m68() { return new Random(68).nextInt(); }
    public int m69() { return new Random(69).nextInt(); }
    public int m70() { return new Random(70).nextInt(); }
    public int m71() { return new Random(71).nextInt(); }
    public int m72() { return new Random(72).nextInt(); }
    public int m73() { return new Random(73).nextInt(); }
    public int m74() { return new Random(74).nextInt(); }
    public int m75() { return new Random(75).nextInt(); }
    public int m76() { return new Random(76).nextInt(); }
    public int m77() { return new Random(77).nextInt(); }
    public int m78() { return new Random(78).nextInt(); }
    public int m79() { return new Random(79).nextInt(); }
    public int m80() { return new Random(80).nextInt(); }
    public int m81() { return new Random(81).nextInt(); }
    public int m82() { return new Random(82).nextInt(); }
    public int m83() { return new Random(83).nextInt(); }
    public int m84() { return new Random(84).nextInt(); }
    public int m85() { return new Random(85).nextInt(); }
    public int m86() { return new Random(86).nextInt(); }
    public int m87() { return new Random(87).nextInt(); }
    public int m88() { return new Random(88).nextInt(); }
    public int m89() { return new Random(89).nextInt(); }
    public int m90() { return new Random(90).nextInt(); }
    public int m91() { return new Random(91).nextInt(); }
    public int m92() { return new Random(92).nextInt(); }
    public int m93() { return new Random(93).nextInt(); }
    public int m94() { return new Random(94).nextInt(); }
    public int m95() { return new Random(95).nextInt(); }
    public int m96() { return new Random(96).nextInt(); }
    public int m97() { return new Random(97).nextInt(); }
    public int m98() { return new Random(98).nextInt(); }
    public int m99() { return new Random(99).nextInt(); }

}
