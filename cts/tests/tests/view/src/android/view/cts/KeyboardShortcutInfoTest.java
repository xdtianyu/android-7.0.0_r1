package android.view.cts;

import android.graphics.drawable.Icon;
import android.os.Parcel;
import android.test.InstrumentationTestCase;
import android.view.KeyEvent;
import android.view.KeyboardShortcutInfo;

/**
 * Tests for {@link android.view.KeyboardShortcutInfo}.
 */
public class KeyboardShortcutInfoTest extends InstrumentationTestCase {
    private static final CharSequence TEST_LABEL = "Test Label";
    private static final char TEST_BASE_CHARACTER = 't';
    private static final int TEST_KEYCODE = KeyEvent.KEYCODE_T;
    private static final int TEST_MODIFIERS = KeyEvent.META_ALT_ON | KeyEvent.META_CTRL_ON;

    public void testCharacterConstructor() {
        KeyboardShortcutInfo info = new KeyboardShortcutInfo(
                TEST_LABEL, TEST_BASE_CHARACTER, TEST_MODIFIERS);
        assertNotNull(info);
        assertEquals(TEST_LABEL, info.getLabel());
        assertEquals(TEST_BASE_CHARACTER, info.getBaseCharacter());
        assertEquals(KeyEvent.KEYCODE_UNKNOWN, info.getKeycode());
        assertEquals(TEST_MODIFIERS, info.getModifiers());
        assertEquals(0, info.describeContents());
    }

    public void testKeycodeConstructor() {
        KeyboardShortcutInfo info = new KeyboardShortcutInfo(
                TEST_LABEL, TEST_KEYCODE, TEST_MODIFIERS);
        assertNotNull(info);
        assertEquals(TEST_LABEL, info.getLabel());
        assertEquals(Character.MIN_VALUE, info.getBaseCharacter());
        assertEquals(TEST_KEYCODE, info.getKeycode());
        assertEquals(TEST_MODIFIERS, info.getModifiers());
        assertEquals(0, info.describeContents());
    }

    public void testConstructorChecksBaseCharacter() {
        try {
            KeyboardShortcutInfo info = new KeyboardShortcutInfo(
                    TEST_LABEL, Character.MIN_VALUE, TEST_MODIFIERS);
        } catch (IllegalArgumentException expected) {
            return;
        }
        fail();
    }

    public void testConstructorChecksKeycode() {
        try {
            KeyboardShortcutInfo info = new KeyboardShortcutInfo(
                    TEST_LABEL, KeyEvent.KEYCODE_UNKNOWN - 1, TEST_MODIFIERS);
        } catch (IllegalArgumentException expected) {
            return;
        }
        fail();
    }

    public void testWriteToParcelAndReadCharacter() {
        Parcel dest = Parcel.obtain();
        KeyboardShortcutInfo info = new KeyboardShortcutInfo(
                TEST_LABEL, TEST_BASE_CHARACTER, TEST_MODIFIERS);
        info.writeToParcel(dest, 0);

        dest.setDataPosition(0);
        KeyboardShortcutInfo result = KeyboardShortcutInfo.CREATOR.createFromParcel(dest);

        assertEquals(TEST_LABEL, result.getLabel());
        assertEquals(TEST_BASE_CHARACTER, result.getBaseCharacter());
        assertEquals(KeyEvent.KEYCODE_UNKNOWN, result.getKeycode());
        assertEquals(TEST_MODIFIERS, result.getModifiers());
    }

    public void testWriteToParcelAndReadKeycode() {
        Parcel dest = Parcel.obtain();
        KeyboardShortcutInfo info = new KeyboardShortcutInfo(
                TEST_LABEL, TEST_KEYCODE, TEST_MODIFIERS);
        info.writeToParcel(dest, 0);

        dest.setDataPosition(0);
        KeyboardShortcutInfo result = KeyboardShortcutInfo.CREATOR.createFromParcel(dest);

        assertEquals(TEST_LABEL, result.getLabel());
        assertEquals(Character.MIN_VALUE, result.getBaseCharacter());
        assertEquals(TEST_KEYCODE, result.getKeycode());
        assertEquals(TEST_MODIFIERS, result.getModifiers());
    }
}
