package android.settings.functional;

import android.app.NotificationManager;
import android.content.ContentResolver;
import android.content.Context;
import android.os.Handler;
import android.os.SystemClock;
import android.provider.Settings;
import android.service.notification.ZenModeConfig;
import android.platform.test.helpers.SettingsHelperImpl;
import android.platform.test.helpers.SettingsHelperImpl.SettingsType;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.Until;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.Suppress;

import com.android.server.notification.ConditionProviders;
import com.android.server.notification.ManagedServices.UserProfiles;
import com.android.server.notification.ZenModeHelper;

public class SoundSettingsTest extends InstrumentationTestCase {
    private static final String PAGE = Settings.ACTION_SOUND_SETTINGS;
    private static final int TIMEOUT = 2000;

    private UiDevice mDevice;
    private ContentResolver mResolver;
    private SettingsHelperImpl mHelper;
    private ZenModeHelper mZenHelper;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mDevice = UiDevice.getInstance(getInstrumentation());
        mDevice.setOrientationNatural();
        mResolver = getInstrumentation().getContext().getContentResolver();
        mHelper = new SettingsHelperImpl(getInstrumentation());
        ConditionProviders cps = new ConditionProviders(
                getInstrumentation().getContext(), new Handler(), new UserProfiles());
        mZenHelper = new ZenModeHelper(getInstrumentation().getContext(),
                getInstrumentation().getContext().getMainLooper(),
                cps);
    }

    @Override
    public void tearDown() throws Exception {
        mDevice.unfreezeRotation();
        super.tearDown();
    }

    @MediumTest
    public void testCallVibrate() throws Exception {
        assertTrue(mHelper.verifyToggleSetting(SettingsType.SYSTEM, PAGE,
                "Also vibrate for calls", Settings.System.VIBRATE_WHEN_RINGING));
        assertTrue(mHelper.verifyToggleSetting(SettingsType.SYSTEM, PAGE,
                "Also vibrate for calls", Settings.System.VIBRATE_WHEN_RINGING));
    }

    @MediumTest
    public void testOtherSounds() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        mHelper.scrollVert(false);
        Thread.sleep(1000);
        mHelper.clickSetting("Other sounds");
        Thread.sleep(1000);
        try {
            assertTrue("Dial pad tones not toggled", mHelper.verifyToggleSetting(
                    SettingsType.SYSTEM, PAGE, "Dial pad tones",
                    Settings.System.DTMF_TONE_WHEN_DIALING));
            assertTrue("Screen locking sounds not toggled",
                    mHelper.verifyToggleSetting(SettingsType.SYSTEM, PAGE,
                    "Screen locking sounds", Settings.System.LOCKSCREEN_SOUNDS_ENABLED));
            assertTrue("Charging sounds not toggled",
                    mHelper.verifyToggleSetting(SettingsType.GLOBAL, PAGE,
                    "Charging sounds", Settings.Global.CHARGING_SOUNDS_ENABLED));
            assertTrue("Touch sounds not toggled",
                    mHelper.verifyToggleSetting(SettingsType.SYSTEM, PAGE,
                    "Touch sounds", Settings.System.SOUND_EFFECTS_ENABLED));
            assertTrue("Vibrate on tap not toggled",
                    mHelper.verifyToggleSetting(SettingsType.SYSTEM, PAGE,
                    "Vibrate on tap", Settings.System.HAPTIC_FEEDBACK_ENABLED));
        } finally {
            mDevice.pressBack();
        }
    }

    @MediumTest
    @Suppress
    public void testDndPriorityAllows() throws Exception {
        SettingsHelperImpl.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        Context ctx = getInstrumentation().getContext();
        try {
            mHelper.clickSetting("Do not disturb");
            try {
                mHelper.clickSetting("Priority only allows");
                ZenModeConfig baseZenCfg = mZenHelper.getConfig();

                mHelper.clickSetting("Reminders");
                mHelper.clickSetting("Events");
                mHelper.clickSetting("Repeat callers");

                ZenModeConfig changedCfg = mZenHelper.getConfig();
                assertFalse(baseZenCfg.allowReminders == changedCfg.allowReminders);
                assertFalse(baseZenCfg.allowEvents == changedCfg.allowEvents);
                assertFalse(baseZenCfg.allowRepeatCallers == changedCfg.allowRepeatCallers);

                mHelper.clickSetting("Reminders");
                mHelper.clickSetting("Events");
                mHelper.clickSetting("Repeat callers");

                changedCfg = mZenHelper.getConfig();
                assertTrue(baseZenCfg.allowReminders == changedCfg.allowReminders);
                assertTrue(baseZenCfg.allowEvents == changedCfg.allowEvents);
                assertTrue(baseZenCfg.allowRepeatCallers == changedCfg.allowRepeatCallers);

                mHelper.clickSetting("Messages");
                mHelper.clickSetting("From anyone");
                mHelper.clickSetting("Calls");
                mHelper.clickSetting("From anyone");

                changedCfg = mZenHelper.getConfig();
                assertFalse(baseZenCfg.allowCallsFrom == changedCfg.allowCallsFrom);
                assertFalse(baseZenCfg.allowMessagesFrom == changedCfg.allowMessagesFrom);
            } finally {
                mDevice.pressBack();
            }
        } finally {
            mDevice.pressHome();
        }
    }

    @MediumTest
    @Suppress
    public void testDndVisualInterruptions() throws Exception {
        SettingsHelper.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        try {
            mHelper.clickSetting("Do not disturb");
            try {
                mHelper.clickSetting("Visual interruptions");
                ZenModeConfig baseZenCfg = mZenHelper.getConfig();

                mHelper.clickSetting("Block when screen is on");
                mHelper.clickSetting("Block when screen is off");

                ZenModeConfig changedCfg = mZenHelper.getConfig();
                assertFalse(baseZenCfg.allowWhenScreenOff == changedCfg.allowWhenScreenOff);
                assertFalse(baseZenCfg.allowWhenScreenOn == changedCfg.allowWhenScreenOn);

                mHelper.clickSetting("Block when screen is on");
                mHelper.clickSetting("Block when screen is off");

                changedCfg = mZenHelper.getConfig();
                assertTrue(baseZenCfg.allowWhenScreenOff == changedCfg.allowWhenScreenOff);
                assertTrue(baseZenCfg.allowWhenScreenOn == changedCfg.allowWhenScreenOn);
            } finally {
                mDevice.pressBack();
            }
        } finally {
            mDevice.pressBack();
        }
    }

    /*
     * Rather than verifying every ringtone, verify the ones least likely to change
     * (None and Hangouts) and an arbitrary one from the ringtone pool.
     */
    @MediumTest
    public void testPhoneRingtoneNone() throws Exception {
        SettingsHelper.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        mHelper.clickSetting("Phone ringtone");
        verifyRingtone(new RingtoneSetting("None", "null"),
                Settings.System.RINGTONE, ScrollDir.UP);
    }

    @MediumTest
    @Suppress
    public void testPhoneRingtoneHangouts() throws Exception {
        SettingsHelper.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        mHelper.clickSetting("Phone ringtone");
        verifyRingtone(new RingtoneSetting("Hangouts Call", "31"), Settings.System.RINGTONE);
    }

    @MediumTest
    public void testPhoneRingtoneUmbriel() throws Exception {
        SettingsHelper.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        mHelper.clickSetting("Phone ringtone");
        verifyRingtone(new RingtoneSetting("Umbriel", "49"),
                Settings.System.RINGTONE, ScrollDir.DOWN);
    }

    @MediumTest
    public void testNotificationRingtoneNone() throws Exception {
        SettingsHelper.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        mHelper.clickSetting("Default notification ringtone");
        verifyRingtone(new RingtoneSetting("None", "null"),
                Settings.System.NOTIFICATION_SOUND, ScrollDir.UP);
    }

    @MediumTest
    @Suppress
    public void testNotificationRingtoneHangouts() throws Exception {
        SettingsHelper.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        mHelper.clickSetting("Default notification ringtone");
        verifyRingtone(new RingtoneSetting("Hangouts Message", "30"),
                Settings.System.NOTIFICATION_SOUND);
    }

    @MediumTest
    public void testNotificationRingtoneTitan() throws Exception {
        SettingsHelper.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        mHelper.clickSetting("Default notification ringtone");
        verifyRingtone(new RingtoneSetting("Titan", "35"),
                Settings.System.NOTIFICATION_SOUND, ScrollDir.DOWN);
    }

    @MediumTest
    public void testAlarmRingtoneNone() throws Exception {
        SettingsHelper.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        mHelper.clickSetting("Default alarm ringtone");
        verifyRingtone(new RingtoneSetting("None", "null"),
                Settings.System.ALARM_ALERT, ScrollDir.UP);
    }

    @MediumTest
    public void testAlarmRingtoneXenon() throws Exception {
        SettingsHelper.launchSettingsPage(getInstrumentation().getContext(), PAGE);
        mHelper.clickSetting("Default alarm ringtone");
        verifyRingtone(new RingtoneSetting("Xenon", "22"),
                Settings.System.ALARM_ALERT, ScrollDir.DOWN);
    }

    private void verifyRingtone(RingtoneSetting r, String settingName) {
        verifyRingtone(r, settingName, ScrollDir.NOSCROLL);
    }

    private void verifyRingtone(RingtoneSetting r, String settingName, ScrollDir dir) {
        if (dir != ScrollDir.NOSCROLL) {
            mHelper.scrollVert(dir == ScrollDir.UP);
            SystemClock.sleep(1000);
        }
        mDevice.wait(Until.findObject(By.text(r.getName())), TIMEOUT).click();
        mDevice.wait(Until.findObject(By.text("OK")), TIMEOUT).click();
        SystemClock.sleep(1000);
        if (r.getVal().equals("null")) {
            assertEquals(null,
                    Settings.System.getString(mResolver, settingName));
        } else if (r.getName().contains("Hangouts")) {
            assertEquals("content://media/external/audio/media/" + r.getVal(),
                    Settings.System.getString(mResolver, settingName));
        } else {
            assertEquals("content://media/internal/audio/media/" + r.getVal(),
                    Settings.System.getString(mResolver, settingName));
        }
    }

    private enum ScrollDir {
        UP,
        DOWN,
        NOSCROLL
    }

    class RingtoneSetting {
        private final String mName;
        private final String mMediaVal;
        public RingtoneSetting(String name, String fname) {
            mName = name;
            mMediaVal = fname;
        }
        public String getName() {
            return mName;
        }
        public String getVal() {
            return mMediaVal;
        }
    }
}
