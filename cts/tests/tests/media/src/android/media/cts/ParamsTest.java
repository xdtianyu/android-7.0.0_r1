/*
 * Copyright 2015 The Android Open Source Project
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

package android.media.cts;

import android.media.cts.R;

import android.media.PlaybackParams;
import android.media.SyncParams;
import android.os.Parcel;
import android.test.AndroidTestCase;

/**
 * General Params tests.
 *
 * In particular, check Params objects' behavior.
 */
public class ParamsTest extends AndroidTestCase {
    private static final String TAG = "ParamsTest";
    private static final float FLOAT_TOLERANCE = .00001f;
    private static final float MAX_DEFAULT_TOLERANCE = 1/24.f;

    public void testSyncParamsConstants() {
        assertEquals(0, SyncParams.SYNC_SOURCE_DEFAULT);
        assertEquals(1, SyncParams.SYNC_SOURCE_SYSTEM_CLOCK);
        assertEquals(2, SyncParams.SYNC_SOURCE_AUDIO);
        assertEquals(3, SyncParams.SYNC_SOURCE_VSYNC);

        assertEquals(0, SyncParams.AUDIO_ADJUST_MODE_DEFAULT);
        assertEquals(1, SyncParams.AUDIO_ADJUST_MODE_STRETCH);
        assertEquals(2, SyncParams.AUDIO_ADJUST_MODE_RESAMPLE);
    }

    public void testSyncParamsDefaults() {
        SyncParams p = new SyncParams();
        try { fail("got " + p.getAudioAdjustMode()); } catch (IllegalStateException e) {}
        try { fail("got " + p.getSyncSource());      } catch (IllegalStateException e) {}
        try { fail("got " + p.getTolerance());       } catch (IllegalStateException e) {}
        try { fail("got " + p.getFrameRate());       } catch (IllegalStateException e) {}

        SyncParams q = p.allowDefaults();
        assertSame(p, q);
        assertEquals(p.AUDIO_ADJUST_MODE_DEFAULT, p.getAudioAdjustMode());
        assertEquals(p.SYNC_SOURCE_DEFAULT,       p.getSyncSource());
        assertTrue(p.getTolerance() >= 0.f
                && p.getTolerance() < MAX_DEFAULT_TOLERANCE + FLOAT_TOLERANCE);
        try { fail("got " + p.getFrameRate());       } catch (IllegalStateException e) {}
    }

    public void testSyncParamsAudioAdjustMode() {
        // setting this cannot fail
        SyncParams p = new SyncParams();
        for (int i : new int[] {
                SyncParams.AUDIO_ADJUST_MODE_STRETCH,
                SyncParams.AUDIO_ADJUST_MODE_RESAMPLE,
                -1 /* invalid */}) {
            SyncParams q = p.setAudioAdjustMode(i); // verify both initial set and update
            assertSame(p, q);
            assertEquals(i, p.getAudioAdjustMode());
            try { fail("got " + p.getSyncSource());      } catch (IllegalStateException e) {}
            try { fail("got " + p.getTolerance());       } catch (IllegalStateException e) {}
            try { fail("got " + p.getFrameRate());       } catch (IllegalStateException e) {}
        }
    }

    public void testSyncParamsSyncSource() {
        // setting this cannot fail
        SyncParams p = new SyncParams();
        for (int i : new int[] {
                SyncParams.SYNC_SOURCE_SYSTEM_CLOCK,
                SyncParams.SYNC_SOURCE_AUDIO,
                -1 /* invalid */}) {
            SyncParams q = p.setSyncSource(i); // verify both initial set and update
            assertSame(p, q);
            try { fail("got " + p.getAudioAdjustMode()); } catch (IllegalStateException e) {}
            assertEquals(i, p.getSyncSource());
            try { fail("got " + p.getTolerance());       } catch (IllegalStateException e) {}
            try { fail("got " + p.getFrameRate());       } catch (IllegalStateException e) {}
        }
    }

    public void testSyncParamsTolerance() {
        // this can fail on values not in [0, 1)

        // test good values
        SyncParams p = new SyncParams();
        float lastValue = 2.f; /* some initial value to avoid compile error */
        for (float f : new float[] { 0.f, .1f, .9999f }) {
            SyncParams q = p.setTolerance(f); // verify both initial set and update
            assertSame(p, q);
            try { fail("got " + p.getAudioAdjustMode()); } catch (IllegalStateException e) {}
            try { fail("got " + p.getSyncSource());      } catch (IllegalStateException e) {}
            assertEquals(f, p.getTolerance(), FLOAT_TOLERANCE);
            try { fail("got " + p.getFrameRate());       } catch (IllegalStateException e) {}
            lastValue = f;
        }

        // test bad values - these should have no effect
        boolean update = true;
        for (float f : new float[] { -.0001f, 1.f }) {
            try {
                p.setTolerance(f);
                fail("set tolerance to " + f);
            } catch (IllegalArgumentException e) {}
            try { fail("got " + p.getAudioAdjustMode()); } catch (IllegalStateException e) {}
            try { fail("got " + p.getSyncSource());      } catch (IllegalStateException e) {}
            if (update) {
                // if updating, last value should remain
                assertEquals(lastValue, p.getTolerance(), FLOAT_TOLERANCE);
            } else {
                // otherwise, it should remain undefined
                try { fail("got " + p.getTolerance());       } catch (IllegalStateException e) {}
            }
            try { fail("got " + p.getFrameRate());       } catch (IllegalStateException e) {}

            // no longer updating in subsequent iterations
            p = new SyncParams();
            update = false;
        }
    }

    public void testSyncParamsFrameRate() {
        // setting this cannot fail, but negative values may be normalized to some negative value
        SyncParams p = new SyncParams();
        for (float f : new float[] { 0.f, .0001f, 30.f, 300.f, -.0001f, -1.f }) {
            SyncParams q = p.setFrameRate(f);
            assertSame(p, q);
            try { fail("got " + p.getAudioAdjustMode()); } catch (IllegalStateException e) {}
            try { fail("got " + p.getSyncSource());      } catch (IllegalStateException e) {}
            try { fail("got " + p.getTolerance());       } catch (IllegalStateException e) {}
            if (f >= 0) {
                assertEquals(f, p.getFrameRate(), FLOAT_TOLERANCE);
            } else {
                assertTrue(p.getFrameRate() < 0.f);
            }
        }
    }

    public void testSyncParamsMultipleSettings() {
        {
            SyncParams p = new SyncParams();
            p.setAudioAdjustMode(p.AUDIO_ADJUST_MODE_STRETCH);
            SyncParams q = p.setTolerance(.5f);
            assertSame(p, q);

            assertEquals(p.AUDIO_ADJUST_MODE_STRETCH, p.getAudioAdjustMode());
            try { fail("got " + p.getSyncSource());      } catch (IllegalStateException e) {}
            assertEquals(.5f, p.getTolerance(), FLOAT_TOLERANCE);
            try { fail("got " + p.getFrameRate());       } catch (IllegalStateException e) {}

            // allowDefaults should not change set values
            q = p.allowDefaults();
            assertSame(p, q);

            assertEquals(p.AUDIO_ADJUST_MODE_STRETCH, p.getAudioAdjustMode());
            assertEquals(p.SYNC_SOURCE_DEFAULT, p.getSyncSource());
            assertEquals(.5f, p.getTolerance(), FLOAT_TOLERANCE);
            try { fail("got " + p.getFrameRate());       } catch (IllegalStateException e) {}
        }

        {
            SyncParams p = new SyncParams();
            p.setSyncSource(p.SYNC_SOURCE_VSYNC);
            SyncParams q = p.setFrameRate(25.f);
            assertSame(p, q);

            try { fail("got " + p.getAudioAdjustMode()); } catch (IllegalStateException e) {}
            assertEquals(p.SYNC_SOURCE_VSYNC, p.getSyncSource());
            try { fail("got " + p.getTolerance());       } catch (IllegalStateException e) {}
            assertEquals(25.f, p.getFrameRate(), FLOAT_TOLERANCE);

            // allowDefaults should not change set values
            q = p.allowDefaults();
            assertSame(p, q);

            assertEquals(p.AUDIO_ADJUST_MODE_DEFAULT, p.getAudioAdjustMode());
            assertEquals(p.SYNC_SOURCE_VSYNC, p.getSyncSource());
            assertTrue(p.getTolerance() >= 0.f
                    && p.getTolerance() < MAX_DEFAULT_TOLERANCE + FLOAT_TOLERANCE);
            assertEquals(25.f, p.getFrameRate(), FLOAT_TOLERANCE);
        }
    }

    public void testPlaybackParamsConstants() {
        assertEquals(0, PlaybackParams.AUDIO_STRETCH_MODE_DEFAULT);
        assertEquals(1, PlaybackParams.AUDIO_STRETCH_MODE_VOICE);

        assertEquals(0, PlaybackParams.AUDIO_FALLBACK_MODE_DEFAULT);
        assertEquals(1, PlaybackParams.AUDIO_FALLBACK_MODE_MUTE);
        assertEquals(2, PlaybackParams.AUDIO_FALLBACK_MODE_FAIL);
    }

    public void testPlaybackParamsDefaults() {
        PlaybackParams p = new PlaybackParams();
        try { fail("got " + p.getAudioFallbackMode()); } catch (IllegalStateException e) {}
        try { fail("got " + p.getAudioStretchMode());  } catch (IllegalStateException e) {}
        try { fail("got " + p.getPitch());             } catch (IllegalStateException e) {}
        try { fail("got " + p.getSpeed());             } catch (IllegalStateException e) {}

        PlaybackParams q = p.allowDefaults();
        assertSame(p, q);
        assertEquals(p.AUDIO_FALLBACK_MODE_DEFAULT, p.getAudioFallbackMode());
        assertEquals(p.AUDIO_STRETCH_MODE_DEFAULT,  p.getAudioStretchMode());
        assertEquals(1.f, p.getPitch(), FLOAT_TOLERANCE);
        assertEquals(1.f, p.getSpeed(), FLOAT_TOLERANCE);
    }

    public void testPlaybackParamsAudioFallbackMode() {
        // setting this cannot fail
        PlaybackParams p = new PlaybackParams();
        for (int i : new int[] {
                PlaybackParams.AUDIO_FALLBACK_MODE_MUTE,
                PlaybackParams.AUDIO_FALLBACK_MODE_FAIL,
                -1 /* invalid */}) {
            PlaybackParams q = p.setAudioFallbackMode(i); // verify both initial set and update
            assertSame(p, q);
            assertEquals(i, p.getAudioFallbackMode());
            try { fail("got " + p.getAudioStretchMode());  } catch (IllegalStateException e) {}
            try { fail("got " + p.getPitch());             } catch (IllegalStateException e) {}
            try { fail("got " + p.getSpeed());             } catch (IllegalStateException e) {}
        }
    }

    public void testPlaybackParamsAudioStretchMode() {
        // setting this cannot fail
        PlaybackParams p = new PlaybackParams();
        for (int i : new int[] {
                PlaybackParams.AUDIO_STRETCH_MODE_DEFAULT,
                PlaybackParams.AUDIO_STRETCH_MODE_VOICE,
                -1 /* invalid */}) {
            PlaybackParams q = p.setAudioStretchMode(i); // verify both initial set and update
            assertSame(p, q);
            try { fail("got " + p.getAudioFallbackMode()); } catch (IllegalStateException e) {}
            assertEquals(i, p.getAudioStretchMode());
            try { fail("got " + p.getPitch());             } catch (IllegalStateException e) {}
            try { fail("got " + p.getSpeed());             } catch (IllegalStateException e) {}
        }
    }

    public void testPlaybackParamsPitch() {
        // this can fail on values not in [0, Inf)

        // test good values
        PlaybackParams p = new PlaybackParams();
        float lastValue = 2.f; /* some initial value to avoid compile error */
        for (float f : new float[] { 0.f, .1f, 9999.f }) {
            PlaybackParams q = p.setPitch(f); // verify both initial set and update
            assertSame(p, q);
            try { fail("got " + p.getAudioFallbackMode()); } catch (IllegalStateException e) {}
            try { fail("got " + p.getAudioStretchMode());  } catch (IllegalStateException e) {}
            assertEquals(f, p.getPitch(), FLOAT_TOLERANCE);
            try { fail("got " + p.getSpeed());             } catch (IllegalStateException e) {}
            lastValue = f;
        }

        // test bad values - these should have no effect
        boolean update = true;
        for (float f : new float[] { -.0001f, -1.f }) {
            try {
                p.setPitch(f);
                fail("set tolerance to " + f);
            } catch (IllegalArgumentException e) {}
            try { fail("got " + p.getAudioFallbackMode()); } catch (IllegalStateException e) {}
            try { fail("got " + p.getAudioStretchMode());  } catch (IllegalStateException e) {}
            if (update) {
                // if updating, last value should remain
                assertEquals(lastValue, p.getPitch(), FLOAT_TOLERANCE);
            } else {
                // otherwise, it should remain undefined
                try { fail("got " + p.getPitch());             } catch (IllegalStateException e) {}
            }
            try { fail("got " + p.getSpeed());             } catch (IllegalStateException e) {}

            // no longer updating in subsequent iterations
            p = new PlaybackParams();
            update = false;
        }
    }

    public void testPlaybackParamsSpeed() {
        // setting this cannot fail
        PlaybackParams p = new PlaybackParams();
        for (float f : new float[] { 0.f, .0001f, 30.f, 300.f, -.0001f, -1.f, -300.f }) {
            PlaybackParams q = p.setSpeed(f);
            assertSame(p, q);
            try { fail("got " + p.getAudioFallbackMode()); } catch (IllegalStateException e) {}
            try { fail("got " + p.getAudioStretchMode());  } catch (IllegalStateException e) {}
            try { fail("got " + p.getPitch());             } catch (IllegalStateException e) {}
            assertEquals(f, p.getSpeed(), FLOAT_TOLERANCE);
        }
    }

    public void testPlaybackParamsMultipleSettings() {
        {
            PlaybackParams p = new PlaybackParams();
            p.setAudioFallbackMode(p.AUDIO_FALLBACK_MODE_MUTE);
            PlaybackParams q = p.setPitch(.5f);
            assertSame(p, q);

            assertEquals(p.AUDIO_FALLBACK_MODE_MUTE, p.getAudioFallbackMode());
            try { fail("got " + p.getAudioStretchMode());  } catch (IllegalStateException e) {}
            assertEquals(.5f, p.getPitch(), FLOAT_TOLERANCE);
            try { fail("got " + p.getSpeed());             } catch (IllegalStateException e) {}

            // allowDefaults should not change set values
            q = p.allowDefaults();
            assertSame(p, q);

            assertEquals(p.AUDIO_FALLBACK_MODE_MUTE, p.getAudioFallbackMode());
            assertEquals(p.AUDIO_STRETCH_MODE_DEFAULT, p.getAudioStretchMode());
            assertEquals(.5f, p.getPitch(), FLOAT_TOLERANCE);
            assertEquals(1.f, p.getSpeed(), FLOAT_TOLERANCE);
        }

        {
            PlaybackParams p = new PlaybackParams();
            p.setAudioStretchMode(p.AUDIO_STRETCH_MODE_VOICE);
            PlaybackParams q = p.setSpeed(25.f);
            assertSame(p, q);

            try { fail("got " + p.getAudioFallbackMode()); } catch (IllegalStateException e) {}
            assertEquals(p.AUDIO_STRETCH_MODE_VOICE, p.getAudioStretchMode());
            try { fail("got " + p.getPitch());             } catch (IllegalStateException e) {}
            assertEquals(25.f, p.getSpeed(), FLOAT_TOLERANCE);

            // allowDefaults should not change set values
            q = p.allowDefaults();
            assertSame(p, q);

            assertEquals(p.AUDIO_FALLBACK_MODE_DEFAULT, p.getAudioFallbackMode());
            assertEquals(p.AUDIO_STRETCH_MODE_VOICE, p.getAudioStretchMode());
            assertEquals(1.f, p.getPitch(), FLOAT_TOLERANCE);
            assertEquals(25.f, p.getSpeed(), FLOAT_TOLERANCE);
        }
    }

    public void testPlaybackParamsDescribeContents() {
        PlaybackParams p = new PlaybackParams();
        assertEquals(0, p.describeContents());
    }

    public void testPlaybackParamsWriteToParcel() {
        PlaybackParams p = new PlaybackParams();
        p.setAudioFallbackMode(PlaybackParams.AUDIO_FALLBACK_MODE_FAIL);
        p.setAudioStretchMode(PlaybackParams.AUDIO_STRETCH_MODE_VOICE);
        p.setPitch(.5f);
        p.setSpeed(.0001f);

        Parcel parcel = Parcel.obtain();
        p.writeToParcel(parcel, 0 /* flags */);
        parcel.setDataPosition(0);
        PlaybackParams q = PlaybackParams.CREATOR.createFromParcel(parcel);

        assertEquals(p.getAudioFallbackMode(), q.getAudioFallbackMode());
        assertEquals(p.getAudioStretchMode(), q.getAudioStretchMode());
        assertEquals(p.getPitch(), q.getPitch());
        assertEquals(p.getSpeed(), q.getSpeed());
        parcel.recycle();
    }

}
